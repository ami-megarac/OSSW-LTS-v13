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
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	if (mlockall(MCL_CURRENT) == -1) {
		perror("FAIL: mlockall failed");
		return 1;
	}

	printf("PASS\n");

	return 0;
}
