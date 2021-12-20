#include <stdio.h>
int *ptr;

/*
 *	Copyright (C) 2002-2005 Novell/SUSE
 *	Copyright (C) 2010 Canonical, Ltd
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */

int main(int argc, char *argv[])
{
	printf("This will cause a sigsegv\n");

	ptr=0;

	*ptr=0xdeadbeef;

	return 0;
}
