/*
 * Copyright (c) 1999-2008 NOVELL (All rights reserved)
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <aalogparse.h>
#include "parser.h"
#include "private.h"

int main(void)
{
	int rc = 0;
	char *retstr = NULL;

	/* hex_to_string() tests */
	retstr = hex_to_string(NULL);
	MY_TEST(!retstr, "basic NULL test");

	retstr = hex_to_string("2F746D702F646F6573206E6F74206578697374");
	MY_TEST(retstr, "basic allocation");
	MY_TEST(strcmp(retstr, "/tmp/does not exist") == 0, "basic dehex 1");
	free(retstr);

	retstr = hex_to_string("61");
	MY_TEST(strcmp(retstr, "a") == 0, "basic dehex 2");
	free(retstr);

	retstr = hex_to_string("");
	MY_TEST(strcmp(retstr, "") == 0, "empty string");
	free(retstr);

	/* ipproto_to_string() tests */
	retstr = ipproto_to_string((unsigned) 99999);
	MY_TEST(strcmp(retstr, "unknown(99999)") == 0, "invalid protocol test");
	free(retstr);

	retstr = ipproto_to_string((unsigned) 6);
	MY_TEST(strcmp(retstr, "tcp") == 0, "protocol=tcp");
	free(retstr);

	return rc;
}


