#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#	Copyright (C) 2017 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME readdir
#=DESCRIPTION 
# AppArmor requires 'r' permission on a directory in order for a confined task
# to be able to read the directory contents.  This test verifies this.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

dir=$tmpdir/tmpdir
# x is not really needed, see chdir.sh
okperm=rix
badperm=ix

mkdir $dir

# The readdir utility expects the return value to be passed as the
# second argument and returns success if the succeeding or failing calls
# match the expected value. It will fail the test if they don't, so for
# example the result differs acrorss getdents() and getdents64() this
# will be detected.

# READDIR TEST
genprofile $dir/:$okperm
runchecktest "READDIR" pass $dir 0

EACCES=13
# READDIR TEST (no perm)
genprofile $dir/:$badperm
runchecktest "READDIR (no perm)" pass $dir ${EACCES}

# READDIR TEST (write perm) - ensure write perm isn't sufficient
genprofile $dir/:w
runchecktest "READDIR (write perm)" pass $dir ${EACCES}

# this test is to make sure the raw 'file' rule allows access
# to directories
genprofile file
runchecktest "READDIR 'file' dir" pass $dir 0

# this test is to make sure the raw 'file' rule allows access
# to '/'
genprofile file
runchecktest "READDIR 'file' '/'" pass '/' 0
