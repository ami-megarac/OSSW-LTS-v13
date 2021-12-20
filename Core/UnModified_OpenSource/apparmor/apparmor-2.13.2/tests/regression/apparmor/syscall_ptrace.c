/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include "changehat.h"

#define FALSE 0
#define TRUE !FALSE

int main(int argc, char *argv[])
{
	pid_t pid;
	int retval = 0;

	if (argc != 2){
		fprintf(stderr, "usage: %s\n", argv[0]);
		return 1;
	}


	pid=fork();

	if (pid){	/* parent */
		int status;

		while (wait(&status) != pid);
		retval = WEXITSTATUS(status);
	}else{
		/* change profile so that ptrace can fail */
		if (change_hat(argv[1], SD_ID_MAGIC + 1) == -1 &&
		    errno != EPERM) {
			/* confined process failed to change_hat */
			fprintf(stderr, "FAIL: changehat %s failed - %s\n",
				argv[1], strerror(errno));
			return errno;
		}
		if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1){
			fprintf(stderr, "FAIL: ptrace failed - %s\n",
				strerror(errno));
			retval = errno;
		}else{
			printf("PASS\n");
		}
	}

	return retval;
}
