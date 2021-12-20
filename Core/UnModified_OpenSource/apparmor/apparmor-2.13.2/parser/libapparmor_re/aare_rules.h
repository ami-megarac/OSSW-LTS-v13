/*
 * (C) 2006, 2007 Andreas Gruenbacher <agruen@suse.de>
 * Copyright (c) 2003-2008 Novell, Inc. (All rights reserved)
 * Copyright 2009-2012 Canonical Ltd.
 *
 * The libapparmor library is licensed under the terms of the GNU
 * Lesser General Public License, version 2.1. Please see the file
 * COPYING.LGPL.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Wrapper around the dfa to convert aa rules into a dfa
 */
#ifndef __LIBAA_RE_RULES_H
#define __LIBAA_RE_RULES_H

#include <stdint.h>

#include "apparmor_re.h"
#include "expr-tree.h"

class UniquePerm {
public:
	bool deny;
	bool exact_match;
	uint32_t perms;
	uint32_t audit;

	bool operator<(UniquePerm const &rhs)const
	{
		if (deny == rhs.deny) {
			if (exact_match == rhs.exact_match) {
				if (perms == rhs.perms)
					return audit < rhs.audit;
				return perms < rhs.perms;
			}
			return exact_match;
		}
		return deny;
	}
};

class UniquePermsCache {
public:
	typedef map<UniquePerm, Node*> UniquePermMap;
	typedef UniquePermMap::iterator iterator;
	UniquePermMap nodes;

	UniquePermsCache(void) { };
	~UniquePermsCache() { clear(); }

	void clear()
	{
		for (iterator i = nodes.begin(); i != nodes.end(); i++) {
			delete i->second;
		}
		nodes.clear();
	}

	Node *insert(bool deny, uint32_t perms, uint32_t audit,
		     bool exact_match)
	{
		UniquePerm tmp = { deny, exact_match, perms, audit };
		iterator res = nodes.find(tmp);
		if (res == nodes.end()) {
			Node *node;
			if (deny)
				node = new DenyMatchFlag(perms, audit);
			else if (exact_match)
				node = new ExactMatchFlag(perms, audit);
			else
				node = new MatchFlag(perms, audit);
			pair<iterator, bool> val = nodes.insert(make_pair(tmp, node));
			if (val.second == false)
				return val.first->second;
			return node;
		}
		return res->second;
	}
};

typedef std::map<Node *, Node *> PermExprMap;

class aare_rules {
	Node *root;
	void add_to_rules(Node *tree, Node *perms);
	UniquePermsCache unique_perms;
	PermExprMap expr_map;
 public:
	int reverse;
	int rule_count;
	aare_rules(void): root(NULL), unique_perms(), expr_map(), reverse(0), rule_count(0) { };
	aare_rules(int reverse): root(NULL), unique_perms(), expr_map(), reverse(reverse), rule_count(0) { };
	~aare_rules();

	bool add_rule(const char *rule, int deny, uint32_t perms,
		      uint32_t audit, dfaflags_t flags);
	bool add_rule_vec(int deny, uint32_t perms, uint32_t audit, int count,
			  const char **rulev, dfaflags_t flags);
	void *create_dfa(size_t *size, dfaflags_t flags);
};

#endif				/* __LIBAA_RE_RULES_H */
