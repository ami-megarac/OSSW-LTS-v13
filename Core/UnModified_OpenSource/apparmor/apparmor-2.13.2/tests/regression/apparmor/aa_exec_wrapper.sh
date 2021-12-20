#! /bin/bash
#	Copyright (C) 2015 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

if [ $# -ne 2 ]; then
	echo "FAIL: usage: $0 AA_EXEC_CMD EXPECTED_PROC_ATTR_CURRENT"
	echo "AA_EXEC_CMD			The path to aa-exec and the arguments to pass"
	echo "EXPECTED_PROC_ATTR_CURRENT	The expected contents of /proc/self/attr/current"
	exit 1
fi

out=$($1 -- cat /proc/self/attr/current 2>&1)
rc=$?

if [ $rc -eq 0 ] && [ "$out" == "$2" ]; then
	echo PASS
	exit 0
elif [ $rc -ne 0 ]; then
	echo "FAIL: aa-exec exited with status ${rc}:\n${out}\n"
	exit 1
else
	echo "FAIL: bad confinement context: \"$out\" != \"$2 $3\""
	exit 1
fi
