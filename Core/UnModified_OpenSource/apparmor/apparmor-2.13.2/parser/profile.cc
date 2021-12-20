/*
 *   Copyright (c) 2012, 2013
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
 */

#include "profile.h"
#include "rule.h"

#include <stdio.h>
#include <stdlib.h>

bool deref_profileptr_lt::operator()(Profile * const &lhs, Profile * const &rhs) const
{
  return *lhs < *rhs;
};

pair<ProfileList::iterator,bool> ProfileList::insert(Profile *p)
{
	return list.insert(p);
}

void ProfileList::erase(ProfileList::iterator pos)
{
	list.erase(pos);
}

void ProfileList::clear(void)
{
	for(ProfileList::iterator i = list.begin(); i != list.end(); ) {
		ProfileList::iterator k = i++;
		delete *k;
		list.erase(k);
	}
}

void ProfileList::dump(void)
{
	for(ProfileList::iterator i = list.begin(); i != list.end(); i++) {
		(*i)->dump();
	}
}

void ProfileList::dump_profile_names(bool children)
{
	for (ProfileList::iterator i = list.begin(); i != list.end();i++) {
		(*i)->dump_name(true);
		printf("\n");
		if (children && !(*i)->hat_table.empty())
			(*i)->hat_table.dump_profile_names(children);
	}
}

bool Profile::alloc_net_table()
{
	if (net.allow)
		return true;
	net.allow = (unsigned int *) calloc(get_af_max(), sizeof(unsigned int));
	net.audit = (unsigned int *) calloc(get_af_max(), sizeof(unsigned int));
	net.deny = (unsigned int *) calloc(get_af_max(), sizeof(unsigned int));
	net.quiet = (unsigned int *) calloc(get_af_max(), sizeof(unsigned int));
	if (!net.allow || !net.audit || !net.deny || !net.quiet)
		return false;

	return true;
}

Profile::~Profile()
{
	hat_table.clear();
	free_cod_entries(entries);

	for (RuleList::iterator i = rule_ents.begin(); i != rule_ents.end(); i++)
		delete *i;
	if (dfa.rules)
		delete dfa.rules;
	if (dfa.dfa)
		free(dfa.dfa);
	if (policy.rules)
		delete policy.rules;
	if (policy.dfa)
		free(policy.dfa);
	if (xmatch)
		free(xmatch);
	if (name)
		free(name);
	if (attachment)
		free(attachment);
	if (ns)
		free(ns);
	for (int i = (AA_EXEC_LOCAL >> 10) + 1; i < AA_EXEC_COUNT; i++)
		if (exec_table[i])
			free(exec_table[i]);
	if (net.allow)
		free(net.allow);
	if (net.audit)
		free(net.audit);
	if (net.deny)
		free(net.deny);
	if (net.quiet)
		free(net.quiet);
}

