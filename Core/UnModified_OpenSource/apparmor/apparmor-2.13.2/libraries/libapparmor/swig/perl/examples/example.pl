require LibAppArmor;

$msg = 'type=APPARMOR_ALLOWED msg=audit(1257283891.471:2232): operation="file_perm" pid=4064 parent=4002 profile="/usr/bin/gedit" requested_mask="w::" denied_mask="w::" fsuid=1000 ouid=1000 name="/home/jamie/.gnome2/accels/gedit"';

my($test) = LibAppArmorc::parse_record($msg);

if (LibAppArmor::aa_log_record::swig_event_get($test) == $LibAppArmor::AA_RECORD_ALLOWED )
{
	print "AA_RECORD_ALLOWED\n";
}

print "Audit ID: " . LibAppArmor::aa_log_record::swig_audit_id_get($test) . "\n";
print "PID: " . LibAppArmor::aa_log_record::swig_pid_get($test) . "\n";

LibAppArmorc::free_record($test);
