#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2015-2018 Christian Boltz <apparmor@cboltz.de>
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import unittest
from common_test import AATest, setup_all_loops, setup_aa, read_file

import os
from apparmor.common import open_file_read

import apparmor.aa
from apparmor.logparser import ReadLog
from apparmor.profile_list import ProfileList

class TestLibapparmorTestMulti(AATest):
    '''Parse all libraries/libapparmor/testsuite/test_multi tests and compare the result with the *.out files'''

    tests = 'invalid'  # filled by parse_test_profiles()

    def _run_test(self, params, expected):
        # tests[][expected] is a dummy, replace it with the real values
        expected = self._parse_libapparmor_test_multi(params)

        with open_file_read('%s.in' % params) as f_in:
            loglines = f_in.readlines()

        loglines2 = []
        for line in loglines:
            if line.strip():
                loglines2 += [line]

        self.assertEqual(len(loglines2), 1, '%s.in should only contain one line!' % params)

        parser = ReadLog('', '', '', '')
        parsed_event = parser.parse_event(loglines2[0])

        if parsed_event and expected:
            parsed_items = dict(parsed_event.items())

            # check if the line passes the regex in logparser.py
            if not parser.RE_LOG_ALL.search(loglines2[0]):
                raise Exception("Log event doesn't match RE_LOG_ALL")

            for label in expected:
                if label in [
                        'file',  # filename of the *.in file
                        'event_type',  # mapped to aamode
                        'audit_id', 'audit_sub_id',  # not set nor relevant
                        'comm',  # not set, and not too useful
                        # XXX most of the keywords listed below mean "TODO"
                        'fsuid', 'ouid',  # file events
                        'flags', 'fs_type',  # mount
                        'namespace',  # file_lock only?? (at least the tests don't contain this in other event types with namespace)
                        'net_local_addr', 'net_foreign_addr', 'net_local_port', 'net_foreign_port',  # detailed network events
                        'peer', 'signal',  # signal
                        'src_name',  # pivotroot
                        'dbus_bus', 'dbus_interface', 'dbus_member', 'dbus_path',  # dbus
                        'peer_pid', 'peer_profile',  # dbus
                        ]:
                    pass
                elif parsed_items['operation'] == 'exec' and label in ['sock_type', 'family', 'protocol']:
                    pass  # XXX 'exec' + network? really?
                elif parsed_items['operation'] == 'ptrace' and label == 'name2' and params.endswith('/ptrace_garbage_lp1689667_1'):
                    pass  # libapparmor would better qualify this case as invalid event
                elif not parsed_items.get(label, None):
                    raise Exception('parsed_items[%s] not set' % label)
                elif not expected.get(label, None):
                    raise Exception('expected[%s] not set' % label)
                else:
                    self.assertEqual(str(parsed_items[label]), expected[label], '%s differs' % label)
        elif expected:
            self.assertIsNone(parsed_event)  # that's why we end up here
            self.assertEqual(dict(), expected, 'parsed_event is none')  # effectively print the content of expected
        elif parsed_event:
            self.assertIsNone(expected)  # that's why we end up here
            self.assertEqual(parsed_event, dict(), 'expected is none')  # effectively print the content of parsed_event
        else:
            self.assertIsNone(expected)  # that's why we end up here
            self.assertIsNone(parsed_event)  # that's why we end up here
            self.assertEqual(parsed_event, expected)  # both are None

    # list of labels that use a different name in logparser.py than in the test_multi *.out files
    # (additionally, .lower() is applied to all labels)
    label_map = {
        'Mask':             'request_mask',
        'Command':          'comm',
        'Token':            'magic_token',
        'ErrorCode':        'error_code',
        'Network family':   'family',
        'Socket type':      'sock_type',
        'Local addr':       'net_local_addr',
        'Foreign addr':     'net_foreign_addr',
        'Local port':       'net_local_port',
        'Foreign port':     'net_foreign_port',
        'Audit subid':      'audit_sub_id',
        'Attribute':        'attr',
        'Epoch':            'time',
    }

    def _parse_libapparmor_test_multi(self, file_with_path):
        '''parse the libapparmor test_multi *.in tests and their expected result in *.out'''

        with open_file_read('%s.out' % file_with_path) as f_in:
            expected = f_in.readlines()

        if expected[0].rstrip('\n') != 'START':
            raise Exception("%s.out doesn't have 'START' in its first line! (%s)" % ( file_with_path, expected[0]))

        expected.pop(0)

        exresult = dict()
        for line in expected:
            label, value = line.split(':', 1)

            # test_multi doesn't always use the original labels :-/
            if label in self.label_map.keys():
                label = self.label_map[label]
            label = label.replace(' ', '_').lower()
            exresult[label] = value.strip()

        if not exresult['event_type'].startswith('AA_RECORD_'):
            raise Exception("event_type doesn't start with AA_RECORD_: %s in file %s" % (exresult['event_type'], file_with_path))

        exresult['aamode'] = exresult['event_type'].replace('AA_RECORD_', '')
        if exresult['aamode'] == 'ALLOWED':
            exresult['aamode'] = 'PERMITTING'
        if exresult['aamode'] == 'DENIED':
            exresult['aamode'] = 'REJECTING'

        if exresult['event_type'] == 'AA_RECORD_INVALID':  # or exresult.get('error_code', 0) != 0:  # XXX should events with errors be ignored?
            exresult = None

        return exresult


