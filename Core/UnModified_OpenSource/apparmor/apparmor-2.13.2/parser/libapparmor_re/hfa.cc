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
 * Base of implementation based on the Lexical Analysis chapter of:
 *   Alfred V. Aho, Ravi Sethi, Jeffrey D. Ullman:
 *   Compilers: Principles, Techniques, and Tools (The "Dragon Book"),
 *   Addison-Wesley, 1986.
 */

#include <list>
#include <vector>
#include <stack>
#include <set>
#include <map>
#include <ostream>
#include <iostream>
#include <fstream>
#include <string.h>

#include "expr-tree.h"
#include "hfa.h"
#include "../immunix.h"


ostream &operator<<(ostream &os, const CacheStats &cache)
{
	/* dump the state label */
	os << "cache: size=";
	os << cache.size();
	os << " dups=";
	os << cache.dup;
	os << " longest=";
	os << cache.max;
	if (cache.size()) {
		os << " avg=";
		os << cache.sum / cache.size();
	}
	return os;
}

ostream &operator<<(ostream &os, const ProtoState &proto)
{
	/* dump the state label */
	os << '{';
	os << proto.nnodes;
	os << ',';
	os << proto.anodes;
	os << '}';
	return os;
}

ostream &operator<<(ostream &os, const State &state)
{
	/* dump the state label */
	os << '{';
	os << state.label;
	os << '}';
	return os;
}

/**
 * diff_weight - Find differential compression distance between @rel and @this
 * @rel: State to compare too
 * Returns: An integer indicating how good rel is as a base, larger == better
 *
 * Find the relative weighted difference for differential state compression
 * with queried state being compressed against @rel
 *
 * +1 for each transition that matches (char and dest - saves a transition)
 * 0 for each transition that doesn't match and exists in both states
 * 0  for transition that self has and @other doesn't (no extra required)
 * -1 for each transition that is in @rel and not in @this (have to override)
 *
 * @rel should not be a state that has already been made differential or it may
 * introduce extra transitions as it does not recurse to find all transitions
 *
 * Should be applied after state minimization 
*/
int State::diff_weight(State *rel)
{
	int weight = 0;

	if (this == rel)
		return 0;

	if (rel->diff->rel) {
		/* Can only be diff encoded against states that are relative
		 * to a state of a lower depth. ie, at most one sibling in
		 * the chain
		 */
		if (rel->diff->rel->diff->depth >= this->diff->depth)
			return 0;
	} else if (rel->diff->depth >= this->diff->depth)
		return 0;

	if (rel->flags & DiffEncodeFlag) {
		for (int i = 0; i < 256; i++) {
			State *state = rel->next(i);
			StateTrans::iterator j = trans.find(i);
			if (j != trans.end()) {
				if (state == j->second)
					weight++;
				/* else
				   0 - keep transition to mask
				*/
			} else if (state == otherwise) {
				/* 0 - match of default against @rel
				 * We don't save a transition but don't have
				 * to mask either
				 */
			} else {
				/* @rel has transition not covered by @this.
				 * Need to add a transition to mask it
				 */
				weight--;
			}
		}
		return weight;
	}

	unsigned int count = 0;
	for (StateTrans::iterator i = rel->trans.begin(); i != rel->trans.end();
 i++) {
		StateTrans::iterator j = trans.find(i->first);
		if (j != trans.end()) {
			if (i->second == j->second)
				weight++;
			/* } else {
				0 - keep transition to mask
			*/
			count++;
		} else if (i->second == otherwise) {
			/* 0 - match of default against @rel
			 * We don't save a transition but don't have to
			 * mask either
			 */
		} else {
			/* rel has transition not covered by @this.  Need to
			 * add a transition to mask
			 */
			weight--;
		}
	}

	/* cover transitions in @this but not in @rel */
	unsigned int this_count = 0;
	if (count < trans.size()) {
		for (StateTrans::iterator i = trans.begin(); i != trans.end(); i++) {
			StateTrans::iterator j = rel->trans.find(i->first);
			if (j == rel->trans.end()) {
				this_count++;
				if (i->second == rel->otherwise)
					/* replaced by rel->cases.otherwise */
					weight++;
			}
		}
	}

	if (rel->otherwise != otherwise) {
		/* rel default transitions have to be masked with transitions
		 * This covers all transitions not covered above
		 */
		weight -= 256 - (rel->trans.size() + this_count);
	}

	return weight;
}

/**
 * make_relative - Make this state relative to @rel
 * @rel: state to make this state relative too
 *
 * @rel can be a relative (differentially compressed state)
 */
int State::make_relative(State *rel)
{
	int weight = 0;

	if (this == rel || !rel)
		return 0;

	if (flags & DiffEncodeFlag)
		return 0;

	flags |= DiffEncodeFlag;

	for (int i = 0; i < 256 ; i++) {
		State *next = rel->next(i);

		StateTrans::iterator j = trans.find(i);
		if (j != trans.end()) {
			if (j->second == next) {
				trans.erase(j);
				weight++;
			}
			/* else keep transition to mask */
		} else if (otherwise == next) {
			/* do nothing, otherwise transition disappears when
			 * reassigned
			 */
		} else {
			/* need a new transition to mask those in lower state */
			trans[i] = otherwise;
			weight--;
		}
	}

	otherwise = rel;

	return weight;
}

