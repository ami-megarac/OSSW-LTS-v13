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

#include <stdlib.h>
#include <string.h>
#include <sys/apparmor.h>

#include <iomanip>
#include <string>
#include <sstream>
#include <map>

#include "lib.h"
#include "parser.h"
#include "profile.h"
#include "parser_yacc.h"
#include "network.h"


int parse_net_mode(const char *str_mode, int *mode, int fail)
{
	return parse_X_mode("net", AA_VALID_NET_PERMS, str_mode, mode, fail);
}

/* Bleah C++ doesn't have non-trivial designated initializers so we just
 * have to make sure these are in order.  This means we are more brittle
 * but there isn't much we can do.
 */
struct sock_type_map {
	const char *name;
	int	value;
};

struct sock_type_map sock_types[] = {
	{ "none",	0 },
	{ "stream",	SOCK_STREAM },
	{ "dgram",	SOCK_DGRAM },
	{ "raw",	SOCK_RAW },
	{ "rdm",	SOCK_RDM },
	{ "seqpacket",	SOCK_SEQPACKET },
	{ "dccp",	SOCK_DCCP },
	{ "invalid",	-1 },
	{ "invalid",	-1 },
	{ "invalid",	-1 },
	{ "packet",	SOCK_PACKET },
	{ NULL, -1 },
	/*
	 * See comment above
	*/
};

int net_find_type_val(const char *type)
{
	int i;
	for (i = 0; sock_types[i].name; i++) {
		if (strcmp(sock_types[i].name, type) == 0)
			return sock_types[i].value;
	}

	return -1;
}

const char *net_find_type_name(int type)
{
	int i;
	for (i = 0; sock_types[i].name; i++) {
		if (sock_types[i].value  == type)
			return sock_types[i].name;
	}

	return NULL;
}


/* FIXME: currently just treating as a bit mask this will have to change
 * set up a table of mappings, there can be several mappings for a
 * given match.
 * currently the mapping does not set the protocol for stream/dgram to
 * anything other than 0.
 *   network inet tcp -> network inet stream 0 instead of
 *   network inet raw tcp.
 * some entries are just provided for completeness at this time
 */
/* values stolen from /etc/protocols - needs to change */
#define RAW_TCP 6
#define RAW_UDP 17
#define RAW_ICMP 1
#define RAW_ICMPv6 58

/* used by af_name.h to auto generate table entries for "name", AF_NAME
 * pair */
#define AA_GEN_NET_ENT(name, AF) \
	{name, AF, "stream",    SOCK_STREAM,    "", 0xffffff}, \
	{name, AF, "dgram",     SOCK_DGRAM,     "", 0xffffff}, \
	{name, AF, "seqpacket", SOCK_SEQPACKET, "", 0xffffff}, \
	{name, AF, "rdm",       SOCK_RDM,       "", 0xffffff}, \
	{name, AF, "raw",       SOCK_RAW,       "", 0xffffff}, \
	{name, AF, "packet",    SOCK_PACKET,    "", 0xffffff},
/*FIXME: missing {name, AF, "dccp", SOCK_DCCP, "", 0xfffffff}, */

static struct network_tuple network_mappings[] = {
	/* basic types */
	#include "af_names.h"
/* FIXME: af_names.h is missing AF_LLC, AF_TIPC */
	/* mapped types */
	{"inet",	AF_INET,	"raw",		SOCK_RAW,
	 "tcp",	        1 << RAW_TCP},
	{"inet",	AF_INET,	"raw",		SOCK_RAW,
	 "udp",		1 << RAW_UDP},
	{"inet",	AF_INET,	"raw",		SOCK_RAW,
	 "icmp",	1 << RAW_ICMP},
	{"inet",	AF_INET,	"tcp",		SOCK_STREAM,
	 "",		0xffffffff},	/* should we give raw tcp too? */
	{"inet",	AF_INET,	"udp",		SOCK_DGRAM,
	 "",		0xffffffff},	/* should these be open masks? */
	{"inet",	AF_INET,	"icmp",		SOCK_RAW,
	 "",		1 << RAW_ICMP},
	{"inet6",	AF_INET6,	"tcp",		SOCK_STREAM,
	 "",		0xffffffff},
	{"inet6",	AF_INET6,	"udp",		SOCK_DGRAM,
	 "",		0xffffffff},
/* what do we do with icmp on inet6?
	{"inet6",	AF_INET,	"icmp",		SOCK_RAW,	0},
	{"inet6",	AF_INET,	"icmpv6",	SOCK_RAW,	0},
*/
	/* terminate */
	{NULL, 0, NULL, 0, NULL, 0}
};

/* The apparmor kernel patches up until 2.6.38 didn't handle networking
 * tables with sizes > AF_MAX correctly.  This could happen when the
 * parser was built against newer kernel headers and then used to load
 * policy on an older kernel.  This could happen during upgrades or
 * in multi-kernel boot systems.
 *
 * Try to detect the running kernel version and use that to determine
 * AF_MAX
 */
#define PROC_VERSION "/proc/sys/kernel/osrelease"
static size_t kernel_af_max(void) {
	char buffer[32];
	int major;
	autoclose int fd = -1;
	int res;

	if (!net_af_max_override) {
		return 0;
	}
	/* the override parameter is specifying the max value */
	if (net_af_max_override > 0)
		return net_af_max_override;

	fd = open(PROC_VERSION, O_RDONLY);
	if (fd == -1)
		/* fall back to default provided during build */
		return 0;
	res = read(fd, &buffer, sizeof(buffer) - 1);
	if (res <= 0)
		return 0;
	buffer[res] = '\0';
	res = sscanf(buffer, "2.6.%d", &major);
	if (res != 1)
		return 0;

	switch(major) {
	case 24:
	case 25:
	case 26:
		return 34;
	case 27:
		return 35;
	case 28:
	case 29:
	case 30:
		return 36;
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
		return 37;
	case 36:
	case 37:
		return 38;
	/* kernels .38 and later should handle this correctly so no
	 * static mapping needed
	 */
	default:
		return 0;
	}
}

