#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME symlink
#=DESCRIPTION creating a symlink should require write access to the new name

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

src=$tmpdir/src1
target=$tmpdir/target

okperm=w
badperm=rlixm

touch $target

# PASS TEST

genprofile ${src}:$okperm

runchecktest "SYMLINK/WRITE PERMS" pass $target ${src}

# FAILURE TEST

rm -f ${src}

genprofile ${src}:$badperm

runchecktest "SYMLINK/NO-WRITE PERMS" fail $target ${src}
