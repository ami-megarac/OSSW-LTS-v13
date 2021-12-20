/*
 * Copyright (c) 1999-2008 NOVELL (All rights reserved)
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @author Matt Barringer <mbarringer@suse.de>
 */

/*
 * TODO:
 *
 * - Convert the text permission mask into a bitmask
 * - Clean up parser grammar
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <aalogparse.h>
#include "parser.h"

/* This is mostly just a wrapper around the code in grammar.y */
aa_log_record *parse_record(char *str)
{
	if (str == NULL)
		return NULL;

	return _parse_yacc(str);
}

void free_record(aa_log_record *record)
{
	if (record != NULL)
	{
		if (record->operation != NULL)
			free(record->operation);
		if (record->requested_mask != NULL)
			free(record->requested_mask);
		if (record->denied_mask != NULL)
			free(record->denied_mask);
		if (record->profile != NULL)
			free(record->profile);
		if (record->peer_profile != NULL)
			free(record->peer_profile);
		if (record->comm != NULL)
			free(record->comm);
		if (record->name != NULL)
			free(record->name);
		if (record->name2 != NULL)
			free(record->name2);
		if (record->namespace != NULL)
			free(record->namespace);
		if (record->attribute != NULL)
			free(record->attribute);
		if (record->info != NULL)
			free(record->info);
		if (record->peer_info != NULL)
			free(record->peer_info);
		if (record->peer != NULL)
			free(record->peer);
		if (record->active_hat != NULL)
			free(record->active_hat);
		if (record->audit_id != NULL)
			free(record->audit_id);
		if (record->net_family != NULL)
			free(record->net_family);
		if (record->net_protocol != NULL)
			free(record->net_protocol);
		if (record->net_sock_type != NULL)
			free(record->net_sock_type);
		if (record->net_local_addr != NULL)
			free(record->net_local_addr);
		if (record->net_foreign_addr != NULL)
			free(record->net_foreign_addr);
		if (record->dbus_bus != NULL)
			free(record->dbus_bus);
		if (record->dbus_path != NULL)
			free(record->dbus_path);
		if (record->dbus_interface != NULL)
			free(record->dbus_interface);
		if (record->dbus_member != NULL)
			free(record->dbus_member);
		if (record->signal != NULL)
			free(record->signal );
		if (record->fs_type != NULL)
			free(record->fs_type);
		if (record->flags != NULL)
			free(record->flags);
		if (record->src_name != NULL)
			free(record->src_name);

		free(record);
	}
	return;
}

/* Set all of the fields to appropriate values */
void _init_log_record(aa_log_record *record)
{
	if (record == NULL)
		return;

	memset(record, 0, sizeof(aa_log_record));

	record->version = AA_RECORD_SYNTAX_UNKNOWN;
	record->event = AA_RECORD_INVALID;
	record->fsuid = (unsigned long) -1;
	record->ouid = (unsigned long) -1;

	return;
}

/* convert a hex-encoded string to its char* version */
char *hex_to_string(char *hexstring)
{
	char *ret = NULL;
	char buf[3], *endptr;
	size_t len;
	int i;

	if (!hexstring)
		goto out;

	len = strlen(hexstring) / 2;
	ret = malloc(len + 1);
	if (!ret)
		goto out;

	for (i = 0; i < len; i++) {
		sprintf(buf, "%.2s", hexstring);
		hexstring += 2;
		ret[i] = (unsigned char) strtoul(buf, &endptr, 16);
	}
	ret[len] = '\0';

out:
	return ret;
}

struct ipproto_pairs {
	unsigned int protocol;
	char *protocol_name;
};

#define AA_GEN_PROTO_ENT(name, IP) {name, IP},

static struct ipproto_pairs ipproto_mappings[] = {
#include "af_protos.h"
	/* terminate */
	{0, NULL}
};

/* convert an ip protocol number to a string */
char *ipproto_to_string(unsigned int proto)
{
	char *ret = NULL;
	struct ipproto_pairs *current = ipproto_mappings;

	while (current->protocol != proto && current->protocol_name != NULL) {
		current++;
	}

	if (current->protocol_name) {
		ret = strdup(current->protocol_name);
	} else {
		if (!asprintf(&ret, "unknown(%u)", proto))
			ret = NULL;
	}

	return ret;
}

