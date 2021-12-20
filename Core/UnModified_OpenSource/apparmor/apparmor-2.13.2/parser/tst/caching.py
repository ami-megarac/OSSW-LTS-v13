#!/usr/bin/env python3
# ------------------------------------------------------------------
#
#   Copyright (C) 2013-2015 Canonical Ltd.
#   Authors: Steve Beattie <steve@nxnw.org>
#            Tyler Hicks <tyhicks@canonical.com>
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of version 2 of the GNU General Public
#   License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

# TODO
# - check cache not used if parser in $PATH is newer
# - check cache used for force-complain, disable symlink, etc.

from argparse import ArgumentParser
import os
import platform
import shutil
import time
import tempfile
import unittest

import testlib

ABSTRACTION_CONTENTS = '''
  # Simple example abstraction
  capability setuid,
'''
ABSTRACTION = 'suid-abstraction'

PROFILE_CONTENTS = '''
# Simple example profile for caching tests

/bin/pingy {
  #include <%s>
  capability net_raw,
  network inet raw,

  /bin/ping mixr,
  /etc/modules.conf r,
}
''' % (ABSTRACTION)
PROFILE = 'sbin.pingy'
config = None


class AAParserCachingCommon(testlib.AATestTemplate):
    do_cleanup = True

    def setUp(self):
        '''setup for each test'''
        global config

        # REPORT ALL THE OUTPUT
        self.maxDiff = None

        self.tmp_dir = tempfile.mkdtemp(prefix='aa-caching-')
        os.chmod(self.tmp_dir, 0o755)

        # write our sample abstraction and profile out
        self.abstraction = testlib.write_file(self.tmp_dir, ABSTRACTION, ABSTRACTION_CONTENTS)
        self.profile = testlib.write_file(self.tmp_dir, PROFILE, PROFILE_CONTENTS)

        if config.debug:
            self.do_cleanup = False
            self.debug = True

        # Warnings break the test harness, but chroots may not be setup
        # to have the config file, etc.
        self.cmd_prefix = [config.parser, '--config-file=./parser.conf', '--base', self.tmp_dir, '--skip-kernel-load']

        if not self.is_apparmorfs_mounted():
            self.cmd_prefix += ['-M', './features_files/features.all']

        # Otherwise get_cache_dir() will try to create /var/cache/apparmor
        # and will fail when the test suite is run as non-root.
        self.cmd_prefix += [
            '--cache-loc', os.path.join(self.tmp_dir, 'cache')
        ]

        # create directory for cached blobs
        # NOTE: get_cache_dir() requires cmd_prefix to be fully initialized
        self.cache_dir = self.get_cache_dir(create=True)

        # default path of the output cache file
        self.cache_file = os.path.join(self.cache_dir, PROFILE)

    def tearDown(self):
        '''teardown for each test'''

        if not self.do_cleanup:
            print("\n===> Skipping cleanup, leaving testfiles behind in '%s'" % (self.tmp_dir))
        else:
            if os.path.exists(self.tmp_dir):
                shutil.rmtree(self.tmp_dir)

    def get_cache_dir(self, create=False):
        cmd = [config.parser, '--print-cache-dir'] + self.cmd_prefix
        rc, report = self.run_cmd(cmd)
        if rc != 0:
            if "unrecognized option '--print-cache-dir'" not in report:
                self.fail('Unknown apparmor_parser error:\n%s' % report)

            cache_dir = os.path.join(self.tmp_dir, 'cache')
        else:
            cache_dir = report.strip()

        if create:
            os.makedirs(cache_dir)

        return cache_dir

    def assert_path_exists(self, path, expected=True):
        if expected is True:
            self.assertTrue(os.path.exists(path),
                            'test did not create file %s, when it was expected to do so' % path)
        else:
            self.assertFalse(os.path.exists(path),
                             'test created file %s, when it was not expected to do so' % path)

    def is_apparmorfs_mounted(self):
        return os.path.exists("/sys/kernel/security/apparmor")

    def require_apparmorfs(self):
        # skip the test if apparmor securityfs isn't mounted
        if not self.is_apparmorfs_mounted():
            raise unittest.SkipTest("WARNING: /sys/kernel/security/apparmor does not exist. Skipping test.")

    def compare_features_file(self, features_path, expected=True):
        # tests that need this function should call require_apparmorfs() early

        # compare features contents
        expected_output = testlib.read_features_dir('/sys/kernel/security/apparmor/features')
        with open(features_path) as f:
            features = f.read()
        if expected:
            self.assertEquals(expected_output, features,
                              "features contents differ, expected:\n%s\nresult:\n%s" % (expected_output, features))
        else:
            self.assertNotEquals(expected_output, features,
                                 "features contents equal, expected:\n%s\nresult:\n%s" % (expected_output, features))


