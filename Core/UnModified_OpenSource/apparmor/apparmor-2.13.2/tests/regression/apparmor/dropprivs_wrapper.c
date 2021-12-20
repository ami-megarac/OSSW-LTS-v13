/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

/* this is a simple wrapper program that attempts to drop privileges
 * before exec()ing the first argument passed in. This is at least
 * useful for the owlsm hardlink test, which can't run as root. */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#define __USE_GNU
#include <unistd.h>

#define NOPRIVUSER "nobody"
#define NOPRIVUID 65534

int main(int argc, char *argv[])
{
	int rc;
	uid_t newuid;
	struct passwd * passwd;
	char * pname = NULL;

	if (argc < 2){
		fprintf(stderr, "usage: %s testprogram [arguments]\n",
			argv[0]);
		return 1;
	}

	while ((passwd = getpwent())) {
		/* printf ("DEBUG: %s\n", passwd->pw_name); */
		if (strcmp (passwd->pw_name, NOPRIVUSER) == 0)
			break;
	}

	if (passwd) {
		newuid = passwd->pw_uid;
	} else {
		newuid = (uid_t) NOPRIVUID;
	}

	rc = setresuid (newuid, newuid, newuid);
	if (rc != 0) {
		perror ("FAIL: attempted to drop privs");
		return errno;
	}

	pname = malloc (strlen(argv[1]) + 3);
	if (!pname) {
		perror ("FAIL: malloc");
		return 1;
	}
	sprintf (pname, "./%s", argv[1]);
	
	exit(execv(pname, &argv[1]));
}
