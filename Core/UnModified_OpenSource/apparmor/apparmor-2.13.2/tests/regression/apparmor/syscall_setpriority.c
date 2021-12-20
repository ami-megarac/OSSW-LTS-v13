/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>


int
main (int argc, char * argv[]) {
	int rc;
	int niceval;

	if (argc == 2) {
		niceval = strtol (argv[1], NULL, 10);
	} else {
		niceval = -5;
	}
	
	rc = setpriority(PRIO_PROCESS, 0, niceval);
	if (rc != 0) {
		perror ("FAIL: setpriority failed");
		return errno;
	}

	printf ("PASS\n");

	return 0;
}

