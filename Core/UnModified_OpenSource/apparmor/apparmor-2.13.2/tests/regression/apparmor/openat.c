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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>

struct option long_options[] = {
        {"delete", 0, 0, 'd'},  /* delete the directory after opening */
        {"rename", 1, 0, 'r'},  /* rename the directory to -- */
        {"help", 0, 0, 'h'},
};

static void usage (char * program)
{
	fprintf(stderr, "usage: %s [--delete] dir file\n", program);
	fprintf(stderr, "%s\n", "$Id$");
	exit(1);
}

int main(int argc, char *argv[])
{
	int fd = -1, dirfd = -1;
	int c;
	int do_delete = 0;
	int do_rename = 0;
	char *dir, *file, *newdir = NULL;

	while ((c = getopt_long (argc, argv, "+hdr:", long_options, NULL)) != -1) {
		switch (c) {
		    case 'd':
			do_delete = 1;
			break;
		    case 'r':
			do_rename = 1;
			newdir = optarg;
			break;
		    case 'h':
			/* fall through */
		    default:
			usage(argv[0]);
			break;
		}
	}

	if (argc - optind != 2)
		usage(argv[0]);

	dir = argv[optind];
	file = argv[optind + 1];

	dirfd = open(dir, O_RDONLY | O_DIRECTORY);
	if (dirfd == -1) {
		fprintf(stderr, "FAIL: open %s failed - %s\n",
			dir, strerror(errno));
		return 1;
	}

	if (do_delete && rmdir(dir) == -1) {
		fprintf(stderr, "FAIL: rmdir %s failed - %s\n",
			dir, strerror(errno));
		return 1;
	} else if (do_rename && rename(dir, newdir) == -1) {
		fprintf(stderr, "FAIL: rename %s, %s failed - %s\n",
			dir, newdir, strerror(errno));
		return 1;
	}

	fd = openat(dirfd, file, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
	if (fd == -1) {
		fprintf(stderr, "FAIL: openat %s failed - %s\n",
			file, strerror(errno));
		close(dirfd);
		return 1;
	}

	close(fd);
	close(dirfd);

	printf("PASS\n");

	return 0;
}
