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

from apparmor.rule.signal import SignalRule, SignalRuleset
from apparmor.rule import BaseRule
from apparmor.common import AppArmorException, AppArmorBug
from apparmor.logparser import ReadLog
from apparmor.translations import init_translation
_ = init_translation()

exp = namedtuple('exp', ['audit', 'allow_keyword', 'deny', 'comment',
        'access', 'all_access', 'signal', 'all_signals', 'peer', 'all_peers'])

# --- tests for single SignalRule --- #

class SignalTest(AATest):
    def _compare_obj(self, obj, expected):
        self.assertEqual(expected.allow_keyword, obj.allow_keyword)
        self.assertEqual(expected.audit, obj.audit)
        self.assertEqual(expected.access, obj.access)
        self.assertEqual(expected.signal, obj.signal)
        if obj.peer:
            self.assertEqual(expected.peer, obj.peer.regex)
        else:
            self.assertEqual(expected.peer, obj.peer)
        self.assertEqual(expected.all_access, obj.all_access)
        self.assertEqual(expected.all_signals, obj.all_signals)
        self.assertEqual(expected.all_peers, obj.all_peers)
        self.assertEqual(expected.deny, obj.deny)
        self.assertEqual(expected.comment, obj.comment)

class SignalTestParse(SignalTest):
    tests = [
        # SignalRule object                       audit  allow  deny   comment        access        all?   signal           all?   peer            all?
        ('signal,'                              , exp(False, False, False, '',        None  ,       True , None,            True,  None,           True     )),
        ('signal send,'                         , exp(False, False, False, '',        {'send'},     False, None,            True,  None,           True     )),
        ('signal (send, receive),'              , exp(False, False, False, '',        {'send', 'receive'}, False, None,     True,  None,           True     )),
        ('signal send set=quit,'                , exp(False, False, False, '',        {'send'},     False, {'quit'},        False, None,           True     )),
        ('deny signal send set=quit, # cmt'     , exp(False, False, True , ' # cmt',  {'send'},     False, {'quit'},        False, None,           True     )),
        ('audit allow signal set=int,'          , exp(True , True , False, '',        None  ,       True , {'int'},         False, None,           True     )),
        ('signal set=quit peer=unconfined,'     , exp(False, False, False, '',        None  ,       True , {'quit'},        False, 'unconfined',   False    )),
        ('signal send set=(quit),'              , exp(False, False, False, '',        {'send'},     False, {'quit'},        False, None,           True     )),
        ('signal send set=(quit, int),'         , exp(False, False, False, '',        {'send'},     False, {'quit', 'int'}, False, None,           True     )),
        ('signal set=(quit, int),'              , exp(False, False, False, '',        None,         True,  {'quit', 'int'}, False, None,           True     )),
        ('signal send  set = ( quit , int ) ,'  , exp(False, False, False, '',        {'send'},     False, {'quit', 'int'}, False, None,           True     )),
        ('signal peer=/foo,'                    , exp(False, False, False, '',        None  ,       True , None,            True,  '/foo',         False    )),
        ('signal r set=quit set=int peer=/foo,' , exp(False, False, False, '',        {'r'},        False, {'quit', 'int'}, False, '/foo',         False    )),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(SignalRule.match(rawrule))
        obj = SignalRule.parse(rawrule)
        self.assertEqual(rawrule.strip(), obj.raw_rule)
        self._compare_obj(obj, expected)

class SignalTestParseInvalid(SignalTest):
    tests = [
        ('signal foo,'                     , AppArmorException),
        ('signal foo bar,'                 , AppArmorException),
        ('signal foo int,'                 , AppArmorException),
        ('signal send bar,'                , AppArmorException),
        ('signal send receive,'            , AppArmorException),
        ('signal set=,'                    , AppArmorException),
        ('signal set=int set=,'            , AppArmorException),
        ('signal set=invalid,'             , AppArmorException),
        ('signal peer=,'                   , AppArmorException),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(SignalRule.match(rawrule))  # the above invalid rules still match the main regex!
        with self.assertRaises(expected):
            SignalRule.parse(rawrule)

class SignalTestParseFromLog(SignalTest):
    def test_signal_from_log(self):
        parser = ReadLog('', '', '', '')
        event = 'type=AVC msg=audit(1409438250.564:201): apparmor="DENIED" operation="signal" profile="/usr/bin/pulseaudio" pid=2531 comm="pulseaudio" requested_mask="send" denied_mask="send" signal=term peer="/usr/bin/pulseaudio///usr/lib/pulseaudio/pulse/gconf-helper"'

        parsed_event = parser.parse_event(event)

        self.assertEqual(parsed_event, {
            'request_mask': 'send',
            'denied_mask': 'send',
            'error_code': 0,
            'magic_token': 0,
            'parent': 0,
            'profile': '/usr/bin/pulseaudio',
            'signal': 'term',
            'peer': '/usr/bin/pulseaudio///usr/lib/pulseaudio/pulse/gconf-helper',
            'operation': 'signal',
            'resource': None,
            'info': None,
            'aamode': 'REJECTING',
            'time': 1409438250,
            'active_hat': None,
            'pid': 2531,
            'task': 0,
            'attr': None,
            'name2': None,
            'name': None,
            'family': None,
            'protocol': None,
            'sock_type': None,
        })

        obj = SignalRule(parsed_event['denied_mask'], parsed_event['signal'], parsed_event['peer'], log_event=parsed_event)

        #              audit  allow  deny   comment    access        all?   signal       all?   peer                                                           all?
        expected = exp(False, False, False, '',        {'send'},     False, {'term'},    False, '/usr/bin/pulseaudio///usr/lib/pulseaudio/pulse/gconf-helper', False)

        self._compare_obj(obj, expected)

        self.assertEqual(obj.get_raw(1), '  signal send set=term peer=/usr/bin/pulseaudio///usr/lib/pulseaudio/pulse/gconf-helper,')

class SignalFromInit(SignalTest):
    tests = [
        # SignalRule object                                               audit  allow  deny   comment        access        all?   signal           all?   peer            all?
        (SignalRule('r',            'hup', 'unconfined', deny=True) , exp(False, False, True , ''           , {'r'},        False, {'hup'},         False, 'unconfined',   False)),
        (SignalRule(('r', 'send'),  ('hup', 'int'), '/bin/foo')     , exp(False, False, False, ''           , {'r', 'send'},False, {'hup', 'int'},  False, '/bin/foo',     False)),
        (SignalRule(SignalRule.ALL, 'int',          '/bin/foo')     , exp(False, False, False, ''           , None,         True,  {'int'},         False, '/bin/foo',     False )),
        (SignalRule('rw',           SignalRule.ALL, '/bin/foo')     , exp(False, False, False, ''           , {'rw'},       False, None,            True,  '/bin/foo',     False )),
        (SignalRule('rw',           ('int'),        SignalRule.ALL) , exp(False, False, False, ''           , {'rw'},       False, {'int'},         False, None,           True )),
        (SignalRule(SignalRule.ALL, SignalRule.ALL, SignalRule.ALL) , exp(False, False, False, ''           , None  ,       True,  None,            True,  None,           True )),
    ]

    def _run_test(self, obj, expected):
        self._compare_obj(obj, expected)

class InvalidSignalInit(AATest):
    tests = [
        # init params                     expected exception
        (['send', ''    , '/foo'    ]    , AppArmorBug), # empty signal
        ([''    , 'int' , '/foo'    ]    , AppArmorBug), # empty access
        (['send', 'int' , ''        ]    , AppArmorBug), # empty peer
        (['    ', 'int' , '/foo'    ]    , AppArmorBug), # whitespace access
        (['send', '   ' , '/foo'    ]    , AppArmorBug), # whitespace signal
        (['send', 'int' , '    '    ]    , AppArmorBug), # whitespace peer
        (['xyxy', 'int' , '/foo'    ]    , AppArmorException), # invalid access
        (['send', 'xyxy', '/foo'    ]    , AppArmorException), # invalid signal
        # XXX is 'invalid peer' possible at all?
        ([dict(), 'int' , '/foo'    ]    , AppArmorBug), # wrong type for access
        ([None  , 'int' , '/foo'    ]    , AppArmorBug), # wrong type for access
        (['send', dict(), '/foo'    ]    , AppArmorBug), # wrong type for signal
        (['send', None  , '/foo'    ]    , AppArmorBug), # wrong type for signal
        (['send', 'int' , dict()    ]    , AppArmorBug), # wrong type for peer
        (['send', 'int' , None      ]    , AppArmorBug), # wrong type for peer
    ]

    def _run_test(self, params, expected):
        with self.assertRaises(expected):
            SignalRule(params[0], params[1], params[2])

    def test_missing_params_1(self):
        with self.assertRaises(TypeError):
            SignalRule()

    def test_missing_params_2(self):
        with self.assertRaises(TypeError):
            SignalRule('r')

    def test_missing_params_3(self):
        with self.assertRaises(TypeError):
            SignalRule('r', 'int')

class InvalidSignalTest(AATest):
    def _check_invalid_rawrule(self, rawrule):
        obj = None
        self.assertFalse(SignalRule.match(rawrule))
        with self.assertRaises(AppArmorException):
            obj = SignalRule(SignalRule.parse(rawrule))

        self.assertIsNone(obj, 'SignalRule handed back an object unexpectedly')

    def test_invalid_signal_missing_comma(self):
        self._check_invalid_rawrule('signal')  # missing comma

    def test_invalid_non_SignalRule(self):
        self._check_invalid_rawrule('dbus,')  # not a signal rule

    def test_empty_data_1(self):
        obj = SignalRule('send', 'quit', '/foo')
        obj.access = ''
        # no access set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_2(self):
        obj = SignalRule('send', 'quit', '/foo')
        obj.signal = ''
        # no signal set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_3(self):
        obj = SignalRule('send', 'quit', '/foo')
        obj.peer = ''
        # no signal set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)


class WriteSignalTestAATest(AATest):
    def _run_test(self, rawrule, expected):
        self.assertTrue(SignalRule.match(rawrule))
        obj = SignalRule.parse(rawrule)
        clean = obj.get_clean()
        raw = obj.get_raw()

        self.assertEqual(expected.strip(), clean, 'unexpected clean rule')
        self.assertEqual(rawrule.strip(), raw, 'unexpected raw rule')

    tests = [
        #  raw rule                                               clean rule
        ('     signal         ,    # foo        '              , 'signal, # foo'),
        ('    audit     signal send,'                          , 'audit signal send,'),
        ('    audit     signal (send  ),'                      , 'audit signal send,'),
        ('    audit     signal (send , receive ),'             , 'audit signal (receive send),'),
        ('   deny signal         send      set=quit,# foo bar' , 'deny signal send set=quit, # foo bar'),
        ('   deny signal         send      set=(quit),  '      , 'deny signal send set=quit,'),
        ('   deny signal         send      set=(int , quit),'  , 'deny signal send set=(int quit),'),
        ('   deny signal         send      set=(quit, int ),'  , 'deny signal send set=(int quit),'),
        ('   deny signal         send      ,# foo bar'         , 'deny signal send, # foo bar'),
        ('   allow signal             set=int    ,# foo bar'   , 'allow signal set=int, # foo bar'),
        ('signal,'                                             , 'signal,'),
        ('signal (receive),'                                   , 'signal receive,'),
        ('signal (send),'                                      , 'signal send,'),
        ('signal (send receive),'                              , 'signal (receive send),'),
        ('signal r,'                                           , 'signal r,'),
        ('signal w,'                                           , 'signal w,'),
        ('signal rw,'                                          , 'signal rw,'),
        ('signal send set=("hup"),'                            , 'signal send set=hup,'),
        ('signal (receive) set=kill,'                          , 'signal receive set=kill,'),
        ('signal w set=(quit int),'                            , 'signal w set=(int quit),'),
        ('signal receive peer=foo,'                            , 'signal receive peer=foo,'),
        ('signal (send receive) peer=/usr/bin/bar,'            , 'signal (receive send) peer=/usr/bin/bar,'),
        ('signal wr set=(pipe, usr1) peer=/sbin/baz,'          , 'signal wr set=(pipe usr1) peer=/sbin/baz,'),
    ]

    def test_write_manually(self):
        obj = SignalRule('send', 'quit', '/foo', allow_keyword=True)

        expected = '    allow signal send set=quit peer=/foo,'

        self.assertEqual(expected, obj.get_clean(2), 'unexpected clean rule')
        self.assertEqual(expected, obj.get_raw(2), 'unexpected raw rule')


class SignalCoveredTest(AATest):
    def _run_test(self, param, expected):
        obj = SignalRule.parse(self.rule)
        check_obj = SignalRule.parse(param)

        self.assertTrue(SignalRule.match(param))

        self.assertEqual(obj.is_equal(check_obj), expected[0], 'Mismatch in is_equal, expected %s' % expected[0])
        self.assertEqual(obj.is_equal(check_obj, True), expected[1], 'Mismatch in is_equal/strict, expected %s' % expected[1])

        self.assertEqual(obj.is_covered(check_obj), expected[2], 'Mismatch in is_covered, expected %s' % expected[2])
        self.assertEqual(obj.is_covered(check_obj, True, True), expected[3], 'Mismatch in is_covered/exact, expected %s' % expected[3])

class SignalCoveredTest_01(SignalCoveredTest):
    rule = 'signal send,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        ('signal,'                         , [ False   , False         , False     , False     ]),
        ('signal send,'                    , [ True    , True          , True      , True      ]),
        ('signal send peer=unconfined,'    , [ False   , False         , True      , True      ]),
        ('signal send, # comment'          , [ True    , False         , True      , True      ]),
        ('allow signal send,'              , [ True    , False         , True      , True      ]),
        ('signal     send,'                , [ True    , False         , True      , True      ]),
        ('signal send set=quit,'           , [ False   , False         , True      , True      ]),
        ('signal send set=int,'            , [ False   , False         , True      , True      ]),
        ('audit signal send,'              , [ False   , False         , False     , False     ]),
        ('audit signal,'                   , [ False   , False         , False     , False     ]),
        ('signal receive,'                 , [ False   , False         , False     , False     ]),
        ('signal set=int,'                 , [ False   , False         , False     , False     ]),
        ('audit deny signal send,'         , [ False   , False         , False     , False     ]),
        ('deny signal send,'               , [ False   , False         , False     , False     ]),
    ]

class SignalCoveredTest_02(SignalCoveredTest):
    rule = 'audit signal send,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'signal send,'              , [ False   , False         , True      , False     ]),
        ('audit signal send,'              , [ True    , True          , True      , True      ]),
        (      'signal send set=quit,'     , [ False   , False         , True      , False     ]),
        ('audit signal send set=quit,'     , [ False   , False         , True      , True      ]),
        (      'signal,'                   , [ False   , False         , False     , False     ]),
        ('audit signal,'                   , [ False   , False         , False     , False     ]),
        ('signal receive,'                 , [ False   , False         , False     , False     ]),
    ]

