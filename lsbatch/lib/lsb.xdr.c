/*
 * Copyright (C) 2007 Platform Computing Inc
 * Copyright (C) 2015 David Bigagli
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

/*
 * Compatibility note:
 *
 * Note we do not support vN library talking
 * to vN-1 daemon. We support backward
 * comparibility where vN daemon talks
 * to vN-1 library.
 */

#include "lsb.h"
#include "../../lsf/intlib/sshare.h"

static int allocLoadIdx(float **, float **, int *, int);
static bool_t xdr_lsbSharedResourceInfo(XDR *,
                                        struct lsbSharedResourceInfo *,
                                        struct LSFHeader *);
static bool_t xdr_lsbShareResourceInstance(XDR *,
                                           struct lsbSharedResourceInstance *,
                                           struct LSFHeader *);
int  lsbSharedResConfigured_ = false;

bool_t
xdr_submitReq(XDR *xdrs, struct submitReq *submitReq, struct LSFHeader *hdr)
{

    int i, nLimits;
    static int numAskedHosts = 0;
    static char **askedHosts = NULL;

    if (xdrs->x_op == XDR_DECODE) {
        submitReq->fromHost[0] = '\0';
        submitReq->jobFile[0] = '\0';
        submitReq->inFile[0] = '\0';
        submitReq->outFile[0] = '\0';
        submitReq->errFile[0] = '\0';
        submitReq->inFileSpool[0] = '\0';
        submitReq->commandSpool[0] = '\0';
        submitReq->chkpntDir[0] = '\0';
        submitReq->hostSpec[0] = '\0';
        submitReq->cwd[0] = '\0';
        submitReq->subHomeDir[0] = '\0';

        FREEUP(submitReq->queue);
        FREEUP(submitReq->command);
        FREEUP(submitReq->jobName);
        FREEUP(submitReq->preExecCmd);
        FREEUP(submitReq->dependCond);
        FREEUP(submitReq->resReq);
        FREEUP(submitReq->mailUser);
        FREEUP(submitReq->projectName);
        FREEUP(submitReq->loginShell);
        FREEUP(submitReq->schedHostType);
        FREEUP(submitReq->userGroup);

        if (numAskedHosts > 0) {
            for (i = 0; i < numAskedHosts; i++)
                FREEUP (askedHosts[i]);
            FREEUP (askedHosts);
        }
        numAskedHosts = 0;
    }


    if (!(xdr_u_int(xdrs, (unsigned int *)&submitReq->options) &&
          xdr_var_string(xdrs, &submitReq->queue) &&
          xdr_var_string(xdrs, &submitReq->resReq) &&
          xdr_int(xdrs, &submitReq->numProcessors))) {
        return false;
    }

    if (!(xdr_string(xdrs, &submitReq->fromHost, MAXHOSTNAMELEN) &&
          xdr_var_string(xdrs, &submitReq->dependCond) &&
          xdr_var_string(xdrs, &submitReq->jobName) &&
          xdr_var_string(xdrs, &submitReq->command) &&
          xdr_string(xdrs, &submitReq->jobFile, MAXFILENAMELEN) &&
          xdr_string(xdrs, &submitReq->inFile, MAXFILENAMELEN) &&
          xdr_string(xdrs, &submitReq->outFile, MAXFILENAMELEN) &&
          xdr_string(xdrs, &submitReq->errFile, MAXFILENAMELEN) &&
          xdr_var_string(xdrs, &submitReq->preExecCmd) &&
          xdr_string(xdrs, &submitReq->hostSpec, MAXHOSTNAMELEN))) {
        return false;
    }

    if (!xdr_int(xdrs, &submitReq->numAskedHosts))
        return false;

    if (xdrs->x_op == XDR_DECODE && submitReq->numAskedHosts > 0) {
        submitReq->askedHosts = (char **)
            calloc(submitReq->numAskedHosts, sizeof (char *));
        if (submitReq->askedHosts == NULL)
            return false;
    }


    for (i = 0; i < submitReq->numAskedHosts;i++ ) {
        if (!xdr_var_string(xdrs, &submitReq->askedHosts[i])) {
            submitReq->numAskedHosts = i;
            goto Error0;
        }
    }

    nLimits = LSF_RLIM_NLIMITS;
    if (!xdr_int(xdrs, &nLimits))
        goto Error0;

    for (i = 0; i < nLimits; i++) {
        if (i < LSF_RLIM_NLIMITS) {
            if (!xdr_int(xdrs, &submitReq->rLimits[i]))
                goto Error0;
        } else {
            int j;

            if (!xdr_int(xdrs, &j))
                goto Error0;
        }
    }

    if (!(xdr_time_t(xdrs, &submitReq->submitTime) &&
          xdr_time_t(xdrs, &submitReq->beginTime) &&
          xdr_time_t(xdrs, &submitReq->termTime) &&
          xdr_int(xdrs,&submitReq->umask) &&
          xdr_int(xdrs,&submitReq->sigValue) &&
          xdr_int(xdrs,&submitReq->restartPid) &&
          xdr_time_t(xdrs,&submitReq->chkpntPeriod) &&
          xdr_string(xdrs, &submitReq->chkpntDir, MAXFILENAMELEN) &&
          xdr_string(xdrs, &submitReq->subHomeDir, MAXFILENAMELEN) &&
          xdr_string(xdrs, &submitReq->cwd, MAXFILENAMELEN))) {
        goto Error0;
    }

    if (!xdr_int(xdrs, &submitReq->nxf))
        goto Error0;

    if (xdrs->x_op == XDR_DECODE && submitReq->nxf > 0) {
        if ((submitReq->xf = (struct xFile *)
             calloc(submitReq->nxf, sizeof(struct xFile))) == NULL)
            goto Error0;
    }

    for (i = 0; i < submitReq->nxf; i++) {
        if (!xdr_arrayElement(xdrs, (char *) &(submitReq->xf[i]),
                              hdr, xdr_xFile))
            goto Error1;
    }

    if (!xdr_var_string(xdrs, &submitReq->mailUser))
        goto Error1;

    if (!xdr_var_string(xdrs, &submitReq->projectName))
        goto Error1;

    if (!xdr_int(xdrs, &submitReq->niosPort))
        goto Error1;
    if (!xdr_int(xdrs, &submitReq->maxNumProcessors))
        goto Error1;
    if (!xdr_var_string(xdrs, &submitReq->loginShell))
        goto Error1;
    if (!xdr_var_string(xdrs, &submitReq->schedHostType))
        goto Error1;

    if (xdrs->x_op == XDR_DECODE) {
        numAskedHosts = submitReq->numAskedHosts;
        askedHosts = submitReq->askedHosts;
    }

    if (!xdr_int(xdrs, &submitReq->options2))
        goto Error1;

    if (!(xdr_string(xdrs, &submitReq->inFileSpool, MAXFILENAMELEN)
          && xdr_string(xdrs, &submitReq->commandSpool, MAXFILENAMELEN))) {
        return false;
    }

    if (!xdr_int(xdrs, &submitReq->userPriority))
        goto Error1;

    /* From now on, OPENLAVA_XDR_VERSION 30 we handle
     * backward compatibility.
     */

    /* Library encode to daemon
     */
    if (xdrs->x_op == XDR_ENCODE
        && hdr->version >= 30) {
        if (!xdr_var_string(xdrs, &submitReq->userGroup))
            goto Error1;
    }

    /* Daemon decode from library
     */
    if (xdrs->x_op == XDR_DECODE) {
        if (hdr->version >= 30) {
            if (!xdr_var_string(xdrs, &submitReq->userGroup))
                goto Error1;
        } else {
            submitReq->userGroup = strdup("");
        }
    }

    return true;

Error1:
    if (xdrs->x_op == XDR_DECODE) {
        FREEUP(submitReq->xf);
        submitReq->xf = NULL;
    }

Error0:
    if (xdrs->x_op == XDR_DECODE) {
        for (i = 0; i < submitReq->numAskedHosts; i++)
            FREEUP(submitReq->askedHosts[i]);
        FREEUP(submitReq->askedHosts);
        submitReq->askedHosts = NULL;
        submitReq->numAskedHosts = 0;
    }

    return false;
}

