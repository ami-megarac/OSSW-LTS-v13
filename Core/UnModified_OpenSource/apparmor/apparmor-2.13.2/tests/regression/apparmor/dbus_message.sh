#! /bin/bash
#	Copyright (C) 2013 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME dbus_message
#=DESCRIPTION
# This test verifies that the dbus message sending is indeed restricted for
# confined processes.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc
requires_kernel_features dbus
requires_parser_support "dbus,"
. $bin/dbus.inc

listnames="--type=method_call --session --name=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.ListNames"

unconfined_log="${tmpdir}/unconfined.log"
unconfined_args="--log=$unconfined_log $listnames"

confined_log="${tmpdir}/confined.log"
confined_args="--log=$confined_log $listnames"

message_gendbusprofile()
{
	gendbusprofile "${confined_log} w,
  $@"
}

start_bus

settest dbus_message

# Make sure can send unconfined

runchecktest "message (unconfined)" pass $unconfined_args

# Make sure send is denied when confined but not allowed

message_gendbusprofile
runchecktest "message (confined w/o dbus allowed)" fail $confined_args

message_gendbusprofile "dbus receive,"
runchecktest "message (receive allowed)" fail $confined_args

message_gendbusprofile "dbus bind,"
runchecktest "message (bind allowed)" fail $confined_args

message_gendbusprofile "dbus (receive, bind),"
runchecktest "message (receive bind allowed)" fail $confined_args

# Make sure send is allowed when confined with appropriate permissions

message_gendbusprofile "dbus,"
runtestfg "message (dbus allowed)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

message_gendbusprofile "dbus send,"
runtestfg "message (send allowed)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

message_gendbusprofile "dbus (send, receive),"
runtestfg "message (send receive allowed)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

message_gendbusprofile "dbus (send, bind),"
runtestfg "message (send bind allowed)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

message_gendbusprofile "dbus (send, receive, bind),"
runtestfg "message (send receive bind allowed)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

# Make sure send is allowed when confined with appropriate permissions along
# with conditionals

message_gendbusprofile "dbus send bus=session,"
runtestfg "message (send allowed w/ bus)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

message_gendbusprofile "dbus send bus=session peer=(name=org.freedesktop.DBus),"
runtestfg "message (send allowed w/ bus, dest)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

message_gendbusprofile "dbus send bus=session path=/org/freedesktop/DBus peer=(name=org.freedesktop.DBus),"
runchecktest "message (send allowed w/ bus, dest, path)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

message_gendbusprofile "dbus send bus=session path=/org/freedesktop/DBus interface=org.freedesktop.DBus peer=(name=org.freedesktop.DBus),"
runtestfg "message (send allowed w/ bus, dest, path, interface)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

message_gendbusprofile "dbus send bus=session path=/org/freedesktop/DBus interface=org.freedesktop.DBus member={Hello,ListNames} peer=(name=org.freedesktop.DBus),"
runtestfg "message (send allowed w/ bus, dest, path, interface, method)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

# Make sure send is allowed when confined with appropriate permissions along
# with conditionals and variables (same tests as above, with vars)

set_dbus_var "@{BUSES}=session system"
message_gendbusprofile "dbus send bus=@{BUSES},"
runtestfg "message (send allowed w/ bus)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

set_dbus_var "@{PEERNAMES}=com.ubuntu.what net.apparmor.wiki org.freedesktop.DBus"
message_gendbusprofile "dbus send bus=session peer=(name=@{PEERNAMES}),"
runtestfg "message (send allowed w/ bus, dest)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

set_dbus_var "@{PATHNAMES}=DBus spork spoon spork"
message_gendbusprofile "dbus send bus=session path=/org/freedesktop/@{PATHNAMES} peer=(name=org.freedesktop.DBus),"
runchecktest "message (send allowed w/ bus, dest, path)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

set_dbus_var "@{INTERFACE_NAMES}=DBus spork spoon spork"
message_gendbusprofile "dbus send bus=session path=/org/freedesktop/DBus interface=org.freedesktop.@{INTERFACE_NAMES} peer=(name=org.freedesktop.DBus),"
runtestfg "message (send allowed w/ bus, dest, path, interface)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

set_dbus_var "@{MEMBERS}=Hello ListNames Spork Spoon"
message_gendbusprofile "dbus send bus=session path=/org/freedesktop/DBus interface=org.freedesktop.DBus member=@{MEMBERS} peer=(name=org.freedesktop.DBus),"
runtestfg "message (send allowed w/ bus, dest, path, interface, method)" pass $confined_args
checktestfg "compare_logs $unconfined_log eq $confined_log"

# Make sure send is denied when confined with appropriate permissions along
# with incorrect conditionals

message_gendbusprofile "dbus send bus=system,"
runtestfg "message (send allowed w/ wrong bus)" fail $confined_args
checktestfg "compare_logs $unconfined_log ne $confined_log"

message_gendbusprofile "dbus send bus=session peer=(name=com.freedesktop.DBus),"
runtestfg "message (send allowed w/ wrong dest)" fail $confined_args
checktestfg "compare_logs $unconfined_log ne $confined_log"

message_gendbusprofile "dbus send bus=session path=/bad/freedesktop/DBus peer=(name=bad.freedesktop.DBus),"
runtestfg "message (send allowed w/ wrong path)" fail $confined_args
checktestfg "compare_logs $unconfined_log ne $confined_log"

message_gendbusprofile "dbus send bus=session path=/org/freedesktop/DBus interface=bad.freedesktop.DBus peer=(name=bad.freedesktop.DBus),"
runtestfg "message (send allowed w/ wrong interface)" fail $confined_args
checktestfg "compare_logs $unconfined_log ne $confined_log"

message_gendbusprofile "dbus send bus=session path=/org/freedesktop/DBus interface=com.freedesktop.DBus member=Hello peer=(name=bad.freedesktop.DBus),"
runtestfg "message (send allowed w/ wrong method)" fail $confined_args
checktestfg "compare_logs $unconfined_log ne $confined_log"