class SignalCoveredTest_03(SignalCoveredTest):
    rule = 'signal send set=quit,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'signal send set=quit,'     , [ True    , True          , True      , True      ]),
        ('allow signal send set=quit,'     , [ True    , False         , True      , True      ]),
        (      'signal send,'              , [ False   , False         , False     , False     ]),
        (      'signal,'                   , [ False   , False         , False     , False     ]),
        (      'signal send set=int,'      , [ False   , False         , False     , False     ]),
        ('audit signal,'                   , [ False   , False         , False     , False     ]),
        ('audit signal send set=quit,'     , [ False   , False         , False     , False     ]),
        ('audit signal set=quit,'          , [ False   , False         , False     , False     ]),
        (      'signal send,'              , [ False   , False         , False     , False     ]),
        (      'signal,'                   , [ False   , False         , False     , False     ]),
    ]

class SignalCoveredTest_04(SignalCoveredTest):
    rule = 'signal,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'signal,'                   , [ True    , True          , True      , True      ]),
        ('allow signal,'                   , [ True    , False         , True      , True      ]),
        (      'signal send,'              , [ False   , False         , True      , True      ]),
        (      'signal w set=quit,'        , [ False   , False         , True      , True      ]),
        (      'signal set=int,'           , [ False   , False         , True      , True      ]),
        (      'signal send set=quit,'     , [ False   , False         , True      , True      ]),
        ('audit signal,'                   , [ False   , False         , False     , False     ]),
        ('deny  signal,'                   , [ False   , False         , False     , False     ]),
    ]

