#!/bin/sh
# Probe FreeBSD UFS file system

# Copyright (C) 2010 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

if test "$VERBOSE" = yes; then
  set -x
  parted --version
fi

: ${srcdir=.}
. $srcdir/t-lib.sh
require_512_byte_sector_size_

dev=loop-file
ss=$sector_size_
n_sectors=8000

fail=0

( type mkfs.ufs ) >/dev/null 2>&1 || skip_test_ "no freebsd-ufs support"

# create a freebsd-ufs file system
dd if=/dev/zero of=$dev bs=1024 count=4096 >/dev/null || fail=1
mkfs.ufs `pwd`/$dev >/dev/null || fail=1

# probe the freebsd-ufs file system
parted -m -s $dev u s print >out 2>&1 || fail=1
grep '^1:.*:freebsd-ufs::;$' out || fail=1

Exit $fail
