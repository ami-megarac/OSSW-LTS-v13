#! /bin/bash
#	Copyright (C) 2013 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME dbus_unrequested_reply
#=DESCRIPTION
# This test verifies that unrequested reply messages are not allowed through.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc
requires_kernel_features dbus
requires_parser_support "dbus,"
. $bin/dbus.inc

service="--$bus --name=$dest $path $iface"
unconfined_log="${tmpdir}/unconfined.log"
confined_log="${tmpdir}/confined.log"

ur_runtestbg()
{
	local lock=${tmpdir}/lock
	local lockfd=-1
	local args=$service

	if [ $# -gt 2 ]
	then
		args="--log=$3 $args"
	fi

	exec {lockfd}>$lock
	flock -n $lockfd
	args="--lock-fd=$lockfd $args"

	runtestbg "$1" "$2" $args

	exec {lockfd}>&-
	flock -w 30 $lock true
	rm $lock
}

ur_checktestbg()
{
	kill -SIGTERM $_pid 2>/dev/null
	checktestbg "$@"
}

ur_runchecktest()
{
	ur_runtestbg "$@"
	ur_checktestbg
}

ur_gendbusprofile()
{
	gendbusprofile "$confined_log w,
  dbus bind bus=$bus name=$dest,
  $@"
}

start_bus

settest dbus_service

# Start a dbus service and send unrequested method_return and error messages to
# the service. The service should always start and stop just fine. The test
# results hinge on comparing the message log from confined services to the
# message log from the initial unconfined run.

# Do an unconfined run to get an "expected" log for comparisons
ur_runtestbg "unrequested_reply (method_return, unconfined)" pass $unconfined_log
sendmethodreturn
ur_checktestbg

# All dbus perms are granted so the logs should be equal
ur_gendbusprofile "dbus,"
ur_runtestbg "unrequested_reply (method_return, dbus allowed)" pass $confined_log
sendmethodreturn
ur_checktestbg "compare_logs $unconfined_log eq $confined_log"

# Only send perm is granted so the confined service should not be able to
# receive unrequested replies from the client
ur_gendbusprofile "dbus send,"
ur_runtestbg "unrequested_reply (method_return, send allowed)" pass $confined_log
sendmethodreturn
ur_checktestbg "compare_logs $unconfined_log ne $confined_log"

# Send and receive perms are granted so the logs should be equal
ur_gendbusprofile "dbus (send receive),"
ur_runtestbg "unrequested_reply (method_return, send receive allowed)" pass $confined_log
sendmethodreturn
ur_checktestbg "compare_logs $unconfined_log eq $confined_log"

# Now test unrequested error replies

# Do an unconfined run to get an "expected" log for comparisons
removeprofile
ur_runtestbg "unrequested_reply (error, unconfined)" pass $unconfined_log
senderror
ur_checktestbg

# All dbus perms are granted so the logs should be equal
ur_gendbusprofile "dbus,"
ur_runtestbg "unrequested_reply (error, dbus allowed)" pass $confined_log
senderror
ur_checktestbg "compare_logs $unconfined_log eq $confined_log"

# Only send perm is granted so the confined service should not be able to
# receive unrequested replies from the client
ur_gendbusprofile "dbus send,"
ur_runtestbg "unrequested_reply (error, send allowed)" pass $confined_log
senderror
ur_checktestbg "compare_logs $unconfined_log ne $confined_log"

# Send and receive perms are granted so the logs should be equal
ur_gendbusprofile "dbus (send receive),"
ur_runtestbg "unrequested_reply (error, send receive allowed)" pass $confined_log
senderror
ur_checktestbg "compare_logs $unconfined_log eq $confined_log"