class SignalCoveredTest_05(SignalCoveredTest):
    rule = 'deny signal send,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'deny signal send,'         , [ True    , True          , True      , True      ]),
        ('audit deny signal send,'         , [ False   , False         , False     , False     ]),
        (           'signal send,'         , [ False   , False         , False     , False     ]), # XXX should covered be true here?
        (      'deny signal receive,'      , [ False   , False         , False     , False     ]),
        (      'deny signal,'              , [ False   , False         , False     , False     ]),
    ]

class SignalCoveredTest_06(SignalCoveredTest):
    rule = 'signal send peer=unconfined,'

    tests = [
        #   rule                                  equal     strict equal    covered     covered exact
        ('signal,'                            , [ False   , False         , False     , False     ]),
        ('signal send,'                       , [ False   , False         , False     , False     ]),
        ('signal send peer=unconfined,'       , [ True    , True          , True      , True      ]),
        ('signal peer=unconfined,'            , [ False   , False         , False     , False     ]),
        ('signal send, # comment'             , [ False   , False         , False     , False     ]),
        ('allow signal send,'                 , [ False   , False         , False     , False     ]),
        ('allow signal send peer=unconfined,' , [ True    , False         , True      , True      ]),
        ('allow signal send peer=/foo/bar,'   , [ False   , False         , False     , False     ]),
        ('allow signal send peer=/**,'        , [ False   , False         , False     , False     ]),
        ('allow signal send peer=**,'         , [ False   , False         , False     , False     ]),
        ('signal    send,'                    , [ False   , False         , False     , False     ]),
        ('signal    send peer=unconfined,'    , [ True    , False         , True      , True      ]),
        ('signal send set=quit,'              , [ False   , False         , False     , False     ]),
        ('signal send set=int peer=unconfined,',[ False   , False         , True      , True      ]),
        ('audit signal send peer=unconfined,' , [ False   , False         , False     , False     ]),
        ('audit signal,'                      , [ False   , False         , False     , False     ]),
        ('signal receive,'                    , [ False   , False         , False     , False     ]),
        ('signal set=int,'                    , [ False   , False         , False     , False     ]),
        ('audit deny signal send,'            , [ False   , False         , False     , False     ]),
        ('deny signal send,'                  , [ False   , False         , False     , False     ]),
    ]