/**
 * flatten_differential - remove differential encode from this state
 */
void State::flatten_relative(void)
{
	if (!(flags & DiffEncodeFlag))
		return;

	map<State *, int> count;

	for (int i = 0; i < 256; i++)
		count[next(i)] += 1;

	int j = 0;
	State *def = next(0);
	for (int i = 1; i < 256; i++) {
		if (count[next(i)] > count[next(j)]) {
			j = i;
			def = next(i);
		}
	}

	for (int i = 0; i < 256; i++) {
		if (trans.find(i) != trans.end()) {
			if (trans[i] == def)
				trans.erase(i);
		} else {
			if (trans[i] != def)
				trans[i] = next(i);
		}
	}

	otherwise = def;
	flags = flags & ~DiffEncodeFlag;
}

static void split_node_types(NodeSet *nodes, NodeSet **anodes, NodeSet **nnodes
)
{
	*anodes = *nnodes = NULL;
	for (NodeSet::iterator i = nodes->begin(); i != nodes->end(); ) {
		if ((*i)->is_accept()) {
			if (!*anodes)
				*anodes = new NodeSet;
			(*anodes)->insert(*i);
			NodeSet::iterator k = i++;
			nodes->erase(k);
		} else
			i++;
	}
	*nnodes = nodes;
}

State *DFA::add_new_state(NodeSet *anodes, NodeSet *nnodes, State *other)
{
	hashedNodeVec *nnodev;
	nnodev = nnodes_cache.insert(nnodes);
	anodes = anodes_cache.insert(anodes);

	ProtoState proto;
	proto.init(nnodev, anodes);
	State *state = new State(node_map.size(), proto, other);
	pair<NodeMap::iterator,bool> x = node_map.insert(proto, state);
	if (x.second == false) {
		delete state;
	} else {
		states.push_back(state);
		work_queue.push_back(state);
	}

	return x.first->second;
}

State *DFA::add_new_state(NodeSet *nodes, State *other)
{
	/* The splitting of nodes should probably get pushed down into
	 * follow(), ie. put in separate lists from the start
	 */
	NodeSet *anodes, *nnodes;
	split_node_types(nodes, &anodes, &nnodes);

	State *state = add_new_state(anodes, nnodes, other);

	return state;
}

void DFA::update_state_transitions(State *state)
{
	/* Compute possible transitions for state->nodes.  This is done by
	 * iterating over all the nodes in state->nodes and combining the
	 * transitions.
	 *
	 * The resultant transition set is a mapping of characters to
	 * sets of nodes.
	 *
	 * Note: the follow set for accept nodes is always empty so we don't
	 * need to compute follow for the accept nodes in a protostate
	 */
	Cases cases;
	for (hashedNodeVec::iterator i = state->proto.nnodes->begin(); i != state->proto.nnodes->end(); i++)
		(*i)->follow(cases);

	/* Now for each set of nodes in the computed transitions, make
	 * sure that there is a state that maps to it, and add the
	 * matching case to the state.
	 */

	/* check the default transition first */
	if (cases.otherwise)
		state->otherwise = add_new_state(cases.otherwise, nonmatching);
	else
		state->otherwise = nonmatching;

	/* For each transition from *from, check if the set of nodes it
	 * transitions to already has been mapped to a state
	 */
	for (Cases::iterator j = cases.begin(); j != cases.end(); j++) {
		State *target;
		target = add_new_state(j->second, nonmatching);

		/* Don't insert transition that the otherwise transition
		 * already covers
		 */
		if (target != state->otherwise)
			state->trans[j->first] = target;
	}
}

/* WARNING: This routine can only be called from within DFA creation as
 * the nodes value is only valid during dfa construction.
 */
void DFA::dump_node_to_dfa(void)
{
	cerr << "Mapping of States to expr nodes\n"
		"  State  <=   Nodes\n"
		"-------------------\n";
	for (Partition::iterator i = states.begin(); i != states.end(); i++)
		cerr << "  " << (*i)->label << " <= " << (*i)->proto << "\n";
}

void DFA::process_work_queue(const char *header, dfaflags_t flags)
{
	int i = 0;

	while (!work_queue.empty()) {
		if (i % 1000 == 0 && (flags & DFA_DUMP_PROGRESS)) {
			cerr << "\033[2K" << header << ": queue "
			     << work_queue.size()
			     << "\tstates "
			     << states.size()
			     << "\teliminated duplicates "
			     << node_map.dup
			     << "\r";
		}
		i++;

		State *from = work_queue.front();
		work_queue.pop_front();

		/* Update 'from's transitions, and if it transitions to any
		 * unknown State create it and add it to the work_queue
		 */
		update_state_transitions(from);
	}  /* while (!work_queue.empty()) */
}

/**
 * Construct a DFA from a syntax tree.
 */
