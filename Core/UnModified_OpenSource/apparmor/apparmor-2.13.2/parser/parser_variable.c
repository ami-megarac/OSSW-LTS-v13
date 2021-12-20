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

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <linux/limits.h>

#include <string>

/* #define DEBUG */

#include "parser.h"
#include "profile.h"
#include "mount.h"
#include "dbus.h"

static inline const char *get_var_end(const char *var)
{
	const char *eptr = var;

	while (*eptr) {
		if (*eptr == '}')
			return eptr;
		/* first character must be alpha */
		if (eptr == var) {
		 	if (!isalpha(*eptr))
				return NULL; /* invalid char */
		} else {
			if (!(*eptr == '_' || isalnum(*eptr)))
				return NULL; /* invalid char */
		}
		eptr++;
	}
	return NULL; /* no terminating '}' */
}

static struct var_string *split_string(const char *string, const char *var_begin,
				       const char *var_end)
{
	struct var_string *n = (struct var_string *) calloc(1, sizeof(struct var_string));
	unsigned int offset = strlen("@{");
	if (!n) {
		PERROR("Memory allocation error\n");
		return NULL;
	}

	if (var_begin != string) {
		n->prefix = strndup(string, var_begin - string);
	}

	n->var = strndup(var_begin + offset, var_end - (var_begin + offset));

	if (strlen(var_end + 1) != 0) {
		n->suffix = strdup(var_end + 1);
	}

	return n;
}

struct var_string *split_out_var(const char *string)
{
	struct var_string *n = NULL;
	const char *sptr;
	BOOL bEscape = 0;	/* flag to indicate escape */

	if (!string) 		/* shouldn't happen */
		return NULL;

	sptr = string;

	while (!n && *sptr) {
		switch (*sptr) {
		case '\\':
			if (bEscape) {
				bEscape = FALSE;
			} else {
				bEscape = TRUE;
			}
			break;
		case '@':
			if (bEscape) {
				bEscape = FALSE;
			} else if (*(sptr + 1) == '{') {
				const char *eptr = get_var_end(sptr + 2);
				if (!eptr)
					break; /* no variable end found */
				if (eptr == sptr + 2) {
					/* XXX - better diagnostics here, please */
					PERROR("Empty variable name found!\n");
					exit(1);
				}
				n = split_string(string, sptr, eptr);
			}
			break;
		default:
			if (bEscape)
				bEscape = FALSE;
		}
		sptr++;
	}

	return n;
}

void free_var_string(struct var_string *var)
{
	if (!var)
		return;
	if (var->prefix)
		free(var->prefix);
	if (var->var)
		free(var->var);
	if (var->suffix)
		free(var->suffix);
	free(var);
}

static void trim_trailing_slash(std::string& str)
{
	std::size_t found = str.find_last_not_of('/');
	if (found != std::string::npos)
		str.erase(found + 1);
	else
		str.clear(); // str is all '/'
}

static void write_replacement(const char separator, const char* value,
			     std::string& replacement, bool filter_leading_slash,
			     bool filter_trailing_slash)
{
	const char *p = value;

	replacement.append(1, separator);

	if (filter_leading_slash)
		while (*p == '/')
			p++;

	replacement.append(p);
	if (filter_trailing_slash)
		trim_trailing_slash(replacement);
}

static int expand_by_alternations(struct set_value **valuelist,
				  struct var_string *split_var,
				  char **name)
{
	char *value, *first_value;
	std::string replacement;
	bool filter_leading_slash = false;
	bool filter_trailing_slash = false;

	first_value = get_next_set_value(valuelist);
	if (!first_value) {
		PERROR("ASSERT: set variable (%s) should always have at least one value assigned to it\n",
		       split_var->var);
		exit(1);
	}

	free(*name);

	value = get_next_set_value(valuelist);
	if (!value) {
		/* only one entry for the variable, so just sub it in */
		if (asprintf(name, "%s%s%s",
			     split_var->prefix ? split_var->prefix : "",
			     first_value,
			     split_var->suffix ? split_var->suffix : "") == -1)
			return -1;
		return 0;
	}

	if (split_var->prefix && split_var->prefix[strlen(split_var->prefix) - 1] == '/')
		filter_leading_slash = true;
	if (split_var->suffix && *split_var->suffix == '/')
		filter_trailing_slash = true;

	write_replacement('{', first_value, replacement, filter_leading_slash, filter_trailing_slash);
	write_replacement(',', value, replacement, filter_leading_slash, filter_trailing_slash);

	while ((value = get_next_set_value(valuelist))) {
		write_replacement(',', value, replacement, filter_leading_slash, filter_trailing_slash);
	}

	if (asprintf(name, "%s%s}%s",
		     split_var->prefix ? split_var->prefix : "",
		     replacement.c_str(),
		     split_var->suffix ? split_var->suffix : "") == -1) {
			return -1;
	}

	return 0;
}

