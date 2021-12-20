/*	Copyright (C) 2002-2006 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define RET_FAILURE 0
#define RET_SUCCESS 1
#define RET_CHLD_SUCCESS 2
#define RET_CHLD_FAILURE 3
#define RET_CHLD_SIGNAL 4

int interp_status(int status)
{
	int rc;

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0) {
			rc = RET_CHLD_SUCCESS;
		} else {
			rc = RET_CHLD_FAILURE;
		}
	} else {
		rc = RET_CHLD_SIGNAL;
	}

	return rc;
}

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	int retval = 1;

	if (argc < 3 || !strchr(argv[2], '=')) {
		fprintf(stderr, "Usage: %s program VAR=value\n", argv[0]);
		return 1;
	}

	putenv(strdup(argv[2]));

	pid = fork();

	if (pid > 0) {
		/* parent */
		while (wait(&status) != pid);
			/* nothing */

		if (!WIFSTOPPED(status)) {
			retval = interp_status(status);
		}

		if (retval == RET_CHLD_SUCCESS) {
			printf("PASS\n");
			retval = 0;
		} else {
			printf("FAIL: Child failed\n");
		}

	} else if (pid == 0) {
		 /* child */
		retval = execl(argv[1], argv[1], argv[2], (char *) NULL);
		return retval;
	} else {
		/* error */
		perror("FAIL: fork() failed:");
		return 1;
	}

	return retval;
}

