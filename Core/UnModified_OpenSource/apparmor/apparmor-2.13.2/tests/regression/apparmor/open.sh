#! /bin/bash
#	Copyright (C) 2002-2007 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME open
#=DESCRIPTION 
# Verify that the open syscall is correctly managed for confined profiles.  
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/file
okperm=rw
badperm1=r
badperm2=w

# PASS UNCONFINED
runchecktest "OPEN unconfined RW (create) " pass $file

# PASS TEST (the file shouldn't exist, so open should create it
rm -f ${file}
genprofile $file:$okperm
runchecktest "OPEN RW (create) " pass $file

# PASS TEST
genprofile $file:$okperm
runchecktest "OPEN RW" pass $file

# FAILURE TEST (1)
genprofile $file:$badperm1
runchecktest "OPEN R" fail $file

# FAILURE TEST (2)
genprofile $file:$badperm2
runchecktest "OPEN W" fail $file

# FAILURE TEST (3)
genprofile $file:$badperm1 cap:dac_override
runchecktest "OPEN R+dac_override" fail $file

# FAILURE TEST (4)
# This is testing for bug: https://bugs.wirex.com/show_bug.cgi?id=2885
# When we open O_CREAT|O_RDWR, we are (were?) allowing only write access
# to be required.
rm -f ${file}
genprofile $file:$badperm2
runchecktest "OPEN W (create)" fail $file

# This is a test where using just a raw 'file,' rule allowing all file
# access
genprofile file
runchecktest "OPEN 'file' RW" pass $file

# this test is to make sure the raw 'file' rule allows access to things
# that are not covered by the owner rule
chown nobody $file
chmod 666 $file
genprofile file
runchecktest "OPEN 'file' RW" pass $file
