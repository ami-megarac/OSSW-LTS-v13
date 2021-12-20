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
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int main(int argc, char *argv[])
{
extern char **environ;

	if (argc < 2){
		fprintf(stderr, "usage: %s program [args] \n", argv[0]);
		return 1;
	}

	(void)execve(argv[1], &argv[1], environ);

	fprintf(stderr, "FAILED: exec failed %s\n", strerror(errno));

	return 1;
}
