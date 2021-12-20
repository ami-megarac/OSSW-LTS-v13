#! /bin/bash
#	Copyright (C) 2016 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME stackonexec
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
onexec="/proc/*/attr/exec:w"

othertest="$pwd/rename"
thirdtest="$pwd/exec"

stackotherok="change_profile->:&$othertest"
stackthirdok="change_profile->:&$thirdtest"

touch $file $otherfile $sharedfile $thirdfile

# Verify file access and contexts by an unconfined process
runchecktest "STACKONEXEC (unconfined - file)" pass -f $file
runchecktest "STACKONEXEC (unconfined - otherfile)" pass -f $otherfile
runchecktest "STACKONEXEC (unconfined - thirdfile)" pass -f $thirdfile
runchecktest "STACKONEXEC (unconfined - sharedfile)" pass -f $sharedfile

runchecktest "STACKONEXEC (unconfined - okcon)" pass -l unconfined -m '(null)'
runchecktest "STACKONEXEC (unconfined - bad label)" fail -l "$test" -m '(null)'
runchecktest "STACKONEXEC (unconfined - bad mode)" fail -l unconfined -m enforce

# Verify file access and contexts by a non-stacked profile
genprofile $fileok $sharedok $getcon
runchecktest "STACKONEXEC (not stacked - file)" pass -f $file
runchecktest_errno EACCES "STACKONEXEC (not stacked - otherfile)" fail -f $otherfile
runchecktest_errno EACCES "STACKONEXEC (not stacked - thirdfile)" fail -f $thirdfile
runchecktest "STACKONEXEC (not stacked - sharedfile)" pass -f $sharedfile

runchecktest "STACKONEXEC (not stacked - okcon)" pass -l "$test" -m enforce
runchecktest "STACKONEXEC (not stacked - bad label)" fail -l "${test}XXX" -m enforce
runchecktest "STACKONEXEC (not stacked - bad mode)" fail -l "$test" -m complain

# Verify file access and contexts by a profile stacked with unconfined
genprofile image=$othertest addimage:$test $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKONEXEC (stacked with unconfined - file)" fail -o $othertest -- $test -f $file
runchecktest "STACKONEXEC (stacked with unconfined - otherfile)" pass -o $othertest -- $test -f $otherfile
runchecktest "STACKONEXEC (stacked with unconfined - sharedfile)" pass -o $othertest -- $test -f $sharedfile

runchecktest "STACKONEXEC (stacked with unconfined - okcon)" pass -o $othertest -- $test -l "unconfined//&${othertest}" -m enforce
runchecktest "STACKONEXEC (stacked with unconfined - bad label)" fail -o $othertest -- $test -l "${test}//&${othertest}" -m enforce
runchecktest "STACKONEXEC (stacked with unconfined - bad mode)" fail -o $othertest -- $test -l "unconfined//&${othertest}" -m "(null)"

removeprofile
# Verify that stacking a nonexistent file is properly handled
runchecktest_errno ENOENT "STACKONEXEC (unconfined - stack nonexistent profile)" fail -o $othertest -- $test -f $file

# Verify file access and contexts by 2 stacked profiles
genprofile $fileok $sharedok $getcon $onexec $stackotherok -- \
	image=$othertest addimage:$test $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKONEXEC (2 stacked - file)" fail -o $othertest -- $test -f $file
runchecktest_errno EACCES "STACKONEXEC (2 stacked - otherfile)" fail -o $othertest -- $test -f $otherfile
runchecktest_errno EACCES "STACKONEXEC (2 stacked - thirdfile)" fail -o $othertest -- $test -f $thirdfile
runchecktest "STACKONEXEC (2 stacked - sharedfile)" pass -o $othertest -- $test -f $sharedfile

runchecktest "STACKONEXEC (2 stacked - okcon)" pass -o $othertest -- $test -l "${test}//&${othertest}" -m enforce
runchecktest "STACKONEXEC (2 stacked - bad label)" fail -o $othertest -- $test -l "${test}//&${test}" -m enforce
runchecktest "STACKONEXEC (2 stacked - bad mode)" fail -o $othertest -- $test -l "${test}//&${test}" -m '(null)'

# Verify that a change_profile rule is required to aa_stack_onexec()
genprofile $fileok $sharedok $getcon $onexec -- \
	image=$othertest addimage:$test $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKONEXEC (2 stacked - no change_profile)" fail -o $othertest -- $test -l "${test}//&${othertest}" -m enforce

# Verify file access and contexts by 3 stacked profiles
genprofile $fileok $sharedok $getcon $onexec $stackotherok $stackthirdok -- \
	image=$othertest addimage:$test $otherok $sharedok $getcon $onexec $stackthirdok -- \
	image=$thirdtest addimage:$test $thirdok $sharedok $getcon
