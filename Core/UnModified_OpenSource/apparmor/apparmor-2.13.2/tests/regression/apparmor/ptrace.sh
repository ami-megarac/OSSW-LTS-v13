#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#       Copyright (c) 2010 - 2014
#       Canonical Ltd. (All rights reserved)
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME ptrace
#=DESCRIPTION 
# Verify ptrace.  The tracing process (attacher or parent of ptrace_me) may 
# not be confined.
# 
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

# Read permission was required for a confined process to be able to be traced 
# using ptrace.  This stopped being required or functioning correctly 
# somewhere between 2.4.18 and 2.4.20.
#

helper=$pwd/ptrace_helper

bin_true=${tmpdir}/true
cp -pL /bin/true ${tmpdir}/true

# -n number of syscalls to perform
# -c have the child call ptrace_me, else parent does ptrace_attach
# -h transition child to ptrace_helper before doing ptrace (used to test
#  x transitions with ptrace)
# test base line of unconfined tracing unconfined
runchecktest "test 1" pass -n 100 ${bin_true}
runchecktest "test 1 -c" pass -c -n 100 ${bin_true}
runchecktest "test 1 -h" pass -h -n 100 $helper
runchecktest "test 1 -hc" pass -h -c -n 100 $helper
runchecktest "test 1 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 1 -hc prog" pass -h -c -n 100 $helper ${bin_true}

# test that unconfined can ptrace before profile attaches
genprofile image=${bin_true} signal:ALL
runchecktest "test 2" pass -n 100 ${bin_true}
runchecktest "test 2 -c" pass -c -n 100 ${bin_true}
runchecktest "test 2 -h" pass -h -n 100 $helper
runchecktest "test 2 -hc" pass -h -c -n 100 $helper
runchecktest "test 2 -h prog" pass -h -n 100 $helper ${bin_true}
runchecktest "test 2 -hc prog" pass -h -c -n 100 $helper ${bin_true}


if [ "$(kernel_features ptrace)" == "true" -a "$(parser_supports 'ptrace,')" == "true" ] ; then
	. $bin/ptrace_v6.inc
else
	. $bin/ptrace_v5.inc
fi
