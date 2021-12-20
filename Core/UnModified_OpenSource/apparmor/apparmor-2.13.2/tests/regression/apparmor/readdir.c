/*
 *	Copyright (C) 2002-2006 Novell/SUSE
 *	Copyright (C) 2017 Canonical, Ltd.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 *
 *	We attempt to test both getdents() and getdents64() here, but
 *	some architectures like aarch64 only support getdents64().
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include <dirent.h>
#include <linux/unistd.h>
#include <string.h>
#include <sys/syscall.h>

/* error if neither SYS_getdents or SYS_getdents64 is defined */
#if !defined(SYS_getdents) && !defined(SYS_getdents64)
  #error "Neither SYS_getdents or SYS_getdents64 is defined, something has gone wrong!"
#endif

/* define DEBUG to enable debugging statements i.e. gcc -DDEBUG */
#ifdef DEBUG
void pdebug(const char *format, ...)
{
	va_list arg;

	va_start(arg, format);
	vprintf(format, arg);
	va_end(arg);
}
#else
void pdebug(const char *format, ...) { return; }
#endif

#ifdef SYS_getdents
int my_readdir(char *dirname)
{
	int fd;
	struct dirent dir;

	fd = open(dirname, O_RDONLY, 0);
	if (fd == -1) {
		pdebug("open failed: %s\n", strerror(errno));
		return errno;
	}

	/* getdents isn't exported by glibc, so must use syscall() */
	if (syscall(SYS_getdents, fd, &dir, sizeof(dir)) == -1){
		pdebug("getdents failed: %s\n", strerror(errno));
		close(fd);
		return errno;
	}

	close(fd);
	return 0;
}
#endif

#ifdef SYS_getdents64
int my_readdir64(char *dirname)
{
	int fd;
	struct dirent64 dir;

	fd = open(dirname, O_RDONLY, 0);
	if (fd == -1) {
		pdebug("open failed: %s\n", strerror(errno));
		return errno;
	}

	/* getdents isn't exported by glibc, so must use syscall() */
	if (syscall(SYS_getdents64, fd, &dir, sizeof(dir)) == -1){
		pdebug("getdents64 failed: %s\n", strerror(errno));
		close(fd);
		return errno;
	}

	close(fd);
	return 0;
}
#endif

int main(int argc, char *argv[])
{
	int rc = 0, err = 0;
	char *dirpath, *endptr;
	int expected;

	if (argc != 3) {
		fprintf(stderr, "usage: %s [dir] [expected retval]\n",
			argv[0]);
		err = 1;
		goto err;
	}

	dirpath = argv[1];

	errno = 0;
	expected = (int) strtol(argv[2], &endptr, 10);
	if (errno != 0 || endptr == argv[2]) {
		fprintf(stderr, "ERROR: couldn't convert '%s' to an integer\n",
			argv[2]);
		err = 1;
		goto err;
	}
	pdebug("expected = %d\n", expected);

#ifdef SYS_getdents
	rc = my_readdir(dirpath);
	if (rc != expected) {
		printf("FAIL - my_readdir returned %d, expected %d\n", rc, expected);
		err = rc;
		goto err;
	}
#endif

#ifdef SYS_getdents64
	rc = my_readdir64(argv[1]);
	if (rc != expected) {
		printf("FAIL - my_readdir64 returned %d, expected %d\n", rc, expected);
		err = rc;
		goto err;
	}
#endif

	printf("PASS\n");

err:
	return err;
}