bool_t
xdr_modifyReq(XDR *xdrs, struct  modifyReq *modifyReq, struct LSFHeader *hdr)
{
    int jobArrId, jobArrElemId;

    if (xdrs->x_op == XDR_DECODE) {

        FREEUP (modifyReq->jobIdStr);
    }

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(modifyReq->jobId, &jobArrId, &jobArrElemId);
    }
    if (!(xdr_int(xdrs, &jobArrId) &&
          xdr_int(xdrs, &(modifyReq->delOptions))))
        return false;

    if (!xdr_arrayElement(xdrs, (char *) &modifyReq->submitReq,
                          hdr, xdr_submitReq))
        return false;

    if (!xdr_var_string(xdrs, &(modifyReq->jobIdStr)))
        return false;

    if (!(xdr_int(xdrs, &(modifyReq->delOptions2)))) {
        return false;
    }
    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }
    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&modifyReq->jobId,jobArrId,jobArrElemId);
    }

    return true;

}

bool_t
xdr_jobInfoReq(XDR *xdrs, struct jobInfoReq *jobInfoReq, struct LSFHeader *hdr)
{
    int jobArrId, jobArrElemId;

    if (!xdr_var_string(xdrs, &jobInfoReq->userName))
        return false;

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(jobInfoReq->jobId, &jobArrId, &jobArrElemId);
    }
    if (!(xdr_int(xdrs, &jobArrId)     &&
          xdr_int(xdrs, &(jobInfoReq->options)) &&
          xdr_var_string(xdrs, &jobInfoReq->queue)))
        return false;

    if (!xdr_var_string(xdrs, &jobInfoReq->host))
        return false;
    if (!xdr_var_string(xdrs, &jobInfoReq->jobName))
        return false;
    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }
    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&jobInfoReq->jobId,jobArrId,jobArrElemId);
    }

    return true;
}

bool_t
xdr_signalReq(XDR *xdrs, struct signalReq *signalReq, struct LSFHeader *hdr)
{
    int jobArrId, jobArrElemId;

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(signalReq->jobId, &jobArrId, &jobArrElemId);
    }
    if (!(xdr_int(xdrs, &(signalReq->sigValue)) &&
          xdr_int(xdrs, &jobArrId)))
        return false;


    if (signalReq->sigValue == SIG_CHKPNT
        || signalReq->sigValue == SIG_DELETE_JOB
        || signalReq->sigValue == SIG_ARRAY_REQUEUE) {
        if (!(xdr_time_t(xdrs, &(signalReq->chkPeriod)) &&
              xdr_int(xdrs, &(signalReq->actFlags)))) {
            return false;
        }
    }

    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&signalReq->jobId,jobArrId,jobArrElemId);
    }

    return true;
}

bool_t
xdr_submitMbdReply(XDR *xdrs, struct submitMbdReply *reply, struct LSFHeader *hdr)
{
    static char queueName[MAX_LSB_NAME_LEN];
    static char jobName[MAX_CMD_DESC_LEN];
    int jobArrId, jobArrElemId;

    if (xdrs->x_op == XDR_DECODE) {
        queueName[0] = '\0';
        jobName[0] = '\0';
        reply->queue = queueName;
        reply->badJobName = jobName;
    }

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(reply->jobId, &jobArrId, &jobArrElemId);
    }
    if (!(xdr_int(xdrs,&(jobArrId)) &&
          xdr_int(xdrs,&(reply->badReqIndx)) &&
          xdr_string(xdrs, &reply->queue, MAX_LSB_NAME_LEN) &&
          xdr_string(xdrs, &reply->badJobName, MAX_CMD_DESC_LEN)))
        return false;

    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&reply->jobId,jobArrId,jobArrElemId);
    }
    return true;
}

bool_t
xdr_parameterInfo(XDR *xdrs,
                  struct parameterInfo *paramInfo,
                  struct LSFHeader *hdr)
{

    if (xdrs->x_op == XDR_DECODE) {
        FREEUP(paramInfo->defaultQueues);
        FREEUP(paramInfo->defaultHostSpec);
        FREEUP(paramInfo->defaultProject);
        FREEUP(paramInfo->pjobSpoolDir);
    }

    if (!(xdr_var_string(xdrs, &paramInfo->defaultQueues) &&
          xdr_var_string(xdrs, &paramInfo->defaultHostSpec)))
        return false;

    if (!(xdr_int(xdrs, &paramInfo->mbatchdInterval) &&
          xdr_int(xdrs, &paramInfo->sbatchdInterval) &&
          xdr_int(xdrs, &paramInfo->jobAcceptInterval) &&
          xdr_int(xdrs, &paramInfo->maxDispRetries) &&
          xdr_int(xdrs, &paramInfo->maxSbdRetries) &&
          xdr_int(xdrs, &paramInfo->cleanPeriod) &&
          xdr_int(xdrs, &paramInfo->maxNumJobs)) &&
          xdr_int(xdrs, &paramInfo->pgSuspendIt))
        return false;

    if (!(xdr_var_string(xdrs, &paramInfo->defaultProject)))
        return false;

    if (!(xdr_int(xdrs,&(paramInfo->maxJobArraySize)))) {
        return false;
    }

    if (!xdr_int(xdrs,&(paramInfo->jobTerminateInterval)))
        return false;


    if (! xdr_bool(xdrs, &paramInfo->disableUAcctMap))
        return false;

    if (!(xdr_int(xdrs, &paramInfo->maxJobArraySize)))
        return false;


    if (!(xdr_var_string(xdrs, &paramInfo->pjobSpoolDir))) {
        return false;
    }

    if (!(xdr_int(xdrs, &paramInfo->maxUserPriority) &&
          xdr_int(xdrs, &paramInfo->jobPriorityValue) &&
          xdr_int(xdrs, &paramInfo->jobPriorityTime))) {
        return false;
    }


    if (!xdr_int(xdrs, &(paramInfo->maxJobId))) {
        return false;
    }

    if (!(xdr_int(xdrs, &paramInfo->maxAcctArchiveNum) &&
          xdr_int(xdrs, &paramInfo->acctArchiveInDays) &&
          xdr_int(xdrs, &paramInfo->acctArchiveInSize))){
        return false;
    }

    if (!(xdr_int(xdrs, &paramInfo->jobDepLastSub) &&
          xdr_int(xdrs, &paramInfo->sharedResourceUpdFactor))) {
        return false;
    }

    if (hdr->version >= 30) {
        if (! xdr_int(xdrs, &paramInfo->maxPreemptJobs))
            return false;
    }

    if (hdr->version >= 31) {
        if (! xdr_int(xdrs, &paramInfo->maxStreamRecords)
            || ! xdr_int(xdrs, &paramInfo->freshPeriod))
            return false;
    }

    return true;
}

bool_t
xdr_jobInfoHead(XDR *xdrs, struct jobInfoHead *jobInfoHead,
                struct LSFHeader *hdr)
{
    static char **hostNames = NULL;
    static int numJobs = 0, numHosts = 0;
    static LS_LONG_INT *jobIds = NULL;
    char *sp;
    int *jobArrIds = NULL, *jobArrElemIds = NULL;
    int i;

