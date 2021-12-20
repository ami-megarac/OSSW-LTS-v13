/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

/* this program serves as a wrapper program that does a change_hat call
 * before fork()ing and exec()ing another test prorgram. This way we can
 * perform some of the additional testcases within a subhat without
 * duplicating the fork/exec/changehat code. The big assumption that
 * must be true (and is part of subdomain's semantics is that an
 * inherited profile that is currently in a subprofile will stay that
 * way across fork()/exec(). */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <linux/unistd.h>
#include "changehat.h"

#define ATTR_FILE   "/proc/self/attr/current"

struct option long_options[] = 
{
	{ "token", 1, 0, 0 },  /* set the magic token */
	{ "manual", 1, 0, 0 }, /* manually write to /proc/self/attr/current
				  for change_hat */
	{ "exit_hat", 0, 0, 0 }, /* whether to attempt to exit the hat */
	{ "help", 0, 0, 0 },
};

static void usage (char * program) {
	fprintf(stderr, "usage: %s testprogram [arguments]\n",
		program);
	fprintf(stderr, "%s\n", "$Id$");
	exit(1);
}

int manual_change_hat (char * subprofile, char * token_string) {
	int fd, rc;
	char * buf;
	int ret = 0;

	fd = open(ATTR_FILE, O_WRONLY);
	if (fd < 0) {
		perror("FAIL: unable to open control file");
		exit(1);
	}

	rc = asprintf(&buf, "changehat %s^%s", 
			token_string ? token_string : "", 
			subprofile ? subprofile : "");
	if (rc < 0) {
		fprintf(stderr, "FAIL: asprintf failed\n");
		exit(1);
	}	

	rc = write (fd, buf, strlen(buf));
	close(fd);
	if (rc != strlen(buf)) {
		if (rc == -1) {
			ret = errno;
		} else {
			ret = EPROTO;
		}
	}

	return ret;
}

int main(int argc, char *argv[]) {
	int rc;
	pid_t pid;
	int waitstatus;
	int filedes[2];
	int c, o;
	char buf[BUFSIZ];
	unsigned long magic_token = SD_ID_MAGIC+1;
	int manual = 0;
	int exit_hat = 0;
	char * manual_string;

	while ((c = getopt_long (argc, argv, "+", long_options, &o)) != -1) {
		if (c == 0) {
			switch (o) {
			    case 0: 
			    	magic_token = strtoul (optarg, NULL, 10);
				break;
			    case 1:
			    	manual = 1;
				manual_string = 
					(char *) malloc(strlen(optarg) + 1);
				if (!manual_string) {
					fprintf(stderr, "FAIL: malloc failed\n");
					exit(1);
				}
				strcpy(manual_string, optarg);
				break;
			    case 2:
			    	exit_hat = 1;
				break;
			    case 3:
			        usage(argv[0]);
				break;
			    default:
			        usage(argv[0]);
				break;
			}
		} else {
			usage(argv[0]);
		}
	}

	if (!argv[optind])
		usage(argv[0]);

	rc = pipe(filedes);
	if (rc != 0) {
		perror("FAIL: pipe failed");
		exit(1);
	}
	
	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "FAIL: fork failed - %s\n",
			strerror(errno));
		exit(1);
	} else if (pid != 0) {
		/* parent */
		close(filedes[1]);
		read(filedes[0], &buf, sizeof(buf));
		rc = wait(&waitstatus);
		if (rc == -1){
			fprintf(stderr, "FAIL: wait failed - %s\n",
				strerror(errno));
			exit(1);
		}
	} else {
		/* child */
		char * pname = malloc (strlen(argv[optind]) + 3);
		if (!pname) {
			perror ("FAIL: child malloc");
			return -1;
		}
		sprintf (pname, "%s", argv[optind]);
		
		rc = !manual ? change_hat(argv[optind], magic_token) 
			     : manual_change_hat(argv[optind], manual_string); 
		if (rc != 0) {
			rc = !manual ? change_hat(NULL, magic_token) 
				     : manual_change_hat(NULL, manual_string); 
			fprintf(stderr, "FAIL: hat for %s does not exist\n",
				argv[optind]);
			exit(1);
		}		

		close(filedes[0]);
		fclose(stdout);
		rc = dup2(filedes[1], STDOUT_FILENO);
		if (rc < 0) {
			perror("FAIL: pipe failed");
			exit(1);
		}

		exit(execv(pname, &argv[optind]));
	}

	if (exit_hat) {
		rc = !manual ? change_hat(NULL, magic_token) 
			     : manual_change_hat(NULL, manual_string); 
		/* shouldn't fail, if it does, we've been killed */
		if (rc != 0) {
			fprintf(stderr, "FAIL: exiting hat '%s' failed\n",
				argv[optind]);
			exit(1);
		}
	}

	if ((WEXITSTATUS(waitstatus) == 0) && strcmp("PASS\n", buf) == 0) {
		printf("PASS\n");
	}

	return WEXITSTATUS(waitstatus);
}
