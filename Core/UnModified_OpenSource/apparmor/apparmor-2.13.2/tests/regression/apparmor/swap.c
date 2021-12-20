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
#include <sys/swap.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s [on|off] swapfile\n",
			argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "on") == 0) {
		if (swapon(argv[2], 0) == -1) {
			fprintf(stderr, "FAIL: swapon %s failed - %s\n",
				argv[2],
				strerror(errno));
			return errno;
		}
	} else if (strcmp(argv[1], "off") == 0) {
		if (swapoff(argv[2]) == -1) {
			fprintf(stderr, "FAIL: swapoff %s failed - %s\n",
				argv[2],
				strerror(errno));
			return errno;
		}
	} else {
		fprintf(stderr, "usage: %s [on|off] swapfile\n",
			argv[0]);
		return 1;
	}

	printf("PASS\n");

	return 0;
}
