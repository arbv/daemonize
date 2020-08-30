/*
MIT/Expat License

Copyright (c) 2016-2020 Artem Boldariev <artem@boldariev.com>

See the LICENSE.txt for details about the terms of use.

This example is Linux specific because it uses signalfd(2).
*/

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#ifdef __linux__
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <fcntl.h>
#include <syslog.h>

#include "daemonize.h"

/* helps to keep track of the largest file descriptor for select() */
#define FD_SET_MAX(fd, setp, maxp) \
    (FD_SET(fd, setp), ((maxp) != NULL && fd > (*maxp)) ? *(maxp) = fd : fd)

/* The daemon process body */
static int example_daemon(void *udata)
{
    int exit = 0;
    int exit_code = EXIT_SUCCESS;

    int sfd = -1;
    sigset_t mask;
    struct signalfd_siginfo si;

    /* open the system log */
    openlog("EXAMPLE", LOG_NDELAY, LOG_DAEMON);

    /* greeting */
    syslog(LOG_INFO, "EXAMPLE daemon started. PID: %ld", (long)getpid());

    /* create a file descriptor for signal handling */
    sigemptyset(&mask);
    /* handle the following signals */
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);

    /* Block the signals so that they aren't handled
       according to their default dispositions */
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        closelog();
        return EXIT_FAILURE;
    }

    sfd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (sfd == -1)
    {
        perror("signalfd failed");
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        closelog();
        return EXIT_FAILURE;
    }

    /* the daemon loop */
    while (!exit)
    {
        int maxfd = -1;
        int result;
        fd_set readset;

        /* add the signal file descriptor to set */
        FD_ZERO(&readset);
        FD_SET_MAX(sfd, &readset, &maxfd);
        /* One could add more file descriptors here
           and handle them accordingly if one wants to build a server using
           event-driven approach. */

        /* wait for the data in the signal file descriptor */
        result = select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (result == -1)
        {
            syslog(LOG_ERR, "Fatal error during select() call.");
            /* a low level error */
            exit_code = EXIT_FAILURE;
            break;
        }

        /* read the data from the signal handler file descriptor */
        if (FD_ISSET(sfd, &readset) && read(sfd, &si, sizeof(si)) > 0)
        {
            /* handle the signals */
            switch (si.ssi_signo)
            {
                case SIGTERM: /* stop the daemon */
                    syslog(LOG_INFO, "Got SIGTERM signal. Stopping daemon...");
                    exit = 1;
                    break;
                case SIGHUP: /* reload the configuration */
                    syslog(LOG_INFO, "Got SIGHUP signal.");
                    break;
                default:
                    syslog(LOG_WARNING, "Got unexpected signal (number: %d).", si.ssi_signo);
                    break;
            }
        }
    }

    /* close the signal file descriptor */
    close(sfd);
    /* remove the signal handlers */
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    /* write an exit code to the system log */
    syslog(LOG_INFO, "Daemon stopped with status code %d.", exit_code);
    /* close the system log */
    closelog();

    return exit_code;
}


int main(int argc, char **argv)
{
    int exit_code = 0;

    pid_t pid = rundaemon(0, /* Daemon creation flags. */
                          example_daemon, NULL, /* Daemon body function and its argument. */
                          &exit_code, /* Pointer to a variable to receive daemon exit code */
                          "/tmp/example.pid"); /* Full path to the PID-file (lock). */
    switch (pid)
    {
        case -1: /* Low level error. See errno for details. */
        {
            perror("Cannot start daemon.");
            return EXIT_FAILURE;
        }
        break;
        case -2: /* Daemon is already running */
        {
            fprintf(stderr,"Daemon already running.\n");
        }
        break;
        case 0: /* Daemon process. */
        {
            return exit_code; /* Return the daemon exit code. */
        }
        default: /* Parent process */
        {
            printf("Parent: %ld, Daemon: %ld\n", (long)getpid(), (long)pid);
        }
        break;
    }

    return EXIT_SUCCESS;
}
#else
int main(int argc, char **argv)
{
    fprintf(stderr, "This program contains Linux specific code. Exiting...\n");
    return EXIT_FAILURE;
}
#endif /* _linux_  */
