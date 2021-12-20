#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME mult_mount
#=DESCRIPTION 
# Verify that multiple mounts of the same device are handled correctly
# for various operations.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

cleandir()
{
	if [ -d $1 ]
	then
		rmdir $1
	fi
}

get_current_logposition()
{
:
}

verify_log_record()
{
:
}

image=$tmpdir/image.ext3
mp1=$tmpdir/mnt1
mp2=$tmpdir/mnt2

file1a=$mp1/file
file1b=$mp1/file2
file2a=$mp2/file
file2b=$mp2/file2

dir1=$mp1/dir/
dir2=$mp2/dir/

mkdirperm=w
mkdirperm_fail=r

linkperm=rl
readperm=r

dd if=/dev/zero of=$image bs=4096 count=20 > /dev/null 2>&1
mkfs.ext2 -F -m 0 -N 10 $image > /dev/null 2>&1

mkdir $mp1 $mp2

mount -o loop $image $mp1
mount --bind $mp1 $mp2

do_onexit="umount $mp2 $mp1"

settest mkdir

# MKDIR PASS TESTS
genprofile $dir1:$mkdirperm
runchecktest "OVERLAP MKDIR PASS1" pass mkdir $dir1
cleandir $dir1

genprofile $dir2:$mkdirperm

runchecktest "OVERLAP MKDIR PASS2" pass mkdir $dir2
cleandir $dir2

# MKDIR FAIL TESTS
genprofile $dir1:$mkdirperm

runchecktest "OVERLAP MKDIR PASS3" fail mkdir $dir2
cleandir $dir2

genprofile $dir2:$mkdirperm

runchecktest "OVERLAP MKDIR PASS4" fail mkdir $dir1
cleandir $dir1

get_current_logposition
genprofile $dir1:$mkdirperm_fail
runchecktest "OVERLAP MKDIR FAIL1" fail mkdir $dir1
verify_log_record
cleandir $dir1

get_current_logposition
genprofile $dir2:$mkdirperm_fail
runchecktest "OVERLAP MKDIR FAIL2" fail mkdir $dir2
verify_log_record
cleandir $dir2

# LINK PASS TESTS

touch $file1a

settest link

# src has link perm (not necessary)
genprofile $file1a:$linkperm $file1b:$linkperm
runchecktest "OVERLAP LINK PASS1a" pass $file1a $file1b
rm -f $file1b

genprofile $file1a:$linkperm $file2b:$linkperm
runchecktest "OVERLAP LINK PASS2a" fail $file1a $file1b
rm -f $file1b

genprofile $file2a:$linkperm $file1b:$linkperm
runchecktest "OVERLAP LINK PASS3a" fail $file1a $file1b
rm -f $file1b

# src lacks link perm, ok as long as dest has linkperm
genprofile $file1a:$readperm $file1b:$linkperm
runchecktest "OVERLAP LINK PASS1b" pass $file1a $file1b
rm -f $file1b

genprofile $file1a:$readperm $file2b:$linkperm
runchecktest "OVERLAP LINK PASS2b" fail $file1a $file1b
rm -f $file1b

genprofile $file2a:$readperm $file1b:$linkperm
runchecktest "OVERLAP LINK PASS3b" fail $file1a $file1b
rm -f $file1b


# LINK FAIL TESTS

# Simple failure, these should fail due to cross device link,
# nothing to do with AA
genprofile $file1a:$linkperm $file2b:$linkperm
runchecktest "OVERLAP LINK FAIL1" fail $file1a $file2b
rm -f $file2b

genprofile $file2a:$linkperm $file1b:$linkperm
runchecktest "OVERLAP LINK FAIL2" fail $file2a $file1b
rm -f $file1b

# dest lacks link perm
get_current_logposition
genprofile $file1a:$linkperm $file1b:$readperm
runchecktest "OVERLAP LINK FAIL3" fail $file1a $file1b
verify_log_record
rm -f $file1b

get_current_logposition
genprofile $file1a:$linkperm $file2b:$readperm
runchecktest "OVERLAP LINK FAIL4" fail $file1a $file1b
verify_log_record
rm -f $file1b

get_current_logposition
genprofile $file2a:$linkperm $file1b:$readperm
runchecktest "OVERLAP LINK FAIL5" fail $file1a $file1b
verify_log_record
rm -f $file1b
