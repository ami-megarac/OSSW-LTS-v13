#define _XOPEN_SOURCE 500

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
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <alloca.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int sock, fd, len;
	struct sockaddr_un remote;
	char read_buffer[17], f_buf[255];
        struct iovec vect;
        struct msghdr mesg;
        struct cmsghdr *ctrl_mesg;

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "FAIL CLIENT - sock %s\n",
			strerror(errno));
		exit(1);
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, argv[1]);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(sock, (struct sockaddr *)&remote, len) == -1) {
		fprintf(stderr, "FAIL CLIENT - connect %s\n",
			strerror(errno));
		exit(1);
	}

        vect.iov_base = f_buf;
        vect.iov_len = 255;

        mesg.msg_name = NULL;
        mesg.msg_namelen=0;
        mesg.msg_iov = &vect;
        mesg.msg_iovlen = 1;

        ctrl_mesg = alloca(sizeof (struct cmsghdr) + sizeof(fd));
        ctrl_mesg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
        mesg.msg_control = ctrl_mesg;
        mesg.msg_controllen = ctrl_mesg->cmsg_len;

        if (!recvmsg(sock, &mesg,0 )) {
		fprintf(stderr, "FAIL CLIENT - recvmsg\n");
                exit(1);
        }

        /* get mr. file descriptor */

        memcpy(&fd, CMSG_DATA(ctrl_mesg), sizeof(fd));

        if (pread(fd, read_buffer, 16, 0) <= 0) {
        	/* Failure */
		fprintf(stderr, "FAIL CLIENT - could not read\n");
		send(sock, "FAILFAILFAILFAIL", 16, 0);
		exit(1);
	} else {
		send(sock, read_buffer, strlen(read_buffer),0);
	}
	
        /* looks like it worked */
        exit(0);
}
