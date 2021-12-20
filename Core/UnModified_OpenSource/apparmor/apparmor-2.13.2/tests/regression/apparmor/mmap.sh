#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME mmap
#=DESCRIPTION 
# This test verifies that mmap based access control is also subject to the 
# AppArmor profiles access specification.    The test needs some 
# attention/rethought,  It is unclear what it's purpose really is. Also why 
# does it fail when the profile is replaced with just read permission as 
# no mapped write is reattempted.   Also a test should be added which 
# causes the initial mmap write to fail (due to lack of write permission).  
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/src
okperm=rw
badperm=r

# PASS TEST (pt 1)

genprofile $file:$okperm signal:receive:peer=unconfined

runtestbg "READ/WRITE pass1" pass $file

sleep 2

# PASS TEST (pt 2)

kill -USR1 $_pid

checktestbg

rm -f $file



# FAILURE TEST (pt 1)

genprofile $file:$okperm signal:receive:peer=unconfined

runtestbg "READ/WRITE pass2" pass $file

sleep 2

genprofile $file:$badperm signal:receive:peer=unconfined 

# FAILURE TEST (pt 2)

kill -USR1 $_pid

checktestbg
