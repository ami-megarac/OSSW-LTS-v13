#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME rename
#=DESCRIPTION
# The rename system call changes the name of a file in the filesystem.
# The test verifies that this operation (which involves AppArmor write
# permission checks) functions correctly for a confined process.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file1=$tmpdir/file1
file2=$tmpdir/file2
dir1=$tmpdir/dir1/
dir2=$tmpdir/dir2/

okfile1perm=rw
badfile1perm1=r
badfile1perm2=w
okfile2perm=w
badfile2perm=r

reset_test() {
	touch $file1
	chmod 600 $file1
	test -d $dir1 || mkdir $dir1
	chmod 700 $dir1
}

# PASS TEST
reset_test

genprofile $file1:$okfile1perm $file2:$okfile2perm $dir1:$okfile1perm $dir2:$okfile2perm

runchecktest "RENAME RW W" pass $file1 $file2
runchecktest "RENAME RW W (dir)" pass $dir1 $dir2

# FAILURE TEST (1) - Bad permissions on target

reset_test

genprofile $file1:$okfile1perm $file2:$badfile2perm $dir1:$okfile1perm $dir2:$badfile2perm

runchecktest "RENAME RW R" fail $file1 $file2
runchecktest "RENAME RW R (dir)" fail $dir1 $dir2

# FAILURE TEST (2) - no permissions on target

reset_test

genprofile $file1:$okfile1perm $dir1:$okfile1perm

runchecktest "RENAME RW -" fail $file1 $file2
runchecktest "RENAME RW - (dir)" fail $dir1 $dir2

# FAILURE TEST (3) - Bad permissions on source

reset_test

genprofile $file1:$badfile1perm1 $file2:$okfile2perm $dir1:$badfile1perm1 $dir2:$okfile2perm

runchecktest "RENAME R W" fail $file1 $file2
runchecktest "RENAME R W (dir)" fail $dir1 $dir2

# FAILURE TEST (4) - Bad permissions on source

reset_test

genprofile $file1:$badfile1perm2 $file2:$okfile2perm $dir1:$badfile1perm2 $dir2:$okfile2perm

runchecktest "RENAME W W" fail $file1 $file2
runchecktest "RENAME W W (dir)" fail $dir1 $dir2

# FAILURE TEST (5) - No permissions on source

reset_test

genprofile $file2:$okfile2perm $dir2:$okfile2perm

runchecktest "RENAME - W" fail $file1 $file2
runchecktest "RENAME - W (dir)" fail $dir1 $dir2
