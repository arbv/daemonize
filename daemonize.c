#include <unistd.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>


#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "daemonize.h"


/* utilities to write and read error code from pipe */
static void write_code(int fd, int code)
{
    write(fd, (void *)&code, sizeof(code));
}

static int read_code(int fd)
{
    int code = -1;
    read(fd, (void *)&code, sizeof(code));
    return code;
}

/* utilities to write and read PID from pipe */
static void write_pid(int fd, pid_t pid)
{
    write(fd, (void *)&pid, sizeof(pid));
}

static pid_t read_pid(int fd)
{
    pid_t pid = -1;
    read(fd, (void *)&pid, sizeof(pid));
    return pid;
}

/* the actual function which performs forking */
static pid_t doublefork(int *pipefd)
{
    pid_t pid;

    switch ((pid = fork()))
    {
        case -1: /* error */
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
            break;
        case 0:  /* first  child */
            close(pipefd[0]); /* close read side of the pipe */

            /* create session */
            if (setsid() == -1) /* error */
            {
                write_code(pipefd[1], errno);
                close(pipefd[1]);
                return -1;
            }

            /* fork daemon */
            switch ((pid = fork()))
            {
                case -1: /* error */
                    write_code(pipefd[1], errno);
                    close(pipefd[1]);
                    return -1;
                    break;
                case 0:  /* second child - daemon */
                    /* write success error code */
                    write_code(pipefd[1], 0);
                    /* write daemon process PID back to the first parent */
                    write_pid(pipefd[1], getpid());
                    close(pipefd[1]);
                    return 0;
                    break;
                default: /* second parent */
                    exit(0);
                    break;
            }
            break;
        default: /* parent */
        {
            int code;
            int status;

            /* wait for child to exit */
            waitpid(pid, &status, 0);
            /* close write side of the pipe */
            close(pipefd[1]);
            /* read error code */
            code = read_code(pipefd[0]);
            /* read daemon process PID on success */
            if (code == 0)
            {
                pid = read_pid(pipefd[0]);
            }

            /* close read end of the pipe */
            close(pipefd[0]);
            /* set errno */
            errno = code;
            if (code == 0)
            {
                return pid;
            }
            else
            {
                return -1;
            }
        }
        break;
    }
    
    /* not reachable */
    return -1;
}

/* redirect standard file descriptors */
static int redirect_fds(void)
{
    /* close standard file descriptors */
    close(0);
    close(1);
    close(2);

    /* redirect standard input */
    if (open("/dev/null", O_RDONLY) != 0)
    {
        return 1;
    }

    /* redirect standard output */
    if (open("/dev/null", O_WRONLY) != 1)
    {
        return 1;
    }

    /* redirect standard protocol */
    if (dup(1) != 2)
    {
        return 1;
    }

    return 0;
}

pid_t daemonize(int flags)
{
    pid_t pid = -1;
    int pipefd[2] = {0};
    struct rlimit rl;
    sigset_t sigset;
    int i;

    /* close all open files, except stdin, stdout, stderr */
    if (!(flags & DMN_NO_CLOSE))
    {
        memset(&rl, 0, sizeof(rl));
        if (getrlimit(RLIMIT_NOFILE, &rl) != 0)
        {
            return -1;
        }

        for (i = 3; i < rl.rlim_cur; i++)
        {
            close(i);
        }
    }

    if (!(flags & DMN_KEEP_SIGNAL_HANDLERS))
    {
        /* reset all signals to their defaults */
#if defined __linux__ &&  defined ( _NSIG )
        /* Linux */
        const int nsig = _NSIG;
#elif defined NSIG
        /* BSD flavours */
        const int nsig = NSIG;
#else
        /* sane default for the less common systems */
        const int nsig = 32;
#endif
        for (i = 0; i < nsig; i++)
        {
            signal(i, SIG_DFL);
        }
    }

    /* reset error code */
    errno = 0;

    /* explicitly block all signals */
    sigfillset(&sigset);
    if (sigprocmask(SIG_BLOCK, &sigset, NULL) != 0)
    {
        return -1;
    }

    /* create pipes for communication with daemon */
    if (pipe(pipefd) != 0)
    {
        return -1;
    }

    /* make double fork - daemonize */
    /* it will also close pipes when appropriately */
    if ((pid = doublefork(pipefd)) < 0)
    {
        return -1;
    }

    /* unblock all signals */
    sigfillset(&sigset);
    if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) != 0)
    {
        return -1;
    }

    if (pid != 0) /* this is the process which started daemon */
    {
        return pid;
    }
    
    /* redirect stdin, stdout, stderr to /dev/null */
    if (!(flags & DMN_NO_CLOSE))
    {
        if (redirect_fds() != 0)
        {
            return -1;
        }
    }

    /* change daemon's working directory */
    if (!(flags & DMN_NO_CHDIR))
    {
        if (chdir("/") != 0)
        {
            return -1;
        }
    }

    /* change umask */
    if (!(flags & DMN_NO_UMASK))
    {
        umask(0);
    }

    return pid;
}

