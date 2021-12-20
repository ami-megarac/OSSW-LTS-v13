#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME mkdir
#=DESCRIPTION mkdir() and rmdir() test

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

dir=$tmpdir/tmpdir/
perms=w
excess_perms=wl
badperms=r

# mkdir TEST

# null profile, verify mkdir/rmdir fail
genprofile

runchecktest "MKDIR (confined - no perms)" fail mkdir $dir

# yeah, looks like NOP, but pass/fail of first shouldn't affect second
# use || : to avoid shell trap error if dir doesn't exist
/bin/rmdir $dir 2> /dev/null || :
/bin/mkdir $dir

runchecktest "RMDIR (confined - no perms)" fail rmdir $dir

# profile with read-only permissions, fail

genprofile $dir:$badperms

/bin/rmdir $dir 2> /dev/null || :
runchecktest "MKDIR (confined read-only)" fail mkdir $dir

# yeah, looks like NOP, but pass/fail of first shouldn't affect second
/bin/rmdir $dir 2> /dev/null || :
/bin/mkdir $dir

runchecktest "RMDIR (confined read-only)" fail rmdir $dir

# profile with permissions, shouldn't fail

genprofile $dir:$perms

/bin/rmdir $dir 2> /dev/null || :
runchecktest "MKDIR (confined)" pass mkdir $dir

# yeah, looks like NOP, but pass/fail of first shouldn't affect second
/bin/rmdir $dir 2> /dev/null || :
/bin/mkdir $dir

runchecktest "RMDIR (confined)" pass rmdir $dir

# profile with excess permissions, shouldn't fail

genprofile $dir:$excess_perms

/bin/rmdir $dir 2> /dev/null || :
runchecktest "MKDIR (confined +l)" pass mkdir $dir

# yeah, looks like NOP, but pass/fail of first shouldn't affect second
/bin/rmdir $dir 2> /dev/null || :
/bin/mkdir $dir

runchecktest "RMDIR (confined +l)" pass rmdir $dir
