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
#ifndef __AA_AF_UNIX_H
#define __AA_AF_UNIX_H

#include "immunix.h"
#include "network.h"
#include "parser.h"
#include "profile.h"
#include "af_rule.h"

int parse_unix_mode(const char *str_mode, int *mode, int fail);

class unix_rule: public af_rule {
	void write_to_prot(std::ostringstream &buffer);
	bool write_addr(std::ostringstream &buffer, const char *addr);
	bool write_label(std::ostringstream &buffer, const char *label);
	void move_conditionals(struct cond_entry *conds);
	void move_peer_conditionals(struct cond_entry *conds);
	void downgrade_rule(Profile &prof);
public:
	char *addr;
	char *peer_addr;
	int mode;
	int audit;
	bool deny;

	unix_rule(unsigned int type_p, bool audit_p, bool denied);
	unix_rule(int mode, struct cond_entry *conds,
		  struct cond_entry *peer_conds);
	virtual ~unix_rule()
	{
		free(addr);
		free(peer_addr);
	};

	virtual bool has_peer_conds(void) {
		return af_rule::has_peer_conds() || peer_addr;
	}

	virtual ostream &dump_local(ostream &os);
	virtual ostream &dump_peer(ostream &os);
	virtual int expand_variables(void);
	virtual int gen_policy_re(Profile &prof);
	virtual void post_process(Profile &prof unused) { };
};

#endif /* __AA_AF_UNIX_H */