/* check if daemon already running */
static int check_pid_file(const char *pid_file_path)
{
    struct flock fl;
    int fd = -1;

    /* try to open file */
    fd = open(pid_file_path, O_RDWR);
    if (fd == -1)
    {
        if (errno == ENOENT) /* file does not exist */
        {
            errno = 0;
            return 0;
        }
        return 1;
    }

    /* set locking parameters */
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();
    
    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
        close(fd);

        /* file is locked by another process */
        if (errno == EAGAIN || errno == EACCES)
        {
            return -1;
        }
        return 1;
    }

    /* unlock */
    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
        return 1;
    }

    /* close  file */
    close(fd);

    return 0;
}

pid_t rundaemon(int flags, int (*daemon_func)(void *), void *udata, int *exit_code, const char *pid_file_path)
{
    pid_t pid;
    int pid_file_fd = -1;
    struct flock fl;
    int daemon_exit_code;

    /* validate arguments */
    if (daemon_func == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    /* check PID file */
    if (pid_file_path != NULL && *pid_file_path)
    {
        int status = check_pid_file(pid_file_path);
        if (status == -1)
        {
            errno = 0;
            return -2; /* the daemon instance seems to be running */
        }
        else if (status == 1)
        {
            return -1; /* internal error */
        }
    }

    /* daemonize process */
    pid = daemonize(flags);
    if (pid == -1) /* error during process daemonization */
    {
        return -1;
    }

    if (pid != 0) /* return - this is the process which starts daemon */
    {
        return pid;
    }

    /* create PID file */
    if (pid_file_path != NULL && *pid_file_path)
    {
        char pid_str[64] = {0};
        int pid_str_len;
        mode_t mask;
        
        /* get PID as string */
        pid_str_len = snprintf(pid_str, sizeof(pid_str), "%ld", (long)getpid());

        /* remove old PID file */
        if (access(pid_file_path, F_OK) != -1)
        {
            unlink(pid_file_path);
            errno = 0;
        }

        /* create new PID file */
        mask = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; /* -rw-r--r-- */
        pid_file_fd = open(pid_file_path, O_RDWR | O_CREAT, mask);
        if (pid_file_fd == -1)
        {
            return -1;
        }

        /* write PID */
        if (write(pid_file_fd, &pid_str[0], pid_str_len) == -1)
        {
            return -1;
        }

        /* set locking parameters */
        memset(&fl, 0, sizeof(fl));
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        fl.l_pid = getpid();

        /* lock file */
        if (fcntl(pid_file_fd, F_SETLK, &fl) == -1)
        {
            close(pid_file_fd);
            return -1;
        }
    }

    /* run daemon code */
    daemon_exit_code = daemon_func(udata);
    if (exit_code != NULL)
    {
        *exit_code = daemon_exit_code; /* save exit code */
    }

    /* remove PID file */
    if (pid_file_path != NULL && *pid_file_path)
    {
        /* unlock */
        fl.l_type = F_UNLCK;
        fcntl(pid_file_fd, F_SETLK, &fl);

        /* close PID-file */
        close(pid_file_fd);
        /* remove file */
        unlink(pid_file_path);
    }
    
    return pid;
}

