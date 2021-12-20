#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2011-2012 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import os
import signal
import subprocess
import tempfile
import unittest

# The locationg of the aa-decode utility can be overridden by setting
# the APPARMOR_DECODE environment variable; this is useful for running
# these tests in an installed environment
aadecode_bin = "../aa-decode"

# http://www.chiark.greenend.org.uk/ucgi/~cjwatson/blosxom/2009-07-02-python-sigpipe.html
# This is needed so that the subprocesses that produce endless output
# actually quit when the reader goes away.
def subprocess_setup():
    # Python installs a SIGPIPE handler by default. This is usually not what
    # non-Python subprocesses expect.
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

def cmd(command, input = None, stderr = subprocess.STDOUT, stdout = subprocess.PIPE, stdin = None, timeout = None):
    '''Try to execute given command (array) and return its stdout, or return
    a textual error if it failed.'''

    try:
        sp = subprocess.Popen(command, stdin=stdin, stdout=stdout, stderr=stderr, close_fds=True, preexec_fn=subprocess_setup)
    except OSError as e:
        return [127, str(e)]

    out, outerr = sp.communicate(input)
    # Handle redirection of stdout
    if out == None:
        out = b''
    # Handle redirection of stderr
    if outerr == None:
        outerr = b''
    return [sp.returncode, out.decode('utf-8') + outerr.decode('utf-8')]


def mkstemp_fill(contents, suffix='', prefix='tst-aadecode-', dir=None):
    '''As tempfile.mkstemp does, return a (file, name) pair, but with prefilled contents.'''

    handle, name = tempfile.mkstemp(suffix=suffix, prefix=prefix, dir=dir)
    os.close(handle)
    handle = open(name, "w+")
    handle.write(contents)
    handle.flush()
    handle.seek(0)

    return handle, name

class AADecodeTest(unittest.TestCase):

    def setUp(self):
        self.tmpfile = None

    def tearDown(self):
        if self.tmpfile and os.path.exists(self.tmpfile):
            os.remove(self.tmpfile)

    def test_help(self):
        '''Test --help argument'''

        expected = 0
        rc, report = cmd([aadecode_bin, "--help"])
        result = 'Got exit code %d, expected %d\n' % (rc, expected)
        self.assertEqual(expected, rc, result + report)

    def _run_file_test(self, content, expected_list):
        '''test case helper function; takes log content and a list of
           expected strings as arguments'''

        expected = 0

        (f, self.tmpfile) = mkstemp_fill(content)

        rc, report = cmd([aadecode_bin], stdin=f)
        result = 'Got exit code %d, expected %d\n' % (rc, expected)
        self.assertEqual(expected, rc, result + report)
        for expected_string in expected_list:
            result = 'could not find expected %s in output:\n' % (expected_string)
            self.assertIn(expected_string, report, result + report)
        f.close()

    def test_simple_decode(self):
        '''Test simple decode on command line'''

        expected = 0
        expected_output = 'Decoded: /tmp/foo bar'
        test_code = '2F746D702F666F6F20626172'

        rc, report = cmd([aadecode_bin, test_code])
        result = 'Got exit code %d, expected %d\n' % (rc, expected)
        self.assertEqual(expected, rc, result + report)
        result = 'Got output "%s", expected "%s"\n' % (report, expected_output)
        self.assertIn(expected_output, report, result + report)

    def test_simple_filter(self):
        '''test simple decoding of the name argument'''

        expected_string = 'name="/tmp/foo bar"'
        content = \
'''type=AVC msg=audit(1348982151.183:2934): apparmor="DENIED" operation="open" parent=30751 profile="/usr/lib/firefox/firefox{,*[^s] [^h]}" name=2F746D702F666F6F20626172 pid=30833 comm="plugin-containe" requested_mask="r" denied_mask="r" fsuid=1000 ouid=0
'''

        self._run_file_test(content, [expected_string])

    def test_simple_multiline(self):
        '''test simple multiline decoding of the name argument'''

        expected_strings = ['ses=4294967295 new ses=2762',
                            'name="/tmp/foo bar"',
                            'name="/home/steve/tmp/my test file"']
        content = \
