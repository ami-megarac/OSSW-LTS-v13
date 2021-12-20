#!/bin/bash
#
#	Copyright (C) 2002-2006 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

if [ -z "$1" ] ; then
	echo "Usage: $0 var=value"
	exit 1;
fi

VAR=${1%%=*}
VALUE=${1#*=}
ENVVAL=$(eval echo \${$VAR})

#echo ENVVAL = $ENVVAL

if [ -z "${ENVVAL}" ] ; then
	echo "FAIL: Environment variable \$$VAR is unset"
	exit 1;
fi

if [ "${ENVVAL}" != "${VALUE}" ] ; then
	echo "FAIL: Environment variable \$$VAR differs; expected $VALUE got ${ENVVAL}"
	exit 1;
fi

exit 0
