/*
 * (C) 2006, 2007 Andreas Gruenbacher <agruen@suse.de>
 * Copyright (c) 2003-2008 Novell, Inc. (All rights reserved)
 * Copyright 2009-2013 Canonical Ltd.
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
 * Functions to create/manipulate an expression tree for regular expressions
 * that have been parsed.
 *
 * The expression tree can be used directly after the parse creates it, or
 * it can be factored so that the set of important nodes is smaller.
 * Having a reduced set of important nodes generally results in a dfa that
 * is closer to minimum (fewer redundant states are created).  It also
 * results in fewer important nodes in a the state set during subset
 * construction resulting in less memory used to create a dfa.
 *
 * Generally it is worth doing expression tree simplification before dfa
 * construction, if the regular expression tree contains any alternations.
 * Even if the regular expression doesn't simplification should be fast
 * enough that it can be used with minimal overhead.
 */
#ifndef __LIBAA_RE_EXPR_H
#define __LIBAA_RE_EXPR_H

#include <map>
#include <set>
#include <stack>
#include <ostream>

#include <stdint.h>

#include "apparmor_re.h"

using namespace std;

typedef unsigned char uchar;
typedef set<uchar> Chars;

ostream &operator<<(ostream &os, uchar c);

/* Compute the union of two sets. */
template<class T> set<T> operator+(const set<T> &a, const set<T> &b)
{
	set<T> c(a);
	c.insert(b.begin(), b.end());
	return c;
}

/**
 * When creating DFAs from regex trees, a DFA state is constructed from
 * a set of important nodes in the syntax tree. This includes AcceptNodes,
 * which indicate that when a match ends in a particular state, the
 * regular expressions that the AcceptNode belongs to match.
 */
class Node;
class ImportantNode;
typedef set<ImportantNode *> NodeSet;

/**
 * Text-dump a state (for debugging).
 */
ostream &operator<<(ostream &os, const NodeSet &state);

/**
 * Out-edges from a state to another: we store the follow-set of Nodes
 * for each input character that is not a default match in
 * cases (i.e., following a CharNode or CharSetNode), and default
 * matches in otherwise as well as in all matching explicit cases
 * (i.e., following an AnyCharNode or NotCharSetNode). This avoids
 * enumerating all the explicit tranitions for default matches.
 */
typedef struct Cases {
	typedef map<uchar, NodeSet *>::iterator iterator;
	iterator begin() { return cases.begin(); }
	iterator end() { return cases.end(); }

	Cases(): otherwise(0) { }
	map<uchar, NodeSet *> cases;
	NodeSet *otherwise;
} Cases;

ostream &operator<<(ostream &os, Node &node);

/* An abstract node in the syntax tree. */
class Node {
public:
	Node(): nullable(false), label(0) { child[0] = child[1] = 0; }
	Node(Node *left): nullable(false), label(0)
	{
		child[0] = left;
		child[1] = 0;
	}
	Node(Node *left, Node *right): nullable(false), label(0)
	{
		child[0] = left;
		child[1] = right;
	}
	virtual ~Node()
	{
		if (child[0])
			child[0]->release();
		if (child[1])
			child[1]->release();
	}

	/**
	 * See the "Dragon Book" for an explanation of nullable, firstpos,
	 * lastpos, and followpos.
	 */
	virtual void compute_nullable() { }
	virtual void compute_firstpos() = 0;
	virtual void compute_lastpos() = 0;
	virtual void compute_followpos() { }
	virtual int eq(Node *other) = 0;
	virtual ostream &dump(ostream &os) = 0;
	void dump_syntax_tree(ostream &os);
	virtual void normalize(int dir)
	{
		if (child[dir])
			child[dir]->normalize(dir);
		if (child[!dir])
			child[!dir]->normalize(dir);
	}
	/* return false if no work done */
	virtual int normalize_eps(int dir __attribute__((unused))) { return 0; }

	bool nullable;
	NodeSet firstpos, lastpos, followpos;
	/* child 0 is left, child 1 is right */
	Node *child[2];

	unsigned int label;	/* unique number for debug etc */
	/**
	 * We indirectly release Nodes through a virtual function because
	 * accept and Eps Nodes are shared, and must be treated specially.
	 * We could use full reference counting here but the indirect release
	 * is sufficient and has less overhead
	 */
	virtual void release(void) { delete this; }
};

