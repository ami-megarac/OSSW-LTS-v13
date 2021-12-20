/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
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
 *   along with this program; if not, contact Novell, Inc.
 */

#include <search.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "immunix.h"
#include "parser.h"
#include "profile.h"

typedef int (*comparison_fn_t)(const void *, const void *);

struct alias_rule {
	char *from;
	char *to;
};

static void *alias_table;

static int compare_alias(const void *a, const void *b)
{
	char *a_name = ((struct alias_rule *) a)->from;
	char *b_name = ((struct alias_rule *) b)->from;
	int res = strcmp(a_name, b_name);
	if (res == 0) {
		a_name = ((struct alias_rule *) a)->to;
		b_name = ((struct alias_rule *) b)->to;
		res = strcmp(a_name, b_name);
	}
	return res;
}

int new_alias(const char *from, const char *to)
{
	struct alias_rule *alias, **result;

	alias = (struct alias_rule *) calloc(1, sizeof(struct alias_rule));
	if (!alias) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		goto fail;
	}
	alias->from = strdup(from);
	if (!alias->from) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		goto fail;
	}
	alias->to = strdup(to);
	if (!alias->to) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		goto fail;
	}

	result = (struct alias_rule **) tsearch(alias, &alias_table, (comparison_fn_t) &compare_alias);
	if (!result) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		goto fail;
	}

	if (*result != alias) {
		/* already existing alias */
		PERROR("'%s' is already defined\n", from);
		goto fail;
	}

	return 1;

fail:
	if (alias) {
		if (alias->from)
			free(alias->from);
		if (alias->to)
			free(alias->to);
		free(alias);
	}
	/* just drop duplicate aliases don't actually fail */
	return 1;
}

static char *do_alias(struct alias_rule *alias, const char *target)
{
	int len = strlen(target) - strlen(alias->from) + strlen(alias->to);
	char *n = (char *) malloc(len + 1);
	if (!n) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		return NULL;
	}
	sprintf(n, "%s%s", alias->to, target + strlen(alias->from));
/*fprintf(stderr, "replaced alias: from: %s, to: %s, name: %s\n  %s\n", alias->from, alias->to, target, new);*/
	return n;
}

static Profile *target_prof;
static struct cod_entry *target_list;
static void process_entries(const void *nodep, VISIT value, int level unused)
{
	struct alias_rule **t = (struct alias_rule **) nodep;
	struct cod_entry *entry, *dup = NULL;
	int len;

	if (value == preorder || value == endorder)
		return;

	len = strlen((*t)->from);

	list_for_each(target_list, entry) {
		if ((entry->mode & AA_SHARED_PERMS) || entry->alias_ignore)
			continue;
		if (entry->name && strncmp((*t)->from, entry->name, len) == 0) {
			char *n = do_alias(*t, entry->name);
			if (!n)
				return;
			dup = copy_cod_entry(entry);
			free(dup->name);
			dup->name = n;
		}
		if (entry->link_name &&
		    strncmp((*t)->from, entry->link_name, len) == 0) {
			char *n = do_alias(*t, entry->link_name);
			if (!n)
				return;
			if (!dup)
				dup = copy_cod_entry(entry);
			free(dup->link_name);
			dup->link_name = n;
		}
		if (dup) {
			dup->alias_ignore = 1;
			/* adds to the front of the list, list iteratition
			 * will skip it
			 */
			entry->next = dup;

			dup = NULL;
		}
	}
}

static void process_name(const void *nodep, VISIT value, int level unused)
{
	struct alias_rule **t = (struct alias_rule **) nodep;
	Profile *prof = target_prof;
	char *name;
	int len;

	if (value == preorder || value == endorder)
		return;

	len = strlen((*t)->from);

	if (prof->attachment)
		name = prof->attachment;
	else
		name = prof->name;

	if (name && strncmp((*t)->from, name, len) == 0) {
		struct alt_name *alt;
		char *n = do_alias(*t, name);
		if (!n)
			return;
		/* aliases create alternate names */
		alt = (struct alt_name *) calloc(1, sizeof(struct alt_name));
		if (!alt) {
			free(n);
			return;
		}
		alt->name = n;
		alt->next = prof->altnames;
		prof->altnames = alt;
	}
}

int replace_profile_aliases(Profile *prof)
{
	target_prof = prof;
	twalk(alias_table, process_name);

	if (prof->entries) {
		target_list = prof->entries;
		target_prof = prof;
		twalk(alias_table, process_entries);
	}

	return 0;
}

static void free_alias(void *nodep)
{
	struct alias_rule *t = (struct alias_rule *)nodep;
	free(t->from);
	free(t->to);
	free(t);
}

void free_aliases(void)
{
	if (alias_table)
		tdestroy(alias_table, &free_alias);
	alias_table = NULL;
}
