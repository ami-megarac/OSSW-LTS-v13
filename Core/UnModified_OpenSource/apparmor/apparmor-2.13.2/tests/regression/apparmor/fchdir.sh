#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME chdir
#=DESCRIPTION 
# Verify change directory functions correctly for a confined process. Subdomain
# should allow 'x' access on a directory without it being explicitly listed in 
# tasks profile.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

dir=$tmpdir/tmpdir/

mkdir $dir

# CHDIR TEST

# no profile, verify we didn't break normal fchdir
runchecktest "FCHDIR/no profile" pass $dir

# null profile, verify fchdir (x) functions
genprofile $dir:r
runchecktest "FCHDIR/profile" pass $dir