    if (!(xdr_int(xdrs, &(jobInfoHead->numJobs)) &&
          xdr_int(xdrs, &(jobInfoHead->numHosts)))) {
        return false;
    }
    if ( jobInfoHead->numJobs > 0) {
        if ((jobArrIds = (int *) calloc(jobInfoHead->numJobs,
                                        sizeof(int))) == NULL) {
            lsberrno = LSBE_NO_MEM;
            return false;
        }
        if ((jobArrElemIds = (int *) calloc(jobInfoHead->numJobs,
                                            sizeof(int))) == NULL) {
            lsberrno = LSBE_NO_MEM;
            FREEUP(jobArrIds);
            return false;
        }
    }
    if (xdrs->x_op == XDR_DECODE) {
        if (jobInfoHead->numJobs > numJobs ) {
            FREEUP (jobIds);
            numJobs = 0;
            if ((jobIds = (LS_LONG_INT *) calloc (jobInfoHead->numJobs,
                                                  sizeof(LS_LONG_INT))) == NULL) {
                lsberrno = LSBE_NO_MEM;
                return false;
            }
            numJobs = jobInfoHead->numJobs;
        }

        if (jobInfoHead->numHosts > numHosts) {
            for (i = 0; i < numHosts; i++)
                FREEUP (hostNames[i]);
            numHosts = 0;
            FREEUP (hostNames);
            if ((hostNames = (char **) calloc(jobInfoHead->numHosts,
                                              sizeof(char *))) == NULL) {
                lsberrno = LSBE_NO_MEM;
                return false;
            }
            for (i = 0; i < jobInfoHead->numHosts; i++) {
                hostNames[i] = malloc(MAXHOSTNAMELEN);
                if (!hostNames[i]) {
                    numHosts = i;
                    lsberrno = LSBE_NO_MEM;
                    return false;
                }
            }
            numHosts = jobInfoHead->numHosts;
        }
        jobInfoHead->jobIds = jobIds;
        jobInfoHead->hostNames = hostNames;
    }

    for (i = 0; i < jobInfoHead->numJobs; i++) {
        if (xdrs->x_op == XDR_ENCODE) {
            jobId64To32(jobInfoHead->jobIds[i],
                        &jobArrIds[i],
                        &jobArrElemIds[i]);
        }
        if (!xdr_int(xdrs,&(jobArrIds[i])))
            return false;
    }

    for (i = 0; i < jobInfoHead->numHosts; i++) {
        sp = jobInfoHead->hostNames[i];
        if (xdrs->x_op == XDR_DECODE)
            sp[0] = '\0';
        if (!(xdr_string(xdrs, &sp, MAXHOSTNAMELEN)))
            return false;
    }

    for (i = 0; i < jobInfoHead->numJobs; i++) {
        if (!xdr_int(xdrs,&jobArrElemIds[i])) {
            return false;
        }
        if (xdrs->x_op == XDR_DECODE) {
            jobId32To64(&jobInfoHead->jobIds[i],jobArrIds[i],jobArrElemIds[i]);
        }
    }

    FREEUP(jobArrIds);
    FREEUP(jobArrElemIds);
    return true;
}

bool_t
xdr_jgrpInfoReply(XDR *xdrs, struct jobInfoReply *jobInfoReply,
                  struct LSFHeader *hdr)
{
    char *sp;
    int jobArrId, jobArrElemId;

    sp = jobInfoReply->userName;
    if (xdrs->x_op == XDR_DECODE) {
        sp[0] = '\0';

        FREEUP( jobInfoReply->jobBill->dependCond );
        FREEUP( jobInfoReply->jobBill->jobName );
    }

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(jobInfoReply->jobId, &jobArrId, &jobArrElemId);
    }
    if (!(xdr_int(xdrs, &jobArrId) &&
          xdr_int(xdrs, (int *)&(jobInfoReply->status)) &&
          xdr_int(xdrs, &(jobInfoReply->reasons)) &&
          xdr_int(xdrs, &(jobInfoReply->subreasons)) &&
          xdr_time_t(xdrs, &(jobInfoReply->startTime)) &&
          xdr_time_t(xdrs, &(jobInfoReply->predictedStartTime)) &&
          xdr_time_t(xdrs, &(jobInfoReply->endTime)) &&
          xdr_float(xdrs, &(jobInfoReply->cpuTime)) &&
          xdr_int(xdrs, &(jobInfoReply->numToHosts)) &&
          xdr_int(xdrs, &jobInfoReply->userId) &&
          xdr_string(xdrs, &sp, MAXLSFNAMELEN))) {
        return false;
    }


    if (!(xdr_var_string(xdrs, &jobInfoReply->jobBill->dependCond) &&
          xdr_time_t(xdrs, &jobInfoReply->jobBill->submitTime) &&
          xdr_var_string(xdrs, &jobInfoReply->jobBill->jobName))) {
        FREEUP( jobInfoReply->jobBill->dependCond );
        FREEUP( jobInfoReply->jobBill->jobName );
        return false;
    }

    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&jobInfoReply->jobId,jobArrId,jobArrElemId);
    }
    return true;

}
bool_t
xdr_jobInfoReply(XDR *xdrs,
                 struct jobInfoReply *jobInfoReply,
                 struct LSFHeader *hdr)
{
    char *sp;
    int i, j;
    static float *loadSched = NULL, *loadStop = NULL;
    static int nIdx = 0;
    static int *reasonTb = NULL;
    static int nReasons = 0;
    int jobArrId, jobArrElemId;

    if (!(xdr_int(xdrs, (int *) &jobInfoReply->jType) &&
          xdr_var_string(xdrs, &jobInfoReply->parentGroup) &&
          xdr_var_string(xdrs, &jobInfoReply->jName))) {
        FREEUP(jobInfoReply->parentGroup);
        FREEUP(jobInfoReply->jName);
        return false;
    }

    for (i = 0; i < NUM_JGRP_COUNTERS; i++ ) {
        if (!xdr_int(xdrs, (int *) &jobInfoReply->counter[i])) {
            FREEUP(jobInfoReply->parentGroup);
            FREEUP(jobInfoReply->jName);
            return false;
        }
    }

    if (jobInfoReply->jType == JGRP_NODE_GROUP) {
        return(xdr_jgrpInfoReply(xdrs, jobInfoReply,hdr));
    }


