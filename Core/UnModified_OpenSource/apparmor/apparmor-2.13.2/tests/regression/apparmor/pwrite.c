/* pwrite/pread - matt */

/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

static int gotsignal;

void handler (int sig) {
	++gotsignal;
}

int main (int argc, char *argv[]) {
	int fd, pass;
	ssize_t ret;
	const char * bugger = "pwrite test";
	char in_buffer[strlen(bugger) +1];
	sigset_t sigset;	

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return(1);
	}

	if (signal(SIGUSR1, handler) == SIG_ERR) {
		fprintf(stderr, "FAIL - signal failed %s\n",
			strerror(errno));
		return(1);
	}
	
	if (sigemptyset(&sigset) == -1) {
		fprintf(stderr, "FAIL - sigemptyset\n");
		return(1);
	}

	fd = open(argv[1], O_CREAT|O_RDWR, S_IWUSR|S_IRUSR);
	if (fd == -1) {
		fprintf(stderr, "FAIL: open %s failed - %s\n",
			argv[1], strerror(errno));
		return(1);
	}
	pass=1;
	/* Try to write to the file */
nextpass:
	ret = pwrite(fd, bugger, strlen(bugger), 0);
	if (ret == 0) {
		fprintf(stderr, "FAIL: pwrite failed - 0 bytes written\n");
		return(1);
	} else if (ret == -1) { 
		fprintf(stderr, "FAIL: pwrite failed - %s\n",
			strerror(errno));
		return(1);
	} else if (ret != strlen(bugger)) {
		fprintf(stderr, "FAIL: pwrite failed - wrote %zi expected %zi\n", 
			ret, strlen(bugger));
		return(1);
	}
	
	/* Cool, now try to read the file */
	ret = pread(fd, in_buffer, strlen(bugger), 0);
	if (ret <= 0) {
		fprintf(stderr, "FAIL: pread failed - %s\n",
			strerror(errno));
		return(1);
	}  else if (ret != strlen(bugger)) {
		fprintf(stderr, "FAIL: pread failed - read %zi expected %zi\n",
			ret, strlen(bugger));
		return(1);
	} else if (memcmp(bugger, in_buffer, strlen(bugger)) != 0) {
		fprintf(stderr, "FAIL: pread comparison mismatch: %s vs. %s\n",
			bugger, in_buffer);
		return(1);
	}

	gotsignal=0;
	
	if (pass==1) {
		while (!gotsignal) {
			(void)sigsuspend(&sigset);
		}

		pass++;
		goto nextpass;
	} 
	printf("PASS\n");
	close(fd);
	return(0);	 
}	

