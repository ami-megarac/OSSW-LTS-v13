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
#include <linux/limits.h>

#include "immunix.h"
#include "parser.h"

typedef int (*comparison_fn_t)(const void *, const void *);
typedef void (*__free_fn_t)(void *);

enum var_type {
	sd_boolean,
	sd_set,
};

struct symtab {
	char *var_name;
	enum var_type type;
	int boolean;
	struct set_value *values;
	struct set_value *expanded;
};

static void *my_symtab = NULL;

static int __expand_variable(struct symtab *symbol);

static struct symtab *new_symtab_entry(const char *name)
{
	struct symtab *n = (struct symtab *) calloc(1, sizeof(*n));

	if (!n) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		return NULL;
	}

	n->var_name = strndup(name, PATH_MAX);
	if (!n->var_name) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		free(n);
		return NULL;
	}

	return n;
}

static struct set_value *new_set_value(const char *val)
{
	struct set_value *n = (struct set_value *) calloc(1, sizeof(*n));

	if (!n) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		return NULL;
	}

	n->val = strndup(val, PATH_MAX);
	if (!n->val) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		free(n);
		return NULL;
	}

	return n;
}

static void free_values(struct set_value *val)
{
	struct set_value *i = val, *tmp;

	while (i) {
		if (i->val)
			free(i->val);
		tmp = i;
		i = i->next;
		free(tmp);
	}
}

static void free_symtab(struct symtab *symtab)
{
	if (!symtab)
		return;

	if (symtab->var_name)
	 	free(symtab->var_name);

	free_values(symtab->values);
	free_values(symtab->expanded);
	free(symtab);
}

/* abstract this out in case we switch data structures */
static void add_to_set(struct set_value **list, const char *val)
{
	struct set_value *new_item = new_set_value(val);

	new_item->next = *list;
	*list = new_item;
}

static int compare_symtabs(const void *a, const void *b)
{
	char *a_name = ((struct symtab *) a)->var_name;
	char *b_name = ((struct symtab *) b)->var_name;
	return strcmp(a_name, b_name);
}

static struct symtab *lookup_existing_symbol(const char *var)
{
	struct symtab *tmp, **lookup;
	struct symtab *result = NULL;

	tmp = new_symtab_entry(var);
	if (!tmp) {
		goto out;
	}

	lookup = (struct symtab **) tfind(tmp, &my_symtab, (comparison_fn_t) &compare_symtabs);
	if (!lookup) {
		goto out;
	}

	result = (*lookup);

out:
	free_symtab(tmp);
	return result;

}

/* add_boolean_var
 * creates copies of arguments, so caller can free them after use
 */

int add_boolean_var(const char *var, int value)
{
	struct symtab *n, **result;
	int rc = 0;

	n = new_symtab_entry(var);
	if (!n) {
		rc = ENOMEM;
		goto err;
	}

	n->type = sd_boolean;
	n->boolean = value;

	result = (struct symtab **) tsearch(n, &my_symtab, (comparison_fn_t) &compare_symtabs);
	if (!result) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		rc = errno;
		goto err;
	}

	if (*result != n) {
		/* already existing variable */
		PERROR("'%s' is already defined\n", var);
		rc = 1;
		goto err;
	}

	return 0;

err:
	free_symtab(n);
	return rc;
};

int get_boolean_var(const char *var)
{
	struct symtab *result;
	int rc = 0;

	result = lookup_existing_symbol(var);
	if (!result) {
		rc = -1;
		goto out;
	}

	if (result->type != sd_boolean) {
		PERROR("Variable %s is not a boolean variable\n", var);
		rc = -2; /* XXX - might change this to specific values */
		goto out;
	}

	rc = result->boolean;
out:
	return rc;
}

/* new_set_var
 * creates copies of arguments, so caller can free them after use
 */
int new_set_var(const char *var, const char *value)
{
	struct symtab *n, **result;
	int rc = 0;

	n = new_symtab_entry(var);
	if (!n) {
		rc = ENOMEM;
		goto err;
	}

	n->type = sd_set;
	add_to_set(&(n->values), value);

	result = (struct symtab **) tsearch(n, &my_symtab, (comparison_fn_t) &compare_symtabs);
	if (!result) {
		PERROR("Failed to allocate memory: %s\n", strerror(errno));
		rc = errno;
		goto err;
	}

	if (*result != n) {
		/* already existing variable */
		PERROR("'%s' is already defined\n", var);
		rc = 1;
		goto err;
	}

	return 0;

err:
	free_symtab(n);
	return rc;
}


