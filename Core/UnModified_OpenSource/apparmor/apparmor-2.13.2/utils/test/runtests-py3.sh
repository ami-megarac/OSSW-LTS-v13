#!/bin/sh
# ------------------------------------------------------------------
#
#    Copyright (C) 2014 Christian Boltz
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

test -z "$RUNTESTS_PY__PYTHON_BINARY" && RUNTESTS_PY__PYTHON_BINARY=python3

export PYTHONPATH=.. 

failed=""
for file in *.py ; do 
	echo "running $file..." 
	"$RUNTESTS_PY__PYTHON_BINARY" $file || { 
		failed="$failed $file"
		echo "*** test $file failed - giving you some time to read the output... ***"
		sleep 10
	}
	echo
done

test -z "$failed" || {
	echo
	echo "*** The following tests failed:"
	echo "***   $failed"
	exit 1
}

