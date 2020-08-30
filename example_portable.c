/*
MIT/Expat License

Copyright (c) 2016-2020 Artem Boldariev <artem@boldariev.com>

See the LICENSE.txt for details about the terms of use.

This example should be possible to build on most Unix-like systems.
It uses self-pipe trick to redirect signal handling to the main event loop
of the application.
*/

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

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

/* global variables */
static int pipe_TERM[2];
static int pipe_HUP[2];

/* signal handler */
static void sig_handler(int signum)
{
    int fd = -1;
    int saved_errno = errno;
    /* handle the signals */
    switch (signum)
    {
        case SIGTERM: /* stop the daemon */
            fd = pipe_TERM[1];
            break;
        case SIGHUP: /* reload the configuration */
            fd = pipe_HUP[1];
            break;
        default:
            syslog(LOG_WARNING, "Got unexpected signal (number: %d).", signum);
            errno = saved_errno;
            return;
            break;
    }

    /* notify about the signal by writing a bite */
    if (write(fd, "x", 1) == -1 && errno != EAGAIN)
    {
        syslog(LOG_ERR, "Failed to handle signal");
        exit(EXIT_FAILURE);
        return;
    }

    errno = saved_errno;
}

static void close_signal_pipes(void)
{
    close(pipe_TERM[0]);
    close(pipe_TERM[1]);
    close(pipe_HUP[0]);
    close(pipe_HUP[1]);
}

/* create self-pipes and sets them into non-blocking mode */
static int open_signal_pipes(void)
{
    int flags = 0;
    /* create pipes */
    if (pipe(pipe_TERM) == -1)
    {
        return -1;
    }

    if (pipe(pipe_HUP) == -1)
    {
        close(pipe_TERM[0]);
        close(pipe_TERM[1]);
        return -1;
    }

    /* mark write ends of the pipes as non-blocking */
    if ( (flags = fcntl(pipe_TERM[0], F_GETFL)) == -1 ||
         fcntl(pipe_TERM[0], F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) == -1 ||
         (flags = fcntl(pipe_TERM[1], F_GETFL)) == -1 ||
         fcntl(pipe_TERM[1], F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) == -1 ||
         (flags = fcntl(pipe_HUP[0], F_GETFL)) == -1 ||
         fcntl(pipe_HUP[0], F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) == -1 ||
         (flags = fcntl(pipe_HUP[1], F_GETFL)) == -1 ||
         fcntl(pipe_HUP[1], F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) == -1)
    {
        close_signal_pipes();
        return -1;
    }

    return 0;
}

static int read_bytes_selfpipe(const int fd)
{
    unsigned char b;
    int count = 0;
    for (;;)
    {
        if (read(fd, &b, 1) == -1)
        {
            if (errno == EAGAIN) /* there are no any bytes left */
            {
                break;
            }
            else
            {
                syslog(LOG_ERR, "Error when reading data from a self-pipe.");
                return -1;
            }
        }
        count++;
    }
    return count;
}

/* The daemon process body */
static int example_daemon(void *udata)
{
    int exit = 0;
    int exit_code = EXIT_SUCCESS;

    sigset_t mask;
    struct sigaction act_TERM, act_HUP, old_act_TERM, old_act_HUP;

    /* open the system log */
    openlog("EXAMPLE", LOG_NDELAY, LOG_DAEMON);

    /* greeting */
    syslog(LOG_INFO, "EXAMPLE daemon has started. PID: %ld", (long)getpid());

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

    if (open_signal_pipes() == -1)
    {
        syslog(LOG_ERR, "Opening self-pipes failed");
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        closelog();
        return EXIT_FAILURE;
    }

    /* set signal handlers */
    memset(&act_TERM, 0, sizeof(act_TERM));
    memset(&act_HUP, 0, sizeof(act_HUP));
    act_TERM.sa_handler = sig_handler;
    act_HUP.sa_handler = sig_handler;
    act_TERM.sa_flags = act_HUP.sa_flags = SA_RESTART;

    if (sigaction(SIGTERM, &act_TERM, &old_act_TERM) == -1)
    {
        syslog(LOG_ERR, "Setting SIGTERM handler failed");
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        closelog();
        return EXIT_FAILURE;
    }

    if (sigaction(SIGHUP, &act_HUP, &old_act_HUP) == -1)
    {
        syslog(LOG_ERR, "Setting SIGHUP handler failed");
        sigaction(SIGTERM, &old_act_TERM, NULL);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        closelog();
        return EXIT_FAILURE;
    }

    /* unblock the signals */
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
        syslog(LOG_ERR, "Setting signal mask failed");
        sigaction(SIGTERM, &old_act_TERM, NULL);
        sigaction(SIGHUP, &old_act_HUP, NULL);
        closelog();
        return EXIT_FAILURE;
    }

    /* the daemon loop */
    while (!exit)
    {
        int maxfd =  -1;
        int result;
        fd_set readset;

        /* add the self-pipe descriptors to the set */
        FD_ZERO(&readset);
        FD_SET_MAX(pipe_TERM[0], &readset, &maxfd);
        FD_SET_MAX(pipe_HUP[0], &readset, &maxfd);
        /* One could add more file descriptors here
           and handle them accordingly if one wants to build a server using
           the event-driven approach. */

        /* wait for the data in the signal file descriptor */
        result = select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (result == -1 && errno != EINTR)
        {
            syslog(LOG_ERR, "Fatal error during select() call.");
            /* a low level error */
            exit_code = EXIT_FAILURE;
            break;
        }
        else if (result == 0 || (result == -1 && errno == EINTR))
        {
            /* continue on timeout or after being interrupted by a signal */
            continue;
        }

        /* read the data from the signal handler file descriptor */
        if (FD_ISSET(pipe_TERM[0], &readset)) /* SIGTERM */
        {
            if (read_bytes_selfpipe(pipe_TERM[0]) == -1)
            {
                exit_code = EXIT_FAILURE;
                break;
            }
            syslog(LOG_INFO, "Got SIGTERM signal. Stopping daemon...");
            exit = 1;

        }
        else if (FD_ISSET(pipe_HUP[0], &readset)) /* SIGHUP */
        {
            if (read_bytes_selfpipe(pipe_HUP[0]) == -1)
            {
                exit_code = EXIT_FAILURE;
                break;
            }
            syslog(LOG_INFO, "Got SIGHUP signal."); /* reload the configuration */
        }
    }

    /* reset the signal handlers */
    sigprocmask(SIG_BLOCK, &mask, NULL);
    /* close the signal self-pipes */
    close_signal_pipes();
    sigaction(SIGTERM, &old_act_TERM, NULL);
    sigaction(SIGHUP, &old_act_HUP, NULL);
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
