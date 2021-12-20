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
	int mode;

	if (argc != 3){
		fprintf(stderr, "usage: %s b|c|f|s|r file\n",
			argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "b") == 0){
		mode=S_IFBLK;
	}else if (strcmp(argv[1], "c") == 0){
		mode=S_IFCHR;
	}else if (strcmp(argv[1], "f") == 0){
		mode=S_IFIFO;
	}else if (strcmp(argv[1], "s") == 0){
		mode=S_IFSOCK;
	}else if (strcmp(argv[1], "r") == 0){
		mode=S_IFREG;
	}else{
		fprintf(stderr, "FAIL: type must be b|c|f|s|r\n");
		return 1;
	}

	if (mknod(argv[2], mode, 1) == -1){
		fprintf(stderr, "FAIL: mknod %s failed - %s\n",
			argv[1],
			strerror(errno));
		return 1;
	}

	printf("PASS\n");

	return 0;
}
