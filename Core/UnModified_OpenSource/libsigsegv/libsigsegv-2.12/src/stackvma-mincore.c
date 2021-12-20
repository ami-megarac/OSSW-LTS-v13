/* Determine the virtual memory area of a given address.
   Copyright (C) 2006, 2008-2010, 2016-2018  Bruno Haible <bruno@clisp.org>

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

/* mincore() is a system call that allows to inquire the status of a
   range of pages of virtual memory.  In particular, it allows to inquire
   whether a page is mapped at all (except on Mac OS X, where mincore
   returns 0 even for unmapped addresses).
   As of 2006, mincore() is supported by:        possible bits:
     - Linux,   since Linux 2.4 and glibc 2.2,   1
     - Solaris, since Solaris 9,                 1
     - MacOS X, since MacOS X 10.3 (at least),   1
     - FreeBSD, since FreeBSD 6.0,               MINCORE_{INCORE,REFERENCED,MODIFIED}
     - NetBSD,  since NetBSD 3.0 (at least),     1
     - OpenBSD, since OpenBSD 2.6 (at least),    1
     - AIX,     since AIX 5.3,                   1
   However, while the API allows to easily determine the bounds of mapped
   virtual memory, it does not make it easy to find the bounds of _unmapped_
   virtual memory ranges.  We try to work around this, but it may still be
   slow.  */

#include "stackvma.h"
#include <limits.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/mman.h>

/* The AIX declaration of mincore() uses 'caddr_t', whereas the other platforms
   use 'void *'. */
#ifdef UNIX_AIX
typedef caddr_t MINCORE_ADDR_T;
#else
typedef void* MINCORE_ADDR_T;
#endif

/* The glibc declaration of mincore() uses 'unsigned char *', whereas the BSD
   declaration uses 'char *'.  */
#if __GLIBC__ >= 2
typedef unsigned char pageinfo_t;
#else
typedef char pageinfo_t;
#endif

/* Cache for getpagesize().  */
static uintptr_t pagesize;

/* Initialize pagesize.  */
static void
init_pagesize (void)
{
#if HAVE_GETPAGESIZE
  pagesize = getpagesize ();
#elif HAVE_SYSCONF_PAGESIZE
  pagesize = sysconf (_SC_PAGESIZE);
#else
  pagesize = PAGESIZE;
#endif
}

/* Test whether the page starting at ADDR is among the address range.
   ADDR must be a multiple of pagesize.  */
static int
is_mapped (uintptr_t addr)
{
  pageinfo_t vec[1];
  return mincore ((MINCORE_ADDR_T) addr, pagesize, vec) >= 0;
}

/* Assuming that the page starting at ADDR is among the address range,
   return the start of its virtual memory range.
   ADDR must be a multiple of pagesize.  */
static uintptr_t
mapped_range_start (uintptr_t addr)
{
  /* Use a moderately sized VEC here, small enough that it fits on the stack
     (without requiring malloc).  */
  pageinfo_t vec[1024];
  uintptr_t stepsize = sizeof (vec);

  for (;;)
    {
      uintptr_t max_remaining;

      if (addr == 0)
        return addr;

      max_remaining = addr / pagesize;
      if (stepsize > max_remaining)
        stepsize = max_remaining;
      if (mincore ((MINCORE_ADDR_T) (addr - stepsize * pagesize),
                   stepsize * pagesize, vec) < 0)
        /* Time to search in smaller steps.  */
        break;
      /* The entire range exists.  Continue searching in large steps.  */
      addr -= stepsize * pagesize;
    }
  for (;;)
    {
      uintptr_t halfstepsize1;
      uintptr_t halfstepsize2;

      if (stepsize == 1)
        return addr;

      /* Here we know that less than stepsize pages exist starting at addr.  */
      halfstepsize1 = (stepsize + 1) / 2;
      halfstepsize2 = stepsize / 2;
      /* halfstepsize1 + halfstepsize2 = stepsize.  */

      if (mincore ((MINCORE_ADDR_T) (addr - halfstepsize1 * pagesize),
                   halfstepsize1 * pagesize, vec) < 0)
        stepsize = halfstepsize1;
      else
        {
          addr -= halfstepsize1 * pagesize;
          stepsize = halfstepsize2;
        }
    }
}

/* Assuming that the page starting at ADDR is among the address range,
   return the end of its virtual memory range + 1.
   ADDR must be a multiple of pagesize.  */
