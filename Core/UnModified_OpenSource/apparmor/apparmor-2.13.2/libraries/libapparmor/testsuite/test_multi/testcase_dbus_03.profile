/tmp/apparmor-2.8.0/tests/regression/apparmor/dbus_service {
  dbus receive bus=session path=/com/apparmor/Test interface=com.apparmor.Test member=Signal peer=(label=unconfined),

}