    sp = jobInfoReply->userName;
    if (xdrs->x_op == XDR_DECODE)
        sp[0] = '\0';

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(jobInfoReply->jobId, &jobArrId, &jobArrElemId);
    }
    if (!(xdr_int(xdrs, &jobArrId) &&
          xdr_int(xdrs, (int *)&(jobInfoReply->status)) &&
          xdr_int(xdrs, &(jobInfoReply->reasons)) &&
          xdr_int(xdrs, &(jobInfoReply->subreasons)) &&
          xdr_time_t(xdrs, &(jobInfoReply->startTime)) &&
          xdr_time_t(xdrs, &(jobInfoReply->endTime)) &&
          xdr_float(xdrs, &(jobInfoReply->cpuTime)) &&
          xdr_int(xdrs, &(jobInfoReply->numToHosts)) &&
          xdr_int(xdrs, &jobInfoReply->nIdx) &&
          xdr_int(xdrs, &jobInfoReply->numReasons) &&
          xdr_int(xdrs, &jobInfoReply->userId) &&
          xdr_string(xdrs, &sp, MAXLSFNAMELEN))) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        if (jobInfoReply->nIdx > nIdx) {
            if (allocLoadIdx(&loadSched, &loadStop, &nIdx,
                             jobInfoReply->nIdx) == -1)
                return false;
        }
        jobInfoReply->loadSched = loadSched;
        jobInfoReply->loadStop = loadStop;

        if (jobInfoReply->numReasons > nReasons) {
            FREEUP (reasonTb);
            nReasons = 0;
            reasonTb = calloc (jobInfoReply->numReasons, sizeof(int));
            if (!reasonTb)
                return false;
            jobInfoReply->reasonTb = reasonTb;
            nReasons = jobInfoReply->numReasons;
        }
    }

    for (i = 0; i < jobInfoReply->nIdx; i++) {
        if (!xdr_float(xdrs, &jobInfoReply->loadSched[i]) ||
            !xdr_float(xdrs, &jobInfoReply->loadStop[i]))
            return false;
    }

    for (i = 0; i < jobInfoReply->numReasons; i++)
        if (!xdr_int(xdrs, &jobInfoReply->reasonTb[i]))
            return false;

    if (xdrs->x_op == XDR_DECODE && jobInfoReply->numToHosts > 0 ) {
        if ((jobInfoReply->toHosts = (char **)
             calloc(jobInfoReply->numToHosts, sizeof (char *))) == NULL) {
            jobInfoReply->numToHosts = 0;
            return false;
        }
    }
    for (i = 0; i < jobInfoReply->numToHosts; i++ ) {
        if (xdrs->x_op == XDR_DECODE) {

            jobInfoReply->toHosts[i] = calloc(1, MAXHOSTNAMELEN);
            if (jobInfoReply->toHosts[i] == NULL) {
                for (j = 0; j < i; j++)
                    free(jobInfoReply->toHosts[j]);
                free(jobInfoReply->toHosts);
                jobInfoReply->numToHosts = 0;
                jobInfoReply->toHosts = NULL;
                return false;
            }
        }
        sp = jobInfoReply->toHosts[i];
        if (!xdr_string(xdrs, &sp, MAXHOSTNAMELEN)) {
            if (xdrs->x_op == XDR_DECODE) {
                for (j=0; j<i; j++)
                    free(jobInfoReply->toHosts[j]);
                free(jobInfoReply->toHosts);
                jobInfoReply->numToHosts = 0;
                jobInfoReply->toHosts = NULL;
            }
            return false;
        }
    }

    if (!xdr_arrayElement(xdrs, (char *) (jobInfoReply->jobBill), hdr,
                          xdr_submitReq)) {
        if (xdrs->x_op == XDR_DECODE) {
            for (j = 0; j < jobInfoReply->numToHosts; j++)
                free(jobInfoReply->toHosts[j]);
            free(jobInfoReply->toHosts);
            jobInfoReply->numToHosts = 0;
            jobInfoReply->toHosts = NULL;
        }
        return false;
    }

    if (!(xdr_int(xdrs, (int *) &jobInfoReply->exitStatus) &&
          xdr_int(xdrs, (int *) &jobInfoReply->execUid) &&
          xdr_var_string(xdrs, &jobInfoReply->execHome) &&
          xdr_var_string(xdrs, &jobInfoReply->execCwd) &&
          xdr_var_string(xdrs, &jobInfoReply->execUsername))) {
        FREEUP( jobInfoReply->execHome );
        FREEUP( jobInfoReply->execCwd );
        FREEUP( jobInfoReply->execUsername );
        return false;
    }

    if (!xdr_time_t(xdrs, &jobInfoReply->reserveTime)
        || !xdr_int(xdrs, &jobInfoReply->jobPid))
        return false;


    if (!(xdr_time_t(xdrs, &(jobInfoReply->jRusageUpdateTime))))
        return false;
    if (!(xdr_jRusage(xdrs, &(jobInfoReply->runRusage), hdr)))
        return false;

    if (!xdr_time_t(xdrs, &(jobInfoReply->predictedStartTime))) {
        return false;
    }

    if (!xdr_u_short(xdrs, &jobInfoReply->port)) {
        return false;
    }

    if (!xdr_int(xdrs, &jobInfoReply->jobPriority)) {
        return false;
    }

    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&jobInfoReply->jobId,jobArrId, jobArrElemId);
    }

    return true;
}

bool_t
xdr_queueInfoReply(XDR *xdrs,
                   struct queueInfoReply *qInfoReply,
                   struct LSFHeader *hdr)
{
    int i;
    static int memSize;
    static struct queueInfoEnt *qInfo;
    static int nIdx;
    static float *loadStop;
    static float *loadSched;

    if (!(xdr_int(xdrs, &(qInfoReply->numQueues))
          && xdr_int(xdrs, &(qInfoReply->badQueue))
          && xdr_int(xdrs, &qInfoReply->nIdx)))
        return false;

    if (xdrs->x_op == XDR_DECODE) {

        if (qInfoReply->numQueues > memSize) {

            for (i = 0; i < memSize; i++) {
                FREEUP(qInfo[i].queue);
                FREEUP(qInfo[i].hostSpec);
            }
            FREEUP(qInfo);
            memSize = 0;
            if (!(qInfo = calloc(qInfoReply->numQueues,
                                 sizeof (struct queueInfoEnt))))
                return false;

            for (i = 0; i < qInfoReply->numQueues; i++) {
                qInfo[i].queue = NULL;
                qInfo[i].hostSpec = NULL;
                memSize = i + 1;
                if (!(qInfo[i].queue = malloc(MAX_LSB_NAME_LEN))
                    || !(qInfo[i].hostSpec = malloc(MAX_LSB_NAME_LEN)))
                    return false;
            }
        }


        for (i = 0; i < qInfoReply->numQueues; i++) {
            FREEUP(qInfo[i].description);
            FREEUP(qInfo[i].userList);
            FREEUP(qInfo[i].hostList);
            FREEUP(qInfo[i].windows);
            FREEUP(qInfo[i].windowsD);
            FREEUP(qInfo[i].defaultHostSpec);
            FREEUP(qInfo[i].admins);
            FREEUP(qInfo[i].preCmd);
            FREEUP(qInfo[i].postCmd);
            FREEUP(qInfo[i].prepostUsername);
            FREEUP(qInfo[i].requeueEValues);
            FREEUP(qInfo[i].resReq);
            FREEUP(qInfo[i].resumeCond);
            FREEUP(qInfo[i].stopCond);
            FREEUP(qInfo[i].jobStarter);
            FREEUP(qInfo[i].chkpntDir);
            FREEUP(qInfo[i].suspendActCmd);
            FREEUP(qInfo[i].resumeActCmd);
            FREEUP(qInfo[i].terminateActCmd);
            FREEUP(qInfo[i].preemption);
        }

        qInfoReply->queues = qInfo;

        if (qInfoReply->numQueues * qInfoReply->nIdx > nIdx
            && qInfoReply->numQueues > 0) {
            if (allocLoadIdx(&loadSched, &loadStop, &nIdx,
                             qInfoReply->numQueues * qInfoReply->nIdx) == -1)
                return false;
        }
    }

    for (i = 0; i < qInfoReply->numQueues; i++) {
        if (xdrs->x_op == XDR_DECODE) {
            qInfoReply->queues[i].loadSched
                = loadSched + (i * qInfoReply->nIdx);
            qInfoReply->queues[i].loadStop = loadStop + (i * qInfoReply->nIdx);
        }

        if (!xdr_arrayElement(xdrs,
                              (char *)&(qInfoReply->queues[i]),
                              hdr,
                              xdr_queueInfoEnt,
                              &qInfoReply->nIdx))
            return false;
    }

    return true;
}

/* xdr_queueInfoEnt()
 */