static uintptr_t
mapped_range_end (uintptr_t addr)
{
  /* Use a moderately sized VEC here, small enough that it fits on the stack
     (without requiring malloc).  */
  pageinfo_t vec[1024];
  uintptr_t stepsize = sizeof (vec);

  addr += pagesize;
  for (;;)
    {
      uintptr_t max_remaining;

      if (addr == 0) /* wrapped around? */
        return addr;

      max_remaining = (- addr) / pagesize;
      if (stepsize > max_remaining)
        stepsize = max_remaining;
      if (mincore ((MINCORE_ADDR_T) addr, stepsize * pagesize, vec) < 0)
        /* Time to search in smaller steps.  */
        break;
      /* The entire range exists.  Continue searching in large steps.  */
      addr += stepsize * pagesize;
    }
  for (;;)
    {
      uintptr_t halfstepsize1;
      uintptr_t halfstepsize2;

      if (stepsize == 1)
        return addr;

      /* Here we know that less than stepsize pages exist starting at addr.  */
      halfstepsize1 = (stepsize + 1) / 2;
      halfstepsize2 = stepsize / 2;
      /* halfstepsize1 + halfstepsize2 = stepsize.  */

      if (mincore ((MINCORE_ADDR_T) addr, halfstepsize1 * pagesize, vec) < 0)
        stepsize = halfstepsize1;
      else
        {
          addr += halfstepsize1 * pagesize;
          stepsize = halfstepsize2;
        }
    }
}

/* Determine whether an address range [ADDR1..ADDR2] is completely unmapped.
   ADDR1 must be <= ADDR2.  */
static int
is_unmapped (uintptr_t addr1, uintptr_t addr2)
{
  uintptr_t count;
  uintptr_t stepsize;

  /* Round addr1 down.  */
  addr1 = (addr1 / pagesize) * pagesize;
  /* Round addr2 up and turn it into an exclusive bound.  */
  addr2 = ((addr2 / pagesize) + 1) * pagesize;

  /* This is slow: mincore() does not provide a way to determine the bounds
     of the gaps directly.  So we have to use mincore() on individual pages
     over and over again.  Only after we've verified that all pages are
     unmapped, we know that the range is completely unmapped.
     If we were to traverse the pages from bottom to top or from top to bottom,
     it would be slow even in the average case.  To speed up the search, we
     exploit the fact that mapped memory ranges are larger than one page on
     average, therefore we have good chances of hitting a mapped area if we
     traverse only every second, or only fourth page, etc.  This doesn't
     decrease the worst-case runtime, only the average runtime.  */
  count = (addr2 - addr1) / pagesize;
  /* We have to test is_mapped (addr1 + i * pagesize) for 0 <= i < count.  */
  for (stepsize = 1; stepsize < count; )
    stepsize = 2 * stepsize;
  for (;;)
    {
      uintptr_t addr_stepsize;
      uintptr_t i;
      uintptr_t addr;

      stepsize = stepsize / 2;
      if (stepsize == 0)
        break;
      addr_stepsize = stepsize * pagesize;
      for (i = stepsize, addr = addr1 + addr_stepsize;
           i < count;
           i += 2 * stepsize, addr += 2 * addr_stepsize)
        /* Here addr = addr1 + i * pagesize.  */
        if (is_mapped (addr))
          return 0;
    }
  return 1;
}

#if STACK_DIRECTION < 0

/* Info about the gap between this VMA and the previous one.
   addr must be < vma->start.  */
static int
mincore_is_near_this (uintptr_t addr, struct vma_struct *vma)
{
  /*   vma->start - addr <= (vma->start - vma->prev_end) / 2
     is mathematically equivalent to
       vma->prev_end <= 2 * addr - vma->start
     <==> is_unmapped (2 * addr - vma->start, vma->start - 1).
     But be careful about overflow: if 2 * addr - vma->start is negative,
     we consider a tiny "guard page" mapping [0, 0] to be present around
     NULL; it intersects the range (2 * addr - vma->start, vma->start - 1),
     therefore return false.  */
  uintptr_t testaddr = addr - (vma->start - addr);
  if (testaddr > addr) /* overflow? */
    return 0;
  /* Here testaddr <= addr < vma->start.  */
  return is_unmapped (testaddr, vma->start - 1);
}

#endif
#if STACK_DIRECTION > 0

/* Info about the gap between this VMA and the next one.
   addr must be > vma->end - 1.  */
static int
mincore_is_near_this (uintptr_t addr, struct vma_struct *vma)
{
  /*   addr - vma->end < (vma->next_start - vma->end) / 2
     is mathematically equivalent to
       vma->next_start > 2 * addr - vma->end
     <==> is_unmapped (vma->end, 2 * addr - vma->end).
     But be careful about overflow: if 2 * addr - vma->end is > ~0UL,
     we consider a tiny "guard page" mapping [0, 0] to be present around
     NULL; it intersects the range (vma->end, 2 * addr - vma->end),
     therefore return false.  */
  uintptr_t testaddr = addr + (addr - vma->end);
  if (testaddr < addr) /* overflow? */
    return 0;
  /* Here vma->end - 1 < addr <= testaddr.  */
  return is_unmapped (vma->end, testaddr);
}

#endif

#ifdef STATIC
STATIC
#endif
int
sigsegv_get_vma (uintptr_t address, struct vma_struct *vma)
{
  if (pagesize == 0)
    init_pagesize ();
  address = (address / pagesize) * pagesize;
  vma->start = mapped_range_start (address);
  vma->end = mapped_range_end (address);
  vma->is_near_this = mincore_is_near_this;
  return 0;
}
