#! /bin/bash
#	Copyright (C) 2016 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME stackprofile
#=DESCRIPTION
# Verifies basic file access permission checks for a parent profile and a
# stacked subprofile
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

requires_kernel_features domain/stack
settest transition

file=$tmpdir/file
otherfile=$tmpdir/file2
thirdfile=$tmpdir/file3
sharedfile=$tmpdir/file.shared
okperm=rw

fileok="${file}:${okperm}"
otherok="${otherfile}:${okperm}"
thirdok="${thirdfile}:${okperm}"
sharedok="${sharedfile}:${okperm}"

getcon="/proc/*/attr/current:r"

othertest="$pwd/rename"
thirdtest="$pwd/exec"

stackotherok="change_profile->:&$othertest"
stackthirdok="change_profile->:&$thirdtest"

touch $file $otherfile $sharedfile $thirdfile

# Verify file access and contexts by an unconfined process
runchecktest "STACKPROFILE (unconfined - file)" pass -f $file
runchecktest "STACKPROFILE (unconfined - otherfile)" pass -f $otherfile
runchecktest "STACKPROFILE (unconfined - thirdfile)" pass -f $thirdfile
runchecktest "STACKPROFILE (unconfined - sharedfile)" pass -f $sharedfile

runchecktest "STACKPROFILE (unconfined - okcon)" pass -l unconfined -m '(null)'
runchecktest "STACKPROFILE (unconfined - bad label)" fail -l "$test" -m '(null)'
runchecktest "STACKPROFILE (unconfined - bad mode)" fail -l unconfined -m enforce

# Verify file access and contexts by a non-stacked profile
genprofile $fileok $sharedok $getcon
runchecktest "STACKPROFILE (not stacked - file)" pass -f $file
runchecktest_errno EACCES "STACKPROFILE (not stacked - otherfile)" fail -f $otherfile
runchecktest_errno EACCES "STACKPROFILE (not stacked - thirdfile)" fail -f $thirdfile
runchecktest "STACKPROFILE (not stacked - sharedfile)" pass -f $sharedfile

runchecktest "STACKPROFILE (not stacked - okcon)" pass -l "$test" -m enforce
runchecktest "STACKPROFILE (not stacked - bad label)" fail -l "${test}XXX" -m enforce
runchecktest "STACKPROFILE (not stacked - bad mode)" fail -l "$test" -m complain

# Verify file access and contexts by a profile stacked with unconfined
genprofile image=$othertest $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKPROFILE (stacked with unconfined - file)" fail -p $othertest -f $file
runchecktest "STACKPROFILE (stacked with unconfined - otherfile)" pass -p $othertest -f $otherfile
runchecktest "STACKPROFILE (stacked with unconfined - sharedfile)" pass -p $othertest -f $sharedfile

runchecktest "STACKPROFILE (stacked with unconfined - okcon)" pass -p $othertest -l "unconfined//&${othertest}" -m enforce
runchecktest "STACKPROFILE (stacked with unconfined - bad label)" fail -p $othertest -l "${test}//&${othertest}" -m enforce
runchecktest "STACKPROFILE (stacked with unconfined - bad mode)" fail -p $othertest -l "unconfined//&${othertest}" -m '(null)'

removeprofile
# Verify that stacking a nonexistent file is properly handled
runchecktest_errno ENOENT "STACKPROFILE (unconfined - stack nonexistent profile)" fail -p $othertest -f $file

# Verify file access and contexts by 2 stacked profiles
genprofile $fileok $sharedok $getcon $stackotherok -- \
	image=$othertest $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKPROFILE (2 stacked - file)" fail -p $othertest -f $file
runchecktest_errno EACCES "STACKPROFILE (2 stacked - otherfile)" fail -p $othertest -f $otherfile
runchecktest_errno EACCES "STACKPROFILE (2 stacked - thirdfile)" fail -p $othertest -f $thirdfile
runchecktest "STACKPROFILE (2 stacked - sharedfile)" pass -p $othertest -f $sharedfile

runchecktest "STACKPROFILE (2 stacked - okcon)" pass -p $othertest -l "${test}//&${othertest}" -m enforce
runchecktest "STACKPROFILE (2 stacked - bad label)" fail -p $othertest -l "${test}//&${test}" -m enforce
runchecktest "STACKPROFILE (2 stacked - bad mode)" fail -p $othertest -l "${test}//&${test}" -m '(null)'

# Verify that a change_profile rule is required to aa_stack_profile())
genprofile $fileok $sharedok $getcon -- \
	image=$othertest $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKPROFILE (2 stacked - no change_profile)" fail -p $othertest -l "${test}//&${othertest}" -m enforce

# Verify file access and contexts by 3 stacked profiles
genprofile $fileok $sharedok $getcon $stackotherok $stackthirdok -- \
	image=$othertest addimage:$test $otherok $sharedok $getcon $stackthirdok -- \
	image=$thirdtest $thirdok $sharedok $getcon