/* add_set_value
 * creates copies of arguments, so caller can free them after use
 */
int add_set_value(const char *var, const char *value)
{
	struct symtab *result;
	int rc = 0;

	result = lookup_existing_symbol(var);
	if (!result) {
		PERROR("Failed to find declaration for: %s\n", var);
		rc = 1;
		goto out;
	}

	if (result->type != sd_set) {
		PERROR("Variable %s is not a set variable\n", var);
		rc = 2;	/* XXX - might change this to specific values */
		goto out;
	}

	if (strcmp(result->var_name, var) != 0) {
		PERROR("ASSERT: tfind found  %s when looking up variable %s\n",
		       result->var_name, var);
		exit(1);
	}

	add_to_set(&(result->values), value);

out:
	return rc;
}

/* returns a pointer to the value list, which should be used as the
 * argument to the get_next_set_value() function. */
struct set_value *get_set_var(const char *var)
{
	struct symtab *result;
	struct set_value *valuelist = NULL;

	result = lookup_existing_symbol(var);
	if (!result) {
		goto out;
	}

	if (result->type != sd_set) {
		goto out;
	}

	if (strcmp(result->var_name, var) != 0) {
		PERROR("ASSERT: tfind found  %s when looking up variable %s\n",
		       result->var_name, var);
		exit(1);
	}

	if (!result->expanded) {
		int err = __expand_variable(result);
		if (err) {
			PERROR("failure expanding variable %s\n", var);
			exit(1);
		}
	}
	valuelist = result->expanded;
out:
	return valuelist;
}

/* iterator to walk the list of set values */
char *get_next_set_value(struct set_value **list)
{
	struct set_value *next;
	char *ret;

	if (!list || !(*list))
		return NULL;

	ret = (*list)->val;
	next = (*list)->next;
	(*list) = next;

	return ret;
}

/* delete_symbol
 * removes an individual variable from the symbol table. We don't
 * support this in the language, but for special variables that change
 * between profiles, we need this.
 */
int delete_set_var(const char *var_name)
{
	int rc = 0;
	struct symtab **result, *n, *var;

	n = new_symtab_entry(var_name);
	if (!n) {
		rc = ENOMEM;
		goto out;
	}

	result = (struct symtab **) tfind(n, &my_symtab, (comparison_fn_t) &compare_symtabs);
	if (!result) {
		/* XXX Warning? */
		goto out;
	}

	var = (*result);

	result = (struct symtab **) tdelete(n, &my_symtab, (comparison_fn_t) &compare_symtabs);
	if (!result) {
		PERROR("ASSERT: delete_set_var: tfind found var %s but tdelete failed to delete it\n",
				var_name);
		exit(1);
	}

	if (var->type != sd_set) {
		PERROR("ASSERT: delete_set_var: deleting %s but is a boolean variable\n",
				var_name);
		exit(1);
	}

	free_symtab(var);

out:
	free_symtab(n);
	return rc;
}

static void *seenlist = NULL;

static int is_seen(const char *var)
{
	char **lookup;
	lookup = (char **) tfind(var, &seenlist, (comparison_fn_t) &strcmp);
	return (lookup != NULL);
}

static void push_seen_var(const char *var)
{
	char **lookup;
	lookup = (char **) tsearch(var, &seenlist, (comparison_fn_t) &strcmp);
	if (*lookup != var) {
		PERROR("ASSERT: '%s' is already in the seenlist\n", var);
		exit(1);
	}
}

static void pop_seen_var(const char *var)
{
	char **lookup;
	lookup = (char **) tdelete(var, &seenlist, (comparison_fn_t) &strcmp);
	if (lookup == NULL) {
		PERROR("ASSERT: popped var '%s' not found on the seenlist\n", var);
		exit(1);
	}
}

