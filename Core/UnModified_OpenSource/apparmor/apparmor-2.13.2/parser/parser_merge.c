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

#include <linux/unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "parser.h"
#include "profile.h"

static int file_comp(const void *c1, const void *c2)
{
	struct cod_entry **e1, **e2;
	e1 = (struct cod_entry **)c1;
	e2 = (struct cod_entry **)c2;
	int res = 0;

	if ((*e1)->link_name) {
		if ((*e2)->link_name)
			res = strcmp((*e1)->link_name, (*e2)->link_name);
		else
			return 1;
	} else if ((*e2)->link_name) {
		return -1;
	}
	if (res)
		return res;

	if ((*e1)->link_name)
		res = (*e2)->subset - (*e1)->subset;
	if (res)
		return res;

	if ((*e1)->deny != (*e2)->deny)
		return (*e1)->deny < (*e2)->deny ? -1 : 1;

	return strcmp((*e1)->name, (*e2)->name);
}

static int process_file_entries(Profile *prof)
{
	struct cod_entry *cur, *next;
	struct cod_entry **table;
	int n, count = 0;

	for (cur = prof->entries; cur; cur = cur->next)
		count++;

	if (count < 2)
		return 0;

	table = (struct cod_entry **) malloc(sizeof(struct cod_entry *) * (count + 1));
	if (!table) {
		PERROR(_("Couldn't merge entries. Out of Memory\n"));
		return ENOMEM;
	}

	for (cur = prof->entries, n = 0; cur; cur = cur->next, n++)
		table[n] = cur;
	qsort(table, count, sizeof(struct cod_entry *), file_comp);
	table[count] = NULL;
	for (n = 0; n < count; n++)
		table[n]->next = table[n + 1];
	prof->entries = table[0];
	free(table);

	/* walk the sorted table merging similar entries */
	for (cur = prof->entries, next = cur->next; next; next = cur->next) {
		if (file_comp(&cur, &next) != 0) {
			cur = next;
			continue;
		}

		/* check for merged x consistency */
		if (!is_merged_x_consistent(cur->mode, next->mode)) {
			PERROR(_("profile %s: has merged rule %s with conflicting x modifiers\n"),
				prof->name, cur->name);
			return -1;
		}
		cur->mode |= next->mode;
		cur->audit |= next->audit;
		cur->next = next->next;

		next->next = NULL;
		free_cod_entries(next);
	}

	return 0;
}

int profile_merge_rules(Profile *prof)
{
  return process_file_entries(prof);
}