bool_t
xdr_queueInfoEnt(XDR *xdrs,
                 struct queueInfoEnt *qInfo,
                 struct LSFHeader *hdr,
                 int *nIdx)
{
    char *sp;
    int i;
    int j;

    if (xdrs->x_op == XDR_FREE) {
        if (qInfo->chkpntDir != 0) {
            FREEUP(qInfo->chkpntDir);
        }
        return true;
    }

    sp = qInfo->queue;
    if (xdrs->x_op == XDR_DECODE) {
        sp[0] = '\0';
        qInfo->suspendActCmd = NULL;
        qInfo->resumeActCmd = NULL;
        qInfo->terminateActCmd = NULL;
    }
    if (!(xdr_string(xdrs, &sp, MAX_LSB_NAME_LEN) &&
          xdr_var_string(xdrs, &qInfo->description)))
        return false;

    if (!(xdr_var_string(xdrs, &qInfo->userList)))
        return false;

    if (!(xdr_var_string(xdrs, &qInfo->hostList)))
        return false;

    if (!(xdr_var_string(xdrs, &qInfo->windows)))
        return false;

    sp = qInfo->hostSpec;
    if (xdrs->x_op == XDR_DECODE)
        sp[0] = '\0';

    if (!(xdr_string(xdrs, &sp, MAX_LSB_NAME_LEN)))
        return false;

    for(i = 0; i < LSF_RLIM_NLIMITS; i++) {
        if( !(xdr_int(xdrs, &qInfo->rLimits[i])))
            return false;
    }

    qInfo->nIdx = *nIdx;
    for (i = 0; i < *nIdx; i++) {
        if (! (xdr_float(xdrs, &(qInfo->loadSched[i])))
            || !(xdr_float(xdrs, &(qInfo->loadStop[i]))))
            return false;
    }

    if (!(xdr_int(xdrs, &qInfo->priority) &&
          xdr_short(xdrs, &qInfo->nice) &&
          xdr_int(xdrs, &qInfo->userJobLimit) &&
          xdr_float(xdrs, &qInfo->procJobLimit) &&
          xdr_int(xdrs, &qInfo->qAttrib) &&
          xdr_int(xdrs, &qInfo->qStatus) &&
          xdr_int(xdrs, &qInfo->maxJobs) &&
          xdr_int(xdrs, &qInfo->numJobs) &&
          xdr_int(xdrs, &qInfo->numPEND) &&
          xdr_int(xdrs, &qInfo->numRUN) &&
          xdr_int(xdrs, &qInfo->numSSUSP) &&
          xdr_int(xdrs, &qInfo->numUSUSP) &&
          xdr_int(xdrs, &qInfo->mig) &&
          xdr_int(xdrs, &qInfo->acceptIntvl) &&
          xdr_int(xdrs, &qInfo->schedDelay))) {
        return false;
    }

    if (!(xdr_var_string(xdrs, &qInfo->windowsD) &&
          xdr_var_string(xdrs, &qInfo->defaultHostSpec)))
        return false;

    if (!(xdr_int(xdrs, &qInfo->procLimit) &&
          xdr_var_string(xdrs, &qInfo->admins) &&
          xdr_var_string(xdrs, &qInfo->preCmd) &&
          xdr_var_string(xdrs, &qInfo->postCmd) &&
          xdr_var_string(xdrs, &qInfo->prepostUsername) &&
          xdr_var_string(xdrs, &qInfo->requeueEValues) &&
          xdr_int(xdrs, &qInfo->hostJobLimit)))
        return false;

    if (!(xdr_var_string(xdrs, &qInfo->resReq) &&
          xdr_int(xdrs, &qInfo->numRESERVE) &&
          xdr_int(xdrs, &qInfo->slotHoldTime) &&
          xdr_var_string(xdrs, &qInfo->resumeCond) &&
          xdr_var_string(xdrs, &qInfo->stopCond) &&
          xdr_var_string(xdrs, &qInfo->jobStarter) &&
          xdr_var_string(xdrs, &qInfo->suspendActCmd) &&
          xdr_var_string(xdrs, &qInfo->resumeActCmd) &&
          xdr_var_string(xdrs, &qInfo->terminateActCmd)))
        return false;

    for (j = 0; j < LSB_SIG_NUM ; j++) {
        if (!xdr_int(xdrs, &qInfo->sigMap[j]))
            return false;
    }

    if (!(xdr_var_string(xdrs, &qInfo->chkpntDir) &&
            xdr_int (xdrs, &qInfo->chkpntPeriod)))
        return false;

    for (i = 0; i < LSF_RLIM_NLIMITS; i++) {
        if (!(xdr_int(xdrs, &qInfo->defLimits[i])))
            return false;
    }

    if (!(xdr_int (xdrs, &qInfo->minProcLimit)
          && xdr_int(xdrs, &qInfo->defProcLimit))) {
        return false;
    }

    /* Handle fairshare data
     */
    if (! xdr_numShareAccts(xdrs, &qInfo->numAccts, &qInfo->saccts, hdr))
        return false;

    if (hdr->version >= 3) {
        if (! xdr_uint32_t(xdrs, &qInfo->numFairSlots))
            return false;
    }

    if (xdrs->x_op == XDR_ENCODE
        && hdr->version >= 3) {
        xdr_var_string(xdrs, &qInfo->preemption);
    }

    if (xdrs->x_op == XDR_DECODE
        && hdr->version >= 3) {
        xdr_var_string(xdrs, &qInfo->preemption);
    }

    return true;
}

/* xdr_numShareAccts()
 */
bool_t
xdr_numShareAccts(XDR *xdrs,
                  int *num,
                  struct share_acct ***s,
                  struct LSFHeader *hdr)
{
    int i;
    struct share_acct **s2;

    /* There must always be num in the stream.
     */
    if (!xdr_int(xdrs, num))
        return false;

    /* Encode fairshare data. If we are
     * encoding to < 3 don't care as
     * the library is not expecting
     * any fairshare data.
     */
    if (xdrs->x_op == XDR_ENCODE
        && hdr->version >= 3) {

        s2 = *s;
        for (i = 0; i < *num; i++) {
            if (! xdr_shareAcct(xdrs, s2[i], hdr))
                return false;
        }

        return true;
    }

    if (xdrs->x_op == XDR_DECODE
        && hdr->version >= 3) {

        if (*num == 0)
            s2 = NULL;
        else
            s2 = calloc(*num, sizeof(struct share_acct *));

        for (i = 0; i < *num; i++) {
            s2[i] = calloc(1, sizeof(struct share_acct));
            if (! xdr_shareAcct(xdrs, s2[i], hdr))
                return false;
        }
    }

    *s = s2;
    return true;
}

/* xdr_shareAcct()
 */
bool_t
xdr_shareAcct(XDR *xdrs, struct share_acct *s, struct LSFHeader *hdr)
{
    if (! xdr_var_string(xdrs, &s->name)
        || ! xdr_uint32_t(xdrs, &s->shares)
        || ! xdr_double(xdrs, &s->dshares)
        || ! xdr_int(xdrs, &s->numPEND)
        || ! xdr_int(xdrs, &s->numRUN)
        || ! xdr_uint32_t(xdrs, &s->options))
        return false;

    return true;
}

bool_t
xdr_infoReq(XDR *xdrs, struct infoReq *infoReq,
            struct LSFHeader *hdr)
{
    int i;
    static int memSize = 0;
    static char **names = NULL, *resReq = NULL;


    if (!(xdr_int(xdrs, &(infoReq->options))))
        return false;


    if (!(xdr_int(xdrs, &(infoReq->numNames))))
        return false;

    if (xdrs->x_op == XDR_DECODE) {

        if (names) {
            for (i = 0; i < memSize; i++)
                FREEUP (names[i]);
        }
        if (infoReq->numNames + 2 > memSize) {

            FREEUP (names);

            memSize = infoReq->numNames + 2;
            if ((names = (char **)calloc (memSize, sizeof(char *))) == NULL) {
                memSize = 0;
                return false;
            }
        }
        infoReq->names = names;
    }


    for (i = 0; i < infoReq->numNames; i++)
        if (!(xdr_var_string(xdrs, &infoReq->names[i])))
            return false;


    if (infoReq->options & CHECK_HOST) {
        if (!(xdr_var_string(xdrs, &infoReq->names[i])))
            return false;
        i++;
    }
    if (infoReq->options & CHECK_USER) {
        if (!(xdr_var_string(xdrs, &infoReq->names[i])))
            return false;
    }


    if (xdrs->x_op == XDR_DECODE) {
        FREEUP (resReq);
    }

    if (!xdr_var_string(xdrs, &infoReq->resReq))
        return false;

    if (xdrs->x_op == XDR_DECODE) {
        resReq = infoReq->resReq;
    }

    return true;

}

bool_t
xdr_hostDataReply(XDR *xdrs,
                  struct hostDataReply *hostDataReply,
                  struct LSFHeader *hdr)
{
    int i, hostCount;
    struct hostInfoEnt *hInfoTmp ;
    char *sp;
    static struct hostInfoEnt *hInfo = NULL;
    static int curNumHInfo = 0;
    static char *mem = NULL;
    static float *loadSched = NULL, *loadStop = NULL, *load = NULL;
    static float *realLoad = NULL;
    static int nIdx = 0, *busySched = NULL, *busyStop = NULL;

    if (! xdr_int(xdrs, &hostDataReply->numHosts)
        || !xdr_int(xdrs, &hostDataReply->badHost)
        || !xdr_int(xdrs, &hostDataReply->nIdx))
        return false;

