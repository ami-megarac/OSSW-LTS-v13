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
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <linux/unistd.h>

#include "changehat.h"

int main(int argc, char *argv[])
{
	int rc;
	pid_t pid;
	int waitstatus;
	int innullprofile=0;

	if (argc != 3){
		fprintf(stderr, "usage: %s profile file\n",
			argv[0]);
		return 1;
	}

	/* change hat if hatname != nochange */
	if (strcmp(argv[1], "nochange") != 0){
		rc = change_hat(argv[1], SD_ID_MAGIC+1);
		/* we want to test what happens when change_hat fails,
		 * changehat.c tests will fail if you can't change_hat
		 * at all. */
		if (rc != 0){
			/* changehat failed, we are likely in a null
			 * profile at this point
			 */
			innullprofile=1;
		}
	}

	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "FAIL: fork failed - %s\n",
			strerror(errno));
		exit(1);
	} else if (pid != 0) {
		/* parent */
		rc = wait(&waitstatus);
		if (rc == -1){
			fprintf(stderr, "FAIL: wait failed - %s\n",
				strerror(errno));
			exit(1);
		}
	} else {
		/* child */
		exit(do_open(argv[2]));
	}

	/* If in a null profile, change hat back to parent */
	if (strcmp(argv[1], "nochange") != 0 && innullprofile){
		rc = change_hat(NULL, SD_ID_MAGIC+1);
	}

	if ((WIFEXITED(waitstatus) != 0) && (WEXITSTATUS(waitstatus) == 0)) {
		printf("PASS\n");
	} else {
		/* if in null profile, FAIL output from do_open will
		 * have been suppressed,  so output a FAIL here now
		 * that we have changehatted back to parent
		 */
		if (innullprofile){
			printf("FAIL %d\n", WEXITSTATUS(waitstatus));
		}
	}

	return 0;
}
