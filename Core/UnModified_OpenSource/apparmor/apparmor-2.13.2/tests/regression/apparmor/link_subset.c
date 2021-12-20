/*
 *	Copyright (C) 2002-2007 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "changehat.h"

/* Create all possible link + other permission combinations
 * actual mapping does NOT match kernel, just needed for bit mangling */
#define TEST_MAY_EXEC			0x001
#define TEST_MAY_WRITE			0x002
#define TEST_MAY_READ			0x004
#define TEST_MAY_APPEND			0x008

#define TEST_MAY_LINK			0x0010
#define TEST_MAY_LOCK			0x0020
#define TEST_MAY_MOUNT			0x0040
#define TEST_EXEC_MMAP			0x0080

#define TEST_EXEC_UNSAFE			0x0100
#define TEST_EXEC_INHERIT			0x0200

#define TEST_EXEC_MOD_0			0x0400
#define TEST_EXEC_MOD_1			0x0800
#define TEST_EXEC_MOD_2			0x1000
#define TEST_EXEC_MOD_3			0x2000

#define TEST_EXEC_MODIFIERS		(TEST_EXEC_MOD_0 | TEST_EXEC_MOD_1 | \
					 TEST_EXEC_MOD_2 | TEST_EXEC_MOD_3)


#define TEST_EXEC_TYPE (TEST_MAY_EXEC | TEST_EXEC_UNSAFE | TEST_EXEC_INHERIT | \
		      TEST_EXEC_MODIFIERS)

#define TEST_EXEC_UNCONFINED TEST_EXEC_MOD_0
#define TEST_EXEC_PROFILE    TEST_EXEC_MOD_1
#define TEST_EXEC_LOCAL     (TEST_EXEC_MOD_0 | TEST_EXEC_MOD_1)

#define MAX_PERM (TEST_EXEC_MOD_2)
#define MAX_PERM_LEN 10


/* Set up permission subset test as a seperate binary to reduce the time
 * as the shell based versions takes for ever
 */

/* test if link_perm is a subset of target_perm */
int valid_link_perm_subset(int tperm, int lperm)
{
	/* link must always have link bit set */
	if (!(lperm & TEST_MAY_LINK))
		return 0;

	lperm = lperm & ~TEST_MAY_LINK;

	/* an empty permission set is always a subset of target */
	if (!lperm)
		return 1;

	/* ix implies mix */
	if (lperm & TEST_EXEC_INHERIT)
		lperm |= TEST_EXEC_MMAP;
	if (tperm & TEST_EXEC_INHERIT)
		tperm |= TEST_EXEC_MMAP;

	/* w implies a */
	if (lperm & TEST_MAY_WRITE)
		lperm |= TEST_MAY_APPEND;
	if (tperm & TEST_MAY_WRITE)
		tperm |= TEST_MAY_APPEND;

	/* currently no such thing as a safe ix - probably should be
	 * depending on how the rule is written */
//	if ((tperm & TEST_EXEC_MODIFIERS) == TEST_EXEC_INHERIT && !(tperm & TEST_EXEC_UNSAFE))
//		tperm |= TEST_EXEC_UNSAFE;

	/* treat safe exec as subset of unsafe exec */
	if (!(lperm & TEST_EXEC_UNSAFE))
		lperm |= TEST_EXEC_UNSAFE & tperm;

	/* check that exec mode, if present, matches */
	if ((lperm & TEST_MAY_EXEC) && ((lperm & TEST_EXEC_TYPE) != (tperm & TEST_EXEC_TYPE)))
		return 0;

	return !(lperm & ~tperm);
}

void permstring(char *buffer, int mask)
{
	char *b = buffer;

	if (mask & TEST_EXEC_MMAP)
		*b++ = 'm';
	if (mask & TEST_MAY_READ)
		*b++ = 'r';
	if (mask & TEST_MAY_WRITE)
		*b++ = 'w';
	else if (mask & TEST_MAY_APPEND)
		*b++ = 'a';
	if (mask & TEST_MAY_EXEC) {
		if (mask & TEST_EXEC_UNSAFE) {
			switch(mask & TEST_EXEC_MODIFIERS) {
			case TEST_EXEC_UNCONFINED:
				*b++ = 'u';
				break;
			case TEST_EXEC_PROFILE:
				*b++ = 'p';
				break;
			case TEST_EXEC_LOCAL:
				*b++ = 'c';
				break;
			default:
				*b++ = 'y';
			}
		} else {
			switch(mask & TEST_EXEC_MODIFIERS) {
			case TEST_EXEC_UNCONFINED:
				*b++ = 'U';
				break;
			case TEST_EXEC_PROFILE:
				*b++ = 'P';
				break;
			case TEST_EXEC_LOCAL:
				*b++ = 'C';
				break;
			default:
				*b++ = 'Y';
			}
		}
		if (mask & TEST_EXEC_INHERIT)
			*b++ = 'i';
		*b++ = 'x';
	}
	if (mask & TEST_MAY_LINK)
		*b++ = 'l';
	if (mask & TEST_MAY_LOCK)
		*b++ = 'k';
	*b++ = '\0';
}

