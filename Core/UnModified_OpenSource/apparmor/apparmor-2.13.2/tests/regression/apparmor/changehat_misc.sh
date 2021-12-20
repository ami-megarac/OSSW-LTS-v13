#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME changehat_misc
#=DESCRIPTION 
# Variety of tests verifying entry to subprofiles and return back to parent.   
# AppArmor has rigid requirements around the correct use of the magic# token 
# passed to changehat.
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

touch $file $subfile

# NO CHANGEHAT TEST

genprofile $file:$okperm

runchecktest "NO CHANGEHAT (access parent file)" pass nochange $file
runchecktest "NO CHANGEHAT (access sub file)" fail nochange $subfile

# CHANGEHAT TEST - test if can enter and exit a subprofile

genprofile $file:$okperm hat:$subtest $subfile:$okperm

runchecktest "CHANGEHAT (access parent file)" pass $subtest $file
runchecktest "CHANGEHAT (access sub file)" fail $subtest $subfile

# CHANGEHAT - test enter subprofile -> fork -> exit subprofile
settest changehat_misc2

genprofile $file:$okperm hat:$subtest $subfile:$okperm

runchecktest "FORK BETWEEN CHANGEHATS (access parent file)" pass $subtest $file
runchecktest "FORK BETWEEN CHANGEHATS (access sub file)" fail $subtest $subfile

# CHANGEHAT FROM ONE SUBPROFILE TO ANOTHER
settest changehat_twice

genprofile hat:$subtest $subfile:$okperm hat:$subtest2 $file:$okperm

runchecktest "CHANGEHAT (subprofile->subprofile)" pass $subtest $subtest2 goodmagic $file

echo
echo "*** A 'Killed' message from bash is expected for the following test"
runchecktest "CHANGEHAT (subprofile->subprofile w/ bad magic)" signal9 $subtest $subtest2 badmagic $file

# 1. ATTEMPT TO CHANGEHAT TO AN INVALID PROFILE, SHOULD PUT US INTO A NULL
#    PROFILE
# 2. ATTEMPT TO CHANGEHAT OUT WITH BAD TOKEN
settest changehat_fail

genprofile hat:$subtest

runchecktest "CHANGEHAT (bad subprofile)" fail ${subtest2}

echo
echo "*** A 'Killed' message from bash is expected for the following test"
runchecktest "CHANGEHAT (bad token)" signal9 ${subtest}

settest changehat_wrapper

genprofile hat:open addimage:${bin}/open ${file}:${okperm}

runchecktest "CHANGEHAT (noexit subprofile (token=0))" pass --token=0 open ${file}
runchecktest "CHANGEHAT (exit noexit subprofile (token=0))" fail --token=0 --exit_hat open ${file}

if [ -f /proc/self/attr/current ] ; then
# do some tests of writing directly to /proc/self/attr/current, skipping
# libimmunx. Only do these tests if the extended attribute exists.
runchecktest "CHANGEHAT (subprofile/write to /proc/attr/current)" pass --manual=123456 open ${file}
runchecktest "CHANGEHAT (exit subprofile/write to /proc/attr/current)" pass --manual=123456 --exit_hat open ${file}
runchecktest "CHANGEHAT (noexit subprofile/write 0 to /proc/attr/current)" pass --manual=0 open ${file}
runchecktest "CHANGEHAT (noexit subprofile/write 00000000 to /proc/attr/current)" pass --manual=00000000 open ${file}
# verify that the kernel accepts the command "changehat ^hat\0" and
# treats an empty token as 0.
runchecktest "CHANGEHAT (noexit subprofile/write \"\" to /proc/attr/current)" fail --manual="" open ${file}
runchecktest "CHANGEHAT (exit of noexit subprofile/write 0 to /proc/attr/current)" fail --manual=0 --exit_hat open ${file}
runchecktest "CHANGEHAT (exit of noexit subprofile/write 00000000 to /proc/attr/current)" fail --manual=00000000 --exit_hat open ${file}
runchecktest "CHANGEHAT (exit of noexit subprofile/write \"\" to /proc/attr/current)" fail --manual="" --exit_hat open ${file}
fi

# CHANGEHAT and PTHREADS: test that change_hat functions correctly in
# the presence of threads
settest changehat_pthread
genprofile $file:$okperm "/proc/**:w" hat:fez $subfile:$okperm "/proc/**:w"
runchecktest "CHANGEHAT PTHREAD (access parent file)" fail $file
runchecktest "CHANGEHAT PTHREAD (access sub file)" pass $subfile
