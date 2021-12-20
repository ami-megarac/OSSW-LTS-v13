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
  */

#ifndef APPARMOR_RE_H
#define APPARMOR_RE_H

typedef int dfaflags_t;


#define DFA_CONTROL_EQUIV 		(1 << 0)
#define DFA_CONTROL_TREE_NORMAL 	(1 << 1)
#define DFA_CONTROL_TREE_SIMPLE 	(1 << 2)
#define DFA_CONTROL_TREE_LEFT 		(1 << 3)
#define DFA_CONTROL_MINIMIZE 		(1 << 4)
#define DFA_CONTROL_FILTER_DENY 	(1 << 6)
#define DFA_CONTROL_REMOVE_UNREACHABLE  (1 << 7)
#define DFA_CONTROL_TRANS_HIGH		(1 << 8)
#define DFA_CONTROL_DIFF_ENCODE		(1 << 9)

#define DFA_DUMP_DIFF_PROGRESS		(1 << 10)
#define DFA_DUMP_DIFF_ENCODE		(1 << 11)
#define DFA_DUMP_DIFF_STATS		(1 << 12)
#define DFA_DUMP_MIN_PARTS 		(1 << 13)
#define DFA_DUMP_UNIQ_PERMS 		(1 << 14)
#define DFA_DUMP_MIN_UNIQ_PERMS 	(1 << 15)
#define DFA_DUMP_TREE_STATS 		(1 << 16)
#define DFA_DUMP_TREE 			(1 << 17)
#define DFA_DUMP_SIMPLE_TREE 		(1 << 18)
#define DFA_DUMP_PROGRESS 		(1 << 19)
#define DFA_DUMP_STATS			(1 << 20)
#define DFA_DUMP_STATES 		(1 << 21)
#define DFA_DUMP_GRAPH			(1 << 22)
#define DFA_DUMP_TRANS_PROGRESS 	(1 << 23)
#define DFA_DUMP_TRANS_STATS 		(1 << 24)
#define DFA_DUMP_TRANS_TABLE 		(1 << 25)
#define DFA_DUMP_EQUIV			(1 << 26)
#define DFA_DUMP_EQUIV_STATS 		(1 << 27)
#define DFA_DUMP_MINIMIZE 		(1 << 28)
#define DFA_DUMP_UNREACHABLE 		(1 << 29)
#define DFA_DUMP_RULE_EXPR 		(1 << 30)
#define DFA_DUMP_NODE_TO_DFA 		(1 << 31)

#endif /* APPARMOR_RE_H */
