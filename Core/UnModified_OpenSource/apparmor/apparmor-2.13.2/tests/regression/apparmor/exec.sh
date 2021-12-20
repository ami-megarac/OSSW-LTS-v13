#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME exec
#=DESCRIPTION Runs exec() through ux, ix & px functionality

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=/bin/true
ok_ix_perm=rix
badperm=r
ok_ux_perm=ux
ok_px_perm=px
bad_mx_perm=rm

# PASS TEST - inherited

genprofile $file:$ok_ix_perm

runchecktest "EXEC with ix" pass $file

# PASS TEST - unconfined

genprofile $file:$ok_ux_perm

runchecktest "EXEC with ux" pass $file

# PASS TEST - profiled

genprofile $file:$ok_px_perm -- image=$file

runchecktest "EXEC with px" pass $file

# FAIL TEST - px/no profile

genprofile $file:$ok_px_perm signal:ALL

runchecktest "EXEC with px - no profile" fail $file

# NOLINK PERMTEST

genprofile $file:$badperm signal:ALL

runchecktest "EXEC no x" fail $file

# MMAP exec

genprofile $file:$bad_mx_perm signal:ALL

runchecktest "EXEC mmap x" fail $file

# UNCONFINED -> CONFINED

genprofile image=$file
runchecktest "EXEC unconfined -> confined" pass $file

# UNCONFINED -> CONFINED no access to self binary

genprofile -I image=$file
runchecktest "EXEC unconfined -> confined/no access to self" pass $file
