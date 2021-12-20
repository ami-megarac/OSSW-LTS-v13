#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2014 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import apparmor.aa as aa
import unittest
from common_test import AATest, setup_all_loops, setup_aa
from apparmor.common import AppArmorBug, AppArmorException

from apparmor.regex import ( strip_parenthesis, strip_quotes, parse_profile_start_line, re_match_include,
     RE_PROFILE_START, RE_PROFILE_DBUS, RE_PROFILE_CAP, RE_PROFILE_PTRACE, RE_PROFILE_SIGNAL )


class AARegexTest(AATest):
    def _run_test(self, params, expected):
        return _regex_test(self, params, expected)

class AANamedRegexTest(AATest):
    def _run_test(self, line, expected):
        '''Run a line through self.regex.search() and verify the result

        Keyword arguments:
        line -- the line to search
        expected -- False if the search isn't expected to match or, if the search
                    is expected to match, a tuple of expected match groups.
        '''
        matches = self.regex.search(line)
        if not expected:
            self.assertFalse(matches)
            return

        self.assertTrue(matches)

        for exp in expected:
            match = matches.group(exp)
            if match:
                match = match
            self.assertEqual(match, expected[exp], 'Group %s mismatch in rule %s' % (exp,line))



class AARegexHasComma(AATest):
    '''Tests for apparmor.aa.RE_RULE_HAS_COMMA'''

    def _check(self, line, expected=True):
        result = aa.RE_RULE_HAS_COMMA.search(line)
        if expected:
            self.assertTrue(result, 'Couldn\'t find a comma in "%s"' % line)
        else:
            self.assertEqual(None, result, 'Found an unexpected comma in "%s"' % line)

regex_has_comma_testcases = [
    ('dbus send%s', 'simple'),
    ('dbus (r, w, bind, eavesdrop)%s', 'embedded parens 01'),
    ('dbus (r, w,, bind, eavesdrop) %s', 'embedded parens 02'),
    ('dbus (r, w,, ) %s', 'embedded parens 03'),
    ('dbus () %s', 'empty parens'),
    ('member={Hello,AddMatch,RemoveMatch,GetNameOwner,NameHasOwner,StartServiceByName} %s ', 'embedded curly braces 01'),
    ('member={Hello,,,,,,AddMatch,,,NameHasOwner,StartServiceByName} %s ', 'embedded curly braces 02'),
    ('member={,Hello,,,,,,AddMatch,,,NameHasOwner,} %s ', 'embedded curly braces 03'),
    ('member={} %s ', 'empty curly braces'),
    ('dbus send%s# this is a comment', 'comment 01'),
    ('dbus send%s# this is a comment,', 'comment 02'),
    ('audit "/tmp/foo, bar" rw%s', 'quotes 01'),
    ('audit "/tmp/foo, bar" rw%s # comment', 'quotes 02'),
    ('audit "/tmp/foo, # bar" rw%s', 'comment embedded in quote 01'),
    ('audit "/tmp/foo, # bar" rw%s # comment', 'comment embedded in quote 02'),

    # lifted from parser/tst/simple_tests/vars/vars_alternation_3.sd
    ('/does/not/@{BAR},exist,notexist} r%s', 'partial alternation'),

    ('signal%s', 'bare signal'),
    ('signal receive%s', 'simple signal'),
    ('signal (send, receive)%s', 'embedded parens signal 01'),
    ('signal (send, receive) set=(hup, quit)%s', 'embedded parens signal 02'),

    ('ptrace%s', 'bare ptrace'),
    ('ptrace trace%s', 'simple ptrace'),
    ('ptrace (tracedby, readby)%s', 'embedded parens ptrace 01'),
    ('ptrace (trace) peer=/usr/bin/foo%s', 'embedded parens ptrace 02'),

    ('pivot_root%s', 'bare pivot_root'),
    ('pivot_root /old%s', 'pivot_root with old'),
    ('pivot_root /old new%s', 'pivot_root with new'),
    ('pivot_root /old /new -> child%s', 'pivot_root with child'),

    ('unix%s', 'bare unix'),
    ('unix create%s', 'simple unix'),
    ('peer=(addr=@abad1dea,label=a_profile) %s ', 'peer parens and comma'),
    ('type=stream%s', 'unix type'),
    ('unix (connect, receive, send)%s', 'unix perms'),

    # the following fail due to inadequacies in the regex
    # ('dbus (r, w, %s', 'incomplete dbus action'),
    # ('member="{Hello,AddMatch,RemoveMatch, %s', 'incomplete {} regex'),  # also invalid policy
    # ('member={Hello,AddMatch,RemoveMatch, %s', 'incomplete {} regex'),  # also invalid policy when trailing comma exists

    # the following passes the tests, but variable declarations are
    # odd in that they *don't* allow trailing commas; commas at the end
    # of the line need to be quoted.
    # ('@{BAR}={bar,baz,blort %s', 'tricksy variable declaration')
    # ('@{BAR}="{bar,baz,blort," %s', 'tricksy variable declaration')
    # The following fails the no comma test, but is invalid
    # ('@{BAR}={bar,baz,blort, %s', 'tricksy variable declaration')
    # The following fails the comma test, because it's really a no comma situation
    # ('@{BAR}="{bar,baz,blort%s" ', 'tricksy variable declaration')
]

