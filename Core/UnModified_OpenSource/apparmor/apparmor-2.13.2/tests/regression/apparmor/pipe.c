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
#include <linux/unistd.h>
#include <string.h>
#include <stdlib.h>
#include "changehat.h"

const char *data="hello world";

int do_read (int fd)
{
	int rc;
	char buf[128];

	if (fd < 0) {
                fprintf(stderr, "FAIL: read failed - no descriptor passed\n");
                return 1;
        }

        rc=read(fd, buf, sizeof(buf));

        if (rc != strlen(data)){
                fprintf(stderr, "FAIL: read failed - %s\n",
                        strerror(errno));
                return 1;
        }

        if (memcmp(buf, data, strlen(data)) != 0){
                fprintf(stderr, "FAIL: comparison failed - %s\n",
                        strerror(errno));
                return 1;
        }

	close(fd);

	return 0;
}

int do_write (int fd)
{
	int rc;

        rc=write(fd, data, strlen(data));

        if (rc != strlen(data)){
                fprintf(stderr, "FAIL: write failed - %s\n",
                        strerror(errno));
                return 1;
        }

	close(fd);

	return 0;
}

int main(int argc, char *argv[])
{
	int rc;
	pid_t pid;
	int waitstatus;
	int filedes[2];
	int read_error = 0;

	if (argc != 2){
		fprintf(stderr, "usage: %s hatname\n",
			argv[0]);
		return 1;
	}

	/* change hat if hatname != nochange */
	if (strcmp(argv[1], "nochange") != 0){
		rc = change_hat(argv[1], SD_ID_MAGIC+1);
		if (rc == -1){
			fprintf(stderr, "FAIL: changehat %s failed - %s\n",
				argv[1], strerror(errno));
			exit(1);
		}
	}

	if (pipe(filedes) == -1){
		fprintf(stderr, "FAIL: pipe() failed - %s\n",
			strerror(errno));
		exit(1);
	}

	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "FAIL: fork failed - %s\n",
			strerror(errno));
		exit(1);
	} else if (pid != 0) {
		/* parent */
		read_error = do_read(filedes[0]);
		rc = wait(&waitstatus);
		if (rc == -1){
			fprintf(stderr, "FAIL: wait failed - %s\n",
				strerror(errno));
			exit(1);
		}
	} else {
		/* child */
		exit(do_write(filedes[1]));
	}

	if ((WIFEXITED(waitstatus) != 0) && (WEXITSTATUS(waitstatus) == 0) 
			&& read_error == 0) {
		printf("PASS\n");
	} else {
		return (WEXITSTATUS(waitstatus) & read_error);
	}

	return 0;
}
