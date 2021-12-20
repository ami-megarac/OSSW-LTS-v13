#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME sd_flags
#=DESCRIPTION Verify that the profile flags are enforced (or not) properly.

pwd=$(dirname $0)
pwd=$(cd $pwd ; /bin/pwd)

bin=$pwd

. $bin/prologue.inc

settest open

file=$tmpdir/file
okperm=rw
badperm1=r
badperm2=w

touch $file
chmod 600 $file

# PASS TEST (noflags)
genprofile $file:$okperm
runchecktest "SD_FLAGS OPEN RW (noflags)" pass $file

# audit alone
# PASS TEST (audit)
genprofile $file:$okperm flag:audit
runchecktest "SD_FLAGS OPEN RW (audit)" pass $file

# FAILURE TEST (audit)
genprofile $file:$badperm1 flag:audit
runchecktest "SD_FLAGS OPEN R (audit)" fail $file

# complain alone
# PASS TEST (complain)
genprofile $file:$okperm flag:complain
runchecktest "SD_FLAGS OPEN RW (complain)" pass $file

# PASS TEST (complain) 2
genprofile flag:complain
runchecktest "SD_FLAGS OPEN noaccess (complain)" pass $file

# need a way to verify that audit is actually auditing...
# PASS TEST (audit,complain,debug)
genprofile flag:audit flag:complain
runchecktest "SD_FLAGS OPEN noaccess (audit,complain)" pass $file

# check for flags on hats...
settest changehat_wrapper

# audit alone
# PASS TEST (noflags)
genprofile hat:open addimage:${bin}/open $file:$okperm
runchecktest "SD_FLAGS HAT/OPEN RW (noflags)" pass open $file

# PASS TEST 1 (audit)
genprofile flag:audit hat:open addimage:${bin}/open $file:$okperm
runchecktest "SD_FLAGS HAT/OPEN RW (audit)" pass open $file

# PASS TEST 2 (audit)
genprofile hat:open addimage:${bin}/open $file:$okperm flag:audit
runchecktest "SD_FLAGS HAT/OPEN RW (audit)" pass open $file

# PASS TEST 3 (audit)
genprofile flag:audit hat:open addimage:${bin}/open $file:$okperm flag:audit
runchecktest "SD_FLAGS HAT/OPEN RW (audit)" pass open $file

# FAILURE TEST 1 (audit)
genprofile flag:audit  hat:open addimage:${bin}/open $file:$badperm1
runchecktest "SD_FLAGS HAT/OPEN R (audit)" fail open $file

# FAILURE TEST 2 (audit)
genprofile hat:open addimage:${bin}/open $file:$badperm1 flag:audit
runchecktest "SD_FLAGS HAT/OPEN R (audit)" fail open $file

# FAILURE TEST 3 (audit)
genprofile flag:audit hat:open addimage:${bin}/open $file:$badperm1 flag:audit
runchecktest "SD_FLAGS HAT/OPEN R (audit)" fail open $file

# complain alone
# PASS TEST 1 (complain)
genprofile flag:complain hat:open addimage:${bin}/open $file:$okperm
runchecktest "SD_FLAGS HAT/OPEN RW (complain)" pass open $file

# PASS TEST 2 (complain)
genprofile hat:open addimage:${bin}/open $file:$okperm flag:complain
runchecktest "SD_FLAGS HAT/OPEN RW (complain)" pass open $file

# PASS TEST 3 (complain)
genprofile flag:complain hat:open addimage:${bin}/open $file:$okperm flag:complain
runchecktest "SD_FLAGS HAT/OPEN RW (complain)" pass open $file

# FAILURE TEST 1 (complain)
genprofile flag:complain  hat:open addimage:${bin}/open $file:$badperm1
runchecktest "SD_FLAGS HAT/OPEN R (complain)" fail open $file

# PASS TEST 4 (complain)
genprofile hat:open addimage:${bin}/open $file:$badperm1 flag:complain
runchecktest "SD_FLAGS HAT/OPEN R (complain)" pass open $file

# PASS TEST 5 (complain)
genprofile flag:complain hat:open addimage:${bin}/open $file:$badperm1 flag:complain
runchecktest "SD_FLAGS HAT/OPEN R (complain)" pass open $file

# PASS TEST 6 (complain) no hat defined
genprofile flag:complain
runchecktest "SD_FLAGS HAT/OPEN R (complain)" pass open $file

# audit + complain
# PASS TEST 3 (audit+complain)
genprofile flag:audit hat:open addimage:${bin}/open $file:$badperm1 flag:complain
runchecktest "SD_FLAGS HAT/OPEN RW (audit+complain)" pass open $file

# FAILURE TEST 3 (complain+audit)
genprofile flag:complain hat:open addimage:${bin}/open $file:$badperm1 flag:audit
runchecktest "SD_FLAGS HAT/OPEN R (complain+audit)" fail open $file

