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
		if (strcmp(argv[1], "nochange") != 0){
			rc = change_hat(NULL, SD_ID_MAGIC+1);
			if (rc == -1){
				fprintf(stderr, "FAIL: changehat %s failed - %s\n",
					argv[1], strerror(errno));
				exit(1);
			}
		}
		exit(do_open(argv[2]));
	}

	/* printf ("0x%x WIFEXITED = 0x%x WEXITSTATUS = 0x%x\n", waitstatus,
			WIFEXITED(waitstatus), WEXITSTATUS(waitstatus)); */

	if ((WIFEXITED(waitstatus) != 0) && (WEXITSTATUS(waitstatus) == 0)) {
		printf("PASS\n");
	} else {
		return WEXITSTATUS(waitstatus);
	}

	return 0;
}
