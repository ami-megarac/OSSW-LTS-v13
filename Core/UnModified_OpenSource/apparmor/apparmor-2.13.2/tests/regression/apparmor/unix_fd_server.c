/* fd passing over unix sockets */

/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *	Copyright (C) 2010 Canonical, Ltd.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

/* this is very ugly */

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <alloca.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <errno.h>
#include <stdlib.h>

int main (int argc, char * argv[]) {
	int sock, in_sock, fd;
	struct sockaddr_un local, remote;
	int len, pfd_ret;
	socklen_t len2;
	char comparison_buffer[17];
	char inbound_buffer[17];
        struct iovec vect;
        struct msghdr mesg;
        struct cmsghdr *ctrl_mesg;
	struct pollfd pfd;

	if (argc < 4 || argc > 5 || (argc == 5 && (strcmp(argv[4], "delete_file") != 0))) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return(1);
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "FAIL - open failed: %s\n",
			strerror(errno));
		return(1);
	}

	if (pread(fd, comparison_buffer, 16,0) <= 0) {
		fprintf(stderr, "FAIL - read failed: %s\n",
			strerror(errno));
		return(1);
	}

	if (argc == 5) {
		if (unlink(argv[1]) == -1){
		    fprintf(stderr, "FAIL: unlink before passing fd - %s\n",
			    strerror(errno));
		    return 1;
		}
	}

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1) {
		fprintf(stderr, "FAIL - socket failed: %s\n",
			strerror(errno));
		return(1);
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, argv[2]);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);
	
	if (bind(sock, (struct sockaddr *) &local, len) != 0) {
		fprintf(stderr, "FAIL - bind failed: %s\n",
			strerror(errno));
		return(1);
	}
	
	if (listen(sock, 2) != 0) {
		fprintf(stderr, "FAIL - listen failed: %s\n",
			strerror(errno));
		return(1);
	}

	/* exec the client */	
	int pid = fork();
	if (!pid) {
		execlp(argv[3], argv[3], argv[2], NULL);
		exit(0);
	}

	len2 = sizeof(remote);

	pfd.fd = sock;
	pfd.events = POLLIN;
	pfd_ret = poll(&pfd, 1, 500);
	if (pfd_ret == 1) {
		if ((in_sock = accept(sock, (struct sockaddr*)&remote, &len2)) == -1) {
			fprintf(stderr, "FAIL - accept: %s\n",
				strerror(errno));
			exit(1);
		}

       	vect.iov_base = argv[2];
       	vect.iov_len = strlen(argv[2]) + 1;

       	mesg.msg_name = NULL;
       	mesg.msg_namelen = 0;
       	mesg.msg_iov = &vect;
       	mesg.msg_iovlen = 1;

      	ctrl_mesg = alloca(sizeof(struct cmsghdr) + sizeof(fd));
       	ctrl_mesg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
       	ctrl_mesg->cmsg_level = SOL_SOCKET;
       	ctrl_mesg->cmsg_type = SCM_RIGHTS;

       	memcpy(CMSG_DATA(ctrl_mesg), &fd, sizeof(fd));
       	mesg.msg_control = ctrl_mesg;
       	mesg.msg_controllen = ctrl_mesg->cmsg_len;

       	/* try to send it */
       	if (sendmsg(in_sock, &mesg, 0) != vect.iov_len) {
               	fprintf(stderr, "FAIL - could not sendmsg\n");
		exit(1);
	}

	/* Check for info re: reading the file */
	memset(inbound_buffer, 0, sizeof(inbound_buffer));
	if (recv(in_sock, inbound_buffer, 16,0) == -1 ) {
		fprintf(stderr, "FAIL - recv %s\n",
			strerror(errno));
		exit(1);
	}

	if (strncmp(comparison_buffer, inbound_buffer,10) != 0) {
		fprintf(stderr, "FAIL - buffer comparison.  Got \"%s\", expected \"%s\"\n", inbound_buffer, comparison_buffer);
		exit(1);
	} else {
		printf("PASS\n");
	}	
		
	exit(0);
	} else {
		/* they timed out */
		fprintf(stderr, "FAIL - poll() timed out\n");
		exit(1);
	}

}
	
