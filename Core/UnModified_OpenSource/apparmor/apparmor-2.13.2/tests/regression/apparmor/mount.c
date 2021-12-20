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
#include <sys/mount.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if (argc != 4) {
		fprintf(stderr, "usage: %s [mount|umount] loopdev mountpoint\n",
			argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "mount") == 0) {
		if (mount(argv[2], argv[3], "ext2", 0xc0ed0000 | MS_NODEV, NULL ) == -1) {
			fprintf(stderr, "FAIL: mount %s on %s failed - %s\n",
				argv[2], argv[3], 
				strerror(errno));
			return errno;
		}
	} else if (strcmp(argv[1], "umount") == 0) {
		if (umount(argv[3]) == -1) {
			fprintf(stderr, "FAIL: umount %s failed - %s\n",
				argv[3],
				strerror(errno));
			return errno;
		}
	} else {
		fprintf(stderr, "usage: %s [mount|umount] loopdev mountpoint\n",
			argv[0]);
		return 1;
	}

	printf("PASS\n");

	return 0;
}
