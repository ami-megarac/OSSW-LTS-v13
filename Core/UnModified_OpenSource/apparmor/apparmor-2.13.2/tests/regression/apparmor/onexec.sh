#! /bin/bash
#	Copyright (C) 2012 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME onexec
#=DESCRIPTION 
# Verifies basic file access permission checks for change_onexec
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

settest transition
file=$tmpdir/file
subfile=$tmpdir/file2
okperm=rw

othertest="$pwd/rename"
subtest="sub"
fqsubbase="$pwd/onexec"
fqsubtest="$fqsubbase//$subtest"
subtest2="$pwd//sub2"
subtest3="$pwd//sub3"

exec_w="/proc/*/attr/exec:w"
attrs_r="/proc/*/attr/{current,exec}:r"

touch $file $subfile

do_test()
{
    local desc="$1"
    local prof="$2"
    local target_prof="$3"
    local res="$4"
    shift 4

    desc="ONEXEC $desc ($prof -> $target_prof)"
    if [ "$target_prof" == "nochange" ] ; then
        runchecktest "$desc" $res -l "$prof" -- "$@"
    else
        runchecktest "$desc" $res -O "$target_prof" -l "$prof" -L "$target_prof" -- "$@"
    fi
}


# ONEXEC from UNCONFINED - don't change profile
do_test "" unconfined nochange pass $bin/open $file

# ONEXEC from UNCONFINED - target does NOT exist
genprofile image=$bin/rw $bin/open:rix $file:rw  -- image=$bin/open
do_test "" unconfined noexist fail $bin/open $file

# ONEXEC from UNCONFINED - change to rw profile, no exec profile to override
genprofile image=$bin/rw $bin/open:rix $file:rw
do_test "no px profile" unconfined $bin/rw pass $bin/open $file

# ONEXEC from UNCONFINED - don't change profile, make sure exec profile is applied
genprofile image=$bin/rw $bin/open:px $file:rw  -- image=$bin/open $file:rw
do_test "nochange px" unconfined nochange pass $bin/open $file

# ONEXEC from UNCONFINED - change to rw profile, override regular exec profile, exec profile doesn't have perms
genprofile image=$bin/rw $bin/open:px $file:rw  -- image=$bin/open
do_test "override px" unconfined $bin/rw pass $bin/open $file

#------

# ONEXEC from CONFINED - don't change profile, open can't exec
genprofile 'change_profile->':$bin/rw $exec_w $attrs_r
do_test "no px perm" $test nochange fail $bin/open $file

# ONEXEC from CONFINED - don't change profile, open is run unconfined
genprofile 'change_profile->':$bin/rw $bin/open:rux $exec_w $attrs_r
do_test "nochange rux" $test nochange pass $bin/open $file

# ONEXEC from CONFINED - don't change profile, open is run confined without necessary perms
genprofile 'change_profile->':$bin/rw $exec_w $attrs_r -- image=$bin/open $file:rw
do_test "nochange px - no px perm" $test nochange fail $bin/open $file

# ONEXEC from CONFINED - don't change profile, open is run confined without necessary perms
genprofile 'change_profile->':$bin/rw $bin/open:rpx $exec_w $attrs_r -- image=$bin/open
do_test "nochange px - no file perm" $test nochange fail $bin/open $file

# ONEXEC from CONFINED - target does NOT exist
genprofile 'change_profile->':$bin/open $exec_w $attrs_r -- image=$bin/rw $bin/open:rix $file:rw  -- image=$bin/open
do_test "noexist px" $test noexist fail $bin/open $file

# ONEXEC from CONFINED - change to rw profile, no exec profile to override
genprofile 'change_profile->':$bin/rw $exec_w $attrs_r -- image=$bin/rw $bin/open:rix $file:rw
do_test "change profile - override rix" $test $bin/rw pass $bin/open $file

# ONEXEC from CONFINED - change to rw profile, no exec profile to override, no explicit write access to /proc/*/attr/exec
genprofile 'change_profile->':$bin/rw $attrs_r -- image=$bin/rw $bin/open:rix $file:rw
do_test "change profile - no exec_w" $test $bin/rw pass $bin/open $file

# ONEXEC from CONFINED - don't change profile, make sure exec profile is applied
genprofile 'change_profile->':$bin/rw $exec_w $attrs_r $bin/open:rpx -- image=$bin/rw $bin/open:rix $file:rw  -- image=$bin/open $file:rw
do_test "nochange px" $test nochange pass $bin/open $file

# ONEXEC from CONFINED - change to rw profile, override regular exec profile, exec profile doesn't have perms
genprofile 'change_profile->':$bin/rw $exec_w $attrs_r -- image=$bin/rw $bin/open:rix $file:rw  -- image=$bin/open
do_test "override px" $test $bin/rw pass $bin/open $file

# ONEXEC from - change to rw profile, override regular exec profile, exec profile has perms, rw doesn't
genprofile 'change_profile->':$bin/rw $exec_w $attrs_r -- image=$bin/rw $bin/open:rix  -- image=$bin/open $file:rw
do_test "override px" $test $bin/rw fail $bin/open $file

# ONEXEC from COFINED - change to rw profile via glob rule, override exec profile, exec profile doesn't have perms
genprofile 'change_profile->':/** $exec_w $attrs_r -- image=$bin/rw $bin/open:rix $file:rw  -- image=$bin/open
do_test "glob override px" $test $bin/rw pass $bin/open $file

# ONEXEC from COFINED - change to exec profile via glob rule, override exec profile, exec profile doesn't have perms
genprofile 'change_profile->':/** $exec_w $attrs_r -- image=$bin/rw $bin/open:rix $file:rw  -- image=$bin/open
do_test "glob override px" $test $bin/open fail $bin/open $file

# ONEXEC from COFINED - change to exec profile via glob rule, override exec profile, exec profile has perms
genprofile 'change_profile->':/** $exec_w $attrs_r -- image=$bin/rw $bin/open:rix $file:rw  -- image=$bin/open $file:rw
do_test "glob override px" $test $bin/rw pass $bin/open $file