runchecktest_errno EACCES "STACKONEXEC (3 stacked - file)" fail -o $othertest -- $test -o $thirdtest -- $test -f $file
runchecktest_errno EACCES "STACKONEXEC (3 stacked - otherfile)" fail -o $othertest -- $test -o $thirdtest -- $test -f $otherfile
runchecktest_errno EACCES "STACKONEXEC (3 stacked - thirdfile)" fail -o $othertest -- $test -o $thirdtest -- $test -f $thirdfile
runchecktest "STACKONEXEC (3 stacked - sharedfile)" pass -o $othertest -- $test -o $thirdtest -- $test -f $sharedfile

runchecktest "STACKONEXEC (3 stacked - okcon)" pass -o $othertest -- $test -o $thirdtest -- $test -l "${thirdtest}//&${test}//&${othertest}" -m enforce

genprofile $fileok $sharedok $getcon $onexec $stackotherok -- \
	image=$othertest addimage:$test $otherok $sharedok $getcon $onexec $stackthirdok -- \
	image=$thirdtest addimage:$test $thirdok $sharedok $getcon
runchecktest_errno EACCES "STACKONEXEC (3 stacked - sharedfile - no change_profile)" fail -o $othertest -- $test -o $thirdtest -- $test -f $sharedfile

ns="ns"
prof="stackonexec"
nstest=":${ns}:${prof}"
# Verify file access and contexts by stacking a profile with a namespaced profile
genprofile --stdin <<EOF
$test {
  file,
  audit deny $otherfile $okperm,
  audit deny $thirdfile $okperm,
  change_profile -> &$nstest,
}

$nstest {
  file,
  audit deny $file $okperm,
  audit deny $thirdfile $okperm,
}
EOF
runchecktest_errno EACCES "STACKONEXEC (stacked with namespaced profile - file)" fail -o $nstest -- $test -f $file
runchecktest_errno EACCES "STACKONEXEC (stacked with namespaced profile - otherfile)" fail -o $nstest -- $test -f $otherfile
runchecktest_errno EACCES "STACKONEXEC (stacked with namespaced profile - thirdfile)" fail -o $nstest -- $test -f $thirdfile
runchecktest "STACKONEXEC (stacked with namespaced profile - sharedfile)" pass -o $nstest -- $test -f $sharedfile

runchecktest "STACKONEXEC (stacked with namespaced profile - okcon)" pass -o $nstest -- $test -l $prof -m enforce

# Verify file access and contexts in mixed mode
genprofile $fileok $sharedok $getcon $onexec $stackotherok -- \
	image=$othertest flag:complain addimage:$test $otherok $sharedok $getcon
runchecktest "STACKONEXEC (mixed mode - file)" pass -o $othertest -- $test -f $file
runchecktest_errno EACCES "STACKONEXEC (mixed mode - otherfile)" fail -o $othertest -- $test -f $otherfile
runchecktest "STACKONEXEC (mixed mode - sharedfile)" pass -o $othertest -- $test -f $sharedfile

runchecktest "STACKONEXEC (mixed mode - okcon)" pass -o $othertest -- $test -l "${othertest}//&${test}" -m mixed

genprofile $fileok $sharedok $getcon $onexec -- \
	image=$othertest flag:complain addimage:$test $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKONEXEC (mixed mode - okcon - no change_profile)" fail -o $othertest -- $test -l "${othertest}//&${test}" -m mixed

genprofile flag:complain $fileok $sharedok $getcon $onexec -- \
	image=$othertest addimage:$test $otherok $sharedok $getcon
runchecktest_errno EACCES "STACKONEXEC (mixed mode 2 - file)" fail -o $othertest -- $test -f $file
runchecktest "STACKONEXEC (mixed mode 2 - otherfile)" pass -o $othertest -- $test -f $otherfile
runchecktest "STACKONEXEC (mixed mode 2 - sharedfile)" pass -o $othertest -- $test -f $sharedfile

runchecktest "STACKONEXEC (mixed mode 2 - okcon)" pass -o $othertest -- $test -l "${othertest}//&${test}" -m mixed

# Verify file access and contexts in complain mode
genprofile flag:complain $getcon -- image=$othertest addimage:$test flag:complain $getcon
runchecktest "STACKONEXEC (complain mode - file)" pass -o $othertest -- $test -f $file

runchecktest "STACKONEXEC (complain mode - okcon)" pass -o $othertest -- $test -l "${test}//&${othertest}" -m complain

# Verify that stacking with a bare namespace is handled. The process is placed
# into the default profile of the namespace, which is unconfined.
genprofile --stdin <<EOF
$test { file, change_profile, }
$nstest { }
EOF
runchecktest "STACKONEXEC (bare :ns:)" pass -o ":${ns}:" -- $test -l unconfined -m "(null)"
runchecktest "STACKONEXEC (bare :ns://)" pass -o ":${ns}://" -- $test -l unconfined -m "(null)"
runchecktest "STACKONEXEC (bare :ns)" pass -o ":${ns}" -- $test -l unconfined -m "(null)"
