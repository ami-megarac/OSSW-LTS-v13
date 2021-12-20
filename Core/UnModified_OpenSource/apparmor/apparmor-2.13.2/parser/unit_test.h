/*
 *   Copyright (c) 2013
 *   Canonical Ltd. (All rights reserved)
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
#ifndef __AA_UNIT_TEST_H
#define __AA_UNIT_TEST_H

#ifdef UNIT_TEST
/* For the unit-test builds, we must include function stubs for stuff that
 * only exists in the excluded object files; everything else should live
 * in parser_common.c.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <linux/limits.h>

#undef _
#define _(s) (s)

/* parser_yacc.y */
void yyerror(const char *msg, ...)
{
        va_list arg;
        char buf[PATH_MAX];

        va_start(arg, msg);
        vsnprintf(buf, sizeof(buf), msg, arg);
        va_end(arg);

        PERROR(_("AppArmor parser error: %s\n"), buf);

        exit(1);
}

#define MY_TEST(statement, error)               \
        if (!(statement)) {                     \
                PERROR("FAIL: %s\n", error);    \
                rc = 1;                         \
        }

#endif /* UNIT_TEST */

#endif /* __AA_UNIT_TEST_H */
