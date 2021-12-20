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

#include "parser.h"
#include "profile.h"
#include "ptrace.h"

#include <iomanip>
#include <string>
#include <iostream>
#include <sstream>

int parse_ptrace_mode(const char *str_mode, int *mode, int fail)
{
	return parse_X_mode("ptrace", AA_VALID_PTRACE_PERMS, str_mode, mode, fail);
}

void ptrace_rule::move_conditionals(struct cond_entry *conds)
{
	struct cond_entry *cond_ent;

	list_for_each(conds, cond_ent) {
		/* for now disallow keyword 'in' (list) */
		if (!cond_ent->eq)
			yyerror("keyword \"in\" is not allowed in ptrace rules\n");

		if (strcmp(cond_ent->name, "peer") == 0) {
			move_conditional_value("ptrace", &peer_label, cond_ent);
		} else {
			yyerror("invalid ptrace rule conditional \"%s\"\n",
				cond_ent->name);
		}
	}
}

ptrace_rule::ptrace_rule(int mode_p, struct cond_entry *conds):
	peer_label(NULL), audit(0), deny(0)
{
	if (mode_p) {
		if (mode_p & ~AA_VALID_PTRACE_PERMS)
			yyerror("mode contains invalid permissions for ptrace\n");
		mode = mode_p;
	} else {
		mode = AA_VALID_PTRACE_PERMS;
	}

	move_conditionals(conds);
	free_cond_list(conds);
}

ostream &ptrace_rule::dump(ostream &os)
{
	if (audit)
		os << "audit ";
	if (deny)
		os << "deny ";

	os << "ptrace";

	if (mode != AA_VALID_PTRACE_PERMS) {
		os << " (";

		if (mode & AA_MAY_READ)
			os << "read ";
		if (mode & AA_MAY_READBY)
			os << "readby ";
		if (mode & AA_MAY_TRACE)
			os << "trace ";
		if (mode & AA_MAY_TRACEDBY)
			os << "tracedby ";
		os << ")";
	}

	if (peer_label)
		os << " " << peer_label;

	os << ",\n";

	return os;
}


int ptrace_rule::expand_variables(void)
{
	return expand_entry_variables(&peer_label);
}

/* do we want to warn once/profile or just once per compile?? */
static void warn_once(const char *name)
{
	static const char *warned_name = NULL;

	if ((warnflags & WARN_RULE_NOT_ENFORCED) && warned_name != name) {
		cerr << "Warning from profile " << name << " (";
		if (current_filename)
			cerr << current_filename;
		else
			cerr << "stdin";
		cerr << ") ptrace rules not enforced\n";
		warned_name = name;
	}
}

int ptrace_rule::gen_policy_re(Profile &prof)
{
	std::ostringstream buffer;
	std::string buf;

	pattern_t ptype;
	int pos;

	/* ?? do we want to generate the rules in the policy so that it
	 * the compile could be used on another kernel unchanged??
	 * Current caching doesn't support this but in the future maybe
	 */
	if (!kernel_supports_ptrace) {
		warn_once(prof.name);
		return RULE_NOT_SUPPORTED;
	}

	/* always generate a label and ptrace entry */
	buffer << "(" << "\\x" << std::setfill('0') << std::setw(2) << std::hex << AA_CLASS_LABEL << "|)";

	buffer << "\\x" << std::setfill('0') << std::setw(2) << std::hex << AA_CLASS_PTRACE;

	if (peer_label) {
		ptype = convert_aaregex_to_pcre(peer_label, 0, glob_default, buf, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
		buffer << buf;
	} else {
		buffer << anyone_match_pattern;
	}

	buf = buffer.str();
	if (mode & AA_VALID_PTRACE_PERMS) {
		if (!prof.policy.rules->add_rule(buf.c_str(), deny, mode, audit,
						 dfaflags))
			goto fail;
	}

	return RULE_OK;

fail:
	return RULE_ERROR;
}

