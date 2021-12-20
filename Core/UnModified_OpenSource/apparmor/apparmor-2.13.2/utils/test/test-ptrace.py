#!/usr/bin/python3
# ----------------------------------------------------------------------
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
from collections import namedtuple
from common_test import AATest, setup_all_loops

from apparmor.rule.ptrace import PtraceRule, PtraceRuleset
from apparmor.rule import BaseRule
from apparmor.common import AppArmorException, AppArmorBug
from apparmor.logparser import ReadLog
from apparmor.translations import init_translation
_ = init_translation()

exp = namedtuple('exp', ['audit', 'allow_keyword', 'deny', 'comment',
        'access', 'all_access', 'peer', 'all_peers'])

# # --- tests for single PtraceRule --- #

class PtraceTest(AATest):
    def _compare_obj(self, obj, expected):
        self.assertEqual(expected.allow_keyword, obj.allow_keyword)
        self.assertEqual(expected.audit, obj.audit)
        self.assertEqual(expected.access, obj.access)
        if obj.peer:
            self.assertEqual(expected.peer, obj.peer.regex)
        else:
            self.assertEqual(expected.peer, obj.peer)
        self.assertEqual(expected.all_access, obj.all_access)
        self.assertEqual(expected.all_peers, obj.all_peers)
        self.assertEqual(expected.deny, obj.deny)
        self.assertEqual(expected.comment, obj.comment)