class InnerNode: public Node {
public:
	InnerNode(): Node() { };
	InnerNode(Node *left): Node(left) { };
	InnerNode(Node *left, Node *right): Node(left, right) { };
};

class OneChildNode: public InnerNode {
public:
	OneChildNode(Node *left): InnerNode(left) { };
};

class TwoChildNode: public InnerNode {
public:
	TwoChildNode(Node *left, Node *right): InnerNode(left, right) { };
	virtual int normalize_eps(int dir);
};

class LeafNode: public Node {
public:
	LeafNode(): Node() { };
	virtual void normalize(int dir __attribute__((unused))) { return; }
};

/* Match nothing (//). */
class EpsNode: public LeafNode {
public:
	EpsNode(): LeafNode()
	{
		nullable = true;
		label = 0;
	}
	void release(void)
	{
		/* don't delete Eps nodes because there is a single static
		 * instance shared by all trees.  Look for epsnode in the code
		 */
	}

	void compute_firstpos() { }
	void compute_lastpos() { }
	int eq(Node *other)
	{
		if (dynamic_cast<EpsNode *>(other))
			return 1;
		return 0;
	}
	ostream &dump(ostream &os)
	{
		return os << "[]";
	}
};

/**
 * Leaf nodes in the syntax tree are important to us: they describe the
 * characters that the regular expression matches. We also consider
 * AcceptNodes import: they indicate when a regular expression matches.
 */
class ImportantNode: public LeafNode {
public:
	ImportantNode(): LeafNode() { }
	void compute_firstpos() { firstpos.insert(this); }
	void compute_lastpos() { lastpos.insert(this); }
	virtual void follow(Cases &cases) = 0;
	virtual int is_accept(void) = 0;
	virtual int is_postprocess(void) = 0;
};

/* common base class for all the different classes that contain
 * character information.
 */
class CNode: public ImportantNode {
public:
	CNode(): ImportantNode() { }
	int is_accept(void) { return false; }
	int is_postprocess(void) { return false; }
};

/* Match one specific character (/c/). */
class CharNode: public CNode {
public:
	CharNode(uchar c): c(c) { }
	void follow(Cases &cases)
	{
		NodeSet **x = &cases.cases[c];
		if (!*x) {
			if (cases.otherwise)
				*x = new NodeSet(*cases.otherwise);
			else
				*x = new NodeSet;
		}
		(*x)->insert(followpos.begin(), followpos.end());
	}
	int eq(Node *other)
	{
		CharNode *o = dynamic_cast<CharNode *>(other);
		if (o) {
			return c == o->c;
		}
		return 0;
	}
	ostream &dump(ostream &os)
	{
		return os << c;
	}

	uchar c;
};

/* Match a set of characters (/[abc]/). */
class CharSetNode: public CNode {
public:
	CharSetNode(Chars &chars): chars(chars) { }
	void follow(Cases &cases)
	{
		for (Chars::iterator i = chars.begin(); i != chars.end(); i++) {
			NodeSet **x = &cases.cases[*i];
			if (!*x) {
				if (cases.otherwise)
					*x = new NodeSet(*cases.otherwise);
				else
					*x = new NodeSet;
			}
			(*x)->insert(followpos.begin(), followpos.end());
		}
	}
	int eq(Node *other)
	{
		CharSetNode *o = dynamic_cast<CharSetNode *>(other);
		if (!o || chars.size() != o->chars.size())
			return 0;

		for (Chars::iterator i = chars.begin(), j = o->chars.begin();
		     i != chars.end() && j != o->chars.end(); i++, j++) {
			if (*i != *j)
				return 0;
		}
		return 1;
	}
	ostream &dump(ostream &os)
	{
		os << '[';
		for (Chars::iterator i = chars.begin(); i != chars.end(); i++)
			os << *i;
		return os << ']';
	}

	Chars chars;
};