static int __expand_variable(struct symtab *symbol)
{
	struct set_value *list, *expanded = NULL;
	int retval = 0;
	struct var_string *split = NULL;

	if (symbol->type == sd_boolean) {
		PERROR("Referenced variable %s is a boolean used in set context\n",
		       symbol->var_name);
		return 2;
	}

	/* already done */
	if (symbol->expanded)
		return 0;

	push_seen_var(symbol->var_name);

	for (list = symbol->values; list; list = list->next) {
		struct set_value *work_list = new_set_value(list->val);
		while (work_list) {
			struct symtab *ref;
			struct set_value *ref_item;
			struct set_value *t_value = work_list;
			int rc;

			work_list = work_list->next;

			split = split_out_var(t_value->val);
			if (!split) {
				/* fully expanded */
				add_to_set(&expanded, t_value->val);
				goto next;
			}


			if (is_seen(split->var)) {
				PERROR("Variable @{%s} is referenced recursively (by @{%s})\n",
		       		       split->var, symbol->var_name);
				retval = 1;
				free_values(t_value);
				goto out;
			}

			ref = lookup_existing_symbol(split->var);
			if (!ref) {
				PERROR("Variable @{%s} references undefined variable @{%s}\n",
				       symbol->var_name, split->var);
				retval = 3;
				free_values(t_value);
				goto out;
			}

			rc = __expand_variable(ref);
			if (rc != 0) {
				retval = rc;
				free_values(t_value);
				goto out;
			}

			if (!ref->expanded) {
				PERROR("ASSERT: Variable @{%s} should have been expanded but isn't\n",
				       split->var);
				exit(1);
			}

			for (ref_item = ref->expanded; ref_item; ref_item = ref_item->next) {
				char *expanded_string;
				if (!asprintf(&expanded_string, "%s%s%s",
					 split->prefix ?  split->prefix : "",
					 ref_item->val,
					 split->suffix ?  split->suffix : "")) {
					PERROR("Out of memory\n");
					exit(1);
				}
				add_to_set(&work_list, expanded_string);
				free(expanded_string);
			}

next:
			t_value->next = NULL;
			free_values(t_value);
			free_var_string(split);
		}
	}

	symbol->expanded = expanded;

out:
	pop_seen_var(symbol->var_name);
	free_var_string(split);
	return retval;
}

static void expand_variable(const void *nodep, VISIT value, int level unused)
{
	struct symtab **t = (struct symtab **) nodep;

	if (value == preorder || value == endorder)
		return;

	if ((*t)->type == sd_boolean)
		return;

	__expand_variable(*t);
}

void expand_variables(void)
{
	twalk(my_symtab, &expand_variable);
}

static inline void dump_set_values(struct set_value *value)
{
	struct set_value *t = value;
	while (t) {
		printf(" \"%s\"", t->val);
		t = t->next;
	}
}

static void __dump_symtab_entry(struct symtab *entry, int do_expanded)
{
	switch (entry->type) {
	case sd_boolean:
		printf("$%s = %s\n", entry->var_name,
		       entry->boolean ?  "true" : "false");
		break;
	case sd_set:
		printf("@%s =", entry->var_name);
		if (do_expanded) {
			if (!entry->expanded) {
				__expand_variable(entry);
			}
			dump_set_values(entry->expanded);
		} else {
			dump_set_values(entry->values);
		}
		printf("\n");
		break;
	default:
		PERROR("ASSERT: unknown symbol table type for %s\n", entry->var_name);
		exit(1);
	}
}

static void dump_symtab_entry(const void *nodep, VISIT value, int level unused)
{
	struct symtab **t = (struct symtab **) nodep;

	if (value == preorder || value == endorder)
		return;

	__dump_symtab_entry(*t, 0);
}

static void dump_expanded_symtab_entry(const void *nodep, VISIT value, int level unused)
{
	struct symtab **t = (struct symtab **) nodep;

	if (value == preorder || value == endorder)
		return;

	__dump_symtab_entry(*t, 1);
}

void dump_symtab(void)
{
	twalk(my_symtab, &dump_symtab_entry);
}

void dump_expanded_symtab(void)
{
	twalk(my_symtab, &dump_expanded_symtab_entry);
}

void free_symtabs(void)
{
	if (my_symtab)
		tdestroy(my_symtab, (__free_fn_t)&free_symtab);
	my_symtab = NULL;
}

#ifdef UNIT_TEST

#include "unit_test.h"