class SignalCoveredTest_07(SignalCoveredTest):
    rule = 'signal send peer=/foo/bar,'

    tests = [
        #   rule                                  equal     strict equal    covered     covered exact
        ('signal,'                            , [ False   , False         , False     , False     ]),
        ('signal send,'                       , [ False   , False         , False     , False     ]),
        ('signal send peer=/foo/bar,'         , [ True    , True          , True      , True      ]),
        ('signal send peer=/foo/*,'           , [ False   , False         , False     , False     ]),
        ('signal send peer=/**,'              , [ False   , False         , False     , False     ]),
        ('signal send peer=/what/*,'          , [ False   , False         , False     , False     ]),
        ('signal peer=/foo/bar,'              , [ False   , False         , False     , False     ]),
        ('signal send, # comment'             , [ False   , False         , False     , False     ]),
        ('allow signal send,'                 , [ False   , False         , False     , False     ]),
        ('allow signal send peer=/foo/bar,'   , [ True    , False         , True      , True      ]),
        ('signal    send,'                    , [ False   , False         , False     , False     ]),
        ('signal    send peer=/foo/bar,'      , [ True    , False         , True      , True      ]),
        ('signal    send peer=/what/ever,'    , [ False   , False         , False     , False     ]),
        ('signal send set=quit,'              , [ False   , False         , False     , False     ]),
        ('signal send set=int peer=/foo/bar,' , [ False   , False         , True      , True      ]),
        ('audit signal send peer=/foo/bar,'   , [ False   , False         , False     , False     ]),
        ('audit signal,'                      , [ False   , False         , False     , False     ]),
        ('signal receive,'                    , [ False   , False         , False     , False     ]),
        ('signal set=int,'                    , [ False   , False         , False     , False     ]),
        ('audit deny signal send,'            , [ False   , False         , False     , False     ]),
        ('deny signal send,'                  , [ False   , False         , False     , False     ]),
    ]