DFA::DFA(Node *root, dfaflags_t flags): root(root)
{
	diffcount = 0;		/* set by diff_encode */

	if (flags & DFA_DUMP_PROGRESS)
		fprintf(stderr, "Creating dfa:\r");

	for (depth_first_traversal i(root); i; i++) {
		(*i)->compute_nullable();
		(*i)->compute_firstpos();
		(*i)->compute_lastpos();
	}

	if (flags & DFA_DUMP_PROGRESS)
		fprintf(stderr, "Creating dfa: followpos\r");
	for (depth_first_traversal i(root); i; i++) {
		(*i)->compute_followpos();
	}

	nonmatching = add_new_state(new NodeSet, NULL);
	start = add_new_state(new NodeSet(root->firstpos), nonmatching);

	/* the work_queue contains the states that need to have their
	 * transitions computed.  This could be done with a recursive
	 * algorithm instead of a work_queue, but it would be slightly slower
	 * and consume more memory.
	 *
	 * TODO: currently the work_queue is treated in a breadth first
	 *       search manner.  Test using the work_queue in a depth first
	 *       manner, this may help reduce the number of entries on the
	 *       work_queue at any given time, thus reducing peak memory use.
	 */
	work_queue.push_back(start);
	process_work_queue("Creating dfa", flags);

	/* cleanup Sets of nodes used computing the DFA as they are no longer
	 * needed.
	 */
	for (depth_first_traversal i(root); i; i++) {
		(*i)->firstpos.clear();
		(*i)->lastpos.clear();
		(*i)->followpos.clear();
	}

	if (flags & DFA_DUMP_NODE_TO_DFA)
		dump_node_to_dfa();

	if (flags & (DFA_DUMP_STATS)) {
		cerr << "\033[2KCreated dfa: states "
		     << states.size()
		     << " proto { "
		     << node_map
		     << " }, nnodes { "
		     << nnodes_cache
		     << " }, anodes { "
		     << anodes_cache
		     << " }\n";
	}

	/* Clear out uniq_nnodes as they are no longer needed.
	 * Do not clear out uniq_anodes, as we need them for minimizations
	 * diffs, unions, ...
	 */
	nnodes_cache.clear();
	node_map.clear();
}

DFA::~DFA()
{
	anodes_cache.clear();
	nnodes_cache.clear();

	for (Partition::iterator i = states.begin(); i != states.end(); i++)
		delete *i;
}

State *DFA::match_len(State *state, const char *str, size_t len)
{
	for (; len > 0; ++str, --len)
		state = state->next(*str);

	return state;
}

State *DFA::match_until(State *state, const char *str, const char term)
{
	while (*str != term)
		state = state->next(*str++);

	return state;
}

State *DFA::match(const char *str)
{
	return match_until(start, str, 0);
}

void DFA::dump_uniq_perms(const char *s)
{
	set<perms_t> uniq;
	for (Partition::iterator i = states.begin(); i != states.end(); i++)
		uniq.insert((*i)->perms);

	cerr << "Unique Permission sets: " << s << " (" << uniq.size() << ")\n";
	cerr << "----------------------\n";
	for (set<perms_t >::iterator i = uniq.begin(); i != uniq.end(); i++) {
		cerr << "  allow:" << hex << i->allow << " deny:"
		     << i->deny << " audit:" << i->audit
		     << " quiet:" << i->quiet << dec << "\n";
	}
}

/* Remove dead or unreachable states */
void DFA::remove_unreachable(dfaflags_t flags)
{
	set<State *> reachable;

	/* find the set of reachable states */
	reachable.insert(nonmatching);
	work_queue.push_back(start);
	while (!work_queue.empty()) {
		State *from = work_queue.front();
		work_queue.pop_front();
		reachable.insert(from);

		if (from->otherwise != nonmatching &&
		    reachable.find(from->otherwise) == reachable.end())
			work_queue.push_back(from->otherwise);

		for (StateTrans::iterator j = from->trans.begin(); j != from->trans.end(); j++) {
			if (reachable.find(j->second) == reachable.end())
				work_queue.push_back(j->second);
		}
	}

	/* walk the set of states and remove any that aren't reachable */
	if (reachable.size() < states.size()) {
		int count = 0;
		Partition::iterator i;
		Partition::iterator next;
		for (i = states.begin(); i != states.end(); i = next) {
			next = i;
			next++;
			if (reachable.find(*i) == reachable.end()) {
				if (flags & DFA_DUMP_UNREACHABLE) {
					cerr << "unreachable: " << **i;
					if (*i == start)
						cerr << " <==";
					if ((*i)->perms.is_accept())
						(*i)->perms.dump(cerr);
					cerr << "\n";
				}
				State *current = *i;
				states.erase(i);
				delete(current);
				count++;
			}
		}

		if (count && (flags & DFA_DUMP_STATS))
			cerr << "DFA: states " << states.size() << " removed "
			     << count << " unreachable states\n";
	}
}

