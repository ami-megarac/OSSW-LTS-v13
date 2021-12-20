/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#include <alloca.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/apparmor.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct clone_arg {
	const char *put_old;
	const char *new_root;
	const char *expected_label;
};

static int _pivot_root(const char *new_root, const char *put_old)
{
#ifdef __NR_pivot_root
	return syscall(__NR_pivot_root, new_root, put_old);
#else
	errno = ENOSYS;
	return -1;
#endif
}

static int pivot_and_verify_label(void *arg)
{
	const char *put_old = ((struct clone_arg *)arg)->put_old;
	const char *new_root = ((struct clone_arg *)arg)->new_root;
	const char *expected_label = ((struct clone_arg *)arg)->expected_label;
	char *label;
	int rc;

	rc = chdir(new_root);
	if (rc < 0) {
		perror("FAIL - chdir");
		exit(100);
	}

	rc = _pivot_root(new_root, put_old);
	if (rc < 0) {
		perror("FAIL - pivot_root");
		exit(101);
	}

	rc = aa_getcon(&label, NULL);
	if (rc < 0) {
		perror("FAIL - aa_getcon");
		exit(102);
	}

	if (strcmp(expected_label, label)) {
		fprintf(stderr, "FAIL - expected_label (%s) != label (%s)\n",
			expected_label, label);
		exit(103);
	}

	free(label);
	exit(0);
}

static pid_t _clone(int (*fn)(void *), void *arg)
{
        size_t stack_size = sysconf(_SC_PAGESIZE);
        void *stack = alloca(stack_size);

#ifdef __ia64__
        return __clone2(pivot_and_verify_label, stack,  stack_size,
			CLONE_NEWNS | SIGCHLD, arg);
#else
        return    clone(pivot_and_verify_label, stack + stack_size,
			CLONE_NEWNS | SIGCHLD, arg);
#endif
}

int main(int argc, char **argv)
{
	struct clone_arg arg;
	pid_t child;
	int child_status, rc;

	if (argc != 4) {
		fprintf(stderr,
			"FAIL - usage: %s <PUT_OLD> <NEW_ROOT> <LABEL>\n\n"
			"  <PUT_OLD>\t\tThe put_old param of pivot_root()\n"
			"  <NEW_ROOT>\t\tThe new_root param of pivot_root()\n"
			"  <LABEL>\t\tThe expected AA label after pivoting\n\n"
			"This program clones itself in a new mount namespace, \n"
			"does a pivot and then calls aa_getcon(). The test fails \n"
			"if <LABEL> does not match the label returned by \n"
			"aa_getcon().\n", argv[0]);
		exit(1);
	}

	arg.put_old      = argv[1];
	arg.new_root     = argv[2];
	arg.expected_label = argv[3];

	child = _clone(pivot_and_verify_label, &arg);
	if (child < 0) {
		perror("FAIL - clone");
		exit(2);
	}

	rc = waitpid(child, &child_status, 0);
	if (rc < 0) {
		perror("FAIL - waitpid");
		exit(3);
	} else if (!WIFEXITED(child_status)) {
		fprintf(stderr, "FAIL - child didn't exit\n");
		exit(4);
	} else if (WEXITSTATUS(child_status)) {
		/* The child has already printed a FAIL message */
		exit(WEXITSTATUS(child_status));
	}

	printf("PASS\n");
	exit (0);
}
