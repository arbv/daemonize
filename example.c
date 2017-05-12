#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include "daemonize.h"


static int signal_pipe[2] = {-1}; /* we will use this pipe for signal code transmission from handler to daemon body */

/* Daemon signal handler. */
static void signal_handler(int sig)
{
    switch (sig)
    {
        case SIGTERM: /* stop daemon */
        case SIGHUP:  /* reload daemon configuration */
            /* write signal code into pipe */
            write(signal_pipe[1], (void *)&sig, sizeof(sig));
            break;
        default:
            return;
            break;
    }
}

/* daemon process body */
static int example_daemon(void *udata)
{
    int exit = 0;
    int exit_code = EXIT_SUCCESS;

    /* open syslog */
    openlog("EXAMPLE", LOG_NDELAY, LOG_DAEMON);

    /* greeting */
    syslog(LOG_INFO, "EXAMPLE daemon started. PID: %ld", (long)getpid());

    /* create pipe for signal handling */
    if (pipe(signal_pipe) == -1)
    {
        syslog(LOG_ERR, "Cannot create pipe for signal handling.");
        return EXIT_FAILURE;
    }

    /* set signal handlers */
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    /* set pipe as non blockable */
    if (fcntl(signal_pipe[0], F_SETFL, O_NONBLOCK) == -1 ||
        fcntl(signal_pipe[1], F_SETFL, O_NONBLOCK) == -1)
    {
        syslog(LOG_ERR, "Cannot set signal pipe options.");
        close(signal_pipe[0]);
        close(signal_pipe[1]);
        return EXIT_FAILURE;
    }

    /* daemon loop */
    while (!exit)
    {
        int sig;
        int result;
        fd_set readset;
        
        /* add pipe to set */
        FD_ZERO(&readset);
        FD_SET(signal_pipe[0], &readset);

        /* wait for data in pipe */
        result = select(FD_SETSIZE, &readset, NULL, NULL, NULL);
        /* If errno equals EINTR then call was interrupted by signal. */
        if (result == -1 && errno != EINTR)
        {
            syslog(LOG_ERR, "Fatal error during select() call.");
            /* low level error */
            exit_code = EXIT_FAILURE;
            break;
        }

        /* read data from pipe */
        if (read(signal_pipe[0], &sig, sizeof(sig)) > 0)
        {
            /* handle signal */
            switch (sig)
            {
                case SIGTERM: /* stop daemon */
                    syslog(LOG_INFO, "Got SIGTERM signal. Stopping daemon...");
                    exit = 1;
                    break;
                case SIGHUP: /* reload configuration */
                    syslog(LOG_INFO, "Got SIGHUP signal.");
                    break;
                default:
                    syslog(LOG_WARNING, "Got unexpected signal (number: %d).", sig);
                    break;
            }
        }
    }

    /* remove signal handlers */
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);

    /* close signal pipe */
    close(signal_pipe[0]);
    close(signal_pipe[1]);

    /* write exit code to the system log */
    syslog(LOG_INFO, "Daemon stopped with status code %d.", exit_code);
    /* close system log */
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
        case -2: /* Daemon already running */
        {
            fprintf(stderr,"Daemon already running.\n");
        }
        break;
        case 0: /* Daemon process. */
        {
            return exit_code; /* Return daemon exit code. */
        }
        default: /* Parent process */
        {
            printf("Parent: %ld, Daemon: %ld\n", (long)getpid(), (long)pid);
        }
        break;
    }

    return EXIT_SUCCESS;
}