/* test if two states have the same transitions under partition_map */
bool DFA::same_mappings(State *s1, State *s2)
{
	/* assumes otherwise is set to best choice, if there are multiple
	 * otherwise choices this will fail to fully minimize the dfa
	 * if we are not careful. Make sure in cases with multiple
	 * equiv otherwise we always choose the same otherwise to avoid
	 */
	if (s1->otherwise->partition != s2->otherwise->partition)
		return false;

	StateTrans::iterator j1;
	StateTrans::iterator j2;
	for (j1 = s1->trans.begin(), j2 = s2->trans.begin();
	     j1 != s1->trans.end() && j2 != s2->trans.end();
	     /*inc inline*/) {
		if (j1->first < j2->first) {
			if (j1->second->partition != s2->otherwise->partition)
				return false;
			j1++;
		} else if (j1->first == j2->first) {
			if (j1->second->partition != j2->second->partition)
				return false;
			j1++;
			j2++;
		} else {
			if (s1->otherwise->partition != j2->second->partition)
				return false;
			j2++;
		}
	}
	for ( ; j1 != s1->trans.end(); j1++) {
		if (j1->second->partition != s2->otherwise->partition)
			return false;
	}
	for ( ; j2 != s2->trans.end(); j2++) {
		if (j2->second->partition != s1->otherwise->partition)
			return false;
	}

	return true;
}

int DFA::apply_and_clear_deny(void)
{
	int c = 0;
	for (Partition::iterator i = states.begin(); i != states.end(); i++)
		c += (*i)->apply_and_clear_deny();

	return c;
}

/* minimize the number of dfa states */
void DFA::minimize(dfaflags_t flags)
{
	map<pair<uint64_t, size_t>, Partition *> perm_map;
	list<Partition *> partitions;

	/* Set up the initial partitions
	 * minimium of - 1 non accepting, and 1 accepting
	 * if trans hashing is used the accepting and non-accepting partitions
	 * can be further split based on the number and type of transitions
	 * a state makes.
	 * If permission hashing is enabled the accepting partitions can
	 * be further divided by permissions.  This can result in not
	 * obtaining a truely minimized dfa but comes close, and can speedup
	 * minimization.
	 */
	int accept_count = 0;
	int final_accept = 0;
	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		size_t hash = 0;
		uint64_t permtype = ((uint64_t) (PACK_AUDIT_CTL((*i)->perms.audit, (*i)->perms.quiet & (*i)->perms.deny)) << 32) | (uint64_t) (*i)->perms.allow;
		pair<uint64_t, size_t> group = make_pair(permtype, hash);
		map<pair<uint64_t, size_t>, Partition *>::iterator p = perm_map.find(group);
		if (p == perm_map.end()) {
			Partition *part = new Partition();
			part->push_back(*i);
			perm_map.insert(make_pair(group, part));
			partitions.push_back(part);
			(*i)->partition = part;
			if (permtype)
				accept_count++;
		} else {
			(*i)->partition = p->second;
			p->second->push_back(*i);
		}

		if ((flags & DFA_DUMP_PROGRESS) && (partitions.size() % 1000 == 0))
			cerr << "\033[2KMinimize dfa: partitions "
			     << partitions.size() << "\tinit " << partitions.size()
			     << " (accept " << accept_count << ")\r";
	}

	/* perm_map is no longer needed so free the memory it is using.
	 * Don't remove - doing it manually here helps reduce peak memory usage.
	 */
	perm_map.clear();

	int init_count = partitions.size();
	if (flags & DFA_DUMP_PROGRESS)
		cerr << "\033[2KMinimize dfa: partitions " << partitions.size()
		     << "\tinit " << init_count << " (accept "
		     << accept_count << ")\r";

	/* Now do repartitioning until each partition contains the set of
	 * states that are the same.  This will happen when the partition
	 * splitting stables.  With a worse case of 1 state per partition
	 * ie. already minimized.
	 */
	Partition *new_part;
	int new_part_count;
	do {
		new_part_count = 0;
		for (list<Partition *>::iterator p = partitions.begin();
		     p != partitions.end(); p++) {
			new_part = NULL;
			State *rep = *((*p)->begin());
			Partition::iterator next;
			for (Partition::iterator s = ++(*p)->begin(); s != (*p)->end();) {
				if (same_mappings(rep, *s)) {
					++s;
					continue;
				}
				if (!new_part) {
					new_part = new Partition;
					list<Partition *>::iterator tmp = p;
					partitions.insert(++tmp, new_part);
					new_part_count++;
				}
				new_part->push_back(*s);
				s = (*p)->erase(s);
			}
			/* remapping partition_map for new_part entries
			 * Do not do this above as it messes up same_mappings
			 */
			if (new_part) {
				for (Partition::iterator m = new_part->begin();
				     m != new_part->end(); m++) {
					(*m)->partition = new_part;
				}
			}
			if ((flags & DFA_DUMP_PROGRESS) && (partitions.size() % 100 == 0))
				cerr << "\033[2KMinimize dfa: partitions "
				     << partitions.size() << "\tinit "
				     << init_count << " (accept "
				     << accept_count << ")\r";
		}
	} while (new_part_count);

	if (partitions.size() == states.size()) {
		if (flags & DFA_DUMP_STATS)
			cerr << "\033[2KDfa minimization no states removed: partitions "
			     << partitions.size() << "\tinit " << init_count
			     << " (accept " << accept_count << ")\n";

		goto out;
	}

	/* Remap the dfa so it uses the representative states
	 * Use the first state of a partition as the representative state
	 * At this point all states with in a partion have transitions
	 * to states within the same partitions, however this can slow
	 * down compressed dfa compression as there are more states,
	 */
	if (flags & DFA_DUMP_MIN_PARTS)
		cerr << "Partitions after minimization\n";
	for (list<Partition *>::iterator p = partitions.begin();
	     p != partitions.end(); p++) {
		/* representative state for this partition */
		State *rep = *((*p)->begin());
		if (flags & DFA_DUMP_MIN_PARTS)
			cerr << *rep << " : ";

		/* update representative state's transitions */
		rep->otherwise = *rep->otherwise->partition->begin();

		for (StateTrans::iterator c = rep->trans.begin(); c != rep->trans.end(); ) {
			Partition *partition = c->second->partition;
			if (rep->otherwise != *partition->begin()) {
				c->second = *partition->begin();
				c++;
			} else
				/* transition is now covered by otherwise */
				c = rep->trans.erase(c);
		}

//if ((*p)->size() > 1)
//cerr << rep->label << ": ";
		/* clear the state label for all non representative states,
		 * and accumulate permissions */
		for (Partition::iterator i = ++(*p)->begin(); i != (*p)->end(); i++) {
//cerr << " " << (*i)->label;
			if (flags & DFA_DUMP_MIN_PARTS)
				cerr << **i << ", ";
			(*i)->label = -1;
			rep->perms.add((*i)->perms);
		}
		if (rep->perms.is_accept())
			final_accept++;
//if ((*p)->size() > 1)
//cerr << "\n";
		if (flags & DFA_DUMP_MIN_PARTS)
			cerr << "\n";
	}
	if (flags & DFA_DUMP_STATS)
		cerr << "\033[2KMinimized dfa: final partitions "
		     << partitions.size() << " (accept " << final_accept
		     << ")" << "\tinit " << init_count << " (accept "
		     << accept_count << ")\n";

	/* make sure nonmatching and start state are up to date with the
	 * mappings */
	{
		Partition *partition = nonmatching->partition;
		if (*partition->begin() != nonmatching) {
			nonmatching = *partition->begin();
		}

		partition = start->partition;
		if (*partition->begin() != start) {
			start = *partition->begin();
		}
	}

	/* Now that the states have been remapped, remove all states
	 * that are not the representive states for their partition, they
	 * will have a label == -1
	 */
	for (Partition::iterator i = states.begin(); i != states.end();) {
		if ((*i)->label == -1) {
			State *s = *i;
			i = states.erase(i);
			delete(s);
		} else
			i++;
	}

