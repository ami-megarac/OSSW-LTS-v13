#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME changeprofile
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
stackfile=$tmpdir/file3
okperm=rw

othertest="$pwd/rename"
subtest="sub"
fqsubbase="$pwd/changeprofile"
fqsubtest="$fqsubbase//$subtest"
subtest2="$pwd//sub2"
subtest3="$pwd//sub3"
nstest=":ns:changeprofile"


touch $file $subfile $stackfile

# CHANGEPROFILE UNCONFINED
runchecktest "CHANGEPROFILE (unconfined - nochange)" pass nochange $file
runchecktest_errno ENOENT "CHANGEPROFILE (unconfined - noent)" fail $othertest $file
genprofile image=$othertest $file:$okperm
runchecktest "CHANGEPROFILE (unconfined)" pass $othertest $file

# NO CHANGEPROFILE TEST file access of base profile okay
genprofile $file:$okperm
runchecktest "NO CHANGEPROFILE (access parent file)" pass nochange $file
runchecktest "NO CHANGEPROFILE (access sub file)" fail nochange $subfile

errno=EACCES
if [ "$(kernel_features domain/stack)" == "true" ]; then
	# The returned errno changed in the set of kernel patches that
	# introduced AppArmor profile stacking
	errno=ENOENT
fi

# CHANGEPROFILE NO Target TEST - NO PERMISSION and target does not exist
runchecktest "CHANGEPROFILE (no target, nochange)" pass nochange $file
runchecktest_errno $errno "CHANGEPROFILE (no target, $file)" fail $othertest $file
runchecktest_errno $errno "CHANGEPROFILE (no target, $subfile)" fail $othertest $subfile

# CHANGEPROFILE NO Target TEST - PERMISSION
genprofile $file:$okperm 'change_profile->':$othertest
runchecktest "CHANGEPROFILE (no target, nochange)" pass nochange $file
runchecktest_errno ENOENT "CHANGEPROFILE (no target, $file)" fail $othertest $file
runchecktest_errno ENOENT "CHANGEPROFILE (no target, $subfile)" fail $othertest $subfile


# CHANGEPROFILE TEST
genprofile $file:$okperm 'change_profile->':$fqsubtest hat:$subtest $subfile:$okperm
runchecktest "CHANGEPROFILE (nochange access file)" pass nochange $file
runchecktest_errno EACCES "CHANGEPROFILE (nochange access subfile)" fail nochange $subfile
runchecktest_errno EACCES "CHANGEPROFILE (access file)" fail $fqsubtest $file
runchecktest "CHANGEPROFILE (access sub file)" pass $fqsubtest $subfile


# CHANGEPROFILE RE TEST
genprofile $file:$okperm 'change_profile->':"$fqsubbase//*" hat:$subtest $subfile:$okperm
runchecktest "CHANGEPROFILE_RE (nochange access file)" pass nochange $file
runchecktest_errno EACCES "CHANGEPROFILE_RE (nochange access subfile)" fail nochange $subfile
runchecktest_errno EACCES "CHANGEPROFILE_RE (access file)" fail $fqsubtest $file
runchecktest "CHANGEPROFILE_RE (access sub file)" pass $fqsubtest $subfile

genprofile --stdin <<EOF
$test { file, change_profile -> ${nstest}, }
$nstest { $subfile ${okperm}, }
EOF
expected_result=pass
if [ "$(kernel_features domain/stack)" != "true" ]; then
	# Fails on older kernels due to namespaces not being well supported
	expected_result=xpass
fi
runchecktest "CHANGEPROFILE_NS (access sub file)" $expected_result $nstest $subfile
runchecktest "CHANGEPROFILE_NS (access file)" fail $nstest $file

if [ "$(kernel_features domain/stack)" != "true" ]; then
	echo "      WARNING: kernel does not support stacking, skipping tests ..."
else
	genprofile $file:$okperm $stackfile:$okperm 'change_profile->':"&$othertest" -- image=$othertest $subfile:$okperm $stackfile:$okperm
	runchecktest "CHANGEPROFILE_STACK (nochange access file)" pass nochange $file
	runchecktest "CHANGEPROFILE_STACK (nochange access sub file)" fail nochange $subfile
	runchecktest "CHANGEPROFILE_STACK (nochange access stack file)" pass nochange $stackfile
	runchecktest "CHANGEPROFILE_STACK (access sub file)" fail "&$othertest" $subfile
	runchecktest "CHANGEPROFILE_STACK (access file)" fail "&$othertest" $file
	runchecktest "CHANGEPROFILE_STACK (access stack file)" pass "&$othertest" $stackfile

	genprofile --stdin <<EOF
$test { file, audit deny $subfile $okperm, $stackfile $okperm, change_profile -> &${nstest}, }
$nstest { $subfile $okperm, $stackfile $okperm, }
EOF
	runchecktest "CHANGEPROFILE_NS_STACK (nochange access file)" pass nochange $file
	runchecktest "CHANGEPROFILE_NS_STACK (nochange access sub file)" fail nochange $subfile
	runchecktest "CHANGEPROFILE_NS_STACK (nochange access stack file)" pass nochange $stackfile
	runchecktest "CHANGEPROFILE_NS_STACK (access sub file)" fail "&$nstest" $subfile
	runchecktest "CHANGEPROFILE_NS_STACK (access file)" fail "&$nstest" $file
	runchecktest "CHANGEPROFILE_NS_STACK (access stack file)" pass "&$nstest" $stackfile
fi
