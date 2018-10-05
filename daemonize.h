#ifndef _DAEMONIZE_H
#define _DAEMONIZE_H

#ifndef _WIN32
/* Daemon creation flags. */
enum {
    DMN_DEFAULT  = 0,
    DMN_NO_CLOSE = 1,     /* Do not close existing file descriptors and do not redirect standard file descriptors to '/dev/null'.  */
    DMN_KEEP_SIGNAL_HANDLERS = 2, /* Do not reset signal handlers to their defaults. */
    DMN_NO_CHDIR = 4,     /* Do not change current directory of the daemon to '/'. */
    DMN_NO_UMASK = 8      /* Do not set umask to 0. */
};

#ifdef __cplusplus
extern "C" {
#endif

extern pid_t daemonize(int flags);
/*
* Description 
daemonize() - low level function to daemonize process.
Roughly corresponds to the BSDs' daemon() function but uses
double fork technique (which is important on System V
flavoured Unix systems) and allows specifying more
daemonization parameters.

* Arguments:
flags - a bit mask of the daemon creation flags, see above.

* Return value
daemonize() follows fork() semantics.
By default function returns PID of the process
to the parent process (which starts daemon) or 0
to the daemon process. On error it returns -1 - this value
might be returned to both parent and daemon process. In this
case errno value will be set accordingly.
*/

extern pid_t rundaemon(int flags,
                       int (*daemon_func)(void *udata),
                       void *udata,
                       int *exit_code,
                       const char *pid_file_path);
/*
* Description
rundaemon() - thin wrapper on top of daemonize which
might check and create PID-file, if desired. It is recommended
to use this function instead of daemonize() whenever applicable.

* Arguments:
flags - daemon creation flags, see above;
daemon_func - function pointer to the function to be called after
successful - daemonization (actual daemon body);
udata - pointer to be passed as value in call to daemon_func;
exit_code - pointer to variable to receive value returned by daemon_func;
pid_file_path - full pathname to the PID file. This file will be
used for checking if daemon is not already running (by checking if
file exists and locked) and will be created if not exists. On daemon exit
this file will be removed under normal conditions.
This value might be NULL, in this case no any checks performed and
no file will be created.

* Return value

This functions shares return values with daemonize() function mentioned
above with one notable addition - if daemon already running it will
return -2 to the process which starts the daemon. No daemonization
will be performed in this case.
*/

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* _DAEMONIZE_H */