/* Match all except one character (/[^abc]/). */
class NotCharSetNode: public CNode {
public:
	NotCharSetNode(Chars &chars): chars(chars) { }
	void follow(Cases &cases)
	{
		if (!cases.otherwise)
			cases.otherwise = new NodeSet;
		for (Chars::iterator j = chars.begin(); j != chars.end(); j++) {
			NodeSet **x = &cases.cases[*j];
			if (!*x)
				*x = new NodeSet(*cases.otherwise);
		}
		/* Note: Add to the nonmatching characters after copying away
		 * the old otherwise state for the matching characters.
		 */
		cases.otherwise->insert(followpos.begin(), followpos.end());
		for (Cases::iterator i = cases.begin(); i != cases.end();
		     i++) {
			if (chars.find(i->first) == chars.end())
				i->second->insert(followpos.begin(),
						  followpos.end());
		}
	}
	int eq(Node *other)
	{
		NotCharSetNode *o = dynamic_cast<NotCharSetNode *>(other);
		if (!o || chars.size() != o->chars.size())
			return 0;

		for (Chars::iterator i = chars.begin(), j = o->chars.begin();
		     i != chars.end() && j != o->chars.end(); i++, j++) {
			if (*i != *j)
				return 0;
		}
		return 1;
	}
	ostream &dump(ostream &os)
	{
		os << "[^";
		for (Chars::iterator i = chars.begin(); i != chars.end(); i++)
			os << *i;
		return os << ']';
	}

	Chars chars;
};

/* Match any character (/./). */
class AnyCharNode: public CNode {
public:
	AnyCharNode() { }
	void follow(Cases &cases)
	{
		if (!cases.otherwise)
			cases.otherwise = new NodeSet;
		cases.otherwise->insert(followpos.begin(), followpos.end());
		for (Cases::iterator i = cases.begin(); i != cases.end();
		     i++)
			i->second->insert(followpos.begin(), followpos.end());
	}
	int eq(Node *other)
	{
		if (dynamic_cast<AnyCharNode *>(other))
			return 1;
		return 0;
	}
	ostream &dump(ostream &os) { return os << "."; }
};

/* Match a node zero or more times. (This is a unary operator.) */
class StarNode: public OneChildNode {
public:
	StarNode(Node *left): OneChildNode(left) { nullable = true; }
	void compute_firstpos() { firstpos = child[0]->firstpos; }
	void compute_lastpos() { lastpos = child[0]->lastpos; }
	void compute_followpos()
	{
		NodeSet from = child[0]->lastpos, to = child[0]->firstpos;
		for (NodeSet::iterator i = from.begin(); i != from.end(); i++) {
			(*i)->followpos.insert(to.begin(), to.end());
		}
	}
	int eq(Node *other)
	{
		if (dynamic_cast<StarNode *>(other))
			return child[0]->eq(other->child[0]);
		return 0;
	}
	ostream &dump(ostream &os)
	{
		os << '(';
		child[0]->dump(os);
		return os << ")*";
	}
};

/* Match a node one or more times. (This is a unary operator.) */
class PlusNode: public OneChildNode {
public:
	PlusNode(Node *left): OneChildNode(left) {
	}
	void compute_nullable() { nullable = child[0]->nullable; }
	void compute_firstpos() { firstpos = child[0]->firstpos; }
	void compute_lastpos() { lastpos = child[0]->lastpos; }
	void compute_followpos()
	{
		NodeSet from = child[0]->lastpos, to = child[0]->firstpos;
		for (NodeSet::iterator i = from.begin(); i != from.end(); i++) {
			(*i)->followpos.insert(to.begin(), to.end());
		}
	}
	int eq(Node *other) {
		if (dynamic_cast<PlusNode *>(other))
			return child[0]->eq(other->child[0]);
		return 0;
	}
	ostream &dump(ostream &os) {
		os << '(';
		child[0]->dump(os);
		return os << ")+";
	}
};

/* Match a pair of consecutive nodes. */
class CatNode: public TwoChildNode {
public:
	CatNode(Node *left, Node *right): TwoChildNode(left, right) { }
	void compute_nullable()
	{
		nullable = child[0]->nullable && child[1]->nullable;
	}
	void compute_firstpos()
	{
		if (child[0]->nullable)
			firstpos = child[0]->firstpos + child[1]->firstpos;
		else
			firstpos = child[0]->firstpos;
	}
	void compute_lastpos()
	{
		if (child[1]->nullable)
			lastpos = child[0]->lastpos + child[1]->lastpos;
		else
			lastpos = child[1]->lastpos;
	}
	void compute_followpos()
	{
		NodeSet from = child[0]->lastpos, to = child[1]->firstpos;
		for (NodeSet::iterator i = from.begin(); i != from.end(); i++) {
			(*i)->followpos.insert(to.begin(), to.end());
		}
	}
	int eq(Node *other)
	{
		if (dynamic_cast<CatNode *>(other)) {
			if (!child[0]->eq(other->child[0]))
				return 0;
			return child[1]->eq(other->child[1]);
		}
		return 0;
	}
	ostream &dump(ostream &os)
	{
		child[0]->dump(os);
		child[1]->dump(os);
		return os;
	}
	void normalize(int dir);
};

