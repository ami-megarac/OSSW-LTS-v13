/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
 *   Copyright (c) 2010 - 2012
 *   Canonical Ltd.
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
 *   along with this program; if not, contact Canonical, Ltd.
 */

/* Handle subdomain includes, as a straight forward preprocessing phase.
   While we are at it we will strip comments.  Why? because it made it
   easier.

   We support 2 types of includes

#include <name> which searches for the first occurance of name in the
   subdomain directory path.

#include "name" which will search for a relative or absolute pathed
   file

-p : preprocess only.  Dump output to stdout
-I path : add a path to be search by #include < >
-b path : set the base path to something other than /etc/subdomain.d

*/

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

#include "lib.h"
#include "parser.h"
#include "parser_include.h"

/* An array of search directories, I sure hope 100's enough */
#define MAX_PATH 100

/* maximum depth of nesting */
#define MAX_NEST_LEVEL 100

/* Location of the subdomain.conf file */
#ifdef SUBDOMAIN_CONFDIR
#define SUBDOMAIN_CONF SUBDOMAIN_CONFDIR "/subdomain.conf"
#else	/* !defined SUBDOMAIN_CONFDIR */
#define SUBDOMAIN_CONF "/etc/subdomain.conf"
#endif	/* SUBDOMAIN_CONFDIR */

static char *path[MAX_PATH] = { NULL };
static int npath = 0;

static int fgetline(FILE * f, char *buffer, size_t len);
static int stripcomment(char *s);
static char *stripblanks(char *s);

/* default base directory is /etc/subdomain.d, it can be overriden
   with the -b option. */

const char *basedir;
static const char *default_basedir = "/etc/apparmor.d";
static const char *old_basedir = "/etc/subdomain.d";


/* set up basedir so that it can be overridden/used later. */
void init_base_dir(void)
{
	int rc;
	struct stat sbuf;

	/* basedir should always start out NULL; if not, something's
	 * wrong.*/
	assert(basedir == NULL);

	rc = stat(default_basedir, &sbuf);
	if (rc == 0 && S_ISDIR(sbuf.st_mode)) {
		basedir = default_basedir;
		return;
	}

	rc = stat(old_basedir, &sbuf);
	if (rc == 0 && S_ISDIR(sbuf.st_mode)) {
		basedir = old_basedir;
		return;
	}
}

/* Set the base dir.  Used to change default path for relative includes */
void set_base_dir(char *dir)
{
	char *t;
	int i, rc;
	struct stat sbuf;

	t = strndup(dir, PATH_MAX);
	if (!t) {
		PERROR(_("Error: Out of memory.\n"));
		return;
	}

	/*strip trailing /'s */
	for (i = strlen(t) - 1; i >= 0 && t[i] == '/'; i--)
		t[i] = 0;

	rc = stat(t, &sbuf);
	if (rc == -1 || !S_ISDIR(sbuf.st_mode)) {
		PERROR(_("Error: basedir %s is not a directory, skipping.\n"), t);
		free(t);
		return;
	}

	basedir = t;
	return;
}

/* Add a directory to the search path. */
int add_search_dir(const char *dir)
{
	char *t;
	size_t len;

	if (npath >= MAX_PATH) {
		PERROR(_("Error: Could not add directory %s to search path.\n"),
		       dir);
		return 0;
	}

	if (!dir)
		return 1;

	len = strlen(dir);
	if (len == 0)
		return 1;

	t = strdup(dir);
	if (t == NULL) {
		PERROR(_("Error: Could not allocate memory.\n"));
		return 0;
	}

	/*strip trailing /'s */
	while (len > 0 && t[--len] == '/')
		t[len] = '\0';
	path[npath] = t;
	npath++;

	return 1;
}

/* Parse Subdomain.conf and put the default dirs in place.  

   subdomain.conf is a shell sourcable file
   we only parse entries starting with
   SUBDOMAIN_PATH=

   if there are multiple entries with SUBDOMAIN_PATH=
   each will get added.

   SUBDOMAIN_PATH=/etc/subdomain.d:/etc/subdomain.d/include
   is the same as
   SUBDOMAIN_PATH=/etc/subdomain.d
   SUBDOMAIN_PATH=/etc/subdomain.d/include */
void parse_default_paths(void)
{
	autofclose FILE *f;
	char buf[1024];
	char *t, *s;
	int saved_npath = npath;

	f = fopen(SUBDOMAIN_CONF, "r");
	if (f == NULL)
		goto out;

	memset(buf, 0, sizeof(buf));

	while (fgetline(f, buf, 1024)) {
		if (stripcomment(buf) && (t = strstr(buf, "SUBDOMAIN_PATH="))) {
			t += 15;
			/* handle : separating path elements */
			do {
				s = strchr(t, ':');
				if (s)
					*s = 0;
				if (!add_search_dir(stripblanks(t)))
					break;
				if (s)
					t = s + 1;
			} while (s != NULL);
		}
	}

	/* if subdomain.conf doesn't set a base search dir set it to this */
out:
	if (npath - saved_npath == 0) {
		add_search_dir(basedir);
	}
}

FILE *search_path(char *filename, char **fullpath)
{
	FILE *newf = NULL;
	char *buf = NULL;
	int i;
	for (i = 0; i < npath; i++) {
		if (asprintf(&buf, "%s/%s", path[i], filename) < 0) {
			perror("asprintf");
			exit(1);
		}
		newf = fopen(buf, "r");
		if (newf && fullpath)
			*fullpath = buf;
		else
			free(buf);
		buf = NULL;
		if (newf)
			break;
	}
	return newf;
}

/* get a line from the file.  If it is to long truncate it. */
static int fgetline(FILE * f, char *buffer, size_t len)
{
	char *b = buffer;
	int c;

	while (((c = fgetc(f)) != EOF) && (c != '\n')
	       && (strlen(buffer) < len - 1)) {
		*b = c;
		b++;
	}
	*b = '\0';
	if (c != EOF)
		return 1;
	return 0;
}

/* If there is a comment null terminate the string,
   return strlen of the stripped string*/
static int stripcomment(char *s)
{
	char *t = s;
	while (*s != '#' && *s != 0)
		s++;
	*s = 0;

	return strlen(t);
}

static char *stripblanks(char *s)
{
	char *c;

	while (isspace(*s))
		s++;
	c = s;
	while (!isspace(*s) && *s != 0)
		s++;
	*s = 0;
	return c;
}

struct include_stack_t {
	char *filename;
	int lineno;
	struct include_stack_t *next;
};

struct include_stack_t *include_stack_head = NULL;

static void start_include_position(const char *filename)
{
	if (current_filename)
		free(current_filename);
	current_filename = strdup(filename ? filename : "stdin");
	current_lineno   = 1;
}

void push_include_stack(char *filename)
{
	struct include_stack_t *include = NULL;

	include = (struct include_stack_t *) malloc(sizeof(*include));
	if (!include) {
		perror("malloc of included file stack tracker");
		/* failures in this area are non-fatal */
		return;
	}

	include->filename  = strdup(current_filename);
	include->lineno    = current_lineno;
	include->next      = include_stack_head;
	include_stack_head = include;

	start_include_position(filename);
}

void pop_include_stack(void)
{
	struct include_stack_t *include = NULL;

	if (!include_stack_head)
		return;

	include = include_stack_head;
	include_stack_head = include->next;

	if (current_filename)
		free(current_filename);
	current_filename = include->filename;
	current_lineno   = include->lineno;
	free(include);
}

void reset_include_stack(const char *filename)
{
	while (include_stack_head)
		pop_include_stack();

	start_include_position(filename);
}