class AAParserBasicCachingTests(AAParserCachingCommon):

    def setUp(self):
        super(AAParserBasicCachingTests, self).setUp()

    def test_no_cache_by_default(self):
        '''test profiles are not cached by default'''

        cmd = list(self.cmd_prefix)
        cmd.extend(['-q', '-r', self.profile])
        self.run_cmd_check(cmd)
        self.assert_path_exists(os.path.join(self.cache_dir, PROFILE), expected=False)

    def test_no_cache_w_skip_cache(self):
        '''test profiles are not cached with --skip-cache'''

        cmd = list(self.cmd_prefix)
        cmd.extend(['-q', '--write-cache', '--skip-cache', '-r', self.profile])
        self.run_cmd_check(cmd)
        self.assert_path_exists(os.path.join(self.cache_dir, PROFILE), expected=False)

    def test_cache_when_requested(self):
        '''test profiles are cached when requested'''

        cmd = list(self.cmd_prefix)
        cmd.extend(['-q', '--write-cache', '-r', self.profile])
        self.run_cmd_check(cmd)
        self.assert_path_exists(os.path.join(self.cache_dir, PROFILE))

    def test_write_features_when_caching(self):
        '''test features file is written when caching'''

        cmd = list(self.cmd_prefix)
        cmd.extend(['-q', '--write-cache', '-r', self.profile])
        self.run_cmd_check(cmd)
        self.assert_path_exists(os.path.join(self.cache_dir, PROFILE))
        self.assert_path_exists(os.path.join(self.cache_dir, '.features'))

    def test_features_match_when_caching(self):
        '''test features file is written when caching'''

        self.require_apparmorfs()

        cmd = list(self.cmd_prefix)
        cmd.extend(['-q', '--write-cache', '-r', self.profile])
        self.run_cmd_check(cmd)
        self.assert_path_exists(os.path.join(self.cache_dir, PROFILE))
        self.assert_path_exists(os.path.join(self.cache_dir, '.features'))

        self.compare_features_file(os.path.join(self.cache_dir, '.features'))


class AAParserAltCacheBasicTests(AAParserBasicCachingTests):
    '''Same tests as above, but with an alternate cache location specified on the command line'''

    def setUp(self):
        super(AAParserAltCacheBasicTests, self).setUp()

        alt_cache_loc = tempfile.mkdtemp(prefix='aa-alt-cache', dir=self.tmp_dir)
        os.chmod(alt_cache_loc, 0o755)

        self.unused_cache_loc = self.cache_dir
        self.cmd_prefix.extend(['--cache-loc', alt_cache_loc])
        self.cache_dir = self.get_cache_dir()

    def tearDown(self):
        if len(os.listdir(self.unused_cache_loc)) > 0:
            self.fail('original cache dir \'%s\' not empty' % self.unused_cache_loc)
        super(AAParserAltCacheBasicTests, self).tearDown()


class AAParserCreateCacheBasicTestsCacheExists(AAParserBasicCachingTests):
    '''Same tests as above, but with create cache option on the command line and the cache already exists'''

    def setUp(self):
        super(AAParserCreateCacheBasicTestsCacheExists, self).setUp()
        self.cmd_prefix.append('--create-cache-dir')


class AAParserCreateCacheBasicTestsCacheNotExist(AAParserBasicCachingTests):
    '''Same tests as above, but with create cache option on the command line and cache dir removed'''

    def setUp(self):
        super(AAParserCreateCacheBasicTestsCacheNotExist, self).setUp()
        shutil.rmtree(self.cache_dir)
        self.cmd_prefix.append('--create-cache-dir')


