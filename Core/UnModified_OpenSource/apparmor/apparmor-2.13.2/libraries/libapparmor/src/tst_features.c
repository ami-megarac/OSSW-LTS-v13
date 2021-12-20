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

#include "private.h"

#include "features.c"

static int test_tokenize_path_components(void)
{
	struct component components[32];
	size_t max = sizeof(components) / sizeof(*components);
	size_t num;
	int rc = 0;

	num = tokenize_path_components(NULL, components, max);
	MY_TEST(num == 0, "basic NULL test");

	num = tokenize_path_components("", components, max);
	MY_TEST(num == 0, "basic empty string test");

	num = tokenize_path_components("a", components, 0);
	MY_TEST(num == 0, "basic empty array test");

	num = tokenize_path_components("a", components, 1);
	MY_TEST(num == 1, "one component full test (num)");
	MY_TEST(!strncmp(components[0].str, "a", components[0].len),
		"one component full test (components[0])");

	num = tokenize_path_components("a/b", components, 2);
	MY_TEST(num == 2, "two component full test (num)");
	MY_TEST(!strncmp(components[0].str, "a", components[0].len),
		"two component full test (components[0])");
	MY_TEST(!strncmp(components[1].str, "b", components[0].len),
		"two component full test (components[1])");

	num = tokenize_path_components("a/b/c", components, 1);
	MY_TEST(num == 1, "not enough components full test (num)");
	MY_TEST(!strncmp(components[0].str, "a/b/c", components[0].len),
		"not enough components full test (components[0])");

	num = tokenize_path_components("/", components, max);
	MY_TEST(num == 0, "no valid components #1 (num)");

	num = tokenize_path_components("////////", components, max);
	MY_TEST(num == 0, "no valid components #2 (num)");

	num = tokenize_path_components("////////////foo////", components, max);
	MY_TEST(num == 1, "many invalid components (num)");
	MY_TEST(!strncmp(components[0].str, "foo", components[0].len),
		"many invalid components (components[0])");

	num = tokenize_path_components("file", components, max);
	MY_TEST(num == 1, "file (num)");
	MY_TEST(!strncmp(components[0].str, "file", components[0].len),
		"file (components[0])");

	num = tokenize_path_components("/policy///versions//v7/", components, max);
	MY_TEST(num == 3, "v7 (num)");
	MY_TEST(!strncmp(components[0].str, "policy", components[0].len),
		"v7 (components[0])");
	MY_TEST(!strncmp(components[1].str, "versions", components[1].len),
		"v7 (components[1])");
	MY_TEST(!strncmp(components[2].str, "v7", components[2].len),
		"v7 (components[2])");

	num = tokenize_path_components("dbus/mask/send", components, max);
	MY_TEST(num == 3, "dbus send (num)");
	MY_TEST(!strncmp(components[0].str, "dbus", components[0].len),
		"dbus send (components[0])");
	MY_TEST(!strncmp(components[1].str, "mask", components[1].len),
		"dbus send (components[1])");
	MY_TEST(!strncmp(components[2].str, "send", components[2].len),
		"dbus send (components[2])");


	return rc;
}

static int do_test_walk_one(const char **str, const struct component *component,
			    bool is_top_level, bool expect_walk, const char *e1,
			    const char *e2, const char *e3)
{
	const char *save = str ? *str : NULL;
	bool walked = walk_one(str, component, is_top_level);
	int rc = 0;

	/* Check if the result of the walk matches the expected result*/
	MY_TEST(expect_walk == walked, e1);
	if (save) {
		/**
		 * If a walk was expected, @*str should have changed. It
		 * shouldn't change if a walk was unexpected.
		 */
		if (expect_walk) {
			MY_TEST(*str != save, e2);
		} else {
			MY_TEST(*str == save, e3);
		}
	}

	return rc;
}

#define MY_WALK_TEST(str, component, is_top_level, expect_walk, error)	\
		if (do_test_walk_one(str, component, is_top_level,	\
				     expect_walk,			\
				     error " (walk check)", 		\
				     error " (str didn't change)",	\
				     error " (str changed)")) {		\
			rc = 1;						\
		}

#define MY_GOOD_WALK_TEST(str, component, is_top_level, error)	\
		MY_WALK_TEST(str, component, is_top_level, true, error)
#define MY_BAD_WALK_TEST(str, component, is_top_level, error)	\
		MY_WALK_TEST(str, component, is_top_level, false, error)

