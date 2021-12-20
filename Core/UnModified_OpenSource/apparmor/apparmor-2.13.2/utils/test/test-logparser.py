# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
#    Copyright (C) 2015 Christian Boltz <apparmor@cboltz.de>
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License as published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
# ----------------------------------------------------------------------
import unittest

from apparmor.logparser import ReadLog

class TestParseEvent(unittest.TestCase):
    def setUp(self):
        self.parser = ReadLog('', '', '', '')

    def test_parse_event_audit_1(self):
        event = 'type=AVC msg=audit(1345027352.096:499): apparmor="ALLOWED" operation="rename_dest" parent=6974 profile="/usr/sbin/httpd2-prefork//vhost_foo" name=2F686F6D652F7777772F666F6F2E6261722E696E2F68747470646F63732F61707061726D6F722F696D616765732F746573742F696D61676520312E6A7067 pid=20143 comm="httpd2-prefork" requested_mask="wc" denied_mask="wc" fsuid=30 ouid=30'
        parsed_event = self.parser.parse_event(event)
        self.assertEqual(parsed_event['name'], '/home/www/foo.bar.in/httpdocs/apparmor/images/test/image 1.jpg')
        self.assertEqual(parsed_event['profile'], '/usr/sbin/httpd2-prefork//vhost_foo')
        self.assertEqual(parsed_event['aamode'], 'PERMITTING')
        self.assertEqual(parsed_event['request_mask'], 'wc')

        self.assertIsNotNone(ReadLog.RE_LOG_ALL.search(event))

    def test_parse_event_audit_2(self):
        event = 'type=AVC msg=audit(1322614918.292:4376): apparmor="ALLOWED" operation="file_perm" parent=16001 profile=666F6F20626172 name="/home/foo/.bash_history" pid=17011 comm="bash" requested_mask="rw" denied_mask="rw" fsuid=0 ouid=1000'
        parsed_event = self.parser.parse_event(event)
        self.assertEqual(parsed_event['name'], '/home/foo/.bash_history')
        self.assertEqual(parsed_event['profile'], 'foo bar')
        self.assertEqual(parsed_event['aamode'], 'PERMITTING')
        self.assertEqual(parsed_event['request_mask'], 'rw')

        self.assertIsNotNone(ReadLog.RE_LOG_ALL.search(event))

    def test_parse_event_syslog_1(self):
        # from https://bugs.launchpad.net/apparmor/+bug/1399027
        event = '2014-06-09T20:37:28.975070+02:00 geeko kernel: [21028.143765] type=1400 audit(1402339048.973:1421): apparmor="ALLOWED" operation="open" profile="/home/cb/linuxtag/apparmor/scripts/hello" name="/dev/tty" pid=14335 comm="hello" requested_mask="rw" denied_mask="rw" fsuid=1000 ouid=0'
        parsed_event = self.parser.parse_event(event)
        self.assertEqual(parsed_event['name'], '/dev/tty')
        self.assertEqual(parsed_event['profile'], '/home/cb/linuxtag/apparmor/scripts/hello')
        self.assertEqual(parsed_event['aamode'], 'PERMITTING')
        self.assertEqual(parsed_event['request_mask'], 'rw')

        self.assertIsNotNone(ReadLog.RE_LOG_ALL.search(event))

    def test_parse_event_syslog_2(self):
        # from https://bugs.launchpad.net/apparmor/+bug/1399027
        event = 'Dec  7 13:18:59 rosa kernel: audit: type=1400 audit(1417954745.397:82): apparmor="ALLOWED" operation="open" profile="/home/simi/bin/aa-test" name="/usr/bin/" pid=3231 comm="ls" requested_mask="r" denied_mask="r" fsuid=1000 ouid=0'
        parsed_event = self.parser.parse_event(event)
        self.assertEqual(parsed_event['name'], '/usr/bin/')
        self.assertEqual(parsed_event['profile'], '/home/simi/bin/aa-test')
        self.assertEqual(parsed_event['aamode'], 'PERMITTING')
        self.assertEqual(parsed_event['request_mask'], 'r')

        self.assertIsNotNone(ReadLog.RE_LOG_ALL.search(event))

    def test_parse_disconnected_path(self):
        # from https://bugzilla.opensuse.org/show_bug.cgi?id=918787
        event = 'type=AVC msg=audit(1424425690.883:716630): apparmor="ALLOWED" operation="file_mmap" info="Failed name lookup - disconnected path" error=-13 profile="/sbin/klogd" name="var/run/nscd/passwd" pid=25333 comm="id" requested_mask="r" denied_mask="r" fsuid=1002 ouid=0'
        parsed_event = self.parser.parse_event(event)

        self.assertEqual(parsed_event, {
            'aamode': 'ERROR',   # aamode for disconnected paths overridden aamode in parse_event()
            'active_hat': None,
            'attr': None,
            'denied_mask': 'r',
            'error_code': 13,
            'fsuid': 1002,
            'info': 'Failed name lookup - disconnected path',
            'magic_token': 0,
            'name': 'var/run/nscd/passwd',
            'name2': None,
            'operation': 'file_mmap',
            'ouid': 0,
            'parent': 0,
            'pid': 25333,
            'profile': '/sbin/klogd',
            'request_mask': 'r',
            'resource': 'Failed name lookup - disconnected path',
            'task': 0,
            'time': 1424425690,
            'family': None,
            'protocol': None,
            'sock_type': None,
        })

        self.assertIsNotNone(ReadLog.RE_LOG_ALL.search(event))


if __name__ == "__main__":
    unittest.main(verbosity=1)
