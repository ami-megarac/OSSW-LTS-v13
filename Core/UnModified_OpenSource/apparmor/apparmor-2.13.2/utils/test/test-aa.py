#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2014-2015 Christian Boltz
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import unittest
from common_test import AATest, setup_all_loops, setup_aa
from common_test import read_file, write_file

import os
import shutil
import sys

import apparmor.aa  # needed to set global vars in some tests
from apparmor.aa import (check_for_apparmor, get_output, get_reqs, get_interpreter_and_abstraction, create_new_profile,
     get_profile_flags, change_profile_flags, set_options_audit_mode, set_options_owner_mode, is_skippable_file, is_skippable_dir,
     parse_profile_start, parse_profile_data, separate_vars, store_list_var, write_header,
     var_transform, serialize_parse_profile_start, get_file_perms, propose_file_rules)
from apparmor.aare import AARE
from apparmor.common import AppArmorException, AppArmorBug
from apparmor.rule.file import FileRule

class AaTestWithTempdir(AATest):
    def AASetup(self):
        self.createTmpdir()


class AaTest_check_for_apparmor(AaTestWithTempdir):
    FILESYSTEMS_WITH_SECURITYFS = 'nodev\tdevtmpfs\nnodev\tsecurityfs\nnodev\tsockfs\n\text3\n\text2\n\text4'
    FILESYSTEMS_WITHOUT_SECURITYFS = 'nodev\tdevtmpfs\nnodev\tsockfs\n\text3\n\text2\n\text4'

    MOUNTS_WITH_SECURITYFS = ( 'proc /proc proc rw,relatime 0 0\n'
        'securityfs %s/security securityfs rw,nosuid,nodev,noexec,relatime 0 0\n'
        '/dev/sda1 / ext3 rw,noatime,data=ordered 0 0' )

    MOUNTS_WITHOUT_SECURITYFS = ( 'proc /proc proc rw,relatime 0 0\n'
        '/dev/sda1 / ext3 rw,noatime,data=ordered 0 0' )

    def test_check_for_apparmor_None_1(self):
        filesystems = write_file(self.tmpdir, 'filesystems', self.FILESYSTEMS_WITHOUT_SECURITYFS)
        mounts = write_file(self.tmpdir, 'mounts', self.MOUNTS_WITH_SECURITYFS)
        self.assertEqual(None, check_for_apparmor(filesystems, mounts))

    def test_check_for_apparmor_None_2(self):
        filesystems = write_file(self.tmpdir, 'filesystems', self.FILESYSTEMS_WITHOUT_SECURITYFS)
        mounts = write_file(self.tmpdir, 'mounts', self.MOUNTS_WITHOUT_SECURITYFS)
        self.assertEqual(None, check_for_apparmor(filesystems, mounts))

    def test_check_for_apparmor_None_3(self):
        filesystems = write_file(self.tmpdir, 'filesystems', self.FILESYSTEMS_WITH_SECURITYFS)
        mounts = write_file(self.tmpdir, 'mounts', self.MOUNTS_WITHOUT_SECURITYFS)
        self.assertEqual(None, check_for_apparmor(filesystems, mounts))

    def test_check_for_apparmor_securityfs_invalid_filesystems(self):
        filesystems = ''
        mounts = write_file(self.tmpdir, 'mounts', self.MOUNTS_WITH_SECURITYFS % self.tmpdir)
        self.assertEqual(None, check_for_apparmor(filesystems, mounts))

    def test_check_for_apparmor_securityfs_invalid_mounts(self):
        filesystems = write_file(self.tmpdir, 'filesystems', self.FILESYSTEMS_WITH_SECURITYFS)
        mounts = ''
        self.assertEqual(None, check_for_apparmor(filesystems, mounts))

    def test_check_for_apparmor_invalid_securityfs_path(self):
        filesystems = write_file(self.tmpdir, 'filesystems', self.FILESYSTEMS_WITH_SECURITYFS)
        mounts = write_file(self.tmpdir, 'mounts', self.MOUNTS_WITH_SECURITYFS % 'xxx')
        self.assertEqual(None, check_for_apparmor(filesystems, mounts))

    def test_check_for_apparmor_securityfs_mounted(self):
        filesystems = write_file(self.tmpdir, 'filesystems', self.FILESYSTEMS_WITH_SECURITYFS)
        mounts = write_file(self.tmpdir, 'mounts', self.MOUNTS_WITH_SECURITYFS % self.tmpdir)
        self.assertEqual('%s/security/apparmor' % self.tmpdir, check_for_apparmor(filesystems, mounts))

class AATest_get_output(AATest):
    tests = [
        (['./fake_ldd', '/AATest/lib64/libc-2.22.so'],  (0, ['        /AATest/lib64/ld-linux-x86-64.so.2 (0x0000556858473000)', '        linux-vdso.so.1 (0x00007ffe98912000)']     )),
        (['./fake_ldd', '/tmp/aa-test-foo'],            (0, ['        not a dynamic executable']                                                                                    )),
        (['./fake_ldd', 'invalid'],                     (1, []                                                                                                                      )),  # stderr is not part of output
    ]
    def _run_test(self, params, expected):
        self.assertEqual(get_output(params), expected)

    def test_get_output_nonexisting(self):
        with self.assertRaises(AppArmorException):
            ret, output = get_output(['./_file_/_not_/_found_'])

class AATest_get_reqs(AATest):
    tests = [
        ('/AATest/bin/bash',    ['/AATest/lib64/libreadline.so.6', '/AATest/lib64/libtinfo.so.6', '/AATest/lib64/libdl.so.2', '/AATest/lib64/libc.so.6', '/AATest/lib64/ld-linux-x86-64.so.2']),
        ('/tmp/aa-test-foo',    []),
        ('/AATest/sbin/ldconfig', []),  # comes with $? == 1
    ]

    def _run_test(self, params, expected):
        # for some reason, setting the ldd config option does not get
        # honored in python2.7
        # XXX KILL when python 2.7 is dropped XXX
        if sys.version_info[0] < 3:
            print("Skipping on python < 3.x")
            return
        apparmor.aa.cfg['settings']['ldd'] = './fake_ldd'

        self.assertEqual(get_reqs(params), expected)

