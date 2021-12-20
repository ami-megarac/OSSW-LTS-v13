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

const char *data="hello world";

int do_read (int fd)
{
	int rc;
	char buf[128];

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

int do_parent (char * hat, char * file)
{
	int fd;

	/* change hat if hatname != nochange */
	if (strcmp(hat, "nochange") != 0){
		if (change_hat(hat, SD_ID_MAGIC+1) == -1){
			fprintf(stderr, "FAIL: changehat %s failed - %s\n",
				hat, strerror(errno));
			exit(1);
		}
	}

	if (alarm(5) != 0) {
		fprintf(stderr, "FAIL: alarm already set\n");
		exit(1);
	}

	fd=open(file, O_RDONLY, 0);
	if (fd == -1){
		fprintf(stderr, "FAIL: open read %s failed - %s\n",
			file,
			strerror(errno));
		return 1;
	}

	alarm(0);

	return(do_read(fd));
}

int do_child (char * hat, char * file)
{
	int fd;

	/* change hat if hatname != nochange */
	if (strcmp(hat, "nochange") != 0){
		if (change_hat(hat, SD_ID_MAGIC+1) == -1){
			fprintf(stderr, "FAIL: changehat %s failed - %s\n",
				hat, strerror(errno));
			exit(1);
		}
	}

	fd=open(file, O_WRONLY, 0);
	if (fd == -1){
		fprintf(stderr, "FAIL: open write %s failed - %s\n",
			file,
			strerror(errno));
		return 1;
	}

	return (do_write(fd));
}

pid_t pid = -1;

void kill_child(void)
{
	if (pid > 0)
		kill(pid, SIGKILL);
}

void sigalrm_handler(int sig)
{
	fprintf(stderr, "FAIL: parent timed out waiting for child\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int rc;
	int waitstatus;
	int read_error = 0;

	if (argc != 4){
		fprintf(stderr, "usage: %s parent_hatname child_hatname filename\n",
			argv[0]);
		return 1;
	}

	if (signal(SIGALRM, sigalrm_handler) == SIG_ERR) {
		fprintf(stderr, "FAIL: signal failed - %s\n",
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
		atexit(kill_child);
		read_error = do_parent(argv[1], argv[3]);
		rc = wait(&waitstatus);
		if (rc == -1){
			fprintf(stderr, "FAIL: wait failed - %s\n",
				strerror(errno));
			exit(1);
		}
	} else {
		/* child */
		exit(do_child(argv[2], argv[3]));
	}

	if ((WIFEXITED(waitstatus) != 0) && (WEXITSTATUS(waitstatus) == 0) 
			&& read_error == 0) {
		printf("PASS\n");
	} else {
		return (WEXITSTATUS(waitstatus) & read_error);
	}

	return 0;
}