class SignalCoveredTest_08(SignalCoveredTest):
    rule = 'signal send peer=**,'

    tests = [
        #   rule                                  equal     strict equal    covered     covered exact
        ('signal,'                            , [ False   , False         , False     , False     ]),
        ('signal send,'                       , [ False   , False         , False     , False     ]),
        ('signal send peer=/foo/bar,'         , [ False   , False         , True      , True      ]),
        ('signal send peer=/foo/*,'           , [ False   , False         , True      , True      ]),
        ('signal send peer=/**,'              , [ False   , False         , True      , True      ]),
        ('signal send peer=/what/*,'          , [ False   , False         , True      , True      ]),
        ('signal peer=/foo/bar,'              , [ False   , False         , False     , False     ]),
        ('signal send, # comment'             , [ False   , False         , False     , False     ]),
        ('allow signal send,'                 , [ False   , False         , False     , False     ]),
        ('allow signal send peer=/foo/bar,'   , [ False   , False         , True      , True      ]),
        ('signal    send,'                    , [ False   , False         , False     , False     ]),
        ('signal    send peer=/foo/bar,'      , [ False   , False         , True      , True      ]),
        ('signal    send peer=/what/ever,'    , [ False   , False         , True      , True      ]),
        ('signal send set=quit,'              , [ False   , False         , False     , False     ]),
        ('signal send set=int peer=/foo/bar,' , [ False   , False         , True      , True      ]),
        ('audit signal send peer=/foo/bar,'   , [ False   , False         , False     , False     ]),
        ('audit signal,'                      , [ False   , False         , False     , False     ]),
        ('signal receive,'                    , [ False   , False         , False     , False     ]),
        ('signal set=int,'                    , [ False   , False         , False     , False     ]),
        ('audit deny signal send,'            , [ False   , False         , False     , False     ]),
        ('deny signal send,'                  , [ False   , False         , False     , False     ]),
    ]

