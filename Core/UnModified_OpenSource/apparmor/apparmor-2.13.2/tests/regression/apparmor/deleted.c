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
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <linux/unistd.h>

#include "changehat.h"

/* A test to validate that we are properly handling the kernel appending
 * (deleted) in d_path lookup.
 * To acheive this the file is opened (the read/write of the file is just to
 * make sure everything is working as expected), deleted without closing the
 * file reference, and doing a changehat.
 * The file is then used inside of the changehat.  This forces the file
 * access permission to be revalidated which will cause a path lookup and
 * expose if the module is not handling the kernel appending (deleted).
 */
int main(int argc, char *argv[])
{
	int fd, fd2, rc;
	int ret = 0;		/* run through all tests to see what breaks */
	const char *data="hello world";
	char buf[128];

	if (argc != 3){
	    int i;
		fprintf(stderr, "usage: %s hat file\n",
			argv[0]);
		for (i=0; i < argc; i++) {
		    fprintf(stderr, "arg%d: %s\n", i, argv[i]);
		}
		return 1;
	}

	fd = open(argv[2], O_RDWR, 0);
	if (fd == -1){
		fprintf(stderr, "FAIL: open %s before changehat failed - %s\n",
			argv[2],
			strerror(errno));
		return 1;
	}

        rc = write(fd, data, strlen(data));
        if (rc != strlen(data)){
                fprintf(stderr, "FAIL: write before changehat failed - %s\n",
                        strerror(errno));
                return 1;
        }

	rc = unlink(argv[2]);
	if (rc == -1){
		fprintf(stderr, "FAIL: unlink before changehat failed - %s\n",
			strerror(errno));
		return 1;
	}
	
	if (strcmp(argv[1], "nochange") != 0){
		rc = change_hat(argv[1], SD_ID_MAGIC+1);
		if (rc == -1){
			fprintf(stderr, "FAIL: changehat %s failed - %s\n",
				argv[1], strerror(errno));
			exit(1);
		}
	}
	/******** The actual tests for (deleted) begin here *******/

	/* changehat causes revalidation */
        (void)lseek(fd, 0, SEEK_SET);
        rc = read(fd, buf, sizeof(buf));

        if (rc != strlen(data)){
                fprintf(stderr, "FAIL: read1 after changehat failed - %s\n",
                        strerror(errno));
		ret = 1;
        }

	/* test that we can create the file.  Not necessarily a (deleted)
	 * case but lets us flush out other combinations.
	 */
	fd2=creat(argv[2], S_IRUSR | S_IWUSR);
	if (fd2 == -1){
		fprintf(stderr, "FAIL: creat %s - %s\n",
			argv[2],
			strerror(errno));
		ret = 1;
	}
	close(fd2);

	/* reread after closing the created file, this should be revalidated
	 * and force a lookup that still has (deleted) appended
	 */
        (void)lseek(fd, 0, SEEK_SET);
        rc=read(fd, buf, sizeof(buf));

        if (rc != strlen(data)){
                fprintf(stderr, "FAIL: read2 after changehat failed - %s\n",
                        strerror(errno));
                ret = 1;
        }

       	close(fd);

	/* should be able to open it.  Shouldn't have (deleted) */
	fd=open(argv[2], O_RDWR, 0);
	if (fd == -1){
		fprintf(stderr, "FAIL: open %s after creat failed - %s\n",
			argv[2],
			strerror(errno));
		ret = 1;
	}

	close(fd);

	/* Hmm it would be nice to add a fork/exec test for (deleted) but
	 * changehat does it for now.
	 */

	if (!ret)
	    printf("PASS\n");

	return ret;
}