int test_compare_symtab(void)
{
	int rc = 0;
	int retval;
	struct symtab *a, *b, *c;

	a = new_symtab_entry("blah");
	b = new_symtab_entry("suck");
	MY_TEST(a && b, "allocation test");

	retval = compare_symtabs(a, b);
	MY_TEST(retval < 0, "comparison 1");

	retval = compare_symtabs(b, a);
	MY_TEST(retval > 0, "comparison 2");

	retval = compare_symtabs(b, a);
	MY_TEST(retval != 0, "comparison 3");

	retval = compare_symtabs(b, b);
	MY_TEST(retval == 0, "comparison 4");

	c = new_symtab_entry("blah");
	retval = compare_symtabs(a, c);
	MY_TEST(retval == 0, "comparison 5");

	free_symtab(a);
	free_symtab(b);
	free_symtab(c);

	return rc;
}

int test_seenlist(void)
{
	int rc = 0;

	MY_TEST(!is_seen("oogabooga"), "lookup unseen variable");

	push_seen_var("oogabooga");
	MY_TEST(is_seen("oogabooga"), "lookup seen variable 1");
	MY_TEST(!is_seen("not_seen"), "lookup unseen variable 2");

	push_seen_var("heebiejeebie");
	MY_TEST(is_seen("oogabooga"), "lookup seen variable 2");
	MY_TEST(is_seen("heebiejeebie"), "lookup seen variable 3");
	MY_TEST(!is_seen("not_seen"), "lookup unseen variable 3");

	pop_seen_var("oogabooga");
	MY_TEST(!is_seen("oogabooga"), "lookup unseen variable 4");
	MY_TEST(is_seen("heebiejeebie"), "lookup seen variable 4");
	MY_TEST(!is_seen("not_seen"), "lookup unseen variable 5");

	pop_seen_var("heebiejeebie");
	MY_TEST(!is_seen("heebiejeebie"), "lookup unseen variable 6");

	//pop_seen_var("not_seen"); /* triggers assert */

	return rc;
}

int test_add_set_to_boolean(void)
{
	int rc = 0;
	int retval;

	/* test adding a set value to a boolean variable */
	retval = add_boolean_var("not_a_set_variable", 1);
	MY_TEST(retval == 0, "new boolean variable 3");
	retval = add_set_value("not_a_set_variable", "a set value");
	MY_TEST(retval != 0, "add set value to boolean");

	free_symtabs();

	return rc;
}

int test_expand_bool_within_set(void)
{
	int rc = 0;
	int retval;
	struct symtab *retsym;

	/* test expanding a boolean var within a set variable */
	retval = add_boolean_var("not_a_set_variable", 1);
	MY_TEST(retval == 0, "new boolean variable 4");
	retval = new_set_var("set_variable", "set_value@{not_a_set_variable}");
	MY_TEST(retval == 0, "add set value with embedded boolean");
	retsym = lookup_existing_symbol("set_variable");
	MY_TEST(retsym != NULL, "get set variable w/boolean");
	retval = __expand_variable(retsym);
	MY_TEST(retval != 0, "expand set variable with embedded boolean");

	free_symtabs();

	return rc;
}

int test_expand_recursive_set_vars(void)
{
	int rc = 0;
	int retval;
	struct symtab *retsym;

	/* test expanding a recursive var within a set variable */
	retval = new_set_var("recursive_1", "set_value@{recursive_2}");
	MY_TEST(retval == 0, "new recursive set variable 1");
	retval = new_set_var("recursive_2", "set_value@{recursive_3}");
	MY_TEST(retval == 0, "new recursive set variable 2");
	retval = new_set_var("recursive_3", "set_value@{recursive_1}");
	MY_TEST(retval == 0, "new recursive set variable 3");
	retsym = lookup_existing_symbol("recursive_1");
	MY_TEST(retsym != NULL, "get recursive set variable");
	retval = __expand_variable(retsym);
	MY_TEST(retval != 0, "expand recursive set variable");

	free_symtabs();

	return rc;
}

int test_expand_undefined_set_var(void)
{
	int rc = 0;
	int retval;
	struct symtab *retsym;

	/* test expanding an undefined var within a set variable */
	retval = new_set_var("defined_var", "set_value@{undefined_var}");
	MY_TEST(retval == 0, "new undefined test set variable");
	retsym = lookup_existing_symbol("defined_var");
	MY_TEST(retsym != NULL, "get undefined test set variable");
	retval = __expand_variable(retsym);
	MY_TEST(retval != 0, "expand undefined set variable");

	free_symtabs();

	return rc;
}