class AaTest_create_new_profile(AATest):
    tests = [
        # file content              expected interpreter    expected abstraction (besides 'base')
        ('#!/bin/bash\ntrue',      (u'/bin/bash',           'abstractions/bash')),
        ('foo bar',                (None,                   None)),
    ]
    def _run_test(self, params, expected):
        apparmor.aa.cfg['settings']['ldd'] = './fake_ldd'
        # for some reason, setting the ldd config option does not get
        # honored in python2.7
        # XXX KILL when python 2.7 is dropped XXX
        if sys.version_info[0] < 3:
            print("Skipping on python < 3.x")
            return

        self.createTmpdir()

        #copy the local profiles to the test directory
        self.profile_dir = '%s/profiles' % self.tmpdir
        shutil.copytree('../../profiles/apparmor.d/', self.profile_dir, symlinks=True)

        # load the abstractions we need in the test
        apparmor.aa.profiledir = self.profile_dir
        apparmor.aa.load_include('abstractions/base')
        apparmor.aa.load_include('abstractions/bash')

        exp_interpreter_path, exp_abstraction = expected
        # damn symlinks!
        if exp_interpreter_path:
            exp_interpreter_path = os.path.realpath(exp_interpreter_path)

        program = self.writeTmpfile('script', params)
        profile = create_new_profile(program)

        if exp_interpreter_path:
            self.assertEqual(set(profile[program][program]['file'].get_clean()), {'%s ix,' % exp_interpreter_path, '%s r,' % program, '',
                    '/AATest/lib64/libtinfo.so.* mr,', '/AATest/lib64/libc.so.* mr,', '/AATest/lib64/libdl.so.* mr,', '/AATest/lib64/libreadline.so.* mr,', '/AATest/lib64/ld-linux-x86-64.so.* mr,' })
        else:
            self.assertEqual(set(profile[program][program]['file'].get_clean()), {'%s mr,' % program, ''})

        if exp_abstraction:
            self.assertEqual(set(profile[program][program]['include'].keys()), {exp_abstraction, 'abstractions/base'})
        else:
            self.assertEqual(set(profile[program][program]['include'].keys()), {'abstractions/base'})

class AaTest_get_interpreter_and_abstraction(AATest):
    tests = [
        ('#!/bin/bash',             ('/bin/bash',           'abstractions/bash')),
        ('#!/bin/dash',             ('/bin/dash',           'abstractions/bash')),
        ('#!/bin/sh',               ('/bin/sh',             'abstractions/bash')),
        ('#!  /bin/sh  ',           ('/bin/sh',             'abstractions/bash')),
        ('#!  /bin/sh  -x ',        ('/bin/sh',             'abstractions/bash')),  # '-x' is not part of the interpreter path
        ('#!/usr/bin/perl',         ('/usr/bin/perl',       'abstractions/perl')),
        ('#!/usr/bin/perl -w',      ('/usr/bin/perl',       'abstractions/perl')),  # '-w' is not part of the interpreter path
        ('#!/usr/bin/python',       ('/usr/bin/python',     'abstractions/python')),
        ('#!/usr/bin/python2',      ('/usr/bin/python2',    'abstractions/python')),
        ('#!/usr/bin/python2.7',    ('/usr/bin/python2.7',  'abstractions/python')),
        ('#!/usr/bin/python3',      ('/usr/bin/python3',    'abstractions/python')),
        ('#!/usr/bin/python4',      ('/usr/bin/python4',    None)),  # python abstraction is only applied to py2 and py3
        ('#!/usr/bin/ruby',         ('/usr/bin/ruby',       'abstractions/ruby')),
        ('#!/usr/bin/ruby2.2',      ('/usr/bin/ruby2.2',    'abstractions/ruby')),
        ('#!/usr/bin/ruby1.9.1',    ('/usr/bin/ruby1.9.1',  'abstractions/ruby')),
        ('#!/usr/bin/foobarbaz',    ('/usr/bin/foobarbaz',  None)),  # we don't have an abstraction for "foobarbaz"
        ('foo',                     (None,                  None)),  # no hashbang - not a script
    ]

    def _run_test(self, params, expected):
        exp_interpreter_path, exp_abstraction = expected

        program = self.writeTmpfile('program', "%s\nfoo\nbar" % params)
        interpreter_path, abstraction = get_interpreter_and_abstraction(program)

        # damn symlinks!
        if exp_interpreter_path:
            exp_interpreter_path = os.path.realpath(exp_interpreter_path)

        self.assertEqual(interpreter_path, exp_interpreter_path)
        self.assertEqual(abstraction, exp_abstraction)

    def test_special_file(self):
        self.assertEqual((None, None), get_interpreter_and_abstraction('/dev/null'))

    def test_file_not_found(self):
        self.createTmpdir()
        self.assertEqual((None, None), get_interpreter_and_abstraction('%s/file-not-found' % self.tmpdir))


class AaTest_get_profile_flags(AaTestWithTempdir):
    def _test_get_flags(self, profile_header, expected_flags):
        file = write_file(self.tmpdir, 'profile', '%s {\n}\n' % profile_header)
        flags = get_profile_flags(file, '/foo')
        self.assertEqual(flags, expected_flags)

    def test_get_flags_01(self):
        self._test_get_flags('/foo', None)
    def test_get_flags_02(self):
        self._test_get_flags('/foo (  complain  )', '  complain  ')
    def test_get_flags_04(self):
        self._test_get_flags('/foo (complain)', 'complain')
    def test_get_flags_05(self):
        self._test_get_flags('/foo flags=(complain)', 'complain')
    def test_get_flags_06(self):
        self._test_get_flags('/foo flags=(complain,  audit)', 'complain,  audit')

    def test_get_flags_invalid_01(self):
        with self.assertRaises(AppArmorException):
            self._test_get_flags('/foo ()', None)
    def test_get_flags_invalid_02(self):
        with self.assertRaises(AppArmorException):
            self._test_get_flags('/foo flags=()', None)
    def test_get_flags_invalid_03(self):
        with self.assertRaises(AppArmorException):
            self._test_get_flags('/foo (  )', '  ')

    def test_get_flags_other_profile(self):
        with self.assertRaises(AppArmorException):
            self._test_get_flags('/no-such-profile flags=(complain)', 'complain')

