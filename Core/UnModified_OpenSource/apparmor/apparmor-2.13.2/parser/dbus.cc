/*
 *   Copyright (c) 2013
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

#include "parser.h"
#include "profile.h"
#include "parser_yacc.h"
#include "dbus.h"


int parse_dbus_mode(const char *str_mode, int *mode, int fail)
{
	return parse_X_mode("DBus", AA_VALID_DBUS_PERMS, str_mode, mode, fail);
}

void dbus_rule::move_conditionals(struct cond_entry *conds)
{
	struct cond_entry *cond_ent;

	list_for_each(conds, cond_ent) {
		/* for now disallow keyword 'in' (list) */
		if (!cond_ent->eq)
			yyerror("keyword \"in\" is not allowed in dbus rules\n");
		if (list_len(cond_ent->vals) > 1)
			yyerror("dbus conditional \"%s\" only supports a single value\n",
				cond_ent->name);

		if (strcmp(cond_ent->name, "bus") == 0) {
			move_conditional_value("dbus", &bus, cond_ent);
		} else if (strcmp(cond_ent->name, "name") == 0) {
			move_conditional_value("dbus", &name, cond_ent);
		} else if (strcmp(cond_ent->name, "label") == 0) {
			move_conditional_value("dbus", &peer_label, cond_ent);
		} else if (strcmp(cond_ent->name, "path") == 0) {
			move_conditional_value("dbus", &path, cond_ent);
		} else if (strcmp(cond_ent->name, "interface") == 0) {
			move_conditional_value("dbus", &interface, cond_ent);
		} else if (strcmp(cond_ent->name, "member") == 0) {
			move_conditional_value("dbus", &member, cond_ent);
		} else {
			yyerror("invalid dbus conditional \"%s\"\n",
				cond_ent->name);
		}
	}
}

dbus_rule::dbus_rule(int mode_p, struct cond_entry *conds,
		     struct cond_entry *peer_conds):
	bus(NULL), name(NULL), peer_label(NULL), path(NULL), interface(NULL), member(NULL),
	mode(0), audit(0), deny(0)
{
	int name_is_subject_cond = 0, message_rule = 0, service_rule = 0;

	/* Move the global/subject conditionals over & check the results */
	move_conditionals(conds);
	if (name)
		name_is_subject_cond = 1;
	if (peer_label)
		yyerror("dbus \"label\" conditional can only be used inside of the \"peer=()\" grouping\n");

	/* Move the peer conditionals */
	move_conditionals(peer_conds);

	if (path || interface || member || peer_label ||
	    (name && !name_is_subject_cond))
		message_rule = 1;

	if (name && name_is_subject_cond)
		service_rule = 1;

	if (message_rule && service_rule)
		yyerror("dbus rule contains message conditionals and service conditionals\n");

	/* Copy mode. If no mode was specified, assign an implied mode. */
	if (mode_p) {
		mode = mode_p;
		if (mode & ~AA_VALID_DBUS_PERMS)
			yyerror("mode contains unknown dbus access\n");
		else if (message_rule && (mode & AA_DBUS_BIND))
			yyerror("dbus \"bind\" access cannot be used with message rule conditionals\n");
		else if (service_rule && (mode & (AA_DBUS_SEND | AA_DBUS_RECEIVE)))
			yyerror("dbus \"send\" and/or \"receive\" accesses cannot be used with service rule conditionals\n");
		else if (mode & AA_DBUS_EAVESDROP &&
			 (path || interface || member ||
			  peer_label || name)) {
			yyerror("dbus \"eavesdrop\" access can only contain a bus conditional\n");
		}
	} else {
		if (message_rule)
			mode = (AA_DBUS_SEND | AA_DBUS_RECEIVE);
		else if (service_rule)
			mode = (AA_DBUS_BIND);
		else
			mode = AA_VALID_DBUS_PERMS;
	}

	free_cond_list(conds);
	free_cond_list(peer_conds);
}

