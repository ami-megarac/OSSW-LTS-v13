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

import os
import shutil
import signal
import subprocess
import tempfile
import time
import unittest

TIMEOUT_ERROR_CODE = 152
DEFAULT_PARSER = '../apparmor_parser'


# http://www.chiark.greenend.org.uk/ucgi/~cjwatson/blosxom/2009-07-02-python-sigpipe.html
# This is needed so that the subprocesses that produce endless output
# actually quit when the reader goes away.
def subprocess_setup():
    # Python installs a SIGPIPE handler by default. This is usually not
    # what non-Python subprocesses expect.
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)


class AANoCleanupMetaClass(type):
    def __new__(cls, name, bases, attrs):

        for attr_name, attr_value in attrs.items():
            if attr_name.startswith("test_"):
                attrs[attr_name] = cls.keep_on_fail(attr_value)
        return super(AANoCleanupMetaClass, cls).__new__(cls, name, bases, attrs)

    @classmethod
    def keep_on_fail(cls, unittest_func):
        '''wrapping function for unittest testcases to detect failure
           and leave behind test files in tearDown(); to be used as
           a decorator'''

        def new_unittest_func(self):
            try:
                return unittest_func(self)
            except unittest.SkipTest:
                raise
            except Exception:
                self.do_cleanup = False
                raise

        return new_unittest_func


class AATestTemplate(unittest.TestCase, metaclass=AANoCleanupMetaClass):
    '''Stub class for use by test scripts'''
    debug = False
    do_cleanup = True

    def run_cmd_check(self, command, input=None, stderr=subprocess.STDOUT, stdout=subprocess.PIPE,
                      stdin=None, timeout=120, expected_rc=0, expected_string=None):
        '''Wrapper around run_cmd that checks the rc code against
           expected_rc and for expected strings in the output if
           passed. The valgrind tests generally don't care what the
           rc is as long as it's not a specific set of return codes,
           so can't push the check directly into run_cmd().'''
        rc, report = self.run_cmd(command, input, stderr, stdout, stdin, timeout)
        self.assertEqual(rc, expected_rc, "Got return code %d, expected %d\nCommand run: %s\nOutput: %s" % (rc, expected_rc, (' '.join(command)), report))
        if expected_string:
            self.assertIn(expected_string, report, 'Expected message "%s", got: \n%s' % (expected_string, report))
        return report

    def run_cmd(self, command, input=None, stderr=subprocess.STDOUT, stdout=subprocess.PIPE,
                stdin=None, timeout=120):
        '''Try to execute given command (array) and return its stdout, or
           return a textual error if it failed.'''

        if self.debug:
            print('\n===> Running command: \'%s\'' % (' '.join(command)))

        try:
            sp = subprocess.Popen(command, stdin=stdin, stdout=stdout, stderr=stderr,
                                  close_fds=True, preexec_fn=subprocess_setup)
        except OSError as e:
            return [127, str(e)]

        timeout_communicate = TimeoutFunction(sp.communicate, timeout)
        out, outerr = (None, None)
        try:
            out, outerr = timeout_communicate(input)
            rc = sp.returncode
        except TimeoutFunctionException as e:
            sp.terminate()
            outerr = b'test timed out, killed'
            rc = TIMEOUT_ERROR_CODE

        # Handle redirection of stdout
        if out is None:
            out = b''
        # Handle redirection of stderr
        if outerr is None:
            outerr = b''

        report = out.decode('utf-8') + outerr.decode('utf-8')

        return [rc, report]


# Timeout handler using alarm() from John P. Speno's Pythonic Avocado
class TimeoutFunctionException(Exception):
    """Exception to raise on a timeout"""
    pass


class TimeoutFunction:
    def __init__(self, function, timeout):
        self.timeout = timeout
        self.function = function

    def handle_timeout(self, signum, frame):
        raise TimeoutFunctionException()

    def __call__(self, *args, **kwargs):
        old = signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.timeout)
        try:
            result = self.function(*args, **kwargs)
        finally:
            signal.signal(signal.SIGALRM, old)
        signal.alarm(0)
        return result


def filesystem_time_resolution():
    '''detect whether the filesystem stores subsecond timestamps'''

    default_diff = 0.1
    result = (True, default_diff)

    tmp_dir = tempfile.mkdtemp(prefix='aa-caching-nanostamp-')
    try:
        last_stamp = None
        for i in range(10):
            s = None

            with open(os.path.join(tmp_dir, 'test.%d' % i), 'w+') as f:
                s = os.fstat(f.fileno())

            if (s.st_mtime == last_stamp):
                print('\n===> WARNING: TMPDIR lacks subsecond timestamp resolution, falling back to slower test')
                result = (False, 1.0)
                break

            last_stamp = s.st_mtime
            time.sleep(default_diff)
    except:
        pass
    finally:
        if os.path.exists(tmp_dir):
            shutil.rmtree(tmp_dir)

    return result


def read_features_dir(path):

    result = ''
    if not os.path.exists(path) or not os.path.isdir(path):
        return result

    for name in os.listdir(path):
        entry = os.path.join(path, name)
        result += '%s {' % name
        if os.path.isfile(entry):
            with open(entry, 'r') as f:
                # don't need extra '\n' here as features file contains it
                result += '%s' % (f.read())
        elif os.path.isdir(entry):
            result += '%s' % (read_features_dir(entry))
        result += '}\n'

    return result


def touch(path):
    return os.utime(path, None)


def write_file(directory, file, contents):
    '''construct path, write contents to it, and return the constructed path'''
    path = os.path.join(directory, file)
    with open(path, 'w+') as f:
        f.write(contents)
    return path


