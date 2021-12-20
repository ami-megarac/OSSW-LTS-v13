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

from apparmor.rule.network import NetworkRule, NetworkRuleset
from apparmor.rule import BaseRule
from apparmor.common import AppArmorException, AppArmorBug
from apparmor.logparser import ReadLog
from apparmor.translations import init_translation
_ = init_translation()

exp = namedtuple('exp', ['audit', 'allow_keyword', 'deny', 'comment',
        'domain', 'all_domains', 'type_or_protocol', 'all_type_or_protocols'])

# --- tests for single NetworkRule --- #

class NetworkTest(AATest):
    def _compare_obj(self, obj, expected):
        self.assertEqual(expected.allow_keyword, obj.allow_keyword)
        self.assertEqual(expected.audit, obj.audit)
        self.assertEqual(expected.domain, obj.domain)
        self.assertEqual(expected.type_or_protocol, obj.type_or_protocol)
        self.assertEqual(expected.all_domains, obj.all_domains)
        self.assertEqual(expected.all_type_or_protocols, obj.all_type_or_protocols)
        self.assertEqual(expected.deny, obj.deny)
        self.assertEqual(expected.comment, obj.comment)

class NetworkTestParse(NetworkTest):
    tests = [
        # rawrule                                     audit  allow  deny   comment        domain    all?   type/proto  all?
        ('network,'                             , exp(False, False, False, ''           , None  ,   True , None     , True )),
        ('network inet,'                        , exp(False, False, False, ''           , 'inet',   False, None     , True )),
        ('network inet stream,'                 , exp(False, False, False, ''           , 'inet',   False, 'stream' , False)),
        ('deny network inet stream, # comment'  , exp(False, False, True , ' # comment' , 'inet',   False, 'stream' , False)),
        ('audit allow network tcp,'             , exp(True , True , False, ''           , None  ,   True , 'tcp'    , False)),
        ('network stream,'                      , exp(False, False, False, ''           , None  ,   True , 'stream' , False)),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(NetworkRule.match(rawrule))
        obj = NetworkRule.parse(rawrule)
        self.assertEqual(rawrule.strip(), obj.raw_rule)
        self._compare_obj(obj, expected)

class NetworkTestParseInvalid(NetworkTest):
    tests = [
        ('network foo,'                     , AppArmorException),
        ('network foo bar,'                 , AppArmorException),
        ('network foo tcp,'                 , AppArmorException),
        ('network inet bar,'                , AppArmorException),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(NetworkRule.match(rawrule))  # the above invalid rules still match the main regex!
        with self.assertRaises(expected):
            NetworkRule.parse(rawrule)

class NetworkTestParseFromLog(NetworkTest):
    def test_net_from_log(self):
        parser = ReadLog('', '', '', '')
        event = 'type=AVC msg=audit(1428699242.551:386): apparmor="DENIED" operation="create" profile="/bin/ping" pid=10589 comm="ping" family="inet" sock_type="raw" protocol=1'

        parsed_event = parser.parse_event(event)

        self.assertEqual(parsed_event, {
            'request_mask': None,
            'denied_mask': None,
            'error_code': 0,
            'family': 'inet',
            'magic_token': 0,
            'parent': 0,
            'profile': '/bin/ping',
            'protocol': 'icmp',
            'sock_type': 'raw',
            'operation': 'create',
            'resource': None,
            'info': None,
            'aamode': 'REJECTING',
            'time': 1428699242,
            'active_hat': None,
            'pid': 10589,
            'task': 0,
            'attr': None,
            'name2': None,
            'name': None,
        })

        obj = NetworkRule(parsed_event['family'], parsed_event['sock_type'], log_event=parsed_event)

        #              audit  allow  deny   comment        domain    all?   type/proto  all?
        expected = exp(False, False, False, ''           , 'inet',   False, 'raw'    , False)

        self._compare_obj(obj, expected)

        self.assertEqual(obj.get_raw(1), '  network inet raw,')


class NetworkFromInit(NetworkTest):
    tests = [
        # NetworkRule object                                  audit  allow  deny   comment        domain    all?   type/proto  all?
        (NetworkRule('inet', 'raw', deny=True)          , exp(False, False, True , ''           , 'inet',   False, 'raw'    , False)),
        (NetworkRule('inet', 'raw')                     , exp(False, False, False, ''           , 'inet',   False, 'raw'    , False)),
        (NetworkRule('inet', NetworkRule.ALL)           , exp(False, False, False, ''           , 'inet',   False, None     , True )),
        (NetworkRule(NetworkRule.ALL, NetworkRule.ALL)  , exp(False, False, False, ''           , None  ,   True , None     , True )),
        (NetworkRule(NetworkRule.ALL, 'tcp')            , exp(False, False, False, ''           , None  ,   True , 'tcp'    , False)),
        (NetworkRule(NetworkRule.ALL, 'stream')         , exp(False, False, False, ''           , None  ,   True , 'stream' , False)),
    ]

    def _run_test(self, obj, expected):
        self._compare_obj(obj, expected)


class InvalidNetworkInit(AATest):
    tests = [
        # init params                     expected exception
        (['inet', ''               ]    , AppArmorBug), # empty type_or_protocol
        ([''    , 'tcp'            ]    , AppArmorBug), # empty domain
        (['    ', 'tcp'            ]    , AppArmorBug), # whitespace domain
        (['inet', '   '            ]    , AppArmorBug), # whitespace type_or_protocol
        (['xyxy', 'tcp'            ]    , AppArmorBug), # invalid domain
        (['inet', 'xyxy'           ]    , AppArmorBug), # invalid type_or_protocol
        ([dict(), 'tcp'            ]    , AppArmorBug), # wrong type for domain
        ([None  , 'tcp'            ]    , AppArmorBug), # wrong type for domain
        (['inet', dict()           ]    , AppArmorBug), # wrong type for type_or_protocol
        (['inet', None             ]    , AppArmorBug), # wrong type for type_or_protocol
    ]

    def _run_test(self, params, expected):
        with self.assertRaises(expected):
            NetworkRule(params[0], params[1])

    def test_missing_params_1(self):
        with self.assertRaises(TypeError):
            NetworkRule()

    def test_missing_params_2(self):
        with self.assertRaises(TypeError):
            NetworkRule('inet')


class InvalidNetworkTest(AATest):
    def _check_invalid_rawrule(self, rawrule):
        obj = None
        self.assertFalse(NetworkRule.match(rawrule))
        with self.assertRaises(AppArmorException):
            obj = NetworkRule(NetworkRule.parse(rawrule))

        self.assertIsNone(obj, 'NetworkRule handed back an object unexpectedly')

    def test_invalid_net_missing_comma(self):
        self._check_invalid_rawrule('network')  # missing comma

    def test_invalid_net_non_NetworkRule(self):
        self._check_invalid_rawrule('dbus,')  # not a network rule

    def test_empty_net_data_1(self):
        obj = NetworkRule('inet', 'stream')
        obj.domain = ''
        # no domain set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_net_data_2(self):
        obj = NetworkRule('inet', 'stream')
        obj.type_or_protocol = ''
        # no type_or_protocol set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)


class WriteNetworkTestAATest(AATest):
    def _run_test(self, rawrule, expected):
        self.assertTrue(NetworkRule.match(rawrule))
        obj = NetworkRule.parse(rawrule)
        clean = obj.get_clean()
        raw = obj.get_raw()

        self.assertEqual(expected.strip(), clean, 'unexpected clean rule')
        self.assertEqual(rawrule.strip(), raw, 'unexpected raw rule')

    tests = [
        #  raw rule                                               clean rule
        ('     network         ,    # foo        '              , 'network, # foo'),
        ('    audit     network inet,'                          , 'audit network inet,'),
        ('   deny network         inet      stream,# foo bar'   , 'deny network inet stream, # foo bar'),
        ('   deny network         inet      ,# foo bar'         , 'deny network inet, # foo bar'),
        ('   allow network         tcp      ,# foo bar'         , 'allow network tcp, # foo bar'),
    ]

    def test_write_manually(self):
        obj = NetworkRule('inet', 'stream', allow_keyword=True)

        expected = '    allow network inet stream,'

        self.assertEqual(expected, obj.get_clean(2), 'unexpected clean rule')
        self.assertEqual(expected, obj.get_raw(2), 'unexpected raw rule')


class NetworkCoveredTest(AATest):
    def _run_test(self, param, expected):
        obj = NetworkRule.parse(self.rule)
        check_obj = NetworkRule.parse(param)

        self.assertTrue(NetworkRule.match(param))

        self.assertEqual(obj.is_equal(check_obj), expected[0], 'Mismatch in is_equal, expected %s' % expected[0])
        self.assertEqual(obj.is_equal(check_obj, True), expected[1], 'Mismatch in is_equal/strict, expected %s' % expected[1])

        self.assertEqual(obj.is_covered(check_obj), expected[2], 'Mismatch in is_covered, expected %s' % expected[2])
        self.assertEqual(obj.is_covered(check_obj, True, True), expected[3], 'Mismatch in is_covered/exact, expected %s' % expected[3])

class NetworkCoveredTest_01(NetworkCoveredTest):
    rule = 'network inet,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        ('network,'                         , [ False   , False         , False     , False     ]),
        ('network inet,'                    , [ True    , True          , True      , True      ]),
        ('network inet, # comment'          , [ True    , False         , True      , True      ]),
        ('allow network inet,'              , [ True    , False         , True      , True      ]),
        ('network     inet,'                , [ True    , False         , True      , True      ]),
        ('network inet stream,'             , [ False   , False         , True      , True      ]),
        ('network inet tcp,'                , [ False   , False         , True      , True      ]),
        ('audit network inet,'              , [ False   , False         , False     , False     ]),
        ('audit network,'                   , [ False   , False         , False     , False     ]),
        ('network unix,'                    , [ False   , False         , False     , False     ]),
        ('network tcp,'                     , [ False   , False         , False     , False     ]),
        ('audit deny network inet,'         , [ False   , False         , False     , False     ]),
        ('deny network inet,'               , [ False   , False         , False     , False     ]),
    ]

class NetworkCoveredTest_02(NetworkCoveredTest):
    rule = 'audit network inet,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'network inet,'              , [ False   , False         , True      , False     ]),
        ('audit network inet,'              , [ True    , True          , True      , True      ]),
        (      'network inet stream,'       , [ False   , False         , True      , False     ]),
        ('audit network inet stream,'       , [ False   , False         , True      , True      ]),
        (      'network,'                   , [ False   , False         , False     , False     ]),
        ('audit network,'                   , [ False   , False         , False     , False     ]),
        ('network unix,'                    , [ False   , False         , False     , False     ]),
    ]


