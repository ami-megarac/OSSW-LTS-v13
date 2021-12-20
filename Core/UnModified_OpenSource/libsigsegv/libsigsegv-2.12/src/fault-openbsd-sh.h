/* Fault handler information.  OpenBSD/SH version.
   Copyright (C) 2010  Bruno Haible <bruno@clisp.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "fault-openbsd.h"

#include <sys/param.h> /* defines macro OpenBSD */

/* See the definition of 'struct sigcontext' in
   openbsd-src/sys/arch/sh/include/signal.h
   and the definition of 'struct reg' in
   openbsd-src/sys/arch/sh/include/reg.h.  */

#if OpenBSD >= 201211 /* OpenBSD version >= 5.2 */
# define SIGSEGV_FAULT_STACKPOINTER  scp->sc_reg[20-15]
#else
# define SIGSEGV_FAULT_STACKPOINTER  scp->sc_reg.r_r15
#endif
