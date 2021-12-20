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
 * Create a compressed hfa from and hfa
 */

#include <map>
#include <vector>
#include <ostream>
#include <iostream>
#include <fstream>

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "hfa.h"
#include "chfa.h"
#include "../immunix.h"
#include "flex-tables.h"

void CHFA::init_free_list(vector<pair<size_t, size_t> > &free_list,
				     size_t prev, size_t start)
{
	for (size_t i = start; i < free_list.size(); i++) {
		if (prev)
			free_list[prev].second = i;
		free_list[i].first = prev;
		prev = i;
	}
	free_list[free_list.size() - 1].second = 0;
}

/**
 * new Construct the transition table.
 */
CHFA::CHFA(DFA &dfa, map<uchar, uchar> &eq, dfaflags_t flags): eq(eq)
{
	if (flags & DFA_DUMP_TRANS_PROGRESS)
		fprintf(stderr, "Compressing HFA:\r");

	if (dfa.diffcount)
		chfaflags = YYTH_FLAG_DIFF_ENCODE;
	else
		chfaflags = 0;

	if (eq.empty())
		max_eq = 255;
	else {
		max_eq = 0;
		for (map<uchar, uchar>::iterator i = eq.begin();
		     i != eq.end(); i++) {
			if (i->second > max_eq)
				max_eq = i->second;
		}
	}

	/* Do initial setup adding up all the transitions and sorting by
	 * transition count.
	 */
	size_t optimal = 2;
	multimap<size_t, State *> order;
	vector<pair<size_t, size_t> > free_list;

	for (Partition::iterator i = dfa.states.begin(); i != dfa.states.end(); i++) {
		if (*i == dfa.start || *i == dfa.nonmatching)
			continue;
		optimal += (*i)->trans.size();
		if (flags & DFA_CONTROL_TRANS_HIGH) {
			size_t range = 0;
			if ((*i)->trans.size())
				range =
				    (*i)->trans.rbegin()->first -
				    (*i)->trans.begin()->first;
			size_t ord = ((256 - (*i)->trans.size()) << 8) | (256 - range);
			/* reverse sort by entry count, most entries first */
			order.insert(make_pair(ord, *i));
		}
	}

	/* Insert the dummy nonmatching transition by hand */
	next_check.push_back(make_pair(dfa.nonmatching, dfa.nonmatching));
	default_base.push_back(make_pair(dfa.nonmatching, 0));
	num.insert(make_pair(dfa.nonmatching, num.size()));

	accept.resize(max(dfa.states.size(), (size_t) 2));
	accept2.resize(max(dfa.states.size(), (size_t) 2));
	next_check.resize(max(optimal, (size_t) 256));
	free_list.resize(next_check.size());

	accept[0] = 0;
	accept2[0] = 0;
	first_free = 1;
	init_free_list(free_list, 0, 1);

	insert_state(free_list, dfa.start, dfa);
	accept[1] = 0;
	accept2[1] = 0;
	num.insert(make_pair(dfa.start, num.size()));

	int count = 2;

	if (!(flags & DFA_CONTROL_TRANS_HIGH)) {
		for (Partition::iterator i = dfa.states.begin(); i != dfa.states.end(); i++) {
			if (*i != dfa.nonmatching && *i != dfa.start) {
				insert_state(free_list, *i, dfa);
				accept[num.size()] = (*i)->perms.allow;
				accept2[num.size()] = PACK_AUDIT_CTL((*i)->perms.audit, (*i)->perms.quiet & (*i)->perms.deny);
				num.insert(make_pair(*i, num.size()));
			}
			if (flags & (DFA_DUMP_TRANS_PROGRESS)) {
				count++;
				if (count % 100 == 0)
					fprintf(stderr, "\033[2KCompressing trans table: insert state: %d/%zd\r",
						count, dfa.states.size());
			}
		}
	} else {
		for (multimap<size_t, State *>::iterator i = order.begin();
		     i != order.end(); i++) {
			if (i->second != dfa.nonmatching &&
			    i->second != dfa.start) {
				insert_state(free_list, i->second, dfa);
				accept[num.size()] = i->second->perms.allow;
				accept2[num.size()] = PACK_AUDIT_CTL(i->second->perms.audit, i->second->perms.quiet & i->second->perms.deny);
				num.insert(make_pair(i->second, num.size()));
			}
			if (flags & (DFA_DUMP_TRANS_PROGRESS)) {
				count++;
				if (count % 100 == 0)
					fprintf(stderr, "\033[2KCompressing trans table: insert state: %d/%zd\r",
						count, dfa.states.size());
			}
		}
	}

	if (flags & (DFA_DUMP_TRANS_STATS | DFA_DUMP_TRANS_PROGRESS)) {
		ssize_t size = 4 * next_check.size() + 6 * dfa.states.size();
		fprintf(stderr, "\033[2KCompressed trans table: states %zd, next/check %zd, optimal next/check %zd avg/state %.2f, compression %zd/%zd = %.2f %%\n",
			dfa.states.size(), next_check.size(), optimal,
			(float)next_check.size() / (float)dfa.states.size(),
			size, 512 * dfa.states.size(),
			100.0 - ((float)size * 100.0 /(float)(512 * dfa.states.size())));
	}
}