class AaTest_change_profile_flags(AaTestWithTempdir):
    def _test_change_profile_flags(self, profile, old_flags, flags_to_change, set_flag, expected_flags, whitespace='', comment='',
                        more_rules='', expected_more_rules='@-@-@',
                        check_new_flags=True, profile_name='/foo'):
        if old_flags:
            old_flags = ' %s' % old_flags

        if expected_flags:
            expected_flags = ' flags=(%s)' % (expected_flags)
        else:
            expected_flags = ''

        if expected_more_rules == '@-@-@':
            expected_more_rules = more_rules

        if comment:
            comment = ' %s' % comment

        dummy_profile_content = '  #include <abstractions/base>\n  capability chown,\n  /bar r,'
        prof_template = '%s%s%s {%s\n%s\n%s\n}\n'
        old_prof = prof_template % (whitespace, profile, old_flags,      comment, more_rules,          dummy_profile_content)
        new_prof = prof_template % (whitespace, profile, expected_flags, comment, expected_more_rules, dummy_profile_content)

        self.file = write_file(self.tmpdir, 'profile', old_prof)
        change_profile_flags(self.file, profile_name, flags_to_change, set_flag)
        if check_new_flags:
            real_new_prof = read_file(self.file)
            self.assertEqual(new_prof, real_new_prof)

    # tests that actually don't change the flags
    def test_change_profile_flags_nochange_02(self):
        self._test_change_profile_flags('/foo', '(  complain  )', 'complain', True, 'complain', whitespace='   ')
    def test_change_profile_flags_nochange_03(self):
        self._test_change_profile_flags('/foo', '(complain)', 'complain', True, 'complain')
    def test_change_profile_flags_nochange_04(self):
        self._test_change_profile_flags('/foo', 'flags=(complain)', 'complain', True, 'complain')
    def test_change_profile_flags_nochange_05(self):
        self._test_change_profile_flags('/foo', 'flags=(complain,  audit)', 'complain', True, 'audit, complain', whitespace='     ')
    def test_change_profile_flags_nochange_06(self):
        self._test_change_profile_flags('/foo', 'flags=(complain,  audit)', 'complain', True, 'audit, complain', whitespace='     ', comment='# a comment')
    def test_change_profile_flags_nochange_07(self):
        self._test_change_profile_flags('/foo', 'flags=(complain,  audit)', 'audit', True, 'audit, complain', whitespace='     ', more_rules='  # a comment\n#another  comment')
    def test_change_profile_flags_nochange_08(self):
        self._test_change_profile_flags('profile /foo', 'flags=(complain)', 'complain', True, 'complain')
    def test_change_profile_flags_nochange_09(self):
        self._test_change_profile_flags('profile xy /foo', 'flags=(complain)', 'complain', True, 'complain', profile_name='xy')
    def test_change_profile_flags_nochange_10(self):
        self._test_change_profile_flags('profile "/foo bar"', 'flags=(complain)', 'complain', True, 'complain', profile_name='/foo bar')
    def test_change_profile_flags_nochange_11(self):
        self._test_change_profile_flags('/foo', '(complain)', 'complain', True, 'complain', profile_name=None)
    def test_change_profile_flags_nochange_12(self):
        # XXX changes the flags for the child profile (which happens to have the same profile name) to 'complain'
        self._test_change_profile_flags('/foo', 'flags=(complain)', 'complain', True, 'complain', more_rules='  profile /foo {\n}', expected_more_rules='  profile /foo flags=(complain) {\n}')

    # tests that change the flags
    def test_change_profile_flags_01(self):
        self._test_change_profile_flags('/foo', '', 'audit', True, 'audit')
    def test_change_profile_flags_02(self):
        self._test_change_profile_flags('/foo', '(  complain  )', 'audit', True, 'audit, complain', whitespace='  ')
    def test_change_profile_flags_04(self):
        self._test_change_profile_flags('/foo', '(complain)', 'audit', True, 'audit, complain')
    def test_change_profile_flags_05(self):
        self._test_change_profile_flags('/foo', 'flags=(complain)', 'audit', True, 'audit, complain')
    def test_change_profile_flags_06(self):
        self._test_change_profile_flags('/foo', 'flags=(complain,  audit)', 'complain', False, 'audit', whitespace='    ')
    def test_change_profile_flags_07(self):
        self._test_change_profile_flags('/foo', 'flags=(complain,  audit)', 'audit', False, 'complain')
    def test_change_profile_flags_08(self):
        self._test_change_profile_flags('/foo', '(  complain  )', 'audit', True, 'audit, complain', whitespace='  ', profile_name=None)
    def test_change_profile_flags_09(self):
        self._test_change_profile_flags('profile /foo', 'flags=(complain)', 'audit', True, 'audit, complain')
    def test_change_profile_flags_10(self):
        self._test_change_profile_flags('profile xy /foo', 'flags=(complain)', 'audit', True, 'audit, complain', profile_name='xy')
    def test_change_profile_flags_11(self):
        self._test_change_profile_flags('profile "/foo bar"', 'flags=(complain)', 'audit', True, 'audit, complain', profile_name='/foo bar')
    def test_change_profile_flags_12(self):
        self._test_change_profile_flags('profile xy "/foo bar"', 'flags=(complain)', 'audit', True, 'audit, complain', profile_name='xy')
    def test_change_profile_flags_13(self):
        self._test_change_profile_flags('/foo', '(audit)', 'audit', False, '')

    # test handling of hat flags
    def test_set_flags_with_hat_01(self):
        self._test_change_profile_flags('/foo', 'flags=(complain)', 'audit', True, 'audit, complain',
            more_rules='\n  ^foobar {\n}\n',
            expected_more_rules='\n  ^foobar flags=(audit) {\n}\n'
        )

    def test_change_profile_flags_with_hat_02(self):
        self._test_change_profile_flags('/foo', 'flags=(complain)', 'audit', False, 'complain',
            profile_name=None,
            more_rules='\n  ^foobar flags=(audit) {\n}\n',
            expected_more_rules='\n  ^foobar {\n}\n'
        )

    def test_change_profile_flags_with_hat_03(self):
        self._test_change_profile_flags('/foo', 'flags=(complain)', 'audit', True, 'audit, complain',
            more_rules='\n^foobar (attach_disconnected) { # comment\n}\n',
            expected_more_rules='\n^foobar flags=(attach_disconnected, audit) { # comment\n}\n'
        )

    def test_change_profile_flags_with_hat_04(self):
        self._test_change_profile_flags('/foo', '', 'audit', True, 'audit',
            more_rules='\n  hat foobar (attach_disconnected) { # comment\n}\n',
            expected_more_rules='\n  hat foobar flags=(attach_disconnected, audit) { # comment\n}\n'
        )

    def test_change_profile_flags_with_hat_05(self):
        self._test_change_profile_flags('/foo', '(audit)', 'audit', False, '',
            more_rules='\n  hat foobar (attach_disconnected) { # comment\n}\n',
            expected_more_rules='\n  hat foobar flags=(attach_disconnected) { # comment\n}\n'
        )

    # test handling of child profiles
    def test_change_profile_flags_with_child_01(self):
        self._test_change_profile_flags('/foo', 'flags=(complain)', 'audit', True, 'audit, complain',
            profile_name=None,
            more_rules='\n  profile /bin/bar {\n}\n',
            expected_more_rules='\n  profile /bin/bar flags=(audit) {\n}\n'
        )

    def test_change_profile_flags_with_child_02(self):
        # XXX child profile flags aren't changed if profile parameter is not None
        self._test_change_profile_flags('/foo', 'flags=(complain)', 'audit', True, 'audit, complain',
            more_rules='\n  profile /bin/bar {\n}\n',
            expected_more_rules='\n  profile /bin/bar {\n}\n'  # flags(audit) should be added
        )


    def test_change_profile_flags_invalid_01(self):
        with self.assertRaises(AppArmorBug):
            self._test_change_profile_flags('/foo', '()', None, False, '', check_new_flags=False)
    def test_change_profile_flags_invalid_02(self):
        with self.assertRaises(AppArmorBug):
            self._test_change_profile_flags('/foo', 'flags=()', None, True, '', check_new_flags=False)
    def test_change_profile_flags_invalid_03(self):
        with self.assertRaises(AppArmorBug):
            self._test_change_profile_flags('/foo', '(  )', '', True, '', check_new_flags=False)
    def test_change_profile_flags_invalid_04(self):
        with self.assertRaises(AppArmorBug):
            self._test_change_profile_flags('/foo', 'flags=(complain,  audit)', '  ', True, 'audit, complain', check_new_flags=False) # whitespace-only newflags

    def test_change_profile_flags_other_profile(self):
        # test behaviour if the file doesn't contain the specified /foo profile
        orig_prof = '/no-such-profile flags=(complain) {\n}'
        self.file = write_file(self.tmpdir, 'profile', orig_prof)

        with self.assertRaises(AppArmorException):
            change_profile_flags(self.file, '/foo', 'audit', True)

        # the file should not be changed
        real_new_prof = read_file(self.file)
        self.assertEqual(orig_prof, real_new_prof)

    def test_change_profile_flags_no_profile_found(self):
        # test behaviour if the file doesn't contain any profile
        orig_prof = '# /comment flags=(complain) {\n# }'
        self.file = write_file(self.tmpdir, 'profile', orig_prof)

        with self.assertRaises(AppArmorException):
            change_profile_flags(self.file, None, 'audit', True)

        # the file should not be changed
        real_new_prof = read_file(self.file)
        self.assertEqual(orig_prof, real_new_prof)

    def test_change_profile_flags_file_not_found(self):
        with self.assertRaises(IOError):
            change_profile_flags('%s/file-not-found' % self.tmpdir, '/foo', 'audit', True)