# tests that do not produce the expected profile (checked with assertNotEqual)
log_to_profile_known_failures = [
    'testcase_dmesg_changeprofile_01',  # change_profile not yet supported in logparser
    'testcase_changeprofile_01',        # change_profile not yet supported in logparser

    'testcase_mount_01',  # mount rules not yet supported in logparser

    'testcase_pivotroot_01',  # pivot_rot not yet supported in logparser

    # exec events
    'testcase01',
    'testcase12',
    'testcase13',

    # null-* hats get ignored by handle_children() if it didn't see an exec event for that null-* hat
    'syslog_datetime_01',
    'syslog_datetime_02',
    'syslog_datetime_03',
    'syslog_datetime_04',
    'syslog_datetime_05',
    'syslog_datetime_06',
    'syslog_datetime_07',
    'syslog_datetime_08',
    'syslog_datetime_09',
    'syslog_datetime_10',
    'syslog_datetime_11',
    'syslog_datetime_12',
    'syslog_datetime_13',
    'syslog_datetime_14',
    'syslog_datetime_15',
    'syslog_datetime_16',
    'syslog_datetime_17',
    'syslog_datetime_18',
    'testcase_network_send_receive',
]

# tests that cause crashes or need user interaction (will be skipped)
log_to_profile_skip = [
    'testcase31',  # XXX AppArmorBug: Log contains unknown mode mrwIxl

    'testcase_dmesg_changehat_negative_error',   # fails in write_header -> quote_if_needed because data is None
    'testcase_syslog_changehat_negative_error',  # fails in write_header -> quote_if_needed because data is None

    'testcase_changehat_01',  # interactive, asks to add a hat
]

class TestLogToProfile(AATest):
    '''Check if the libraries/libapparmor/testsuite/test_multi tests result in the expected profile'''

    tests = 'invalid'  # filled by parse_test_profiles()

    def _run_test(self, params, expected):
        logfile = '%s.in' % params
        profile_dummy_file = 'AATest_does_exist'

        # we need to find out the profile name and aamode (complain vs. enforce mode) so that the test can access the correct place in storage
        parser = ReadLog('', '', '', '')
        parsed_event = parser.parse_event(read_file(logfile))

        if not parsed_event:  # AA_RECORD_INVALID
            return

        if params.split('/')[-1] in log_to_profile_skip:
            return

        aamode = parsed_event['aamode']

        if aamode in['AUDIT', 'STATUS', 'HINT']: # ignore some event types  # XXX maybe we shouldn't ignore AUDIT events?
            return

        if aamode not in ['PERMITTING', 'REJECTING']:
            raise Exception('Unexpected aamode %s' % parsed_event['aamode'])

        # cleanup apparmor.aa storage
        apparmor.aa.log = dict()
        apparmor.aa.aa = apparmor.aa.hasher()
        apparmor.aa.prelog = apparmor.aa.hasher()

        profile = parsed_event['profile']
        hat = profile
        if '//' in profile:
            profile, hat = profile.split('//')

        apparmor.aa.active_profiles = ProfileList()

        # optional for now, might be needed one day
        # if profile.startswith('/'):
        #     apparmor.aa.active_profiles.add(profile_dummy_file, profile, profile)
        # else:
        apparmor.aa.active_profiles.add(profile_dummy_file, profile, '')

        log_reader = ReadLog(dict(), logfile, apparmor.aa.active_profiles, '')
        log = log_reader.read_log('')

        for root in log:
            apparmor.aa.handle_children('', '', root)  # interactive for exec events!

        log_dict = apparmor.aa.collapse_log()

        apparmor.aa.filelist = apparmor.aa.hasher()
        apparmor.aa.filelist[profile_dummy_file]['profiles'][profile] = True

        new_profile = apparmor.aa.serialize_profile(log_dict[aamode][profile], profile, None)

        expected_profile = read_file('%s.profile' % params)

        if params.split('/')[-1] in log_to_profile_known_failures:
            self.assertNotEqual(new_profile, expected_profile)  # known failure
        else:
            self.assertEqual(new_profile, expected_profile)


def find_test_multi(log_dir):
    '''find all log sniplets in the given log_dir'''

    log_dir = os.path.abspath(log_dir)

    tests = []
    for root, dirs, files in os.walk(log_dir):
        for file in files:
            if file.endswith('.in'):
                file_with_path = os.path.join(root, file[:-3])  # filename without '.in'
                tests.append([file_with_path, True])  # True is a dummy testresult, parsing of the *.out files is done while running the tests

            elif file.endswith('.out') or file.endswith('.err') or file.endswith('.profile'):
                pass
            else:
                raise Exception('Found unknown file %s in libapparmor test_multi' % file)

    return tests


print('Testing libapparmor test_multi tests...')
TestLibapparmorTestMulti.tests = find_test_multi('../../libraries/libapparmor/testsuite/test_multi/')
TestLogToProfile.tests = find_test_multi('../../libraries/libapparmor/testsuite/test_multi/')

setup_aa(apparmor.aa)
setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
