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
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#define FALSE 0
#define TRUE !FALSE

#define MAX_FILES 5

int (*pass)[MAX_FILES];

void test_files(int num_files, char *files[], int index)
{
	int fd, i;

	for (i = 0; i < num_files; i++) {
		fd = open(files[i], O_RDWR);

		if (fd == -1) {
			if (errno == ENOENT) {
				pass[index][i] = -1;
			} else {
				pass[index][i] = 0;
			}
		} else {
			pass[index][i] = 1;
			close(fd);
		}
	}
}

int main(int argc, char *argv[])
{
	int num_files, i, shmid;
	pid_t pid;
	struct shmid_ds shm_desc;

	if (argc < 2) {
		fprintf(stderr, "usage: %s program [args] \n", argv[0]);
		return 1;
	}

	num_files = argc - 1;
	if (num_files > MAX_FILES) {
		fprintf(stderr, "ERROR: a maximum of %d files is supported\n",
			MAX_FILES);
		return 1;
	}

	shmid = shmget(IPC_PRIVATE, sizeof(int[2][MAX_FILES]), IPC_CREAT);
	if (shmid == -1) {
		fprintf(stderr, "FAIL: shmget failed %s\n", strerror(errno));
		return 1;
	}

	pass = (int(*)[MAX_FILES])shmat(shmid, NULL, 0);

	if (pass == (void *)-1) {
		fprintf(stderr, "FAIL: shmat failed %s\n", strerror(errno));
		return 1;
	}

	pid = fork();

	if (pid) {		/* parent */
		int status;
		int allpassed = TRUE;

		test_files(num_files, &argv[1], 0);

		while (wait(&status) != pid) ;

		for (i = 0; i < num_files; i++) {
			if (pass[0][i] != pass[1][i] ||
			    pass[0][i] == -1 || pass[1][i] == -1) {
				if (allpassed) {
					fprintf(stderr, "FAILED:");
					allpassed = FALSE;
				}

				fprintf(stderr, " file%d(%d:%d)",
					i + 1, pass[0][i], pass[1][i]);
			}
		}

		if (allpassed) {
			printf("PASS\n");
		} else {
			fprintf(stderr, "\n");
		}

		(void)shmdt(pass);
		shmctl(shmid, IPC_RMID, &shm_desc);

	} else {
		test_files(num_files, &argv[1], 1);
	}

	return 0;
}
