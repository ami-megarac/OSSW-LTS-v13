/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

/* test whether the sched_setscheduler(2) syscall is allowed. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sched.h>

int
main (int argc, char * argv[]) {
	struct sched_param sp, saved_sp, new_sp;
	int saved_sched_policy, new_sched_policy;

	/* save the default policy and priority */
	memset(&saved_sp, 0, sizeof(saved_sp));
	saved_sched_policy = sched_getscheduler(0);
	if (saved_sched_policy < 0) {
		perror ("FAIL: Couldn't get scheduler policy\n");
		saved_sched_policy = SCHED_OTHER;
		return errno;
	}
	saved_sp.sched_priority = sched_get_priority_max(saved_sched_policy);

	memset(&sp, 0, sizeof(sp));
	sp.sched_priority = sched_get_priority_max(SCHED_RR) - 1;
	if (sched_setscheduler(0, SCHED_RR, &sp)) {
		perror("FAIL: Can't set SCHED_RR");
		return errno;
	}

	memset(&new_sp, 0, sizeof(new_sp));
	new_sched_policy = sched_getscheduler(0);
	if (new_sched_policy != SCHED_RR) {
		fprintf (stderr, "FAIL: set policy is %d, not SCHED_RR\n",
			new_sched_policy);
		return 1;
	}

	printf ("PASS\n");

	return 0;
}
