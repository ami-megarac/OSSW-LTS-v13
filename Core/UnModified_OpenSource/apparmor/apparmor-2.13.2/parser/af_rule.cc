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
#include <iostream>
#include <sstream>

#include "network.h"
#include "parser.h"
#include "profile.h"
#include "parser_yacc.h"
#include "af_rule.h"


/* need to move this table to stl */
static struct supported_cond supported_conds[] = {
	{ "type", true, false, false, local_cond },
	{ "protocol", false, false, false, local_cond },
	{ "label", true, false, false, peer_cond },
	{ NULL, false, false, false, local_cond },	/* eol sentinal */
};

bool af_rule::cond_check(struct supported_cond *conds, struct cond_entry *ent,
			 bool peer, const char *rname)
{
	struct supported_cond *i;
	for (i = conds; i->name; i++) {
		if (strcmp(ent->name, i->name) != 0)
			continue;
		if (!i->supported)
			yyerror("%s rule: '%s' conditional is not currently supported\n", rname, ent->name);
		if (!peer && (i->side == peer_cond))
			yyerror("%s rule: '%s' conditional is only valid in the peer expression\n", rname, ent->name);
		if (peer && (i->side == local_cond))
			yyerror("%s rule: '%s' conditional is not allowed in the peer expression\n", rname, ent->name);
		if (!ent->eq && !i->in)
			yyerror("%s rule: keyword 'in' is not allowed in '%s' socket conditional\n", rname, ent->name);
		if (list_len(ent->vals) > 1 && !i->multivalue)
			yyerror("%s rule: conditional '%s' only supports a single value\n", rname, ent->name);
		return true;
	}

	/* not in support table */
	return false;
}

/* generic af supported conds.
 * returns: true if processed, else false
 */
int af_rule::move_base_cond(struct cond_entry *ent, bool peer)
{
	if (!cond_check(supported_conds, ent, peer, "unknown"))
		return false;

	if (strcmp(ent->name, "type") == 0) {
		move_conditional_value("socket rule", &sock_type, ent);
		sock_type_n = net_find_type_val(sock_type);
		if (sock_type_n == -1)
			yyerror("socket rule: invalid socket type '%s'", sock_type);
	} else if (strcmp(ent->name, "protocol") == 0) {
		yyerror("socket rule: 'protocol' conditional is not currently supported\n");
	} else if (strcmp(ent->name, "label") == 0) {
		if (!peer)
			move_conditional_value("unix", &label, ent);
		else
			move_conditional_value("unix", &peer_label, ent);
	} else
		return false;

	return true;
}

ostream &af_rule::dump_prefix(ostream &os)
{
	if (audit)
		os << "audit ";
	if (deny)
		os << "deny ";
	return os;
}

ostream &af_rule::dump_local(ostream &os)
{
	if (mode != AA_VALID_NET_PERMS) {
		os << " (";

		if (mode & AA_NET_SEND)
			os << "send ";
		if (mode & AA_NET_RECEIVE)
			os << "receive ";
		if (mode & AA_NET_CREATE)
			os << "create ";
		if (mode & AA_NET_SHUTDOWN)
			os << "shutdown ";
		if (mode & AA_NET_CONNECT)
			os << "connect ";
		if (mode & AA_NET_SETATTR)
			os << "setattr ";
		if (mode & AA_NET_GETATTR)
			os << "getattr ";
		if (mode & AA_NET_BIND)
			os << "bind ";
		if (mode & AA_NET_ACCEPT)
			os << "accept ";
		if (mode & AA_NET_LISTEN)
			os << "listen ";
		if (mode & AA_NET_SETOPT)
			os << "setopt ";
		if (mode & AA_NET_GETOPT)
			os << "getopt ";
		os << ")";
	}

	if (sock_type)
		os << " type=" << sock_type;
	if (proto)
		os << " protocol=" << proto;
	return os;
}

ostream &af_rule::dump_peer(ostream &os)
{
	if (peer_label)
		os << " label=\"" << peer_label << "\"";

	return os;
}

ostream &af_rule::dump(ostream &os)
{
	dump_prefix(os);
	os << af_name;
	dump_local(os);
	if (has_peer_conds()) {
		os << " peer=(";
		dump_peer(os);
		os  << ")";
	}
	os << ",\n";

	return os;
}

int af_rule::expand_variables(void)
{
	int error = expand_entry_variables(&label);
	if (error)
		return error;
	error = expand_entry_variables(&peer_label);
	if (error)
		return error;

	return 0;
}