class NetworkCoveredTest_03(NetworkCoveredTest):
    rule = 'network inet stream,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'network inet stream,'       , [ True    , True          , True      , True      ]),
        ('allow network inet stream,'       , [ True    , False         , True      , True      ]),
        (      'network inet,'              , [ False   , False         , False     , False     ]),
        (      'network,'                   , [ False   , False         , False     , False     ]),
        (      'network inet tcp,'          , [ False   , False         , False     , False     ]),
        ('audit network,'                   , [ False   , False         , False     , False     ]),
        ('audit network inet stream,'       , [ False   , False         , False     , False     ]),
        (      'network unix,'              , [ False   , False         , False     , False     ]),
        (      'network,'                   , [ False   , False         , False     , False     ]),
    ]

class NetworkCoveredTest_04(NetworkCoveredTest):
    rule = 'network,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'network,'                   , [ True    , True          , True      , True      ]),
        ('allow network,'                   , [ True    , False         , True      , True      ]),
        (      'network inet,'              , [ False   , False         , True      , True      ]),
        (      'network inet6 stream,'      , [ False   , False         , True      , True      ]),
        (      'network tcp,'               , [ False   , False         , True      , True      ]),
        (      'network inet raw,'          , [ False   , False         , True      , True      ]),
        ('audit network,'                   , [ False   , False         , False     , False     ]),
        ('deny  network,'                   , [ False   , False         , False     , False     ]),
    ]

