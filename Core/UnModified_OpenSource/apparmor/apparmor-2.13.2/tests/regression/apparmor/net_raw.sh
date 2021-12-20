#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME net_raw
#=DESCRIPTION 
# AppArmor requires the net_raw capability in order to open a raw socket.  
# This test verifies this.
#=END

pwd=$(dirname $0)
pwd=$(cd $pwd ; /bin/pwd)

bin=$pwd

. $bin/prologue.inc

dir=$tmpdir/tmpdir

# NET_RAW FAIL
genprofile 
runchecktest "RAW SOCKET (no cap)" fail

# NET_RAW PASS (?)
genprofile cap:net_raw network:
runchecktest "RAW SOCKET (cap net_raw)" pass

