/*
 *   Copyright (C) 2015 Canonical Ltd.
 *
 *   This program is free software; you can redistribute it and/or
 *    modify it under the terms of version 2 of the GNU General Public
 *   License published by the Free Software Foundation.
 */

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#define _(s) gettext(s)

#include <sys/apparmor.h>

void print_help(const char *command)
{
	printf(_("%s: [options]\n"
		 "  options:\n"
		 "  -q | --quiet        Don't print out any messages\n"
		 "  -h | --help         Print help\n"),
	       command);
	exit(1);
}


/* Exit statuses and meanings are documented in the aa-enabled.pod file */
static void exit_with_error(int saved_errno, int quiet)
{
	int err;

	switch(saved_errno) {
	case ENOSYS:
		if (!quiet)
			printf(_("No - not available on this system.\n"));
		exit(1);
	case ECANCELED:
		if (!quiet)
			printf(_("No - disabled at boot.\n"));
		exit(1);
	case ENOENT:
		if (!quiet)
			printf(_("Maybe - policy interface not available.\n"));
		exit(3);
	case EPERM:
	case EACCES:
		if (!quiet)
			printf(_("Maybe - insufficient permissions to determine availability.\n"));
		exit(4);
	}

	if (!quiet)
		printf(_("Error - %s\n"), strerror(saved_errno));
	exit(64);
}

int main(int argc, char **argv)
{
	int enabled;
	int quiet = 0;

	setlocale(LC_MESSAGES, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	if (argc > 2) {
		printf(_("unknown or incompatible options\n"));
		print_help(argv[0]);
	} else if (argc == 2) {
		if (strcmp(argv[1], "--quiet") == 0 ||
		    strcmp(argv[1], "-q") == 0) {
			quiet = 1;
		} else if (strcmp(argv[1], "--help") == 0 ||
			   strcmp(argv[1], "-h") == 0) {
			print_help(argv[0]);
		} else {
			printf(_("unknown option '%s'\n"), argv[1]);
			print_help(argv[0]);
		}
	}

	enabled = aa_is_enabled();
	if (!enabled)
		exit_with_error(errno, quiet);

	if (!quiet)
		printf(_("Yes\n"));
	exit(0);
}