class PtraceTestParse(PtraceTest):
    tests = [
        # PtraceRule object                       audit  allow  deny   comment        access        all?   peer            all?
        ('ptrace,'                              , exp(False, False, False, '',        None  ,       True , None,           True     )),
#        ('ptrace (),'                           , exp(False, False, False, '',        None  ,       True , None,           True     )), # XXX also broken in SignalRule?
        ('ptrace read,'                         , exp(False, False, False, '',        {'read'},     False, None,           True     )),
        ('ptrace (read, tracedby),'             , exp(False, False, False, '',        {'read', 'tracedby'}, False, None,   True     )),
        ('ptrace read,'                         , exp(False, False, False, '',        {'read'},     False, None,           True     )),
        ('deny ptrace read, # cmt'              , exp(False, False, True , ' # cmt',  {'read'},     False, None,           True     )),
        ('audit allow ptrace,'                  , exp(True , True , False, '',        None  ,       True , None,           True     )),
        ('ptrace peer=unconfined,'              , exp(False, False, False, '',        None  ,       True , 'unconfined',   False    )),
        ('ptrace peer="unconfined",'            , exp(False, False, False, '',        None  ,       True , 'unconfined',   False    )),
        ('ptrace read,'                         , exp(False, False, False, '',        {'read'},     False, None,           True     )),
        ('ptrace peer=/foo,'                    , exp(False, False, False, '',        None  ,       True , '/foo',         False    )),
        ('ptrace r peer=/foo,'                  , exp(False, False, False, '',        {'r'},        False, '/foo',         False    )),
        ('ptrace r peer="/foo bar",'            , exp(False, False, False, '',        {'r'},        False, '/foo bar',     False    )),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(PtraceRule.match(rawrule))
        obj = PtraceRule.parse(rawrule)
        self.assertEqual(rawrule.strip(), obj.raw_rule)
        self._compare_obj(obj, expected)

class PtraceTestParseInvalid(PtraceTest):
    tests = [
        ('ptrace foo,'                     , AppArmorException),
        ('ptrace foo bar,'                 , AppArmorException),
        ('ptrace foo int,'                 , AppArmorException),
        ('ptrace read bar,'                , AppArmorException),
        ('ptrace read tracedby,'           , AppArmorException),
        ('ptrace peer=,'                   , AppArmorException),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(PtraceRule.match(rawrule))  # the above invalid rules still match the main regex!
        with self.assertRaises(expected):
            PtraceRule.parse(rawrule)

class PtraceTestParseFromLog(PtraceTest):
    def test_ptrace_from_log(self):
        parser = ReadLog('', '', '', '')
        event = 'type=AVC msg=audit(1409700683.304:547661): apparmor="DENIED" operation="ptrace" profile="/home/ubuntu/bzr/apparmor/tests/regression/apparmor/ptrace" pid=22465 comm="ptrace" requested_mask="tracedby" denied_mask="tracedby" peer="/home/ubuntu/bzr/apparmor/tests/regression/apparmor/ptrace"'


        parsed_event = parser.parse_event(event)

        self.assertEqual(parsed_event, {
            'request_mask': 'tracedby',
            'denied_mask': 'tracedby',
            'error_code': 0,
            'magic_token': 0,
            'parent': 0,
            'profile': '/home/ubuntu/bzr/apparmor/tests/regression/apparmor/ptrace',
            'peer': '/home/ubuntu/bzr/apparmor/tests/regression/apparmor/ptrace',
            'operation': 'ptrace',
            'resource': None,
            'info': None,
            'aamode': 'REJECTING',
            'time': 1409700683,
            'active_hat': None,
            'pid': 22465,
            'task': 0,
            'attr': None,
            'name2': None,
            'name': None,
            'family': None,
            'protocol': None,
            'sock_type': None,
        })

        obj = PtraceRule(parsed_event['denied_mask'], parsed_event['peer'], log_event=parsed_event)

        #              audit  allow  deny   comment    access        all?   peer                                                           all?
        expected = exp(False, False, False, '',        {'tracedby'}, False, '/home/ubuntu/bzr/apparmor/tests/regression/apparmor/ptrace', False)

        self._compare_obj(obj, expected)

        self.assertEqual(obj.get_raw(1), '  ptrace tracedby peer=/home/ubuntu/bzr/apparmor/tests/regression/apparmor/ptrace,')

class PtraceFromInit(PtraceTest):
    tests = [
        # PtraceRule object                                   audit  allow  deny   comment        access        all?   peer            all?
        (PtraceRule('r',                'unconfined', deny=True) , exp(False, False, True , ''           , {'r'},        False, 'unconfined',   False)),
        (PtraceRule(('r', 'read'),      '/bin/foo')     , exp(False, False, False, ''           , {'r', 'read'},False, '/bin/foo',     False)),
        (PtraceRule(PtraceRule.ALL,     '/bin/foo')     , exp(False, False, False, ''           , None,         True,  '/bin/foo',     False )),
        (PtraceRule('rw',               '/bin/foo')     , exp(False, False, False, ''           , {'rw'},       False, '/bin/foo',     False )),
        (PtraceRule('rw',               PtraceRule.ALL) , exp(False, False, False, ''           , {'rw'},       False, None,           True )),
        (PtraceRule(PtraceRule.ALL,     PtraceRule.ALL) , exp(False, False, False, ''           , None  ,       True,  None,           True )),
    ]

    def _run_test(self, obj, expected):
        self._compare_obj(obj, expected)

class InvalidPtraceInit(AATest):
    tests = [
        # init params                     expected exception
        ([''    ,   '/foo'    ]    , AppArmorBug), # empty access
        (['read',   ''        ]    , AppArmorBug), # empty peer
        (['    ',   '/foo'    ]    , AppArmorBug), # whitespace access
        (['read',   '    '    ]    , AppArmorBug), # whitespace peer
        (['xyxy',   '/foo'    ]    , AppArmorException), # invalid access
        # XXX is 'invalid peer' possible at all?
        ([dict(),   '/foo'    ]    , AppArmorBug), # wrong type for access
        ([None  ,   '/foo'    ]    , AppArmorBug), # wrong type for access
        (['read',   dict()    ]    , AppArmorBug), # wrong type for peer
        (['read',   None      ]    , AppArmorBug), # wrong type for peer
    ]

    def _run_test(self, params, expected):
        with self.assertRaises(expected):
            PtraceRule(params[0], params[1])

    def test_missing_params_1(self):
        with self.assertRaises(TypeError):
            PtraceRule()

    def test_missing_params_2(self):
        with self.assertRaises(TypeError):
            PtraceRule('r')

class InvalidPtraceTest(AATest):
    def _check_invalid_rawrule(self, rawrule):
        obj = None
        self.assertFalse(PtraceRule.match(rawrule))
        with self.assertRaises(AppArmorException):
            obj = PtraceRule(PtraceRule.parse(rawrule))

        self.assertIsNone(obj, 'PtraceRule handed back an object unexpectedly')

    def test_invalid_ptrace_missing_comma(self):
        self._check_invalid_rawrule('ptrace')  # missing comma

    def test_invalid_non_PtraceRule(self):
        self._check_invalid_rawrule('dbus,')  # not a ptrace rule

    def test_empty_data_1(self):
        obj = PtraceRule('read', '/foo')
        obj.access = ''
        # no access set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_2(self):
        obj = PtraceRule('read', '/foo')
        obj.peer = ''
        # no ptrace set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)


class WritePtraceTestAATest(AATest):
    def _run_test(self, rawrule, expected):
        self.assertTrue(PtraceRule.match(rawrule))
        obj = PtraceRule.parse(rawrule)
        clean = obj.get_clean()
        raw = obj.get_raw()

        self.assertEqual(expected.strip(), clean, 'unexpected clean rule')
        self.assertEqual(rawrule.strip(), raw, 'unexpected raw rule')

    tests = [
        #  raw rule                                               clean rule
        ('ptrace,'                                              , 'ptrace,'),
        ('     ptrace         ,    # foo        '               , 'ptrace, # foo'),
        ('    audit     ptrace read,'                           , 'audit ptrace read,'),
        ('    audit     ptrace (read  ),'                       , 'audit ptrace read,'),
        ('    audit     ptrace (read , tracedby ),'             , 'audit ptrace (read tracedby),'),
        ('   deny ptrace         read      ,# foo bar'          , 'deny ptrace read, # foo bar'),
        ('   deny ptrace      (  read      ),  '                , 'deny ptrace read,'),
        ('   allow ptrace                        ,# foo bar'    , 'allow ptrace, # foo bar'),
        ('ptrace,'                                              , 'ptrace,'),
        ('ptrace (trace),'                                      , 'ptrace trace,'),
        ('ptrace (tracedby),'                                   , 'ptrace tracedby,'),
        ('ptrace (read),'                                       , 'ptrace read,'),
        ('ptrace (readby),'                                     , 'ptrace readby,'),
        ('ptrace (trace read),'                                 , 'ptrace (read trace),'),
        ('ptrace (read tracedby),'                              , 'ptrace (read tracedby),'),
        ('ptrace r,'                                            , 'ptrace r,'),
        ('ptrace w,'                                            , 'ptrace w,'),
        ('ptrace rw,'                                           , 'ptrace rw,'),
        ('ptrace read,'                                         , 'ptrace read,'),
        ('ptrace (tracedby),'                                   , 'ptrace tracedby,'),
        ('ptrace w,'                                            , 'ptrace w,'),
        ('ptrace read peer=foo,'                                , 'ptrace read peer=foo,'),
        ('ptrace tracedby peer=foo,'                            , 'ptrace tracedby peer=foo,'),
        ('ptrace (read tracedby) peer=/usr/bin/bar,'            , 'ptrace (read tracedby) peer=/usr/bin/bar,'),
        ('ptrace (trace read) peer=/usr/bin/bar,'               , 'ptrace (read trace) peer=/usr/bin/bar,'),
        ('ptrace wr peer=/sbin/baz,'                            , 'ptrace wr peer=/sbin/baz,'),
    ]

    def test_write_manually(self):
        obj = PtraceRule('read', '/foo', allow_keyword=True)

        expected = '    allow ptrace read peer=/foo,'

        self.assertEqual(expected, obj.get_clean(2), 'unexpected clean rule')
        self.assertEqual(expected, obj.get_raw(2), 'unexpected raw rule')


class PtraceCoveredTest(AATest):
    def _run_test(self, param, expected):
        obj = PtraceRule.parse(self.rule)
        check_obj = PtraceRule.parse(param)

        self.assertTrue(PtraceRule.match(param))

        self.assertEqual(obj.is_equal(check_obj), expected[0], 'Mismatch in is_equal, expected %s' % expected[0])
        self.assertEqual(obj.is_equal(check_obj, True), expected[1], 'Mismatch in is_equal/strict, expected %s' % expected[1])

        self.assertEqual(obj.is_covered(check_obj), expected[2], 'Mismatch in is_covered, expected %s' % expected[2])
        self.assertEqual(obj.is_covered(check_obj, True, True), expected[3], 'Mismatch in is_covered/exact, expected %s' % expected[3])

class PtraceCoveredTest_01(PtraceCoveredTest):
    rule = 'ptrace read,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        ('ptrace,'                          , [ False   , False         , False     , False     ]),
        ('ptrace read,'                     , [ True    , True          , True      , True      ]),
        ('ptrace read peer=unconfined,'     , [ False   , False         , True      , True      ]),
        ('ptrace read, # comment'           , [ True    , False         , True      , True      ]),
        ('allow ptrace read,'               , [ True    , False         , True      , True      ]),
        ('ptrace     read,'                 , [ True    , False         , True      , True      ]),
        ('audit ptrace read,'               , [ False   , False         , False     , False     ]),
        ('audit ptrace,'                    , [ False   , False         , False     , False     ]),
        ('ptrace tracedby,'                 , [ False   , False         , False     , False     ]),
        ('audit deny ptrace read,'          , [ False   , False         , False     , False     ]),
        ('deny ptrace read,'                , [ False   , False         , False     , False     ]),
    ]

class PtraceCoveredTest_02(PtraceCoveredTest):
    rule = 'audit ptrace read,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'ptrace read,'               , [ False   , False         , True      , False     ]),
        ('audit ptrace read,'               , [ True    , True          , True      , True      ]),
        (      'ptrace,'                    , [ False   , False         , False     , False     ]),
        ('audit ptrace,'                    , [ False   , False         , False     , False     ]),
        ('ptrace tracedby,'                 , [ False   , False         , False     , False     ]),
    ]