out:
	/* Cleanup */
	while (!partitions.empty()) {
		Partition *p = partitions.front();
		partitions.pop_front();
		delete(p);
	}
}


/* diff_encode helper functions */
static unsigned int add_to_dag(DiffDag *dag, State *state,
			       State *parent)
{
	unsigned int rc = 0;
	if (!state->diff) {
		dag->rel = NULL;
		if (parent)
			dag->depth = parent->diff->depth + 1;
		else
			dag->depth = 1;
		dag->state = state;
		state->diff = dag;
		rc = 1;
	}
	if (parent && parent->diff->depth < state->diff->depth)
		state->diff->parents.push_back(parent);
	return rc;
}

static int diff_partition(State *state, Partition &part, State **candidate)
{
	int weight = 0;
	*candidate = NULL;

	for (Partition::iterator i = part.begin(); i != part.end(); i++) {
		if (*i == state)
			continue;

		int tmp = state->diff_weight(*i);
		if (tmp > weight) {
			weight = tmp;
			*candidate = *i;
		}
	}
	return weight;
}

/**
 * diff_encode - compress dfa by differentially encoding state transitions
 * @dfa_flags: flags controling dfa creation
 *
 * This function reduces the number of transitions that need to be stored
 * by encoding transitions as the difference between the state and a
 * another transitions that is set as the states default.
 *
 * For performance reasons this function does not try to compute the
 * absolute best encoding (maximal spanning tree) but instead computes
 * a very good encoding within the following limitations.
 *   - Not all states have to be differentially encoded.  This allows for
 *     multiple states to be used as a terminating basis.
 *   - The number of state transitions needed to match an input of length
 *     m will be 2m
 *
 * To guarentee this the ordering and distance calculation is done in the
 * following manner.
 * - A DAG of the DFA is created starting with the start state(s).
 * - A state can only be relative (have a differential encoding) to
 *   another state if that state has
 *   - a lower depth in the DAG
 *   - is a sibling (same depth) that is not relative
 *   - is a sibling that is relative to a state with lower depth in the DAG
 *
 * The run time constraints are maintained by the DAG ordering + relative
 * state constraints.  For any input character C when at state S with S being
 * at level N in the DAG then at most 2N states must be traversed to find the
 * transition for C.  However on the maximal number of transitions is not m*m,
 * because  when a character is matched and forward movement is made through
 * the DFA any relative transition search will move back through the DAG order.
 * So say for character C we start matching on a state S that is at depth 10
 * in the DAG.  The transition for C is not found in S and we recurse backwards
 * to a depth of 6.  A transition is found and it steps to the next state, but
 * the state transition at most will only move 1 deeper into the DAG so for
 * the next state the maximum number of states traversed is 2*7.
 */
