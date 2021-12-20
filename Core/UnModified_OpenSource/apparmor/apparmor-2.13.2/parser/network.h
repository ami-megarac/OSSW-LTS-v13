/*
 *   Copyright (c) 2014
 *   Canonical, Ltd. (All rights reserved)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, contact Novell, Inc. or Canonical
 *   Ltd.
 */

#ifndef __AA_NETWORK_H
#define __AA_NETWORK_H

#include <fcntl.h>
#include <netinet/in.h>
#include <linux/socket.h>
#include <linux/limits.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "parser.h"
#include "rule.h"


#define AA_NET_WRITE		0x0002
#define AA_NET_SEND		AA_NET_WRITE
#define AA_NET_READ		0x0004
#define AA_NET_RECEIVE		AA_NET_READ

#define AA_NET_CREATE		0x0010
#define AA_NET_SHUTDOWN		0x0020		/* alias delete */
#define AA_NET_CONNECT		0x0040		/* alias open */

#define AA_NET_SETATTR		0x0100
#define AA_NET_GETATTR		0x0200

//#define AA_NET_CHMOD		0x1000		/* pair */
//#define AA_NET_CHOWN		0x2000		/* pair */
//#define AA_NET_CHGRP		0x4000		/* pair */
//#define AA_NET_LOCK		0x8000		/* LINK_SUBSET overlaid */

#define AA_NET_ACCEPT		0x00100000
#define AA_NET_BIND		0x00200000
#define AA_NET_LISTEN		0x00400000

#define AA_NET_SETOPT		0x01000000
#define AA_NET_GETOPT		0x02000000

#define AA_CONT_MATCH		0x08000000

#define AA_VALID_NET_PERMS (AA_NET_SEND | AA_NET_RECEIVE | AA_NET_CREATE | \
			    AA_NET_SHUTDOWN | AA_NET_CONNECT | \
			    AA_NET_SETATTR | AA_NET_GETATTR | AA_NET_BIND | \
			    AA_NET_ACCEPT | AA_NET_LISTEN | AA_NET_SETOPT | \
			    AA_NET_GETOPT | AA_CONT_MATCH)
#define AA_LOCAL_NET_PERMS (AA_NET_CREATE | AA_NET_SHUTDOWN | AA_NET_SETATTR |\
			    AA_NET_GETATTR | AA_NET_BIND | AA_NET_ACCEPT |    \
			    AA_NET_LISTEN | AA_NET_SETOPT | AA_NET_GETOPT)
#define AA_NET_OPT	(AA_NET_SETOPT | AA_NET_GETOPT)
#define AA_LOCAL_NET_CMD (AA_NET_LISTEN | AA_NET_OPT)
#define AA_PEER_NET_PERMS (AA_VALID_NET_PERMS & (~AA_LOCAL_NET_PERMS | \
						 AA_NET_ACCEPT))

struct network_tuple {
	const char *family_name;
	unsigned int family;
	const char *type_name;
	unsigned int type;
	const char *protocol_name;
	unsigned int protocol;
};

/* supported AF protocols */
struct aa_network_entry {
	unsigned int family;
	unsigned int type;
	unsigned int protocol;

	struct aa_network_entry *next;
};

int parse_net_mode(const char *str_mode, int *mode, int fail);
extern struct aa_network_entry *new_network_ent(unsigned int family,
						unsigned int type,
						unsigned int protocol);
extern struct aa_network_entry *network_entry(const char *family,
					      const char *type,
					      const char *protocol);
extern size_t get_af_max(void);

void __debug_network(unsigned int *array, const char *name);

struct network {
	unsigned int *allow;		/* array of type masks
						 * indexed by AF_FAMILY */
	unsigned int *audit;
	unsigned int *deny;
	unsigned int *quiet;

	network(void) { allow = audit = deny = quiet = NULL; }

	void dump(void) {
		if (allow)
			__debug_network(allow, "Network");
		if (audit)
			__debug_network(audit, "Audit Net");
		if (deny)
			__debug_network(deny, "Deny Net");
		if (quiet)
			__debug_network(quiet, "Quiet Net");
	}
};

int net_find_type_val(const char *type);
const char *net_find_type_name(int type);
const char *net_find_af_name(unsigned int af);
const struct network_tuple *net_find_mapping(const struct network_tuple *map,
					     const char *family,
					     const char *type,
					     const char *protocol);

#endif /* __AA_NETWORK_H */