def setup_has_comma_testcases():
    i = 0
    for (test_string, description) in regex_has_comma_testcases:
        i += 1
        def stub_test_comma(self, test_string=test_string):
            self._check(test_string % ',')
        def stub_test_no_comma(self, test_string=test_string):
            self._check(test_string % ' ', False)
        stub_test_comma.__doc__ = "test %s (w/comma)" % (description)
        stub_test_no_comma.__doc__ = "test %s (no comma)" % (description)
        setattr(AARegexHasComma, 'test_comma_%d' % (i), stub_test_comma)
        setattr(AARegexHasComma, 'test_no_comma_%d' % (i), stub_test_no_comma)

class AARegexSplitComment(AATest):
    '''Tests for RE_HAS_COMMENT_SPLIT'''

    def _check(self, line, expected, comment=None, not_comment=None):
        result = aa.RE_HAS_COMMENT_SPLIT.search(line)
        if expected:
            self.assertTrue(result, 'Couldn\'t find a comment in "%s"' % line)
            self.assertEqual(result.group('comment'), comment, 'Expected comment "%s", got "%s"'
                             % (comment, result.group('comment')))
            self.assertEqual(result.group('not_comment'), not_comment, 'Expected not comment "%s", got "%s"'
                             % (not_comment, result.group('not_comment')))
        else:
            self.assertEqual(None, result, 'Found an unexpected comment "%s" in "%s"'
                             % ("" if result is None else result.group('comment'), line ))

# Tuples of (string, expected result), where expected result is False if
# the string should not be considered as having a comment, or a second
# tuple of the not comment and comment sections split apart
regex_split_comment_testcases = [
    ('dbus send # this is a comment', ('dbus send ', '# this is a comment')),
    ('dbus send member=no_comment', False),
    ('dbus send member=no_comment, ', False),
    ('audit "/tmp/foo, # bar" rw', False),
    ('audit "/tmp/foo, # bar" rw # comment', ('audit "/tmp/foo, # bar" rw ', '# comment')),
    ('file,', False),
    ('file, # bare', ('file, ', '# bare')),
    ('file /tmp/foo rw, # read-write', ('file /tmp/foo rw, ', '# read-write')),
    ('signal, # comment', ('signal, ', '# comment')),
    ('signal receive set=(usr1 usr2) peer=foo,', False),
    ('ptrace, # comment', ('ptrace, ', '# comment')),
    ('ptrace (trace read) peer=/usr/bin/foo,', False),
    ('pivot_root, # comment', ('pivot_root, ', '# comment')),
    ('pivot_root /old /new -> child,', False),
]

