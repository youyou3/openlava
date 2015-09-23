/*
 * Copyright (C) 2015 David Bigagli
 * Copyright (C) 2007 Platform Computing Inc
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "../lsf.h"
#include "lib.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef LSFRU_FIELD_ADD
#define LSFRU_FIELD_ADD(a,b) \
{ \
    if ((a) < 0 || (b) < 0) { \
        (a) = MAX((a), (b)); \
    } else { \
        (a) += (b); \
    } \
}
#endif

void
ls_ruunix2lsf(struct rusage *rusage, struct lsfRusage *lsfRusage)
{
    lsfRusage->ru_utime = rusage->ru_utime.tv_sec
        + rusage->ru_utime.tv_usec / 1000000.0;

    lsfRusage->ru_stime = rusage->ru_stime.tv_sec
        + rusage->ru_stime.tv_usec / 1000000.0;

    lsfRusage->ru_maxrss = rusage->ru_maxrss;
    lsfRusage->ru_ixrss = rusage->ru_ixrss;

    lsfRusage->ru_ismrss = -1.0;

    lsfRusage->ru_isrss = rusage->ru_isrss;
    lsfRusage->ru_minflt = rusage->ru_minflt;
    lsfRusage->ru_majflt = rusage->ru_majflt;
    lsfRusage->ru_nswap = rusage->ru_nswap;
    lsfRusage->ru_inblock = rusage->ru_inblock;
    lsfRusage->ru_oublock = rusage->ru_oublock;

    lsfRusage->ru_ioch = -1.0;

    lsfRusage->ru_idrss = rusage->ru_idrss;
    lsfRusage->ru_msgsnd = rusage->ru_msgsnd;
    lsfRusage->ru_msgrcv = rusage->ru_msgrcv;
    lsfRusage->ru_nsignals = rusage->ru_nsignals;
    lsfRusage->ru_nvcsw = rusage->ru_nvcsw;
    lsfRusage->ru_nivcsw = rusage->ru_nivcsw;
    lsfRusage->ru_exutime = -1.0;


}

void
ls_rulsf2unix(struct lsfRusage *lsfRusage, struct rusage *rusage)
{
    rusage->ru_utime.tv_sec = MIN( lsfRusage->ru_utime, LONG_MAX );
    rusage->ru_stime.tv_sec = MIN( lsfRusage->ru_stime, LONG_MAX );

    rusage->ru_utime.tv_usec = MIN(( lsfRusage->ru_utime
				- rusage->ru_utime.tv_sec) * 1000000, LONG_MAX);
    rusage->ru_stime.tv_usec = MIN(( lsfRusage->ru_stime
				- rusage->ru_stime.tv_sec) * 1000000, LONG_MAX);

    rusage->ru_maxrss = MIN( lsfRusage->ru_maxrss, LONG_MAX );
    rusage->ru_ixrss = MIN( lsfRusage->ru_ixrss, LONG_MAX );
    rusage->ru_isrss = MIN( lsfRusage->ru_isrss, LONG_MAX );

    rusage->ru_idrss = MIN( lsfRusage->ru_idrss, LONG_MAX );
    rusage->ru_isrss = MIN( lsfRusage->ru_isrss, LONG_MAX );
    rusage->ru_minflt = MIN( lsfRusage->ru_minflt, LONG_MAX );
    rusage->ru_majflt = MIN( lsfRusage->ru_majflt, LONG_MAX );
    rusage->ru_nswap = MIN( lsfRusage->ru_nswap, LONG_MAX );
    rusage->ru_inblock = MIN( lsfRusage->ru_inblock, LONG_MAX );
    rusage->ru_oublock = MIN( lsfRusage->ru_oublock, LONG_MAX );

    rusage->ru_msgsnd = MIN( lsfRusage->ru_msgsnd, LONG_MAX );
    rusage->ru_msgrcv = MIN( lsfRusage->ru_msgrcv, LONG_MAX );
    rusage->ru_nsignals = MIN( lsfRusage->ru_nsignals, LONG_MAX );
    rusage->ru_nvcsw = MIN( lsfRusage->ru_nvcsw, LONG_MAX );
    rusage->ru_nivcsw = MIN( lsfRusage->ru_nivcsw, LONG_MAX );

}

int
lsfRu2Str(FILE *log_fp, struct lsfRusage *lsfRu)
{
    return (fprintf(log_fp, " %1.6f %1.6f %1.0f %1.0f %1.0f %1.0f \
%1.0f %1.0f %1.0f %1.0f %1.0f %1.0f %1.0f %1.0f %1.0f %1.0f \
%1.0f %1.0f %1.0f", lsfRu->ru_utime, lsfRu->ru_stime, lsfRu->ru_maxrss,
        lsfRu->ru_ixrss, lsfRu->ru_ismrss, lsfRu->ru_idrss, lsfRu->ru_isrss,
        lsfRu->ru_minflt, lsfRu->ru_majflt, lsfRu->ru_nswap, lsfRu->ru_inblock,
        lsfRu->ru_oublock, lsfRu->ru_ioch, lsfRu->ru_msgsnd, lsfRu->ru_msgrcv,
        lsfRu->ru_nsignals, lsfRu->ru_nvcsw, lsfRu->ru_nivcsw,
        lsfRu->ru_exutime));

}

int
str2lsfRu(char *line, struct lsfRusage *lsfRu, int *ccount)
{
    int cc;

    cc = sscanf(line, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf \
%lf %lf %lf%n", &(lsfRu->ru_utime), &(lsfRu->ru_stime), &(lsfRu->ru_maxrss),
        &(lsfRu->ru_ixrss), &(lsfRu->ru_ismrss), &(lsfRu->ru_idrss),
        &(lsfRu->ru_isrss), &(lsfRu->ru_minflt), &(lsfRu->ru_majflt),
        &(lsfRu->ru_nswap), &(lsfRu->ru_inblock), &(lsfRu->ru_oublock),
        &(lsfRu->ru_ioch), &(lsfRu->ru_msgsnd), &(lsfRu->ru_msgrcv),
        &(lsfRu->ru_nsignals), &(lsfRu->ru_nvcsw), &(lsfRu->ru_nivcsw),
        &(lsfRu->ru_exutime), ccount);
    return cc;
}


void
lsfRusageAdd_ (struct lsfRusage *lsfRusage1, struct lsfRusage *lsfRusage2)
{
    LSFRU_FIELD_ADD(lsfRusage1->ru_utime, lsfRusage2->ru_utime);
    LSFRU_FIELD_ADD(lsfRusage1->ru_stime, lsfRusage2->ru_stime);
    LSFRU_FIELD_ADD(lsfRusage1->ru_maxrss,lsfRusage2->ru_maxrss);
    LSFRU_FIELD_ADD(lsfRusage1->ru_ixrss, lsfRusage2->ru_ixrss);
    LSFRU_FIELD_ADD(lsfRusage1->ru_ismrss, lsfRusage2->ru_ismrss);
    LSFRU_FIELD_ADD(lsfRusage1->ru_idrss, lsfRusage2->ru_idrss);
    LSFRU_FIELD_ADD(lsfRusage1->ru_isrss, lsfRusage2->ru_isrss);
    LSFRU_FIELD_ADD(lsfRusage1->ru_minflt, lsfRusage2->ru_minflt);
    LSFRU_FIELD_ADD(lsfRusage1->ru_majflt, lsfRusage2->ru_majflt);
    LSFRU_FIELD_ADD(lsfRusage1->ru_nswap, lsfRusage2->ru_nswap);
    LSFRU_FIELD_ADD(lsfRusage1->ru_inblock, lsfRusage2->ru_inblock);
    LSFRU_FIELD_ADD(lsfRusage1->ru_oublock, lsfRusage2->ru_oublock);
    LSFRU_FIELD_ADD(lsfRusage1->ru_ioch, lsfRusage2->ru_ioch);
    LSFRU_FIELD_ADD(lsfRusage1->ru_msgsnd, lsfRusage2->ru_msgsnd);
    LSFRU_FIELD_ADD(lsfRusage1->ru_msgrcv, lsfRusage2->ru_msgrcv);
    LSFRU_FIELD_ADD(lsfRusage1->ru_nsignals, lsfRusage2->ru_nsignals);
    LSFRU_FIELD_ADD(lsfRusage1->ru_nvcsw, lsfRusage2->ru_nvcsw);
    LSFRU_FIELD_ADD(lsfRusage1->ru_nivcsw, lsfRusage2->ru_nivcsw);
    LSFRU_FIELD_ADD(lsfRusage1->ru_exutime, lsfRusage2->ru_exutime);

}

void
cleanLsfRusage (struct lsfRusage *lsfRusage)
{
    lsfRusage->ru_utime = -1.0;
    lsfRusage->ru_stime = -1.0;
    lsfRusage->ru_maxrss = -1.0;
    lsfRusage->ru_ixrss = -1.0;
    lsfRusage->ru_ismrss = -1.0;
    lsfRusage->ru_idrss = -1.0;
    lsfRusage->ru_isrss = -1.0;
    lsfRusage->ru_minflt = -1.0;
    lsfRusage->ru_majflt = -1.0;
    lsfRusage->ru_nswap = -1.0;
    lsfRusage->ru_inblock = -1.0;
    lsfRusage->ru_oublock = -1.0;
    lsfRusage->ru_ioch = -1.0;
    lsfRusage->ru_msgsnd = -1.0;
    lsfRusage->ru_msgrcv = -1.0;
    lsfRusage->ru_nsignals = -1.0;
    lsfRusage->ru_nvcsw = -1.0;
    lsfRusage->ru_nivcsw = -1.0;
    lsfRusage->ru_exutime = -1.0;

}

void
cleanRusage (struct rusage *rusage)
{
    rusage->ru_utime.tv_sec = -1;
    rusage->ru_utime.tv_usec = -1;

    rusage->ru_stime.tv_sec = -1;
    rusage->ru_stime.tv_usec = -1;

    rusage->ru_maxrss = -1;
    rusage->ru_ixrss = -1;

    rusage->ru_isrss = -1;
    rusage->ru_minflt = -1;
    rusage->ru_majflt = -1;
    rusage->ru_nswap = -1;
    rusage->ru_inblock = -1;
    rusage->ru_oublock = -1;

    rusage->ru_idrss = -1;
    rusage->ru_msgsnd = -1;
    rusage->ru_msgrcv = -1;
    rusage->ru_nsignals = -1;
    rusage->ru_nvcsw = -1;
    rusage->ru_nivcsw = -1;
}

/* ls_time()
 *
 * Whoever wrote ctime()....
 */
