/*
 * Copyright (c) 1997-8 Andrew G. Morgan <morgan@kernel.org>
 *
 * This is a file to make the capability <-> string mappings for
 * libcap.
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * #include 'sed' generated array
 */

struct {
    const char *name;
    int index;
} const list[] = {
#include "cap_names.list.h"
    {NULL, -1}
};

int main(void)
{
    int i, maxcaps=0;
    const char **pointers = NULL, **pointers_tmp;
    int pointers_avail = 0;

    for ( i=0; list[i].index >= 0 && list[i].name; ++i ) {
	if (maxcaps <= list[i].index) {
	    maxcaps = list[i].index + 1;
	}
	if (list[i].index >= pointers_avail) {
            pointers_avail = 2 * list[i].index + 1;
	    pointers_tmp = realloc(pointers, pointers_avail * sizeof(char *));
	    if (!pointers_tmp) {
		fputs("out of memory", stderr);
		exit(1);
	    }
	    pointers = pointers_tmp;
	}
	pointers[list[i].index] = list[i].name;
    }

    printf("/*\n"
	   " * DO NOT EDIT: this file is generated automatically from\n"
	   " *\n"
	   " *     <linux/capability.h>\n"
	   " */\n"
	   "#define __CAP_BITS   %d\n"
	   "\n"
	   "#ifdef LIBCAP_PLEASE_INCLUDE_ARRAY\n"
	   "  char const *_cap_names[__CAP_BITS] = {\n", maxcaps);

    for (i=0; i<maxcaps; ++i) {
	if (pointers[i])
	    printf("      /* %d */\t\"%s\",\n", i, pointers[i]);
	else
	    printf("      /* %d */\tNULL,\t\t/* - presently unused */\n", i);
    }

    printf("  };\n"
	   "#endif /* LIBCAP_PLEASE_INCLUDE_ARRAY */\n"
	   "\n"
	   "/* END OF FILE */\n");

    free(pointers);
    exit(0);
}