/* Match one of two alternative nodes. */
class AltNode: public TwoChildNode {
public:
	AltNode(Node *left, Node *right): TwoChildNode(left, right) { }
	void compute_nullable()
	{
		nullable = child[0]->nullable || child[1]->nullable;
	}
	void compute_lastpos()
	{
		lastpos = child[0]->lastpos + child[1]->lastpos;
	}
	void compute_firstpos()
	{
		firstpos = child[0]->firstpos + child[1]->firstpos;
	}
	int eq(Node *other)
	{
		if (dynamic_cast<AltNode *>(other)) {
			if (!child[0]->eq(other->child[0]))
				return 0;
			return child[1]->eq(other->child[1]);
		}
		return 0;
	}
	ostream &dump(ostream &os)
	{
		os << '(';
		child[0]->dump(os);
		os << '|';
		child[1]->dump(os);
		os << ')';
		return os;
	}
	void normalize(int dir);
};

class SharedNode: public ImportantNode {
public:
	SharedNode() { }
	void release(void)
	{
		/* don't delete SharedNodes via release as they are shared, and
		 * will be deleted when the table they are stored in is deleted
		 */
	}

	void follow(Cases &cases __attribute__ ((unused)))
	{
		/* Nothing to follow. */
	}

	/* requires shared nodes to be common by pointer */
	int eq(Node *other) { return (this == other); }
};

/**
 * Indicate that a regular expression matches. An AcceptNode itself
 * doesn't match anything, so it will never generate any transitions.
 */
class AcceptNode: public SharedNode {
public:
	AcceptNode() { }
	int is_accept(void) { return true; }
	int is_postprocess(void) { return false; }
};

class MatchFlag: public AcceptNode {
public:
	MatchFlag(uint32_t flag, uint32_t audit): flag(flag), audit(audit) { }
	ostream &dump(ostream &os) { return os << "< 0x" << hex << flag << '>'; }

	uint32_t flag;
	uint32_t audit;
};

class ExactMatchFlag: public MatchFlag {
public:
	ExactMatchFlag(uint32_t flag, uint32_t audit): MatchFlag(flag, audit) {}
};

class DenyMatchFlag: public MatchFlag {
public:
	DenyMatchFlag(uint32_t flag, uint32_t quiet): MatchFlag(flag, quiet) {}
};

/* Traverse the syntax tree depth-first in an iterator-like manner. */
class depth_first_traversal {
	stack<Node *>pos;
	void push_left(Node *node) {
		pos.push(node);

		while (dynamic_cast<InnerNode *>(node)) {
			pos.push(node->child[0]);
			node = node->child[0];
		}
	}
public:
	depth_first_traversal(Node *node) { push_left(node); }
	Node *operator*() { return pos.top(); }
	Node *operator->() { return pos.top(); }
	operator  bool() { return !pos.empty(); }
	void operator++(int)
	{
		Node *last = pos.top();
		pos.pop();

		if (!pos.empty()) {
			/* no need to dynamic cast, as we just popped a node so
			 * the top node must be an inner node */
			InnerNode *node = (InnerNode *) (pos.top());
			if (node->child[1] && node->child[1] != last) {
				push_left(node->child[1]);
			}
		}
	}
};

struct node_counts {
	int charnode;
	int charset;
	int notcharset;
	int alt;
	int plus;
	int star;
	int any;
	int cat;
};

extern EpsNode epsnode;

int debug_tree(Node *t);
Node *simplify_tree(Node *t, dfaflags_t flags);
void label_nodes(Node *root);
unsigned long hash_NodeSet(NodeSet *ns);
void flip_tree(Node *node);



/*
 * hashedNodes - for efficient set comparison
 */