/* doesn't handle variables in options atm */
int expand_entry_variables(char **name)
{
	struct set_value *valuelist;
	struct var_string *split_var;
	int ret;

	assert(name);

	if (!*name) 		/* can happen when entry is optional */
		return 0;

	while ((split_var = split_out_var(*name))) {
		valuelist = get_set_var(split_var->var);
		if (!valuelist) {
			int boolean = get_boolean_var(split_var->var);
			if (boolean == -1)
				PERROR("Found reference to variable %s, but is never declared\n",
				       split_var->var);
			else
				PERROR("Found reference to set variable %s, but declared boolean\n",
				       split_var->var);
			exit(1);
		}

		ret = expand_by_alternations(&valuelist, split_var, name);

		free_var_string(split_var);
		if (ret != 0)
			return -1;

	}
	return 0;
}

static int process_variables_in_entries(struct cod_entry *entry_list)
{
	int error = 0;
	struct cod_entry *entry;

	list_for_each(entry_list, entry) {
		error = expand_entry_variables(&entry->name);
		if (error)
			return error;
		if (entry->link_name) {
			error = expand_entry_variables(&entry->link_name);
			if (error)
				return error;
		}
	}

	return 0;
}

static int process_variables_in_rules(Profile &prof)
{
	for (RuleList::iterator i = prof.rule_ents.begin(); i != prof.rule_ents.end(); i++) {
	  int error = (*i)->expand_variables();
		if (error)
			return error;
	}

	return 0;
}

static int process_variables_in_name(Profile &prof)
{
	/* this needs to be done before alias expansion, ie. altnames are
	 * setup
	 */
	int error = expand_entry_variables(&prof.name);
	if (!error && prof.attachment)
		error = expand_entry_variables(&prof.attachment);

	return error;
}

static std::string escape_re(std::string str)
{
	for (size_t i = 0; i < str.length(); i++) {
		if (str[i] == '\\') {
			/* skip \ and follow char. Skipping \ and first
			 * char is enough for multichar escape sequence
			 */
			i++;
			continue;
		}
		if (strchr("{}[]*?", str[i]) != NULL) {
			str.insert(i++, "\\");
		}
	}

	return str;
}

int process_profile_variables(Profile *prof)
{
	int error = 0, rc;

	/* needs to be before PROFILE_NAME_VARIABLE so that variable will
	 * have the correct name
	 */
	error = process_variables_in_name(*prof);

	if (!error) {
		/* escape profile name elements that could be interpreted
		 * as regular expressions.
		 */
		error = new_set_var(PROFILE_NAME_VARIABLE, escape_re(prof->get_name(false)).c_str());
	}

	if (!error)
		error = process_variables_in_entries(prof->entries);

	if (!error)
		error = process_variables_in_rules(*prof);

	rc = delete_set_var(PROFILE_NAME_VARIABLE);
	if (!error)
		error = rc;

	return error;
}

#ifdef UNIT_TEST

#include "unit_test.h"

int test_get_var_end(void)
{
	int rc = 0;
	const char *retchar;
	const char *testchar;

	testchar = "TRUE}";
	retchar = get_var_end(testchar);
	MY_TEST(retchar - testchar == strlen("TRUE"), "get var end for TRUE}");

	testchar = "some_var}some other text";
	retchar = get_var_end(testchar);
	MY_TEST(retchar - testchar == strlen("some_var"), "get var end for some_var}");

	testchar = "some_var}some other} text";
	retchar = get_var_end(testchar);
	MY_TEST(retchar - testchar == strlen("some_var"), "get var end for some_var} 2");

	testchar = "FALSE";
	retchar = get_var_end(testchar);
	MY_TEST(retchar == NULL, "get var end for FALSE");

	testchar = "pah,pah}pah ";
	retchar = get_var_end(testchar);
	MY_TEST(retchar == NULL, "get var end for pah,pah}");

	return rc;
}

