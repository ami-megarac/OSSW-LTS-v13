#! /bin/bash
#	Copyright (C) 2002-2007 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME open
#=DESCRIPTION
# Verify that the openat syscall is correctly managed for confined profiles.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

dir=${tmpdir}
subdir=deleteme
otherdir=otherdir
file=${subdir}/file
filepath=${dir}/${file}
okperm=rw
badperm1=r
badperm2=w

resettest () {
	rm -rf ${dir}/${subdir} ${dir}/${otherdir}
	mkdir ${dir}/${subdir}
}

# PASS UNCONFINED
resettest
runchecktest "OPENAT unconfined RW (create) " pass $dir $file

# PASS TEST (the file shouldn't exist, so open should create it
resettest
genprofile ${dir}/:r ${filepath}:$okperm
runchecktest "OPENAT RW (create) " pass $dir $file

# PASS TEST
resettest
touch ${filepath}
genprofile ${dir}/:r ${filepath}:$okperm
runchecktest "OPENAT RW (exists)" pass $dir $file

# FAILURE TEST (1)
resettest
touch ${filepath}
genprofile ${dir}/:r ${filepath}:$badperm1
runchecktest "OPENAT R" fail $dir $file

# FAILURE TEST (2)
resettest
touch ${filepath}
genprofile ${dir}/:r ${filepath}:$badperm2
runchecktest "OPENAT W (exists)" fail $dir $file

# FAILURE TEST (3)
resettest
genprofile ${dir}/:r ${filepath}:$badperm1 cap:dac_override
runchecktest "OPENAT R+dac_override" fail $dir $file

# FAILURE TEST (4)
# This is testing for bug: https://bugs.wirex.com/show_bug.cgi?id=2885
# When we open O_CREAT|O_RDWR, we are (were?) allowing only write access
# to be required.
# This test currently passes when it should fail because of the o_creat bug
resettest
genprofile ${dir}/:r ${filepath}:$badperm2
runchecktest "OPENAT W (create)" fail $dir $file

# PASS rename of directory in between opendir/openat
resettest
genprofile ${dir}/${subdir}/:rw ${dir}/otherdir/:w ${dir}/otherdir/file:rw
runchecktest "OPENAT RW (rename/newpath)" pass --rename ${dir}/otherdir ${dir}/${subdir} file

# PASS rename of directory in between opendir/openat - file exists
resettest
touch ${filepath}
genprofile ${dir}/${subdir}/:rw ${dir}/otherdir/:w ${dir}/otherdir/file:rw
runchecktest "OPENAT RW (rename/newpath)" pass --rename ${dir}/otherdir ${dir}/${subdir} file

# FAIL rename of directory in between opendir/openat - use old name
resettest
genprofile ${dir}/${subdir}/:rw ${dir}/otherdir/:w ${dir}/${subdir}/file:rw
runchecktest "OPENAT RW (rename/newpath)" fail --rename ${dir}/otherdir ${dir}/${subdir} file

# FAIL rename of directory in between opendir/openat - use old name, file exists
resettest
touch ${filepath}
genprofile ${dir}/${subdir}/:rw ${dir}/otherdir/:w ${dir}/${subdir}/file:rw
runchecktest "OPENAT RW (rename/newpath)" fail --rename ${dir}/otherdir ${dir}/${subdir} file
