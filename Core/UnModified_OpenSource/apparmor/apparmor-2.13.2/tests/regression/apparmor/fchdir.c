/*
 *	Copyright (C) 2002-2007 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int dirfd;
	char *dir = argv[1];

	if (argc != 2){
		fprintf(stderr, "usage: %s dir\n",
			argv[0]);
		return 1;
	}

        dirfd = open(dir, O_RDONLY | O_DIRECTORY);
	if (dirfd == -1) {
		fprintf(stderr, "FAIL: open %s failed - %s\n",
			dir, strerror(errno));
		return 1;
	}

	if (fchdir(dirfd) == 0) {
		printf("PASS\n");
	} else {
		printf("FAIL - %s\n", strerror(errno));
	}

	return 0;
}
