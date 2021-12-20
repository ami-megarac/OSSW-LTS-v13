/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact Canonical Ltd.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/apparmor.h>

#define OPT_NEW			"new"
#define OPT_REMOVE		"remove"
#define OPT_REMOVE_POLICY	"remove-policy"
#define OPT_REPLACE_ALL		"replace-all"
#define OPT_FLAG_MAX_CACHES	"--max-caches"

static void usage(const char *prog)
{
	fprintf(stderr,
		"FAIL - usage: %s %s [%s N] <PATH>\n"
		"              %s %s <PATH>\n"
		"              %s %s <PROFILE_NAME>\n"
		"              %s %s [%s N] <PATH>\n",
		prog, OPT_NEW, OPT_FLAG_MAX_CACHES,
		prog, OPT_REMOVE,
		prog, OPT_REMOVE_POLICY,
		prog, OPT_REPLACE_ALL, OPT_FLAG_MAX_CACHES);
}

static int test_new(const char *path, uint16_t max_caches)
{
	aa_policy_cache *policy_cache = NULL;
	int rc = 1;

	if (aa_policy_cache_new(&policy_cache, NULL,
				AT_FDCWD, path, max_caches)) {
		perror("FAIL - aa_policy_cache_new");
		goto out;
	}

	rc = 0;
out:
	aa_policy_cache_unref(policy_cache);
	return rc;
}

static int test_remove(const char *path)
{
	int rc = 1;

	if (aa_policy_cache_remove(AT_FDCWD, path)) {
		perror("FAIL - aa_policy_cache_remove");
		goto out;
	}

	rc = 0;
out:
	return rc;
}

static int test_remove_policy(const char *name)
{
	aa_kernel_interface *kernel_interface = NULL;
	int rc = 1;

	if (aa_kernel_interface_new(&kernel_interface, NULL, NULL)) {
		perror("FAIL - aa_kernel_interface_new");
		goto out;
	}

	if (aa_kernel_interface_remove_policy(kernel_interface, name)) {
		perror("FAIL - aa_kernel_interface_remove_policy");
		goto out;
	}

	rc = 0;
out:
	aa_kernel_interface_unref(kernel_interface);
	return rc;
}

static int test_replace_all(const char *path, uint16_t max_caches)
{
	aa_policy_cache *policy_cache = NULL;
	int rc = 1;

	if (aa_policy_cache_new(&policy_cache, NULL,
				AT_FDCWD, path, max_caches)) {
		perror("FAIL - aa_policy_cache_new");
		goto out;
	}

	if (aa_policy_cache_replace_all(policy_cache, NULL)) {
		perror("FAIL - aa_policy_cache_replace_all");
		goto out;
	}

	rc = 0;
out:
	aa_policy_cache_unref(policy_cache);
	return rc;
}

int main(int argc, char **argv)
{
	uint16_t max_caches = 0;
	const char *str = NULL;
	int rc = 1;

	if (!(argc == 3 || argc == 5)) {
		usage(argv[0]);
		exit(1);
	}

	str = argv[argc - 1];

	if (argc == 5) {
		unsigned long tmp;

		errno = 0;
		tmp = strtoul(argv[3], NULL, 10);
		if ((errno == ERANGE && tmp == ULONG_MAX) ||
		    (errno && tmp == 0)) {
			perror("FAIL - strtoul");
			exit(1);
		}

		if (tmp > UINT16_MAX) {
			fprintf(stderr, "FAIL - %lu larger than UINT16_MAX\n",
				tmp);
			exit(1);
		}

		max_caches = tmp;
	}

	if (strcmp(argv[1], OPT_NEW) == 0) {
		rc = test_new(str, max_caches);
	} else if (strcmp(argv[1], OPT_REMOVE) == 0 && argc == 3) {
		rc = test_remove(str);
	} else if (strcmp(argv[1], OPT_REMOVE_POLICY) == 0 && argc == 3) {
		rc = test_remove_policy(str);
	} else if (strcmp(argv[1], OPT_REPLACE_ALL) == 0) {
		rc = test_replace_all(str, max_caches);
	} else {
		usage(argv[0]);
	}

	if (!rc)
		printf("PASS\n");

	exit(rc);
}
