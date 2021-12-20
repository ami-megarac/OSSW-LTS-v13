#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME clone
#=DESCRIPTION
# Verifies that clone is allowed under AppArmor, but that CLONE_NEWNS is
# restriced.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

# TEST1 unconfined

runchecktest "CLONE/unconfined" pass

# TEST2. confined

genprofile
runchecktest "CLONE/confined" pass

# TEST3. confined + CLONE_NEWNS

genprofile
runchecktest "CLONE/confined/NEWNS" fail --newns

# TEST4. confined + CLONE_NEWNS + cap_sys_admin

genprofile cap:sys_admin
runchecktest "CLONE/confined/NEWNS" pass --newns