class SignalCoveredTest_09(SignalCoveredTest):
    rule = 'signal (send, receive) set=(int, quit),'

    tests = [
        #   rule                                  equal     strict equal    covered     covered exact
        ('signal,'                            , [ False   , False         , False     , False     ]),
        ('signal send,'                       , [ False   , False         , False     , False     ]),
        ('signal send set=int,'               , [ False   , False         , True      , True      ]),
        ('signal receive set=quit,'           , [ False   , False         , True      , True      ]),
        ('signal (receive,send) set=int,'     , [ False   , False         , True      , True      ]),
        ('signal (receive,send) set=(int quit),',[True    , False         , True      , True      ]),
        ('signal send set=(quit int),'        , [ False   , False         , True      , True      ]),
        ('signal send peer=/foo/bar,'         , [ False   , False         , False     , False     ]),
        ('signal send peer=/foo/*,'           , [ False   , False         , False     , False     ]),
        ('signal send peer=/**,'              , [ False   , False         , False     , False     ]),
        ('signal send peer=/what/*,'          , [ False   , False         , False     , False     ]),
        ('signal peer=/foo/bar,'              , [ False   , False         , False     , False     ]),
        ('signal send, # comment'             , [ False   , False         , False     , False     ]),
        ('allow signal send,'                 , [ False   , False         , False     , False     ]),
        ('allow signal send peer=/foo/bar,'   , [ False   , False         , False     , False     ]),
        ('signal    send,'                    , [ False   , False         , False     , False     ]),
        ('signal    send peer=/foo/bar,'      , [ False   , False         , False     , False     ]),
        ('signal    send peer=/what/ever,'    , [ False   , False         , False     , False     ]),
        ('signal send set=quit,'              , [ False   , False         , True      , True      ]),
        ('signal send set=int peer=/foo/bar,' , [ False   , False         , True      , True      ]),
        ('audit signal send peer=/foo/bar,'   , [ False   , False         , False     , False     ]),
        ('audit signal,'                      , [ False   , False         , False     , False     ]),
        ('signal receive,'                    , [ False   , False         , False     , False     ]),
        ('signal set=int,'                    , [ False   , False         , False     , False     ]),
        ('audit deny signal send,'            , [ False   , False         , False     , False     ]),
        ('deny signal send,'                  , [ False   , False         , False     , False     ]),
    ]



