#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME changehat_fork
#=DESCRIPTION 
# As 'changehat' but access checks for hats are verified across a fork
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/file
subfile=$tmpdir/file2
okperm=rw

touch $file $subfile

# NO CHANGEHAT TEST

genprofile $file:$okperm

runchecktest "NO CHANGEHAT (access parent file)" pass nochange $file
runchecktest "NO CHANGEHAT (access sub file)" fail nochange $subfile

# CHANGEHAT TEST
# Note: As of AppArmor 2.1 (opensuse 10.3) hats are no longer atomic
# to profile load/replacement so we need to remove them manually
subtest=sub

genprofile $file:$okperm hat:$subtest $subfile:$okperm

runchecktest "CHANGEHAT (access parent file 1)" fail $subtest $file
runchecktest "CHANGEHAT (access sub file)" pass $subtest $subfile
echo -n "${testexec}//${subtest}" >/sys/kernel/security/apparmor/.remove

# CHANGEHAT TEST -- multiple subprofiles

subtest2=sub2
subtest3=sub3

genprofile $file:$okperm hat:$subtest $subfile:$okperm hat:$subtest2 $subfile:$okperm hat:$subtest3 $subfile:$okperm

runchecktest "CHANGEHAT (access parent file 2)" fail $subtest $file
runchecktest "CHANGEHAT (access sub file)" pass $subtest $subfile
runchecktest "CHANGEHAT (access sub file)" pass $subtest2 $subfile
runchecktest "CHANGEHAT (access sub file)" pass $subtest3 $subfile
echo -n "${testexec}//${subtest}" >/sys/kernel/security/apparmor/.remove
echo -n "${testexec}//${subtest2}" >/sys/kernel/security/apparmor/.remove
echo -n "${testexec}//${subtest3}" >/sys/kernel/security/apparmor/.remove

# CHANGEHAT TEST -- non-existent subprofile access
# Should put us into a null-profile

# NOTE: As of AppArmor 2.1 (opensuse 10.3) this test now passes as
# the change_hat failes but it no longer entires the null profile
genprofile $file:$okperm hat:$subtest $subfile:$okperm hat:$subtest2 $subfile:$okperm

runchecktest "CHANGEHAT (access parent file 3)" pass $subtest3 $file
runchecktest "CHANGEHAT (access sub file)" fail $subtest3 $subfile