int test_expand_set_var_during_dump(void)
{
	int rc = 0;
	int retval;
	struct symtab *retsym;

	/* test expanding an defined var within a set variable during var dump*/
	retval = new_set_var("set_var_1", "set_value@{set_var_2}");
	MY_TEST(retval == 0, "new dump expansion set variable 1");
	retval = new_set_var("set_var_2", "some other set_value");
	MY_TEST(retval == 0, "new dump expansion set variable 2");
	retsym = lookup_existing_symbol("set_var_1");
	MY_TEST(retsym != NULL, "get dump expansion set variable 1");
	__dump_symtab_entry(retsym, 0);
	__dump_symtab_entry(retsym, 1);
	__dump_symtab_entry(retsym, 0);

	free_symtabs();

	return rc;
}

int test_delete_set_var(void)
{
	int rc = 0;
	int retval;

	retval = new_set_var("deleteme", "delete this variable");
	MY_TEST(retval == 0, "new delete set variable");
	retval = delete_set_var("deleteme");
	MY_TEST(retval == 0, "delete set variable");

	free_symtabs();

	return rc;
}

int main(void)
{
	int rc = 0;
	int retval;
	struct set_value *retptr;

	rc = test_compare_symtab();

	retval = test_seenlist();
	if (rc == 0)
		rc = retval;

	retval = test_add_set_to_boolean();
	if (rc == 0)
		rc = retval;

	retval = test_expand_bool_within_set();
	if (rc == 0)
		rc = retval;

	retval = test_expand_recursive_set_vars();
	if (rc == 0)
		rc = retval;

	retval = test_expand_undefined_set_var();
	if (rc == 0)
		rc = retval;

	retval = test_expand_set_var_during_dump();
	if (rc == 0)
		rc = retval;

	retval = test_delete_set_var();
	if (rc == 0)
		rc = retval;

	retval = new_set_var("test", "test value");
	MY_TEST(retval == 0, "new set variable 1");

	retval = new_set_var("test", "different value");
	MY_TEST(retval != 0, "new set variable 2");

	retval = new_set_var("testing", "testing");
	MY_TEST(retval == 0, "new set variable 3");

	retval = new_set_var("monopuff", "Mockingbird");
	MY_TEST(retval == 0, "new set variable 4");

	retval = new_set_var("stereopuff", "Unsupervised");
	MY_TEST(retval == 0, "new set variable 5");

	retval = add_set_value("stereopuff", "Fun to Steal");
	MY_TEST(retval == 0, "add set value 1");

	retval = add_set_value("stereopuff", "/in/direction");
	MY_TEST(retval == 0, "add set value 2");

	retval = add_set_value("no_such_variable", "stereopuff");
	MY_TEST(retval != 0, "add to non-existent set var");

	retval = add_boolean_var("abuse", 0);
	MY_TEST(retval == 0, "new boolean variable 1");

	retval = add_boolean_var("abuse", 1);
	MY_TEST(retval != 0, "duplicate boolean variable 1");

	retval = add_boolean_var("stereopuff", 1);
	MY_TEST(retval != 0, "duplicate boolean variable 2");

	retval = add_boolean_var("shenanigan", 1);
	MY_TEST(retval == 0, "new boolean variable 2");

	retval = get_boolean_var("shenanigan");
	MY_TEST(retval == 1, "get boolean variable 1");

	retval = get_boolean_var("abuse");
	MY_TEST(retval == 0, "get boolean variable 2");

	retval = get_boolean_var("non_existant");
	MY_TEST(retval < 0, "get nonexistant boolean variable");

	retval = get_boolean_var("stereopuff");
	MY_TEST(retval < 0, "get boolean variable that's declared a set var");

	retptr = get_set_var("daves_not_here_man");
	MY_TEST(retptr == NULL, "get non-existent set variable");

	retptr = get_set_var("abuse");
	MY_TEST(retptr == NULL, "get set variable that's declared a boolean");

	/* test walking set values */
	retptr = get_set_var("monopuff");
	MY_TEST(retptr != NULL, "get set variable 1");
	retval = strcmp(get_next_set_value(&retptr), "Mockingbird");
	MY_TEST(retval == 0, "get set value 1");
	MY_TEST(get_next_set_value(&retptr) == NULL, "get no more set values 1");

	retval = new_set_var("eek", "Mocking@{monopuff}bir@{stereopuff}d@{stereopuff}");
	MY_TEST(retval == 0, "new set variable 4");

	dump_symtab();
	expand_variables();
	dump_symtab();
	dump_expanded_symtab();

	free_symtabs();

	return rc;
}
#endif /* UNIT_TEST */