/* Yuck. We grab AF_* values to define above from linux/socket.h because
 * they are more accurate than sys/socket.h for what the kernel actually
 * supports. However, we can't just include linux/socket.h directly,
 * because the AF_* definitions are protected with an ifdef KERNEL
 * wrapper, but we don't want to define that because that can cause
 * other redefinitions from glibc. However, because the kernel may have
 * more definitions than glibc, we need make sure AF_MAX reflects this,
 * hence the wrapping function.
 */
size_t get_af_max() {
	size_t af_max;
	/* HACK: declare that version without "create" had a static AF_MAX */
	if (!perms_create && !net_af_max_override)
		net_af_max_override = -1;

#if AA_AF_MAX > AF_MAX
	af_max = AA_AF_MAX;
#else
	af_max = AF_MAX;
#endif

	/* HACK: some kernels didn't handle network tables from parsers
	 * compiled against newer kernel headers as they are larger than
	 * the running kernel expected.  If net_override is defined check
	 * to see if there is a static max specified for that kernel
	 */
	if (net_af_max_override) {
		size_t max = kernel_af_max();
		if (max && max < af_max)
			return max;
	}

	return af_max;
}
struct aa_network_entry *new_network_ent(unsigned int family,
					 unsigned int type,
					 unsigned int protocol)
{
	struct aa_network_entry *new_entry;
	new_entry = (struct aa_network_entry *) calloc(1, sizeof(struct aa_network_entry));
	if (new_entry) {
		new_entry->family = family;
		new_entry->type = type;
		new_entry->protocol = protocol;
		new_entry->next = NULL;
	}
	return new_entry;
}


const struct network_tuple *net_find_mapping(const struct network_tuple *map,
					     const char *family,
					     const char *type,
					     const char *protocol)
{
	if (!map)
		map = network_mappings;
	else
		/* assumes it points to last entry returned */
		map++;

	for (; map->family_name; map++) {
		if (family) {
			PDEBUG("Checking family %s\n", map->family_name);
			if (strcmp(family, map->family_name) != 0)
				continue;
			PDEBUG("Found family %s\n", family);
		}
		if (type) {
			PDEBUG("Checking type %s\n", map->type_name);
			if (strcmp(type, map->type_name) != 0)
				continue;
			PDEBUG("Found type %s\n", type);
		}
		if (protocol) {
			/* allows the proto to be the "type", ie. tcp implies
			 * stream */
			if (!type) {
				PDEBUG("Checking protocol type %s\n", map->type_name);
				if (strcmp(protocol, map->type_name) == 0)
					goto match;
			}
			PDEBUG("Checking type %s protocol %s\n", map->type_name, map->protocol_name);
			if (strcmp(protocol, map->protocol_name) != 0)
				continue;
			/* fixme should we allow specifying protocol by #
			 * without needing the protocol mapping? */
		}

		/* if we get this far we have a match */
	match:
		return map;
	}

	return NULL;
}

struct aa_network_entry *network_entry(const char *family, const char *type,
				       const char *protocol)
{
	struct aa_network_entry *new_entry, *entry = NULL;
	const struct network_tuple *mapping = NULL;

	while ((mapping = net_find_mapping(mapping, family, type, protocol))) {
		new_entry = new_network_ent(mapping->family, mapping->type,
					    mapping->protocol);
		if (!new_entry)
			yyerror(_("Memory allocation error."));
		new_entry->next = entry;
		entry = new_entry;
	}

	return entry;
};

#define ALL_TYPES 0x43e

const char *net_find_af_name(unsigned int af)
{
	size_t i;

	if (af < 0 || af > get_af_max())
		return NULL;

	for (i = 0; i < sizeof(network_mappings) / sizeof(*network_mappings); i++) {
		if (network_mappings[i].family == af)
			return network_mappings[i].family_name;
	}

	return NULL;
}

void __debug_network(unsigned int *array, const char *name)
{
	unsigned int count = sizeof(sock_types)/sizeof(sock_types[0]);
	unsigned int mask = ~((1 << count) -1);
	unsigned int i, j;
	int none = 1;
	size_t af_max = get_af_max();

	for (i = AF_UNSPEC; i < af_max; i++)
		if (array[i]) {
			none = 0;
			break;
		}

	if (none)
		return;

	printf("%s: ", name);

	/* This can only be set by an unqualified network rule */
	if (array[AF_UNSPEC]) {
		printf("<all>\n");
		return;
	}

	for (i = 0; i < af_max; i++) {
		if (array[i]) {
			const char *fam = net_find_af_name(i);
			if (fam)
				printf("%s ", fam);
			else
				printf("#%u ", i);

			/* All types/protocols */
			if (array[i] == 0xffffffff || array[i] == ALL_TYPES)
				continue;

			printf("{ ");

			for (j = 0; j < count; j++) {
				const char *type;
				if (array[i] & (1 << j)) {
					type = sock_types[j].name;
					if (type)
						printf("%s ", type);
					else
						printf("#%u ", j);
				}
			}
			if (array[i] & mask)
				printf("#%x ", array[i] & mask);

			printf("} ");
		}
	}
	printf("\n");
}
