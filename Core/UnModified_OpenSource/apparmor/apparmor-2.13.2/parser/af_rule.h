/*
 *   Copyright (c) 2014
 *   Canonical Ltd. (All rights reserved)
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
#ifndef __AA_AF_RULE_H
#define __AA_AF_RULE_H

#include "immunix.h"
#include "network.h"
#include "parser.h"
#include "profile.h"

#include "rule.h"

enum cond_side { local_cond, peer_cond, either_cond };

struct supported_cond {
	const char *name;
	bool supported;
	bool in;
	bool multivalue;
	enum cond_side side ;
};

class af_rule: public rule_t {
public:
	std::string af_name;
	char *sock_type;
	int sock_type_n;
	char *proto;
	int proto_n;
	char *label;
	char *peer_label;
	int mode;
	int audit;
	bool deny;

	af_rule(const char *name): af_name(name), sock_type(NULL),
		sock_type_n(-1), proto(NULL), proto_n(0), label(NULL),
		peer_label(NULL), mode(0), audit(0), deny(0)
	{}

	virtual ~af_rule()
	{
		free(sock_type);
		free(proto);
		free(label);
		free(peer_label);
	};

	bool cond_check(struct supported_cond *cond, struct cond_entry *ent,
			bool peer, const char *rname);
	int move_base_cond(struct cond_entry *conds, bool peer);

	virtual bool has_peer_conds(void) { return peer_label ? true : false; }
	virtual ostream &dump_prefix(ostream &os);
	virtual ostream &dump_local(ostream &os);
	virtual ostream &dump_peer(ostream &os);
	virtual ostream &dump(ostream &os);
	virtual int expand_variables(void);
	virtual int gen_policy_re(Profile &prof) = 0;
	virtual void post_process(Profile &prof unused) { };
};

#endif /* __AA_AF_RULE_H */
