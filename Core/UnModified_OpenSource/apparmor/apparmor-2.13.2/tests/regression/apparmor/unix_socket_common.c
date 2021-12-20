/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "unix_socket_common.h"

int get_sock_io_timeo(int sock)
{
	struct timeval tv;
	socklen_t tv_len = sizeof(tv);
	int rc;

	rc = getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, &tv_len);
	if (rc == -1) {
		perror("FAIL - getsockopt");
		return 1;
	}

	return 0;
}

int set_sock_io_timeo(int sock)
{
	struct timeval tv;
	socklen_t tv_len = sizeof(tv);
	int rc;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	rc = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, tv_len);
	if (rc == -1) {
		perror("FAIL - setsockopt (SO_RCVTIMEO)");
		return 1;
	}

	rc = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, tv_len);
	if (rc == -1) {
		perror("FAIL - setsockopt (SO_SNDTIMEO)");
		return 1;
	}

	return 0;
}
