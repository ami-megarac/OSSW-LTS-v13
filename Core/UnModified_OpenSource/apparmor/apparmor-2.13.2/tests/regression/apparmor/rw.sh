#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME rw
#=DESCRIPTION 
# This test verifies read/write operation.   AppArmor caches a successful open 
# but checks (on read/write) to see if a confined processes profile has been 
# replaced asynchronously. If it has, access is reevaluated.  The test waits 
# for a signal at which point it reattempts to write, read and verify data. The
# controlling script performs a profile replacement before sending the signal 
# for the test to reattempt the io.
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

runtestbg "READ/WRITE pass" pass $file

sleep 2

# PASS TEST (pt 2)

kill -USR1 $_pid

checktestbg

rm -f $file

# Disabled revalidation/revocation test as this can not be reliably tested
# at this time 
## FAILURE TEST (pt 1)
#
#genprofile $file:$okperm
#
#runtestbg "READ/WRITE fail" fail $file
#
#sleep 2
#
## FAILURE TEST (pt 2)
#
#genprofile $file:$badperm
#
## problem the shell and the test program are racing, after profile replacement
## if the shell runs immediately after profile replacement instead of the
## test program it will will.  We insert a small sleep to make this unlikely
#
#sleep 1
#
#kill -USR1 $_pid
#
#checktestbg