class AaTest_set_options_audit_mode(AATest):
    tests = [
        ((FileRule.parse('audit /foo/bar r,'),      ['/foo/bar r,', '/foo/* r,', '/** r,']                      ), ['audit /foo/bar r,', 'audit /foo/* r,', 'audit /** r,']),
        ((FileRule.parse('audit /foo/bar r,'),      ['/foo/bar r,', 'audit /foo/* r,', 'audit /** r,']          ), ['audit /foo/bar r,', 'audit /foo/* r,', 'audit /** r,']),
        ((FileRule.parse('/foo/bar r,'),            ['/foo/bar r,', '/foo/* r,', '/** r,']                      ), ['/foo/bar r,', '/foo/* r,', '/** r,']),
        ((FileRule.parse('/foo/bar r,'),            ['audit /foo/bar r,', 'audit /foo/* r,', 'audit /** r,']    ), ['/foo/bar r,', '/foo/* r,', '/** r,']),
        ((FileRule.parse('audit /foo/bar r,'),      ['/foo/bar r,', '/foo/* r,', '#include <abstractions/base>']), ['audit /foo/bar r,', 'audit /foo/* r,', '#include <abstractions/base>']),
    ]

    def _run_test(self, params, expected):
        rule_obj, options = params
        new_options = set_options_audit_mode(rule_obj, options)
        self.assertEqual(new_options, expected)

class AaTest_set_options_owner_mode(AATest):
    tests = [
        ((FileRule.parse('owner /foo/bar r,'),      ['/foo/bar r,', '/foo/* r,', '/** r,']                                  ), ['owner /foo/bar r,', 'owner /foo/* r,', 'owner /** r,']),
        ((FileRule.parse('owner /foo/bar r,'),      ['/foo/bar r,', 'owner /foo/* r,', 'owner /** r,']                      ), ['owner /foo/bar r,', 'owner /foo/* r,', 'owner /** r,']),
        ((FileRule.parse('/foo/bar r,'),            ['/foo/bar r,', '/foo/* r,', '/** r,']                                  ), ['/foo/bar r,', '/foo/* r,', '/** r,']),
        ((FileRule.parse('/foo/bar r,'),            ['owner /foo/bar r,', 'owner /foo/* r,', 'owner /** r,']                ), ['/foo/bar r,', '/foo/* r,', '/** r,']),
        ((FileRule.parse('audit owner /foo/bar r,'),['audit /foo/bar r,', 'audit /foo/* r,', '#include <abstractions/base>']), ['audit owner /foo/bar r,', 'audit owner /foo/* r,', '#include <abstractions/base>']),
    ]

    def _run_test(self, params, expected):
        rule_obj, options = params
        new_options = set_options_owner_mode(rule_obj, options)
        self.assertEqual(new_options, expected)

