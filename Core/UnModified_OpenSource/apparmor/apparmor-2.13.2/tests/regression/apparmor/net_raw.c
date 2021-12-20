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
//#include <netinet/in.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> 
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

int 
main (int argc, char * argv[]) {
	int sockfd;

	/* create a socket */
	if ((sockfd = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		perror ("FAIL: Server: can't open raw socket");
		return 1;
	}	

	close (sockfd);

	printf ("PASS\n");

	return 0;
}
