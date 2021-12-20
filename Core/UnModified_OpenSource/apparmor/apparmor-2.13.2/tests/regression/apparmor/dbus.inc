# vim:syntax=sh
#
#	Copyright (C) 2013 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

gendbusprofile()
{
	genprofile --stdin <<EOF
${__dbus_var_decl}
$test {
  @{gen $test}
  unix,
  $@
  signal receive peer=unconfined,
}
EOF
	unset __dbus_var_decl
}

# the arguments passed are emitted in the profile's prologue, for
# setting profile variables, e.g.
#	set_dbus_var "@{MY_DBUS_VAR}=stuff"
# the saved variable declaration gets unset after each test run
set_dbus_var()
{
  __dbus_var_decl=$@
}

start_bus()
{
	out=$(dbus-daemon --fork --print-pid --print-address --config-file=dbus.conf)
	if [ $? -ne 0 ]
	then
		fatalerror "Failed to start DBus daemon"
	fi

	bus_addr=$(echo $out | cut -d\  -f 1)
	bus_pid=$(echo $out | cut -d\  -f 2)

	# The daemon may error out during initialization. Unfortunately,
	# there's no good way to detect that, but this will work under normal
	# conditions.
	sleep 1
	kill -0 $bus_pid 2>/dev/null
	if [ $? -ne 0 ]
	then
		fatalerror "DBus daemon unexpectedly stopped"
	fi

	do_onexit="kill $bus_pid"
	export DBUS_SESSION_BUS_ADDRESS=$bus_addr
}

bus="session"
dest=com.apparmor.Test
path=/com/apparmor/Test
iface=com.apparmor.Test

# parameters: bus message_type destination path interface.member
#
# destination must be a connection name or "broadcast" for a broadcast signal
send()
{
  d=""

  if [ "$3" == "broadcast" ]
  then
    if [ "$2" != "signal" ]
    then
      fatalerror "Cannot send broadcast for message type \"$2\""
    fi
  else
    d="--name=$3"
  fi

  out=$(./dbus_message --$1 --type=$2 $d $4 $5 2>&1)
  if [ $? -ne 0 ]
  then
    fatalerror "$out"
  fi
}

sendsignal()
{
  send "$bus" "signal" "$dest" "$path" "${iface}.Signal"
}

sendbroadcastsignal()
{
  send "$bus" "signal" "broadcast" "$path" "${iface}.Signal"
}

sendmethod()
{
  send "$bus" "method_call" "$dest" "$path" "${iface}.Method"
}

# parameters: bus message_type destination
#
# destination must be a connection name
sendunrequestedreply()
{
  out=$(./dbus_unrequested_reply --$1 --type=$2 --name=$3 2>&1)
  if [ $? -ne 0 ]
  then
    fatalerror "$out"
  fi
}

sendmethodreturn()
{
  sendunrequestedreply "$bus" "method_return" "$dest"
}

senderror()
{
  sendunrequestedreply "$bus" "error" "$dest"
}

compare_logs()
{
	local msg
	local rc=0

	cmp -s "$1" "$3" || rc=$?
	if [ $rc -ne 0 ] && [ "$2" == "eq" ]
	then
		msg="Log files \"$1\" and \"$3\" are different, but should be equal."
	elif [ $rc -eq 0 ] && [ "$2" == "ne" ]
	then
		msg="Log files \"$1\" and \"$3\" are the same, but should be different."
	else
		return
	fi

	echo "Error: ${testname} failed. Test '${_testdesc}' produced unexpected log contents. ${msg}"
	testfailed
}
