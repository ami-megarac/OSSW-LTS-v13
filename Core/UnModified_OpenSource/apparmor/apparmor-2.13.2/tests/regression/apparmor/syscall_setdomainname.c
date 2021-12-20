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

/* this implicitly tests getdomainname, too. */

#define RESET_DOMAIN_WHEN_DONE

#define BUFSIZE 4096
int main(int argc, char *argv[])
{
	char saved_domain[BUFSIZE];
	char new_domain[BUFSIZE];
	size_t len = sizeof(saved_domain);
	size_t newlen;
	int error = 0;

	if (argc != 2) {
		fprintf(stderr, "usage: %s domain\n",
				argv[0]);
		return 1;
	}

	newlen = strlen(argv[1]);
	if (newlen <= 0 || newlen >= BUFSIZE) {
		fprintf(stderr, "FAIL: invalid domain '%s'\n",
				argv[1]);
		return 1;
	}
	
	if (getdomainname(saved_domain, len) == -1) {
		fprintf(stderr, "FAIL: getdomainname failed - %s\n",
			strerror(errno));
		return 1;
	}
	/* printf("old domainname is %s\n", saved_domain ? saved_domain : "NULL"); */

	if (setdomainname(argv[1], strlen(argv[1])) == -1) {
		fprintf(stderr, "FAIL: setdomainname failed - %s\n",
			strerror(errno));
		return 1;
	}

	len = sizeof(new_domain);
	if (getdomainname(new_domain, len) == -1) {
		fprintf(stderr, "FAIL: getdomainname failed - %s\n",
			strerror(errno));
		error = 1;
		goto cleanup;
	}

	if (strcmp(new_domain, argv[1]) != 0) {
		fprintf(stderr, "FAIL: attempted to set domainname to '%s', "
				"but '%s' was the result\n",
			argv[1],
			new_domain);
		error = 1;
		goto cleanup;
	}

cleanup:
#ifdef RESET_DOMAIN_WHEN_DONE
	if (setdomainname(saved_domain, strlen(saved_domain)) == -1) {
		fprintf(stderr, "FAIL: setdomainname failed restting to old name - %s\n",
			strerror(errno));
		error = 1;
	}
#endif /* RESET_DOMAIN_WHEN_DONE */
	
	if (error == 0)
		printf("PASS\n");

	return error;
}
