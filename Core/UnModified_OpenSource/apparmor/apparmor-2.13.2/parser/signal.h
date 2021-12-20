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

#ifndef __AA_SIGNAL_H
#define __AA_SIGNAL_H

#include "parser.h"
#include "rule.h"
#include "profile.h"


#define AA_MAY_SEND			(1 << 1)
#define AA_MAY_RECEIVE			(1 << 2)
#define AA_VALID_SIGNAL_PERMS		(AA_MAY_SEND | AA_MAY_RECEIVE)


typedef set<int> Signals;

int parse_signal_mode(const char *str_mode, int *mode, int fail);

class signal_rule: public rule_t {
	void extract_sigs(struct value_list **list);
	void move_conditionals(struct cond_entry *conds);
public:
	Signals signals;
	char *peer_label;
	int mode;
	int audit;
	int deny;

	signal_rule(int mode, struct cond_entry *conds);
	virtual ~signal_rule() {
		signals.clear();
		free(peer_label);
	};

	virtual ostream &dump(ostream &os);
	virtual int expand_variables(void);
	virtual int gen_policy_re(Profile &prof);
	virtual void post_process(Profile &prof unused) { };
};

#endif /* __AA_SIGNAL_H */
