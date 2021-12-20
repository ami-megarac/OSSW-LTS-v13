#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

# strace.sh, helper function for generating ptrace data

# Trace output is not needed by test harness so /dev/null by default
# Change here if data is needed to be retained for manual examination.
_tracefile=/dev/null

ret=`/usr/bin/strace -o $_tracefile $* 2>&1`

if [ $? -eq 0 ]
then
	echo "PASS"
else
	echo "FAIL: $ret"
fi
