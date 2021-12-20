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
#include <sys/sysctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define BUFSIZE 4096

static int name[] = {CTL_KERN, KERN_MAX_THREADS};

int read_max_threads(int *max_threads)
{
	size_t save_sz = sizeof(*max_threads);

	if (sysctl(name, sizeof(name), max_threads, &save_sz, NULL, 0) == -1){
		fprintf(stderr, "FAIL: sysctl read failed - %s\n",
			strerror(errno));
		return 1;
	}
	return 0;
}

int write_max_threads(int new_max_threads)
{
	size_t save_sz = sizeof(new_max_threads);

	if (sysctl(name, sizeof(name), NULL, 0, &new_max_threads, save_sz) == -1){
		fprintf(stderr, "FAIL: sysctl write failed - %s\n",
			strerror(errno));
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int save_max_threads, new_max_threads, read_new_max_threads;
	int readonly = 0;
	
	if ((argc > 1) && strcmp(argv[1],"ro") == 0)
		readonly = 1;

	if (read_max_threads(&save_max_threads) != 0)
		return 1;

	/* printf("Kernel max threads (saved) is %d\n", save_max_threads); */

	if (readonly) {
		printf ("PASS\n");
		return 0;
	}

	new_max_threads = save_max_threads + 1024;

	if (write_max_threads(new_max_threads) != 0)
		return 1;

	if (read_max_threads(&read_new_max_threads) != 0)
		return 1;

	/* printf("Kernel max threads (new) is %d\n", read_new_max_threads); */

	if (read_new_max_threads != new_max_threads) {
		/* the kernel possibly rejected our updated max threads
		 * as being too large; try decreasing max threads. */

		new_max_threads = save_max_threads - 1024;

		if (write_max_threads(new_max_threads) != 0)
			return 1;

		if (read_max_threads(&read_new_max_threads) != 0)
			return 1;

		/* printf("Kernel max threads (new, 2nd attempt) is %d\n", read_new_max_threads); */

		if (read_new_max_threads != new_max_threads) {
			fprintf(stderr, "FAIL: read value does not match written values\n");
			return 1;
		}
	}

	if (write_max_threads(save_max_threads) != 0)
		return 1;

	if (read_max_threads(&read_new_max_threads) != 0)
		return 1;

	/* printf("Kernel max threads (saved) is %d\n", read_new_max_threads);*/

	if (read_new_max_threads != save_max_threads) {
		fprintf(stderr, "FAIL: read value does not match written values\n");
		return 1;
	}

	printf("PASS\n");

	return 0;
}