class PtraceCoveredTest_03(PtraceCoveredTest):
    rule = 'ptrace,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'ptrace,'                    , [ True    , True          , True      , True      ]),
        ('allow ptrace,'                    , [ True    , False         , True      , True      ]),
        (      'ptrace read,'               , [ False   , False         , True      , True      ]),
        (      'ptrace w,'                  , [ False   , False         , True      , True      ]),
        ('audit ptrace,'                    , [ False   , False         , False     , False     ]),
        ('deny  ptrace,'                    , [ False   , False         , False     , False     ]),
    ]

class PtraceCoveredTest_04(PtraceCoveredTest):
    rule = 'deny ptrace read,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'deny ptrace read,'          , [ True    , True          , True      , True      ]),
        ('audit deny ptrace read,'          , [ False   , False         , False     , False     ]),
        (           'ptrace read,'          , [ False   , False         , False     , False     ]), # XXX should covered be true here?
        (      'deny ptrace tracedby,'      , [ False   , False         , False     , False     ]),
        (      'deny ptrace,'               , [ False   , False         , False     , False     ]),
    ]

class PtraceCoveredTest_05(PtraceCoveredTest):
    rule = 'ptrace read peer=unconfined,'

    tests = [
        #   rule                                    equal     strict equal    covered     covered exact
        ('ptrace,'                              , [ False   , False         , False     , False     ]),
        ('ptrace read,'                         , [ False   , False         , False     , False     ]),
        ('ptrace read peer=unconfined,'         , [ True    , True          , True      , True      ]),
        ('ptrace peer=unconfined,'              , [ False   , False         , False     , False     ]),
        ('ptrace read, # comment'               , [ False   , False         , False     , False     ]),
        ('allow ptrace read,'                   , [ False   , False         , False     , False     ]),
        ('allow ptrace read peer=unconfined,'   , [ True    , False         , True      , True      ]),
        ('allow ptrace read peer=/foo/bar,'     , [ False   , False         , False     , False     ]),
        ('allow ptrace read peer=/**,'          , [ False   , False         , False     , False     ]),
        ('allow ptrace read peer=**,'           , [ False   , False         , False     , False     ]),
        ('ptrace    read,'                      , [ False   , False         , False     , False     ]),
        ('ptrace    read peer=unconfined,'      , [ True    , False         , True      , True      ]),
        ('audit ptrace read peer=unconfined,'   , [ False   , False         , False     , False     ]),
        ('audit ptrace,'                        , [ False   , False         , False     , False     ]),
        ('ptrace tracedby,'                     , [ False   , False         , False     , False     ]),
        ('audit deny ptrace read,'              , [ False   , False         , False     , False     ]),
        ('deny ptrace read,'                    , [ False   , False         , False     , False     ]),
    ]

