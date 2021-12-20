/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
 *
 *   Copyright (c) 2010 - 2012
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

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <search.h>
#include <string.h>
#include <errno.h>
#include <sys/apparmor.h>

#include "parser.h"
#include "profile.h"
#include "parser_yacc.h"

/* #define DEBUG */
#ifdef DEBUG
#undef PDEBUG
#define PDEBUG(fmt, args...) fprintf(stderr, "Lexer: " fmt, ## args)
#else
#undef PDEBUG
#define PDEBUG(fmt, args...)	/* Do nothing */
#endif
#define NPDEBUG(fmt, args...)	/* Do nothing */


ProfileList policy_list;


void add_to_list(Profile *prof)
{
	pair<ProfileList::iterator, bool> res = policy_list.insert(prof);
	if (!res.second) {
		PERROR("Multiple definitions for profile %s exist,"
		       "bailing out.\n", prof->name);
		exit(1);
	}
}

void add_hat_to_policy(Profile *prof, Profile *hat)
{
	hat->parent = prof;

	pair<ProfileList::iterator, bool> res = prof->hat_table.insert(hat);
	if (!res.second) {
		PERROR("Multiple definitions for hat %s in profile %s exist,"
		       "bailing out.\n", hat->name, prof->name);
		exit(1);
	}
}

int add_entry_to_x_table(Profile *prof, char *name)
{
	int i;
	for (i = (AA_EXEC_LOCAL >> 10) + 1; i < AA_EXEC_COUNT; i++) {
		if (!prof->exec_table[i]) {
			prof->exec_table[i] = name;
			return i;
		} else if (strcmp(prof->exec_table[i], name) == 0) {
			/* name already in table */
			free(name);
			return i;
		}
	}
	free(name);
	return 0;
}

static int add_named_transition(Profile *prof, struct cod_entry *entry)
{
	char *name = NULL;

	/* check to see if it is a local transition */
	if (!label_contains_ns(entry->nt_name)) {
		char *sub = strstr(entry->nt_name, "//");
		/* does the subprofile name match the rule */

		if (sub && strncmp(prof->name, sub, sub - entry->nt_name) &&
		    strcmp(sub + 2, entry->name) == 0) {
			free(entry->nt_name);
			entry->nt_name = NULL;
			return AA_EXEC_LOCAL >> 10;
		} else if (((entry->mode & AA_USER_EXEC_MODIFIERS) ==
			     SHIFT_MODE(AA_EXEC_LOCAL, AA_USER_SHIFT)) ||
			    ((entry->mode & AA_OTHER_EXEC_MODIFIERS) ==
			     SHIFT_MODE(AA_EXEC_LOCAL, AA_OTHER_SHIFT))) {
			if (strcmp(entry->nt_name, entry->name) == 0) {
				free(entry->nt_name);
				entry->nt_name = NULL;
				return AA_EXEC_LOCAL >> 10;
			}
			/* specified as cix so profile name is implicit */
			name = (char *) malloc(strlen(prof->name) + strlen(entry->nt_name)
				      + 3);
			if (!name) {
				PERROR("Memory allocation error\n");
				exit(1);
			}
			sprintf(name, "%s//%s", prof->name, entry->nt_name);
			free(entry->nt_name);
			entry->nt_name = NULL;
		} else {
			/**
			 * pass control of the memory pointed to by nt_name
			 * from entry to add_entry_to_x_table()
			 */
			name = entry->nt_name;
			entry->nt_name = NULL;
		}
	} else {
		/**
		 * pass control of the memory pointed to by nt_name
		 * from entry to add_entry_to_x_table()
		 */
		name = entry->nt_name;
		entry->nt_name = NULL;
	}

	return add_entry_to_x_table(prof, name);
}

void add_entry_to_policy(Profile *prof, struct cod_entry *entry)
{
	entry->next = prof->entries;
	prof->entries = entry;
}

void post_process_file_entries(Profile *prof)
{
	struct cod_entry *entry;
	int cp_mode = 0;

	list_for_each(prof->entries, entry) {
		if (entry->nt_name) {
			int mode = 0;
			int n = add_named_transition(prof, entry);
			if (!n) {
				PERROR("Profile %s has too many specified profile transitions.\n", prof->name);
				exit(1);
			}
			if (entry->mode & AA_USER_EXEC)
				mode |= SHIFT_MODE(n << 10, AA_USER_SHIFT);
			if (entry->mode & AA_OTHER_EXEC)
				mode |= SHIFT_MODE(n << 10, AA_OTHER_SHIFT);
			entry->mode = ((entry->mode & ~AA_ALL_EXEC_MODIFIERS) |
				       (mode & AA_ALL_EXEC_MODIFIERS));
		}
		/* FIXME: currently change_profile also implies onexec */
		cp_mode |= entry->mode & (AA_CHANGE_PROFILE);
	}

	/* if there are change_profile rules, this implies that we need
	 * access to /proc/self/attr/current
	 */
	if (cp_mode & AA_CHANGE_PROFILE) {
		/* FIXME: should use @{PROC}/@{PID}/attr/{current,exec} */
		struct cod_entry *new_ent;
		char *buffer = strdup("/proc/*/attr/{current,exec}");
		if (!buffer) {
			PERROR("Memory allocation error\n");
			exit(1);
		}
		new_ent = new_entry(buffer, AA_MAY_WRITE, NULL);
		if (!new_ent) {
			PERROR("Memory allocation error\n");
			exit(1);
		}
		add_entry_to_policy(prof, new_ent);
	}
}

