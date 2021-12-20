/*  -*- Mode: C -*-  */

/* mem.c --- memory management routines
 * Copyright (C) 2002 Gary V. Vaughan
 * Originally by Paolo Bonzini, 2002
 * This file is part of Snprintfv
 *
 * Snprintfv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Snprintfv program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * As a special exception to the GNU General Public License, if you
 * distribute this file as part of a program that also links with and
 * uses the libopts library from AutoGen, you may include it under
 * the same distribution terms used by the libopts library.
 */

/* Code: */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_DMALLOC
#  include <dmalloc.h>
#endif

#include "mem.h"


/* We deliberately don't prototype the malloc functions;  they are cast
   to match the function pointers we expose to avoid compiler warnings
   from mismatched prototypes (if we find a host implementation.

   Not also that if this file is compiled -DWITH_DMALLOC, the inclusion
   in mem.h will cause the malloc references below to be redirected
   correctly. */
malloc_proc_t*  snv_malloc  = (malloc_proc_t*)malloc;
realloc_proc_t* snv_realloc = (realloc_proc_t*)realloc;
free_proc_t*    snv_free    = (free_proc_t*)free;

/* Unportable memory management functions are reimplemented tout court.  */
snv_pointer
snv_xrealloc (snv_pointer old, size_t count)
{
  if (count < 1)
    {
      snv_free (old);
      return NULL;
    }
  if (!old)
    return snv_malloc (count);
  else
    return snv_realloc (old, count);
}

char *
snv_strdup (const char *str)
{
  size_t len = strlen (str);
  char *result = snv_malloc (len + 1);
  memcpy (result, str, len + 1);
  return result;
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "gnu"
 * indent-tabs-mode: nil
 * End:
 * end of snprintfv/mem.c */