class AAParserCreateCacheAltCacheTestsCacheNotExist(AAParserBasicCachingTests):
    '''Same tests as above, but with create cache option on the command line,
       alt cache specified, and cache dir removed'''

    def setUp(self):
        super(AAParserCreateCacheAltCacheTestsCacheNotExist, self).setUp()
        shutil.rmtree(self.cache_dir)
        self.cmd_prefix.append('--create-cache-dir')


class AAParserCachingTests(AAParserCachingCommon):

    def setUp(self):
        super(AAParserCachingTests, self).setUp()

        r = testlib.filesystem_time_resolution()
        self.mtime_res = r[1]

    def _generate_cache_file(self):

        cmd = list(self.cmd_prefix)
        cmd.extend(['-q', '--write-cache', '-r', self.profile])
        self.run_cmd_check(cmd)
        self.assert_path_exists(self.cache_file)

    def _assertTimeStampEquals(self, time1, time2):
        '''Compare two timestamps to ensure equality'''

        # python 3.2 and earlier don't support writing timestamps with
        # nanosecond resolution, only microsecond. When comparing
        # timestamps in such an environment, loosen the equality bounds
        # to compensate
        # Reference: https://bugs.python.org/issue12904
        (major, minor, _) = platform.python_version_tuple()
        if (int(major) < 3) or ((int(major) == 3) and (int(minor) <= 2)):
            self.assertAlmostEquals(time1, time2, places=5)
        else:
            self.assertEquals(time1, time2)

    def _set_mtime(self, path, mtime):
        atime = os.stat(path).st_atime
        os.utime(path, (atime, mtime))
        self._assertTimeStampEquals(os.stat(path).st_mtime, mtime)

    def test_cache_loaded_when_exists(self):
        '''test cache is loaded when it exists, is newer than profile,  and features match'''

        self._generate_cache_file()

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Cached reload succeeded')

    def test_cache_not_loaded_when_skip_arg(self):
        '''test cache is not loaded when --skip-cache is passed'''

        self._generate_cache_file()

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '--skip-cache', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')

    def test_cache_not_loaded_when_skip_read_arg(self):
        '''test cache is not loaded when --skip-read-cache is passed'''

        self._generate_cache_file()

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '--skip-read-cache', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')

    def test_cache_not_loaded_when_features_differ(self):
        '''test cache is not loaded when features file differs'''

        self._generate_cache_file()

        testlib.write_file(self.cache_dir, '.features', 'monkey\n')

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')

    def test_cache_writing_does_not_overwrite_features_when_features_differ(self):
        '''test cache writing does not overwrite the features files when it differs and --skip-bad-cache is given'''

        self.require_apparmorfs()

        features_file = testlib.write_file(self.cache_dir, '.features', 'monkey\n')

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '--write-cache', '--skip-bad-cache', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')
        self.assert_path_exists(features_file)
        # ensure that the features does *not* match the current features set
        self.compare_features_file(features_file, expected=False)

    def test_cache_writing_skipped_when_features_differ(self):
        '''test cache writing is skipped when features file differs'''

        testlib.write_file(self.cache_dir, '.features', 'monkey\n')

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '--write-cache', '--skip-bad-cache', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')
        self.assert_path_exists(self.cache_file, expected=False)

    def test_cache_writing_collision_of_features(self):
        '''test cache writing collision of features'''
        # cache dir with different features causes a collision resulting
        # in a new cache dir
        self.require_apparmorfs()

        features_file = testlib.write_file(self.cache_dir, '.features', 'monkey\n')
        new_file = self.get_cache_dir()
        new_features_file = new_file + '/.features';

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '--write-cache', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')
        self.assert_path_exists(features_file)
        self.assert_path_exists(new_features_file)
        self.compare_features_file(new_features_file)

    def test_cache_writing_updates_cache_file(self):
        '''test cache writing updates cache file'''

        cache_file = testlib.write_file(self.cache_dir, PROFILE, 'monkey\n')
        orig_stat = os.stat(cache_file)

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '--write-cache', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')
        self.assert_path_exists(cache_file)
        stat = os.stat(cache_file)
        # We check sizes here rather than whether the string monkey is
        # in cache_contents because of the difficulty coercing cache
        # file bytes into strings in python3
        self.assertNotEquals(orig_stat.st_size, stat.st_size, 'Expected cache file to be updated, size is not changed.')
        self.assertEquals(os.stat(self.profile).st_mtime, stat.st_mtime)

    def test_cache_writing_clears_all_files(self):
        '''test cache writing clears all cache files'''

        check_file = testlib.write_file(self.cache_dir, 'monkey', 'monkey\n')

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '--write-cache', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')
        self.assert_path_exists(check_file, expected=False)

    def test_profile_mtime_preserved(self):
        '''test profile mtime is preserved when it is newest'''
        expected = 1
        self._set_mtime(self.abstraction, 0)
        self._set_mtime(self.profile, expected)
        self._generate_cache_file()
        self.assertEquals(expected, os.stat(self.cache_file).st_mtime)

    def test_abstraction_mtime_preserved(self):
        '''test abstraction mtime is preserved when it is newest'''
        expected = 1000
        self._set_mtime(self.profile, 0)
        self._set_mtime(self.abstraction, expected)
        self._generate_cache_file()
        self.assertEquals(expected, os.stat(self.cache_file).st_mtime)

    def test_equal_mtimes_preserved(self):
        '''test equal profile and abstraction mtimes are preserved'''
        expected = 10000 + self.mtime_res
        self._set_mtime(self.profile, expected)
        self._set_mtime(self.abstraction, expected)
        self._generate_cache_file()
        self.assertEquals(expected, os.stat(self.cache_file).st_mtime)

    def test_profile_newer_skips_cache(self):
        '''test cache is skipped if profile is newer'''

        self._generate_cache_file()
        profile_mtime = os.stat(self.cache_file).st_mtime + self.mtime_res
        self._set_mtime(self.profile, profile_mtime)

        orig_stat = os.stat(self.cache_file)

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')

        stat = os.stat(self.cache_file)
        self.assertEquals(orig_stat.st_size, stat.st_size)
        self.assertEquals(orig_stat.st_ino, stat.st_ino)
        self.assertEquals(orig_stat.st_mtime, stat.st_mtime)

    def test_abstraction_newer_skips_cache(self):
        '''test cache is skipped if abstraction is newer'''

        self._generate_cache_file()
        abstraction_mtime = os.stat(self.cache_file).st_mtime + self.mtime_res
        self._set_mtime(self.abstraction, abstraction_mtime)

        orig_stat = os.stat(self.cache_file)

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')

        stat = os.stat(self.cache_file)
        self.assertEquals(orig_stat.st_size, stat.st_size)
        self.assertEquals(orig_stat.st_ino, stat.st_ino)
        self.assertEquals(orig_stat.st_mtime, stat.st_mtime)

    def test_profile_newer_rewrites_cache(self):
        '''test cache is rewritten if profile is newer'''

        self._generate_cache_file()
        profile_mtime = os.stat(self.cache_file).st_mtime + self.mtime_res
        self._set_mtime(self.profile, profile_mtime)

        orig_stat = os.stat(self.cache_file)

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '-r', '-W', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')

        stat = os.stat(self.cache_file)
        self.assertNotEquals(orig_stat.st_ino, stat.st_ino)
        self._assertTimeStampEquals(profile_mtime, stat.st_mtime)

    def test_abstraction_newer_rewrites_cache(self):
        '''test cache is rewritten if abstraction is newer'''

        self._generate_cache_file()
        abstraction_mtime = os.stat(self.cache_file).st_mtime + self.mtime_res
        self._set_mtime(self.abstraction, abstraction_mtime)

        orig_stat = os.stat(self.cache_file)

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '-r', '-W', self.profile])
        self.run_cmd_check(cmd, expected_string='Replacement succeeded for')

        stat = os.stat(self.cache_file)
        self.assertNotEquals(orig_stat.st_ino, stat.st_ino)
        self._assertTimeStampEquals(abstraction_mtime, stat.st_mtime)

    def test_parser_newer_uses_cache(self):
        '''test cache is not skipped if parser is newer'''

        self._generate_cache_file()

        # copy parser
        os.mkdir(os.path.join(self.tmp_dir, 'parser'))
        new_parser = os.path.join(self.tmp_dir, 'parser', 'apparmor_parser')
        shutil.copy(config.parser, new_parser)
        self._set_mtime(new_parser, os.stat(self.cache_file).st_mtime + self.mtime_res)

        cmd = list(self.cmd_prefix)
        cmd[0] = new_parser
        cmd.extend(['-v', '-r', self.profile])
        self.run_cmd_check(cmd, expected_string='Cached reload succeeded for')

    def _purge_cache_test(self, location):

        cache_file = testlib.write_file(self.cache_dir, location, 'monkey\n')

        cmd = list(self.cmd_prefix)
        cmd.extend(['-v', '--purge-cache', '-r', self.profile])
        self.run_cmd_check(cmd)
        # no message is output
        self.assert_path_exists(cache_file, expected=False)

    def test_cache_purge_removes_features_file(self):
        '''test cache --purge-cache removes .features file'''
        self._purge_cache_test('.features')

    def test_cache_purge_removes_cache_file(self):
        '''test cache --purge-cache removes profile cache file'''
        self._purge_cache_test(PROFILE)

    def test_cache_purge_removes_other_cache_files(self):
        '''test cache --purge-cache removes other cache files'''
        self._purge_cache_test('monkey')