class PtraceCoveredTest_06(PtraceCoveredTest):
    rule = 'ptrace read peer=/foo/bar,'

    tests = [
        #   rule                                  equal     strict equal    covered     covered exact
        ('ptrace,'                              , [ False   , False         , False     , False     ]),
        ('ptrace read,'                         , [ False   , False         , False     , False     ]),
        ('ptrace read peer=/foo/bar,'           , [ True    , True          , True      , True      ]),
        ('ptrace read peer=/foo/*,'             , [ False   , False         , False     , False     ]),
        ('ptrace read peer=/**,'                , [ False   , False         , False     , False     ]),
        ('ptrace read peer=/what/*,'            , [ False   , False         , False     , False     ]),
        ('ptrace peer=/foo/bar,'                , [ False   , False         , False     , False     ]),
        ('ptrace read, # comment'               , [ False   , False         , False     , False     ]),
        ('allow ptrace read,'                   , [ False   , False         , False     , False     ]),
        ('allow ptrace read peer=/foo/bar,'     , [ True    , False         , True      , True      ]),
        ('ptrace    read,'                      , [ False   , False         , False     , False     ]),
        ('ptrace    read peer=/foo/bar,'        , [ True    , False         , True      , True      ]),
        ('ptrace    read peer=/what/ever,'      , [ False   , False         , False     , False     ]),
        ('audit ptrace read peer=/foo/bar,'     , [ False   , False         , False     , False     ]),
        ('audit ptrace,'                        , [ False   , False         , False     , False     ]),
        ('ptrace tracedby,'                     , [ False   , False         , False     , False     ]),
        ('audit deny ptrace read,'              , [ False   , False         , False     , False     ]),
        ('deny ptrace read,'                    , [ False   , False         , False     , False     ]),
    ]

