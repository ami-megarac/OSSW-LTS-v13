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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BUFSIZE 4096
int main(int argc, char *argv[])
{
	char read_buffer[BUFSIZE], verify_buffer[BUFSIZE];
	ssize_t read_size, write_size;
	int fd;

	if ((argc < 3) || (argc == 4 && strcmp(argv[2],"w")) || argc > 4) {
		fprintf(stderr, "Usage: %s sysctl_path {r,w,rw} [value]\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[2],"r") == 0) {
		fd = open(argv[1], O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "FAIL: proc sysctl open r failed - %s\n",
				strerror(errno));
			return 1;
		}
		read_size = read(fd, &read_buffer, sizeof(read_buffer));
		if (read_size == -1) {
			fprintf(stderr, "FAIL: proc sysctl read failed - %s\n",
				strerror(errno));
			return 1;
		}
	}

	if (strcmp(argv[2], "w") == 0) {
		fd = open(argv[1], O_WRONLY);
		if (fd == -1) {
			fprintf(stderr, "FAIL: proc sysctl open w failed - %s\n",
				strerror(errno));
			return 1;
		}
		write_size = write(fd, argv[3], strlen(argv[3]));
		if (write_size == -1) {
			fprintf(stderr, "FAIL: proc sysctl write failed - %s\n",
				strerror(errno));
			return 1;
		}

	}

	if (strcmp(argv[2], "rw") == 0) {
		fd = open(argv[1], O_RDWR);
		if (fd == -1) {
			fprintf(stderr, "FAIL: proc sysctl open rw failed - %s\n",
				strerror(errno));
			return 1;
		}
		read_size = read(fd, &read_buffer, sizeof(read_buffer));
		if (read_size == -1) {
			fprintf(stderr, "FAIL: proc sysctl read(rw) failed - %s\n",
				strerror(errno));
			return 1;
		}
		lseek(fd, 0, SEEK_SET);
		write_size = write(fd, &read_buffer, read_size);
		if (write_size == -1 || write_size != read_size) {
			fprintf(stderr, "FAIL: proc sysctl write(rw) failed - %s\n",
				strerror(errno));
			return 1;
		}

		lseek(fd, 0, SEEK_SET);
		read_size = read(fd, &verify_buffer, sizeof(verify_buffer));
		if (read_size == -1 || read_size != write_size) {
			fprintf(stderr, "FAIL: proc sysctl verify(rw) failed || %zd != %zd - %s\n", read_size, write_size,
				strerror(errno));
			return 1;
		}
		if (memcmp(read_buffer, verify_buffer, read_size) != 0) {
			fprintf(stderr, "FAIL: proc sysctl verify failed - %s\n",
				strerror(errno));
			return 1;
		}
	}

	printf("PASS\n");

	return 0;
}