int test_split_string(void)
{
	int rc = 0;
	char *tst_string, *var_start, *var_end;
	struct var_string *ret_struct;
	const char *prefix = "abcdefg";
	const char *var = "boogie";
	const char *suffix = "suffixication";

	asprintf(&tst_string, "%s@{%s}%s", prefix, var, suffix);
	var_start = tst_string + strlen(prefix);
	var_end = var_start + strlen(var) + strlen("@\{");
	ret_struct = split_string(tst_string, var_start, var_end);
	MY_TEST(strcmp(ret_struct->prefix, prefix) == 0, "split string 1 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split string 1 var");
	MY_TEST(strcmp(ret_struct->suffix, suffix) == 0, "split string 1 suffix");
	free_var_string(ret_struct);
	free(tst_string);

	asprintf(&tst_string, "@{%s}%s", var, suffix);
	var_start = tst_string;
	var_end = var_start + strlen(var) + strlen("@\{");
	ret_struct = split_string(tst_string, var_start, var_end);
	MY_TEST(ret_struct->prefix == NULL, "split string 2 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split string 2 var");
	MY_TEST(strcmp(ret_struct->suffix, suffix) == 0, "split string 2 suffix");
	free_var_string(ret_struct);
	free(tst_string);

	asprintf(&tst_string, "%s@{%s}", prefix, var);
	var_start = tst_string + strlen(prefix);
	var_end = var_start + strlen(var) + strlen("@\{");
	ret_struct = split_string(tst_string, var_start, var_end);
	MY_TEST(strcmp(ret_struct->prefix, prefix) == 0, "split string 3 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split string 3 var");
	MY_TEST(ret_struct->suffix == NULL, "split string 3 suffix");
	free_var_string(ret_struct);
	free(tst_string);

	asprintf(&tst_string, "@{%s}", var);
	var_start = tst_string;
	var_end = var_start + strlen(var) + strlen("@\{");
	ret_struct = split_string(tst_string, var_start, var_end);
	MY_TEST(ret_struct->prefix == NULL, "split string 4 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split string 4 var");
	MY_TEST(ret_struct->suffix == NULL, "split string 4 suffix");
	free_var_string(ret_struct);
	free(tst_string);

	return rc;
}