class PtraceCoveredTest_07(PtraceCoveredTest):
    rule = 'ptrace read peer=**,'

    tests = [
        #   rule                                    equal     strict equal    covered     covered exact
        ('ptrace,'                              , [ False   , False         , False     , False     ]),
        ('ptrace read,'                         , [ False   , False         , False     , False     ]),
        ('ptrace read peer=/foo/bar,'           , [ False   , False         , True      , True      ]),
        ('ptrace read peer=/foo/*,'             , [ False   , False         , True      , True      ]),
        ('ptrace read peer=/**,'                , [ False   , False         , True      , True      ]),
        ('ptrace read peer=/what/*,'            , [ False   , False         , True      , True      ]),
        ('ptrace peer=/foo/bar,'                , [ False   , False         , False     , False     ]),
        ('ptrace read, # comment'               , [ False   , False         , False     , False     ]),
        ('allow ptrace read,'                   , [ False   , False         , False     , False     ]),
        ('allow ptrace read peer=/foo/bar,'     , [ False   , False         , True      , True      ]),
        ('ptrace    read,'                      , [ False   , False         , False     , False     ]),
        ('ptrace    read peer=/foo/bar,'        , [ False   , False         , True      , True      ]),
        ('ptrace    read peer=/what/ever,'      , [ False   , False         , True      , True      ]),
        ('audit ptrace read peer=/foo/bar,'     , [ False   , False         , False     , False     ]),
        ('audit ptrace,'                        , [ False   , False         , False     , False     ]),
        ('ptrace tracedby,'                     , [ False   , False         , False     , False     ]),
        ('audit deny ptrace read,'              , [ False   , False         , False     , False     ]),
        ('deny ptrace read,'                    , [ False   , False         , False     , False     ]),
    ]

class PtraceCoveredTest_08(PtraceCoveredTest):
    rule = 'ptrace (trace, tracedby) peer=/foo/*,'

    tests = [
        #   rule                                  equal     strict equal    covered     covered exact
        ('ptrace,'                              , [ False   , False         , False     , False     ]),
        ('ptrace trace,'                        , [ False   , False         , False     , False     ]),
        ('ptrace (tracedby, trace),'            , [ False   , False         , False     , False     ]),
        ('ptrace trace peer=/foo/bar,'          , [ False   , False         , True      , True      ]),
        ('ptrace (tracedby trace) peer=/foo/bar,',[ False   , False         , True      , True      ]),
        ('ptrace (tracedby, trace) peer=/foo/*,', [ True    , False         , True      , True      ]),
        ('ptrace tracedby peer=/foo/bar,'       , [ False   , False         , True      , True      ]),
        ('ptrace trace peer=/foo/*,'            , [ False   , False         , True      , True      ]),
        ('ptrace trace peer=/**,'               , [ False   , False         , False     , False     ]),
        ('ptrace trace peer=/what/*,'           , [ False   , False         , False     , False     ]),
        ('ptrace peer=/foo/bar,'                , [ False   , False         , False     , False     ]),
        ('ptrace trace, # comment'              , [ False   , False         , False     , False     ]),
        ('allow ptrace trace,'                  , [ False   , False         , False     , False     ]),
        ('allow ptrace trace peer=/foo/bar,'    , [ False   , False         , True      , True      ]),
        ('ptrace    trace,'                     , [ False   , False         , False     , False     ]),
        ('ptrace    trace peer=/foo/bar,'       , [ False   , False         , True      , True      ]),
        ('ptrace    trace peer=/what/ever,'     , [ False   , False         , False     , False     ]),
        ('audit ptrace trace peer=/foo/bar,'    , [ False   , False         , False     , False     ]),
        ('audit ptrace,'                        , [ False   , False         , False     , False     ]),
        ('ptrace tracedby,'                     , [ False   , False         , False     , False     ]),
        ('audit deny ptrace trace,'             , [ False   , False         , False     , False     ]),
        ('deny ptrace trace,'                   , [ False   , False         , False     , False     ]),
    ]



