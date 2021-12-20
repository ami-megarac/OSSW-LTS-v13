#!/bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME pwrite
#=DESCRIPTION pread/pwrite test

pwd=`dirname $0`
pwd=`cd $pwd ; pwd`

bin=$pwd

. $bin/prologue.inc

file=${tmpdir}/pwrite
okperm=rw
badwriteperm=r
badreadperm=w

# PASS TEST (pass 1)

genprofile $file:$okperm signal:receive:peer=unconfined

runtestbg "PREAD/PWRITE with rw" pass $file

sleep 2

# PASS TEST (pass 2)

kill -USR1 $_pid

checktestbg

rm -f ${file}

# Disabled revalidation/revocation test as this can not be reliably tested
# at this time 
## FAIL TEST - PWRITE (pass 1)
#
#genprofile $file:$okperm
#
#runtestbg "PWRITE without w" fail $file
#
#sleep 2
#
## FAIL TEST - PWRITE (pass 2)
#
#genprofile $file:$badwriteperm
#
#sleep 2
#
#kill -USR1 $_pid
#
#checktestbg
#
#rm -f ${file}

# Disabled revalidation/revocation test as this can not be reliably tested
# at this time 
# FAIL TEST - PREAD (pass 1)
#
#genprofile $file:$okperm
#
#runtestbg "PREAD without r" fail $file
#
#sleep 2
#
##FAIL TEST - PREAD (pass 2)
#genprofile $file:$badreadperm
#
#sleep 2
#
#kill -USR1 $_pid
#
#checktestbg
#
#rm -f ${file}
