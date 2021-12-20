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
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <linux/unistd.h>

#include "changehat.h"

int main(int argc, char *argv[])
{
	if (argc != 2){
		fprintf(stderr, "usage: %s profile\n",
			argv[0]);
		return 1;
	}

	/* change hat if hatname != nochange */
	if (strcmp(argv[1], "nochange") != 0){
		if (change_hat(argv[1], SD_ID_MAGIC+1)) {
			/* In a null-profile at this point, all file 
			 * i/o is restricted including stdout.
			 * Need to change back to parent.  If the 
			 * changehat back fails, no point trying to print
			 * any error, so (void) call.
			 */
			(void)change_hat(NULL, SD_ID_MAGIC+1);
			fprintf(stderr, "FAIL: changehat %s failed - %s\n",
				argv[1], strerror(errno));
			exit(1);
		}
	}

	if (strcmp(argv[1], "nochange") != 0) {
		if (change_hat(NULL, SD_ID_MAGIC ^ 0xdeadbeef)) {
			fprintf(stderr, "FAIL: changehat %s failed - %s\n",
				argv[1], strerror(errno));
			exit(1);
		}
	}

	printf("PASS\n");
	return 0;
}