char *
ls_time(time_t t)
{
    static char buf[64];

    sprintf(buf, "%.15s ", ctime(&t) + 4);

    return buf;
}

/* Here come the functions related to cgroup management
 */

/* ls_get_numcpus()
 */
int
ls_get_numcpus(void)
{
    int l;
    int n;
    FILE *fp;
    char buf[128];

    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL)
        return -1;

    l = strlen("processor");
    n = 0;
    while ((fscanf(fp, "%s", buf)) != EOF) {
        if (strncmp(buf, "processor", l) == 0)
            ++n;
    }

    if (n == 0)
        return 0;

    fclose(fp);
    return n;
}

/* ls_check_mount()
 */
bool_t
ls_check_mount(const char *path)
{
    FILE *fp;
    int l;
    char buf[PATH_MAX];

    if (path == NULL)
        return false;

    fp = fopen("/proc/self/mountinfo", "r");
    if (fp == NULL) {
        return false;
    }

    l = strlen(path);
    while ((fscanf(fp, "%*s%*s%*s%*s%s", buf)) != EOF) {
        if (strncmp(buf, path, l) == 0) {
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}

/* ls_constrain_mem()
 *
 * Constrain the memory of the given pid
 */
int
ls_constrain_mem(int mem_limit, pid_t pid)
{
    FILE *fp;
    char *root;
    static char buf[PATH_MAX];

    root = genParams_[OL_CGROUP_ROOT].paramValue;
    if (root == NULL)
        return 0;

    sprintf(buf, "%s/memory/openlava", root);
    if (mkdir(buf, 0755) < 0 && errno != EEXIST)
        return false;

    sprintf(buf, "%s/memory/openlava/res", root);
    if (mkdir(buf, 0755) < 0 && errno != EEXIST)
        return false;

    sprintf(buf, "%s/memory/openlava/res/%d", root, pid);
    if (mkdir(buf, 0755) < 0 && errno != EEXIST)
        return false;

    sprintf(buf, "%s/memory/openlava/res/%d/memory.limit_in_bytes", root, pid);

    fp = fopen(buf, "a");
    if (fp == NULL)
        return -1;
    if (fprintf(fp, "%d\n", mem_limit) < 0) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    sprintf(buf, "%s/memory/openlava/res/%d/tasks", root, pid);

    fp = fopen(buf, "a");
    if (fp == NULL)
        return -1;

    if (fprintf(fp, "%d\n", pid) < 0) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    return 0;
}

/* lsb_constrain_mem()
 */
int
lsb_constrain_mem(const char *job_id, int mem_limit, pid_t pid)
{
    FILE *fp;
    static char buf[PATH_MAX];
    char *root;

    root = genParams_[OL_CGROUP_ROOT].paramValue;
    if (root == NULL)
        return 0;

    if (job_id == NULL)
        return -1;

    sprintf(buf, "%s/memory/openlava", root);
    if (mkdir(buf, 0755) < 0 && errno != EEXIST)
        return false;

    sprintf(buf, "%s/memory/openlava/%s", root, job_id);
    if (mkdir(buf, 0755) < 0 && errno != EEXIST)
        return false;

    sprintf(buf, "%s/memory/openlava/%s/%d", root, job_id, pid);
    if (mkdir(buf, 0755) < 0 && errno != EEXIST)
        return false;

    sprintf(buf, "\
%s/memory/openlava/%s/%d/memory.limit_in_bytes", root, job_id, pid);

    fp = fopen(buf, "a");
    if (fp == NULL)
        return -1;
    if (fprintf(fp, "%d\n", mem_limit) < 0) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    sprintf(buf, "%s/memory/openlava/%s/%d/tasks", root, job_id, pid);

    fp = fopen(buf, "a");
    if (fp == NULL)
        return -1;

    if (fprintf(fp, "%d\n", pid) < 0) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    return 0;
}


/* ls_rmcgroup_mem
 */
int
ls_rmcgroup_mem(pid_t pid)
{
    static char buf[PATH_MAX];
    char *root;

    root = genParams_[OL_CGROUP_ROOT].paramValue;
    if (root == NULL)
        return 0;

    sprintf(buf, "%s/memory/openlava/res/%d", root, pid);

    if (rmdir(buf) < 0)
        return -1;

    return 0;
}

/* lsb_rmcgroup_mem
 */
int
lsb_rmcgroup_mem(const char *job_id,  pid_t pid)
{
    static char buf[PATH_MAX];
    char *root;

    root = genParams_[OL_CGROUP_ROOT].paramValue;
    if (root == NULL)
        return 0;

    sprintf(buf, "%s/memory/openlava/%s/%d", root, job_id, pid);

    if (rmdir(buf) < 0)
        return -1;

    sprintf(buf, "%s/memory/openlava/%s", root, job_id);

    if (rmdir(buf) < 0)
        return -1;

    return 0;
}