int test_split_out_var(void)
{
	int rc = 0;
	char *tst_string, *tmp;
	struct var_string *ret_struct;
	const char *prefix = "abcdefg";
	const char *var = "boogie";
	const char *var2 = "V4rW1thNum5";
	const char *var3 = "boogie_board";
	const char *suffix = "suffixication";

	/* simple case */
	asprintf(&tst_string, "%s@{%s}%s", prefix, var, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(strcmp(ret_struct->prefix, prefix) == 0, "split out var 1 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split out var 1 var");
	MY_TEST(strcmp(ret_struct->suffix, suffix) == 0, "split out var 1 suffix");
	free_var_string(ret_struct);
	free(tst_string);

	/* no prefix */
	asprintf(&tst_string, "@{%s}%s", var, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct->prefix == NULL, "split out var 2 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split out var 2 var");
	MY_TEST(strcmp(ret_struct->suffix, suffix) == 0, "split out var 2 suffix");
	free_var_string(ret_struct);
	free(tst_string);

	/* no suffix */
	asprintf(&tst_string, "%s@{%s}", prefix, var);
	ret_struct = split_out_var(tst_string);
	MY_TEST(strcmp(ret_struct->prefix, prefix) == 0, "split out var 3 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split out var 3 var");
	MY_TEST(ret_struct->suffix == NULL, "split out var 3 suffix");
	free_var_string(ret_struct);
	free(tst_string);

	/* var only */
	asprintf(&tst_string, "@{%s}", var);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct->prefix == NULL, "split out var 4 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split out var 4 var");
	MY_TEST(ret_struct->suffix == NULL, "split out var 4 suffix");
	free_var_string(ret_struct);
	free(tst_string);

	/* quoted var, shouldn't split  */
	asprintf(&tst_string, "%s\\@{%s}%s", prefix, var, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct == NULL, "split out var - quoted @");
	free_var_string(ret_struct);
	free(tst_string);

	/* quoted \, split should succeed */
	asprintf(&tst_string, "%s\\\\@{%s}%s", prefix, var, suffix);
	ret_struct = split_out_var(tst_string);
	tmp = strndup(tst_string, strlen(prefix) + 2);
	MY_TEST(strcmp(ret_struct->prefix, tmp) == 0, "split out var 5 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split out var 5 var");
	MY_TEST(strcmp(ret_struct->suffix, suffix) == 0, "split out var 5 suffix");
	free_var_string(ret_struct);
	free(tst_string);
	free(tmp);

	/* un terminated var, should fail */
	asprintf(&tst_string, "%s@{%s%s", prefix, var, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct == NULL, "split out var - un-terminated var");
	free_var_string(ret_struct);
	free(tst_string);

	/* invalid char in var, should fail */
	asprintf(&tst_string, "%s@{%s^%s}%s", prefix, var, var, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct == NULL, "split out var - invalid char in var");
	free_var_string(ret_struct);
	free(tst_string);

	/* two vars, should only strip out first */
	asprintf(&tmp, "@{%s}%s}", suffix, suffix);
	asprintf(&tst_string, "%s@{%s}%s", prefix, var, tmp);
	ret_struct = split_out_var(tst_string);
	MY_TEST(strcmp(ret_struct->prefix, prefix) == 0, "split out var 6 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split out var 6 var");
	MY_TEST(strcmp(ret_struct->suffix, tmp) == 0, "split out var 6 suffix");
	free_var_string(ret_struct);
	free(tst_string);
	free(tmp);

	/* quoted @ followed by var, split should succeed */
	asprintf(&tst_string, "%s\\@@{%s}%s", prefix, var, suffix);
	ret_struct = split_out_var(tst_string);
	tmp = strndup(tst_string, strlen(prefix) + 2);
	MY_TEST(strcmp(ret_struct->prefix, tmp) == 0, "split out var 7 prefix");
	MY_TEST(strcmp(ret_struct->var, var) == 0, "split out var 7 var");
	MY_TEST(strcmp(ret_struct->suffix, suffix) == 0, "split out var 7 suffix");
	free_var_string(ret_struct);
	free(tst_string);
	free(tmp);

	/* numeric char in var, should succeed */
	asprintf(&tst_string, "%s@{%s}%s", prefix, var2, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct && strcmp(ret_struct->prefix, prefix) == 0, "split out numeric var prefix");
	MY_TEST(ret_struct && strcmp(ret_struct->var, var2) == 0, "split numeric var var");
	MY_TEST(ret_struct && strcmp(ret_struct->suffix, suffix) == 0, "split out numeric var suffix");
	free_var_string(ret_struct);
	free(tst_string);

	/* numeric first char in var, should fail */
	asprintf(&tst_string, "%s@{6%s}%s", prefix, var2, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct == NULL, "split out var - numeric first char in var");
	free_var_string(ret_struct);
	free(tst_string);

	/* underscore char in var, should succeed */
	asprintf(&tst_string, "%s@{%s}%s", prefix, var3, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct && strcmp(ret_struct->prefix, prefix) == 0, "split out underscore var prefix");
	MY_TEST(ret_struct && strcmp(ret_struct->var, var3) == 0, "split out underscore var");
	MY_TEST(ret_struct && strcmp(ret_struct->suffix, suffix) == 0, "split out underscore var suffix");
	free_var_string(ret_struct);
	free(tst_string);

	/* underscore first char in var, should fail */
	asprintf(&tst_string, "%s@{_%s%s}%s", prefix, var2, var3, suffix);
	ret_struct = split_out_var(tst_string);
	MY_TEST(ret_struct == NULL, "split out var - underscore first char in var");
	free_var_string(ret_struct);
	free(tst_string);

	return rc;
}
int main(void)
{
	int rc = 0;
	int retval;

	retval = test_get_var_end();
	if (retval != 0)
		rc = retval;

	retval = test_split_string();
	if (retval != 0)
		rc = retval;

	retval = test_split_out_var();
	if (retval != 0)
		rc = retval;

	return rc;
}
#endif /* UNIT_TEST */
