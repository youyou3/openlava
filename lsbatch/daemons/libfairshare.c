/*
 * Copyright (C) 2014-2015 David Bigagli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 *
 */
#include "fairshare.h"

static struct tree_node_ *get_user_node(struct hash_tab *,
                                        struct jData *);

/* fs_init()
 */
int
fs_init(struct qData *qPtr, struct userConf *uConf)
{
    qPtr->fsSched->tree
        = sshare_make_tree(qPtr->fairshare,
                           (uint32_t )uConf->numUgroups,
                           (struct group_acct *)uConf->ugroups);

    if (qPtr->fsSched->tree == NULL) {
        ls_syslog(LOG_ERR, "\
%s: queues %s failed to fairshare configuration, fairshare disabled",
                  __func__, qPtr->queue);
        _free_(qPtr->fairshare);
        qPtr->qAttrib &= ~Q_ATTRIB_FAIRSHARE;
        return -1;
    }

    return 0;
}

/* fs_update_sacct()
 */
int
fs_update_sacct(struct qData *qPtr,
                struct jData *jPtr,
                int numJobs,
                int numPEND,
                int numRUN)
{
    struct tree_ *t;
    struct tree_node_ *n;
    struct share_acct *sacct;
    int numRAN;

    t = qPtr->fsSched->tree;

    n = get_user_node(t->node_tab, jPtr);
    if (n == NULL)
        return -1;

    sacct = n->data;
    sacct->uid = jPtr->userId;

    /* Hoard the running jobs
     */
    numRAN = 0;
    if (numRUN > 0)
        numRAN = numRUN;

    while (n) {
        sacct = n->data;
        sacct->numPEND = sacct->numPEND + numPEND;
        sacct->numRUN = sacct->numRUN + numRUN;
        sacct->numRAN = sacct->numRAN + numRAN;
        n = n->parent;
    }

    return 0;
}

/* fs_init_sched_session()
 */
int
fs_init_sched_session(struct qData *qPtr)
{
    struct tree_ *t;

    if (qPtr->numFairSlots == 0)
	return 0;

    t = qPtr->fsSched->tree;

    /* Distribute the tokens all the way
     * down the leafs
     */
    sshare_distribute_slots(t, qPtr->numFairSlots);

    return 0;
}

/* fs_elect_job()
 */
int
fs_elect_job(struct qData *qPtr,
             LIST_T *jRefList,
             struct jRef **jRef)
{
    struct tree_node_ *n;
    struct share_acct *s;
    link_t *l;
    struct jRef *jref;
    struct jData *jPtr;
    uint32_t sent;

    l = qPtr->fsSched->tree->leafs;
    if (LINK_NUM_ENTRIES(l) == 0) {
        *jRef = NULL;
        return -1;
    }

    /* pop() so if the num sent drops
     * to zero we remove it and never traverse
     * it again.
     */
    sent = 0;
    while ((n = pop_link(l))) {
        s = n->data;
        if (s->sent > 0) {
            s->sent--;
            ++sent;
            break;
        }
    }

    /* No more jobs to sent
     */
    if (sent == 0) {
        *jRef = NULL;
        return -1;
    }

    for (jref = (struct jRef *)jRefList->back;
         jref != (void *)jRefList;
         jref = jref->back) {

        jPtr = jref->job;

        /* end of the reference list or the job
         * ahead belongs to a different queue
         */
        if (jref->back == (void *)jRefList
            || jPtr->qPtr->queueId != jPtr->back->qPtr->queueId) {
            *jRef = jref;
            return 0;
        }
        /* fixme: handle user priority here
         */
        if (jPtr->userId == s->uid)
            break;
    }

    /* More to dispatch from this node
     * so back to the leaf link
     */
    if (s->sent > 0)
        push_link(l, n);

    *jRef = jref;

    return 0;
}

/* fs_fin_sched_session()
 */
int
fs_fin_sched_session(struct qData *qPtr)
{
    return 0;
}

/* fs_get_saccts()
 *
 * Return an array of share accounts for a queue tree.
 * In other words flatten the tree.
 */
int
fs_get_saccts(struct qData *qPtr, int *num, struct share_acct ***as)
{
    struct tree_ *t;
    struct tree_node_ *n;
    struct share_acct **s;
    int nents;
    int i;

    t = qPtr->fsSched->tree;

    /* First let's count the number of nodes
     */
    n = t->root;
    nents = 0;
    while ((n = tree_next_node(n)))
        ++nents;

    s = calloc(nents, sizeof(struct share_acct *));
    assert(s);
    /* Now simply address the array elemens
     * to the nodes. Don't dup memory
     */
    i = 0;
    n = t->root;
    while ((n = tree_next_node(n))) {
        s[i] = n->data;
        ++i;
    }

    *as = s;
    *num = nents;

    return 0;
}

/* get_user_node()
 */
static struct tree_node_ *
get_user_node(struct hash_tab *node_tab,
              struct jData *jPtr)
{
    struct tree_node_ *n;
    struct tree_node_ *n2;
    struct share_acct *sacct;
    struct share_acct *sacct2;
    uint32_t sum;
    char key[MAXLSFNAMELEN];;

    if (jPtr->shared->jobBill.userGroup[0] != 0) {
        sprintf(key, "\
%s/%s", jPtr->shared->jobBill.userGroup, jPtr->userName);
    } else {
        sprintf(key, "%s", jPtr->userName);
    }

    /* First direct lookup on the table
     * of nodes.
     */
    n = hash_lookup(node_tab, key);
    if (n)
        return n;

    /* If job specifies parent group lookup
     * all in parent group
     */
    if (jPtr->shared->jobBill.userGroup[0] != 0) {
        sprintf(key, "%s/all", jPtr->shared->jobBill.userGroup);
        n = hash_lookup(node_tab, key);
    } else {
        /* Job specifies no parent group
         * so lookup all in any group
         */
        n = hash_lookup(node_tab, "all");
    }

    if (n == NULL)
        return NULL;

    sacct = n->data;
    assert(sacct->options & SACCT_USER);

    n2 = calloc(1, sizeof(struct tree_node_));
    sacct2 = make_sacct(jPtr->userName, sacct->shares);
    sacct2->uid = jPtr->userId;
    n2->data = sacct2;
    n2->name = strdup(jPtr->userName);

    tree_insert_node(n->parent, n2);
    sacct2->options |= SACCT_USER;
    sprintf(key, "%s/%s", n2->parent->name, n2->name);
    hash_install(node_tab, key, n2, NULL);
    sprintf(key, "%s", n2->name);
    hash_install(node_tab, key, n2, NULL);

    sum = 0;
    n2 = n->parent->child;
    while (n2) {

        sacct = n2->data;
        if (sacct->options & SACCT_USER_ALL) {
            n2 = n2->right;
            continue;
        }
        sum = sum + sacct->shares;
        n2 = n2->right;
    }

    n2 = n->parent->child;
    while (n2) {

        sacct = n2->data;
        if (sacct->options & SACCT_USER_ALL) {
            n2 = n2->right;
            continue;
        }
        sacct->dshares = (double)sacct->shares/(double)sum;
        n2 = n2->right;
    }

    return n->parent->child;
}