/**
 * Does <trans> fit into position <base> of the transition table?
 */
bool CHFA::fits_in(vector<pair<size_t, size_t> > &free_list
			      __attribute__ ((unused)), size_t pos,
			      StateTrans &trans)
{
	size_t c, base = pos - trans.begin()->first;
	for (StateTrans::iterator i = trans.begin(); i != trans.end(); i++) {
		c = base + i->first;
		/* if it overflows the next_check array it fits in as we will
		 * resize */
		if (c >= next_check.size())
			return true;
		if (next_check[c].second)
			return false;
	}

	return true;
}

/**
 * Insert <state> of <dfa> into the transition table.
 */
void CHFA::insert_state(vector<pair<size_t, size_t> > &free_list,
				   State *from, DFA &dfa)
{
	State *default_state = dfa.nonmatching;
	size_t base = 0;
	int resize;

	StateTrans &trans = from->trans;
	size_t c = trans.begin()->first;
	size_t prev = 0;
	size_t x = first_free;

	if (from->otherwise)
		default_state = from->otherwise;
	if (trans.empty())
		goto do_insert;

repeat:
	resize = 0;
	/* get the first free entry that won't underflow */
	while (x && (x < c)) {
		prev = x;
		x = free_list[x].second;
	}

	/* try inserting until we succeed. */
	while (x && !fits_in(free_list, x, trans)) {
		prev = x;
		x = free_list[x].second;
	}
	if (!x) {
		resize = 256 - trans.begin()->first;
		x = free_list.size();
		/* set prev to last free */
	} else if (x + 255 - trans.begin()->first >= next_check.size()) {
		resize = (255 - trans.begin()->first - (next_check.size() - 1 - x));
		for (size_t y = x; y; y = free_list[y].second)
			prev = y;
	}
	if (resize) {
		/* expand next_check and free_list */
		size_t old_size = free_list.size();
		next_check.resize(next_check.size() + resize);
		free_list.resize(free_list.size() + resize);
		init_free_list(free_list, prev, old_size);
		if (!first_free)
			first_free = old_size;;
		if (x == old_size)
			goto repeat;
	}

	base = x - c;
	for (StateTrans::iterator j = trans.begin(); j != trans.end(); j++) {
		next_check[base + j->first] = make_pair(j->second, from);
		size_t prev = free_list[base + j->first].first;
		size_t next = free_list[base + j->first].second;
		if (prev)
			free_list[prev].second = next;
		if (next)
			free_list[next].first = prev;
		if (base + j->first == first_free)
			first_free = next;
	}

do_insert:
	if (from->flags & DiffEncodeFlag)
		base |= DiffEncodeBit32;
	default_base.push_back(make_pair(default_state, base));
}

/**
 * Text-dump the transition table (for debugging).
 */
void CHFA::dump(ostream &os)
{
	map<size_t, const State *> st;
	for (map<const State *, size_t>::iterator i = num.begin(); i != num.end(); i++) {
		st.insert(make_pair(i->second, i->first));
	}

	os << "size=" << default_base.size() << " (accept, default, base):  {state} -> {default state}" << "\n";
	for (size_t i = 0; i < default_base.size(); i++) {
		os << i << ": ";
		os << "(" << accept[i] << ", " << num[default_base[i].first]
		   << ", " << default_base[i].second << ")";
		if (st[i])
			os << " " << *st[i];
		if (default_base[i].first)
			os << " -> " << *default_base[i].first;
		os << "\n";
	}

	os << "size=" << next_check.size() << " (next, check): {check state} -> {next state} : offset from base\n";
	for (size_t i = 0; i < next_check.size(); i++) {
		if (!next_check[i].second)
			continue;

		os << i << ": ";
		if (next_check[i].second) {
			os << "(" << num[next_check[i].first] << ", "
			   << num[next_check[i].second] << ")" << " "
			   << *next_check[i].second << " -> "
			   << *next_check[i].first << ": ";

			size_t offs = i - base_mask_size(default_base[num[next_check[i].second]].second);
			if (eq.size())
				os << offs;
			else
				os << (uchar) offs;
		}
		os << "\n";
	}
}

/**
 * Create a flex-style binary dump of the DFA tables. The table format
 * was partly reverse engineered from the flex sources and from
 * examining the tables that flex creates with its --tables-file option.
 * (Only the -Cf and -Ce formats are currently supported.)
 */