class AAParserAltCacheTests(AAParserCachingTests):
    '''Same tests as above, but with an alternate cache location specified on the command line'''
    check_orig_cache = True

    def setUp(self):
        super(AAParserAltCacheTests, self).setUp()

        alt_cache_loc = tempfile.mkdtemp(prefix='aa-alt-cache', dir=self.tmp_dir)
        os.chmod(alt_cache_loc, 0o755)

        self.orig_cache_dir = self.cache_dir
        self.cmd_prefix.extend(['--cache-loc', alt_cache_loc])
        self.cache_dir = self.get_cache_dir(create=True)
        self.cache_file = os.path.join(self.cache_dir, PROFILE)

    def tearDown(self):
        if self.check_orig_cache and len(os.listdir(self.orig_cache_dir)) > 0:
            self.fail('original cache dir \'%s\' not empty' % self.orig_cache_dir)
        super(AAParserAltCacheTests, self).tearDown()

    def test_cache_purge_leaves_original_cache_alone(self):
        '''test cache purging only touches alt cache'''

        # skip tearDown check to ensure non-alt cache is empty
        self.check_orig_cache = False
        filelist = [PROFILE, '.features', 'monkey']

        for f in filelist:
            testlib.write_file(self.orig_cache_dir, f, 'monkey\n')

        self._purge_cache_test(PROFILE)

        for f in filelist:
            if not os.path.exists(os.path.join(self.orig_cache_dir, f)):
                self.fail('cache purge removed %s, was not supposed to' % (os.path.join(self.orig_cache_dir, f)))


