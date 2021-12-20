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
#include <sys/io.h> /* for glibc */
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int ring;
	if (argc != 2) {
		fprintf(stderr, "usage: %s <ring>\n",
				argv[0]);
		return 1;
	}

	if ( (ring = strtol(argv[1], NULL, 0)) == 0) {
		if (errno == EINVAL) {
		fprintf(stderr, "FAIL: no <ring> argument in '%s'\n",
				argv[1]);
		}
	}

	if (ring > 3 || ring < 0) {
		fprintf(stderr, "FAIL: out of range (0-3)\n");
	}

	if (iopl(ring) == -1) {
		fprintf(stderr, "FAIL: iopl failed - %s\n",
			strerror(errno));
		return 1;
	}

	printf("PASS\n");

	return 0;
}
