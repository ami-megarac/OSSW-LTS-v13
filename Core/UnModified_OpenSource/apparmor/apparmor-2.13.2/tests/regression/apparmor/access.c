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
int rc, mode;
char *path, *perm, *tmp;

	if (argc != 3){
		fprintf(stderr, "usage: %s path mode\n",
			argv[0]);
		return 1;
	}

	path=argv[1];
	perm=argv[2];

	tmp = perm;
	mode = 0;
	while (*tmp){
		switch (*tmp){
			case 'r':  mode |= R_OK; break;
			case 'w':  mode |= W_OK; break;
			case 'x':  mode |= X_OK; break;
			default:   fprintf(stderr, "Invalid perm '%c'\n",
					*tmp);
				   return 1;
		}
		tmp++;
	}
	

	rc=access(path, mode);
	if (rc == -1){
		fprintf(stderr, "FAIL: access %s %s failed - %s\n",
			path, perm, strerror(errno));
		return 1;
	}

	printf("PASS\n");

	return 0;
}
