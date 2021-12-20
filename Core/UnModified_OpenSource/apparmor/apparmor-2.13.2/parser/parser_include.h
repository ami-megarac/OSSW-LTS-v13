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

#ifndef PARSER_INCLUDE_H
#define PARSER_INCLUDE_H

extern int preprocess_only;

extern int add_search_dir(const char *dir);
extern void init_base_dir(void);
extern void set_base_dir(char *dir);
extern void parse_default_paths(void);
extern int do_include_preprocessing(char *profilename);
FILE *search_path(char *filename, char **fullpath);

extern void push_include_stack(char *filename);
extern void pop_include_stack(void);
extern void reset_include_stack(const char *filename);

#endif
