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

#include <errno.h>
#include <getopt.h>
#include <libintl.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/apparmor.h>
#include <unistd.h>
#define _(s) gettext(s)

static const char *opt_profile = NULL;
static const char *opt_namespace = NULL;
static bool opt_debug = false;
static bool opt_immediate = false;
static bool opt_verbose = false;

static void usage(const char *name, bool error)
{
	FILE *stream = stdout;
	int status = EXIT_SUCCESS;

	if (error) {
		stream = stderr;
		status = EXIT_FAILURE;
	}

	fprintf(stream,
		_("USAGE: %s [OPTIONS] <prog> <args>\n"
		"\n"
		"Confine <prog> with the specified PROFILE.\n"
		"\n"
		"OPTIONS:\n"
		"  -p PROFILE, --profile=PROFILE		PROFILE to confine <prog> with\n"
		"  -n NAMESPACE, --namespace=NAMESPACE	NAMESPACE to confine <prog> in\n"
		"  -d, --debug				show messages with debugging information\n"
		"  -i, --immediate			change profile immediately instead of at exec\n"
		"  -v, --verbose				show messages with stats\n"
		"  -h, --help				display this help\n"
		"\n"), name);
	exit(status);
}

#define error(fmt, args...) _error(_("aa-exec: ERROR: " fmt "\n"), ## args)
static void _error(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}

#define debug(fmt, args...) _debug(_("aa-exec: DEBUG: " fmt "\n"), ## args)
static void _debug(const char *fmt, ...)
{
	va_list args;

	if (!opt_debug)
		return;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

#define verbose(fmt, args...) _verbose(_(fmt "\n"), ## args)
static void _verbose(const char *fmt, ...)
{
	va_list args;

	if (!opt_verbose)
		return;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

static void verbose_print_argv(char **argv)
{
	if (!opt_verbose)
		return;

	fprintf(stderr, _("exec"));
	for (; *argv; argv++)
		fprintf(stderr, " %s", *argv);
	fprintf(stderr, "\n");
}

static char **parse_args(int argc, char **argv)
{
	int opt;
	struct option long_opts[] = {
		{"debug", no_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{"profile", required_argument, 0, 'p'},
		{"namespace", required_argument, 0, 'n'},
		{"immediate", no_argument, 0, 'i'},
		{"verbose", no_argument, 0, 'v'},
	};

	while ((opt = getopt_long(argc, argv, "+dhp:n:iv", long_opts, NULL)) != -1) {
		switch (opt) {
		case 'd':
			opt_debug = true;
			break;
		case 'h':
			usage(argv[0], false);
			break;
		case 'p':
			opt_profile = optarg;
			break;
		case 'n':
			opt_namespace = optarg;
			break;
		case 'i':
			opt_immediate = true;
			break;
		case 'v':
			opt_verbose = true;
			break;
		default:
			usage(argv[0], true);
			break;
		}
	}

	if (optind >= argc)
		usage(argv[0], true);

	return argv + optind;
}

static void build_name(char *name, size_t name_len,
		       const char *namespace, const char *profile)
{
	size_t required_len = 1; /* reserve 1 byte for NUL-terminator */

	if (namespace)
		required_len += 1 + strlen(namespace) + 3; /* :<NAMESPACE>:// */

	if (profile)
		required_len += strlen(profile);

	if (required_len > name_len)
		error("name too long (%zu > %zu)", required_len, name_len);

	name[0] = '\0';

	if (namespace) {
		strcat(name, ":");
		strcat(name, namespace);
		strcat(name, "://");
	}

	if (profile)
		strcat(name, profile);
}

int main(int argc, char **argv)
{
	char name[PATH_MAX];
	int rc = 0;

	argv = parse_args(argc, argv);

	if (opt_namespace || opt_profile)
		build_name(name, sizeof(name), opt_namespace, opt_profile);
	else
		goto exec;

	if (opt_immediate) {
		verbose("aa_change_profile(\"%s\")", name);
		rc = aa_change_profile(name);
		debug("%d = aa_change_profile(\"%s\")", rc, name);
	} else {
		verbose("aa_change_onexec(\"%s\")", name);
		rc = aa_change_onexec(name);
		debug("%d = aa_change_onexec(\"%s\")", rc, name);
	}

	if (rc) {
		if (errno == ENOENT || errno == EACCES) {
			error("%s '%s' does not exist\n",
			      opt_profile ? "profile" : "namespace", name);
		} else if (errno == EINVAL) {
			error("AppArmor interface not available");
		} else {
			error("%m");
		}
	}

exec:
	verbose_print_argv(argv);
	execvp(argv[0], argv);
	error("Failed to execute \"%s\": %m", argv[0]);
}
