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
#ifndef __AA_PTRACE_H
#define __AA_PTRACE_H

#include "immunix.h"
#include "parser.h"

#define AA_MAY_TRACE	AA_MAY_WRITE
#define AA_MAY_READBY	0x10		/* MAY_CREATE in new encoding */
#define AA_MAY_TRACEDBY	AA_MAY_APPEND
#define AA_VALID_PTRACE_PERMS (AA_MAY_READ | AA_MAY_TRACE | AA_MAY_READBY | \
			       AA_MAY_TRACEDBY)

int parse_ptrace_mode(const char *str_mode, int *mode, int fail);

class ptrace_rule: public rule_t {
	void move_conditionals(struct cond_entry *conds);
public:
	char *peer_label;
	int mode;
	int audit;
	int deny;

	ptrace_rule(int mode, struct cond_entry *conds);
	virtual ~ptrace_rule()
	{
		free(peer_label);
	};

	virtual ostream &dump(ostream &os);
	virtual int expand_variables(void);
	virtual int gen_policy_re(Profile &prof);
	virtual void post_process(Profile &prof unused) { };
};

#endif /* __AA_PTRACE_H */
