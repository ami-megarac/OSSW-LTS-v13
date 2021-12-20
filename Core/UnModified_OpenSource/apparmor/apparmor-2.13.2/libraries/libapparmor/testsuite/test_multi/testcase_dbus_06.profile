/tmp/apparmor-2.8.0/tests/regression/apparmor/dbus_service {
  dbus receive bus=system path=/org/freedesktop/nm_dhcp_client interface=org.freedesktop.nm_dhcp_client member=Signal peer=(label=unconfined),

}
