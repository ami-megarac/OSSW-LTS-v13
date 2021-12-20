/*	Copyright (C) 2002-2006 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	char *var, *val, *p, *envval;

	if (argc < 2 || !(p = strchr(argv[1], '='))) {
		fprintf(stderr, "Usage: %s VAR=value\n", argv[0]);
		return 1;
	}

	var = strndup(argv[1], p - argv[1]);
	val = strdup(p + 1);

	envval = getenv(var);
	if (!envval) {
		fprintf(stderr, "FAIL: environment variable not found\n");
		return 1;
	}

	if (strcmp(envval, val) != 0) {
		fprintf(stderr, "FAIL: environment variable differs: expected '%s', found '%s'\n",
			val, envval);
		return 1;
	}
	return 0;
}