/* generate the filename based off of perm set. */
void build_filename(const char *name, int perm, char *buffer)
{
	char perms[10];
	permstring(perms, perm);
	sprintf(buffer, "%s%s", name, perms);
}

int is_valid_perm_set(int perm) {
	if (TEST_EXEC_TYPE & perm) {
		/* exec mods need the perm bit set */
		if (!(perm & TEST_MAY_EXEC))
			return 0;

		/* unconfined can't inherit */
		if (((perm & TEST_EXEC_MODIFIERS) == TEST_EXEC_UNCONFINED) &&
		    (perm & TEST_EXEC_INHERIT))
			return 0;

		/* no such thing as an unsafe ix */
		if ((perm & TEST_EXEC_MODIFIERS) == 0 && (perm & TEST_EXEC_INHERIT) && (perm & TEST_EXEC_UNSAFE))
			return 0;

		/* check exec_modifiers in range */
		if (!((perm & TEST_EXEC_MODIFIERS) > 0 && (perm & TEST_EXEC_MODIFIERS) < TEST_EXEC_MOD_2))
			return 0;
	}
	/* only 1 of append or write should be set */
	if ((perm & TEST_MAY_WRITE) && (perm & TEST_MAY_APPEND))
		return 0;

	/* not using mount yet, how should mount perms affect link? */
	if (perm & TEST_MAY_MOUNT)
		return 0;

	return 1;
}

int main(int argc, char *argv[])
{
	int fail = 0, pass = 0;
	int tperm, lperm;
	char *lname, *tname;
	int res;

	if (argc != 3){
		fprintf(stderr, "usage: %s target_file link_file\n",
			argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "--filenames") == 0) {
		/* just output the filename and permissions used */
		char perms[10];
		char *b, *buffer = malloc(MAX_PERM * (strlen(argv[2] + 2*MAX_PERM_LEN +2)));
		if (!buffer)
			goto fail;
		b = buffer;

		lname = malloc(strlen(argv[2]) + 2*MAX_PERM_LEN + 2);
		if (!lname)
			goto fail;
		for (lperm = 1; lperm < MAX_PERM; lperm++) {
			if (!is_valid_perm_set(lperm))
				continue;
			if (lperm)
				*b++ = ' ';
			permstring(perms, lperm);
			sprintf(lname, "%s%s:%s", argv[2], perms, perms);
			strcpy(b, lname);
			b += strlen(lname);
		}
		printf("%s", buffer);
		return 0;
	}


	tname = malloc(strlen(argv[1]) + 2*MAX_PERM_LEN + 2);
	lname = malloc(strlen(argv[2]) + 2*MAX_PERM_LEN + 2);
	/* no perms on link or target - no target file */

	for (tperm = 0; tperm < MAX_PERM; tperm++) {
		if (!is_valid_perm_set(tperm))
			continue;
		build_filename(argv[1], tperm, tname);
		for (lperm = 0; lperm < MAX_PERM; lperm++) {
			if (!is_valid_perm_set(lperm))
				continue;
			build_filename(argv[2], lperm, lname);

			errno = 0;
			res = link(tname, lname) == 0;
			if (valid_link_perm_subset(tperm, lperm) != res) {
				printf("FAIL(%s) - link %s to %s (%s expected to %s)\n",
				       strerror(errno), lname, tname,
				       res ? "passed" : "failed", res ? "fail" : "pass");
				fail++;
//				if (fail > 5)
//				  return 1;
			} else {
				pass++;
			}
			if (res) {
			    if (change_hat("remove_link", SD_ID_MAGIC+1) == -1)
				printf("FAIL(%s) - failed change_hat remove_link\n", strerror(errno));
			    if (unlink(lname) != 0) {
				printf("FAIL(%s) - failed to remove link file %s\n", strerror(errno), lname);
			    }
			    if (change_hat(NULL, SD_ID_MAGIC+1) == -1)
				printf("FAIL(%s) - failed change_hat NULL\n", strerror(errno));
			}
		}
	}

	if (fail)
		printf("FAIL - %d of %d link subset tests failed\n", fail, fail + pass);
	else
		printf("PASS\n");

	return 0;
fail:
	printf("FAIL - %s\n", strerror(errno));
	return errno;
}
