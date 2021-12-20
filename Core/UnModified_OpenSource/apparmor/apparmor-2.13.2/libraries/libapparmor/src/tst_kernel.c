/*
 *   Copyright (c) 2015
 *   Canonical, Ltd. (All rights reserved)
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
 *   along with this program; if not, contact Novell, Inc. or Canonical
 *   Ltd.
 */

#include <stdio.h>
#include <string.h>

#include "kernel.c"

static int nullcmp_and_strcmp(const void *s1, const void *s2)
{
	/* Return 0 if both pointers are NULL & non-zero if only one is NULL */
	if (!s1 || !s2)
		return s1 != s2;

	return strcmp(s1, s2);
}

static int do_test_splitcon(char *con, int size, bool strip_nl, char **mode,
			    const char *expected_label,
			    const char *expected_mode, const char *error)
{
	char *label;
	int rc = 0;

	label = splitcon(con, size, strip_nl, mode);

	if (nullcmp_and_strcmp(label, expected_label)) {
		fprintf(stderr, "FAIL: %s: label \"%s\" != \"%s\"\n",
			error, label, expected_label);
		rc = 1;
	}

	if (mode && nullcmp_and_strcmp(*mode, expected_mode)) {
		fprintf(stderr, "FAIL: %s: mode \"%s\" != \"%s\"\n",
			error, *mode, expected_mode);
		rc = 1;
	}

	return rc;
}

static int do_test_aa_splitcon(char *con, char **mode,
			       const char *expected_label,
			       const char *expected_mode, const char *error)
{
	char *label;
	int rc = 0;

	label = aa_splitcon(con, mode);

	if (nullcmp_and_strcmp(label, expected_label)) {
		fprintf(stderr, "FAIL: %s: label \"%s\" != \"%s\"\n",
			error, label, expected_label);
		rc = 1;
	}

	if (mode && nullcmp_and_strcmp(*mode, expected_mode)) {
		fprintf(stderr, "FAIL: %s: mode \"%s\" != \"%s\"\n",
			error, *mode, expected_mode);
		rc = 1;
	}

	return rc;
}

#define TEST_SPLITCON(con, size, strip_nl, expected_label,		\
		      expected_mode, error)				\
	do {								\
		char c1[] = con;					\
		char c2[] = con;					\
		size_t sz = size < 0 ? strlen(con) : size;		\
		char *mode;						\
									\
		if (do_test_splitcon(c1, sz, strip_nl, &mode,		\
				expected_label, expected_mode,		\
				"splitcon: " error)) {			\
			rc = 1;						\
		} else if (do_test_splitcon(c2, sz, strip_nl, NULL,	\
				expected_label, NULL,			\
				"splitcon: " error " (NULL mode)")) {	\
			rc = 1;						\
		}							\
	} while (0)

#define TEST_AA_SPLITCON(con, expected_label, expected_mode, error)	\
	do {								\
		char c1[] = con;					\
		char c2[] = con;					\
		char c3[] = con "\n";					\
		char *mode;						\
									\
		if (do_test_aa_splitcon(c1, &mode, expected_label,	\
				expected_mode, "aa_splitcon: " error)) {\
			rc = 1;						\
		} else if (do_test_aa_splitcon(c2, NULL, expected_label,\
				NULL,					\
				"aa_splitcon: " error " (NULL mode)")) {\
			rc = 1;						\
		} else if (do_test_aa_splitcon(c3, &mode,		\
				expected_label, expected_mode,		\
				"aa_splitcon: " error " (newline)")) {	\
			rc = 1;						\
		}							\
	} while (0)

static int test_splitcon(void)
{
	int rc = 0;

	/**
	 * NOTE: the TEST_SPLITCON() macro automatically generates
	 * corresponding tests with a NULL mode pointer.
	 */

	TEST_SPLITCON("", 0, true, NULL, NULL, "empty string test #1");
	TEST_SPLITCON("", 0, false, NULL, NULL, "empty string test #2");

	TEST_SPLITCON("unconfined", -1, true, "unconfined", NULL,
		      "unconfined #1");
	TEST_SPLITCON("unconfined", -1, false, "unconfined", NULL,
		      "unconfined #2");
	TEST_SPLITCON("unconfined\n", -1, true, "unconfined", NULL,
		      "unconfined #3");
	TEST_SPLITCON("unconfined\n", -1, false, NULL, NULL,
		      "unconfined #4");

	TEST_SPLITCON("label (mode)", -1, true, "label", "mode",
		      "basic split #1");
	TEST_SPLITCON("label (mode)", -1, false, "label", "mode",
		      "basic split #2");
	TEST_SPLITCON("label (mode)\n", -1, true, "label", "mode",
		      "basic split #3");
	TEST_SPLITCON("label (mode)\n", -1, false, NULL, NULL,
		      "basic split #4");

	TEST_SPLITCON("/a/b/c (enforce)", -1, true, "/a/b/c", "enforce",
		      "path enforce split #1");
	TEST_SPLITCON("/a/b/c (enforce)", -1, false, "/a/b/c", "enforce",
		      "path enforce split #2");
	TEST_SPLITCON("/a/b/c (enforce)\n", -1, true, "/a/b/c", "enforce",
		      "path enforce split #3");
	TEST_SPLITCON("/a/b/c (enforce)\n", -1, false, NULL, NULL,
		      "path enforce split #4");

	return rc;
}


static int test_aa_splitcon(void)
{
	int rc = 0;

	/**
	 * NOTE: the TEST_AA_SPLITCON() macro automatically generates
	 * corresponding tests with a NULL mode pointer and contexts with
	 * trailing newline characters.
	 */

	TEST_AA_SPLITCON("label (mode)", "label", "mode", "basic split");

	TEST_AA_SPLITCON("/a/b/c (enforce)", "/a/b/c", "enforce",
			 "path enforce split");

	TEST_AA_SPLITCON("/a/b/c (complain)", "/a/b/c", "complain",
			 "path complain split");

	TEST_AA_SPLITCON("profile_name (enforce)", "profile_name", "enforce",
			 "name enforce split");

	TEST_AA_SPLITCON("profile_name (complain)", "profile_name", "complain",
			 "name complain split");

	TEST_AA_SPLITCON("unconfined", "unconfined", NULL, "unconfined");

	TEST_AA_SPLITCON("(odd) (enforce)", "(odd)", "enforce",
			 "parenthesized label #1");

	TEST_AA_SPLITCON("(odd) (enforce) (enforce)", "(odd) (enforce)",
			 "enforce", "parenthesized label #2");

	TEST_AA_SPLITCON("/usr/bin/😺 (enforce)", "/usr/bin/😺", "enforce",
			 "non-ASCII path");

	TEST_AA_SPLITCON("👍 (enforce)", "👍", "enforce",
			 "non-ASCII profile name");

	/* Negative tests */

	TEST_AA_SPLITCON("", NULL, NULL, "empty string test");

	TEST_AA_SPLITCON("profile\t(enforce)", NULL, NULL,
			 "invalid tab separator");

	TEST_AA_SPLITCON("profile(enforce)", NULL, NULL,
			 "invalid missing separator");

	return rc;
}

int main(void)
{
	int retval, rc = 0;

	retval = test_splitcon();
	if (retval)
		rc = retval;

	retval = test_aa_splitcon();
	if (retval)
		rc = retval;

	return rc;
}