void DFA::diff_encode(dfaflags_t flags)
{
	DiffDag *dag;
	unsigned int xcount = 0, xweight = 0, transitions = 0, depth = 0;

	/* clear the depth flag */
	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		(*i)->diff = NULL;
		transitions += (*i)->trans.size();
	}

	/* Prealloc structures we need.  We know the exact number of elements,
	 * and once setup they don't change so we don't need the flexibility
	 * or overhead of stl, just allocate the needed data as an array
	 */
	dag = new DiffDag [states.size()];

	/* Generate DAG ordering and parent sets */
	add_to_dag(&dag[0], nonmatching, NULL);
	add_to_dag(&dag[1], start, NULL);

	unsigned int tail = 2;
	for (unsigned int i = 1; i < tail; i++) {
		State *state = dag[i].state;
		State *child = dag[i].state->otherwise;
		if (child)
			tail += add_to_dag(&dag[tail], child, state);

		for (StateTrans::iterator j = state->trans.begin(); j != state->trans.end(); j++) {
			child = j->second;
			tail += add_to_dag(&dag[tail], child, state);
		}
	}
	depth = dag[tail - 1].depth;

	/* calculate which state to make a transitions relative too */
	for (unsigned int i = 2; i < tail; i++) {
		State *state = dag[i].state;
		State *candidate = NULL;

		int weight = diff_partition(state,
					    state->otherwise->diff->parents,
					    &candidate);

		for (StateTrans::iterator j = state->trans.begin(); j != state->trans.end(); j++) {
			State *tmp_candidate;
			int tmp = diff_partition(state,
						 j->second->diff->parents,
						 &tmp_candidate);
			if (tmp > weight) {
				weight = tmp;
				candidate = tmp_candidate;
			}
		}

		if ((flags & DFA_DUMP_DIFF_PROGRESS) && (i % 100 == 0))
			cerr << "\033[2KDiff Encode: " << i << " of "
			     << tail << ".  Diff states " << xcount
			     << " Savings " << xweight << "\r";

		state->diff->rel = candidate;
		if (candidate) {
			xcount++;
			xweight += weight;
		}
	}

	/* now make transitions relative, start at the back of the list so
	 * as to start with the last transitions and work backwards to avoid
	 * having to traverse multiple previous states (that have been made
	 * relative already) to reconstruct previous state transition table
	 */
	unsigned int aweight = 0;
	diffcount = 0;
	for (int i = tail - 1; i > 1; i--) {
		if (dag[i].rel) {
			int weight = dag[i].state->make_relative(dag[i].rel);
			aweight += weight;
			diffcount++;
		}
	}

	if (flags & DFA_DUMP_DIFF_STATS)
		cerr << "Diff encode  states: " << diffcount << " of "
                     << tail << " reached @ depth "  << depth << ". "
		     <<  aweight << " trans removed\n";

	if (xweight != aweight)
		cerr << "Diff encode error: actual savings " << aweight
		     << " != expected " << xweight << "\n";

	if (xcount != diffcount)
		cerr << "Diff encode error: actual count " << diffcount
		     << " != expected " << xcount << " \n";

	/* cleanup */
	for (unsigned int i = 0; i < tail; i++)
		dag[i].parents.clear();
	delete [] dag;
}

/**
 * flatten_differential - remove differential state encoding
 *
 * Flatten the dfa back into a flat encoding.
 */
void DFA::undiff_encode(void)
{
	for (Partition::iterator i = states.begin(); i != states.end(); i++)
		(*i)->flatten_relative();
	diffcount = 0;
}

void DFA::dump_diff_chain(ostream &os, map<State *, Partition> &relmap,
			  Partition &chain, State *state, unsigned int &count,
			  unsigned int &total, unsigned int &max)
{
	if (relmap[state].size() == 0) {
		for (Partition::iterator i = chain.begin(); i != chain.end(); i++)
			os << **i << " <- ";
		os << *state << "\n";

		count++;
		total += chain.size() + 1;
		if (chain.size() + 1 > max)
			max = chain.size() + 1;
	}

	chain.push_back(state);
	for (Partition::iterator i = relmap[state].begin(); i != relmap[state].end(); i++)
		dump_diff_chain(os, relmap, chain, *i, count, total, max);
	chain.pop_back();
}

/* Dump the DFA diff_encoding chains */
void DFA::dump_diff_encode(ostream &os)
{
	map<State *, Partition> rel;
	Partition base, chain;

	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		if ((*i)->flags & DiffEncodeFlag)
			rel[(*i)->otherwise].push_back(*i);
		else
			base.push_back(*i);
	}

	unsigned int count = 0, total = 0, max = 0;
	for (Partition::iterator i = base.begin(); i != base.end(); i++)
		dump_diff_chain(os, rel, chain, *i, count, total, max);

	os << base.size() << " non-differentially encoded states\n";
	os << "chains: " << count - base.size() << "\n";
	os << "average chain size: " << (double) (total - base.size()) / (double) (count - base.size()) << "\n";
	os << "longest chain: " << max << "\n";
}

/**
 * text-dump the DFA (for debugging).
 */
