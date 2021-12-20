/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
 *
 *   Copyright (c) 2010 - 2014
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
 *   along with this program; if not, contact Novell, Inc. or Canonical,
 *   Ltd.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_optarg.h"
#include "parser.h"

optflag_table_t dumpflag_table[] = {
	{ 1, "rule-exprs", "Dump rule to expr tree conversions",
	  DFA_DUMP_RULE_EXPR },
	{ 1, "expr-stats", "Dump stats on expr tree", DFA_DUMP_TREE_STATS },
	{ 1, "expr-tree", "Dump expression tree", DFA_DUMP_TREE },
	{ 1, "expr-simplified", "Dump simplified expression tree",
	  DFA_DUMP_SIMPLE_TREE },
	{ 1, "stats", "Dump all compile stats",
	  DFA_DUMP_TREE_STATS | DFA_DUMP_STATS | DFA_DUMP_TRANS_STATS |
	  DFA_DUMP_EQUIV_STATS | DFA_DUMP_DIFF_STATS },
	{ 1, "progress", "Dump progress for all compile phases",
	  DFA_DUMP_PROGRESS | DFA_DUMP_STATS | DFA_DUMP_TRANS_PROGRESS |
	  DFA_DUMP_TRANS_STATS | DFA_DUMP_DIFF_PROGRESS | DFA_DUMP_DIFF_STATS },
	{ 1, "dfa-progress", "Dump dfa creation as in progress",
	  DFA_DUMP_PROGRESS | DFA_DUMP_STATS },
	{ 1, "dfa-stats", "Dump dfa creation stats", DFA_DUMP_STATS },
	{ 1, "dfa-states", "Dump dfa state diagram", DFA_DUMP_STATES },
	{ 1, "dfa-graph", "Dump dfa dot (graphviz) graph", DFA_DUMP_GRAPH },
	{ 1, "dfa-minimize", "Dump dfa minimization", DFA_DUMP_MINIMIZE },
	{ 1, "dfa-unreachable", "Dump dfa unreachable states",
	  DFA_DUMP_UNREACHABLE },
	{ 1, "dfa-node-map", "Dump expr node set to state mapping",
	  DFA_DUMP_NODE_TO_DFA },
	{ 1, "dfa-uniq-perms", "Dump unique perms",
	  DFA_DUMP_UNIQ_PERMS },
	{ 1, "dfa-minimize-uniq-perms", "Dump unique perms post minimization",
	  DFA_DUMP_MIN_UNIQ_PERMS },
	{ 1, "dfa-minimize-partitions", "Dump dfa minimization partitions",
	  DFA_DUMP_MIN_PARTS },
	{ 1, "compress-progress", "Dump progress of compression",
	  DFA_DUMP_TRANS_PROGRESS | DFA_DUMP_TRANS_STATS },
	{ 1, "compress-stats", "Dump stats on compression",
	  DFA_DUMP_TRANS_STATS },
	{ 1, "compressed-dfa", "Dump compressed dfa", DFA_DUMP_TRANS_TABLE },
	{ 1, "equiv-stats", "Dump equivalence class stats",
	  DFA_DUMP_EQUIV_STATS },
	{ 1, "equiv", "Dump equivalence class", DFA_DUMP_EQUIV },
	{ 1, "diff-encode", "Dump differential encoding",
	  DFA_DUMP_DIFF_ENCODE },
	{ 1, "diff-stats", "Dump differential encoding stats",
	  DFA_DUMP_DIFF_STATS },
	{ 1, "diff-progress", "Dump progress of differential encoding",
	  DFA_DUMP_DIFF_PROGRESS | DFA_DUMP_DIFF_STATS },
	{ 0, NULL, NULL, 0 },
};

optflag_table_t optflag_table[] = {
	{ 2, "0", "no optimizations",
	  DFA_CONTROL_TREE_NORMAL | DFA_CONTROL_TREE_SIMPLE |
	  DFA_CONTROL_MINIMIZE | DFA_CONTROL_REMOVE_UNREACHABLE |
	  DFA_CONTROL_DIFF_ENCODE
	},
	{ 1, "equiv", "use equivalent classes", DFA_CONTROL_EQUIV },
	{ 1, "expr-normalize", "expression tree normalization",
	  DFA_CONTROL_TREE_NORMAL },
	{ 1, "expr-simplify", "expression tree simplification",
	  DFA_CONTROL_TREE_SIMPLE },
	{ 0, "expr-left-simplify", "left simplification first",
	  DFA_CONTROL_TREE_LEFT },
	{ 2, "expr-right-simplify", "right simplification first",
	  DFA_CONTROL_TREE_LEFT },
	{ 1, "minimize", "dfa state minimization", DFA_CONTROL_MINIMIZE },
	{ 1, "filter-deny", "filter out deny information from final dfa",
	  DFA_CONTROL_FILTER_DENY },
	{ 1, "remove-unreachable", "dfa unreachable state removal",
	  DFA_CONTROL_REMOVE_UNREACHABLE },
	{ 0, "compress-small",
	  "do slower dfa transition table compression",
	  DFA_CONTROL_TRANS_HIGH },
	{ 2, "compress-fast", "do faster dfa transition table compression",
	  DFA_CONTROL_TRANS_HIGH },
	{ 1, "diff-encode", "Differentially encode transitions",
	  DFA_CONTROL_DIFF_ENCODE },
	{ 0, NULL, NULL, 0 },
};

void print_flag_table(optflag_table_t *table)
{
	int i;
	unsigned int longest = 0;
	for (i = 0; table[i].option; i++) {
		if (strlen(table[i].option) > longest)
			longest = strlen(table[i].option);
	}

	for (i = 0; table[i].option; i++) {
		printf("%5s%-*s \t%s\n", (table[i].control & 1) ? "[no-]" : "",
		       longest, table[i].option, table[i].desc);
	}
}

int handle_flag_table(optflag_table_t *table, const char *optarg,
		      dfaflags_t *flags)
{
	const char *arg = optarg;
	int i, invert = 0;

	if (strncmp(optarg, "no-", 3) == 0) {
		arg = optarg + 3;
		invert = 1;
	}

	for (i = 0; table[i].option; i++) {
		if (strcmp(table[i].option, arg) == 0) {
			/* check if leading no- was specified but is not
			 * supported by the option */
			if (invert && !(table[i].control & 1))
				return 0;
			if (table[i].control & 2)
				invert |= 1;
			if (invert)
				*flags &= ~table[i].flags;
			else
				*flags |= table[i].flags;
			return 1;
		}
	}
	return 0;
}

void display_dump(const char *command)
{
	display_version();
	printf("\n%s: --dump [Option]\n\n"
	       "Options:\n"
	       "--------\n"
	       "     variables      \tDump variables\n"
	       "     expanded-variables\t Dump variables after expansion\n"
	       ,command);
	print_flag_table(dumpflag_table);
}

void display_optimize(const char *command)
{
	display_version();
	printf("\n%s: -O [Option]\n\n"
	       "Options:\n"
	       "--------\n"
	       ,command);
	print_flag_table(optflag_table);
}
