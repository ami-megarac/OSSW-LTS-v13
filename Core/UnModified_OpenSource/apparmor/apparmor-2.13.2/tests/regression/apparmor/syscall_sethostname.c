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

/* this implicitly tests gethostname, too. */

#define RESET_HOSTNAME_WHEN_DONE

#define BUFSIZE 4096
int main(int argc, char *argv[])
{
	char saved_hostname[BUFSIZE];
	char new_hostname[BUFSIZE];
	size_t len = sizeof(saved_hostname);
	size_t newlen;
	int error = 0;

	if (argc != 2) {
		fprintf(stderr, "usage: %s hostname\n",
				argv[0]);
		return 1;
	}

	newlen = strlen(argv[1]);
	if (newlen <= 0 || newlen >= BUFSIZE) {
		fprintf(stderr, "FAIL: invalid hostname '%s'\n",
				argv[1]);
		return 1;
	}
	
	if (gethostname(saved_hostname, len) == -1) {
		fprintf(stderr, "FAIL: gethostname failed - %s\n",
			strerror(errno));
		return 1;
	}
	/* printf("old hostname is %s\n", saved_hostname); */

	if (sethostname(argv[1], strlen(argv[1])) == -1) {
		fprintf(stderr, "FAIL: sethostname failed - %s\n",
			strerror(errno));
		return 1;
	}

	len = sizeof(new_hostname);
	if (gethostname(new_hostname, len) == -1) {
		fprintf(stderr, "FAIL: gethostname failed - %s\n",
			strerror(errno));
		error = 1;
		goto cleanup;
	}

	if (strcmp(new_hostname, argv[1]) != 0) {
		fprintf(stderr, "FAIL: attempted to set hostname to '%s', but "
				"'%s' was the result\n",
			argv[1],
			new_hostname);
		error = 1;
		goto cleanup;
	}

cleanup:
#ifdef RESET_HOSTNAME_WHEN_DONE
	if (sethostname(saved_hostname, strlen(saved_hostname)) == -1) {
		fprintf(stderr, "FAIL: sethostname failed restting to old name - %s\n",
			strerror(errno));
		error = 1;
	}
#endif /* RESET_HOSTNAME_WHEN_DONE */
	
	if (error == 0)
		printf("PASS\n");

	return error;
}
