#! /bin/bash
#	Copyright (C) 2013 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME dbus_service
#=DESCRIPTION
# This test verifies that dbus services are restricted for confined processes.
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

service_runtestbg()
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

service_checktestbg()
{
	kill -SIGTERM $_pid 2>/dev/null
	checktestbg "$@"
}

service_runchecktest()
{
	service_runtestbg "$@"
	service_checktestbg
}

service_gendbusprofile()
{
	gendbusprofile "$unconfined_log w,
  $@"
}

start_bus

# Make sure we can bind a bus name and receive a message unconfined

settest dbus_service

service_runtestbg "service (unconfined)" pass $confined_log
sendmethod
sendsignal
service_checktestbg

# Make sure we get denials when confined but not allowed

genprofile
service_runchecktest "service (confined w/o dbus perms)" fail

service_gendbusprofile "dbus send,"
service_runchecktest "service (send allowed)" fail

service_gendbusprofile "dbus receive,"
service_runchecktest "service (receive allowed)" fail

service_gendbusprofile "dbus bind,"
service_runchecktest "service (bind allowed)" fail

# Make sure we're okay when confined with appropriate permissions

service_gendbusprofile "dbus,"
service_runtestbg "service (dbus allowed)" pass $unconfined_log
sendmethod
sendsignal
service_checktestbg "compare_logs $unconfined_log eq $confined_log"

service_gendbusprofile "dbus (send, receive, bind),"
service_runtestbg "service (send receive bind allowed)" pass $unconfined_log
sendmethod
sendsignal
service_checktestbg "compare_logs $unconfined_log eq $confined_log"

service_gendbusprofile "dbus (send receive bind) bus=session,"
service_runtestbg "service (send receive bind w/ bus)" pass $unconfined_log
sendmethod
sendsignal
service_checktestbg "compare_logs $unconfined_log eq $confined_log"

service_gendbusprofile "dbus bind bus=session name=$dest, \
		dbus receive bus=session, \
		dbus send bus=session peer=(name=org.freedesktop.DBus),"
service_runtestbg "service (receive bind w/ bus, dest)" pass $unconfined_log
sendmethod
sendsignal
service_checktestbg "compare_logs $unconfined_log eq $confined_log"

service_gendbusprofile "dbus bind bus=session name=$dest, \
		dbus receive bus=session, \
		dbus send bus=session peer=(name=org.freedesktop.DBus),"
service_runtestbg "service (receive bind w/ bus, dest)" pass $unconfined_log
sendmethod
sendsignal
service_checktestbg "compare_logs $unconfined_log eq $confined_log"

# Make sure we're denied when confined without appropriate conditionals

service_gendbusprofile "dbus bind bus=system name=$dest, \
		dbus receive bus=system, \
		dbus send bus=session peer=(name=org.freedesktop.DBus),"
service_runchecktest "service (receive bind w/ wrong bus)" fail

service_gendbusprofile "dbus bind bus=session name=${dest}.BAD, \
		dbus receive bus=session, \
		dbus send bus=session peer=(name=org.freedesktop.DBus),"
service_runchecktest "service (receive bind w/ wrong dest)" fail
