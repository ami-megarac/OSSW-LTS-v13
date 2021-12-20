/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *	Copyright (C) 2013 Canonical Ltd.
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

int main(int argc, char *argv[])
{
	int rc; 
	char *profile, *mode;

        if (argc < 3 || argc > 4) {
                fprintf(stderr, "usage: %s <task> <expected profile> [<expect mode>]\n",
                        argv[0]);
                return 1;
        }

        if (strcmp(argv[1], "self") == 0){
		rc = aa_getcon(&profile, &mode);
		if (rc == -1) {
			int serrno = errno;
                        fprintf(stderr,
				"FAIL: introspect_confinement %s failed - %s\n",
                                argv[1], strerror(errno));
                        exit(serrno);
		}
	} else {
		char *end;
		pid_t pid = strtol(argv[1], &end, 10);
		if (end == argv[1] || *end != 0) {
			int serrno = errno;
			fprintf(stderr,
				"FAIL: query_confinement - invalid pid: %s\n",
				argv[1]);
			exit(serrno);
		} else {
			rc = aa_gettaskcon(pid, &profile, &mode);
			if (rc == -1) {
				int serrno = errno;
				fprintf(stderr,
					"FAIL: query_confinement %s failed - %s\n",
					argv[1], strerror(errno));
				exit(serrno);
			}
		}
	}
	if (strcmp(profile, argv[2]) != 0) {
		fprintf(stderr,
			"FAIL: expected confinement \"%s\" != \"%s\"\n", argv[2],
			profile);
		exit(1);
	}
	if (mode) {
		if (rc != strlen(profile) + strlen(mode) + 4) {
			/* rc includes mode. + 2 null term + 1 ( + 1 space */
			fprintf(stderr,
				"FAIL: expected return len %zd != actual %d\n",
				strlen(profile) + strlen(mode) + 4, rc);
			exit(1);
		}
	} else if (rc != strlen(profile) + 1) {
		/* rc includes null termination */
		fprintf(stderr,
			"FAIL: expected return len %zd != actual %d\n",
			strlen(profile) + 1, rc);
		exit(1);
	}
	if (argv[3] && (!mode || strcmp(mode, argv[3]) != 0)) {
		fprintf(stderr,
			"FAIL: expected mode \"%s\" != \"%s\"\n", argv[3],
			mode ? mode : "(null)");
		exit(1);
	}
	free(profile);

        printf("PASS\n");
	return 0;
}
