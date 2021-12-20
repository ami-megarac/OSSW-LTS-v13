#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
#    Copyright (C) 2015 Christian Boltz <apparmor@cboltz.de>
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import unittest
from common_test import AATest, setup_all_loops

from copy import deepcopy
import re
from apparmor.common import convert_regexp, AppArmorBug, AppArmorException
from apparmor.aare import AARE, convert_expression_to_aare

class TestConvert_regexp(AATest):
    tests = [
        ('/foo',                '^/foo$'),
        ('/{foo,bar}',          '^/(foo|bar)$'),
        # ('/\{foo,bar}',         '^/\{foo,bar}$'), # XXX gets converted to ^/\(foo|bar)$
        ('/fo[abc]',            '^/fo[abc]$'),
        ('/foo bar',            '^/foo bar$'),
        ('/x\y',                '^/x\y$'),
        ('/x\[y',               '^/x\[y$'),
        ('/x\\y',               '^/x\\y$'),
        ('/fo?',                '^/fo[^/\000]$'),
        ('/foo/*',              '^/foo/(((?<=/)[^/\000]+)|((?<!/)[^/\000]*))$'),
        ('/foo/**.bar',         '^/foo/(((?<=/)[^\000]+)|((?<!/)[^\000]*))\.bar$'),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(convert_regexp(params), expected)

class Test_convert_expression_to_aare(AATest):
    tests = [
        # note that \ always needs to be escaped in python, so \\ is actually just \ in the string
        ('/foo',        '/foo'                  ),
        ('/foo?',       '/foo\\?'               ),
        ('/foo*',       '/foo\\*'               ),
        ('/foo[bar]',   '/foo\\[bar\\]'         ),
        ('/foo{bar}',   '/foo\\{bar\\}'         ),
        ('/foo{',       '/foo\\{'               ),
        ('/foo\\',      '/foo\\\\'              ),
        ('/foo"',       '/foo\\"'               ),
        ('}]"\\[{',     '\\}\\]\\"\\\\\\[\\{'   ),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(convert_expression_to_aare(params), expected)

class TestConvert_regexpAndAAREMatch(AATest):
    tests = [
        #  aare                  path to check                         match expected?
        (['/foo/**/bar/',       '/foo/user/tools/bar/'              ], True),
        (['/foo/**/bar/',       '/foo/apparmor/bar/'                ], True),
        (['/foo/**/bar/',       '/foo/apparmor/bar'                 ], False),
        (['/foo/**/bar/',       '/a/foo/apparmor/bar/'              ], False),
        (['/foo/**/bar/',       '/foo/apparmor/bar/baz'             ], False),

        (['/foo/*/bar/',        '/foo/apparmor/bar/'                ], True),
        (['/foo/*/bar/',        '/foo/apparmor/tools/bar/'          ], False),
        (['/foo/*/bar/',        '/foo/apparmor/bar'                 ], False),

        (['/foo/user/ba?/',     '/foo/user/bar/'                    ], True),
        (['/foo/user/ba?/',     '/foo/user/bar/apparmor/'           ], False),
        (['/foo/user/ba?/',     '/foo/user/ba/'                     ], False),
        (['/foo/user/ba?/',     '/foo/user/ba//'                    ], False),

        (['/foo/user/bar/**',   '/foo/user/bar/apparmor'            ], True),
        (['/foo/user/bar/**',   '/foo/user/bar/apparmor/tools'      ], True),
        (['/foo/user/bar/**',   '/foo/user/bar/'                    ], False),

        (['/foo/user/bar/*',    '/foo/user/bar/apparmor'            ], True),
        (['/foo/user/bar/*',    '/foo/user/bar/apparmor/tools'      ], False),
        (['/foo/user/bar/*',    '/foo/user/bar/'                    ], False),
        (['/foo/user/bar/*',    '/foo/user/bar/apparmor/'           ], False),

        (['/foo/**.jpg',        '/foo/bar/baz/foobar.jpg'           ], True),
        (['/foo/**.jpg',        '/foo/bar/foobar.jpg'               ], True),
        (['/foo/**.jpg',        '/foo/bar/*.jpg'                    ], True),
        (['/foo/**.jpg',        '/foo/bar.jpg'                      ], True),
        (['/foo/**.jpg',        '/foo/**.jpg'                       ], True),
        (['/foo/**.jpg',        '/foo/*.jpg'                        ], True),
        (['/foo/**.jpg',        '/foo/barjpg'                       ], False),
        (['/foo/**.jpg',        '/foo/.*'                           ], False),
        (['/foo/**.jpg',        '/bar.jpg'                          ], False),
        (['/foo/**.jpg',        '/**.jpg'                           ], False),
        (['/foo/**.jpg',        '/*.jpg'                            ], False),
        (['/foo/**.jpg',        '/foo/*.bar'                        ], False),

        (['/foo/{**,}',         '/foo/'                             ], True),
        (['/foo/{**,}',         '/foo/bar'                          ], True),
        (['/foo/{**,}',         '/foo/bar/'                         ], True),
        (['/foo/{**,}',         '/foo/bar/baz'                      ], True),
        (['/foo/{**,}',         '/foo/bar/baz/'                     ], True),
        (['/foo/{**,}',         '/bar/'                             ], False),

        (['/foo/{,**}',         '/foo/'                             ], True),
        (['/foo/{,**}',         '/foo/bar'                          ], True),
        (['/foo/{,**}',         '/foo/bar/'                         ], True),
        (['/foo/{,**}',         '/foo/bar/baz'                      ], True),
        (['/foo/{,**}',         '/foo/bar/baz/'                     ], True),
        (['/foo/{,**}',         '/bar/'                             ], False),

        (['/foo/a[bcd]e',       '/foo/abe'                          ], True),
        (['/foo/a[bcd]e',       '/foo/abend'                        ], False),
        (['/foo/a[bcd]e',       '/foo/axe'                          ], False),

        (['/foo/a[b-d]e',       '/foo/abe'                          ], True),
        (['/foo/a[b-d]e',       '/foo/ace'                          ], True),
        (['/foo/a[b-d]e',       '/foo/abend'                        ], False),
        (['/foo/a[b-d]e',       '/foo/axe'                          ], False),

        (['/foo/a[^bcd]e',      '/foo/abe'                          ], False),
        (['/foo/a[^bcd]e',      '/foo/abend'                        ], False),
        (['/foo/a[^bcd]e',      '/foo/axe'                          ], True),

        (['/foo/{foo,bar,user,other}/bar/',                         '/foo/user/bar/'                ], True),
        (['/foo/{foo,bar,user,other}/bar/',                         '/foo/bar/bar/'                 ], True),
        (['/foo/{foo,bar,user,other}/bar/',                         '/foo/wrong/bar/'               ], False),

        (['/foo/{foo,bar,user,other}/test,ca}se/{aa,sd,nd}/bar/',   '/foo/user/test,ca}se/aa/bar/'  ], True),
        (['/foo/{foo,bar,user,other}/test,ca}se/{aa,sd,nd}/bar/',   '/foo/bar/test,ca}se/sd/bar/'   ], True),
        (['/foo/{foo,bar,user,other}/test,ca}se/{aa,sd,nd}/bar/',   '/foo/wrong/user/bar/'          ], False),
        (['/foo/{foo,bar,user,other}/test,ca}se/{aa,sd,nd}/bar/',   '/foo/user/wrong/bar/'          ], False),
        (['/foo/{foo,bar,user,other}/test,ca}se/{aa,sd,nd}/bar/',   '/foo/wrong/aa/bar/'            ], False),
    ]

    def _run_test(self, params, expected):
        regex, path = params
        parsed_regex = re.compile(convert_regexp(regex))
        self.assertEqual(bool(parsed_regex.search(path)), expected, 'Incorrectly Parsed regex: %s' %regex)

        aare_obj = AARE(regex, True)
        self.assertEqual(aare_obj.match(path), expected, 'Incorrectly parsed AARE object: %s' % regex)
        if not ('*' in path or '{' in path or '}' in path or '?' in path):
            self.assertEqual(aare_obj.match(AARE(path, False)), expected, 'Incorrectly parsed AARE object: AARE(%s)' % regex)


    def test_multi_usage(self):
        aare_obj = AARE('/foo/*', True)
        self.assertTrue(aare_obj.match('/foo/bar'))
        self.assertFalse(aare_obj.match('/foo/bar/'))
        self.assertTrue(aare_obj.match('/foo/asdf'))

    def test_match_against_AARE_1(self):
        aare_obj_1 = AARE('@{foo}/[a-d]**', True)
        aare_obj_2 = AARE('@{foo}/[a-d]**', True)
        self.assertTrue(aare_obj_1.match(aare_obj_2))
        self.assertTrue(aare_obj_1.is_equal(aare_obj_2))

    def test_match_against_AARE_2(self):
        aare_obj_1 = AARE('@{foo}/[a-d]**', True)
        aare_obj_2 = AARE('@{foo}/*[a-d]*', True)
        self.assertFalse(aare_obj_1.match(aare_obj_2))
        self.assertFalse(aare_obj_1.is_equal(aare_obj_2))

    def test_match_invalid_1(self):
        aare_obj = AARE('@{foo}/[a-d]**', True)
        with self.assertRaises(AppArmorBug):
            aare_obj.match(set())

class TestAAREMatchFromLog(AATest):
    tests = [
        #  AARE                 log event                  match expected?
        (['/foo/bar',           '/foo/bar'              ], True),
        (['/foo/*',             '/foo/bar'              ], True),
        (['/**',                '/foo/bar'              ], True),
        (['/foo/*',             '/bar/foo'              ], False),
        (['/foo/*',             '/foo/"*'               ], True),
        (['/foo/bar',           '/foo/*'                ], False),
        (['/foo/?',             '/foo/('                ], True),
        (['/foo/{bar,baz}',     '/foo/bar'              ], True),
        (['/foo/{bar,baz}',     '/foo/bars'             ], False),
    ]

    def _run_test(self, params, expected):
        regex, log_event = params
        aare_obj_1 = AARE(regex, True)
        aare_obj_2 = AARE(log_event, True, log_event=True)
        self.assertEqual(aare_obj_1.match(aare_obj_2), expected)

class TestAAREIsEqual(AATest):
    tests = [
        # regex         is path?    check for       expected
        (['/foo',       True,       '/foo'      ],  True ),
        (['@{foo}',     True,       '@{foo}'    ],  True ),
        (['/**',        True,       '/foo'      ],  False),
    ]

    def _run_test(self, params, expected):
        regex, is_path, check_for = params
        aare_obj_1 = AARE(regex, is_path)
        aare_obj_2 = AARE(check_for, is_path)
        self.assertEqual(expected, aare_obj_1.is_equal(check_for))
        self.assertEqual(expected, aare_obj_1.is_equal(aare_obj_2))

    def test_is_equal_invalid_1(self):
        aare_obj = AARE('/foo/**', True)
        with self.assertRaises(AppArmorBug):
            aare_obj.is_equal(42)

class TestAAREIsPath(AATest):
    tests = [
        # regex         is path?    match for       expected
        (['/foo*',      True,       '/foobar'   ],  True ),
        (['@{PROC}/',   True,       '/foobar'   ],  False),
        (['foo*',       False,      'foobar'    ],  True ),
    ]

    def _run_test(self, params, expected):
        regex, is_path, check_for = params
        aare_obj = AARE(regex, is_path)
        self.assertEqual(expected, aare_obj.match(check_for))

    def test_path_missing_slash(self):
        with self.assertRaises(AppArmorException):
            AARE('foo*', True)

class TestAARERepr(AATest):
    def test_repr(self):
        obj = AARE('/foo', True)
        self.assertEqual(str(obj), "AARE('/foo')")

class TestAAREDeepcopy(AATest):
    tests = [
        # regex         is path?    log event     expected (dummy value)
        (AARE('/foo',   False)                  , True),
        (AARE('/foo',   False,      True)       , True),
        (AARE('/foo',   True)                   , True),
        (AARE('/foo',   True,       True)       , True),
    ]

    def _run_test(self, params, expected):
        dup = deepcopy(params)

        self.assertTrue(params.match('/foo'))
        self.assertTrue(dup.match('/foo'))

        self.assertEqual(params.regex, dup.regex)
        self.assertEqual(params.orig_regex, dup.orig_regex)
        self.assertEqual(params.orig_regex, dup.orig_regex)

class TestAAREglobPath(AATest):
    tests = [
        # _run_test() will also run each test with '/' appended
        # regex                     expected AARE.regex
        ('/foo/bar/baz**',          '/foo/bar/**'),
        ('/foo/bar/**baz',          '/foo/bar/**'),
        ('/foo/bar/fo**baz',        '/foo/bar/**'),
        ('/foo/bar/**foo**',        '/foo/bar/**'),
        ('/foo/bar/**f?o**',        '/foo/bar/**'),
        ('/foo/bar/**fo[a-z]**',    '/foo/bar/**'),

        ('/foo/bar/baz',            '/foo/bar/*'),
        ('/foo/bar/baz*',           '/foo/bar/*'),
        ('/foo/bar/*baz',           '/foo/bar/*'),
        ('/foo/bar/fo*baz',         '/foo/bar/*'),
        ('/foo/bar/*foo*',          '/foo/bar/*'),

        ('/foo/bar/b[a-z]z',        '/foo/bar/*'),
        ('/foo/bar/{bar,baz}',      '/foo/bar/*'),
        ('/foo/bar/{bar,ba/z}',     '/foo/bar/{bar,ba/*'),  # XXX
        ('/foo/*/baz',              '/foo/*/*'),

        ('/foo/bar/**',             '/foo/**'),
        ('/foo/bar/*',              '/foo/**'),

        ('/foo/**/*',               '/foo/**'),
        ('/foo/*/**',               '/foo/**'),
        ('/foo/*/*',                '/foo/**'),

        ('/foo',                    '/*',),
        ('/b*',                     '/*',),
        ('/*b',                     '/*',),
        ('/*',                      '/*',),
        ('/*.foo',                  '/*',),
        ('/**.foo',                 '/**',),
        ('/foo/*',                  '/**',),
        ('/usr/foo/*',              '/usr/**',),
        ('/usr/foo/**',             '/usr/**',),
        ('/usr/foo/bar**',          '/usr/foo/**',),
        ('/usr/foo/**bar',          '/usr/foo/**',),
        ('/usr/bin/foo**bar',       '/usr/bin/**',),
        ('/usr/foo/**/bar',         '/usr/foo/**/*',),
        ('/usr/foo/**/*',           '/usr/foo/**',),
        ('/usr/foo/*/bar',          '/usr/foo/*/*',),
        ('/usr/bin/foo*bar',        '/usr/bin/*',),
        ('/usr/bin/*foo*',          '/usr/bin/*',),
        ('/usr/foo/*/*',            '/usr/foo/**',),
        ('/usr/foo/*/**',           '/usr/foo/**',),
        ('/**',                     '/**',),

    ]

    def _run_test(self, params, expected):
        # test for files
        oldpath = AARE(params, True)
        newpath = oldpath.glob_path()
        self.assertEqual(expected, newpath.regex)

        # test for directories
        oldpath = AARE(params + '/', True)
        newpath = oldpath.glob_path()
        self.assertEqual(expected + '/', newpath.regex)

class TestAAREglobPathWithExt(AATest):
    tests = [
        # _run_test() will also run each test with '/' appended
        # regex                     expected AARE.regex

        # no extension - shouldn't change
        ('/foo/bar/baz**',          '/foo/bar/baz**'),
        ('/foo/bar/**baz',          '/foo/bar/**baz'),
        ('/foo/bar/fo**baz',        '/foo/bar/fo**baz'),
        ('/foo/bar/**foo**',        '/foo/bar/**foo**'),
        ('/foo/bar/**f?o**',        '/foo/bar/**f?o**'),
        ('/foo/bar/**fo[a-z]**',    '/foo/bar/**fo[a-z]**'),

        ('/foo/bar/baz',            '/foo/bar/baz'),
        ('/foo/bar/baz*',           '/foo/bar/baz*'),
        ('/foo/bar/*baz',           '/foo/bar/*baz'),
        ('/foo/bar/fo*baz',         '/foo/bar/fo*baz'),
        ('/foo/bar/*foo*',          '/foo/bar/*foo*'),

        ('/foo/bar/b[a-z]z',        '/foo/bar/b[a-z]z'),
        ('/foo/bar/{bar,baz}',      '/foo/bar/{bar,baz}'),
        ('/foo/bar/{bar,ba/z}',     '/foo/bar/{bar,ba/z}'),
        ('/foo/*/baz',              '/foo/*/baz'),

        ('/foo/bar/**',             '/foo/bar/**'),
        ('/foo/bar/*',              '/foo/bar/*'),

        ('/foo/**/*',               '/foo/**/*'),
        ('/foo/*/**',               '/foo/*/**'),
        ('/foo/*/*',                '/foo/*/*'),

        ('/*',                      '/*'),
        ('/**',                     '/**'),

        ('/foo/bar',                '/foo/bar'),
        ('/foo/**/bar',             '/foo/**/bar'),
        ('/foo.bar',                '/*.bar'),
        ('/*.foo',                  '/*.foo' ),
        ('/usr/*.bar',              '/**.bar'),
        ('/usr/**.bar',             '/**.bar'),
        ('/usr/foo**.bar',          '/usr/**.bar'),
        ('/usr/foo*.bar',           '/usr/*.bar'),
        ('/usr/fo*oo.bar',          '/usr/*.bar'),
        ('/usr/*foo*.bar',          '/usr/*.bar'),
        ('/usr/**foo.bar',          '/usr/**.bar'),
        ('/usr/*foo.bar',           '/usr/*.bar'),
        ('/usr/foo.b*',             '/usr/*.b*'),

        # with extension added
        ('/foo/bar/baz**.xy',          '/foo/bar/**.xy'),
        ('/foo/bar/**baz.xy',          '/foo/bar/**.xy'),
        ('/foo/bar/fo**baz.xy',        '/foo/bar/**.xy'),
        ('/foo/bar/**foo**.xy',        '/foo/bar/**.xy'),
        ('/foo/bar/**f?o**.xy',        '/foo/bar/**.xy'),
        ('/foo/bar/**fo[a-z]**.xy',    '/foo/bar/**.xy'),

        ('/foo/bar/baz.xy',            '/foo/bar/*.xy'),
        ('/foo/bar/baz*.xy',           '/foo/bar/*.xy'),
        ('/foo/bar/*baz.xy',           '/foo/bar/*.xy'),
        ('/foo/bar/fo*baz.xy',         '/foo/bar/*.xy'),
        ('/foo/bar/*foo*.xy',          '/foo/bar/*.xy'),

        ('/foo/bar/b[a-z]z.xy',        '/foo/bar/*.xy'),
        ('/foo/bar/{bar,baz}.xy',      '/foo/bar/*.xy'),
        ('/foo/bar/{bar,ba/z}.xy',     '/foo/bar/{bar,ba/*.xy'),  # XXX
        ('/foo/*/baz.xy',              '/foo/*/*.xy'),

        ('/foo/bar/**.xy',             '/foo/**.xy'),
        ('/foo/bar/*.xy',              '/foo/**.xy'),

        ('/foo/**/*.xy',               '/foo/**.xy'),
        ('/foo/*/**.xy',               '/foo/**.xy'),
        ('/foo/*/*.xy',                '/foo/**.xy'),

        ('/*.foo',                      '/*.foo'),
        ('/**.foo',                     '/**.foo'),

        ('/foo.baz',                    '/*.baz'),
        ('/b*.baz',                     '/*.baz'),
        ('/*b.baz',                     '/*.baz'),
        ('/foo/*.baz',                  '/**.baz'),
        ('/usr/foo/*.baz',              '/usr/**.baz'),
        ('/usr/foo/**.baz',             '/usr/**.baz'),
        ('/usr/foo/bar**.baz',          '/usr/foo/**.baz'),
        ('/usr/foo/**bar.baz',          '/usr/foo/**.baz'),
        ('/usr/bin/foo**bar.baz',       '/usr/bin/**.baz'),
        ('/usr/foo/**/bar.baz',         '/usr/foo/**/*.baz'),
        ('/usr/foo/**/*.baz',           '/usr/foo/**.baz'),
        ('/usr/foo/*/bar.baz',          '/usr/foo/*/*.baz'),
        ('/usr/bin/foo*bar.baz',        '/usr/bin/*.baz'),
        ('/usr/bin/*foo*.baz',          '/usr/bin/*.baz'),
        ('/usr/foo/*/*.baz',            '/usr/foo/**.baz'),
        ('/usr/foo/*/**.baz',           '/usr/foo/**.baz'),


    ]

    def _run_test(self, params, expected):
        # test for files
        oldpath = AARE(params, True)
        newpath = oldpath.glob_path_withext()
        self.assertEqual(expected, newpath.regex)

        # test for directories - should be kept unchanged
        oldpath = AARE(params + '/', True)
        newpath = oldpath.glob_path_withext()
        self.assertEqual(params + '/', newpath.regex)  # note that we compare to params, not expected here


setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