class NetworkCoveredTest_05(NetworkCoveredTest):
    rule = 'deny network inet,'

    tests = [
        #   rule                                equal     strict equal    covered     covered exact
        (      'deny network inet,'         , [ True    , True          , True      , True      ]),
        ('audit deny network inet,'         , [ False   , False         , False     , False     ]),
        (           'network inet,'         , [ False   , False         , False     , False     ]), # XXX should covered be true here?
        (      'deny network unix,'         , [ False   , False         , False     , False     ]),
        (      'deny network,'              , [ False   , False         , False     , False     ]),
    ]


class NetworkCoveredTest_Invalid(AATest):
    def test_borked_obj_is_covered_1(self):
        obj = NetworkRule.parse('network inet,')

        testobj = NetworkRule('inet', 'stream')
        testobj.domain = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_borked_obj_is_covered_2(self):
        obj = NetworkRule.parse('network inet,')

        testobj = NetworkRule('inet', 'stream')
        testobj.type_or_protocol = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_covered(self):
        obj = NetworkRule.parse('network inet,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_equal(self):
        obj = NetworkRule.parse('network inet,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_equal(testobj)

class NetworkLogprofHeaderTest(AATest):
    tests = [
        ('network,',                    [                               _('Network Family'), _('ALL'),     _('Socket Type'), _('ALL'),     ]),
        ('network inet,',               [                               _('Network Family'), 'inet',       _('Socket Type'), _('ALL'),     ]),
        ('network inet stream,',        [                               _('Network Family'), 'inet',       _('Socket Type'), 'stream',     ]),
        ('deny network,',               [_('Qualifier'), 'deny',        _('Network Family'), _('ALL'),     _('Socket Type'), _('ALL'),     ]),
        ('allow network inet,',         [_('Qualifier'), 'allow',       _('Network Family'), 'inet',       _('Socket Type'), _('ALL'),     ]),
        ('audit network inet stream,',  [_('Qualifier'), 'audit',       _('Network Family'), 'inet',       _('Socket Type'), 'stream',     ]),
        ('audit deny network inet,',    [_('Qualifier'), 'audit deny',  _('Network Family'), 'inet',       _('Socket Type'), _('ALL'),     ]),
    ]

    def _run_test(self, params, expected):
        obj = NetworkRule._parse(params)
        self.assertEqual(obj.logprof_header(), expected)

class NetworkRuleReprTest(AATest):
    tests = [
        (NetworkRule('inet', 'stream'),                             '<NetworkRule> network inet stream,'),
        (NetworkRule.parse(' allow  network  inet  stream, # foo'), '<NetworkRule> allow  network  inet  stream, # foo'),
    ]
    def _run_test(self, params, expected):
        self.assertEqual(str(params), expected)


## --- tests for NetworkRuleset --- #

class NetworkRulesTest(AATest):
    def test_empty_ruleset(self):
        ruleset = NetworkRuleset()
        ruleset_2 = NetworkRuleset()
        self.assertEqual([], ruleset.get_raw(2))
        self.assertEqual([], ruleset.get_clean(2))
        self.assertEqual([], ruleset_2.get_raw(2))
        self.assertEqual([], ruleset_2.get_clean(2))

    def test_ruleset_1(self):
        ruleset = NetworkRuleset()
        rules = [
            'network tcp,',
            'network inet,',
        ]

        expected_raw = [
            'network tcp,',
            'network inet,',
            '',
        ]

        expected_clean = [
            'network inet,',
            'network tcp,',
            '',
        ]

        for rule in rules:
            ruleset.add(NetworkRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw())
        self.assertEqual(expected_clean, ruleset.get_clean())

    def test_ruleset_2(self):
        ruleset = NetworkRuleset()
        rules = [
            'network inet6 raw,',
            'allow network inet,',
            'deny network udp, # example comment',
        ]

        expected_raw = [
            '  network inet6 raw,',
            '  allow network inet,',
            '  deny network udp, # example comment',
            '',
        ]

        expected_clean = [
            '  deny network udp, # example comment',
            '',
            '  allow network inet,',
            '  network inet6 raw,',
            '',
        ]

        for rule in rules:
            ruleset.add(NetworkRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw(1))
        self.assertEqual(expected_clean, ruleset.get_clean(1))


class NetworkGlobTestAATest(AATest):
    def setUp(self):
        self.maxDiff = None
        self.ruleset = NetworkRuleset()

    def test_glob_1(self):
        self.assertEqual(self.ruleset.get_glob('network inet,'), 'network,')

    # not supported or used yet
    # def test_glob_2(self):
    #     self.assertEqual(self.ruleset.get_glob('network inet raw,'), 'network inet,')

    def test_glob_ext(self):
        with self.assertRaises(NotImplementedError):
            # get_glob_ext is not available for network rules
            self.ruleset.get_glob_ext('network inet raw,')

class NetworkDeleteTestAATest(AATest):
    pass

class NetworkRulesetReprTest(AATest):
    def test_network_ruleset_repr(self):
        obj = NetworkRuleset()
        obj.add(NetworkRule('inet', 'stream'))
        obj.add(NetworkRule.parse(' allow  network  inet  stream, # foo'))

        expected = '<NetworkRuleset>\n  network inet stream,\n  allow  network  inet  stream, # foo\n</NetworkRuleset>'
        self.assertEqual(str(obj), expected)



setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
