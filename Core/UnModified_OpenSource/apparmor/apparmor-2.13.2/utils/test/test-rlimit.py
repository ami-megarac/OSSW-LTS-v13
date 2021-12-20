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

from apparmor.rule.rlimit import RlimitRule, RlimitRuleset, split_unit
from apparmor.rule import BaseRule
from apparmor.common import AppArmorException, AppArmorBug
#from apparmor.logparser import ReadLog
from apparmor.translations import init_translation
_ = init_translation()

exp = namedtuple('exp', ['audit', 'allow_keyword', 'deny', 'comment',
        'rlimit', 'value', 'all_values'])

# --- tests for single RlimitRule --- #

class RlimitTest(AATest):
    def _compare_obj(self, obj, expected):
        self.assertEqual(expected.allow_keyword, obj.allow_keyword)
        self.assertEqual(expected.audit, obj.audit)
        self.assertEqual(expected.rlimit, obj.rlimit)
        self.assertEqual(expected.value, obj.value)
        self.assertEqual(expected.all_values, obj.all_values)
        self.assertEqual(expected.deny, obj.deny)
        self.assertEqual(expected.comment, obj.comment)

class RlimitTestParse(RlimitTest):
    tests = [
        # rawrule                                    audit  allow  deny   comment         rlimit          value      all/infinity?
        ('set rlimit as <= 2047MB,'            , exp(False, False, False, ''           , 'as'           , '2047MB'      , False)),
        ('set rlimit as <= 2047 MB,'           , exp(False, False, False, ''           , 'as'           , '2047 MB'     , False)),
        ('set rlimit cpu <= 1024,'             , exp(False, False, False, ''           , 'cpu'          , '1024'        , False)),
        ('set rlimit stack <= 1024GB,'         , exp(False, False, False, ''           , 'stack'        , '1024GB'      , False)),
        ('set rlimit stack <= 1024 GB,'        , exp(False, False, False, ''           , 'stack'        , '1024 GB'     , False)),
        ('set rlimit rtprio <= 10, # comment'  , exp(False, False, False, ' # comment' , 'rtprio'       , '10'          , False)),
        ('set rlimit core <= 44444KB,'         , exp(False, False, False, ''           , 'core'         , '44444KB'     , False)),
        ('set rlimit core <= 44444 KB,'        , exp(False, False, False, ''           , 'core'         , '44444 KB'    , False)),
        ('set rlimit rttime <= 60ms,'          , exp(False, False, False, ''           , 'rttime'       , '60ms'        , False)),
        ('set rlimit cpu <= infinity,'         , exp(False, False, False, ''           , 'cpu'          , None          , True )),
        ('set rlimit nofile <= 256,'           , exp(False, False, False, ''           , 'nofile'       , '256'         , False)),
        ('set rlimit data <= 4095KB,'          , exp(False, False, False, ''           , 'data'         , '4095KB'      , False)),
        ('set rlimit cpu <= 12, # cmt'         , exp(False, False, False, ' # cmt'     , 'cpu'          , '12'          , False)),
        ('set rlimit ofile <= 1234,'           , exp(False, False, False, ''           , 'ofile'        , '1234'        , False)),
        ('set rlimit msgqueue <= 4444,'        , exp(False, False, False, ''           , 'msgqueue'     , '4444'        , False)),
        ('set rlimit nice <= -10,'             , exp(False, False, False, ''           , 'nice'         , '-10'         , False)),
        ('set rlimit rttime <= 60minutes,'     , exp(False, False, False, ''           , 'rttime'       , '60minutes'   , False)),
        ('set rlimit fsize <= 1023MB,'         , exp(False, False, False, ''           , 'fsize'        , '1023MB'      , False)),
        ('set rlimit nproc <= 1,'              , exp(False, False, False, ''           , 'nproc'        , '1'           , False)),
        ('set rlimit rss <= infinity, # cmt'   , exp(False, False, False, ' # cmt'     , 'rss'          , None          , True )),
        ('set rlimit memlock <= 10240,'        , exp(False, False, False, ''           , 'memlock'      , '10240'       , False)),
        ('set rlimit sigpending <= 42,'        , exp(False, False, False, ''           , 'sigpending'   , '42'          , False)),
     ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(RlimitRule.match(rawrule))
        obj = RlimitRule.parse(rawrule)
        self.assertEqual(rawrule.strip(), obj.raw_rule)
        self._compare_obj(obj, expected)

class RlimitTestParseInvalid(RlimitTest):
    tests = [
        ('set rlimit,'                      , AppArmorException), # missing parts
        ('set rlimit <= 5,'                 , AppArmorException),
        ('set rlimit cpu <= ,'              , AppArmorException),
        ('set rlimit cpu <= "",'            , AppArmorBug),       # evil quoting trick
        ('set rlimit foo <= 5,'             , AppArmorException), # unknown rlimit
        ('set rlimit rttime <= 60m,'        , AppArmorException), # 'm' could mean 'ms' or 'minutes'
        ('set rlimit cpu <= 20ms,'          , AppArmorException), # cpu doesn't support 'ms'...
        ('set rlimit cpu <= 20us,'          , AppArmorException), # ... or 'us'
        ('set rlimit nice <= 20MB,'         , AppArmorException), # invalid unit for this rlimit type
        ('set rlimit cpu <= 20MB,'          , AppArmorException),
        ('set rlimit data <= 20seconds,'    , AppArmorException),
        ('set rlimit locks <= 20seconds,'   , AppArmorException),
    ]

    def _run_test(self, rawrule, expected):
        #self.assertFalse(RlimitRule.match(rawrule))  # the main regex isn't very strict
        with self.assertRaises(expected):
            RlimitRule.parse(rawrule)

class RlimitTestParseFromLog(RlimitTest):
    pass
    # def test_net_from_log(self):
    #   parser = ReadLog('', '', '', '')

    #   event = 'type=AVC ...'

    #   parsed_event = parser.parse_event(event)

    #   self.assertEqual(parsed_event, {
    #       ...
    #   })

    #   obj = RlimitRule(RlimitRule.ALL, parsed_event['name2'], log_event=parsed_event)

    #   #              audit  allow  deny   comment        rlimit  value     all?
    #   expected = exp(False, False, False, ''           , None,   '/foo/rename', False)

    #   self._compare_obj(obj, expected)

    #   self.assertEqual(obj.get_raw(1), '  rlimit -> /foo/rename,')


class RlimitFromInit(RlimitTest):
    tests = [
        # RlimitRule object                                  audit  allow  deny   comment        rlimit         value     all/infinity?
        (RlimitRule('as', '2047MB')                     , exp(False, False, False, ''           , 'as'      ,   '2047MB'    , False)),
        (RlimitRule('as', '2047 MB')                    , exp(False, False, False, ''           , 'as'      ,   '2047 MB'   , False)),
        (RlimitRule('cpu', '1024')                      , exp(False, False, False, ''           , 'cpu'     ,   '1024'      , False)),
        (RlimitRule('rttime', '60minutes')              , exp(False, False, False, ''           , 'rttime'  ,    '60minutes', False)),
        (RlimitRule('nice', '-10')                      , exp(False, False, False, ''           , 'nice'    ,   '-10'       , False)),
        (RlimitRule('rss', RlimitRule.ALL)              , exp(False, False, False, ''           , 'rss'     ,   None        , True )),
    ]

    def _run_test(self, obj, expected):
        self._compare_obj(obj, expected)


class InvalidRlimitInit(AATest):
    tests = [
        # init params                     expected exception
        (['as'  , ''               ]    , AppArmorBug), # empty value
        ([''    , '1024'           ]    , AppArmorException), # empty rlimit
        (['    ', '1024'           ]    , AppArmorException), # whitespace rlimit
        (['as'  , '   '            ]    , AppArmorBug), # whitespace value
        (['xyxy', '1024'           ]    , AppArmorException), # invalid rlimit
        ([dict(), '1024'           ]    , AppArmorBug), # wrong type for rlimit
        ([None  , '1024'           ]    , AppArmorBug), # wrong type for rlimit
        (['as'  , dict()           ]    , AppArmorBug), # wrong type for value
        (['as'  , None             ]    , AppArmorBug), # wrong type for value
        (['cpu' , '100xy2'         ]    , AppArmorException), # invalid unit
    ]

    def _run_test(self, params, expected):
        with self.assertRaises(expected):
            RlimitRule(params[0], params[1])

    def test_missing_params_1(self):
        with self.assertRaises(TypeError):
            RlimitRule()

    def test_missing_params_2(self):
        with self.assertRaises(TypeError):
            RlimitRule('as')

    def test_allow_keyword(self):
        with self.assertRaises(AppArmorBug):
            RlimitRule('as', '1024MB', allow_keyword=True)

    def test_deny_keyword(self):
        with self.assertRaises(AppArmorBug):
            RlimitRule('as', '1024MB', deny=True)

    def test_audit_keyword(self):
        with self.assertRaises(AppArmorBug):
            RlimitRule('as', '1024MB', audit=True)


class InvalidRlimitTest(AATest):
    def _check_invalid_rawrule(self, rawrule):
        obj = None
        self.assertFalse(RlimitRule.match(rawrule))
        with self.assertRaises(AppArmorException):
            obj = RlimitRule(RlimitRule.parse(rawrule))

        self.assertIsNone(obj, 'RlimitRule handed back an object unexpectedly')

    def test_invalid_net_missing_comma(self):
        self._check_invalid_rawrule('rlimit')  # missing comma

    def test_invalid_net_non_RlimitRule(self):
        self._check_invalid_rawrule('dbus,')  # not a rlimit rule

    def test_empty_net_data_1(self):
        obj = RlimitRule('as', '1024MB')
        obj.rlimit = ''
        # no rlimit set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_net_data_2(self):
        obj = RlimitRule('as', '1024MB')
        obj.value = ''
        # no value set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)


class WriteRlimitTest(AATest):
    tests = [
        #  raw rule                                                      clean rule
        ('     set rlimit    cpu   <= 1024     ,    # foo        '              , 'set rlimit cpu <= 1024, # foo'),
        ('        set     rlimit  stack <= 1024GB  ,'                          , 'set rlimit stack <= 1024GB,'),
        ('   set rlimit    rttime <= 100ms   ,   # foo bar'   , 'set rlimit rttime <= 100ms, # foo bar'),
        ('   set rlimit      cpu   <= infinity   ,  '         , 'set rlimit cpu <= infinity,'),
        ('    set rlimit     msgqueue <=   4444 ,   '         , 'set rlimit msgqueue <= 4444,'),
        ('    set rlimit   nice    <=  5   ,   # foo bar'     , 'set rlimit nice <= 5, # foo bar'),
        ('    set rlimit   nice <=   -5    , # cmt'           , 'set rlimit nice <= -5, # cmt'),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(RlimitRule.match(rawrule))
        obj = RlimitRule.parse(rawrule)
        clean = obj.get_clean()
        raw = obj.get_raw()

        self.assertEqual(expected.strip(), clean, 'unexpected clean rule')
        self.assertEqual(rawrule.strip(), raw, 'unexpected raw rule')

    def test_write_manually(self):
        obj = RlimitRule('as', '1024MB')

        expected = '    set rlimit as <= 1024MB,'

        self.assertEqual(expected, obj.get_clean(2), 'unexpected clean rule')
        self.assertEqual(expected, obj.get_raw(2), 'unexpected raw rule')


class RlimitCoveredTest(AATest):
    def _run_test(self, param, expected):
        obj = RlimitRule.parse(self.rule)
        check_obj = RlimitRule.parse(param)

        self.assertTrue(RlimitRule.match(param))

        self.assertEqual(obj.is_equal(check_obj), expected[0], 'Mismatch in is_equal, expected %s' % expected[0])
        self.assertEqual(obj.is_equal(check_obj, True), expected[1], 'Mismatch in is_equal/strict, expected %s' % expected[1])

        self.assertEqual(obj.is_covered(check_obj), expected[2], 'Mismatch in is_covered, expected %s' % expected[2])
        self.assertEqual(obj.is_covered(check_obj, True, True), expected[3], 'Mismatch in is_covered/exact, expected %s' % expected[3])

class RlimitCoveredTest_01(RlimitCoveredTest):
    rule = 'set rlimit cpu <= 150,'

    tests = [
        #   rule                            equal     strict equal    covered     covered exact
        ('set rlimit as <= 100MB,'      , [ False   , False         , False     , False     ]),
        ('set rlimit rttime <= 150,'    , [ False   , False         , False     , False     ]),
        ('set rlimit cpu <= 100,'       , [ False   , False         , True      , True      ]),
        ('set rlimit cpu <= 150,'       , [ True    , True          , True      , True      ]),
        ('set rlimit cpu <= 300,'       , [ False   , False         , False     , False     ]),
        ('set rlimit cpu <= 10seconds,' , [ False   , False         , True      , True      ]),
        ('set rlimit cpu <= 150seconds,', [ True    , False         , True      , True      ]),
        ('set rlimit cpu <= 300seconds,', [ False   , False         , False     , False     ]),
        ('set rlimit cpu <= 1minutes,'  , [ False   , False         , True      , True      ]),
        ('set rlimit cpu <= 1min,'      , [ False   , False         , True      , True      ]),
        ('set rlimit cpu <= 3minutes,'  , [ False   , False         , False     , False     ]),
        ('set rlimit cpu <= 1hour,'     , [ False   , False         , False     , False     ]),
        ('set rlimit cpu <= 2 days,'    , [ False   , False         , False     , False     ]),
        ('set rlimit cpu <= 1 week,'    , [ False   , False         , False     , False     ]),
    ]

class RlimitCoveredTest_02(RlimitCoveredTest):
    rule = 'set rlimit data <= 4MB,'

    tests = [
        #   rule                            equal     strict equal    covered     covered exact
        ('set rlimit data <= 100,'      , [ False   , False         , True      , True      ]),
        ('set rlimit data <= 2KB,'      , [ False   , False         , True      , True      ]),
        ('set rlimit data <= 2MB,'      , [ False   , False         , True      , True      ]),
        ('set rlimit data <= 4194304,'  , [ True    , False         , True      , True      ]),
        ('set rlimit data <= 4096KB,'   , [ True    , False         , True      , True      ]),
        ('set rlimit data <= 4MB,'      , [ True    , True          , True      , True      ]),
        ('set rlimit data <= 4 MB,'     , [ True    , False         , True      , True      ]),
        ('set rlimit data <= 6MB,'      , [ False   , False         , False     , False     ]),
        ('set rlimit data <= 6 MB,'     , [ False   , False         , False     , False     ]),
        ('set rlimit data <= 1GB,'      , [ False   , False         , False     , False     ]),
    ]

class RlimitCoveredTest_03(RlimitCoveredTest):
    rule = 'set rlimit nice <= -1,'

    tests = [
        #   rule                            equal     strict equal    covered     covered exact
        ('set rlimit nice <= 5,'        , [ False   , False         , True      , True      ]),
        ('set rlimit nice <= 0,'        , [ False   , False         , True      , True      ]),
        ('set rlimit nice <= -1,'       , [ True    , True          , True      , True      ]),
        ('set rlimit nice <= -3,'       , [ False   , False         , False     , False     ]),
    ]

class RlimitCoveredTest_04(RlimitCoveredTest):
    rule = 'set rlimit locks <= 42,'

    tests = [
        #   rule                            equal     strict equal    covered     covered exact
        ('set rlimit locks <= 20,'      , [ False   , False         , True      , True      ]),
        ('set rlimit locks <= 40,'      , [ False   , False         , True      , True      ]),
        ('set rlimit locks <= 42,'      , [ True    , True          , True      , True      ]),
        ('set rlimit locks <= 60,'      , [ False   , False         , False     , False     ]),
        ('set rlimit locks <= infinity,', [ False   , False         , False     , False     ]),
    ]

class RlimitCoveredTest_05(RlimitCoveredTest):
    rule = 'set rlimit locks <= infinity,'

    tests = [
        #   rule                            equal     strict equal    covered     covered exact
        ('set rlimit locks <= 20,'      , [ False   , False         , True      , True      ]),
        ('set rlimit cpu <= 40,'        , [ False   , False         , False     , False     ]),
        ('set rlimit locks <= infinity,', [ True    , True          , True      , True      ]),
    ]

class RlimitCoveredTest_Invalid(AATest):
    def test_borked_obj_is_covered_1(self):
        obj = RlimitRule.parse('set rlimit cpu <= 1024,')

        testobj = RlimitRule('cpu', '1024')
        testobj.rlimit = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_borked_obj_is_covered_2(self):
        obj = RlimitRule.parse('set rlimit cpu <= 1024,')

        testobj = RlimitRule('cpu', '1024')
        testobj.value = ''

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_covered(self):
        obj = RlimitRule.parse('set rlimit cpu <= 1024,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_equal(self):
        obj = RlimitRule.parse('set rlimit cpu <= 1024,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_equal(testobj)

class RlimitLogprofHeaderTest(AATest):
    tests = [
        ('set rlimit cpu <= infinity,',         [_('Rlimit'), 'cpu',     _('Value'), 'infinity', ]),
        ('set rlimit as <= 200MB,',             [_('Rlimit'), 'as',      _('Value'), '200MB',    ]),
        ('set rlimit rttime <= 200ms,',         [_('Rlimit'), 'rttime',  _('Value'), '200ms',    ]),
        ('set rlimit nproc <= 1,',              [_('Rlimit'), 'nproc',   _('Value'), '1',        ]),
    ]

    def _run_test(self, params, expected):
        obj = RlimitRule._parse(params)
        self.assertEqual(obj.logprof_header(), expected)

# --- tests for RlimitRuleset --- #

class RlimitRulesTest(AATest):
    def test_empty_ruleset(self):
        ruleset = RlimitRuleset()
        ruleset_2 = RlimitRuleset()
        self.assertEqual([], ruleset.get_raw(2))
        self.assertEqual([], ruleset.get_clean(2))
        self.assertEqual([], ruleset_2.get_raw(2))
        self.assertEqual([], ruleset_2.get_clean(2))

    def test_ruleset_1(self):
        ruleset = RlimitRuleset()
        rules = [
            '  set rlimit cpu  <= 100,',
            '  set rlimit as   <= 50MB,',
        ]

        expected_raw = [
            'set rlimit cpu  <= 100,',
            'set rlimit as   <= 50MB,',
            '',
        ]

        expected_clean = [
            'set rlimit as <= 50MB,',
            'set rlimit cpu <= 100,',
            '',
        ]

        for rule in rules:
            ruleset.add(RlimitRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw())
        self.assertEqual(expected_clean, ruleset.get_clean())

class RlimitGlobTestAATest(AATest):
    def setUp(self):
        self.ruleset = RlimitRuleset()

    def test_glob_1(self):
        with self.assertRaises(AppArmorBug):
            self.ruleset.get_glob('set rlimit cpu <= 100,')

    # not supported or used yet, glob behaviour not decided yet
    # def test_glob_2(self):
    #     self.assertEqual(self.ruleset.get_glob('rlimit /foo -> /bar,'), 'rlimit -> /bar,')

    def test_glob_ext(self):
        with self.assertRaises(NotImplementedError):
            # get_glob_ext is not available for rlimit rules
            self.ruleset.get_glob_ext('set rlimit cpu <= 100,')

class RlimitDeleteTestAATest(AATest):
    # no de-duplication tests for rlimit - de-duplication consists of is_covered() (which we already test) and
    # code from BaseRuleset (which is already tested in the capability, network and change_profile tests).
    # Therefore no need to test it once more ;-)
    pass


# --- other tests --- #
class RlimitSplit_unitTest(AATest):
    tests = [
        ('40MB'  , ( 40, 'MB',)),
        ('40 MB' , ( 40, 'MB',)),
        ('40'  ,   ( 40, '',  )),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(split_unit(params), expected)

    def test_invalid_split_unit(self):
        with self.assertRaises(AppArmorBug):
            split_unit('MB')

class RlimitSize_to_intTest(AATest):
    def AASetup(self):
        self.obj = RlimitRule('cpu', '1')

    tests = [
        ('40GB'  , 40 * 1024 * 1024 * 1024),
        ('40MB'  , 41943040),
        ('40KB'  , 40960),
        ('40'    , 40),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(self.obj.size_to_int(params), expected)

    def test_invalid_size_to_int_01(self):
        with self.assertRaises(AppArmorException):
            self.obj.size_to_int('20mice')

class RlimitTime_to_intTest(AATest):
    def AASetup(self):
        self.obj = RlimitRule('cpu', '1')

    tests = [
        ('40'       ,       0.00004),
        ('30us'     ,       0.00003),
        ('40ms'     ,       0.04),
        ('40seconds',      40),
        ('2minutes' ,    2*60),
        ('2hours'   , 2*60*60),
        ('1 day'    , 1*60*60*24),
        ('2 weeks'  , 2*60*60*24*7),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(self.obj.time_to_int(params, 'us'), expected)

    def test_with_seconds_as_default(self):
        self.assertEqual(self.obj.time_to_int('40', 'seconds'), 40)

    def test_with_ms_as_default(self):
        self.assertEqual(self.obj.time_to_int('40', 'ms'), 0.04)

    def test_with_us_as_default(self):
        self.assertEqual(self.obj.time_to_int('30', 'us'), 0.00003)

    def test_invalid_time_to_int(self):
        with self.assertRaises(AppArmorException):
            self.obj.time_to_int('20mice', 'seconds')



setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
