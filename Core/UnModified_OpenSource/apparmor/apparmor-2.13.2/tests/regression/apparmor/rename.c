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
	int retval;

	if (argc != 3){
		fprintf(stderr, "usage: %s oldfile newfile\n",
			argv[0]);
		return 1;
	}

	retval=rename(argv[1], argv[2]);
	if (retval == -1){
		fprintf(stderr, "FAIL: rename from %s to %s failed - %s\n",
			argv[1], argv[2],
			strerror(errno));
		return errno;
	}

	printf("PASS\n");

	return 0;
}
