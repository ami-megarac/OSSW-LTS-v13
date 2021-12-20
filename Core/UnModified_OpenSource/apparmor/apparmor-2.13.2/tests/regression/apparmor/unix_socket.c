/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact Canonical Ltd.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

#include "unix_socket_common.h"

#define MSG_BUF_MAX 		1024
#define PATH_FOR_UNNAMED	"none"

static int connection_based_messaging(int sock, int sock_is_peer_sock,
				      char *msg_buf, size_t msg_buf_len)
{
	int peer_sock, rc;

	if (sock_is_peer_sock) {
		peer_sock = sock;
	} else {
		peer_sock = accept(sock, NULL, NULL);
		if (peer_sock < 0) {
			perror("FAIL - accept");
			return 1;
		}
	}

	rc = write(peer_sock, msg_buf, msg_buf_len);
	if (rc < 0) {
		perror("FAIL - write");
		return 1;
	}

	rc = read(peer_sock, msg_buf, msg_buf_len);
	if (rc < 0) {
		perror("FAIL - read");
		return 1;
	}

	return 0;
}

static int connectionless_messaging(int sock, char *msg_buf, size_t msg_buf_len)
{
	struct sockaddr_un peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);
	int rc;

	peer_addr.sun_family = AF_UNIX;
	rc = recvfrom(sock, NULL, 0, 0, (struct sockaddr *)&peer_addr,
		      &peer_addr_len);
	if (rc < 0) {
		perror("FAIL - recvfrom");
		return 1;
	}

	rc = sendto(sock, msg_buf, msg_buf_len, 0,
		    (struct sockaddr *)&peer_addr, peer_addr_len);
	if (rc < 0) {
		perror("FAIL - sendto");
		return 1;
	}

	rc = recv(sock, msg_buf, msg_buf_len, 0);
	if (rc < 0) {
		perror("FAIL - recv");
		return 1;
	}

	return 0;
}

int main (int argc, char *argv[])
{
	struct sockaddr_un addr;
	char msg_buf[MSG_BUF_MAX];
	size_t msg_buf_len;
	const char *sun_path;
	size_t sun_path_len;
	pid_t pid;
	int sock, peer_sock, type, rc;
	int unnamed = 0;

	if (argc != 5) {
		fprintf(stderr,
			"Usage: %s <socket> <type> <message> <client>\n\n"
			"  socket\t\ta path for a bound socket or a name prepended with '@' for an abstract socket\n"
			"  type\t\tstream, dgram, or seqpacket\n",
			argv[0]);
		exit(1);
	}

	addr.sun_family = AF_UNIX;
	memset(addr.sun_path, 0, sizeof(addr.sun_path));

	sun_path = argv[1];
	sun_path_len = strlen(sun_path);
	if (sun_path[0] == '@') {
		if (sun_path_len > sizeof(addr.sun_path)) {
			fprintf(stderr, "FAIL - socket addr too big\n");
			exit(1);
		}
		memcpy(addr.sun_path, sun_path, sun_path_len);
		addr.sun_path[0] = '\0';
	} else if (!strcmp(sun_path, PATH_FOR_UNNAMED)) {
		unnamed = 1;
	} else {
		/* include the nul terminator for pathname addr types */
		sun_path_len++;
		if (sun_path_len > sizeof(addr.sun_path)) {
			fprintf(stderr, "FAIL - socket addr too big\n");
			exit(1);
		}
		memcpy(addr.sun_path, sun_path, sun_path_len);
	}

	if (!strcmp(argv[2], "stream")) {
		type = SOCK_STREAM;
	} else if (!strcmp(argv[2], "dgram")) {
		type = SOCK_DGRAM;
	} else if (!strcmp(argv[2], "seqpacket")) {
		type = SOCK_SEQPACKET;
	} else {
		fprintf(stderr, "FAIL - bad socket type: %s\n", argv[2]);
		exit(1);
	}

	msg_buf_len = strlen(argv[3]) + 1;
	if (msg_buf_len > MSG_BUF_MAX) {
		fprintf(stderr, "FAIL - message too big\n");
		exit(1);
	}
	memcpy(msg_buf, argv[3], msg_buf_len);

	if (unnamed) {
		int sv[2];

		rc = socketpair(AF_UNIX, type, 0, sv);
		if (rc == -1) {
			perror("FAIL - socketpair");
			exit(1);
		}
		sock = sv[0];
		peer_sock = sv[1];

		rc = fcntl(sock, F_SETFD, FD_CLOEXEC);
		if (rc == -1) {
			perror("FAIL - fcntl");
			exit(1);
		}
	} else {
		sock = socket(AF_UNIX, type | SOCK_CLOEXEC, 0);
		if (sock == -1) {
			perror("FAIL - socket");
			exit(1);
		}
	}

	rc = set_sock_io_timeo(sock);
	if (rc)
		exit(1);

	if (!unnamed) {
		rc = bind(sock, (struct sockaddr *)&addr,
			  sun_path_len + sizeof(addr.sun_family));
		if (rc < 0) {
			perror("FAIL - bind");
			exit(1);
		}

		if (type & SOCK_STREAM || type & SOCK_SEQPACKET) {
			rc = listen(sock, 2);
			if (rc < 0) {
				perror("FAIL - listen");
				exit(1);
			}
		}
	}

	rc = get_sock_io_timeo(sock);
	if (rc)
		exit(1);

	pid = fork();
	if (pid < 0) {
		perror("FAIL - fork");
		exit(1);
	} else if (!pid) {
		char *fd_number = NULL;

		if (unnamed) {
			rc = asprintf(&fd_number, "%d", peer_sock);
			if (rc == -1) {
				perror("FAIL - asprintf");
				exit(1);
			}
		}

		/* fd_number will be NULL for pathname and abstract sockets */
		execl(argv[4], argv[4], sun_path, argv[2], fd_number, NULL);
		perror("FAIL - execl");
		free(fd_number);
		exit(1);
	}

	rc = (type & SOCK_STREAM || type & SOCK_SEQPACKET) ?
		connection_based_messaging(sock, unnamed, msg_buf, msg_buf_len) :
		connectionless_messaging(sock, msg_buf, msg_buf_len);
	if (rc)
		exit(1);

	if (memcmp(argv[3], msg_buf, msg_buf_len)) {
		msg_buf[msg_buf_len] = '\0';
		fprintf(stderr, "FAIL - buffer comparison. Got \"%s\", expected \"%s\"\n",
			msg_buf, argv[3]);
		exit(1);
	}

	rc = shutdown(sock, SHUT_RDWR);
	if (rc == -1) {
		perror("FAIL - shutdown");
		exit(1);
	}

	printf("PASS\n");
	exit(0);
}