def setup_split_comment_testcases():
    i = 0
    for (test_string, result) in regex_split_comment_testcases:
        i += 1
        def stub_test(self, test_string=test_string, result=result):
            if result is False:
                self._check(test_string, False)
            else:
                self._check(test_string, True, not_comment=result[0], comment=result[1])
        stub_test.__doc__ = "test '%s'" % (test_string)
        setattr(AARegexSplitComment, 'test_split_comment_%d' % (i), stub_test)


def _regex_test(self, line, expected):
    '''Run a line through self.regex.search() and verify the result

    Keyword arguments:
    line -- the line to search
    expected -- False if the search isn't expected to match or, if the search
                is expected to match, a tuple of expected match groups with all
                of the strings stripped
    '''
    result = self.regex.search(line)
    if not expected:
        self.assertFalse(result)
        return

    self.assertTrue(result)

    groups = result.groups()
    self.assertEqual(len(groups), len(expected))
    for (i, group) in enumerate(groups):
        if group:
            group = group.strip()
        self.assertEqual(group, expected[i], 'Group %d mismatch in rule %s' % (i,line))





class AARegexCapability(AARegexTest):
    '''Tests for RE_PROFILE_CAP'''

    def AASetup(self):
        self.regex = RE_PROFILE_CAP

    tests = [
        ('   capability net_raw,', (None, None, 'net_raw', 'net_raw', None)),
        ('capability     net_raw   ,  ', (None, None, 'net_raw', 'net_raw', None)),
        ('   capability,', (None, None, None, None, None)),
        ('   capability   ,  ', (None, None, None, None, None)),
        ('   capabilitynet_raw,', False)
    ]

class AARegexDbus(AARegexTest):
    '''Tests for RE_PROFILE_DBUS'''

    def AASetup(self):
        self.regex = RE_PROFILE_DBUS

    tests = [
        ('   dbus,',                                    (None,      None,   'dbus,',                            None,                       None)),
        ('   audit dbus,',                              ('audit',   None,   'dbus,',                            None,                       None)),
        ('   dbus send member=no_comment,',             (None,      None,   'dbus send member=no_comment,',     'send member=no_comment',   None)),
        ('   dbus send member=no_comment, # comment',   (None,      None,   'dbus send member=no_comment,',     'send member=no_comment',   '# comment')),

        ('   dbusdriver,', False),
        ('   audit dbusdriver,', False),
    ]

class AARegexMount(AARegexTest):
    '''Tests for RE_PROFILE_MOUNT'''

    def AASetup(self):
        self.regex = aa.RE_PROFILE_MOUNT

    tests = [
        ('   mount,', (None, None, 'mount,', 'mount', None, None)),
        ('   audit mount,', ('audit', None, 'mount,', 'mount', None, None)),
        ('   umount,', (None, None, 'umount,', 'umount', None, None)),
        ('   audit umount,', ('audit', None, 'umount,', 'umount', None, None)),
        ('   unmount,', (None, None, 'unmount,', 'unmount', None, None)),
        ('   audit unmount,', ('audit', None, 'unmount,', 'unmount', None, None)),
        ('   remount,', (None, None, 'remount,', 'remount', None, None)),
        ('   deny remount,', (None, 'deny', 'remount,', 'remount', None, None)),

        ('   mount, # comment', (None, None, 'mount,', 'mount', None, '# comment')),

        ('   mountain,', False),
        ('   audit mountain,', False),
    ]