def main():
    global config
    p = ArgumentParser()
    p.add_argument('-p', '--parser', default=testlib.DEFAULT_PARSER, action="store", dest='parser')
    p.add_argument('-v', '--verbose', action="store_true", dest="verbose")
    p.add_argument('-d', '--debug', action="store_true", dest="debug")
    config = p.parse_args()

    verbosity = 1
    if config.verbose:
        verbosity = 2

    test_suite = unittest.TestSuite()
    test_suite.addTest(unittest.TestLoader().loadTestsFromTestCase(AAParserBasicCachingTests))
    test_suite.addTest(unittest.TestLoader().loadTestsFromTestCase(AAParserAltCacheBasicTests))
    test_suite.addTest(unittest.TestLoader().loadTestsFromTestCase(AAParserCreateCacheBasicTestsCacheExists))
    test_suite.addTest(unittest.TestLoader().loadTestsFromTestCase(AAParserCreateCacheBasicTestsCacheNotExist))
    test_suite.addTest(unittest.TestLoader().loadTestsFromTestCase(AAParserCreateCacheAltCacheTestsCacheNotExist))
    test_suite.addTest(unittest.TestLoader().loadTestsFromTestCase(AAParserCachingTests))
    test_suite.addTest(unittest.TestLoader().loadTestsFromTestCase(AAParserAltCacheTests))
    rc = 0
    try:
        result = unittest.TextTestRunner(verbosity=verbosity).run(test_suite)
        if not result.wasSuccessful():
            rc = 1
    except:
        rc = 1

    return rc

if __name__ == "__main__":
    rc = main()
    exit(rc)
