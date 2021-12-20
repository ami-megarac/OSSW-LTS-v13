#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME unlink
#=DESCRIPTION 
# In order to unlink a file, a confined process must have 'w' permission in
# it's profile for the relevant file.  This test verifies this.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/file
okperm=rwix
okperm2=w
nowriteperm=rixl

# PASS TEST

touch $file 
genprofile $file:$okperm

runchecktest "w/ WRITE PERM" pass $file
test -f $file && rm -f $file

# PASS TEST

touch $file
genprofile $file:$okperm2

runchecktest "w/ ONLY WRITE PERM" pass $file
test -f $file && rm -f $file

# NO WRITE PERMTEST

touch $file 
genprofile $file:$nowriteperm

runchecktest "No WRITE PERM" fail $file
test -f $file && rm -f $file
