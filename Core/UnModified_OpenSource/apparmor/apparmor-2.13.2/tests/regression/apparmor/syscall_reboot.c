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
#include <sys/reboot.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int cmd = RB_ENABLE_CAD;
	
	if (argc != 2) {
		fprintf(stderr, "Usage %s [on|off|reboot]\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "off") == 0) 
		cmd = RB_DISABLE_CAD;
	else if (strcmp(argv[1], "on") == 0) 
		/* dangerous, a CAD will do a forced reboot (no shutdown) */
		cmd = RB_ENABLE_CAD;
	else if (strcmp(argv[1], "reboot") == 0) 
		/* Aiieee, you could lose data if you do this */
		cmd = RB_AUTOBOOT;
	else {
		fprintf(stderr, "Usage %s [on|off|reboot]\n", argv[0]);
		return 1;
	}
		
	if (reboot(cmd) == -1){
		fprintf(stderr, "FAIL: reboot failed - %s\n",
			strerror(errno));
		return 1;
	}

	printf("PASS\n");

	return 0;
}