void DFA::dump(ostream & os)
{
	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		if (*i == start || (*i)->perms.is_accept()) {
			os << **i;
			if (*i == start)
				os << " <== (allow/deny/audit/quiet)";
			if ((*i)->perms.is_accept())
				(*i)->perms.dump(os);
			os << "\n";
		}
	}
	os << "\n";

	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		Chars excluded;

		for (StateTrans::iterator j = (*i)->trans.begin();
		     j != (*i)->trans.end(); j++) {
			if (j->second == nonmatching) {
				excluded.insert(j->first);
			} else {
				os << **i;
				if ((*i)->perms.is_accept())
					os << " ", (*i)->perms.dump(os);
				os << " -> " << *(j)->second << ": 0x"
				   << hex << (int) j->first;
				if (isprint(j->first))
					os << " " << j->first;
				os << dec << "\n";
			}
		}

		if ((*i)->otherwise != nonmatching) {
			os << **i;
			if ((*i)->perms.is_accept())
				os << " ", (*i)->perms.dump(os);
			os << " -> " << *(*i)->otherwise << ": [";
			if (!excluded.empty()) {
				os << "^";
				for (Chars::iterator k = excluded.begin();
				     k != excluded.end(); k++) {
					if (isprint(*k))
						os << *k;
					else
						os << "\\0x" << hex << (int) *k << dec;
				}
			}
			os << "]\n";
		}
	}
	os << "\n";
}

/**
 * Create a dot (graphviz) graph from the DFA (for debugging).
 */
void DFA::dump_dot_graph(ostream & os)
{
	os << "digraph \"dfa\" {" << "\n";

	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		if (*i == nonmatching)
			continue;

		os << "\t\"" << **i << "\" [" << "\n";
		if (*i == start) {
			os << "\t\tstyle=bold" << "\n";
		}
		if ((*i)->perms.is_accept()) {
			os << "\t\tlabel=\"" << **i << "\\n";
			(*i)->perms.dump(os);
			os << "\"\n";
		}
		os << "\t]" << "\n";
	}
	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		Chars excluded;

		for (StateTrans::iterator j = (*i)->trans.begin(); j != (*i)->trans.end(); j++) {
			if (j->second == nonmatching)
				excluded.insert(j->first);
			else {
				os << "\t\"" << **i << "\" -> \"" << *j->second
				   << "\" [" << "\n";
				os << "\t\tlabel=\"";
				if (isprint(j->first))
					os << j->first;
				else
					os << "\\0x" << hex << (int) j->first << dec;

				os << "\"\n\t]" << "\n";
			}
		}
		if ((*i)->otherwise != nonmatching) {
			os << "\t\"" << **i << "\" -> \"" << *(*i)->otherwise
			   << "\" [" << "\n";
			if (!excluded.empty()) {
				os << "\t\tlabel=\"[^";
				for (Chars::iterator i = excluded.begin();
				     i != excluded.end(); i++) {
					if (isprint(*i))
						os << *i;
					else
						os << "\\0x" << hex << (int) *i << dec;
				}
				os << "]\"" << "\n";
			}
			os << "\t]" << "\n";
		}
	}
	os << '}' << "\n";
}

/**
 * Compute character equivalence classes in the DFA to save space in the
 * transition table.
 */
map<uchar, uchar> DFA::equivalence_classes(dfaflags_t flags)
{
	map<uchar, uchar> classes;
	uchar next_class = 1;

	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		/* Group edges to the same next state together */
		map<const State *, Chars> node_sets;
		for (StateTrans::iterator j = (*i)->trans.begin(); j != (*i)->trans.end(); j++)
			node_sets[j->second].insert(j->first);

		for (map<const State *, Chars>::iterator j = node_sets.begin();
		     j != node_sets.end(); j++) {
			/* Group edges to the same next state together by class */
			map<uchar, Chars> node_classes;
			bool class_used = false;
			for (Chars::iterator k = j->second.begin();
			     k != j->second.end(); k++) {
				pair<map<uchar, uchar>::iterator, bool> x = classes.insert(make_pair(*k, next_class));
				if (x.second)
					class_used = true;
				pair<map<uchar, Chars>::iterator, bool> y = node_classes.insert(make_pair(x.first->second, Chars()));
				y.first->second.insert(*k);
			}
			if (class_used) {
				next_class++;
				class_used = false;
			}
			for (map<uchar, Chars>::iterator k = node_classes.begin();
			     k != node_classes.end(); k++) {
			  /**
			   * If any other characters are in the same class, move
			   * the characters in this class into their own new
			   * class
			   */
				map<uchar, uchar>::iterator l;
				for (l = classes.begin(); l != classes.end(); l++) {
					if (l->second == k->first &&
					    k->second.find(l->first) == k->second.end()) {
						class_used = true;
						break;
					}
				}
				if (class_used) {
					for (Chars::iterator l = k->second.begin();
					     l != k->second.end(); l++) {
						classes[*l] = next_class;
					}
					next_class++;
					class_used = false;
				}
			}
		}
	}

	if (flags & DFA_DUMP_EQUIV_STATS)
		fprintf(stderr, "Equiv class reduces to %d classes\n",
			next_class - 1);
	return classes;
}