static int test_walk_one(void)
{
	struct component c;
	const char *str;
	int rc = 0;

	MY_BAD_WALK_TEST(NULL, &c, true, "basic NULL str test");

	str = NULL;
	MY_BAD_WALK_TEST(&str, &c, true, "basic NULL *str test");

	str = "test { a b }";
	MY_BAD_WALK_TEST(&str, NULL, true, "basic NULL component test");

	str = "test { a b }";
	c = (struct component) { NULL, 8 };
	MY_BAD_WALK_TEST(&str, &c, true, "basic NULL c.str test");

	str = "test { a b }";
	c = (struct component) { "", 0 };
	MY_BAD_WALK_TEST(&str, &c, true, "basic empty c.str test");

	str = "test";
	c = (struct component) { "test", 4 };
	MY_GOOD_WALK_TEST(&str, &c, true, "single component");

	str = "testX";
	c = (struct component) { "test", 4 };
	MY_BAD_WALK_TEST(&str, &c, true, "single component bad str");

	str = "test";
	c = (struct component) { "testX", 5 };
	MY_BAD_WALK_TEST(&str, &c, true, "single component bad c.str");

	str = "test {     }";
	c = (struct component) { "test", 4 };
	MY_GOOD_WALK_TEST(&str, &c, true, "single component empty braces #1");

	str = "test {\n\t}";
	c = (struct component) { "test", 4 };
	MY_GOOD_WALK_TEST(&str, &c, true, "single component empty braces #2");

	str = "test{}";
	c = (struct component) { "test", 4 };
	MY_GOOD_WALK_TEST(&str, &c, true, "single component empty braces #3");

	str = "test\t{}\n       ";
	c = (struct component) { "test", 4 };
	MY_GOOD_WALK_TEST(&str, &c, true, "single component empty braces #4");

	str = "test {}";
	c = (struct component) { "test", 4 };
	MY_BAD_WALK_TEST(&str, &c, false, "single component bad is_top_level");

	str = "front{back";
	c = (struct component) { "frontback", 9};
	MY_BAD_WALK_TEST(&str, &c, true, "brace in the middle #1");
	MY_BAD_WALK_TEST(&str, &c, false, "brace in the middle #2");

	str = "ardvark { bear cat { deer } }";
	c = (struct component) { "ardvark", 7 };
	MY_GOOD_WALK_TEST(&str, &c, true, "animal walk good ardvark");
	c = (struct component) { "deer", 4 };
	MY_BAD_WALK_TEST(&str, &c, false, "animal walk bad deer");
	MY_BAD_WALK_TEST(&str, &c, true, "animal walk bad top-level deer");
	c = (struct component) { "bear", 4 };
	MY_BAD_WALK_TEST(&str, &c, true, "animal walk bad bear");
	c = (struct component) { "cat", 3 };
	MY_GOOD_WALK_TEST(&str, &c, false, "animal walk good cat");
	c = (struct component) { "ardvark", 7 };
	MY_BAD_WALK_TEST(&str, &c, true, "animal walk bad ardvark");
	c = (struct component) { "deer", 4 };
	MY_GOOD_WALK_TEST(&str, &c, false, "animal walk good deer");

	str = "dbus {mask {acquire send receive\n}\n}\nsignal {mask {hup int\n}\n}";
	c = (struct component) { "hup", 3 };
	MY_BAD_WALK_TEST(&str, &c, true, "dbus/signal bad hup #1");
	MY_BAD_WALK_TEST(&str, &c, false, "dbus/signal bad hup #2");
	c = (struct component) { "signal", 6 };
	MY_BAD_WALK_TEST(&str, &c, false, "dbus/signal bad signal");
	MY_GOOD_WALK_TEST(&str, &c, true, "dbus/signal good signal");
	c = (struct component) { "mask", 4 };
	MY_BAD_WALK_TEST(&str, &c, true, "dbus/signal bad mask");
	MY_GOOD_WALK_TEST(&str, &c, false, "dbus/signal good mask");
	c = (struct component) { "hup", 3 };
	MY_GOOD_WALK_TEST(&str, &c, false, "dbus/signal good hup");

	str = "policy {set_load {yes\n}\nversions {v7 {yes\n}\nv6 {yes\n}";
	c = (struct component) { "policy", 6 };
	MY_GOOD_WALK_TEST(&str, &c, true, "policy good");
	c = (struct component) { "versions", 8 };
	MY_GOOD_WALK_TEST(&str, &c, false, "versions good");
	c = (struct component) { "v7", 2 };
	MY_GOOD_WALK_TEST(&str, &c, false, "v7 good");

	return rc;
}

int main(void)
{
	int retval, rc = 0;

	retval = test_tokenize_path_components();
	if (retval)
		rc = retval;

	retval = test_walk_one();
	if (retval)
		rc = retval;

	return rc;
}
