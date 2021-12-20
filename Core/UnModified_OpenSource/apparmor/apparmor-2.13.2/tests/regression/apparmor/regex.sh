#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME regex
#=DESCRIPTION 
# This test verifies that tail globbing and regex globbing (perl regex engine) 
# are functioning correctly for confined processes.  Single character, multi 
# character and character class regexes are verified.
#=END

# A series of tests for regex expressions. this should help verify that the 
# parser is handling regex's correctly, as well as verifying that the module 
# pcre stuff works as well.

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

file=$tmpdir/file
file2=$tmpdir/filealpha
okperm=rw
badperm1=r
badperm2=w

touch $file $file2
chmod 600 $file  $file2

# read/write tests
settest open

# PASS TEST - 'testfile
genprofile ${tmpdir}/\'testfile:$okperm
runchecktest "\'testfile" pass ${tmpdir}/\'testfile

# PASS TEST - <testfile
genprofile ${tmpdir}/\<testfile:$okperm
runchecktest "\<testfile" pass ${tmpdir}/\<testfile

# PASS TEST - >testfile
genprofile ${tmpdir}/\>testfile:$okperm
runchecktest "\>testfile" pass ${tmpdir}/\>testfile

# PASS TEST - |testfile
genprofile ${tmpdir}/\|testfile:$okperm
runchecktest "ESCAPED PIPE" pass ${tmpdir}/\|testfile

# PASS TEST - ;testfile
genprofile ${tmpdir}/\;testfile:$okperm
runchecktest "\;testfile" pass ${tmpdir}/\;testfile

# PASS TEST - #testfile
#genprofile ${tmpdir}/\\#testfile:$okperm
#runchecktest "\#testfile" pass ${tmpdir}/\#testfile

# PASS TEST - looking for *
genprofile ${tmpdir}/\*:$okperm
runchecktest "SINGLE TAILGLOB" pass $file

# PASS TEST - looking for **
genprofile /\*\*:$okperm
runchecktest "DOUBLE TAILGLOB" pass $file

# PASS TEST - looking for { , }
genprofile ${tmpdir}/\{file,nofile\}:$okperm
runchecktest "CURLY BRACES" pass $file

# PASS TEST - looking for []
genprofile ${tmpdir}/\[aefg\]ile:$okperm
runchecktest "SQUARE BRACES 1" pass $file

# PASS TEST - looking for []
genprofile ${tmpdir}/\[a-g\]ile:$okperm
runchecktest "SQUARE BRACES 2" pass $file

# PASS TEST - looking for ?
genprofile ${tmpdir}/\?ile:$okperm
runchecktest "QUESTION MARK" pass $file

# FAIL TEST - looking for *
genprofile ${tmpdir}/z\*:$okperm
runchecktest "SINGLE TAILGLOB (fail)" fail $file

# FAIL TEST - looking for **
genprofile /does_not_exist\*\*:$okperm
runchecktest "DOUBLE TAILGLOB (fail)" fail $file

# FAIL TEST - looking for { , }
genprofile ${tmpdir}/\{elif,nofile\}:$okperm
runchecktest "CURLY BRACES (fail)" fail $file

# FAIL TEST - looking for []
genprofile ${tmpdir}/\[aeg\]ile:$okperm
runchecktest "SQUARE BRACES 1 (fail)" fail $file

# FAIL TEST - looking for []
genprofile ${tmpdir}/\[g-j\]ile:$okperm
runchecktest "SQUARE BRACES 2 (fail)" fail $file

# FAIL TEST - looking for ?
genprofile ${tmpdir}/\?ine:$okperm
runchecktest "QUESTION MARK (fail)" fail $file

# FAIL TEST - looking for literal '*' followed by **
# TEST for https://bugs.wirex.com/show_bug.cgi?id=2895
genprofile "${tmpdir}/file\*/beta**:$okperm"
runchecktest "GLOB FOLLOWED BY DOUBLE TAILGLOB (fail)" fail ${file}

# FAIL TEST - looking for literal '*' followed by **
# TEST for https://bugs.wirex.com/show_bug.cgi?id=2895
genprofile "${tmpdir}/file\*/beta**:$okperm"
runchecktest "GLOB FOLLOWED BY DOUBLE TAILGLOB (fail)" fail ${file2}

settest exec
file=/bin/true
okperm=rix
baderm=r

# PASS TEST - looking for *
genprofile /bin/\*:$okperm
runchecktest "SINGLE TAILGLOB (exec)" pass $file

# PASS TEST - looking for **
genprofile /bi\*\*:$okperm
runchecktest "DOUBLE TAILGLOB (exec)" pass $file

# PASS TEST - looking for { , }
genprofile /bin/\{true,false\}:$okperm
runchecktest "CURLY BRACES (exec)" pass $file

# PASS TEST - looking for []
genprofile /bin/\[aeft\]rue:$okperm
runchecktest "SQUARE BRACES 1 (exec)" pass $file

# PASS TEST - looking for []
genprofile /bin/\[s-v\]rue:$okperm
runchecktest "SQUARE BRACES 2 (exec)" pass $file

# PASS TEST - looking for ?
genprofile /bin/t\?ue:$okperm
runchecktest "QUESTION MARK (exec)" pass $file

# FAIL TEST - looking for *
genprofile /sbin/\*:$okperm signal:ALL
runchecktest "SINGLE TAILGLOB (exec, fail)" fail $file

# FAIL TEST - looking for **
genprofile /sbi\*\*:$okperm signal:ALL
runchecktest "DOUBLE TAILGLOB (exec, fail)" fail $file

# FAIL TEST - looking for { , }
genprofile /bin/\{flase,false\}:$okperm signal:ALL
runchecktest "CURLY BRACES (exec, fail)" fail $file

# FAIL TEST - looking for []
genprofile /bin/\[aef\]rue:$okperm signal:ALL
runchecktest "SQUARE BRACES 1 (exec, fail)" fail $file

# FAIL TEST - looking for []
genprofile /bin/\[u-x\]rue:$okperm signal:ALL
runchecktest "SQUARE BRACES 2 (exec, fail)" fail $file

# FAIL TEST - looking for ?
genprofile /bin/b\?ue:$okperm signal:ALL
runchecktest "QUESTION MARK (exec, fail)" fail $file
