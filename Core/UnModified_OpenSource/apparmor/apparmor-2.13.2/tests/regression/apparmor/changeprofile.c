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
#include <stdlib.h>
#include <linux/unistd.h>

#include <sys/apparmor.h>
#include "changehat.h"

int main(int argc, char *argv[])
{
	int rc; 

	if (argc != 3){
		fprintf(stderr, "usage: %s profile file\n",
			argv[0]);
		return 1;
	}

	/* change profile if profile name != nochange */
	if (strcmp(argv[1], "nochange") != 0){
		rc = aa_change_profile(argv[1]);
		if (rc == -1){
			fprintf(stderr, "FAIL: changeprofile %s failed - %s\n",
				argv[1], strerror(errno));
			exit(errno);
		}
	}

	rc = do_open(argv[2]);
	if (rc == 0)
		printf("PASS\n");

	return rc;
}
