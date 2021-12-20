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
 * Create a compressed hfa (chfa) from and hfa
 */
#ifndef __LIBAA_RE_CHFA_H
#define __LIBAA_RE_CHFA_H

#include <map>
#include <vector>

#include "hfa.h"

#define BASE32_FLAGS 0xff000000
#define DiffEncodeBit32 0x80000000
#define base_mask_size(X) ((X) & ~BASE32_FLAGS)

using namespace std;

class CHFA {
	typedef vector<pair<const State *, size_t> > DefaultBase;
	typedef vector<pair<const State *, const State *> > NextCheck;
      public:
	CHFA(DFA &dfa, map<uchar, uchar> &eq, dfaflags_t flags);
	void dump(ostream & os);
	void flex_table(ostream &os, const char *name);
	void init_free_list(vector<pair<size_t, size_t> > &free_list,
			    size_t prev, size_t start);
	bool fits_in(vector<pair<size_t, size_t> > &free_list, size_t base,
		     StateTrans &cases);
	void insert_state(vector<pair<size_t, size_t> > &free_list,
			  State *state, DFA &dfa);

      private:
	vector<uint32_t> accept;
	vector<uint32_t> accept2;
	DefaultBase default_base;
	NextCheck next_check;
	map<const State *, size_t> num;
	map<uchar, uchar> &eq;
	uchar max_eq;
	size_t first_free;
	unsigned int chfaflags;
};

#endif /* __LIBAA_RE_CHFA_H */
