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
#include <fcntl.h>
#include <signal.h>
#include <string.h>

void handler(int sig)
{
}

int main(int argc, char *argv[])
{
int fd;

	if (argc != 2){
		fprintf(stderr, "usage: %s file\n",
			argv[0]);
		return 1;
	}

	if (signal(SIGUSR1, handler) == SIG_ERR){
		fprintf(stderr, "FAIL: signal failed - %s:%s\n",
			argv[1],
			strerror(errno));
		return 1;
	}

	/* wait for signal -- this allows parent to inspect what profile
	 * confinement we have via /proc/pid/attr/current
	 */
	(void)pause();

	fd=open(argv[1], O_RDWR, 0);
	if (fd == -1){
		fprintf(stderr, "FAIL: open %s failed - %s\n",
			argv[1],
			strerror(errno));
		return 1;
	}

	close(fd);

	printf("PASS\n");

	return 0;
}
