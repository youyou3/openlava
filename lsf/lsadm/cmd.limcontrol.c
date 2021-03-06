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
#include "../intlib/intlibout.h"

extern void millisleep_(int);
extern char isint_(char *);
extern int  getConfirm(char *);
extern int checkConf(int, int);
extern char *myGetOpt(int, char **, char *);
static void doHosts(int, char **, int);
static void doAllHosts(int);
static void operateHost(char *, int, int);

static int  exitrc;
static int fFlag;

int
limCtrl(int argc, char **argv, int opCode)
{
    char *optName;
    char *localHost;
    int config = 0;
	int checkReply = 0;

    fFlag = 0;
    if (strcmp(argv[optind-1], "reconfig") == 0) {
	config = 1;
    }

    while ((optName = myGetOpt(argc, argv, "f|v|")) != NULL) {
        switch (optName[0]) {
            case 'v':
                if (opCode == LIM_CMD_SHUTDOWN)
                    return -2;
                break;
            case 'f':
                fFlag = 1;
                break;
            default:
                return -2;
        }
    }
    exitrc = 0;
    if (config && optind != argc)
        return -2;

    switch (checkReply)  {
	case EXIT_FATAL_ERROR:
            return -1;
	case EXIT_WARNING_ERROR:
            if (fFlag)
                break;
            if (!getConfirm("Do you want to reconfigure? [y/n] ")) {
                fprintf(stderr, "Reconfiguration aborted.\n");
                return -1;
            }
            break;
        default:
            break;
    }

    if (config) {
        doAllHosts(opCode);
        return(exitrc);
    }

    if (optind == argc) {
	if ((localHost = ls_getmyhostname()) == NULL) {
            ls_perror("ls_getmyhostname");
            return -1;
	}
        operateHost(localHost, opCode, 0);
    } else {
	doHosts(argc, argv, opCode);
    }

    return exitrc;

}

static
void doHosts(int argc, char **argv, int opCode)
{
    if (optind == argc-1 && strcmp(argv[optind], "all") == 0) {

	doAllHosts(opCode);
	return;
    }
    for (; optind < argc; optind++)
	operateHost(argv[optind], opCode, 0);

}

static
void doAllHosts(int opCode)
{
    int numhosts = 0, i;
    struct hostInfo *hostinfo;
    int ask = FALSE, try = FALSE;
    char msg[128];

    hostinfo = ls_gethostinfo("-:server", &numhosts, NULL, 0, LOCAL_ONLY);
    if (hostinfo == NULL) {
	ls_perror("ls_gethostinfo");
	fprintf(stderr, "Operation aborted\n");
        exitrc = -1;
	return;
    }

    if (!fFlag) {
	if (opCode == LIM_CMD_REBOOT)
            sprintf(msg, "Do you really want to restart LIMs on all hosts? [y/n] ");
        else
	    sprintf(msg, "Do you really want to shut down LIMs on all hosts? [y/n] ");
	ask = (!getConfirm(msg));
    }

    for (i = 0; i < numhosts; i++)
        if (hostinfo[i].maxCpus > 0)
	    operateHost (hostinfo[i].hostName, opCode, ask);
        else
	    try = 1;
    if (try) {
        fprintf(stderr, "\n%s :\n\n", "Trying unavailable hosts");
        for (i = 0; i < numhosts; i++)
            if (hostinfo[i].maxCpus <= 0)
	        operateHost (hostinfo[i].hostName, opCode, ask);
    }

}

static void
operateHost(char *host, int opCode, int confirm)
{
    char msg1[MAXLINELEN];
    char msg[MAXLINELEN];

    if (opCode == LIM_CMD_REBOOT)
	sprintf(msg1, "Restart LIM on <%s>", host);
    else
	sprintf(msg1, "Shut down LIM on <%s>", host);

    if (confirm) {
	sprintf(msg, "%s ? [y/n] ", msg1);
        if (!getConfirm(msg))
	    return;
    }

    fprintf(stderr, "%s ...... ", msg1);
    fflush(stderr);
    if (ls_limcontrol(host, opCode) == -1) {
	ls_perror ("ls_limcontrol");
	exitrc = -1;
    } else {
	char *delay = getenv("LSF_RESTART_DELAY");
	int  delay_time;
	if (delay == 0)
	    delay_time = 500;
	else
	    delay_time = atoi(delay) * 1000;

	millisleep_(delay_time);
	fprintf(stderr, "done\n");
    }
}

int
limLock(int argc, char **argv)
{
    int duration;
    char *optName;

    duration = 0;
    while ((optName = myGetOpt(argc, argv, "l:")) != NULL) {
        switch(optName[0]) {
            case 'l':
                duration = atoi(optarg);
                if (!isint_(optarg) || atoi(optarg) <= 0) {
	            fprintf(stderr, "\
The host locking duration <%s> should be a positive integer\n", optarg);
                    return -2;
		}
		break;
            default:
                return -2;
        }
    }

    if (argc > optind)
        return -2;

    if (ls_lockhost(duration) < 0) {
	ls_perror("failed");
        return -1;
    }

    if (duration)
        printf("Host is locked for %d seconds\n", duration);
    else
        printf("Host is locked\n");

    return 0;
}

int
limUnlock(int argc, char **argv)
{

    if (argc > optind) {
	fprintf(stderr, "Syntax error: too many arguments.\n");
	return -2;
    }

    if (ls_unlockhost() < 0) {
        ls_perror("ls_unlockhost");
        return -1;
    }

    printf("Host is unlocked\n");

    return 0;
}
