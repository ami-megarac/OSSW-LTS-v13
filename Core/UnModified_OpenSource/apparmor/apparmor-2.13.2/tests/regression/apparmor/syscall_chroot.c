/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

struct dirlist {
	struct dirlist * next;
	struct dirent dirent;
};

struct dirlist *
read_directory (char * dirname) {

	DIR * dir;
	struct dirent * dirent;
	struct dirlist * dirlist = NULL, * tmp;

	if (!dirname) return NULL;
	dir = opendir (dirname);

	if (!dir) {
		perror ("FAIL: couldn't open directory");
		exit (errno);
	}

	while ((dirent = readdir (dir)) != NULL) {
		tmp = malloc (sizeof (struct dirlist));
		if (!tmp) {
			perror ("FAIL: failed allocation");
			exit (1);
		}

		memcpy (&tmp->dirent, dirent, sizeof (struct dirent));
		tmp->next = dirlist;
		dirlist = tmp;
		/* printf ("Adding '%s'\n", tmp->dirent.d_name); */
	}

	return dirlist;
}

static inline int
contains_dir (struct dirlist * dirlist, char * dirname) {
	struct dirlist * this;

	for (this = dirlist; this != NULL; this = this->next)
		if (strcmp (this->dirent.d_name, dirname) == 0)
			return 1;

	return 0;
}

int
compare_dirlists (struct dirlist * a, struct dirlist * b) {
	struct dirlist * this;

	for (this = a; this != NULL; this = this->next) 
		if (!contains_dir (b, this->dirent.d_name))
			return 1;

	for (this = b; this != NULL; this = this->next) 
		if (!contains_dir (a, this->dirent.d_name))
			return 1;

	return 0;
}	
	
		
int main(int argc, char *argv[])
{
	struct dirlist * before, * after;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <dir>\n",
				argv[0]);
		return 1;
	}

	before = read_directory("/");

	if (chroot(argv[1]) == -1) {
		perror("FAIL: chroot failed");
		return 1;
	}

	after = read_directory("/");

	if (compare_dirlists(before, after) == 0) {
		fprintf (stderr, "FAIL: root directories are the same\n");
		return 1;
	}

	printf("PASS\n");

	return 0;
}
