/*
 * Copyright (C) 2014-2016 Canonical, Ltd.
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

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/apparmor.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "changehat.h" /* for do_open() */

#define STACK_DELIM	"//&"
#define STACK_DELIM_LEN	strlen(STACK_DELIM)

#define NO_MODE		"(null)"

#define CHANGE_PROFILE	1
#define CHANGE_ONEXEC	2
#define STACK_PROFILE	3
#define STACK_ONEXEC	4

static void file_io(const char *file)
{
	int rc = do_open(file);

	if (rc != 0)
		exit(rc);
}

struct single_label {
	const char *label;
	size_t len;
};

#define MAX_LABELS	32

struct compound_label {
	size_t num_labels;
	struct single_label labels[MAX_LABELS];
};

/**
 * Initializes @sl by parsing @compound_label. Returns a pointer to the
 * location of the next label in the compound label string, which should be
 * passed in as @compound_label the next time that next_label() is called. NULL
 * is returned when there are no more labels in @compound_label.
 */
static const char *next_label(struct single_label *sl,
			      const char *compound_label)
{
	const char *delim;

	if (!compound_label || compound_label[0] == '\0')
		return NULL;

	delim = strstr(compound_label, STACK_DELIM);
	if (!delim) {
		sl->label = compound_label;
		sl->len = strlen(sl->label);
		return sl->label + sl->len;
	}

	sl->label = compound_label;
	sl->len = delim - sl->label;
	return delim + STACK_DELIM_LEN;
}

/* Returns true if the compound label was constructed successfully */
static bool compound_label_init(struct compound_label *cl,
				const char *compound_label)
{
	memset(cl, 0, sizeof(*cl));
	while ((compound_label = next_label(&cl->labels[cl->num_labels],
					    compound_label))) {
		cl->num_labels++;

		if (cl->num_labels == MAX_LABELS)
			return false;
	}

	return true;
}

/* Returns true if the compound label contains the single label */
static bool compound_label_contains(struct compound_label *cl,
				    struct single_label *sl)
{
	bool matched = false;
	size_t i;

	for (i = 0; !matched && i < cl->num_labels; i++) {
		if (cl->labels[i].len != sl->len)
			continue;

		if (strncmp(cl->labels[i].label, sl->label, sl->len))
			continue;

		matched = true;
	}

	return matched;
}

/* Returns true if the two compound labels contain the same label sets */
static bool compound_labels_equal(struct compound_label *cl1,
				  struct compound_label *cl2)
{
	size_t i;

	if (cl1->num_labels != cl2->num_labels)
		return false;

	for (i = 0; i < cl1->num_labels; i++) {
		if (!compound_label_contains(cl2, &cl1->labels[i]))
			return false;
	}

	return true;
}

/**
 * Verifies that the current confinement context matches the expected context.
 *
 * @attr is the file in /proc/self/attr/ that you want to verify. It is passed
 * directly to aa_getprocattr(2).
 *
 * Either @expected_label or @expected_mode can be NULL if their values should
 * not be verified. If a NULL mode is expected, as what happens when an
 * unconfined process calls aa_getcon(2), then @expected_mode should be equal
 * to NO_MODE.
 */
static void verify_confinement_context(const char *attr,
				       const char *expected_label,
				       const char *expected_mode)
{
	char *label, *mode;
	int expected_rc, rc;
	bool null_expected_mode = expected_mode ?
				  strcmp(NO_MODE, expected_mode) == 0 : false;

	rc = aa_getprocattr(getpid(), attr, &label, &mode);
	if (rc < 0) {
		int err = errno;
		fprintf(stderr, "FAIL - aa_getprocattr (%s): %m", attr);
		exit(err);
	}

	if (expected_label) {
		struct compound_label cl, expected_cl;

		if (!compound_label_init(&cl, label)) {
			fprintf(stderr, "FAIL - could not parse current compound label: %s\n",
				label);
			rc = EINVAL;
			goto err;
		}

		if (!compound_label_init(&expected_cl, expected_label)) {
			fprintf(stderr, "FAIL - could not parse expected compound label: %s\n",
				expected_label);
			rc = EINVAL;
			goto err;
		}

		if (!compound_labels_equal(&cl, &expected_cl)) {
			fprintf(stderr, "FAIL - %s label \"%s\" != expected_label \"%s\"\n",
				attr, label, expected_label);
			rc = EINVAL;
			goto err;
		}
	}

	if (expected_mode &&
	    ((!mode && !null_expected_mode) ||
	     (mode && strcmp(mode, expected_mode)))) {
		fprintf(stderr, "FAIL - %s mode \"%s\" != expected_mode \"%s\"\n",
			attr, mode, expected_mode);
		rc = EINVAL;
		goto err;
	}

	expected_rc = expected_label ? strlen(expected_label) : strlen(label);

	/**
	 * Add the expected bytes following the returned label string:
	 *
	 *   ' ' + '(' + mode + ')'
	 */
	if (expected_mode && !null_expected_mode)
		expected_rc += 1 + 1 + strlen(expected_mode) + 1;
	else if (mode)
		expected_rc += 1 + 1 + strlen(mode) + 1;

	expected_rc++; /* Trailing NUL terminator */

	if (rc != expected_rc) {
		fprintf(stderr, "FAIL - rc (%d) != expected_rc (%d)\n",
			rc, expected_rc);
		rc = EINVAL;
		goto err;
	}

	return;
err:
	free(label);
	exit(EINVAL);
}

static void verify_current(const char *expected_label,
			   const char *expected_mode)
{
	verify_confinement_context("current", expected_label, expected_mode);
}

