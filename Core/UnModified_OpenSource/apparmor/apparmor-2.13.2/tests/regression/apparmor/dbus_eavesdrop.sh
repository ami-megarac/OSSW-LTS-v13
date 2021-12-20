#! /bin/bash
#	Copyright (C) 2013 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME dbus_eavesdrop
#=DESCRIPTION
# This test verifies that dbus eavesdropping is restricted for confined
# processes.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc
requires_kernel_features dbus
requires_parser_support "dbus,"
. $bin/dbus.inc

args="--session"

start_bus

# Make sure we can eavesdrop unconfined

settest dbus_eavesdrop

runchecktest "eavesdrop (unconfined)" pass $args

# Make sure we get denials when confined but not allowed

genprofile
runchecktest "eavesdrop (confined w/o dbus perms)" fail $args

gendbusprofile "dbus send,"
runchecktest "eavesdrop (confined w/ only send allowed)" fail $args

gendbusprofile "dbus eavesdrop,"
runchecktest "eavesdrop (confined w/ only eavesdrop allowed)" fail $args

# Make sure we're okay when confined with appropriate permissions

gendbusprofile "dbus,"
runchecktest "eavesdrop (dbus allowed)" pass $args

gendbusprofile "dbus (send eavesdrop),"
runchecktest "eavesdrop (send, eavesdrop allowed)" pass $args

gendbusprofile "dbus (send eavesdrop) bus=session,"
runchecktest "eavesdrop (send, eavesdrop allowed w/ bus conditional)" pass $args

gendbusprofile "dbus send bus=session path=/org/freedesktop/DBus \
			interface=org.freedesktop.DBus \
			member=Hello, \
		dbus send bus=session path=/org/freedesktop/DBus \
			interface=org.freedesktop.DBus \
			member=AddMatch, \
		dbus eavesdrop bus=session,"
runchecktest "eavesdrop (send, eavesdrop allowed w/ bus and send member conditionals)" pass $args

gendbusprofile "dbus send, \
		audit dbus eavesdrop,"
runchecktest "eavesdrop (send allowed, eavesdrop audited)" pass $args

# Make sure we're denied when confined without appropriate conditionals

gendbusprofile "dbus send bus=session, \
		dbus eavesdrop bus=system,"
runchecktest "eavesdrop (wrong bus)" fail $args

gendbusprofile "dbus send, \
		deny dbus eavesdrop,"
runchecktest "eavesdrop (send allowed, eavesdrop denied)" fail $args