''' type=LOGIN msg=audit(1348980001.155:2925): login pid=17875 uid=0 old auid=4294967295 new auid=0 old ses=4294967295 new ses=2762
type=AVC msg=audit(1348982151.183:2934): apparmor="DENIED" operation="open" parent=30751 profile="/usr/lib/firefox/firefox{,*[^s] [^h]}" name=2F746D702F666F6F20626172 pid=30833 comm="plugin-containe" requested_mask="r" denied_mask="r" fsuid=1000 ouid=0
type=AVC msg=audit(1348982148.195:2933): apparmor="DENIED" operation="file_lock" parent=5490 profile="/usr/lib/firefox/firefox{,*[^s][^h]}" name=2F686F6D652F73746576652F746D702F6D7920746573742066696C65 pid=30737 comm="firefox" requested_mask="k" denied_mask="k" fsuid=1000 ouid=1000
'''

        self._run_file_test(content, expected_strings)

    def test_simple_profile(self):
        '''test simple decoding of the profile argument'''

        '''Example take from LP: #897957'''
        expected_strings = ['name="/lib/x86_64-linux-gnu/libdl-2.13.so"',
                            'profile="/test space"']
        content = \
'''[289763.843292] type=1400 audit(1322614912.304:857): apparmor="ALLOWED" operation="getattr" parent=16001 profile=2F74657374207370616365 name="/lib/x86_64-linux-gnu/libdl-2.13.so" pid=17011 comm="bash" requested_mask="r" denied_mask="r" fsuid=0 ouid=0
'''

        self._run_file_test(content, expected_strings)

    def test_simple_profile2(self):
        '''test simple decoding of name and profile argument'''

        '''Example take from LP: #897957'''
        expected_strings = ['name="/home/steve/tmp/my test file"',
                            'profile="/home/steve/tmp/my prog.sh"']
        content = \
'''type=AVC msg=audit(1349805073.402:6857): apparmor="DENIED" operation="mknod" parent=5890 profile=2F686F6D652F73746576652F746D702F6D792070726F672E7368 name=2F686F6D652F73746576652F746D702F6D7920746573742066696C65 pid=5891 comm="touch" requested_mask="c" denied_mask="c" fsuid=1000 ouid=1000
'''

        self._run_file_test(content, expected_strings)

    def test_simple_embedded_carat(self):
        '''test simple decoding of embedded ^ in files'''

        expected_strings = ['name="/home/steve/tmp/my test ^file"']
        content = \
'''type=AVC msg=audit(1349805073.402:6857): apparmor="DENIED" operation="mknod" parent=5890 profile="/usr/bin/test_profile" name=2F686F6D652F73746576652F746D702F6D792074657374205E66696C65 pid=5891 comm="touch" requested_mask="c" denied_mask="c" fsuid=1000 ouid=1000
'''

        self._run_file_test(content, expected_strings)

    def test_simple_embedded_backslash_carat(self):
        '''test simple decoding of embedded \^ in files'''

        expected_strings = ['name="/home/steve/tmp/my test \^file"']
        content = \
'''type=AVC msg=audit(1349805073.402:6857): apparmor="DENIED" operation="mknod" parent=5890 profile="/usr/bin/test_profile" name=2F686F6D652F73746576652F746D702F6D792074657374205C5E66696C65 pid=5891 comm="touch" requested_mask="c" denied_mask="c" fsuid=1000 ouid=1000
'''

        self._run_file_test(content, expected_strings)

    def test_simple_embedded_singlequote(self):
        '''test simple decoding of embedded \' in files'''

        expected_strings = ['name="/home/steve/tmp/my test \'file"']
        content = \
'''type=AVC msg=audit(1349805073.402:6857): apparmor="DENIED" operation="mknod" parent=5890 profile="/usr/bin/test_profile" name=2F686F6D652F73746576652F746D702F6D792074657374202766696C65 pid=5891 comm="touch" requested_mask="c" denied_mask="c" fsuid=1000 ouid=1000
'''

        self._run_file_test(content, expected_strings)

    def test_simple_encoded_nonpath_profiles(self):
        '''test simple decoding of nonpath profiles'''

        expected_strings = ['name="/lib/x86_64-linux-gnu/libdl-2.13.so"',
                            'profile="test space"']
        content = \
'''[289763.843292] type=1400 audit(1322614912.304:857): apparmor="ALLOWED" operation="getattr" parent=16001 profile=74657374207370616365 name="/lib/x86_64-linux-gnu/libdl-2.13.so" pid=17011 comm="bash" requested_mask="r" denied_mask="r" fsuid=0 ouid=0
'''

        self._run_file_test(content, expected_strings)

#
# Main
#
if __name__ == '__main__':
    if 'APPARMOR_DECODE' in os.environ:
        aadecode_bin = os.environ['APPARMOR_DECODE']
    unittest.main(verbosity=1)