static void verify_exec(const char *expected_label,
			const char *expected_mode)
{
	verify_confinement_context("exec", expected_label, expected_mode);
}

static void handle_transition(int transition, const char *target)
{
	const char *msg;
	int rc = 0;

	switch (transition) {
	case CHANGE_ONEXEC:
		msg = "FAIL - aa_change_onexec";
		rc = aa_change_onexec(target);
		break;
	case CHANGE_PROFILE:
		msg = "FAIL - aa_change_profile";
		rc = aa_change_profile(target);
		break;
	case STACK_ONEXEC:
		msg = "FAIL - aa_stack_onexec";
#ifdef WITHOUT_STACKING
		rc = -1;
		errno = ENOTSUP;
#else
		rc = aa_stack_onexec(target);
#endif
		break;
	case STACK_PROFILE:
		msg = "FAIL - aa_stack_profile";
#ifdef WITHOUT_STACKING
		rc = -1;
		errno = ENOTSUP;
#else
		rc = aa_stack_profile(target);
#endif
		break;
	default:
		msg = "FAIL - handle_transition";
		rc = -1;
		errno = ENOTSUP;
	}

	if (rc != 0) {
		int err = errno;
		perror(msg);
		exit(err);
	}
}

static void exec(const char *prog, char **argv)
{
	int err;

	execv(prog, argv);
	err = errno;
	perror("FAIL - execv");
	exit(err);
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"%s: [-O <LABEL> | -P <LABEL> | -o <LABEL> | -p <LABEL>] [-L <LABEL>] [-M <MODE>] [-l <LABEL>] [-m <MODE>] [-f <FILE>] [-- ... [-- ...]]\n"
		"  -O <LABEL>\tCall aa_change_onexec(LABEL)\n"
		"  -P <LABEL>\tCall aa_change_profile(LABEL)\n"
		"  -o <LABEL>\tCall aa_stack_onexec(LABEL)\n"
		"  -p <LABEL>\tCall aa_stack_profile(LABEL)\n"
		"  -L <LABEL>\tVerify that /proc/self/attr/exec contains LABEL\n"
		"  -M <MODE>\tVerify that /proc/self/attr/exec contains MODE. Set to \"%s\" if a NULL mode is expected.\n"
		"  -l <LABEL>\tVerify that /proc/self/attr/current contains LABEL\n"
		"  -m <MODE>\tVerify that /proc/self/attr/current contains MODE. Set to \"%s\" if a NULL mode is expected.\n"
		"  -f <FILE>\tOpen FILE and attempt to write to and read from it\n\n"
		"If \"--\" is encountered, execv() will be called using the following argument\n"
		"as the program to execute and passing it all of the arguments following the\n"
		"program name.\n", prog, NO_MODE, NO_MODE);
	exit(EINVAL);
}

struct options {
	const char *file;
	const char *expected_current_label;
	const char *expected_current_mode;
	const char *expected_exec_label;
	const char *expected_exec_mode;

	int transition;		/* CHANGE_PROFILE, STACK_ONEXEC, etc. */
	const char *target;	/* The target label of the transition */

	const char *exec;
	char **exec_argv;
};

static void set_transition(const char *prog, struct options *opts,
			   int transition, const char *target)
{
	/* Can only specify one transition */
	if (opts->transition || opts->target)
		usage(prog);

	opts->transition = transition;
	opts->target = target;
}

static void parse_opts(int argc, char **argv, struct options *opts)
{
	const char *prog = argv[0];
	int o;

	memset(opts, 0, sizeof(*opts));
	while ((o = getopt(argc, argv, "f:L:M:l:m:O:P:o:p:")) != -1) {
		switch (o) {
		case 'f': /* file */
			opts->file = optarg;
			break;
		case 'L': /* expected exec label */
			opts->expected_exec_label = optarg;
			break;
		case 'M': /* expected exec mode */
			opts->expected_exec_mode = optarg;
			break;
		case 'l': /* expected current label */
			opts->expected_current_label = optarg;
			break;
		case 'm': /* expected current mode */
			opts->expected_current_mode = optarg;
			break;
		case 'O': /* aa_change_profile */
			set_transition(prog, opts, CHANGE_ONEXEC, optarg);
			break;
		case 'P': /* aa_change_profile */
			set_transition(prog, opts, CHANGE_PROFILE, optarg);
			break;
		case 'o': /* aa_stack_onexec */
			set_transition(prog, opts, STACK_ONEXEC, optarg);
			break;
		case 'p': /* aa_stack_profile */
			set_transition(prog, opts, STACK_PROFILE, optarg);
			break;
		default: /* '?' */
			usage(prog);
		}
	}

	if (optind < argc) {
		/* Ensure that the previous option was "--" */
		if (optind == 0 || strcmp("--", argv[optind - 1]))
			usage(prog);

		opts->exec = argv[optind];
		opts->exec_argv = &argv[optind];
	}
}

int main(int argc, char **argv)
{
	struct options opts;

	parse_opts(argc, argv, &opts);

	if (opts.transition)
		handle_transition(opts.transition, opts.target);

	if (opts.file)
		file_io(opts.file);

	if (opts.expected_current_label || opts.expected_current_mode)
		verify_current(opts.expected_current_label,
			       opts.expected_current_mode);

	if (opts.expected_exec_label || opts.expected_exec_mode)
		verify_exec(opts.expected_exec_label, opts.expected_exec_mode);

	if (opts.exec)
		exec(opts.exec, opts.exec_argv);

	printf("PASS\n");
	exit(0);
}