    if (xdrs->x_op == XDR_DECODE) {
        hostCount = hostDataReply->numHosts;

        if (curNumHInfo < hostCount) {
            hInfoTmp = calloc(hostCount, sizeof(struct hostInfoEnt));
            if (hInfoTmp == NULL) {
                lsberrno = LSBE_NO_MEM;
                return false;
            }
            if (!(sp = malloc(hostCount
                              * (MAXHOSTNAMELEN + MAXLINELEN)))) {
                FREEUP(hInfoTmp);
                lsberrno = LSBE_NO_MEM;
                return false;
            }
            if (hInfo != NULL){
                FREEUP(mem);
                FREEUP(hInfo);
            }
            hInfo = hInfoTmp;
            curNumHInfo = hostCount;
            mem = sp;
            for (i = 0; i < hostCount; i++ ){
                hInfo[i].host = sp;
                sp += MAXHOSTNAMELEN;
                hInfo[i].windows = sp;
                sp += MAXLINELEN;
            }
        }
        hostDataReply->hosts = hInfo;

        if (hostDataReply->nIdx * hostDataReply->numHosts > nIdx) {
            if (allocLoadIdx(&loadSched,
                             &loadStop,
                             &nIdx,
                             hostDataReply->nIdx * hostDataReply->numHosts) == -1)
                return false;
        }
        FREEUP(load);
        FREEUP(realLoad);
        FREEUP(busySched);
        FREEUP(busyStop);

        if (hostDataReply->numHosts > 0
            && (realLoad = malloc(hostDataReply->nIdx
                                  * hostDataReply->numHosts * sizeof(float))) == NULL)
            return false;

        if (hostDataReply->numHosts > 0
            && (load = malloc(hostDataReply->nIdx
                              * hostDataReply->numHosts * sizeof(float))) == NULL)
            return false;
        if (hostDataReply->numHosts > 0
            && (busySched = malloc(GET_INTNUM (hostDataReply->nIdx)
                                   * hostDataReply->numHosts * sizeof(int))) == NULL)
            return false;
        if (hostDataReply->numHosts > 0
            && (busyStop = (int *) malloc(GET_INTNUM (hostDataReply->nIdx)
                                          * hostDataReply->numHosts * sizeof(int))) == NULL)
            return false;
    }

    for (i = 0; i < hostDataReply->numHosts; i++) {
        if (xdrs->x_op == XDR_DECODE) {
            hostDataReply->hosts[i].loadSched =
                loadSched + (i * hostDataReply->nIdx);
            hostDataReply->hosts[i].loadStop =
                loadStop + (i * hostDataReply->nIdx);
            hostDataReply->hosts[i].realLoad =
                realLoad + (i * hostDataReply->nIdx);
            hostDataReply->hosts[i].load =
                load + (i * hostDataReply->nIdx);
            hostDataReply->hosts[i].busySched =
                busySched + (i * GET_INTNUM (hostDataReply->nIdx));
            hostDataReply->hosts[i].busyStop =
                busyStop + (i * GET_INTNUM (hostDataReply->nIdx));
        }

        if (!xdr_arrayElement(xdrs,
                              (char *)&(hostDataReply->hosts[i]),
                              hdr,
                              xdr_hostInfoEnt,
                              (char *) &hostDataReply->nIdx))
            return false;
    }

    if (!xdr_int(xdrs, &(hostDataReply->flag))) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        if (hostDataReply->flag & LOAD_REPLY_SHARED_RESOURCE)
            lsbSharedResConfigured_ = true;
    }

    return true;
}

bool_t
xdr_hostInfoEnt(XDR *xdrs, struct hostInfoEnt *hostInfoEnt,
                struct LSFHeader *hdr, int *nIdx)
{
    char *sp = hostInfoEnt->host;
    char *wp = hostInfoEnt->windows;
    int i;

    if (xdrs->x_op == XDR_DECODE) {
        sp[0] = '\0';
        wp[0] = '\0';
    }

    if (!(xdr_string(xdrs, &sp, MAXHOSTNAMELEN) &&
          xdr_float(xdrs,&hostInfoEnt->cpuFactor) &&
          xdr_string(xdrs, &wp, MAXLINELEN) &&
          xdr_int(xdrs, &hostInfoEnt->userJobLimit) &&
          xdr_int(xdrs, &hostInfoEnt->maxJobs) &&
          xdr_int(xdrs, &hostInfoEnt->numJobs) &&
          xdr_int(xdrs, &hostInfoEnt->numRUN) &&
          xdr_int(xdrs, &hostInfoEnt->numSSUSP) &&
          xdr_int(xdrs, &hostInfoEnt->numUSUSP) &&
          xdr_int(xdrs, &hostInfoEnt->hStatus) &&
          xdr_int(xdrs, &hostInfoEnt->attr) &&
          xdr_int(xdrs, &hostInfoEnt->mig)))
        return false;

    hostInfoEnt->nIdx = *nIdx;

    for (i = 0; i < GET_INTNUM (*nIdx); i++) {
        if (!xdr_int(xdrs, &hostInfoEnt->busySched[i])
            || !xdr_int(xdrs, &hostInfoEnt->busyStop[i]))
            return false;
    }
    for (i = 0; i < *nIdx; i++) {
        if (!xdr_float(xdrs,&hostInfoEnt->loadSched[i]) ||
            !xdr_float(xdrs,&hostInfoEnt->loadStop[i]))
            return false;
    }

    for (i = 0; i < *nIdx; i++) {
        if (!xdr_float(xdrs,&hostInfoEnt->realLoad[i]))
            return false;
        if (!xdr_float(xdrs,&hostInfoEnt->load[i]))
            return false;
    }
    if (!xdr_int(xdrs, &hostInfoEnt->numRESERVE))
        return false;

    return true;
}

bool_t
xdr_userInfoReply(XDR *xdrs, struct userInfoReply *userInfoReply,
                  struct LSFHeader *hdr)
{
    int i;
    struct userInfoEnt *uInfoTmp;
    char *sp;

    static struct userInfoEnt *uInfo = NULL;
    static int curNumUInfo = 0;
    static char *mem = NULL;

    if (!(xdr_int(xdrs,&(userInfoReply->numUsers))
          && xdr_int(xdrs,&(userInfoReply->badUser))))
        return false;


    if (xdrs->x_op == XDR_DECODE) {
        if (curNumUInfo < userInfoReply->numUsers) {
            uInfoTmp = (struct userInfoEnt *)
                calloc(userInfoReply->numUsers,
                       sizeof(struct userInfoEnt));
            if (uInfoTmp == NULL) {
                lsberrno = LSBE_NO_MEM;
                return false;
            }
            if (!(sp = malloc(userInfoReply->numUsers * MAX_LSB_NAME_LEN))) {
                free(uInfoTmp);
                lsberrno = LSBE_NO_MEM;
                return false;
            }
            if (uInfo != NULL){
                free(mem);
                free(uInfo);
            }
            uInfo = uInfoTmp;
            curNumUInfo = userInfoReply->numUsers;
            mem = sp;
            for ( i = 0; i < userInfoReply->numUsers; i++ ){
                uInfo[i].user = sp;
                sp += MAX_LSB_NAME_LEN;
            }
        }
        userInfoReply->users = uInfo;
    }

    for (i = 0; i < userInfoReply->numUsers; i++ )
        if (!xdr_arrayElement(xdrs, (char *)&(userInfoReply->users[i]), hdr,
                              xdr_userInfoEnt)) {
            return false;
        }

    return true;

}

bool_t
xdr_userInfoEnt(XDR *xdrs, struct userInfoEnt *userInfoEnt,
                struct LSFHeader *hdr)
{
    char *sp;

    sp = userInfoEnt->user;
    if (xdrs->x_op == XDR_DECODE)
        sp[0] = '\0';

    if (!(xdr_string(xdrs, &sp, MAX_LSB_NAME_LEN) &&
          xdr_float(xdrs, &userInfoEnt->procJobLimit) &&
          xdr_int(xdrs, &userInfoEnt->maxJobs) &&
          xdr_int(xdrs, &userInfoEnt->numStartJobs)))

        return false;