class AARegexSignal(AARegexTest):
    '''Tests for RE_PROFILE_SIGNAL'''

    def AASetup(self):
        self.regex = RE_PROFILE_SIGNAL

    tests = [
        ('   signal,',                                  (None,    None, 'signal,',                                  None,                               None)),
        ('   audit signal,',                            ('audit', None, 'signal,',                                  None,                               None)),
        ('   signal receive,',                          (None,    None, 'signal receive,',                          'receive',                          None)),
        ('   signal (send, receive),',                  (None,    None, 'signal (send, receive),',                  '(send, receive)',                  None)),
        ('   audit signal (receive),',                  ('audit', None, 'signal (receive),',                        '(receive)',                        None)),
        ('   signal (send, receive) set=(usr1 usr2),',  (None,    None, 'signal (send, receive) set=(usr1 usr2),',  '(send, receive) set=(usr1 usr2)',  None)),
        ('   signal send set=(hup, quit) peer=/usr/sbin/daemon,', (None, None, 'signal send set=(hup, quit) peer=/usr/sbin/daemon,',
                                                                                                          'send set=(hup, quit) peer=/usr/sbin/daemon', None)),

        ('   signalling,', False),
        ('   audit signalling,', False),
        ('   signalling receive,', False),
    ]


class AARegexPtrace(AARegexTest):
    '''Tests for RE_PROFILE_PTRACE'''

    def AASetup(self):
        self.regex = RE_PROFILE_PTRACE

    tests = [
        #                                            audit      allow  rule                                     rule details                    comment
        ('   ptrace,',                              (None,      None, 'ptrace,',                                None,                           None)),
        ('   audit ptrace,',                        ('audit',   None, 'ptrace,',                                None,                           None)),
        ('   ptrace trace,',                        (None,      None, 'ptrace trace,',                          'trace',                        None)),
        ('   ptrace (tracedby, readby),',           (None,      None, 'ptrace (tracedby, readby),',             '(tracedby, readby)',           None)),
        ('   audit ptrace (read),',                 ('audit',   None, 'ptrace (read),',                         '(read)',                       None)),
        ('   ptrace trace peer=/usr/sbin/daemon,',  (None,      None, 'ptrace trace peer=/usr/sbin/daemon,',    'trace peer=/usr/sbin/daemon',  None)),

        ('   ptraceback,', False),
        ('   audit ptraceback,', False),
        ('   ptraceback trace,', False),
    ]


class AARegexPivotRoot(AARegexTest):
    '''Tests for RE_PROFILE_PIVOT_ROOT'''

    def AASetup(self):
        self.regex = aa.RE_PROFILE_PIVOT_ROOT

    tests = [
        ('   pivot_root,', (None, None, 'pivot_root,', None)),
        ('   audit pivot_root,', ('audit', None, 'pivot_root,', None)),
        ('   pivot_root oldroot=/new/old,',
         (None, None, 'pivot_root oldroot=/new/old,', None)),
        ('   pivot_root oldroot=/new/old /new,',
         (None, None, 'pivot_root oldroot=/new/old /new,', None)),
        ('   pivot_root oldroot=/new/old /new -> child,',
         (None, None, 'pivot_root oldroot=/new/old /new -> child,', None)),
        ('   audit pivot_root oldroot=/new/old /new -> child,',
         ('audit', None, 'pivot_root oldroot=/new/old /new -> child,', None)),

        ('pivot_root', False),  # comma missing

        ('pivot_rootbeer,', False),
        ('pivot_rootbeer    ,  ', False),
        ('pivot_rootbeer, # comment', False),
        ('pivot_rootbeer /new,  ', False),
        ('pivot_rootbeer /new, # comment', False),
    ]

class AARegexUnix(AARegexTest):
    '''Tests for RE_PROFILE_UNIX'''

    def AASetup(self):
        self.regex = aa.RE_PROFILE_UNIX

    tests = [
        ('   unix,', (None, None, 'unix,', None)),
        ('   audit unix,', ('audit', None, 'unix,', None)),
        ('   unix accept,', (None, None, 'unix accept,', None)),
        ('   allow unix connect,', (None, 'allow', 'unix connect,', None)),
        ('   audit allow unix bind,', ('audit', 'allow', 'unix bind,', None)),
        ('   deny unix bind,', (None, 'deny', 'unix bind,', None)),
        ('unix peer=(label=@{profile_name}),',
         (None, None, 'unix peer=(label=@{profile_name}),', None)),
        ('unix (receive) peer=(label=unconfined),',
         (None, None, 'unix (receive) peer=(label=unconfined),', None)),
        (' unix (getattr, shutdown) peer=(addr=none),',
         (None, None, 'unix (getattr, shutdown) peer=(addr=none),', None)),
        ('unix (connect, receive, send) type=stream peer=(label=unconfined,addr="@/tmp/dbus-*"),',
         (None, None, 'unix (connect, receive, send) type=stream peer=(label=unconfined,addr="@/tmp/dbus-*"),', None)),
        ('unixlike', False),
        ('deny unixlike,', False),
    ]