runchecktest_errno EACCES "STACKPROFILE (3 stacked - file)" fail -p $othertest -- $test -p $thirdtest -f $file
runchecktest_errno EACCES "STACKPROFILE (3 stacked - otherfile)" fail -p $othertest -- $test -p $thirdtest -f $otherfile
runchecktest_errno EACCES "STACKPROFILE (3 stacked - thirdfile)" fail -p $othertest -- $test -p $thirdtest -f $thirdfile
runchecktest "STACKPROFILE (3 stacked - sharedfile)" pass -p $othertest -- $test -p $thirdtest -f $sharedfile

runchecktest "STACKPROFILE (3 stacked - okcon)" pass -p $othertest -- $test -p $thirdtest -l "${thirdtest}//&${test}//&${othertest}" -m enforce

genprofile $fileok $sharedok $getcon $stackotherok -- \
	image=$othertest addimage:$test $otherok $sharedok $getcon $stackthirdok -- \
	image=$thirdtest $thirdok $sharedok $getcon
runchecktest_errno EACCES "STACKPROFILE (3 stacked - sharedfile - no change_profile)" fail -p $othertest -- $test -p $thirdtest -f $sharedfile

ns="ns"
prof="stackprofile"
nstest=":${ns}:${prof}"
# Verify file access and contexts by stacking a profile with a namespaced profile
genprofile --stdin <<EOF
$test {
  file,
  audit deny $otherfile $okperm,
  change_profile -> &$nstest,
}

$nstest {
  $otherfile $okperm,
  $sharedfile $okperm,
  /proc/*/attr/current r,
}
EOF
runchecktest_errno EACCES "STACKPROFILE (stacked with namespaced profile - file)" fail -p $nstest -f $file
runchecktest_errno EACCES "STACKPROFILE (stacked with namespaced profile - otherfile)" fail -p $nstest -f $otherfile
runchecktest_errno EACCES "STACKPROFILE (stacked with namespaced profile - thirdfile)" fail -p $nstest -f $thirdfile
runchecktest "STACKPROFILE (stacked with namespaced profile - sharedfile)" pass -p $nstest -f $sharedfile

runchecktest "STACKPROFILE (stacked with namespaced profile - okcon)" pass -p $nstest -l $prof -m enforce

# Verify file access and contexts in mixed mode
genprofile $fileok $sharedok $getcon $stackotherok -- \
	image=$othertest flag:complain $otherok $sharedok $getcon
runchecktest "STACKPROFILE (mixed mode - file)" pass -p $othertest -f $file
runchecktest_errno EACCES "STACKPROFILE (mixed mode - otherfile)" fail -p $othertest -f $otherfile
runchecktest "STACKPROFILE (mixed mode - sharedfile)" pass -p $othertest -f $sharedfile

runchecktest "STACKPROFILE (mixed mode - okcon)" pass -p $othertest -l "${othertest}//&${test}" -m mixed

genprofile $fileok $sharedok $getcon -- \
	image=$othertest flag:complain $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKPROFILE (mixed mode - okcon - no change_profile)" fail -p $othertest -l "${othertest}//&${test}" -m mixed

genprofile flag:complain $fileok $sharedok $getcon $stackotherok -- \
	image=$othertest $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKPROFILE (mixed mode 2 - file)" fail -p $othertest -f $file
runchecktest "STACKPROFILE (mixed mode 2 - otherfile)" pass -p $othertest -f $otherfile
runchecktest "STACKPROFILE (mixed mode 2 - sharedfile)" pass -p $othertest -f $sharedfile

runchecktest "STACKPROFILE (mixed mode 2 - okcon)" pass -p $othertest -l "${othertest}//&${test}" -m mixed

genprofile flag:complain $fileok $sharedok $getcon -- \
	image=$othertest $otherok $sharedok $getcon
runchecktest "STACKPROFILE (mixed mode 2 - okcon - no change_profile)" pass -p $othertest -l "${othertest}//&${test}" -m mixed

# Verify file access and contexts in complain mode
genprofile flag:complain $getcon -- image=$othertest flag:complain $getcon
runchecktest "STACKPROFILE (complain mode - file)" pass -p $othertest -f $file

runchecktest "STACKPROFILE (complain mode - okcon)" pass -p $othertest -l "${test}//&${othertest}" -m complain

# Verify that stacking with a bare namespace is handled
genprofile --stdin <<EOF
$test { file, change_profile, }
$nstest { }
EOF
runchecktest "STACKPROFILE (bare :ns:)" pass -p ":${ns}:"
runchecktest "STACKPROFILE (bare :ns://)" pass -p ":${ns}://"
runchecktest "STACKPROFILE (bare :ns)" pass -p ":${ns}"
