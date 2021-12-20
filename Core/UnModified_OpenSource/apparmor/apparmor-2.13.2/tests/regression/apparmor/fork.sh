#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME fork
#=DESCRIPTION 
# Verifies that profiles are duplicated correctly for fork (the subtask 
# receives a copy of it's parents profile).  The test attempts to access the 
# files passed as arguments for both a parent and a child.    The test is 
# repeated for permissive and restrictive profiles.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file1=$tmpdir/file1
file2=$tmpdir/file2
file3=$tmpdir/file3
cap=ipc_owner
okperm=rw
badperm=r

touch $file1 $file2 $file3

# TEST1

genprofile cap:$cap $file1:$okperm $file2:$okperm $file3:$okperm

runchecktest "FORK with rw" pass $file1 $file2 $file3

# TEST2.  All will fail, but parent and child will equally fail

genprofile cap:$cap $file1:$badperm $file2:$badperm $file3:$badperm

runchecktest "FORK w/o w" pass $file1 $file2 $file3

# TEST3.  Some pass, some fail, but parent and child will equally fail

genprofile cap:$cap $file1:$badperm $file2:$okperm $file3:$badperm

runchecktest "FORK mixed" pass $file1 $file2 $file3
