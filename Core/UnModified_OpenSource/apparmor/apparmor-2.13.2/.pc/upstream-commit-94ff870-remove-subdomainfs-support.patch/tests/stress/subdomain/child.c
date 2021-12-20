#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

static int zombies;

void sigchld(int num)
{
	zombies++;
}

int main()
{
	pid_t pid;
	int i;
	
	signal(SIGCHLD, sigchld);
again:
	for (i = 0; i < 500; i++) {
		pid = fork();
		if (pid > 0) 
			continue;
		else if (pid == 0)
			exit(0);
		else
			printf("fork: %s\n", strerror(errno));
	}
	while (waitpid(0, NULL, WNOHANG) > 0);
	goto again;
}
