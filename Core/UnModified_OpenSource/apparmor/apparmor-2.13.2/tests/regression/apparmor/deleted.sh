#! /bin/bash
#
#	Copyright (C) 2002-2005 Novell/SUSE
#	Copyright (C) 2010 Canonical, Ltd
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME deleted
#=DESCRIPTION 
# Test AppArmor is properly working around a kernel in which the kernel 
# appends (deleted) to deleted files verifies that the d_path appending 
# (deleted) fix is working
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/file
file2="$tmpdir/file (deleted)"
file3="$tmpdir/unavailable"
okperm=rwl

subtest=sub

touch $file
touch "$file2"

# NO PROFILE TEST

runchecktest "NO PROFILE (access file)" pass nochange $file
runchecktest "NO PROFILE (access file (deleted))" pass nochange "$file2"


# NO CHANGEHAT TEST - doesn't force revalidation

genprofile $file:$okperm
runchecktest "NO CHANGEHAT (access file)" pass nochange $file
runchecktest "NO CHANGEHAT (cannot access unavailable)" fail nochange $file3

genprofile "$file2":$okperm
runchecktest "NO CHANGEHAT (access file (delete))" pass nochange "$file2"

# CHANGEHAT TEST - force revalidation using changehat
genprofile $file:$okperm hat:$subtest $file:$okperm
runchecktest "CHANGEHAT (access file)" pass $subtest $file
runchecktest "CHANGEHAT (cannot access unavailable)" fail $subtest $file3

genprofile "$file2":$okperm hat:$subtest "$file2":$okperm
runchecktest "CHANGEHAT (access file (deleted))" pass $subtest "$file2"

# EXEC TEST - force revalidation using a fork exec that inherits the open file
#             but uses a different profile
settest unix_fd_server
file=${tmpdir}/file
socket=${tmpdir}/unix_fd_test
fd_client=$PWD/unix_fd_client
okperm=rwl
badperm=wl
af_unix=""

if [ "$(kernel_features network/af_unix)" == "true" -a "$(parser_supports 'unix,')" == "true" ]; then
	af_unix="unix:create"
fi

# Content generated with:
# dd if=/dev/urandom bs=32 count=4 2> /dev/null | od -x | head -8 | sed -e 's/^[[:xdigit:]]\{7\}//g' -e 's/ //g'
cat > ${file} << EOM
aabcc2739c621194a00b6cb7875dcdeb
72f485a783219817c81c65f3e1b2bc80
4366ba09e881286c834e67b34ae6f186
ccc2c402fcc6e66d5cfaa0c68b94211c
163f7beeb9a320ab859189a82d695713
175797a8cf2e2435dd98551385e96d8f
05f82e8e0e146be0d4655d4681cb08b6
ed15ad1d4fb9959008589e589206ee13
EOM

# lets just be on the safe side
rm -f ${socket}

# PASS - unconfined client

genprofile $af_unix $file:$okperm $socket:rw $fd_client:ux

runchecktest "fd passing; unconfined client" pass $file $socket $fd_client "delete_file"

sleep 1
cat > ${file} << EOM
aabcc2739c621194a00b6cb7875dcdeb
72f485a783219817c81c65f3e1b2bc80
4366ba09e881286c834e67b34ae6f186
ccc2c402fcc6e66d5cfaa0c68b94211c
163f7beeb9a320ab859189a82d695713
175797a8cf2e2435dd98551385e96d8f
05f82e8e0e146be0d4655d4681cb08b6
ed15ad1d4fb9959008589e589206ee13
EOM
rm -f ${socket}

# PASS - confined client, rw access to the file
genprofile $af_unix $file:$okperm $socket:rw $fd_client:px -- image=$fd_client $af_unix $file:$okperm $socket:rw
runchecktest "fd passing; confined client w/ rw" pass $file $socket $fd_client "delete_file"

sleep 1
cat > ${file} << EOM
aabcc2739c621194a00b6cb7875dcdeb
72f485a783219817c81c65f3e1b2bc80
4366ba09e881286c834e67b34ae6f186
ccc2c402fcc6e66d5cfaa0c68b94211c
163f7beeb9a320ab859189a82d695713
175797a8cf2e2435dd98551385e96d8f
05f82e8e0e146be0d4655d4681cb08b6
ed15ad1d4fb9959008589e589206ee13
EOM
rm -f ${socket}
# FAIL - confined client, w access to the file

genprofile $af_unix $file:$okperm $socket:rw $fd_client:px -- image=$fd_client $af_unix $file:$badperm $socket:rw
runchecktest "fd passing; confined client w/ w only" fail $file $socket $fd_client "delete_file"

sleep 1
rm -f ${socket}

