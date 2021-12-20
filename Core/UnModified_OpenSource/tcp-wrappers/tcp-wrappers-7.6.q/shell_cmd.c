 /*
  * shell_cmd() takes a shell command after %<character> substitutions. The
  * command is executed by a /bin/sh child process, with standard input,
  * standard output and standard error connected to /dev/null.
  * 
  * Diagnostics are reported through syslog(3).
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) shell_cmd.c 1.5 94/12/28 17:42:44";
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

extern void exit();

/* Local stuff. */

#include "tcpd.h"

/* Forward declarations. */

static void do_child();

/*
 * The sigchld handler. If there is a SIGCHLD caused by a child other than
 * ours, we set a flag and raise the signal later.
 */
volatile static int foreign_sigchld;
volatile static int our_child_pid;
static void sigchld(int sig, siginfo_t *si, void *unused)
{
    if (si && si->si_pid != our_child_pid)
	foreign_sigchld = 1;
}

/* shell_cmd - execute shell command */

void    shell_cmd(command)
char   *command;
{
    int     child_pid;

    struct sigaction new_action, old_action;
    sigset_t new_mask, old_mask, empty_mask;

    new_action.sa_sigaction = &sigchld;
    new_action.sa_flags = SA_SIGINFO;
    sigemptyset(&new_action.sa_mask);
    sigemptyset(&new_mask);
    sigemptyset(&empty_mask);
    sigaddset(&new_mask, SIGCHLD);

    /*
     * Set the variables for handler, set the handler and block the signal
     * until we have the pid.
     */
    foreign_sigchld = 0; our_child_pid = 0;
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);
    sigaction(SIGCHLD, &new_action, &old_action);

    /*
     * Most of the work is done within the child process, to minimize the
     * risk of damage to the parent.
     */

    switch (child_pid = fork()) {
    case -1:					/* error */
	tcpd_warn("cannot fork: %m");
	break;
    case 00:					/* child */
	/* Clear the blocked mask for the child not to be surprised. */
	sigprocmask(SIG_SETMASK, &empty_mask, 0);
	do_child(command);
	/* NOTREACHED */
    default:					/* parent */
	our_child_pid = child_pid;
	sigprocmask(SIG_UNBLOCK, &new_mask, 0);
	while (waitpid(child_pid, (int *) 0, 0) == -1 && errno == EINTR);
    }

    /*
     * Revert the signal mask and the SIGCHLD handler.
     */
    sigprocmask(SIG_SETMASK, &old_mask, 0);
    sigaction(SIGCHLD, &old_action, 0);

    /* If there was a foreign SIGCHLD, raise it after we have restored the old
     * mask and handler. */
    if (foreign_sigchld)
	raise(SIGCHLD);
}

/* do_child - exec command with { stdin, stdout, stderr } to /dev/null */

static void do_child(command)
char   *command;
{
    char   *error;
    int     tmp_fd;

    /*
     * Systems with POSIX sessions may send a SIGHUP to grandchildren if the
     * child exits first. This is sick, sessions were invented for terminals.
     */

    signal(SIGHUP, SIG_IGN);

    /* Set up new stdin, stdout, stderr, and exec the shell command. */

    for (tmp_fd = 0; tmp_fd < 3; tmp_fd++)
	(void) close(tmp_fd);
    if (open("/dev/null", 2) != 0) {
	error = "open /dev/null: %m";
    } else if (dup(0) != 1 || dup(0) != 2) {
	error = "dup: %m";
    } else {
	(void) execl("/bin/sh", "sh", "-c", command, (char *) 0);
	error = "execl /bin/sh: %m";
    }

    /* Something went wrong. We MUST terminate the child process. */

    tcpd_warn(error);
    _exit(0);
}