    if (!(xdr_int(xdrs, &userInfoEnt->numJobs) &&
          xdr_int(xdrs, &userInfoEnt->numPEND) &&
          xdr_int(xdrs, &userInfoEnt->numRUN) &&
          xdr_int(xdrs, &userInfoEnt->numSSUSP) &&
          xdr_int(xdrs, &userInfoEnt->numUSUSP)))
        return false;

    if (!xdr_int(xdrs, &userInfoEnt->numRESERVE))
        return false;

    return true;
}


bool_t
xdr_jobPeekReq(XDR *xdrs, struct jobPeekReq *jobPeekReq, struct LSFHeader *hdr)
{

    int jobArrId, jobArrElemId;

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(jobPeekReq->jobId, &jobArrId, &jobArrElemId);
    }
    if (!xdr_int(xdrs, &jobArrId))
        return false;

    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&jobPeekReq->jobId,jobArrId,jobArrElemId);
    }

    return true;
}

bool_t
xdr_jobPeekReply(XDR *xdrs, struct jobPeekReply *jobPeekReply,
                 struct LSFHeader *hdr)
{
    static char outFile[MAXFILENAMELEN];

    if (xdrs->x_op == XDR_DECODE) {
        outFile [0] = '\0';
        jobPeekReply->outFile = outFile;
    }

    if (!xdr_string(xdrs, &(jobPeekReply->outFile), MAXFILENAMELEN))
        return false;


    if (!(xdr_var_string(xdrs, &jobPeekReply->pSpoolDir))) {
        return false;
    }

    return true;
}


bool_t
xdr_groupInfoReply(XDR *xdrs,
                   struct groupInfoReply *groupInfoReply,
                   struct LSFHeader *hdr)
{
    int i;

    if (xdrs->x_op == XDR_FREE) {
        for (i = 0; i < groupInfoReply->numGroups; i++) {
            FREEUP(groupInfoReply->groups[i].group);
            FREEUP(groupInfoReply->groups[i].memberList);
            FREEUP(groupInfoReply->groups[i].group_slots);
        }
        FREEUP(groupInfoReply->groups);
        groupInfoReply->numGroups = 0;
        return true;
    }


    if (xdrs->x_op == XDR_DECODE) {
        groupInfoReply->numGroups = 0;
        groupInfoReply->groups = NULL;
    }

    if (!xdr_int(xdrs, &(groupInfoReply->numGroups)))
        return false;


    if (xdrs->x_op == XDR_DECODE &&  groupInfoReply->numGroups != 0) {

        groupInfoReply->groups = calloc(groupInfoReply->numGroups,
                                        sizeof(struct groupInfoEnt));
        if (groupInfoReply->groups == NULL)
            return false;
    }

    for (i = 0; i < groupInfoReply->numGroups; i++) {

        if (!xdr_arrayElement(xdrs,
                              (char *)&(groupInfoReply->groups[i]),
                              hdr,
                              xdr_groupInfoEnt))
            return false;
    }

    if (xdrs->x_op == XDR_FREE)
        FREEUP(groupInfoReply->groups);

    return true;
}


bool_t
xdr_groupInfoEnt(XDR *xdrs, struct groupInfoEnt *gInfo,
                 struct LSFHeader *hdr)
{

    if (xdrs->x_op == XDR_DECODE) {
        gInfo->group = NULL;
        gInfo->memberList = NULL;
    }

    if (!xdr_var_string(xdrs, &(gInfo->group)) ||
        !xdr_var_string(xdrs, &(gInfo->memberList)))
        return false;

    if (xdrs->x_op == XDR_FREE) {
        FREEUP(gInfo->group);
        FREEUP(gInfo->memberList);
        FREEUP(gInfo->group_slots);
    }

    if (! xdr_var_string(xdrs, &gInfo->group_slots))
        return false;

    return true;

}

bool_t
xdr_controlReq(XDR *xdrs, struct controlReq *controlReq,
               struct LSFHeader *hdr)
{
    static char *sp = NULL;
    static int first = true;



    if (xdrs->x_op == XDR_DECODE) {
        if (first == true) {
            sp = (char *) malloc (MAXHOSTNAMELEN);
            if (sp == NULL)
                return false;
            first = false;
        }
        controlReq->name = sp;
        sp[0] = '\0';
    } else {
        sp = controlReq->name;
    }
    if (!(xdr_int(xdrs, &controlReq->opCode) &&
          xdr_string(xdrs, &sp, MAXHOSTNAMELEN)))
        return false;

    return true;
}

bool_t
xdr_jobSwitchReq (XDR *xdrs, struct jobSwitchReq *jobSwitchReq,
                  struct LSFHeader *hdr)
{
    char *sp;
    int jobArrId, jobArrElemId;

    sp = jobSwitchReq->queue;
    if (xdrs->x_op == XDR_DECODE)
        sp[0] = '\0';

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(jobSwitchReq->jobId, &jobArrId, &jobArrElemId);
    }
    if ( !(xdr_int(xdrs, &jobArrId) &&
           xdr_string(xdrs, &sp, MAX_LSB_NAME_LEN)))
        return false;
    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&jobSwitchReq->jobId,jobArrId,jobArrElemId);
    }
    return true;
}

bool_t
xdr_jobMoveReq (XDR *xdrs, struct jobMoveReq *jobMoveReq, struct LSFHeader *hdr)
{
    int jobArrId, jobArrElemId;

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(jobMoveReq->jobId, &jobArrId, &jobArrElemId);
    }
    if ( !(xdr_int(xdrs,&(jobMoveReq->opCode)) &&
           xdr_int(xdrs, &jobArrId) &&
           xdr_int(xdrs, &(jobMoveReq->position))) )
        return false;
    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&jobMoveReq->jobId,jobArrId,jobArrElemId);
    }
    return true;
}

bool_t
xdr_migReq (XDR *xdrs, struct migReq *req, struct LSFHeader *hdr)
{
    char *sp;
    int i;
    int jobArrId, jobArrElemId;

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(req->jobId, &jobArrId, &jobArrElemId);
    }
    if (!(xdr_int(xdrs, &jobArrId) &&
          xdr_int(xdrs, &req->options)))
        return false;

    if (!xdr_int(xdrs, &req->numAskedHosts))
        return false;

    if (xdrs->x_op == XDR_DECODE && req->numAskedHosts > 0) {
        req->askedHosts = (char **)
            calloc(req->numAskedHosts, sizeof (char *));
        if (req->askedHosts == NULL)
            return false;
        for (i = 0; i < req->numAskedHosts; i++) {
            req->askedHosts[i] = calloc (1, MAXHOSTNAMELEN);
            if (req->askedHosts[i] == NULL) {
                while (i--)
                    free(req->askedHosts[i]);
                free(req->askedHosts);
                return false;
            }
        }
    }


    for (i = 0; i < req->numAskedHosts; i++) {
        sp = req->askedHosts[i];
        if (xdrs->x_op == XDR_DECODE)
            sp[0] = '\0';
        if (!xdr_string(xdrs, &sp, MAXHOSTNAMELEN)) {
            for (i = 0; i < req->numAskedHosts; i++)
                free(req->askedHosts[i]);
            free(req->askedHosts);
            return false;
        }
    }
    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&req->jobId,jobArrId,jobArrElemId);
    }

    return true;
}



bool_t
xdr_xFile(XDR *xdrs, struct xFile *xf, struct LSFHeader *hdr)
{
    char *sp;

    if (xdrs->x_op == XDR_DECODE) {
        xf->subFn[0] = '\0';
        xf->execFn[0] = '\0';
    }
    sp = xf->subFn;

    if (!xdr_string(xdrs, &sp, MAXFILENAMELEN))
        return false;

    sp = xf->execFn;
    if (!xdr_string(xdrs, &sp, MAXFILENAMELEN))
        return false;

    if (!xdr_int(xdrs, &xf->options))
        return false;

    return true;
}


static int
allocLoadIdx(float **loadSched, float **loadStop, int *outSize, int size)
{
    if (*loadSched)
        free(*loadSched);
    if (*loadStop)
        free(*loadStop);

    *outSize = 0;

    if ((*loadSched = (float *) calloc(size, sizeof(float))) == NULL)
        return -1;

    if ((*loadStop = (float *) calloc(size, sizeof(float))) == NULL)
        return -1;

    *outSize = size;

    return 0;
}


