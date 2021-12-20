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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_LEN		128
#define FD_STR_LEN	4

int main(int argc, char *argv[])
{
	char buf[BUF_LEN + 1], fd_str[FD_STR_LEN + 1];
	int fd, rc;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <file> <exec>\n", argv[0]);
		exit(1);
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("FAIL - open");
		exit(1);
	}

	memset(buf, 0, sizeof(buf));
	rc = read(fd, buf, BUF_LEN);
	if (rc < 0) {
		perror("FAIL - read");
		exit(1);
	}

	memset(fd_str, 0, sizeof(fd_str));
	snprintf(fd_str, FD_STR_LEN, "%d", fd);
	execl(argv[2], argv[2], fd_str, buf, NULL);

	perror("FAIL - execl");
	exit(1);
}
