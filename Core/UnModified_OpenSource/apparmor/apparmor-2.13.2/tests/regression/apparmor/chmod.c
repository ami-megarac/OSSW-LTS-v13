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
#include <string.h>

int main(int argc, char *argv[])
{
	mode_t mode;

	if (argc != 3) {
		fprintf(stderr, "usage: %s file mode\n",
			argv[0]);
		return 1;
	}

	if (sscanf(argv[2], "%o", &mode) != 1) {
		fprintf(stderr, "FAIL: bad mode %s\n", argv[2]);
		return 1;
	}
	
	if (chmod(argv[1], mode) == -1) {
		fprintf(stderr, "FAIL: fchmod %s %o failed - %s\n",
			argv[1], mode, strerror(errno));
		return 1;
	}

	printf("PASS\n");

	return 0;
}
