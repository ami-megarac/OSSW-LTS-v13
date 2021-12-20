#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME exec_qual
#=DESCRIPTION 
# See 'matrix.doc' in the SubDomain/Documentation directory.  This test 
# currently verifies the enforce mode handling of exec between the various 
# confinement conditions for execer and execee. It needs to be extended to 
# include the complain mode verification.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=/etc/group

test1=$bin/exec_qual
test2=$bin/exec_qual2
test2_rex1=$bin/exec_\*
test2_rex2=$bin/exec_qual[1-9]

test2perm=rpx
fileperm=rw

local_runchecktest()
{
	desc=$1
	passfail=$2
	expected_confinement=$3
	actual_confinement=""

	shift 3

	runtestbg "$desc" $passfail $*

	sleep 1

	if [ -r /proc/$_pid/attr/current ]
	then
		actual_confinement=`cat /proc/$_pid/attr/current | cut -d ' ' -f1`

		# signal pid to continue
		kill -USR1 $_pid
	elif [ -z $outfile ]
	then
		echo "FAIL: Unable to determine confinment for pid $pid" >> $outfile
	fi

	checktestbg

	if [ "$teststatus" == "pass" -a -n "$actual_confinement" -a "$actual_confinement" != "$expected_confinement" ]
	then
		 echo "Error: ${testname} failed. Test '${_testdesc}' actual confinement '$actual_confinement' differed from expected confinement '$expected_confinement'"
		testfailed
	fi
}


# ENFORCE mode

# confined parent, exec child with 'px'
# case 1: parent profile grants access (should be irrelevant)
#	  child profile grants access
#	  expected behaviour: child should be able to access resource

genprofile $test2:px $file:$fileperm signal:receive:peer=unconfined -- image=$test2 $file:$fileperm signal:receive
local_runchecktest "enforce px case1" pass $test2 $test2 $file

# case 2: parent profile grants access (should be irrelevant)
#	  child profile disallows access
#	  expected behaviour: child should be unable to access resource

genprofile $test2:px $file:$fileperm signal:receive:peer=unconfined -- image=$test2 signal:receive
local_runchecktest "enforce px case2" fail $test2 $test2 $file

# case 3: parent profile disallows access (should be irrelevant)
#	  child profile allows access
#	  expected behaviour: child should be able to access resource

genprofile $test2:px signal:receive:peer=unconfined -- image=$test2 $file:$fileperm signal:receive
local_runchecktest "enforce px case3" pass $test2 $test2 $file

# case 4: parent profile grants access (should be irrelevant)
#	  missing child profile
#	  expected behaviour: exec of child fails

genprofile $test2:px $file:$fileperm signal:receive:peer=unconfined
local_runchecktest "enforce px case4" fail "n/a" $test2 $file

# confined parent, exec child with 'ix'
# case 1: parent profile grants access
#	  child profile grants access (should be irrelevant)
#	  expected behaviour: child should be able to access resource

genprofile $test2:rix $file:$fileperm signal:receive:peer=unconfined -- image=$test2 $file:$fileperm signal:receive
local_runchecktest "enforce ix case1" pass $test1 $test2 $file

# case 2: parent profile grants access
#	  child profile disallows access (should be irrelevant)
#	  expected behaviour: child should be able to access resource

genprofile $test2:rix $file:$fileperm signal:receive:peer=unconfined -- image=$test2 signal:receive
local_runchecktest "enforce ix case2" pass $test1 $test2 $file

# case 3: parent profile disallows access
#	  child profile allows access (should be irrelevant)
#	  expected behaviour: child should be unable to access resource

genprofile $test2:rix signal:receive:peer=unconfined -- image=$test2 $file:$fileperm signal:receive
local_runchecktest "enforce ix case3" fail $test1 $test2 $file

# case 4: parent profile grants access
#	  missing child profile (irrelvant)
#	  expected behaviour: child should be able to access resource

genprofile $test2:rix $file:$fileperm signal:receive:peer=unconfined
local_runchecktest "enforce ix case4" pass $test1 $test2 $file

# confined parent, exec child with 'ux'
# case 1: parent profile grants access (should be irrelevant)
#	  expected behaviour, child should be able to access resource

genprofile $test2:ux $file:$fileperm signal:receive:peer=unconfined
local_runchecktest "enforce ux case1" pass "unconfined" $test2 $file

# case 2: parent profile denies access (should be irrelevant)
#	  expected behaviour, child should be able to access resource

genprofile $test2:ux signal:receive:peer=unconfined
local_runchecktest "enforce ux case1" pass "unconfined" $test2 $file

# confined parent, exec child with conflicting exec qualifiers
# that overlap in such away that px is prefered (ix is glob, px is exact
# match).  Other overlap tests should be in the parser.
# case 1: 
#	  expected behaviour: exec of child passes

genprofile $test2:px $test2_rex1:ix signal:receive:peer=unconfined -- image=$test2 $file:$fileperm signal:receive
local_runchecktest "enforce conflicting exec qual" pass $test2 $test2 $file

# unconfined parent
# case 1: child profile exists, child profile grants access
#	  expected behaviour: child should be able to access resource

genprofile image=$test2 $file:$fileperm signal:receive:peer=unconfined
local_runchecktest "enforce unconfined case1" pass $test2 $test2 $file

# case 2: child profile exists, child profile denies access
#	  expected behaviour: child should be unable to access resource

genprofile image=$test2 signal:receive:peer=unconfined
local_runchecktest "enforce unconfined case2" fail $test2 $test2 $file

# case 3: no child profile exists, unconfined
#	  expected behaviour: child should be able to access resource

removeprofile
local_runchecktest "enforce unconfined case3" pass "unconfined" $test2 $file

# -----------------------------------------------------------------------

# COMPLAIN mode -- all the tests again but with profiles loaded in
# complain mode rather than enforce mode

# confined parent, exec child with 'px'
# case 1: expected behaviour: as enforce
# case 2: expected behaviour: child should be able to access resource
# case 3: expected behaviour: as enforce
# case 4: expected behaviour: child should be able to access resource
#	  verify child is in null-complain-profile

# confined parent, exec child with 'ix'
# case 1: expected behaviour: as enforce
# case 2: expected behaviour: as enforce
# case 3: expected behaviour: child should be able to access resource
# case 4: expected behaviour: as enforce

# constrined parent, exec child with 'ux'
# case 1: expected behaviour, child should be able to access resource
# case 2: expected behaviour, child should be able to access resource

# confined parent, exec child with conflicting exec qualifiers
# case 1: child should be able to access resource
#	  verify that child is in null-complain-profile

# unconfined parent
# case 1: expected behaviour: as enforce
# case 2: expected behaviour, child should be able to access resource
# case 3: expected behaviour: as enforce

