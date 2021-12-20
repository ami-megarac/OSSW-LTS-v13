/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
 *
 *   Copyright (c) 2010 - 2014
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
 *   along with this program; if not, contact Novell, Inc. or Canonical,
 *   Ltd.
 */
#ifndef __AA_COMMON_OPTARG_H
#define __AA_COMMON_OPTARG_H

#include "libapparmor_re/apparmor_re.h"

/*
 * flag: 1 - allow no- inversion
 * flag: 2 - flags specified should be masked off
 */
typedef struct {
	int control;
	const char *option;
	const char *desc;
	dfaflags_t flags;
} optflag_table_t;

extern optflag_table_t dumpflag_table[];
extern optflag_table_t optflag_table[];

void print_flag_table(optflag_table_t *table);
int handle_flag_table(optflag_table_t *table, const char *optarg,
		      dfaflags_t *flags);
void display_dump(const char *command);
void display_optimize(const char *command);

#endif /* __AA_COMMON_OPTARG_H */

