/tmp/apparmor-2.8.0/tests/regression/apparmor/dbus_service {
  dbus send bus=system path=/org/freedesktop/DBus interface=org.freedesktop.DBus member=Hello peer=(label=unconfined),

}