class SignalCoveredTest_Invalid(AATest):
    def test_borked_obj_is_covered_1(self):
        obj = SignalRule.parse('signal send peer=/foo,')

        testobj = SignalRule('send', 'quit', '/foo')
        testobj.access = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_borked_obj_is_covered_2(self):
        obj = SignalRule.parse('signal send set=quit peer=/foo,')

        testobj = SignalRule('send', 'quit', '/foo')
        testobj.signal = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_borked_obj_is_covered_3(self):
        obj = SignalRule.parse('signal send set=quit peer=/foo,')

        testobj = SignalRule('send', 'quit', '/foo')
        testobj.peer = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_covered(self):
        obj = SignalRule.parse('signal send,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_equal(self):
        obj = SignalRule.parse('signal send,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_equal(testobj)

class SignalLogprofHeaderTest(AATest):
    tests = [
        ('signal,',                     [                               _('Access mode'), _('ALL'),         _('Signal'), _('ALL'),      _('Peer'), _('ALL'),    ]),
        ('signal send,',                [                               _('Access mode'), 'send',           _('Signal'), _('ALL'),      _('Peer'), _('ALL'),    ]),
        ('signal send set=quit,',       [                               _('Access mode'), 'send',           _('Signal'), 'quit',        _('Peer'), _('ALL'),    ]),
        ('deny signal,',                [_('Qualifier'), 'deny',        _('Access mode'), _('ALL'),         _('Signal'), _('ALL'),      _('Peer'), _('ALL'),    ]),
        ('allow signal send,',          [_('Qualifier'), 'allow',       _('Access mode'), 'send',           _('Signal'), _('ALL'),      _('Peer'), _('ALL'),    ]),
        ('audit signal send set=quit,', [_('Qualifier'), 'audit',       _('Access mode'), 'send',           _('Signal'), 'quit',        _('Peer'), _('ALL'),    ]),
        ('audit deny signal send,',     [_('Qualifier'), 'audit deny',  _('Access mode'), 'send',           _('Signal'), _('ALL'),      _('Peer'), _('ALL'),    ]),
        ('signal set=(int, quit),',     [                               _('Access mode'), _('ALL'),         _('Signal'), 'int quit',    _('Peer'), _('ALL'),    ]),
        ('signal set=( quit, int),',    [                               _('Access mode'), _('ALL'),         _('Signal'), 'int quit',    _('Peer'), _('ALL'),    ]),
        ('signal (send, receive) set=( quit, int) peer=/foo,',    [     _('Access mode'), 'receive send',   _('Signal'), 'int quit',    _('Peer'), '/foo',      ]),
    ]

    def _run_test(self, params, expected):
        obj = SignalRule._parse(params)
        self.assertEqual(obj.logprof_header(), expected)

## --- tests for SignalRuleset --- #

class SignalRulesTest(AATest):
    def test_empty_ruleset(self):
        ruleset = SignalRuleset()
        ruleset_2 = SignalRuleset()
        self.assertEqual([], ruleset.get_raw(2))
        self.assertEqual([], ruleset.get_clean(2))
        self.assertEqual([], ruleset_2.get_raw(2))
        self.assertEqual([], ruleset_2.get_clean(2))

    def test_ruleset_1(self):
        ruleset = SignalRuleset()
        rules = [
            'signal set=int,',
            'signal send,',
        ]

        expected_raw = [
            'signal set=int,',
            'signal send,',
            '',
        ]

        expected_clean = [
            'signal send,',
            'signal set=int,',
            '',
        ]

        for rule in rules:
            ruleset.add(SignalRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw())
        self.assertEqual(expected_clean, ruleset.get_clean())

    def test_ruleset_2(self):
        ruleset = SignalRuleset()
        rules = [
            'signal send set=int,',
            'allow signal send,',
            'deny signal set=quit, # example comment',
        ]

        expected_raw = [
            '  signal send set=int,',
            '  allow signal send,',
            '  deny signal set=quit, # example comment',
            '',
        ]

        expected_clean = [
            '  deny signal set=quit, # example comment',
            '',
            '  allow signal send,',
            '  signal send set=int,',
            '',
        ]

        for rule in rules:
            ruleset.add(SignalRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw(1))
        self.assertEqual(expected_clean, ruleset.get_clean(1))


class SignalGlobTestAATest(AATest):
    def setUp(self):
        self.maxDiff = None
        self.ruleset = SignalRuleset()

    def test_glob_1(self):
        self.assertEqual(self.ruleset.get_glob('signal send,'), 'signal,')

    # not supported or used yet
    # def test_glob_2(self):
    #     self.assertEqual(self.ruleset.get_glob('signal send raw,'), 'signal send,')

    def test_glob_ext(self):
        with self.assertRaises(NotImplementedError):
            # get_glob_ext is not available for signal rules
            self.ruleset.get_glob_ext('signal send set=int,')

#class SignalDeleteTestAATest(AATest):
#    pass

setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
