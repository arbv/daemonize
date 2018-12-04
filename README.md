# Overview

Writing UNIX-daemons implies writing a lot of highly repeatable
code. On the other hand, this code might be written and debugged once
and used in many programs which need to function like daemons. This
problem was addressed on BSD UNIX derivatives via a `daemon()`
function although partially - this function implements only a subset of
the steps needed to be performed during daemon initialization on most
UNIX-like operating systems.

This project implements two functions (roughly modelled on BSDs'
`daemon()`) which might be used in a project to daemonize process on
UNIX-like operating systems which follow System V semantics as well as
other UNIX derivatives. These functions fully implement all the steps
that need to be performed by a classical daemon process
(e.g. double-forking, PID-file checking, creation and locking, etc).

Please see the [following post](https://chaoticlab.io/c/c++/unix/2018/10/01/daemonize.html) for the additional information.

# Exported Functions
***
```
extern pid_t daemonize(int flags)
```
It is a low-level function to daemonize process.  Roughly corresponds
to the BSDs' `daemon()` function but uses the double-fork technique (which
is important on System V flavoured Unix systems) and allows specifying
more daemonization parameters.

## Arguments:
- `int flags` - a bit mask of the daemon creation flags:
   - **DMN_DEFAULT** - Create daemon with default daemonization parameters (close existing file descriptors, reset signal handlers, change the current directory to **/**, set **umask** to 0).
   - **DMN_NO_CLOSE** -  Do not close existing file descriptors and do not redirect standard file descriptors to **/dev/null**.
   - **DMN_KEEP_SIGNAL_HANDLERS** - Do not reset signal handlers to their defaults.
   - **DMN_NO_CHDIR** - Do not change the current directory of the daemon to **/**.
   - **DMN_NO_UMASK** - Do not set **umask** to 0.

## Return value
`daemonize()` follows `fork()` semantics.  By design, the function returns PID
of the process to the parent process (which starts daemon) or 0 to the
daemon process. On error it returns -1 - this value might be returned
to both parent and daemon process. In this case, **errno** value will
be set accordingly.

***
```
extern pid_t rundaemon(int flags,
                       int (*daemon_func)(void *udata),
                       void *udata,
                       int *exit_code,
                       const char *pid_file_path);
```

It is a thin wrapper on top of `daemonize()` which might check and create
PID-file if desired. It is recommended to use this function instead
of *daemonize()* whenever applicable.

## Arguments
- `int flags` - daemon creation flags, see above;
- `int (*daemon_func)(void *udata)` - function pointer to the function to be called after successful daemonization (actual daemon body);
- `void *udata` - pointer to be passed as the value in a call to daemon_func;
- `int exit_code` - pointer to variable to receive value returned by daemon_func;
- `const char *pid_file_path` - full pathname to the PID-file. This file will be used for checking if the daemon is not already running (by checking if the file exists and locked) and will be created if not exists. On the daemon exit, this file will be removed under normal conditions. This value might be NULL, in this case no any checks performed and no file will be created.

## Return value
This functions shares return values with *daemonize()* function
mentioned above with one notable addition - if daemon already running
it will return -2 to the process which starts the daemon. No
daemonization will be performed in this case.

# Examples
An example comes with this project which one could use as the template
for one's own daemon. The example can be found in `example.c`.

