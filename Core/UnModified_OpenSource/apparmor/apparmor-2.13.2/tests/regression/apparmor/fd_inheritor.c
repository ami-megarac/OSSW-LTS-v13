/*
 *   Copyright (c) 2013
 *   Canonical, Ltd. (All rights reserved)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, contact Novell, Inc. or Canonical
 *   Ltd.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_LEN		128

int main(int argc, char *argv[])
{
	char buf[BUF_LEN + 1];
	int fd, rc;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <fd#> <contents>\n", argv[0]);
		exit(1);
	}

	fd = atoi(argv[1]);

	rc = lseek(fd, 0, SEEK_SET);
	if (rc) {
		perror("FAIL INHERITOR - lseek");
		exit(1);
	}

	memset(buf, 0, sizeof(buf));
	rc = read(fd, buf, BUF_LEN);
	if (rc < 0) {
		perror("FAIL INHERITOR - read");
		exit(1);
	}

	if (strcmp(argv[2], buf)) {
		fprintf(stderr,
			"FAIL INHERITOR - expected \"%s\" but read \"%s\"\n",
			argv[2], buf);
		exit(1);
	}

	printf("PASS\n");
	exit(0);
}