class AANamedRegexProfileStart_2(AANamedRegexTest):
    '''Tests for RE_PROFILE_START'''

    def AASetup(self):
        self.regex = RE_PROFILE_START

    tests = [
        ('/bin/foo ', False), # no '{'
        ('/bin/foo /bin/bar', False), # missing 'profile' keyword
        ('profile {', False), # no attachment
        ('   profile foo bar /foo {', False), # missing quotes around "foo bar"
        ('bin/foo {', False), # not starting with '/'
        ('"bin/foo" {', False), # not starting with '/', quoted version

        ('   /foo {',                     { 'plainprofile': '/foo',    'namedprofile': None,          'attachment': None,     'flags': None,       'comment': None }),
        ('   "/foo" {',                   { 'plainprofile': '"/foo"',  'namedprofile': None,          'attachment': None,     'flags': None,       'comment': None }),
        ('   profile /foo {',             { 'plainprofile': None,      'namedprofile': '/foo',        'attachment': None,     'flags': None,       'comment': None }),
        ('   profile "/foo" {',           { 'plainprofile': None,      'namedprofile': '"/foo"',      'attachment': None,     'flags': None,       'comment': None }),
        ('   profile foo /foo {',         { 'plainprofile': None,      'namedprofile': 'foo',         'attachment': '/foo',   'flags': None,       'comment': None }),
        ('   profile foo /foo (audit) {', { 'plainprofile': None,      'namedprofile': 'foo',         'attachment': '/foo',   'flags': 'audit',    'comment': None }),
        ('   profile "foo" "/foo" {',     { 'plainprofile': None,      'namedprofile': '"foo"',       'attachment': '"/foo"', 'flags': None,       'comment': None }),
        ('   profile "foo bar" /foo {',   { 'plainprofile': None,      'namedprofile': '"foo bar"',   'attachment': '/foo',   'flags': None,       'comment': None }),
        ('   /foo (complain) {',          { 'plainprofile': '/foo',    'namedprofile': None,          'attachment': None,     'flags': 'complain', 'comment': None }),
        ('   /foo flags=(complain) {',    { 'plainprofile': '/foo',    'namedprofile': None,          'attachment': None,     'flags': 'complain', 'comment': None }),
        ('   /foo (complain) { # x',      { 'plainprofile': '/foo',    'namedprofile': None,          'attachment': None,     'flags': 'complain', 'comment': '# x'}),
        ('   /foo flags = ( complain ){#',{ 'plainprofile': '/foo',    'namedprofile': None,          'attachment': None,     'flags': ' complain ', 'comment': '#'}),
        ('  @{foo} {',                    { 'plainprofile': '@{foo}',  'namedprofile': None,          'attachment': None,     'flags': None,       'comment': None }),
        ('  profile @{foo} {',            { 'plainprofile': None,      'namedprofile': '@{foo}',      'attachment': None,     'flags': None,       'comment': None }),
        ('  profile @{foo} /bar {',       { 'plainprofile': None,      'namedprofile': '@{foo}',      'attachment': '/bar',   'flags': None,       'comment': None }),
        ('  profile foo @{bar} {',        { 'plainprofile': None,      'namedprofile': 'foo',         'attachment': '@{bar}', 'flags': None,       'comment': None }),
        ('  profile @{foo} @{bar} {',     { 'plainprofile': None,      'namedprofile': '@{foo}',      'attachment': '@{bar}', 'flags': None,       'comment': None }),

        ('   /foo {',                     { 'plainprofile': '/foo',     'namedprofile': None,   'leadingspace': '   ' }),
        ('/foo {',                        { 'plainprofile': '/foo',     'namedprofile': None,   'leadingspace': ''    }),
        ('   profile foo {',              { 'plainprofile': None,       'namedprofile': 'foo',  'leadingspace': '   ' }),
        ('profile foo {',                 { 'plainprofile': None,       'namedprofile': 'foo',  'leadingspace': ''    }),
    ]


