#!/usr/bin/env python3
# ------------------------------------------------------------------
#
#   Copyright (C) 2013 Canonical Ltd.
#   Author: Steve Beattie <steve@nxnw.org>
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of version 2 of the GNU General Public
#   License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

# TODO
# - finish adding suppressions for valgrind false positives

from argparse import ArgumentParser  # requires python 2.7 or newer
import os
import sys
import tempfile
import unittest
import testlib

DEFAULT_TESTDIR = "./simple_tests/vars"
VALGRIND_ERROR_CODE = 151
VALGRIND_ARGS = ['--leak-check=full', '--error-exitcode=%d' % (VALGRIND_ERROR_CODE)]

VALGRIND_SUPPRESSIONS = '''
{
    valgrind-serialize_profile-obsessive-overreads
    Memcheck:Addr4
    fun:_Z*sd_serialize_profile*
    ...
    fun:_Z*__sd_serialize_profile*
    fun:_Z*load_profile*
    fun:_Z*load_policy_list*
}'''


class AAParserValgrindTests(testlib.AATestTemplate):
    def setUp(self):
        # REPORT ALL THE OUTPUT
        self.maxDiff = None

    def _runtest(self, testname, config):
        parser_args = ['-Q', '-I', config.testdir, '-M', './features_files/features.all']
        failure_rc = [VALGRIND_ERROR_CODE, testlib.TIMEOUT_ERROR_CODE]
        command = [config.valgrind]
        command.extend(VALGRIND_ARGS)
        command.append(config.parser)
        command.extend(parser_args)
        command.append(testname)
        rc, output = self.run_cmd(command, timeout=120)
        self.assertNotIn(rc, failure_rc,
                    "valgrind returned error code %d, gave the following output\n%s\ncommand run: %s" % (rc, output, " ".join(command)))


def find_testcases(testdir):
    '''dig testcases out of passed directory'''

    for (fdir, direntries, files) in os.walk(testdir):
        for f in files:
            if f.endswith(".sd"):
                yield os.path.join(fdir, f)


def create_suppressions():
    '''generate valgrind suppressions file'''

    handle, name = tempfile.mkstemp(suffix='.suppressions', prefix='aa-parser-valgrind')
    os.close(handle)
    handle = open(name,"w+")
    handle.write(VALGRIND_SUPPRESSIONS)
    handle.close()
    return name

def main():
    rc = 0
    p = ArgumentParser()
    p.add_argument('-p', '--parser', default=testlib.DEFAULT_PARSER, action="store", dest='parser',
                   help="Specify path of apparmor parser to use [default = %(default)s]")
    p.add_argument('-v', '--verbose', action="store_true", dest="verbose")
    p.add_argument('-V', '--valgrind', default='/usr/bin/valgrind', action="store", dest="valgrind",
                   help="Specify path of valgrind to use [default = %(default)s]")
    p.add_argument('-s', '--skip-suppressions', action="store_true", dest="skip_suppressions",
                   help="Don't use valgrind suppressions to skip false positives")
    p.add_argument('--dump-suppressions', action="store_true", dest="dump_suppressions",
                   help="Dump valgrind suppressions to stdout")
    p.add_argument('testdir', action="store", default=DEFAULT_TESTDIR, nargs='?', metavar='dir',
                   help="run tests on alternate directory [default = %(default)s]")
    config = p.parse_args()

    if config.dump_suppressions:
        print(VALGRIND_SUPPRESSIONS)
        return rc

    if not os.path.exists(config.valgrind):
        print("Unable to find valgrind at '%s', ensure that it is installed" % (config.valgrind),
              file=sys.stderr)
        exit(1)

    verbosity = 1
    if config.verbose:
        verbosity = 2

    if not config.skip_suppressions:
        suppression_file = create_suppressions()
        VALGRIND_ARGS.append('--suppressions=%s' % (suppression_file))

    for f in find_testcases(config.testdir):
        def stub_test(self, testname=f):
            self._runtest(testname, config)
        stub_test.__doc__ = "test %s" % (f)
        setattr(AAParserValgrindTests, 'test_%s' % (f), stub_test)
    test_suite = unittest.TestSuite()
    test_suite.addTest(unittest.TestLoader().loadTestsFromTestCase(AAParserValgrindTests))

    try:
        result = unittest.TextTestRunner(verbosity=verbosity).run(test_suite)
        if not result.wasSuccessful():
            rc = 1
    except:
        rc = 1
    finally:
        os.remove(suppression_file)

    return rc

if __name__ == "__main__":
    rc = main()
    exit(rc)
