#include <sys/types.h>

/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */
#include <sys/xattr.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define msgstring "hello"
#define msglen (strlen(msgstring)+1)
/*
 *NAME xattrs-write
 *DESCRIPTION this test does setxattr on a file
 *
 *argv[1] - file to test
 *argv[2] - xattr namespace to test
 *argv[3] - "read", "write", "remove"
 */
int main(int argc, char *argv[])
{
	ssize_t len;
	char name[256];

	if (argc != 4) {
		int i;
		fprintf(stderr, "usage: %s file xattr-name_space {read,write,remove}\n",
			argv[0]);
		for (i=0; i < argc; i++) {
			fprintf(stderr, "arg%d: %s\n", i, argv[i]);
		}
		return 1;
	}
	
	snprintf(name, sizeof(name), "%s.sdtest", argv[2]);
	if (strcmp(argv[3], "read") == 0) {
		char value[256];
		len = lgetxattr(argv[1], name, &value, 256);
		if (len == -1) {
			fprintf(stderr, "FAIL: get of %s on %s.  %s\n", name,
				argv[1], strerror(errno));
			return 1;
		}
	} else if (strcmp(argv[3], "write") == 0) {
		len = lsetxattr(argv[1], name, msgstring, msglen, 0);
		if (len == -1) {
			fprintf(stderr, "FAIL: set of %s on %s.  %s\n", name,
				argv[1], strerror(errno));
			return 1;
		}
	} else if (strcmp(argv[3], "remove") == 0) {
		len = lremovexattr(argv[1], name);
		if (len == -1) {
			fprintf(stderr, "FAIL: remove of %s on %s.  %s\n",
				name, argv[1], strerror(errno));
			return 1;
		}
	} else {
		fprintf(stderr, "usage: %s invalid option %s is not one of {read, write, remove}\n", argv[0], argv[3]);
		return 1;
	}

	printf("PASS\n");

	return 0;
}