class Test_parse_profile_start_line(AATest):
    tests = [
        ('   /foo {',                     { 'profile': '/foo',    'profile_keyword': False, 'plainprofile': '/foo', 'namedprofile': None,   'attachment': None,   'flags': None,       'comment': None }),
        ('   "/foo" {',                   { 'profile': '/foo',    'profile_keyword': False, 'plainprofile': '/foo', 'namedprofile': None,   'attachment': None,   'flags': None,       'comment': None }),
        ('   profile /foo {',             { 'profile': '/foo',    'profile_keyword': True,  'plainprofile': None,   'namedprofile': '/foo', 'attachment': None,   'flags': None,       'comment': None }),
        ('   profile "/foo" {',           { 'profile': '/foo',    'profile_keyword': True,  'plainprofile': None,   'namedprofile': '/foo', 'attachment': None,   'flags': None,       'comment': None }),
        ('   profile foo /foo {',         { 'profile': 'foo',     'profile_keyword': True,  'plainprofile': None,   'namedprofile': 'foo',  'attachment': '/foo', 'flags': None,       'comment': None }),
        ('   profile foo /foo (audit) {', { 'profile': 'foo',     'profile_keyword': True,  'plainprofile': None,   'namedprofile': 'foo',  'attachment': '/foo', 'flags': 'audit',    'comment': None }),
        ('   profile "foo" "/foo" {',     { 'profile': 'foo',     'profile_keyword': True,  'plainprofile': None,   'namedprofile': 'foo',  'attachment': '/foo', 'flags': None,       'comment': None }),
        ('   profile "foo bar" /foo {',   { 'profile': 'foo bar', 'profile_keyword': True,  'plainprofile': None, 'namedprofile': 'foo bar','attachment': '/foo', 'flags': None,    'comment': None }),
        ('   /foo (complain) {',          { 'profile': '/foo',    'profile_keyword': False, 'plainprofile': '/foo', 'namedprofile': None,   'attachment': None,   'flags': 'complain', 'comment': None }),
        ('   /foo flags=(complain) {',    { 'profile': '/foo',    'profile_keyword': False, 'plainprofile': '/foo', 'namedprofile': None,   'attachment': None,   'flags': 'complain', 'comment': None }),
        ('   /foo flags = ( complain ){', { 'profile': '/foo',    'profile_keyword': False, 'plainprofile': '/foo', 'namedprofile': None,   'attachment': None,   'flags': ' complain ', 'comment': None }),
        ('   /foo (complain) { # x',      { 'profile': '/foo',    'profile_keyword': False, 'plainprofile': '/foo', 'namedprofile': None,   'attachment': None,   'flags': 'complain', 'comment': '# x'}),

        ('   /foo {',                     { 'profile': '/foo',    'plainprofile': '/foo', 'namedprofile': None,  'leadingspace': '   ' }),
        ('/foo {',                        { 'profile': '/foo',    'plainprofile': '/foo', 'namedprofile': None,  'leadingspace': None  }),
        ('   profile foo {',              { 'profile': 'foo',     'plainprofile': None,   'namedprofile': 'foo', 'leadingspace': '   ' }),
        ('profile foo {',                 { 'profile': 'foo',     'plainprofile': None,   'namedprofile': 'foo', 'leadingspace': None  }),
        ('  @{foo} {',                    { 'profile': '@{foo}',  'plainprofile': '@{foo}',  'namedprofile': None,          'attachment': None,     'flags': None,       'comment': None }),
        ('  profile @{foo} {',            { 'profile': '@{foo}',  'plainprofile': None,      'namedprofile': '@{foo}',      'attachment': None,     'flags': None,       'comment': None }),
        ('  profile @{foo} /bar {',       { 'profile': '@{foo}',  'plainprofile': None,      'namedprofile': '@{foo}',      'attachment': '/bar',   'flags': None,       'comment': None }),
        ('  profile foo @{bar} {',        { 'profile': 'foo',     'plainprofile': None,      'namedprofile': 'foo',         'attachment': '@{bar}', 'flags': None,       'comment': None }),
        ('  profile @{foo} @{bar} {',     { 'profile': '@{foo}',  'plainprofile': None,      'namedprofile': '@{foo}',      'attachment': '@{bar}', 'flags': None,       'comment': None }),
    ]

    def _run_test(self, line, expected):
        matches = parse_profile_start_line(line, 'somefile')

        self.assertTrue(matches)

        for exp in expected:
            self.assertEqual(matches[exp], expected[exp], 'Group %s mismatch in rule %s' % (exp,line))