ostream &dbus_rule::dump(ostream &os)
{
	if (audit)
		os << "audit ";
	if (deny)
		os << "deny ";

	os << "dbus ( ";

	if (mode & AA_DBUS_SEND)
		os << "send ";
	if (mode & AA_DBUS_RECEIVE)
		os << "receive ";
	if (mode & AA_DBUS_BIND)
		os << "bind ";
	if (mode & AA_DBUS_EAVESDROP)
		os << "eavesdrop ";
	os << ")";

	if (bus)
		os << " bus=\"" << bus << "\"";
	if ((mode & AA_DBUS_BIND) && name)
		os << " name=\"" << name << "\"";
	if (path)
		os << " path=\"" << path << "\"";
	if (interface)
		os << " interface=\"" << interface << "\"";
	if (member)
		os << " member=\"" << member << "\"";

	if (!(mode & AA_DBUS_BIND) && (peer_label || name)) {
		os << " peer=( ";
		if (peer_label)
			os << "label=\"" << peer_label << "\" ";
		if (name)
			os << "name=\"" << name << "\" ";
		os << ")";
	}

	os << ",\n";

	return os;
}

int dbus_rule::expand_variables(void)
{
	int error = expand_entry_variables(&bus);
	if (error)
		return error;
	error = expand_entry_variables(&name);
	if (error)
		return error;
	error = expand_entry_variables(&peer_label);
	if (error)
		return error;
	error = expand_entry_variables(&path);
	if (error)
		return error;
	error = expand_entry_variables(&interface);
	if (error)
		return error;
	error = expand_entry_variables(&member);
	if (error)
		return error;

	return 0;
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
		cerr << ") dbus rules not enforced\n";
		warned_name = name;
	}
}

int dbus_rule::gen_policy_re(Profile &prof)
{
	std::string busbuf;
	std::string namebuf;
	std::string peer_labelbuf;
	std::string pathbuf;
	std::string ifacebuf;
	std::string memberbuf;
	std::ostringstream buffer;
	const char *vec[6];

	pattern_t ptype;
	int pos;

	if (!kernel_supports_dbus) {
		warn_once(prof.name);
		return RULE_NOT_SUPPORTED;
	}

	buffer << "\\x" << std::setfill('0') << std::setw(2) << std::hex << AA_CLASS_DBUS;
	busbuf.append(buffer.str());

	if (bus) {
		ptype = convert_aaregex_to_pcre(bus, 0, glob_default, busbuf, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
	} else {
		/* match any char except \000 0 or more times */
		busbuf.append(default_match_pattern);
	}
	vec[0] = busbuf.c_str();

	if (name) {
		ptype = convert_aaregex_to_pcre(name, 0, glob_default, namebuf, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
		vec[1] = namebuf.c_str();
	} else {
		/* match any char except \000 0 or more times */
		vec[1] = default_match_pattern;
	}

	if (peer_label) {
		ptype = convert_aaregex_to_pcre(peer_label, 0, glob_default,
						peer_labelbuf, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
		vec[2] = peer_labelbuf.c_str();
	} else {
		/* match any char except \000 0 or more times */
		vec[2] = default_match_pattern;
	}

	if (path) {
		ptype = convert_aaregex_to_pcre(path, 0, glob_default, pathbuf, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
		vec[3] = pathbuf.c_str();
	} else {
		/* match any char except \000 0 or more times */
		vec[3] = default_match_pattern;
	}

	if (interface) {
		ptype = convert_aaregex_to_pcre(interface, 0, glob_default, ifacebuf, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
		vec[4] = ifacebuf.c_str();
	} else {
		/* match any char except \000 0 or more times */
		vec[4] = default_match_pattern;
	}

	if (member) {
		ptype = convert_aaregex_to_pcre(member, 0, glob_default, memberbuf, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
		vec[5] = memberbuf.c_str();
	} else {
		/* match any char except \000 0 or more times */
		vec[5] = default_match_pattern;
	}

	if (mode & AA_DBUS_BIND) {
		if (!prof.policy.rules->add_rule_vec(deny, mode & AA_DBUS_BIND,
						    audit & AA_DBUS_BIND,
						    2, vec, dfaflags))
			goto fail;
	}
	if (mode & (AA_DBUS_SEND | AA_DBUS_RECEIVE)) {
		if (!prof.policy.rules->add_rule_vec(deny,
				       mode & (AA_DBUS_SEND | AA_DBUS_RECEIVE),
				       audit & (AA_DBUS_SEND | AA_DBUS_RECEIVE),
				       6, vec, dfaflags))
			goto fail;
	}
	if (mode & AA_DBUS_EAVESDROP) {
		if (!prof.policy.rules->add_rule_vec(deny,
						    mode & AA_DBUS_EAVESDROP,
						    audit & AA_DBUS_EAVESDROP,
						    1, vec, dfaflags))
			goto fail;
	}

	return RULE_OK;

fail:
	return RULE_ERROR;
}
