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

from apparmor.rule.change_profile import ChangeProfileRule, ChangeProfileRuleset
from apparmor.rule import BaseRule
from apparmor.common import AppArmorException, AppArmorBug
from apparmor.logparser import ReadLog
from apparmor.translations import init_translation
_ = init_translation()

exp = namedtuple('exp', ['audit', 'allow_keyword', 'deny', 'comment',
        'execmode', 'execcond', 'all_execconds', 'targetprofile', 'all_targetprofiles'])

# --- tests for single ChangeProfileRule --- #

class ChangeProfileTest(AATest):
    def _compare_obj(self, obj, expected):
        self.assertEqual(expected.allow_keyword, obj.allow_keyword)
        self.assertEqual(expected.audit, obj.audit)
        self.assertEqual(expected.execmode, obj.execmode)
        self.assertEqual(expected.execcond, obj.execcond)
        self.assertEqual(expected.targetprofile, obj.targetprofile)
        self.assertEqual(expected.all_execconds, obj.all_execconds)
        self.assertEqual(expected.all_targetprofiles, obj.all_targetprofiles)
        self.assertEqual(expected.deny, obj.deny)
        self.assertEqual(expected.comment, obj.comment)

class ChangeProfileTestParse(ChangeProfileTest):
    tests = [
        # rawrule                                            audit  allow  deny   comment        execmode    execcond  all?   targetprof  all?
        ('change_profile,'                             , exp(False, False, False, ''           , None      , None  ,   True , None     , True )),
        ('change_profile /foo,'                        , exp(False, False, False, ''           , None      , '/foo',   False, None     , True )),
        ('change_profile safe /foo,'                   , exp(False, False, False, ''           , 'safe'    , '/foo',   False, None     , True )),
        ('change_profile unsafe /foo,'                 , exp(False, False, False, ''           , 'unsafe'  , '/foo',   False, None     , True )),
        ('change_profile /foo -> /bar,'                , exp(False, False, False, ''           , None      , '/foo',   False, '/bar'   , False)),
        ('change_profile safe /foo -> /bar,'           , exp(False, False, False, ''           , 'safe'    , '/foo',   False, '/bar'   , False)),
        ('change_profile unsafe /foo -> /bar,'         , exp(False, False, False, ''           , 'unsafe'  , '/foo',   False, '/bar'   , False)),
        ('deny change_profile /foo -> /bar, # comment' , exp(False, False, True , ' # comment' , None      , '/foo',   False, '/bar'   , False)),
        ('audit allow change_profile safe /foo,'       , exp(True , True , False, ''           , 'safe'    , '/foo',   False, None     , True )),
        ('change_profile -> /bar,'                     , exp(False, False, False, ''           , None      , None  ,   True , '/bar'   , False)),
        ('audit allow change_profile -> /bar,'         , exp(True , True , False, ''           , None      , None  ,   True , '/bar'   , False)),
        # quoted versions
        ('change_profile "/foo",'                      , exp(False, False, False, ''           , None      , '/foo',   False, None     , True )),
        ('change_profile "/foo" -> "/bar",'            , exp(False, False, False, ''           , None      , '/foo',   False, '/bar'   , False)),
        ('deny change_profile "/foo" -> "/bar", # cmt' , exp(False, False, True, ' # cmt'      , None      , '/foo',   False, '/bar'   , False)),
        ('audit allow change_profile "/foo",'          , exp(True , True , False, ''           , None      , '/foo',   False, None     , True )),
        ('change_profile -> "/bar",'                   , exp(False, False, False, ''           , None      , None  ,   True , '/bar'   , False)),
        ('audit allow change_profile -> "/bar",'       , exp(True , True , False, ''           , None      , None  ,   True , '/bar'   , False)),
        # with globbing and/or named profiles
        ('change_profile,'                             , exp(False, False, False, ''           , None      , None  ,   True , None     , True )),
        ('change_profile /*,'                          , exp(False, False, False, ''           , None      , '/*'  ,   False, None     , True )),
        ('change_profile /* -> bar,'                   , exp(False, False, False, ''           , None      , '/*'  ,   False, 'bar'    , False)),
        ('deny change_profile /** -> bar, # comment'   , exp(False, False, True , ' # comment' , None      , '/**' ,   False, 'bar'    , False)),
        ('audit allow change_profile /**,'             , exp(True , True , False, ''           , None      , '/**' ,   False, None     , True )),
        ('change_profile -> "ba r",'                   , exp(False, False, False, ''           , None      , None  ,   True , 'ba r'   , False)),
        ('audit allow change_profile -> "ba r",'       , exp(True , True , False, ''           , None      , None  ,   True , 'ba r'   , False)),
     ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(ChangeProfileRule.match(rawrule))
        obj = ChangeProfileRule.parse(rawrule)
        self.assertEqual(rawrule.strip(), obj.raw_rule)
        self._compare_obj(obj, expected)

class ChangeProfileTestParseInvalid(ChangeProfileTest):
    tests = [
        ('change_profile -> ,'                     , AppArmorException),
        ('change_profile foo -> ,'                 , AppArmorException),
        ('change_profile notsafe,'                 , AppArmorException),
        ('change_profile safety -> /bar,'          , AppArmorException),
    ]

    def _run_test(self, rawrule, expected):
        self.assertFalse(ChangeProfileRule.match(rawrule))
        with self.assertRaises(expected):
            ChangeProfileRule.parse(rawrule)

class ChangeProfileTestParseFromLog(ChangeProfileTest):
    def test_change_profile_from_log(self):
        parser = ReadLog('', '', '', '')

        event = 'type=AVC msg=audit(1428699242.551:386): apparmor="DENIED" operation="change_profile" profile="/foo/changeprofile" pid=3459 comm="changeprofile" target="/foo/rename"'

        # libapparmor doesn't understand this log format (from JJ)
        # event = '[   97.492562] audit: type=1400 audit(1431116353.523:77): apparmor="DENIED" operation="change_profile" profile="/foo/changeprofile" pid=3459 comm="changeprofile" target="/foo/rename"'

        parsed_event = parser.parse_event(event)

        self.assertEqual(parsed_event, {
            'request_mask': None,
            'denied_mask': None,
            'error_code': 0,
            'magic_token': 0,
            'parent': 0,
            'profile': '/foo/changeprofile',
            'operation': 'change_profile',
            'resource': None,
            'info': None,
            'aamode': 'REJECTING',
            'time': 1428699242,
            'active_hat': None,
            'pid': 3459,
            'task': 0,
            'attr': None,
            'name2': '/foo/rename', # target
            'name': None,
            'family': None,
            'protocol': None,
            'sock_type': None,
        })

        obj = ChangeProfileRule(None, ChangeProfileRule.ALL, parsed_event['name2'], log_event=parsed_event)

        #              audit  allow  deny   comment        execmode execcond  all?   targetprof     all?
        expected = exp(False, False, False, ''           , None,    None,     True,  '/foo/rename', False)

        self._compare_obj(obj, expected)

        self.assertEqual(obj.get_raw(1), '  change_profile -> /foo/rename,')


class ChangeProfileFromInit(ChangeProfileTest):
    tests = [
        # ChangeProfileRule object                                             audit  allow  deny   comment        execmode execcond    all?   targetprof  all?
        (ChangeProfileRule(None    , '/foo', '/bar', deny=True)          , exp(False, False, True , ''           , None    , '/foo',   False, '/bar'    , False)),
        (ChangeProfileRule(None    , '/foo', '/bar')                     , exp(False, False, False, ''           , None    , '/foo',   False, '/bar'    , False)),
        (ChangeProfileRule('safe'  , '/foo', '/bar')                     , exp(False, False, False, ''           , 'safe'  , '/foo',   False, '/bar'    , False)),
        (ChangeProfileRule('unsafe', '/foo', '/bar')                     , exp(False, False, False, ''           , 'unsafe', '/foo',   False, '/bar'    , False)),
        (ChangeProfileRule(None    , '/foo', ChangeProfileRule.ALL)      , exp(False, False, False, ''           , None  , '/foo',   False,  None     , True )),
        (ChangeProfileRule(None    , ChangeProfileRule.ALL, '/bar')      , exp(False, False, False, ''           , None  , None  ,   True , '/bar'    , False)),
        (ChangeProfileRule(None    , ChangeProfileRule.ALL,
                             ChangeProfileRule.ALL)            , exp(False, False, False, ''           , None, None  ,   True , None      , True )),
    ]

    def _run_test(self, obj, expected):
        self._compare_obj(obj, expected)


class InvalidChangeProfileInit(AATest):
    tests = [
        # init params                     expected exception
        ([None    , '/foo', ''               ]    , AppArmorBug), # empty targetprofile
        ([None    , ''    , '/bar'           ]    , AppArmorBug), # empty execcond
        ([None    , '    ', '/bar'           ]    , AppArmorBug), # whitespace execcond
        ([None    , '/foo', '   '            ]    , AppArmorBug), # whitespace targetprofile
        ([None    , 'xyxy', '/bar'           ]    , AppArmorException), # invalid execcond
        ([None    , dict(), '/bar'           ]    , AppArmorBug), # wrong type for execcond
        ([None    , None  , '/bar'           ]    , AppArmorBug), # wrong type for execcond
        ([None    , '/foo', dict()           ]    , AppArmorBug), # wrong type for targetprofile
        ([None    , '/foo', None             ]    , AppArmorBug), # wrong type for targetprofile
        (['maybe' , '/foo', '/bar'           ]    , AppArmorBug), # invalid keyword for execmode
    ]

    def _run_test(self, params, expected):
        with self.assertRaises(expected):
            ChangeProfileRule(params[0], params[1], params[2])

    def test_missing_params_1(self):
        with self.assertRaises(TypeError):
            ChangeProfileRule()

    def test_missing_params_2(self):
        with self.assertRaises(TypeError):
            ChangeProfileRule('inet')


class InvalidChangeProfileTest(AATest):
    def _check_invalid_rawrule(self, rawrule):
        obj = None
        self.assertFalse(ChangeProfileRule.match(rawrule))
        with self.assertRaises(AppArmorException):
            obj = ChangeProfileRule(ChangeProfileRule.parse(rawrule))

        self.assertIsNone(obj, 'ChangeProfileRule handed back an object unexpectedly')

    def test_invalid_net_missing_comma(self):
        self._check_invalid_rawrule('change_profile')  # missing comma

    def test_invalid_net_non_ChangeProfileRule(self):
        self._check_invalid_rawrule('dbus,')  # not a change_profile rule

    def test_empty_net_data_1(self):
        obj = ChangeProfileRule(None, '/foo', '/bar')
        obj.execcond = ''
        # no execcond set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_net_data_2(self):
        obj = ChangeProfileRule(None, '/foo', '/bar')
        obj.targetprofile = ''
        # no targetprofile set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)


class WriteChangeProfileTestAATest(AATest):
    tests = [
        #  raw rule                                                      clean rule
        ('     change_profile         ,    # foo        '              , 'change_profile, # foo'),
        ('    audit     change_profile /foo,'                          , 'audit change_profile /foo,'),
        ('   deny change_profile         /foo      -> bar,# foo bar'   , 'deny change_profile /foo -> bar, # foo bar'),
        ('   deny change_profile         /foo      ,# foo bar'         , 'deny change_profile /foo, # foo bar'),
        ('   allow change_profile   ->    /bar     ,# foo bar'         , 'allow change_profile -> /bar, # foo bar'),
        ('   allow change_profile   unsafe  /** ->    /bar     ,# foo bar'     , 'allow change_profile unsafe /** -> /bar, # foo bar'),
        ('   allow change_profile   "/fo o" ->    "/b ar",'            , 'allow change_profile "/fo o" -> "/b ar",'),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(ChangeProfileRule.match(rawrule))
        obj = ChangeProfileRule.parse(rawrule)
        clean = obj.get_clean()
        raw = obj.get_raw()

        self.assertEqual(expected.strip(), clean, 'unexpected clean rule')
        self.assertEqual(rawrule.strip(), raw, 'unexpected raw rule')

    def test_write_manually(self):
        obj = ChangeProfileRule(None, '/foo', 'bar', allow_keyword=True)

        expected = '    allow change_profile /foo -> bar,'

        self.assertEqual(expected, obj.get_clean(2), 'unexpected clean rule')
        self.assertEqual(expected, obj.get_raw(2), 'unexpected raw rule')


class ChangeProfileCoveredTest(AATest):
    def _run_test(self, param, expected):
        obj = ChangeProfileRule.parse(self.rule)
        check_obj = ChangeProfileRule.parse(param)

        self.assertTrue(ChangeProfileRule.match(param))

        self.assertEqual(obj.is_equal(check_obj), expected[0], 'Mismatch in is_equal, expected %s' % expected[0])
        self.assertEqual(obj.is_equal(check_obj, True), expected[1], 'Mismatch in is_equal/strict, expected %s' % expected[1])

        self.assertEqual(obj.is_covered(check_obj), expected[2], 'Mismatch in is_covered, expected %s' % expected[2])
        self.assertEqual(obj.is_covered(check_obj, True, True), expected[3], 'Mismatch in is_covered/exact, expected %s' % expected[3])

class ChangeProfileCoveredTest_01(ChangeProfileCoveredTest):
    rule = 'change_profile /foo,'

    tests = [
        #   rule                                        equal     strict equal    covered     covered exact
        ('           change_profile,'               , [ False   , False         , False     , False     ]),
        ('           change_profile /foo,'          , [ True    , True          , True      , True      ]),
        ('           change_profile safe /foo,'     , [ True    , False         , True      , True      ]),
        ('           change_profile unsafe /foo,'   , [ False   , False         , False     , False     ]),
        ('           change_profile /foo, # comment', [ True    , False         , True      , True      ]),
        ('     allow change_profile /foo,'          , [ True    , False         , True      , True      ]),
        ('           change_profile     /foo,'      , [ True    , False         , True      , True      ]),
        ('           change_profile /foo -> /bar,'  , [ False   , False         , True      , True      ]),
        ('           change_profile /foo -> bar,'   , [ False   , False         , True      , True      ]),
        ('audit      change_profile /foo,'          , [ False   , False         , False     , False     ]),
        ('audit      change_profile,'               , [ False   , False         , False     , False     ]),
        ('           change_profile /asdf,'         , [ False   , False         , False     , False     ]),
        ('           change_profile -> /bar,'       , [ False   , False         , False     , False     ]),
        ('audit deny change_profile /foo,'          , [ False   , False         , False     , False     ]),
        ('      deny change_profile /foo,'          , [ False   , False         , False     , False     ]),
    ]

class ChangeProfileCoveredTest_02(ChangeProfileCoveredTest):
    rule = 'audit change_profile /foo,'

    tests = [
        #   rule                                       equal     strict equal    covered     covered exact
        (      'change_profile /foo,'              , [ False   , False         , True      , False     ]),
        ('audit change_profile /foo,'              , [ True    , True          , True      , True      ]),
        (      'change_profile /foo -> /bar,'      , [ False   , False         , True      , False     ]),
        (      'change_profile safe /foo -> /bar,' , [ False   , False         , True      , False     ]),
        ('audit change_profile /foo -> /bar,'      , [ False   , False         , True      , True      ]), # XXX is "covered exact" correct here?
        (      'change_profile,'                   , [ False   , False         , False     , False     ]),
        ('audit change_profile,'                   , [ False   , False         , False     , False     ]),
        ('      change_profile -> /bar,'           , [ False   , False         , False     , False     ]),
    ]


class ChangeProfileCoveredTest_03(ChangeProfileCoveredTest):
    rule = 'change_profile /foo -> /bar,'

    tests = [
        #   rule                                       equal     strict equal    covered     covered exact
        (      'change_profile /foo -> /bar,'      , [ True    , True          , True      , True      ]),
        ('allow change_profile /foo -> /bar,'      , [ True    , False         , True      , True      ]),
        (      'change_profile /foo,'              , [ False   , False         , False     , False     ]),
        (      'change_profile,'                   , [ False   , False         , False     , False     ]),
        (      'change_profile /foo -> /xyz,'      , [ False   , False         , False     , False     ]),
        ('audit change_profile,'                   , [ False   , False         , False     , False     ]),
        ('audit change_profile /foo -> /bar,'      , [ False   , False         , False     , False     ]),
        (      'change_profile      -> /bar,'      , [ False   , False         , False     , False     ]),
        (      'change_profile,'                   , [ False   , False         , False     , False     ]),
    ]

class ChangeProfileCoveredTest_04(ChangeProfileCoveredTest):
    rule = 'change_profile,'

    tests = [
        #   rule                                       equal     strict equal    covered     covered exact
        (      'change_profile,'                   , [ True    , True          , True      , True      ]),
        ('allow change_profile,'                   , [ True    , False         , True      , True      ]),
        (      'change_profile /foo,'              , [ False   , False         , True      , True      ]),
        (      'change_profile /xyz -> bar,'       , [ False   , False         , True      , True      ]),
        (      'change_profile -> /bar,'           , [ False   , False         , True      , True      ]),
        (      'change_profile /foo -> /bar,'      , [ False   , False         , True      , True      ]),
        ('audit change_profile,'                   , [ False   , False         , False     , False     ]),
        ('deny  change_profile,'                   , [ False   , False         , False     , False     ]),
    ]

class ChangeProfileCoveredTest_05(ChangeProfileCoveredTest):
    rule = 'deny change_profile /foo,'

    tests = [
        #   rule                                       equal     strict equal    covered     covered exact
        (      'deny change_profile /foo,'         , [ True    , True          , True      , True      ]),
        ('audit deny change_profile /foo,'         , [ False   , False         , False     , False     ]),
        (           'change_profile /foo,'         , [ False   , False         , False     , False     ]), # XXX should covered be true here?
        (      'deny change_profile /bar,'         , [ False   , False         , False     , False     ]),
        (      'deny change_profile,'              , [ False   , False         , False     , False     ]),
    ]

class ChangeProfileCoveredTest_06(ChangeProfileCoveredTest):
    rule = 'change_profile safe /foo,'

    tests = [
        #   rule                                       equal     strict equal    covered     covered exact
        (      'deny change_profile /foo,'         , [ False   , False         , False     , False     ]),
        ('audit deny change_profile /foo,'         , [ False   , False         , False     , False     ]),
        (           'change_profile /foo,'         , [ True    , False         , True      , True      ]),
        (      'deny change_profile /bar,'         , [ False   , False         , False     , False     ]),
        (      'deny change_profile,'              , [ False   , False         , False     , False     ]),
    ]

class ChangeProfileCoveredTest_Invalid(AATest):
    def test_borked_obj_is_covered_1(self):
        obj = ChangeProfileRule.parse('change_profile /foo,')

        testobj = ChangeProfileRule(None, '/foo', '/bar')
        testobj.execcond = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_borked_obj_is_covered_2(self):
        obj = ChangeProfileRule.parse('change_profile /foo,')

        testobj = ChangeProfileRule(None, '/foo', '/bar')
        testobj.targetprofile = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_covered(self):
        obj = ChangeProfileRule.parse('change_profile /foo,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_equal(self):
        obj = ChangeProfileRule.parse('change_profile -> /bar,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_equal(testobj)

class ChangeProfileLogprofHeaderTest(AATest):
    tests = [
        ('change_profile,',                         [                                                         _('Exec Condition'), _('ALL'),  _('Target Profile'), _('ALL'),   ]),
        ('change_profile -> /bin/ping,',            [                                                         _('Exec Condition'), _('ALL'),  _('Target Profile'), '/bin/ping',]),
        ('change_profile /bar -> /bin/bar,',        [                                                         _('Exec Condition'), '/bar',    _('Target Profile'), '/bin/bar', ]),
        ('change_profile safe /foo,',                    [                          _('Exec Mode'), 'safe',   _('Exec Condition'), '/foo',    _('Target Profile'), _('ALL'),   ]),
        ('audit change_profile -> /bin/ping,',      [_('Qualifier'), 'audit',                                 _('Exec Condition'), _('ALL'),  _('Target Profile'), '/bin/ping',]),
        ('deny change_profile /bar -> /bin/bar,',   [_('Qualifier'), 'deny',                                  _('Exec Condition'), '/bar',    _('Target Profile'), '/bin/bar', ]),
        ('allow change_profile unsafe /foo,',       [_('Qualifier'), 'allow',       _('Exec Mode'), 'unsafe', _('Exec Condition'), '/foo',    _('Target Profile'), _('ALL'),   ]),
        ('audit deny change_profile,',              [_('Qualifier'), 'audit deny',                            _('Exec Condition'), _('ALL'),  _('Target Profile'), _('ALL'),   ]),
    ]

    def _run_test(self, params, expected):
        obj = ChangeProfileRule._parse(params)
        self.assertEqual(obj.logprof_header(), expected)

# --- tests for ChangeProfileRuleset --- #

class ChangeProfileRulesTest(AATest):
    def test_empty_ruleset(self):
        ruleset = ChangeProfileRuleset()
        ruleset_2 = ChangeProfileRuleset()
        self.assertEqual([], ruleset.get_raw(2))
        self.assertEqual([], ruleset.get_clean(2))
        self.assertEqual([], ruleset_2.get_raw(2))
        self.assertEqual([], ruleset_2.get_clean(2))

    def test_ruleset_1(self):
        ruleset = ChangeProfileRuleset()
        rules = [
            'change_profile -> /bar,',
            'change_profile /foo,',
        ]

        expected_raw = [
            'change_profile -> /bar,',
            'change_profile /foo,',
            '',
        ]

        expected_clean = [
            'change_profile -> /bar,',
            'change_profile /foo,',
            '',
        ]

        for rule in rules:
            ruleset.add(ChangeProfileRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw())
        self.assertEqual(expected_clean, ruleset.get_clean())

    def test_ruleset_2(self):
        ruleset = ChangeProfileRuleset()
        rules = [
            'change_profile /foo -> /bar,',
            'allow change_profile /asdf,',
            'deny change_profile -> xy, # example comment',
        ]

        expected_raw = [
            '  change_profile /foo -> /bar,',
            '  allow change_profile /asdf,',
            '  deny change_profile -> xy, # example comment',
            '',
        ]

        expected_clean = [
            '  deny change_profile -> xy, # example comment',
            '',
            '  allow change_profile /asdf,',
            '  change_profile /foo -> /bar,',
            '',
        ]

        for rule in rules:
            ruleset.add(ChangeProfileRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw(1))
        self.assertEqual(expected_clean, ruleset.get_clean(1))


class ChangeProfileGlobTestAATest(AATest):
    def setUp(self):
        self.ruleset = ChangeProfileRuleset()

    def test_glob_1(self):
        self.assertEqual(self.ruleset.get_glob('change_profile /foo,'), 'change_profile,')

    # not supported or used yet, glob behaviour not decided yet
    # def test_glob_2(self):
    #     self.assertEqual(self.ruleset.get_glob('change_profile /foo -> /bar,'), 'change_profile -> /bar,')

    def test_glob_ext(self):
        with self.assertRaises(NotImplementedError):
            # get_glob_ext is not available for change_profile rules
            self.ruleset.get_glob_ext('change_profile /foo -> /bar,')

class ChangeProfileDeleteTestAATest(AATest):
    pass

setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
