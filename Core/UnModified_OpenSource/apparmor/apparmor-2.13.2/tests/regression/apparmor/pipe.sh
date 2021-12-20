#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME pipe
#=DESCRIPTION 
# This test is structured similarly to named_pipe except it uses the pipe(2) 
# syscall to create a communication channel between parent and child rather 
# than a node in the filesystem.   AppArmor does not mediate pipe io for either
# confined or non confined processes. This test verifies that io functions as 
# expected for both an unconfined process and a confined process with an empty 
# profile.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

subtest=sub

# PIPE - no confinement 

runchecktest "PIPE (no confinement)" pass nochange

# PIPE - confined.

genprofile 

runchecktest "PIPE (confinement)" pass nochange

# PIPE - in a subprofile.

genprofile hat:$subtest

runchecktest "PIPE (subprofile)" pass ${subtest}
