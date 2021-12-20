/*
 *	Copyright (C) 2006 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/syscall.h>
#include "changehat.h"

void static inline dump_me(const char *msg)
{
	//printf("(%d/%ld/0x%lx) %s\n", getpid(), syscall(SYS_gettid), pthread_self(), msg);
}

static int is_done;
static int thread_rc;

void *worker (void *param)
{
	char *file = (char *) param;
	int rc;

	dump_me("worker called, changing hat");
	change_hat ("fez", 12345);
	rc = do_open(file);
	dump_me("worker changed hat");
	change_hat (NULL, 12345);
	if (rc == 0) {
		printf("PASS\n");
	}
	thread_rc = rc;
	is_done = 1;
	return 0;
}

int main (int argc, char **argv)
{
	pthread_t child_process;
	if (argc != 2) {
		fprintf(stderr, "FAIL: usage '%s <filename>'\n", argv[0]);
		exit(1);
	}

	dump_me("main");
	pthread_create(&child_process, NULL, worker, argv[1]);
	while (!is_done) {
		sleep(1);
	}
	exit(thread_rc);
}
