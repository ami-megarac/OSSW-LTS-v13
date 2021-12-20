#include <stdio.h>

/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char *argv[])
{
pid_t pid;

extern char **environ;

	if (argc < 2){
		fprintf(stderr, "usage: %s program [args] \n", argv[0]);
		return 1;
	}

	pid=fork();

	if (pid){	/* parent */
		int status;

		while (wait(&status) != pid);

		if (WIFEXITED(status)){
			printf("PASS\n");
		}else{
			fprintf(stderr, "FAILED, child did not exit normally\n");
		}
	}else{
		/* child */

		(void)execve(argv[1], &argv[1], environ);

		/* exec failed, kill outselves to flag parent */

		(void)kill(getpid(), SIGKILL);
	}

	return 0;
}
