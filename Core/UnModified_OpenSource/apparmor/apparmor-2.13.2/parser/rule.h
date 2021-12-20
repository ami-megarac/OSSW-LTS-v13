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
#ifndef __AA_RULE_H
#define __AA_RULE_H

#include <list>
#include <ostream>

#include "policydb.h"

class Profile;

#define RULE_NOT_SUPPORTED 0
#define RULE_ERROR -1
#define RULE_OK 1

class rule_t {
public:
	virtual ~rule_t() { };

	//virtual bool operator<(rule_t const &rhs)const = 0;
	virtual std::ostream &dump(std::ostream &os) = 0;
	virtual int expand_variables(void) = 0;
	virtual int gen_policy_re(Profile &prof) = 0;
	virtual void post_process(Profile &prof) = 0;
};

std::ostream &operator<<(std::ostream &os, rule_t &rule);

typedef std::list<rule_t *> RuleList;

#endif /* __AA_RULE_H */

