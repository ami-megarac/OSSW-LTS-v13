#!/bin/bash
#
#   Copyright (c) 2013
#   Canonical, Ltd. (All rights reserved)
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of version 2 of the GNU General Public
#   License published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, contact Novell, Inc. or Canonical
#   Ltd.
#

#=NAME fd_inheritance
#=DESCRIPTION
# This test verifies that file descriptor inheritance results in the expected
# delegation, or lack thereof, between various combinations of confined and
# unconfined processes.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/file
inheritor=$bin/fd_inheritor
okperm=r
badperm=w

cat > $file << EOF
0a7eb75b2a54eaf86aa8d7b4c6cc945c
70a2265ba96d962d993c97689ff09904
d3e773e2a4a0cc9d7e28eb217a4241ce
1437d6c55ef788d3bcd27ab14e9382a9
EOF

runchecktest "fd inheritance; unconfined -> unconfined" pass $file $inheritor

genprofile $file:$okperm $inheritor:Ux
runchecktest "fd inheritance; confined -> unconfined" pass $file $inheritor

genprofile $file:$badperm $inheritor:Ux
runchecktest "fd inheritance; confined (bad perm) -> unconfined" fail $file $inheritor

genprofile $inheritor:Ux
runchecktest "fd inheritance; confined (no perm) -> unconfined" fail $file $inheritor

genprofile image=$inheritor $file:$okperm
runchecktest "fd inheritance; unconfined -> confined" pass $file $inheritor

genprofile image=$inheritor
runchecktest "fd inheritance; unconfined -> confined (no perm)" pass $file $inheritor

genprofile $file:$okperm $inheritor:Px -- image=$inheritor $file:$okperm
runchecktest "fd inheritance; confined -> confined" pass $file $inheritor

genprofile $file:$badperm $inheritor:Px -- image=$inheritor $file:$okperm
runchecktest "fd inheritance; confined (bad perm) -> confined" fail $file $inheritor

genprofile $inheritor:Px -- image=$inheritor $file:$okperm
runchecktest "fd inheritance; confined (no perm) -> confined" fail $file $inheritor

genprofile $file:$okperm $inheritor:Px -- image=$inheritor $file:$badperm
runchecktest "fd inheritance; confined -> confined (bad perm)" fail $file $inheritor

genprofile $file:$okperm $inheritor:Px -- image=$inheritor
runchecktest "fd inheritance; confined -> confined (no perm)" fail $file $inheritor