class AaTest_is_skippable_file(AATest):
    def test_not_skippable_01(self):
        self.assertFalse(is_skippable_file('bin.ping'))
    def test_not_skippable_02(self):
        self.assertFalse(is_skippable_file('usr.lib.dovecot.anvil'))
    def test_not_skippable_03(self):
        self.assertFalse(is_skippable_file('bin.~ping'))
    def test_not_skippable_04(self):
        self.assertFalse(is_skippable_file('bin.rpmsave.ping'))
    def test_not_skippable_05(self):
        # normally is_skippable_file should be called without directory, but it shouldn't hurt too much
        self.assertFalse(is_skippable_file('/etc/apparmor.d/bin.ping'))
    def test_not_skippable_06(self):
        self.assertFalse(is_skippable_file('bin.pingrej'))

    def test_skippable_01(self):
        self.assertTrue(is_skippable_file('bin.ping.dpkg-new'))
    def test_skippable_02(self):
        self.assertTrue(is_skippable_file('bin.ping.dpkg-old'))
    def test_skippable_03(self):
        self.assertTrue(is_skippable_file('bin.ping..dpkg-dist'))
    def test_skippable_04(self):
        self.assertTrue(is_skippable_file('bin.ping..dpkg-bak'))
    def test_skippable_05(self):
        self.assertTrue(is_skippable_file('bin.ping.dpkg-remove'))
    def test_skippable_06(self):
        self.assertTrue(is_skippable_file('bin.ping.pacsave'))
    def test_skippable_07(self):
        self.assertTrue(is_skippable_file('bin.ping.pacnew'))
    def test_skippable_08(self):
        self.assertTrue(is_skippable_file('bin.ping.rpmnew'))
    def test_skippable_09(self):
        self.assertTrue(is_skippable_file('bin.ping.rpmsave'))
    def test_skippable_10(self):
        self.assertTrue(is_skippable_file('bin.ping.orig'))
    def test_skippable_11(self):
        self.assertTrue(is_skippable_file('bin.ping.rej'))
    def test_skippable_12(self):
        self.assertTrue(is_skippable_file('bin.ping~'))
    def test_skippable_13(self):
        self.assertTrue(is_skippable_file('.bin.ping'))
    def test_skippable_14(self):
        self.assertTrue(is_skippable_file(''))  # empty filename
    def test_skippable_15(self):
        self.assertTrue(is_skippable_file('/etc/apparmor.d/'))  # directory without filename
    def test_skippable_16(self):
        self.assertTrue(is_skippable_file('README'))


