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
#include <map>

#include "parser.h"
#include "profile.h"
#include "parser_yacc.h"
#include "signal.h"

#define MAXMAPPED_SIG 35
#define MINRT_SIG 128		/* base of RT sigs */
#define MAXRT_SIG 32		/* Max RT above MINRT_SIG */

/* Signal names mapped to and internal ordering */
static struct signal_map { const char *name; int num; } signal_map[] = {
	{"hup",		1},
	{"int",		2},
	{"quit",	3},
	{"ill",		4},
	{"trap",	5},
	{"abrt",	6},
	{"bus",		7},
	{"fpe",		8},
	{"kill",	9},
	{"usr1",	10},
	{"segv",	11},
	{"usr2",	12},
	{"pipe",	13},
	{"alrm",	14},
	{"term",	15},
	{"stkflt",	16},
	{"chld",	17},
	{"cont",	18},
	{"stop",	19},
	{"stp",		20},
	{"ttin",	21},
	{"ttou",	22},
	{"urg",		23},
	{"xcpu",	24},
	{"xfsz",	25},
	{"vtalrm",	26},
	{"prof",	27},
	{"winch",	28},
	{"io",		29},
	{"pwr",		30},
	{"sys",		31},
	{"emt",		32},
	{"exists",	35},

	/* terminate */
	{NULL,		0}
};

/* this table is ordered post sig_map[sig] mapping */
static const char *const sig_names[MAXMAPPED_SIG + 1] = {
	"unknown",
	"hup",
	"int",
	"quit",
	"ill",
	"trap",
	"abrt",
	"bus",
	"fpe",
	"kill",
	"usr1",
	"segv",
	"usr2",
	"pipe",
	"alrm",
	"term",
	"stkflt",
	"chld",
	"cont",
	"stop",
	"stp",
	"ttin",
	"ttou",
	"urg",
	"xcpu",
	"xfsz",
	"vtalrm",
	"prof",
	"winch",
	"io",
	"pwr",
	"sys",
	"emt",
	"lost",
	"unused",

	"exists",	/* always last existance test mapped to MAXMAPPED_SIG */
};


int parse_signal_mode(const char *str_mode, int *mode, int fail)
{
	return parse_X_mode("signal", AA_VALID_SIGNAL_PERMS, str_mode, mode, fail);
}

static int find_signal_mapping(const char *sig)
{
	if (strncmp("rtmin+", sig, 6) == 0) {
		char *end;
		unsigned long n = strtoul(sig + 6, &end, 10);
		if (end == sig || n > MAXRT_SIG)
			return -1;
		return MINRT_SIG + n;
	} else {
		for (int i = 0; signal_map[i].name; i++) {
			if (strcmp(sig, signal_map[i].name) == 0)
				return signal_map[i].num;
		}
	}
	return -1;
}

void signal_rule::extract_sigs(struct value_list **list)
{
	struct value_list *entry, *tmp, *prev = NULL;
	list_for_each_safe(*list, entry, tmp) {
		int i = find_signal_mapping(entry->value);
		if (i != -1) {
			signals.insert(i);
			list_remove_at(*list, prev, entry);
			free_value_list(entry);
		} else {
			yyerror("unknown signal \"%s\"\n", entry->value);
			prev = entry;
		}
	}
}

void signal_rule::move_conditionals(struct cond_entry *conds)
{
	struct cond_entry *cond_ent;

	list_for_each(conds, cond_ent) {
		/* for now disallow keyword 'in' (list) */
		if (!cond_ent->eq)
			yyerror("keyword \"in\" is not allowed in signal rules\n");
		if (strcmp(cond_ent->name, "set") == 0) {
			extract_sigs(&cond_ent->vals);
		} else if (strcmp(cond_ent->name, "peer") == 0) {
			move_conditional_value("signal", &peer_label, cond_ent);
		} else {
			yyerror("invalid signal rule conditional \"%s\"\n",
				cond_ent->name);
		}
	}
}

signal_rule::signal_rule(int mode_p, struct cond_entry *conds):
	signals(), peer_label(NULL), audit(0), deny(0)
{
	if (mode_p) {
		mode = mode_p;
		if (mode & ~AA_VALID_SIGNAL_PERMS)
			yyerror("mode contains invalid permission for signals\n");
	} else {
		mode = AA_VALID_SIGNAL_PERMS;
	}

	move_conditionals(conds);

	free_cond_list(conds);
}

ostream &signal_rule::dump(ostream &os)
{
	if (audit)
		os << "audit ";
	if (deny)
		os << "deny ";

	os << "signal";

	if (mode != AA_VALID_SIGNAL_PERMS) {
		os << " (";

		if (mode & AA_MAY_SEND)
			os << "send ";
		if (mode & AA_MAY_RECEIVE)
			os << "receive ";
		os << ")";
	}

	if (!signals.empty()) {
		os << " set=(";
		for (Signals::iterator i = signals.begin(); i != signals.end();
		     i++) {
			if (i != signals.begin())
				os << ", ";
			if (*i < MINRT_SIG)
				os << sig_names[*i];
			else
				os << "rtmin+" << (*i - MINRT_SIG);
		}
		os << ")";
	}

	if (peer_label)
		os << " " << peer_label;

	os << ",\n";

	return os;
}

int signal_rule::expand_variables(void)
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
		cerr << ") signal rules not enforced\n";
		warned_name = name;
	}
}

int signal_rule::gen_policy_re(Profile &prof)
{
	std::ostringstream buffer;
	std::string buf;

	pattern_t ptype;
	int pos;

	/* Currently do not generate the rules if the kernel doesn't support
	 * it. We may want to switch this so that a compile could be
	 * used for full support on kernels that don't support the feature
	 */
	if (!kernel_supports_signal) {
		warn_once(prof.name);
		return RULE_NOT_SUPPORTED;
	}

	if (signals.size() == 0) {
		/* not conditional on signal set, so will generate a label
		 * rule as well
		 */
		buffer << "(" << "\\x" << std::setfill('0') << std::setw(2) << std::hex << AA_CLASS_LABEL << "\\x" << AA_CLASS_SIGNAL << "|";
	}

	buffer << "\\x" << std::setfill('0') << std::setw(2) << std::hex << AA_CLASS_SIGNAL;

	if (signals.size()) {
		buffer << "[";
		for (Signals::iterator i = signals.begin(); i != signals.end(); i++) {
			buffer << "\\x" << std::setfill('0') << std::setw(2) << std::hex << *i;
		}
		buffer << "]";
	} else {
		/* match any char */
		buffer << ".";
	}

	if (signals.size() == 0) {
		/* close alternation */
		buffer << ")";
	}
	if (peer_label) {
		ptype = convert_aaregex_to_pcre(peer_label, 0, glob_default, buf, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
		buffer << buf;
	} else {
		buffer << anyone_match_pattern;
	}

	buf = buffer.str();
	if (mode & (AA_MAY_SEND | AA_MAY_RECEIVE)) {
		if (!prof.policy.rules->add_rule(buf.c_str(), deny, mode, audit,
						 dfaflags))
			goto fail;
	}

	return RULE_OK;

fail:
	return RULE_ERROR;
}