void post_process_rule_entries(Profile *prof)
{
	for (RuleList::iterator i = prof->rule_ents.begin(); i != prof->rule_ents.end(); i++)
		(*i)->post_process(*prof);
}


#define CHANGEHAT_PATH "/proc/[0-9]*/attr/current"

/* add file rules to access /proc files to call change_hat()
 */
static int profile_add_hat_rules(Profile *prof)
{
	struct cod_entry *entry;

	/* TODO: ??? fix logic for when to add to hat/base vs. local */
	/* don't add hat rules for local_profiles or base profiles */
	if (prof->local || prof->hat_table.empty())
		return 0;

	/* add entry to hat */
	entry = new_entry(strdup(CHANGEHAT_PATH), AA_MAY_WRITE, NULL);
	if (!entry)
		return ENOMEM;

	add_entry_to_policy(prof, entry);

	return 0;
}

int load_policy_list(ProfileList &list, int option,
		     aa_kernel_interface *kernel_interface, int cache_fd)
{
	int res = 0;

	for (ProfileList::iterator i = list.begin(); i != list.end(); i++) {
		res = load_profile(option, kernel_interface, *i, cache_fd);
		if (res != 0)
			break;
	}

	return res;
}

int load_flattened_hats(Profile *prof, int option,
			aa_kernel_interface *kernel_interface, int cache_fd)
{
	return load_policy_list(prof->hat_table, option, kernel_interface,
				cache_fd);
}

int load_policy(int option, aa_kernel_interface *kernel_interface, int cache_fd)
{
	return load_policy_list(policy_list, option, kernel_interface, cache_fd);
}

int load_hats(std::ostringstream &buf, Profile *prof)
{
	for (ProfileList::iterator i = prof->hat_table.begin(); i != prof->hat_table.end(); i++) {
		sd_serialize_profile(buf, *i, 0);
	}

	return 0;
}


void dump_policy(void)
{
	policy_list.dump();
}

void dump_policy_names(void)
{
	policy_list.dump_profile_names(true);
}

/* merge_hats: merges hat_table into hat_table owned by prof */
static void merge_hats(Profile *prof, ProfileList &hats)
{
	for (ProfileList::iterator i = hats.begin(); i != hats.end(); ) {
		ProfileList::iterator cur = i++;
		add_hat_to_policy(prof, *cur);
		hats.erase(cur);
	}

}

Profile *merge_policy(Profile *a, Profile *b)
{
	Profile *ret = a;
	struct cod_entry *last;

	if (!a) {
		ret = b;
		goto out;
	}
	if (!b)
		goto out;

	if (a->name || b->name) {
                PERROR("ASSERT: policy merges shouldn't have names %s %s\n",
		       a->name ? a->name : "",
		       b->name ? b->name : "");
		exit(1);
	}

	if (a->entries) {
		list_last_entry(a->entries, last);
		last->next = b->entries;
	} else {
		a->entries = b->entries;
	}
	b->entries = NULL;

	a->flags.complain = a->flags.complain || b->flags.complain;
	a->flags.audit = a->flags.audit || b->flags.audit;

	a->caps.allow |= b->caps.allow;
	a->caps.audit |= b->caps.audit;
	a->caps.deny |= b->caps.deny;
	a->caps.quiet |= b->caps.quiet;

	if (a->net.allow) {
		size_t i;
		for (i = 0; i < get_af_max(); i++) {
			a->net.allow[i] |= b->net.allow[i];
			a->net.audit[i] |= b->net.audit[i];
			a->net.deny[i] |= b->net.deny[i];
			a->net.quiet[i] |= b->net.quiet[i];
		}
	}

	merge_hats(a, b->hat_table);
	delete b;
out:
	return ret;
}

int process_profile_rules(Profile *profile)
{
	int error;

	error = process_profile_regex(profile);
	if (error) {
		PERROR(_("ERROR processing regexs for profile %s, failed to load\n"), profile->name);
		exit(1);
		return error;
	}

	error = process_profile_policydb(profile);
	if (error) {
		PERROR(_("ERROR processing policydb rules for profile %s, failed to load\n"),
		       (profile)->name);
		exit(1);
		return error;
	}

	return 0;
}

int post_process_policy_list(ProfileList &list, int debug_only);
int post_process_profile(Profile *profile, int debug_only)
{
	int error = 0;

	error = profile_add_hat_rules(profile);
	if (error) {
		PERROR(_("ERROR adding hat access rule for profile %s\n"),
		       profile->name);
		return error;
	}

	error = process_profile_variables(profile);
	if (error) {
		PERROR(_("ERROR expanding variables for profile %s, failed to load\n"), profile->name);
		exit(1);
		return error;
	}

	error = replace_profile_aliases(profile);
	if (error) {
		PERROR(_("ERROR replacing aliases for profile %s, failed to load\n"), profile->name);
		return error;
	}

	error = profile_merge_rules(profile);
	if (error) {
		PERROR(_("ERROR merging rules for profile %s, failed to load\n"), profile->name);
		exit(1);
		return error;
	}

	if (!debug_only) {
		error = process_profile_rules(profile);
		if (error)
			return error;
	}

	error = post_process_policy_list(profile->hat_table, debug_only);
	return error;
}

int post_process_policy_list(ProfileList &list, int debug_only)
{
	int error = 0;
	for (ProfileList::iterator i = list.begin(); i != list.end(); i++) {
		error = post_process_profile(*i, debug_only);
		if (error)
			break;
	}

	return error;
}

int post_process_policy(int debug_only)
{
	return post_process_policy_list(policy_list, debug_only);
}

void free_policies(void)
{
	policy_list.clear();
}
