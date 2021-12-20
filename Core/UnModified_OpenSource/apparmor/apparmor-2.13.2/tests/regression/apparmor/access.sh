#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME access
#=DESCRIPTION
# Verify that the access syscall is correctly managed for confined profiles
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/file

dir=$tmpdir/dir/
rperm=r
rwxperm=rwix
wxperm=wix

touch $file
chmod 777 $file	# full perms so discretionary access checks succeed

# PASS TEST 
genprofile $file:$rwxperm
runchecktest "ACCESS file r (rwx)" pass $file r
runchecktest "ACCESS file rx (rwx)" pass $file rx
runchecktest "ACCESS file rwx (rwx)" pass $file rwx

genprofile $file:$rperm
runchecktest "ACCESS file r (r)" pass $file r
runchecktest "ACCESS file rx (r)" xfail $file rx
runchecktest "ACCESS file rwx (r)" xfail $file rwx

genprofile $file:$wxperm
runchecktest "ACCESS file x (wx)" pass $file x
runchecktest "ACCESS file w (wx)" pass $file w
runchecktest "ACCESS file wx (wx)" pass $file wx

genprofile $file:$wxperm
runchecktest "ACCESS file r (wx)" xfail $file r
runchecktest "ACCESS file rx (wx)" xfail $file rx
runchecktest "ACCESS file rwx (wx)" xfail $file rwx

# wx are not necessary for directory write or traverse
# only r is required
mkdir $dir
chmod 777 $dir	# full perms so discretionary access checks succeed

genprofile $dir:$rwxperm
runchecktest "ACCESS dir r (rwx)" pass $dir r
runchecktest "ACCESS dir rx (rwx)" pass $dir rx
runchecktest "ACCESS dir rwx (rwx)" pass $dir rwx

genprofile $dir:$rperm
runchecktest "ACCESS dir r (r)" pass $dir r
runchecktest "ACCESS dir rx (r)" pass $dir rx
runchecktest "ACCESS dir rwx (r)" xfail $dir rwx

genprofile $dir:$wxperm
runchecktest "ACCESS dir x (wx)" pass $dir x
runchecktest "ACCESS dir w (wx)" pass $dir w
runchecktest "ACCESS dir wx (wx)" pass $dir wx

genprofile $dir:$wxperm
runchecktest "ACCESS dir r (wx)" xfail $dir r
runchecktest "ACCESS dir rx (wx)" xfail $dir rx
runchecktest "ACCESS dir rwx (wx)" xfail $dir rwx
