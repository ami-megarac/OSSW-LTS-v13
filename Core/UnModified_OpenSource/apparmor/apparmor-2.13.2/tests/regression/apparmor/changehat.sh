#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME changehat
#=DESCRIPTION 
# Verifies basic file access permission checks for a parent profile and one 
# subprofile/hat
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/file
subfile=$tmpdir/file2
okperm=rw

subtest=sub
subtest2=sub2
subtest3=sub3

touch $file $subfile

# CHANGEHAT UNCONFINED
runchecktest "CHANGEHAT (unconfined - nochange)" pass nochange $file
runchecktest_errno EPERM "CHANGEHAT (unconfined)" fail $subtest $file

# NO CHANGEHAT TEST
genprofile $file:$okperm

runchecktest "NO CHANGEHAT (access parent file)" pass nochange $file
runchecktest "NO CHANGEHAT (access sub file)" fail nochange $subfile

# CHANGEHAT NO HATS TEST
runchecktest "CHAGEHAT (no hats, nochange)" pass nochange $file
runchecktest_errno ECHILD "CHANGEHAT (no hats, $file)" fail $subtest $file
runchecktest_errno ECHILD "CHANGEHAT (no hats, $subfile)" fail $subtest $subfile

# CHANGEHAT TEST

genprofile $file:$okperm hat:$subtest $subfile:$okperm

runchecktest "CHANGEHAT (access parent file)" fail $subtest $file
runchecktest "CHANGEHAT (access sub file)" pass $subtest $subfile

# CHANGEHAT TEST -- multiple subprofiles

genprofile $file:$okperm hat:$subtest $subfile:$okperm hat:$subtest2 $subfile:$okperm hat:$subtest3 $subfile:$okperm

runchecktest "CHANGEHAT (access parent file)" fail $subtest $file
runchecktest "CHANGEHAT (access sub file)" pass $subtest $subfile
runchecktest "CHANGEHAT (access sub file)" pass $subtest2 $subfile
runchecktest "CHANGEHAT (access sub file)" pass $subtest3 $subfile

