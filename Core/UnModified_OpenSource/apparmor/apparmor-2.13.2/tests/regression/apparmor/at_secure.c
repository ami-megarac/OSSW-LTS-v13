/**
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact Canonical Ltd.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/auxv.h>

int check_at_secure(unsigned long expected)
{
	unsigned long at_secure;

	errno = 0;
	at_secure = getauxval(AT_SECURE);
	if (at_secure == 0 && errno != 0) {
		perror("FAIL - getauxval");
		return 1;
	}

	if (at_secure != expected) {
		fprintf(stderr,
			"FAIL - AT_SECURE value (%lu) did not match the expected value (%lu)\n",
			at_secure, expected);
		return 1;
	}

	printf("PASS\n");
	return 0;
}

int main(int argc, char **argv)
{
	unsigned long expected;

	if (argc != 2) {
		fprintf(stderr, "usage: %s EXPECTED_AT_SECURE\n", argv[0]);
		return 1;
	}

	expected = strtoul(argv[1], NULL, 10);
	return check_at_secure(expected);
}
