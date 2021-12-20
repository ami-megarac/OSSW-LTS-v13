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
#include <linux/module.h>

#define BUFSIZE 4096
int main(int argc, char *argv[])
{
	char buffer[BUFSIZE];
	size_t ret = 0;
	struct module_info modinfo;
	
	if (argc > 2) {
		fprintf(stderr, "Usage: %s [module]\n",
			argv[0]);
		return 1;
	} else if (argc == 1) {
		if (query_module(NULL, QM_MODULES, &buffer, BUFSIZE,
				&ret) == -1) {
			fprintf(stderr, "FAIL: query_module failed - %s\n",
				strerror(errno));
			return 1;
		}
		/* printf ("First module: %s\n", buffer); */
	} else {
		if (query_module(argv[1], QM_INFO, &modinfo, sizeof(modinfo),
				&ret) == -1) {
			fprintf(stderr, "FAIL: query_module failed - %s\n",
				strerror(errno));
			return 1;
		}
		/* printf ("module %s info: address 0x%8x size %u flags 0x%8x "
				"usecount %d\n", 
				argv[1],
				modinfo.addr,
				modinfo.size,
				modinfo.flags,
				modinfo.usecount); */
	}

	/* printf("Kernel release version is %s\n", release); */
	printf("PASS\n");

	return 0;
}
