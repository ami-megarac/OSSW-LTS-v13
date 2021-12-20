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
	unsigned long from, num;
	if (argc != 3) {
		fprintf(stderr, "usage: %s <from> <num>\n",
				argv[0]);
		return 1;
	}

	if ( (from = strtoul(argv[1], NULL, 0)) == 0) {
		if (errno == EINVAL) {
		fprintf(stderr, "FAIL: no <from> argument in '%s'\n",
				argv[1]);
		}
	}

	if ( (num = strtoul(argv[2], NULL, 0)) == 0) {
		if (errno == EINVAL) {
		fprintf(stderr, "FAIL: no <num> argument in '%s'\n",
				argv[1]);
		}
	}

	if (from > 0x3ff || (from + num) > 0x3ff) {
		fprintf(stderr, "FAIL: out of range (0x3ff)\n");
	}

	if (ioperm(from, num, 1) == -1) {
		fprintf(stderr, "FAIL: ioperm failed - %s\n",
			strerror(errno));
		return 1;
	}

	printf("PASS\n");

	return 0;
}