/**
 * Text-dump the equivalence classes (for debugging).
 */
void dump_equivalence_classes(ostream &os, map<uchar, uchar> &eq)
{
	map<uchar, Chars> rev;

	for (map<uchar, uchar>::iterator i = eq.begin(); i != eq.end(); i++) {
		Chars &chars = rev.insert(make_pair(i->second, Chars())).first->second;
		chars.insert(i->first);
	}
	os << "(eq):" << "\n";
	for (map<uchar, Chars>::iterator i = rev.begin(); i != rev.end(); i++) {
		os << (int)i->first << ':';
		Chars &chars = i->second;
		for (Chars::iterator j = chars.begin(); j != chars.end(); j++) {
			os << ' ' << *j;
		}
		os << "\n";
	}
}

/**
 * Replace characters with classes (which are also represented as
 * characters) in the DFA transition table.
 */
void DFA::apply_equivalence_classes(map<uchar, uchar> &eq)
{
    /**
     * Note: We only transform the transition table; the nodes continue to
     * contain the original characters.
     */
	for (Partition::iterator i = states.begin(); i != states.end(); i++) {
		map<uchar, State *> tmp;
		tmp.swap((*i)->trans);
		for (StateTrans::iterator j = tmp.begin(); j != tmp.end(); j++)
			(*i)->trans.insert(make_pair(eq[j->first], j->second));
	}
}

#if 0
typedef set <ImportantNode *>AcceptNodes;
map<ImportantNode *, AcceptNodes> dominance(DFA & dfa)
{
	map<ImportantNode *, AcceptNodes> is_dominated;

	for (States::iterator i = dfa.states.begin(); i != dfa.states.end(); i++) {
		AcceptNodes set1;
		for (State::iterator j = (*i)->begin(); j != (*i)->end(); j++) {
			if (AcceptNode * accept = dynamic_cast<AcceptNode *>(*j))
				set1.insert(accept);
		}
		for (AcceptNodes::iterator j = set1.begin(); j != set1.end(); j++) {
			pair<map<ImportantNode *, AcceptNodes>::iterator, bool> x = is_dominated.insert(make_pair(*j, set1));
			if (!x.second) {
				AcceptNodes & set2(x.first->second), set3;
				for (AcceptNodes::iterator l = set2.begin();
				     l != set2.end(); l++) {
					if (set1.find(*l) != set1.end())
						set3.insert(*l);
				}
				set3.swap(set2);
			}
		}
	}
	return is_dominated;
}
#endif

static inline int diff_qualifiers(uint32_t perm1, uint32_t perm2)
{
	return ((perm1 & AA_EXEC_TYPE) && (perm2 & AA_EXEC_TYPE) &&
		(perm1 & AA_EXEC_TYPE) != (perm2 & AA_EXEC_TYPE));
}

/**
 * Compute the permission flags that this state corresponds to. If we
 * have any exact matches, then they override the execute and safe
 * execute flags.
 */
int accept_perms(NodeSet *state, perms_t &perms)
{
	int error = 0;
	uint32_t exact_match_allow = 0;
	uint32_t exact_audit = 0;

	perms.clear();

	if (!state)
		return error;

	for (NodeSet::iterator i = state->begin(); i != state->end(); i++) {
		MatchFlag *match;
		if (!(match = dynamic_cast<MatchFlag *>(*i)))
			continue;
		if (dynamic_cast<ExactMatchFlag *>(match)) {
			/* exact match only ever happens with x */
			if (!is_merged_x_consistent(exact_match_allow,
						    match->flag))
				error = 1;;
			exact_match_allow |= match->flag;
			exact_audit |= match->audit;
		} else if (dynamic_cast<DenyMatchFlag *>(match)) {
			perms.deny |= match->flag;
			perms.quiet |= match->audit;
		} else {
			if (!is_merged_x_consistent(perms.allow, match->flag))
				error = 1;
			perms.allow |= match->flag;
			perms.audit |= match->audit;
		}
	}

	perms.allow |= exact_match_allow & ~(ALL_AA_EXEC_TYPE);
	perms.audit |= exact_audit & ~(ALL_AA_EXEC_TYPE);
	
	if (exact_match_allow & AA_USER_EXEC) {
		perms.allow = (exact_match_allow & AA_USER_EXEC_TYPE) |
			(perms.allow & ~AA_USER_EXEC_TYPE);
		perms.exact = AA_USER_EXEC_TYPE;
	}
	if (exact_match_allow & AA_OTHER_EXEC) {
		perms.allow = (exact_match_allow & AA_OTHER_EXEC_TYPE) |
			(perms.allow & ~AA_OTHER_EXEC_TYPE);
		perms.exact |= AA_OTHER_EXEC_TYPE;
	}
	if (AA_USER_EXEC & perms.deny)
		perms.deny |= AA_USER_EXEC_TYPE;

	if (AA_OTHER_EXEC & perms.deny)
		perms.deny |= AA_OTHER_EXEC_TYPE;

	perms.allow &= ~perms.deny;
	perms.quiet &= perms.deny;

	if (error)
		fprintf(stderr, "profile has merged rule with conflicting x modifiers\n");

	return error;
}