class hashedNodeSet {
public:
	unsigned long hash;
	NodeSet *nodes;

	hashedNodeSet(NodeSet *n): nodes(n)
	{
		hash = hash_NodeSet(n);
	}

	bool operator<(hashedNodeSet const &rhs)const
	{
		if (hash == rhs.hash) {
			if (nodes->size() == rhs.nodes->size())
				return *nodes < *(rhs.nodes);
			else
				return nodes->size() < rhs.nodes->size();
		} else {
			return hash < rhs.hash;
		}
	}
};


class hashedNodeVec {
public:
	typedef ImportantNode ** iterator;
	iterator begin() { return nodes; }
	iterator end() { iterator t = nodes ? &nodes[len] : NULL; return t; }

	unsigned long hash;
	unsigned long len;
	ImportantNode **nodes;

	hashedNodeVec(NodeSet *n)
	{
		hash = hash_NodeSet(n);
		len = n->size();
		nodes = new ImportantNode *[n->size()];

		unsigned int j = 0;
		for (NodeSet::iterator i = n->begin(); i != n->end(); i++, j++) {
			nodes[j] = *i;
		}
	}

	hashedNodeVec(NodeSet *n, unsigned long h): hash(h)
	{
		len = n->size();
		nodes = new ImportantNode *[n->size()];
		ImportantNode **j = nodes;
		for (NodeSet::iterator i = n->begin(); i != n->end(); i++) {
			*(j++) = *i;
		}
	}

	~hashedNodeVec()
	{
		delete [] nodes;
	}

	unsigned long size()const { return len; }

	bool operator<(hashedNodeVec const &rhs)const
	{
		if (hash == rhs.hash) {
			if (len == rhs.size()) {
				for (unsigned int i = 0; i < len; i++) {
					if (nodes[i] != rhs.nodes[i])
						return nodes[i] < rhs.nodes[i];
				}
				return false;
			}
			return len < rhs.size();
		}
		return hash < rhs.hash;
	}
};

class CacheStats {
public:
	unsigned long dup, sum, max;

	CacheStats(void): dup(0), sum(0), max(0) { };

	void clear(void) { dup = sum = max = 0; }
	virtual unsigned long size(void) const = 0;
};

class NodeCache: public CacheStats {
public:
	set<hashedNodeSet> cache;

	NodeCache(void): cache() { };
	~NodeCache() { clear(); };

	virtual unsigned long size(void) const { return cache.size(); }

	void clear()
	{
		for (set<hashedNodeSet>::iterator i = cache.begin();
		     i != cache.end(); i++) {
			delete i->nodes;
		}
		cache.clear();
		CacheStats::clear();
	}

	NodeSet *insert(NodeSet *nodes)
	{
		if (!nodes)
			return NULL;
		pair<set<hashedNodeSet>::iterator,bool> uniq;
		uniq = cache.insert(hashedNodeSet(nodes));
		if (uniq.second == false) {
			delete(nodes);
			dup++;
		} else {
			sum += nodes->size();
			if (nodes->size() > max)
				max = nodes->size();
		}
		return uniq.first->nodes;
	}
};

struct deref_less_than {
       bool operator()(hashedNodeVec * const &lhs, hashedNodeVec * const &rhs)const
		{
			return *lhs < *rhs;
		}
};

class NodeVecCache: public CacheStats {
public:
	set<hashedNodeVec *, deref_less_than> cache;

	NodeVecCache(void): cache() { };
	~NodeVecCache() { clear(); };

	virtual unsigned long size(void) const { return cache.size(); }

	void clear()
	{
		for (set<hashedNodeVec *>::iterator i = cache.begin();
		     i != cache.end(); i++) {
			delete *i;
		}
		cache.clear();
		CacheStats::clear();
	}

	hashedNodeVec *insert(NodeSet *nodes)
	{
		if (!nodes)
			return NULL;
		pair<set<hashedNodeVec *>::iterator,bool> uniq;
		hashedNodeVec *nv = new hashedNodeVec(nodes);
		uniq = cache.insert(nv);
		if (uniq.second == false) {
			delete nv;
			dup++;
		} else {
			sum += nodes->size();
			if (nodes->size() > max)
				max = nodes->size();
		}
		delete(nodes);
		return (*uniq.first);
	}
};

#endif /* __LIBAA_RE_EXPR */
