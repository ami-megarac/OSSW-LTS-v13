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
#include <string.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
	if (argc != 3){
		fprintf(stderr, "usage: %s [mkdir|rmdir] dir\n",
			argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "mkdir") == 0) {
		if (mkdir(argv[2], S_IRWXU)) {
			fprintf(stderr, "FAIL: mkdir %s failed - %s\n",
				argv[2],
				strerror(errno));
			return errno;
		}
	} else if (strcmp(argv[1], "rmdir") == 0) {
		if (rmdir(argv[2])) {
			fprintf(stderr, "FAIL: rmdir %s failed - %s\n",
				argv[2],
				strerror(errno));
			return errno;
		}
	} else {
		fprintf(stderr, "usage: %s [mkdir|rmdir] dir\n",
			argv[0]);
		return 1;
	}

	printf("PASS\n");

	return 0;
}
