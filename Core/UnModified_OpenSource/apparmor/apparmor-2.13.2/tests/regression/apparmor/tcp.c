/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

/* Basic tcp networking test. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define HOSTIP "127.0.0.1"
#define HOSTPORT 9876
#define BUFSIZE 4096
#define WAITLEN 1 /* number of seconds for select to wait */
#define DATASOURCE "/dev/urandom"

char data[BUFSIZE];

/* server_bind_address returns the socket filedescriptor */
int
server_bind_address (int port) {
	int sockfd;
	struct sockaddr_in serv_addr;
	int value = 1;

	/* create a socket */
	if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
		perror ("FAIL: Server: can't open stream socket");
		return -1;
	}	

	/* don't use SO_REUSEADDR in production code. */
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

	memset(&serv_addr, 0UL, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_pton (AF_INET, HOSTIP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(port);

	if (bind (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) != 0) {
		perror("FAIL: Server: can't bind socket");
		close(sockfd);
		return -1;
	}	
	
	listen (sockfd, 1);

	return sockfd;
}

int my_select_read (int fd, int secs) {
	fd_set rfds;
	struct timeval tv;
	int retval;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	tv.tv_sec = secs;
	tv.tv_usec = 0;
	retval = select (fd + 1, &rfds, NULL, NULL, &tv);
	if ((retval == -1) || !(FD_ISSET(fd, &rfds))) {
		return -1;
	}

	return 0;
}

int my_select_write (int fd, int secs) {
	fd_set wfds;
	struct timeval tv;
	int retval;

	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);
	tv.tv_sec = secs;
	tv.tv_usec = 0;
	retval = select (fd + 1, NULL, &wfds, NULL, &tv);
	if ((retval == -1) || !(FD_ISSET(fd, &wfds))) {
		return -1;
	}

	return 0;
}

int
do_server (int sockfd) {
	int newsockfd = -1;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	int retval = 0, len;
	char buffer[BUFSIZE];

	/* socket should already have listen() called on it by now */
	retval = my_select_read (sockfd, WAITLEN);
	if (retval != 0) {
		perror ("FAIL: Server: select failed");
		goto out;
	}

	clilen = sizeof (cli_addr);
	newsockfd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);

	if (newsockfd < 0) {
		perror ("FAIL: Server: can't accept socket");
		retval = errno;
		goto out;
	}	

	retval = my_select_read (newsockfd, WAITLEN);
	if (retval != 0) {
		perror ("FAIL: Server: select failed");
		goto out;
	}

	len = recv (newsockfd, buffer, sizeof(buffer), 0);
	if (len <= 0) {
		perror ("FAIL: Server: read failed");
		retval = errno;
		goto out;
	}

	/* verify data received from the client */
	if (memcmp((void *) buffer, (void *) data, sizeof(buffer)) != 0) {
		fprintf(stderr, "FAIL: Server: memory comparison failed\n");
		retval = -1;
		goto out;
	}

	/* send the data back to the client */
	if (send(newsockfd, data, sizeof(buffer), 0) < sizeof(buffer)) {
		perror("FAIL: Server: problem sending data");
		retval = errno;
		goto out;
	}	

out:
	if (newsockfd >= 0) { 
		close (newsockfd);
	}
	close (sockfd);
	return retval;
}


int
do_client (int port) {
	int sockfd;
	struct sockaddr_in serv_addr;
	int retval, len;
	char buffer[BUFSIZE];
	
	/* create a socket */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("FAIL: Client: can't open stream socket");
		retval = errno;
		goto out;
	}	

	/* Set to non_block. */
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	//setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

	memset(&serv_addr, 0UL, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_pton(AF_INET, HOSTIP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		if (errno != EINPROGRESS) {
			perror("FAIL: Client: can't connect to socket");
			retval = errno;
			goto out;
		}
	}	

	retval = my_select_write (sockfd, WAITLEN);
	if (retval != 0) {
		perror ("FAIL: Client: select failed");
		goto out;
	}

	/* send the data to the server */
	if (send(sockfd, data, sizeof(buffer), 0) < sizeof(buffer)) {
		perror("FAIL: Client: problem sending data");
		retval = errno;
		goto out;
	}	

	retval = my_select_read (sockfd, WAITLEN);
	if (retval != 0) {
		perror ("FAIL: Client: select failed");
		goto out;
	}

	len = recv (sockfd, buffer, sizeof(buffer), 0);
	if (len <= 0) {
		perror ("FAIL: Client: read failed");
		retval = errno;
		goto out;
	}

	/* verify the data returned from the server */
	if (memcmp((void *) buffer, (void *) data, sizeof(buffer)) != 0) {
		fprintf(stderr, "FAIL: Client: memory comparison failed\n");
		retval = -1;
		goto out;
	}

	retval = 0;
out:
	if (sockfd >= 0) close(sockfd);
	return retval;
}

int 
main (int argc, char * argv[]) {
	pid_t childpid;
	int fd, retval, waitstatus;
	int sockfd = -1;
	int serv_port = HOSTPORT;

	fd = open(DATASOURCE, O_RDONLY);
	if (fd < 0) {
		perror("FAIL: open()ing data source failed");
		return errno;
	}

	if ((retval = read(fd, data, sizeof(data))) < sizeof(data)) {
		perror("FAIL: problem reading data source");
		close(fd);
		return errno;
	}
	close(fd);

	if (argc > 1) {
		serv_port = strtol(argv[1], NULL, 10);
	}

	/* get the server to listen, so the child has something to
	 * connect to if it wins the race. */
	if ((sockfd = server_bind_address(serv_port)) < 0) {
		return errno;
	}
	
	if ((childpid = fork()) < 0) {
		perror("FAIL: fork() failed");
		return errno;
	}

	if (childpid == 0) {
		return do_client (serv_port);
	} else {
		do_server(sockfd);
	}

	wait(&waitstatus);

	if ((WIFEXITED(waitstatus) != 0) && (WEXITSTATUS(waitstatus) == 0)) {
		printf("PASS\n");
	}

	return 0;
}
