#! /bin/bash
#	Copyright (C) 20011 Canonical
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME introspect
#=DESCRIPTION Test process confinement introspection

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

ok_ix_perm=rix
badperm=r
ok_ux_perm=ux
ok_px_perm=px
bad_mx_perm=rm

#self unconfined
runchecktest "introspect self unconfined" pass self unconfined

#self unconfined (mode)
runchecktest "introspect self unconfined (mode)" fail self unconfined enforce

#self confined - no access to introspection
genprofile
runchecktest "introspect self confined" fail self "$testexec"

#self confined
genprofile "/proc/*/attr/current":r
runchecktest "introspect self confined" pass self "$testexec"

#self confined (enforce)
runchecktest "introspect self confined" pass self "$testexec" enforce

#### TODO

# query unconfined of unconfined

# query unconfined of confined

# query unconfined of confined (enfore)

# query confined of unconfined - no access permission

# query confined of unconfined - access permission

# query confined of unconfined (mode) - access permission

# query confined of confined same profile - no access permission

# query confined of confined same profile

# query confined of confined same profile (enforce)

# query confined of confined diff profile - no access permission

# query confined of confined diff profile

# query confined of confined diff profile (enforce)