bool_t
xdr_lsbShareResourceInfoReply(XDR *xdrs,
                              struct  lsbShareResourceInfoReply *lsbShareResourceInfoReply,
                              struct LSFHeader *hdr)
{
    int i, status;

    if (xdrs->x_op == XDR_DECODE) {
        lsbShareResourceInfoReply->numResources = 0;
        lsbShareResourceInfoReply->resources = NULL;
    }
    if (!(xdr_int(xdrs, &lsbShareResourceInfoReply->numResources)
          && xdr_int(xdrs, &lsbShareResourceInfoReply->badResource)))
        return false;

    if (xdrs->x_op == XDR_DECODE &&  lsbShareResourceInfoReply->numResources > 0) {
        if ((lsbShareResourceInfoReply->resources = (struct lsbSharedResourceInfo *)
             malloc (lsbShareResourceInfoReply->numResources
                     * sizeof (struct lsbSharedResourceInfo))) == NULL) {
            lserrno = LSE_MALLOC;
            return false;
        }
    }
    for (i = 0; i < lsbShareResourceInfoReply->numResources; i++) {
        status = xdr_arrayElement(xdrs,
                                  (char *)&lsbShareResourceInfoReply->resources[i],
                                  hdr,
                                  xdr_lsbSharedResourceInfo);
        if (! status) {
            lsbShareResourceInfoReply->numResources = i;

            return false;
        }
    }
    if (xdrs->x_op == XDR_FREE && lsbShareResourceInfoReply->numResources > 0) {
        FREEUP(lsbShareResourceInfoReply->resources);
        lsbShareResourceInfoReply->numResources = 0;
    }
    return true;
}


static bool_t
xdr_lsbSharedResourceInfo (XDR *xdrs, struct
                           lsbSharedResourceInfo *lsbSharedResourceInfo,
                           struct LSFHeader *hdr)
{
    int i, status;

    if (xdrs->x_op == XDR_DECODE) {
        lsbSharedResourceInfo->resourceName = NULL;
        lsbSharedResourceInfo->instances = NULL;
        lsbSharedResourceInfo->nInstances = 0;
    }
    if (!(xdr_var_string (xdrs, &lsbSharedResourceInfo->resourceName) &&
          xdr_int(xdrs, &lsbSharedResourceInfo->nInstances)))
        return false;

    if (xdrs->x_op == XDR_DECODE &&  lsbSharedResourceInfo->nInstances > 0) {
        if ((lsbSharedResourceInfo->instances =
             (struct lsbSharedResourceInstance *) malloc (lsbSharedResourceInfo->nInstances * sizeof (struct lsbSharedResourceInstance))) == NULL) {
            lserrno = LSE_MALLOC;
            return false;
        }
    }
    for (i = 0; i < lsbSharedResourceInfo->nInstances; i++) {
        status = xdr_arrayElement(xdrs,
                                  (char *)&lsbSharedResourceInfo->instances[i],
                                  hdr,
                                  xdr_lsbShareResourceInstance);
        if (! status) {
            lsbSharedResourceInfo->nInstances = i;
            return false;
        }
    }
    if (xdrs->x_op == XDR_FREE && lsbSharedResourceInfo->nInstances > 0) {
        FREEUP (lsbSharedResourceInfo->instances);
        lsbSharedResourceInfo->nInstances = 0;
    }
    return true;
}

static bool_t
xdr_lsbShareResourceInstance (XDR *xdrs,
                              struct  lsbSharedResourceInstance *instance,
                              struct LSFHeader *hdr)
{

    if (xdrs->x_op == XDR_DECODE) {
        instance->totalValue = NULL;
        instance->rsvValue = NULL;
        instance->hostList = NULL;
        instance->nHosts = 0;
    }
    if (!(xdr_var_string (xdrs, &instance->totalValue)
          && xdr_var_string (xdrs, &instance->rsvValue)
          && xdr_int(xdrs, &instance->nHosts)))
        return false;

    if (xdrs->x_op == XDR_DECODE &&  instance->nHosts > 0) {
        if ((instance->hostList = (char **)
             malloc (instance->nHosts * sizeof (char *))) == NULL) {
            lserrno = LSE_MALLOC;
            return false;
        }
    }
    if (! xdr_array_string(xdrs, instance->hostList,
                           MAXHOSTNAMELEN, instance->nHosts)) {
        if (xdrs->x_op == XDR_DECODE) {
            FREEUP(instance->hostList);
            instance->nHosts = 0;
        }
        return false;
    }
    if (xdrs->x_op == XDR_FREE && instance->nHosts > 0) {
        FREEUP (instance->hostList);
        instance->nHosts = 0;
    }
    return true;
}

bool_t
xdr_runJobReq(XDR*                   xdrs,
              struct runJobRequest*  runJobRequest,
              struct LSFHeader*      lsfHeader)
{
    int i;
    int jobArrId, jobArrElemId;

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(runJobRequest->jobId, &jobArrId, &jobArrElemId);
    }
    if (!xdr_int(xdrs, &(runJobRequest->numHosts)) ||
        !xdr_int(xdrs, &jobArrId) ||
        !xdr_int(xdrs, &(runJobRequest->options))) {
        return false;
    }


    if (xdrs->x_op == XDR_DECODE) {
        runJobRequest->hostname = (char **)calloc(runJobRequest->numHosts,
                                                  sizeof(char *));
        if (runJobRequest->hostname == NULL)
            return false;
    }

    for (i = 0; i < runJobRequest->numHosts; i++) {
        if (!xdr_var_string(xdrs, &(runJobRequest->hostname[i]))) {
            return false;
        }
    }

    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&runJobRequest->jobId,jobArrId,jobArrElemId);
    }
    if (xdrs->x_op == XDR_FREE) {
        runJobRequest->numHosts = 0;
        FREEUP(runJobRequest->hostname);
    }

    return true;
}

bool_t
xdr_jobAttrReq(XDR *xdrs, struct jobAttrInfoEnt *jobAttr, struct LSFHeader *hdr)
{
    char *sp = jobAttr->hostname;

    int jobArrId, jobArrElemId;

    if (xdrs->x_op == XDR_ENCODE) {
        jobId64To32(jobAttr->jobId, &jobArrId, &jobArrElemId);
    }
    if (!xdr_int(xdrs, &jobArrId)) {
        return false;
    }
    if (!(xdr_u_short(xdrs, &(jobAttr->port))
          && xdr_string(xdrs, &sp, MAXHOSTNAMELEN))) {
        return false;
    }

    if (!xdr_int(xdrs, &jobArrElemId)) {
        return false;
    }

    if (xdrs->x_op == XDR_DECODE) {
        jobId32To64(&jobAttr->jobId,jobArrId,jobArrElemId);
    }

    return true;
}

bool_t
xdr_jobID(XDR *xdrs,
          LS_LONG_INT *jobID,
          struct LSFHeader *hdr)
{
    int jobid;
    int elem;

    if (xdrs->x_op == XDR_ENCODE)
        jobId64To32(*jobID, &jobid, &elem);

    if (! xdr_int(xdrs, &jobid)
        || !xdr_int(xdrs, &elem))
        return false;

    if (xdrs->x_op == XDR_DECODE)
        jobId32To64(jobID, jobid, elem);

    return true;
}

bool_t
xdr_lsbMsg(XDR *xdrs,
           struct lsbMsg *m,
           struct LSFHeader *hdr)
{
    if (! xdr_time_t(xdrs, &m->t))
        return false;

    if (! xdr_wrapstring(xdrs, &m->msg))
        return false;

    return true;
}

bool_t
xdr_batchRes(XDR *xdrs,
             struct batchRes *res,
             struct LSFHeader *hdr)
{
    if (! xdr_wrapstring(xdrs, &res->name)
        || ! xdr_int(xdrs, &res->value)
        || ! xdr_int(xdrs, &res->options))
        return false;

    return true;
}