#define YYTH_REGEX_MAGIC 0x1B5E783D

static inline size_t pad64(size_t i)
{
	return (i + (size_t) 7) & ~(size_t) 7;
}

string fill64(size_t i)
{
	const char zeroes[8] = { };
	string fill(zeroes, (i & 7) ? 8 - (i & 7) : 0);
	return fill;
}

template<class Iter> size_t flex_table_size(Iter pos, Iter end)
{
	return pad64(sizeof(struct table_header) + sizeof(*pos) * (end - pos));
}

template<class Iter>
    void write_flex_table(ostream &os, int id, Iter pos, Iter end)
{
	struct table_header td = { 0, 0, 0, 0 };
	size_t size = end - pos;

	td.td_id = htons(id);
	td.td_flags = htons(sizeof(*pos));
	td.td_lolen = htonl(size);
	os.write((char *)&td, sizeof(td));

	for (; pos != end; ++pos) {
		switch (sizeof(*pos)) {
		case 4:
			os.put((char)(*pos >> 24));
			os.put((char)(*pos >> 16));
		case 2:
			os.put((char)(*pos >> 8));
		case 1:
			os.put((char)*pos);
		}
	}

	os << fill64(sizeof(td) + sizeof(*pos) * size);
}

void CHFA::flex_table(ostream &os, const char *name)
{
	const char th_version[] = "notflex";
	struct table_set_header th = { 0, 0, 0, 0 };

	/**
	 * Change the following two data types to adjust the maximum flex
	 * table size.
	 */
	typedef uint16_t state_t;
	typedef uint32_t trans_t;

	if (default_base.size() >= (state_t) - 1) {
		cerr << "Too many states (" << default_base.size() << ") for "
		    "type state_t\n";
		exit(1);
	}
	if (next_check.size() >= (trans_t) - 1) {
		cerr << "Too many transitions (" << next_check.size()
		     << ") for " "type trans_t\n";
		exit(1);
	}

	/**
	 * Create copies of the data structures so that we can dump the tables
	 * using the generic write_flex_table() routine.
	 */
	vector<uint8_t> equiv_vec;
	if (eq.size()) {
		equiv_vec.resize(256);
		for (map<uchar, uchar>::iterator i = eq.begin(); i != eq.end(); i++) {
			equiv_vec[i->first] = i->second;
		}
	}

	vector<state_t> default_vec;
	vector<trans_t> base_vec;
	for (DefaultBase::iterator i = default_base.begin(); i != default_base.end(); i++) {
		default_vec.push_back(num[i->first]);
		base_vec.push_back(i->second);
	}

	vector<state_t> next_vec;
	vector<state_t> check_vec;
	for (NextCheck::iterator i = next_check.begin(); i != next_check.end(); i++) {
		next_vec.push_back(num[i->first]);
		check_vec.push_back(num[i->second]);
	}

	/* Write the actual flex parser table. */

	size_t hsize = pad64(sizeof(th) + sizeof(th_version) + strlen(name) + 1);
	th.th_magic = htonl(YYTH_REGEX_MAGIC);
	th.th_flags = htonl(chfaflags);
	th.th_hsize = htonl(hsize);
	th.th_ssize = htonl(hsize +
			    flex_table_size(accept.begin(), accept.end()) +
			    flex_table_size(accept2.begin(), accept2.end()) +
			    (eq.size() ? flex_table_size(equiv_vec.begin(), equiv_vec.end()) : 0) +
			    flex_table_size(base_vec.begin(), base_vec.end()) +
			    flex_table_size(default_vec.begin(), default_vec.end()) +
			    flex_table_size(next_vec.begin(), next_vec.end()) +
			    flex_table_size(check_vec.begin(), check_vec.end()));
	os.write((char *)&th, sizeof(th));
	os << th_version << (char)0 << name << (char)0;
	os << fill64(sizeof(th) + sizeof(th_version) + strlen(name) + 1);

	write_flex_table(os, YYTD_ID_ACCEPT, accept.begin(), accept.end());
	write_flex_table(os, YYTD_ID_ACCEPT2, accept2.begin(), accept2.end());
	if (eq.size())
		write_flex_table(os, YYTD_ID_EC, equiv_vec.begin(),
				 equiv_vec.end());
	write_flex_table(os, YYTD_ID_BASE, base_vec.begin(), base_vec.end());
	write_flex_table(os, YYTD_ID_DEF, default_vec.begin(), default_vec.end());
	write_flex_table(os, YYTD_ID_NXT, next_vec.begin(), next_vec.end());
	write_flex_table(os, YYTD_ID_CHK, check_vec.begin(), check_vec.end());
}
