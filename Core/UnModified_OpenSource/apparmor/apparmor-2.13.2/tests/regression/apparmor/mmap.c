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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>

static int gotsigusr1;

void handler(int sig)
{
	++gotsigusr1;
}

int main(int argc, char *argv[])
{
int fd, rc, pass;
const char *data="hello world";
char *mptr;
char buf[128];
sigset_t sigset;


	if (argc != 2){
		fprintf(stderr, "usage: %s file\n",
			argv[0]);
		return 1;
	}

	if (signal(SIGUSR1, handler) == SIG_ERR){
		fprintf(stderr, "FAIL: internal error signal failed - %s\n",
			strerror(errno));
		return 1;
	}

	if (sigemptyset(&sigset) == -1) {
		perror ("FAIL: nullifying sigset");
		return 1;
	}

	fd=open(argv[1], O_CREAT|O_EXCL|O_RDWR, S_IWUSR|S_IRUSR);
	if (fd == -1){
		fprintf(stderr, "FAIL: create %s failed - %s\n",
			argv[1],
			strerror(errno));
		return 1;
	}

	if (ftruncate(fd, getpagesize()) == -1){
		fprintf(stderr, "FAIL: ftruncate  %s failed - %s\n",
			argv[1],
			strerror(errno));
	}

	mptr=mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (mptr == MAP_FAILED){
		fprintf(stderr, "FAIL: mmap failed - %s\n",
			strerror(errno));
		return 1;
	}

	pass=1;

nextpass:
	(void)memcpy(mptr, data, strlen(data));

	if (msync(mptr, strlen(data), MS_SYNC) == -1){
		fprintf(stderr, "FAIL: msync failed - %s\n",
			strerror(errno));
		return 1;
	}


	(void)lseek(fd, 0, SEEK_SET);
	rc=read(fd, buf, strlen(data));

	if (rc != strlen(data)){
		fprintf(stderr, "FAIL: read failed - %s\n",
			strerror(errno));
		return 1;
	}

	if (memcmp(buf, mptr, strlen(data)) != 0){
		fprintf(stderr, "FAIL: read comparison failed\n");
	}

	gotsigusr1=0;

	if (pass == 1){
		while (!gotsigusr1){
			(void)sigsuspend(&sigset);
		}

		pass++;
		goto nextpass;
	}

	printf("PASS\n");

	return 0;
}
