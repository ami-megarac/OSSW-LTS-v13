/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s dstfile srcfile\n",
			argv[0]);
		return 1;
	}

	if (symlink(argv[1], argv[2]) == 0) {
		printf("PASS\n");
	} else {
		printf("FAIL - %s\n", strerror(errno));
	}

	return 0;
}