class PtraceCoveredTest_Invalid(AATest):
    def test_borked_obj_is_covered_1(self):
        obj = PtraceRule.parse('ptrace read peer=/foo,')

        testobj = PtraceRule('read', '/foo')
        testobj.access = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_borked_obj_is_covered_2(self):
        obj = PtraceRule.parse('ptrace read peer=/foo,')

        testobj = PtraceRule('read', '/foo')
        testobj.peer = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_covered(self):
        obj = PtraceRule.parse('ptrace read,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_equal(self):
        obj = PtraceRule.parse('ptrace read,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_equal(testobj)


class PtraceLogprofHeaderTest(AATest):
    tests = [
        ('ptrace,',                             [                               _('Access mode'), _('ALL'),         _('Peer'), _('ALL'),    ]),
        ('ptrace read,',                        [                               _('Access mode'), 'read',           _('Peer'), _('ALL'),    ]),
        ('deny ptrace,',                        [_('Qualifier'), 'deny',        _('Access mode'), _('ALL'),         _('Peer'), _('ALL'),    ]),
        ('allow ptrace read,',                  [_('Qualifier'), 'allow',       _('Access mode'), 'read',           _('Peer'), _('ALL'),    ]),
        ('audit ptrace read,',                  [_('Qualifier'), 'audit',       _('Access mode'), 'read',           _('Peer'), _('ALL'),    ]),
        ('audit deny ptrace read,',             [_('Qualifier'), 'audit deny',  _('Access mode'), 'read',           _('Peer'), _('ALL'),    ]),
        ('ptrace (read, tracedby) peer=/foo,',  [                               _('Access mode'), 'read tracedby',  _('Peer'), '/foo',      ]),
    ]

    def _run_test(self, params, expected):
        obj = PtraceRule._parse(params)
        self.assertEqual(obj.logprof_header(), expected)

## --- tests for PtraceRuleset --- #

class PtraceRulesTest(AATest):
    def test_empty_ruleset(self):
        ruleset = PtraceRuleset()
        ruleset_2 = PtraceRuleset()
        self.assertEqual([], ruleset.get_raw(2))
        self.assertEqual([], ruleset.get_clean(2))
        self.assertEqual([], ruleset_2.get_raw(2))
        self.assertEqual([], ruleset_2.get_clean(2))

    def test_ruleset_1(self):
        ruleset = PtraceRuleset()
        rules = [
            'ptrace peer=/foo,',
            'ptrace read,',
        ]

        expected_raw = [
            'ptrace peer=/foo,',
            'ptrace read,',
            '',
        ]

        expected_clean = [
            'ptrace peer=/foo,',
            'ptrace read,',
            '',
        ]

        for rule in rules:
            ruleset.add(PtraceRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw())
        self.assertEqual(expected_clean, ruleset.get_clean())

    def test_ruleset_2(self):
        ruleset = PtraceRuleset()
        rules = [
            'ptrace read peer=/foo,',
            'allow ptrace read,',
            'deny ptrace peer=/bar, # example comment',
        ]

        expected_raw = [
            '  ptrace read peer=/foo,',
            '  allow ptrace read,',
            '  deny ptrace peer=/bar, # example comment',
            '',
        ]

        expected_clean = [
            '  deny ptrace peer=/bar, # example comment',
            '',
            '  allow ptrace read,',
            '  ptrace read peer=/foo,',
            '',
        ]

        for rule in rules:
            ruleset.add(PtraceRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw(1))
        self.assertEqual(expected_clean, ruleset.get_clean(1))


class PtraceGlobTestAATest(AATest):
    def setUp(self):
        self.maxDiff = None
        self.ruleset = PtraceRuleset()

    def test_glob_1(self):
        self.assertEqual(self.ruleset.get_glob('ptrace read,'), 'ptrace,')

    # not supported or used yet
    # def test_glob_2(self):
    #     self.assertEqual(self.ruleset.get_glob('ptrace read raw,'), 'ptrace read,')

    def test_glob_ext(self):
        with self.assertRaises(NotImplementedError):
            # get_glob_ext is not available for ptrace rules
            self.ruleset.get_glob_ext('ptrace read peer=/foo,')

#class PtraceDeleteTestAATest(AATest):
#    pass

setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