class AaTest_is_skippable_dir(AATest):
    tests = [
        ('disable',                     True),
        ('cache',                       True),
        ('lxc',                         True),
        ('force-complain',              True),
        ('/etc/apparmor.d/cache',       True),
        ('/etc/apparmor.d/cache.d',     True),
        ('/etc/apparmor.d/cache.d/',    True),
        ('/etc/apparmor.d/lxc/',        True),
        ('/etc/apparmor.d/.git/',       True),

        ('dont_disable',                False),
        ('/etc/apparmor.d/cache_foo',   False),
        ('abstractions',                False),
        ('apache2.d',                   False),
        ('/etc/apparmor.d/apache2.d',   False),
        ('local',                       False),
        ('/etc/apparmor.d/local/',      False),
        ('tunables',                    False),
        ('/etc/apparmor.d/tunables',    False),
        ('/etc/apparmor.d/tunables/multiarch.d',            False),
        ('/etc/apparmor.d/tunables/xdg-user-dirs.d',        False),
        ('/etc/apparmor.d/tunables/home.d',                 False),
        ('/etc/apparmor.d/abstractions',                    False),
        ('/etc/apparmor.d/abstractions/ubuntu-browsers.d',  False),
        ('/etc/apparmor.d/abstractions/apparmor_api',       False),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(is_skippable_dir(params), expected)

class AaTest_parse_profile_start(AATest):
    def _parse(self, line, profile, hat):
        return parse_profile_start(line, 'somefile', 1, profile, hat)
        # (profile, hat, flags, in_contained_hat, pps_set_profile, pps_set_hat_external)

    def test_parse_profile_start_01(self):
        result = self._parse('/foo {', None, None)
        expected = ('/foo', '/foo', None, None, False, False, False)
        self.assertEqual(result, expected)

    def test_parse_profile_start_02(self):
        result = self._parse('/foo (complain) {', None, None)
        expected = ('/foo', '/foo', None, 'complain', False, False, False)
        self.assertEqual(result, expected)

    def test_parse_profile_start_03(self):
        result = self._parse('profile foo /foo {', None, None) # named profile
        expected = ('foo', 'foo', '/foo', None, False, False, False)
        self.assertEqual(result, expected)

    def test_parse_profile_start_04(self):
        result = self._parse('profile /foo {', '/bar', '/bar') # child profile
        expected = ('/bar', '/foo', None, None, True, True, False)
        self.assertEqual(result, expected)

    def test_parse_profile_start_05(self):
        result = self._parse('/foo//bar {', None, None) # external hat
        expected = ('/foo', 'bar', None, None, False, False, True)
        self.assertEqual(result, expected)

    def test_parse_profile_start_06(self):
        result = self._parse('profile "/foo" (complain) {', None, None)
        expected = ('/foo', '/foo', None, 'complain', False, False, False)
        self.assertEqual(result, expected)

    def test_parse_profile_start_unsupported_01(self):
        with self.assertRaises(AppArmorException):
            self._parse('/foo///bar///baz {', None, None)  # XXX deeply nested external hat

    def test_parse_profile_start_invalid_01(self):
        with self.assertRaises(AppArmorException):
            self._parse('/foo {', '/bar', '/bar') # child profile without profile keyword

    def test_parse_profile_start_invalid_02(self):
        with self.assertRaises(AppArmorBug):
            self._parse('xy', '/bar', '/bar') # not a profile start

class AaTest_parse_profile_data(AATest):
    def test_parse_empty_profile_01(self):
        prof = parse_profile_data('/foo {\n}\n'.split(), 'somefile', False)

        self.assertEqual(list(prof.keys()), ['/foo'])
        self.assertEqual(list(prof['/foo'].keys()), ['/foo'])
        self.assertEqual(prof['/foo']['/foo']['name'], '/foo')
        self.assertEqual(prof['/foo']['/foo']['filename'], 'somefile')
        self.assertEqual(prof['/foo']['/foo']['flags'], None)

    def test_parse_empty_profile_02(self):
        with self.assertRaises(AppArmorException):
            # file contains two profiles with the same name
            parse_profile_data('profile /foo {\n}\nprofile /foo {\n}\n'.split(), 'somefile', False)

class AaTest_separate_vars(AATest):
    tests = [
        (''                             , set()                      ),
        ('       '                      , set()                      ),
        ('  foo bar'                    , {'foo', 'bar'             }),
        ('foo "  '                      , AppArmorException          ),
        (' " foo '                      , AppArmorException          ), # half-quoted
        ('  foo bar   '                 , {'foo', 'bar'             }),
        ('  foo bar   # comment'        , {'foo', 'bar', '#', 'comment'}), # XXX should comments be stripped?
        ('foo'                          , {'foo'                    }),
        ('"foo" "bar baz"'              , {'foo', 'bar baz'         }),
        ('foo "bar baz" xy'             , {'foo', 'bar baz', 'xy'   }),
        ('foo "bar baz '                , AppArmorException          ), # half-quoted
        ('  " foo" bar'                 , {' foo', 'bar'            }),
        ('  " foo" bar x'               , {' foo', 'bar', 'x'       }),
        ('""'                           , {''                       }), # empty value
        ('"" foo'                       , {'', 'foo'                }), # empty value + 'foo'
        ('"" foo "bar"'                 , {'', 'foo', 'bar'         }), # empty value + 'foo' + 'bar' (bar has superfluous quotes)
        ('"bar"'                        , {'bar'                    }), # 'bar' with superfluous quotes
    ]

    def _run_test(self, params, expected):
        if expected == AppArmorException:
            with self.assertRaises(expected):
                separate_vars(params)
        else:
            result = separate_vars(params)
            self.assertEqual(result, expected)


class AaTest_store_list_var(AATest):
    tests = [
        #  old var                        value        operation   expected (False for exception)
        ([ {}                           , 'foo'         , '='   ], {'foo'}                      ), # set
        ([ {}                           , 'foo bar'     , '='   ], {'foo', 'bar'}               ), # set multi
        ([ {'@{var}': {'foo'}}          , 'bar'         , '='   ], False                        ), # redefine var
        ([ {}                           , 'bar'         , '+='  ], False                        ), # add to undefined var
        ([ {'@{var}': {'foo'}}          , 'bar'         , '+='  ], {'foo', 'bar'}               ), # add
        ([ {'@{var}': {'foo'}}          , 'bar baz'     , '+='  ], {'foo', 'bar', 'baz'}        ), # add multi
        ([ {'@{var}': {'foo', 'xy'}}    , 'bar baz'     , '+='  ], {'foo', 'xy', 'bar', 'baz'}  ), # add multi to multi
        ([ {}                           , 'foo'         , '-='  ], False                        ), # unknown operation
    ]

    def _run_test(self, params, expected):
        var         = params[0]
        value       = params[1]
        operation   = params[2]

        if not expected:
            with self.assertRaises(AppArmorException):
                store_list_var(var, '@{var}', value, operation, 'somefile')
            return

        # dumy value that must not be changed
        var['@{foo}'] = {'one', 'two'}

        exp_var = {
            '@{foo}':   {'one', 'two'},
            '@{var}':   expected,
        }

        store_list_var(var, '@{var}', value, operation, 'somefile')

        self.assertEqual(var.keys(), exp_var.keys())

        for key in exp_var:
            self.assertEqual(var[key], exp_var[key])


class AaTest_write_header(AATest):
    tests = [
        # name       embedded_hat    write_flags    depth   flags           attachment  prof.keyw.  comment    expected
        (['/foo',    False,          True,          1,      'complain',     None,       None,       None    ],  '  /foo flags=(complain) {'),
        (['/foo',    True,           True,          1,      'complain',     None,       None,       None    ],  '  profile /foo flags=(complain) {'),
        (['/foo sp', False,          False,         2,      'complain',     None,       None,       None    ],  '    "/foo sp" {'),
        (['/foo'    ,False,          False,         2,      'complain',     None,       None,       None    ],  '    /foo {'),
        (['/foo',    True,           False,         2,      'complain',     None,       None,       None    ],  '    profile /foo {'),
        (['/foo',    False,          True,          0,      None,           None,       None,       None    ],  '/foo {'),
        (['/foo',    True,           True,          0,      None,           None,       None,       None    ],  'profile /foo {'),
        (['/foo',    False,          False,         0,      None,           None,       None,       None    ],  '/foo {'),
        (['/foo',    True,           False,         0,      None,           None,       None,       None    ],  'profile /foo {'),
        (['bar',     False,          True,          1,      'complain',     None,       None,       None    ],  '  profile bar flags=(complain) {'),
        (['bar',     False,          True,          1,      'complain',     '/foo',     None,       None    ],  '  profile bar /foo flags=(complain) {'),
        (['bar',     True,           True,          1,      'complain',     '/foo',     None,       None    ],  '  profile bar /foo flags=(complain) {'),
        (['bar baz', False,          True,          1,      None,           '/foo',     None,       None    ],  '  profile "bar baz" /foo {'),
        (['bar',     True,           True,          1,      None,           '/foo',     None,       None    ],  '  profile bar /foo {'),
        (['bar baz', False,          True,          1,      'complain',     '/foo sp',  None,       None    ],  '  profile "bar baz" "/foo sp" flags=(complain) {'),
        (['^foo',    False,          True,          1,      'complain',     None,       None,       None    ],  '  profile ^foo flags=(complain) {'),
        (['^foo',    True,           True,          1,      'complain',     None,       None,       None    ],  '  ^foo flags=(complain) {'),
        (['^foo',    True,           True,          1.5,    'complain',     None,       None,       None    ],  '   ^foo flags=(complain) {'),
        (['^foo',    True,           True,          1.3,    'complain',     None,       None,       None    ],  '  ^foo flags=(complain) {'),
        (['/foo',    False,          True,          1,      'complain',     None,       'profile',  None    ],  '  profile /foo flags=(complain) {'),
        (['/foo',    True,           True,          1,      'complain',     None,       'profile',  None    ],  '  profile /foo flags=(complain) {'),
        (['/foo',    False,          True,          1,      'complain',     None,       None,       '# x'   ],  '  /foo flags=(complain) { # x'),
        (['/foo',    True,           True,          1,      None,           None,       None,       '# x'   ],  '  profile /foo { # x'),
        (['/foo',    False,          True,          1,      None,           None,       'profile',  '# x'   ],  '  profile /foo { # x'),
        (['/foo',    True,           True,          1,      'complain',     None,       'profile',  '# x'   ],  '  profile /foo flags=(complain) { # x'),
     ]

    def _run_test(self, params, expected):
        name = params[0]
        embedded_hat = params[1]
        write_flags = params[2]
        depth = params[3]
        prof_data = { 'flags': params[4], 'attachment': params[5], 'profile_keyword': params[6], 'header_comment': params[7] }

        result = write_header(prof_data, depth, name, embedded_hat, write_flags)
        self.assertEqual(result, [expected])

class AaTest_var_transform(AATest):
    tests = [
        (['foo', ''],           'foo ""'        ),
        (['foo', 'bar'],        'foo bar'       ),
        ([''],                  '""'            ),
        (['bar baz', 'foo'],    '"bar baz" foo' ),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(var_transform(params), expected)

class AaTest_serialize_parse_profile_start(AATest):
    def _parse(self, line, profile, hat, prof_data_profile, prof_data_external):
        # 'correct' is always True in the code that uses serialize_parse_profile_start() (set some lines above the function call)
        return serialize_parse_profile_start(line, 'somefile', 1, profile, hat, prof_data_profile, prof_data_external, True)

    def test_serialize_parse_profile_start_01(self):
        result = self._parse('/foo {', None, None, False, False)
        expected = ('/foo', '/foo', None, None, False, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_02(self):
        result = self._parse('/foo (complain) {', None, None, False, False)
        expected = ('/foo', '/foo', None, 'complain', False, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_03(self):
        result = self._parse('profile foo /foo {', None, None, False, False) # named profile
        expected = ('foo', 'foo', '/foo', None, False, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_04(self):
        result = self._parse('profile /foo {', '/bar', '/bar', False, False) # child profile
        expected = ('/bar', '/foo', None, None, True, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_05(self):
        result = self._parse('/foo//bar {', None, None, False, False) # external hat
        expected = ('/foo', 'bar', None, None, False, False) # note correct == False here
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_06(self):
        result = self._parse('profile "/foo" (complain) {', None, None, False, False)
        expected = ('/foo', '/foo', None, 'complain', False, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_07(self):
        result = self._parse('/foo {', None, None, True, False)
        expected = ('/foo', '/foo', None, None, False, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_08(self):
        result = self._parse('/foo {', None, None, False, True)
        expected = ('/foo', '/foo', None, None, False, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_09(self):
        result = self._parse('/foo {', None, None, True, True)
        expected = ('/foo', '/foo', None, None, False, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_10(self):
        result = self._parse('profile /foo {', '/bar', '/bar', True, False) # child profile
        expected = ('/bar', '/foo', None, None, True, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_11(self):
        result = self._parse('profile /foo {', '/bar', '/bar', False, True) # child profile
        expected = ('/bar', '/foo', None, None, True, True)
        self.assertEqual(result, expected)

    def test_serialize_parse_profile_start_12(self):
        result = self._parse('profile /foo {', '/bar', '/bar', True, True) # child profile
        expected = ('/bar', '/foo', None, None, True, True)
        self.assertEqual(result, expected)

class AaTestInvalid_serialize_parse_profile_start(AATest):
    tests = [
        # line              profile     hat     p_d_profile p_d_external   expected
        (['/foo {',         '/bar',     '/bar', False,      False       ], AppArmorException), # child profile without 'profile' keyword
        (['profile /foo {', '/bar',     '/xy',  False,      False       ], AppArmorException), # already inside a child profile - nesting level reached
        (['/ext//hat {',    '/bar',     '/bar', True,       True        ], AppArmorException), # external hat inside a profile
        (['/ext//hat {',    '/bar',     '/bar', True,       False       ], AppArmorException), # external hat inside a profile
        (['xy',             '/bar',     '/bar', False,      False       ], AppArmorBug      ), # not a profile start
    ]

    def _run_test(self, params, expected):
        line = params[0]
        profile = params[1]
        hat = params[2]
        prof_data_profile = params[3]
        prof_data_external = params[4]

        with self.assertRaises(expected):
            # 'correct' is always True in the code that uses serialize_parse_profile_start() (set some lines above the function call)
            serialize_parse_profile_start(line, 'somefile', 1, profile, hat, prof_data_profile, prof_data_external, True)

class AaTest_get_file_perms_1(AATest):
    tests = [
        ('/usr/share/common-licenses/foo/bar',      {'allow': {'all': set(),            'owner': {'w'}  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/usr/share/common-licenses/**'}  }),
        ('/dev/null',                               {'allow': {'all': {'r', 'w', 'k'},  'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/dev/null'}                      }),
        ('/foo/bar',                                {'allow': {'all': {'r', 'w'},       'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/foo/bar'}                       }),  # exec perms not included
        ('/no/thing',                               {'allow': {'all': set(),            'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': set()                              }),
        ('/usr/lib/ispell/',                        {'allow': {'all': set(),            'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': set()                              }),
        ('/usr/lib/aspell/*.so',                    {'allow': {'all': set(),            'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': set()                              }),
    ]

    def _run_test(self, params, expected):
        self.createTmpdir()

        #copy the local profiles to the test directory
        self.profile_dir = '%s/profiles' % self.tmpdir
        shutil.copytree('../../profiles/apparmor.d/', self.profile_dir, symlinks=True)

        profile = apparmor.aa.ProfileStorage('/test', '/test', 'test-aa.py')

        # simple profile without any includes
        profile['file'].add(FileRule.parse('owner /usr/share/common-licenses/**  w,'))
        profile['file'].add(FileRule.parse('/dev/null rwk,'))
        profile['file'].add(FileRule.parse('/foo/bar rwix,'))

        perms = get_file_perms(profile, params, False, False)  # only testing with audit and deny = False
        self.assertEqual(perms, expected)

class AaTest_get_file_perms_2(AATest):
    tests = [
        ('/usr/share/common-licenses/foo/bar',      {'allow': {'all': {'r'},            'owner': {'w'}  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/usr/share/common-licenses/**'}              }),
        ('/usr/share/common-licenses/what/ever',    {'allow': {'all': {'r'},            'owner': {'w'}  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/usr/share/common-licenses/**', '/usr/share/common-licenses/what/ever'}      }),
        ('/dev/null',                               {'allow': {'all': {'r', 'w', 'k'},  'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/dev/null'}                                  }),
        ('/foo/bar',                                {'allow': {'all': {'r', 'w'},       'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/foo/bar'}                                   }),  # exec perms not included
        ('/no/thing',                               {'allow': {'all': set(),            'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': set()                                          }),
        ('/usr/lib/ispell/',                        {'allow': {'all': {'r'},            'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/usr/lib/ispell/', '/{usr/,}lib{,32,64}/**'}    }),  # from abstractions/enchant
        ('/usr/lib/aspell/*.so',                    {'allow': {'all': {'m', 'r'},       'owner': set()  }, 'deny': {'all':set(),    'owner': set()},    'paths': {'/usr/lib/aspell/*', '/usr/lib/aspell/*.so', '/{usr/,}lib{,32,64}/**', '/{usr/,}lib{,32,64}/**.so*'} }),  # from abstractions/aspell via abstractions/enchant and from abstractions/base
    ]

    def _run_test(self, params, expected):
        self.createTmpdir()

        #copy the local profiles to the test directory
        self.profile_dir = '%s/profiles' % self.tmpdir
        shutil.copytree('../../profiles/apparmor.d/', self.profile_dir, symlinks=True)

        # load the abstractions we need in the test
        apparmor.aa.profiledir = self.profile_dir
        apparmor.aa.load_include('abstractions/base')
        apparmor.aa.load_include('abstractions/bash')
        apparmor.aa.load_include('abstractions/enchant')
        apparmor.aa.load_include('abstractions/aspell')

        profile = apparmor.aa.ProfileStorage('/test', '/test', 'test-aa.py')
        profile['include']['abstractions/base'] = True
        profile['include']['abstractions/bash'] = True
        profile['include']['abstractions/enchant'] = True  # includes abstractions/aspell

        profile['file'].add(FileRule.parse('owner /usr/share/common-licenses/**  w,'))
        profile['file'].add(FileRule.parse('owner /usr/share/common-licenses/what/ever a,'))  # covered by the above 'w' rule, so 'a' should be ignored
        profile['file'].add(FileRule.parse('/dev/null rwk,'))
        profile['file'].add(FileRule.parse('/foo/bar rwix,'))

        perms = get_file_perms(profile, params, False, False)  # only testing with audit and deny = False
        self.assertEqual(perms, expected)

class AaTest_propose_file_rules(AATest):
    tests = [
        # log event path                   and perms    expected proposals
        (['/usr/share/common-licenses/foo/bar', 'w'],   ['/usr/share/common*/foo/* rw,', '/usr/share/common-licenses/** rw,', '/usr/share/common-licenses/foo/bar rw,']         ),
        (['/dev/null',                          'wk'],  ['/dev/null rwk,']                                                                                                      ),
        (['/foo/bar',                           'rw'],  ['/foo/bar rw,']                                                                                                        ),
        (['/usr/lib/ispell/',                   'w'],   ['/{usr/,}lib{,32,64}/** rw,', '/usr/lib/ispell/ rw,']                                                                     ),
        (['/usr/lib/aspell/some.so',            'k'],   ['/usr/lib/aspell/* mrk,', '/usr/lib/aspell/*.so mrk,', '/{usr/,}lib{,32,64}/** mrk,', '/{usr/,}lib{,32,64}/**.so* mrk,', '/usr/lib/aspell/some.so mrk,']     ),
        (['/foo/log',                           'w'],   ['/foo/log w,']                                                                                                            ),
    ]

    def _run_test(self, params, expected):
        self.createTmpdir()

        #copy the local profiles to the test directory
        self.profile_dir = '%s/profiles' % self.tmpdir
        shutil.copytree('../../profiles/apparmor.d/', self.profile_dir, symlinks=True)

        # load the abstractions we need in the test
        apparmor.aa.profiledir = self.profile_dir
        apparmor.aa.load_include('abstractions/base')
        apparmor.aa.load_include('abstractions/bash')
        apparmor.aa.load_include('abstractions/enchant')
        apparmor.aa.load_include('abstractions/aspell')

        # add some user_globs ('(N)ew') to simulate a professional aa-logprof user (and to make sure that part of the code also gets tested)
        apparmor.aa.user_globs['/usr/share/common*/foo/*'] = AARE('/usr/share/common*/foo/*', True)
        apparmor.aa.user_globs['/no/thi*ng'] = AARE('/no/thi*ng', True)

        profile = apparmor.aa.ProfileStorage('/test', '/test', 'test-aa.py')
        profile['include']['abstractions/base'] = True
        profile['include']['abstractions/bash'] = True
        profile['include']['abstractions/enchant'] = True  # includes abstractions/aspell

        profile['file'].add(FileRule.parse('owner /usr/share/common-licenses/**  w,'))
        profile['file'].add(FileRule.parse('/dev/null rwk,'))
        profile['file'].add(FileRule.parse('/foo/bar rwix,'))
        profile['file'].add(FileRule.parse('/foo/log a,'))  # will be replaced with '/foo/log w,' (not 'wa')

        rule_obj = FileRule(params[0], params[1], None, FileRule.ALL, owner=False, log_event=True)
        proposals = propose_file_rules(profile, rule_obj)
        self.assertEqual(proposals, expected)


class AaTest_propose_file_rules_with_absolute_includes(AATest):
    tests = [
        # log event path       and perms    expected proposals
        (['/not/found/anywhere',    'r'],   ['/not/found/anywhere r,']),
        (['/dev/null',              'w'],   ['/dev/null rw,']),
        (['/some/random/include',   'r'],   ['/some/random/include rw,']),
        (['/some/other/include',    'w'],   ['/some/other/* rw,', '/some/other/inc* rw,', '/some/other/include rw,']),
    ]

    def _run_test(self, params, expected):
        self.createTmpdir()

        #copy the local profiles to the test directory
        self.profile_dir = '%s/profiles' % self.tmpdir
        shutil.copytree('../../profiles/apparmor.d/', self.profile_dir, symlinks=True)

        # load the abstractions we need in the test
        apparmor.aa.profiledir = self.profile_dir
        apparmor.aa.load_include('abstractions/base')

        abs_include1 = write_file(self.tmpdir, 'test-abs1', "/some/random/include rw,")
        apparmor.aa.load_include(abs_include1)

        abs_include2 = write_file(self.tmpdir, 'test-abs2', "/some/other/* rw,")
        apparmor.aa.load_include(abs_include2)

        abs_include3 = write_file(self.tmpdir, 'test-abs3', "/some/other/inc* rw,")
        apparmor.aa.load_include(abs_include3)

        profile = apparmor.aa.ProfileStorage('/test', '/test', 'test-aa.py')
        profile['include']['abstractions/base'] = False
        profile['include'][abs_include1] = False
        profile['include'][abs_include2] = False
        profile['include'][abs_include3] = False

        rule_obj = FileRule(params[0], params[1], None, FileRule.ALL, owner=False, log_event=True)
        proposals = propose_file_rules(profile, rule_obj)
        self.assertEqual(proposals, expected)


class AaTest_nonexistent_includes(AATest):
    def test_bad_includes(self):
        tests = [
            "/nonexistent/absolute/path",
            "nonexistent/relative/path",
        ]

        for i in tests:
            with self.assertRaises(AppArmorException):
                apparmor.aa.load_include(i)


setup_aa(apparmor.aa)
setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
