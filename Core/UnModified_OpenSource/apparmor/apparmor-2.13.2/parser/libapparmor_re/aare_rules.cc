/*
 * (C) 2006, 2007 Andreas Gruenbacher <agruen@suse.de>
 * Copyright (c) 2003-2008 Novell, Inc. (All rights reserved)
 * Copyright 2009-2013 Canonical Ltd. (All rights reserved)
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

#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ext/stdio_filebuf.h>
#include <assert.h>
#include <stdlib.h>

#include "aare_rules.h"
#include "expr-tree.h"
#include "parse.h"
#include "hfa.h"
#include "chfa.h"
#include "../immunix.h"


aare_rules::~aare_rules(void)
{
	if (root)
		root->release();

	unique_perms.clear();
	expr_map.clear();
}

bool aare_rules::add_rule(const char *rule, int deny, uint32_t perms,
			  uint32_t audit, dfaflags_t flags)
{
	return add_rule_vec(deny, perms, audit, 1, &rule, flags);
}

void aare_rules::add_to_rules(Node *tree, Node *perms)
{
	if (reverse)
		flip_tree(tree);
	Node *base = expr_map[perms];
	if (base)
		expr_map[perms] = new AltNode(base, tree);
	else
		expr_map[perms] = tree;
}

static Node *cat_with_null_seperator(Node *l, Node *r)
{
	return new CatNode(new CatNode(l, new CharNode(0)), r);
}

bool aare_rules::add_rule_vec(int deny, uint32_t perms, uint32_t audit,
			      int count, const char **rulev, dfaflags_t flags)
{
	Node *tree = NULL, *accept;
	int exact_match;

	if (regex_parse(&tree, rulev[0]))
		return false;
	for (int i = 1; i < count; i++) {
		Node *subtree = NULL;
		if (regex_parse(&subtree, rulev[i]))
			return false;
		tree = cat_with_null_seperator(tree, subtree);
	}

	/*
	 * Check if we have an expression with or without wildcards. This
	 * determines how exec modifiers are merged in accept_perms() based
	 * on how we split permission bitmasks here.
	 */
	exact_match = 1;
	for (depth_first_traversal i(tree); i && exact_match; i++) {
		if (dynamic_cast<StarNode *>(*i) ||
		    dynamic_cast<PlusNode *>(*i) ||
		    dynamic_cast<AnyCharNode *>(*i) ||
		    dynamic_cast<CharSetNode *>(*i) ||
		    dynamic_cast<NotCharSetNode *>(*i))
			exact_match = 0;
	}

	if (reverse)
		flip_tree(tree);

	accept = unique_perms.insert(deny, perms, audit, exact_match);

	if (flags & DFA_DUMP_RULE_EXPR) {
		cerr << "rule: ";
		cerr << rulev[0];
		for (int i = 1; i < count; i++) {
			cerr << "\\x00";
			cerr << rulev[i];
		}
		cerr << "  ->  ";
		tree->dump(cerr);
		if (deny)
			cerr << " deny";
		cerr << " (0x" << hex << perms <<"/" << audit << dec << ")";
		accept->dump(cerr);
 		cerr << "\n\n";
	}

	add_to_rules(tree, accept);

	rule_count++;

	return true;
}

/* create a dfa from the ruleset
 * returns: buffer contain dfa tables, @size set to the size of the tables
 *          else NULL on failure
 */
void *aare_rules::create_dfa(size_t *size, dfaflags_t flags)
{
	char *buffer = NULL;

	/* finish constructing the expr tree from the different permission
	 * set nodes */
	PermExprMap::iterator i = expr_map.begin();
	if (i != expr_map.end()) {
		if (flags & DFA_CONTROL_TREE_SIMPLE) {
			Node *tmp = simplify_tree(i->second, flags);
			root = new CatNode(tmp, i->first);
		} else
			root = new CatNode(i->second, i->first);
		for (i++; i != expr_map.end(); i++) {
			Node *tmp;
			if (flags & DFA_CONTROL_TREE_SIMPLE) {
				tmp = simplify_tree(i->second, flags);
			} else
				tmp = i->second;
			root = new AltNode(root, new CatNode(tmp, i->first));
		}
	}

	/* dumping of the none simplified tree without -O no-expr-simplify
	 * is broken because we need to build the tree above first, and
	 * simplification is woven into the build. Reevaluate how to fix
	 * this debug dump.
	 */
	label_nodes(root);
	if (flags & DFA_DUMP_TREE) {
		cerr << "\nDFA: Expression Tree\n";
		root->dump(cerr);
		cerr << "\n\n";
	}

	if (flags & DFA_CONTROL_TREE_SIMPLE) {
		/* This is old total tree, simplification point
		 * For now just do simplification up front. It gets most
		 * of the benefit running on the smaller chains, and is
		 * overall faster because there are less nodes. Reevaluate
		 * once tree simplification is rewritten
		 */
		//root = simplify_tree(root, flags);

		if (flags & DFA_DUMP_SIMPLE_TREE) {
			cerr << "\nDFA: Simplified Expression Tree\n";
			root->dump(cerr);
			cerr << "\n\n";
		}
	}

	stringstream stream;
	try {
		DFA dfa(root, flags);
		if (flags & DFA_DUMP_UNIQ_PERMS)
			dfa.dump_uniq_perms("dfa");

		if (flags & DFA_CONTROL_MINIMIZE) {
			dfa.minimize(flags);

			if (flags & DFA_DUMP_MIN_UNIQ_PERMS)
				dfa.dump_uniq_perms("minimized dfa");
		}

		if (flags & DFA_CONTROL_FILTER_DENY &&
		    flags & DFA_CONTROL_MINIMIZE &&
		    dfa.apply_and_clear_deny()) {
			/* Do a second minimization pass as removal of deny
			 * information has moved some states from accepting
			 * to none accepting partitions
			 *
			 * TODO: add this as a tail pass to minimization
			 *       so we don't need to do a full second pass
			 */
			dfa.minimize(flags);

			if (flags & DFA_DUMP_MIN_UNIQ_PERMS)
				dfa.dump_uniq_perms("minimized dfa");
		}

		if (flags & DFA_CONTROL_REMOVE_UNREACHABLE)
			dfa.remove_unreachable(flags);

		if (flags & DFA_DUMP_STATES)
			dfa.dump(cerr);

		if (flags & DFA_DUMP_GRAPH)
			dfa.dump_dot_graph(cerr);

		map<uchar, uchar> eq;
		if (flags & DFA_CONTROL_EQUIV) {
			eq = dfa.equivalence_classes(flags);
			dfa.apply_equivalence_classes(eq);

			if (flags & DFA_DUMP_EQUIV) {
				cerr << "\nDFA equivalence class\n";
				dump_equivalence_classes(cerr, eq);
			}
		} else if (flags & DFA_DUMP_EQUIV)
			cerr << "\nDFA did not generate an equivalence class\n";

		if (flags & DFA_CONTROL_DIFF_ENCODE) {
			dfa.diff_encode(flags);

			if (flags & DFA_DUMP_DIFF_ENCODE)
				dfa.dump_diff_encode(cerr);
		}

		CHFA chfa(dfa, eq, flags);
		if (flags & DFA_DUMP_TRANS_TABLE)
			chfa.dump(cerr);
		chfa.flex_table(stream, "");
	}
	catch(int error) {
		*size = 0;
		return NULL;
	}

	stringbuf *buf = stream.rdbuf();

	buf->pubseekpos(0);
	*size = buf->in_avail();

	buffer = (char *)malloc(*size);
	if (!buffer)
		return NULL;
	buf->sgetn(buffer, *size);
	return buffer;
}