class TestInvalid_parse_profile_start_line(AATest):
    tests = [
        ('/bin/foo ', False), # no '{'
        ('/bin/foo /bin/bar', False), # missing 'profile' keyword
        ('profile {', False), # no attachment
        ('   profile foo bar /foo {', False), # missing quotes around "foo bar"
    ]

    def _run_test(self, line, expected):
        with self.assertRaises(AppArmorBug):
            parse_profile_start_line(line, 'somefile')

class Test_re_match_include(AATest):
    tests = [
        ('#include <abstractions/base>',            'abstractions/base'         ),  # magic path
        ('#include <abstractions/base> # comment',  'abstractions/base'         ),
        ('#include<abstractions/base>#comment',     'abstractions/base'         ),
        ('   #include    <abstractions/base>  ',    'abstractions/base'         ),
        ('#include "/foo/bar"',                     '/foo/bar'                  ),  # absolute path
        ('#include "/foo/bar" # comment',           '/foo/bar'                  ),
        ('#include "/foo/bar"#comment',             '/foo/bar'                  ),
        ('   #include "/foo/bar"  ',                '/foo/bar'                  ),
        ('include <abstractions/base>',            'abstractions/base'          ),  # magic path
        ('include <abstractions/base> # comment',  'abstractions/base'          ),
        ('include<abstractions/base>#comment',     'abstractions/base'          ),
        ('   include    <abstractions/base>  ',    'abstractions/base'          ),
        ('include "/foo/bar"',                     '/foo/bar'                   ),  # absolute path
        ('include "/foo/bar" # comment',           '/foo/bar'                   ),
        ('include "/foo/bar"#comment',             '/foo/bar'                   ),
        ('   include "/foo/bar"  ',                '/foo/bar'                   ),

        (' some #include <abstractions/base>',      None,                       ),  # non-matching
        ('  /etc/fstab r,',                         None,                       ),
        ('/usr/include r,',                         None,                       ),
        ('/include r,',                             None,                       ),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(re_match_include(params), expected)

class TestInvalid_re_match_include(AATest):
    tests = [
        ('#include <>',                             AppArmorException   ),  # '#include'
        ('#include <  >',                           AppArmorException   ),
        ('#include ""',                             AppArmorException   ),
        ('#include "  "',                           AppArmorException   ),
        ('#include',                                AppArmorException   ),
        ('#include  ',                              AppArmorException   ),
        ('#include "foo"',                          AppArmorException   ),  # LP: 1738880 (relative)
        ('#include "foo" # comment',                AppArmorException   ),
        ('#include "foo"#comment',                  AppArmorException   ),
        ('   #include "foo"  ',                     AppArmorException   ),
        ('#include "foo/bar"',                      AppArmorException   ),
        ('#include "foo/bar" # comment',            AppArmorException   ),
        ('#include "foo/bar"#comment',              AppArmorException   ),
        ('   #include "foo/bar"  ',                 AppArmorException   ),
        ('#include foo',                            AppArmorException   ),  # LP: 1738879 (no quotes)
        ('#include foo/bar',                        AppArmorException   ),
        ('#include /foo/bar',                       AppArmorException   ),
        ('#include foo bar',                        AppArmorException   ),  # LP: 1738877 (space in name)
        ('#include foo bar/baz',                    AppArmorException   ),
        ('#include "foo bar"',                      AppArmorException   ),
        ('#include /foo bar',                       AppArmorException   ),
        ('#include "/foo bar"',                     AppArmorException   ),
        ('#include "foo bar/baz"',                  AppArmorException   ),

        ('include <>',                              AppArmorException   ),  # 'include'
        ('include <  >',                            AppArmorException   ),
        ('include ""',                              AppArmorException   ),
        ('include "  "',                            AppArmorException   ),
        ('include',                                 AppArmorException   ),
        ('include  ',                               AppArmorException   ),
        ('include "foo"',                           AppArmorException   ),  # LP: 1738880 (relative)
        ('include "foo" # comment',                 AppArmorException   ),
        ('include "foo"#comment',                   AppArmorException   ),
        ('   include "foo"  ',                      AppArmorException   ),
        ('include "foo/bar"',                       AppArmorException   ),
        ('include "foo/bar" # comment',             AppArmorException   ),
        ('include "foo/bar"#comment',               AppArmorException   ),
        ('   include "foo/bar"  ',                  AppArmorException   ),
        ('include foo',                             AppArmorException   ),  # LP: 1738879 (no quotes)
        ('include foo/bar',                         AppArmorException   ),
        ('include /foo/bar',                        AppArmorException   ),
        ('include foo bar',                         AppArmorException   ),  # LP: 1738877 (space in name)
        ('include foo bar/baz',                     AppArmorException   ),
        ('include "foo bar"',                       AppArmorException   ),
        ('include /foo bar',                        AppArmorException   ),
        ('include "/foo bar"',                      AppArmorException   ),
        ('include "foo bar/baz"',                   AppArmorException   ),
    ]

    def _run_test(self, params, expected):
        with self.assertRaises(expected):
            re_match_include(params)


class TestStripParenthesis(AATest):
    tests = [
        ('foo',         'foo'       ),
        ('(foo)',       'foo'       ),
        ('(  foo )',    'foo'       ),
        ('(foo',        '(foo'      ),
        ('foo  )',      'foo  )'    ),
        ('foo ()',      'foo ()'    ),
        ('()',          ''          ),
        ('(  )',        ''          ),
        ('(())',        '()'        ),
        (' (foo)',       '(foo)'    ),  # parenthesis not first char, whitespace stripped nevertheless
        ('(foo) ',       '(foo)'    ),  # parenthesis not last char, whitespace stripped nevertheless
    ]

    def _run_test(self, params, expected):
        self.assertEqual(strip_parenthesis(params), expected)

class TestStripQuotes(AATest):
    def test_strip_quotes_01(self):
        self.assertEqual('foo', strip_quotes('foo'))
    def test_strip_quotes_02(self):
        self.assertEqual('foo', strip_quotes('"foo"'))
    def test_strip_quotes_03(self):
        self.assertEqual('"foo', strip_quotes('"foo'))
    def test_strip_quotes_04(self):
        self.assertEqual('foo"', strip_quotes('foo"'))
    def test_strip_quotes_05(self):
        self.assertEqual('', strip_quotes('""'))
    def test_strip_quotes_06(self):
        self.assertEqual('foo"bar', strip_quotes('foo"bar'))
    def test_strip_quotes_07(self):
        self.assertEqual('foo"bar', strip_quotes('"foo"bar"'))
    def test_strip_quotes_08(self):
        self.assertEqual('"""foo"bar"""', strip_quotes('""""foo"bar""""'))



setup_aa(aa)
setup_all_loops(__name__)
if __name__ == '__main__':
    # these two are not converted to a tests[] loop yet
    setup_has_comma_testcases()
    setup_split_comment_testcases()

    unittest.main(verbosity=1)
