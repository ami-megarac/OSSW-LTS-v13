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

from apparmor.rule.dbus import DbusRule, DbusRuleset
from apparmor.rule import BaseRule
from apparmor.common import AppArmorException, AppArmorBug
from apparmor.logparser import ReadLog
from apparmor.translations import init_translation
_ = init_translation()

exp = namedtuple('exp', ['audit', 'allow_keyword', 'deny', 'comment',
        'access', 'all_access', 'bus', 'all_buses', 'path', 'all_paths', 'name', 'all_names', 'interface', 'all_interfaces', 'member', 'all_members', 'peername', 'all_peernames', 'peerlabel', 'all_peerlabels'])

# --- tests for single DbusRule --- #

class DbusTest(AATest):
    def _compare_obj(self, obj, expected):
        self.assertEqual(obj.allow_keyword, expected.allow_keyword)
        self.assertEqual(obj.audit, expected.audit)
        self.assertEqual(obj.deny, expected.deny)
        self.assertEqual(obj.comment, expected.comment)

        self.assertEqual(obj.access, expected.access)
        self._assertEqual_aare(obj.bus, expected.bus)
        self._assertEqual_aare(obj.path, expected.path)
        self._assertEqual_aare(obj.name, expected.name)
        self._assertEqual_aare(obj.interface, expected.interface)
        self._assertEqual_aare(obj.member, expected.member)
        self._assertEqual_aare(obj.peername, expected.peername)
        self._assertEqual_aare(obj.peerlabel, expected.peerlabel)

        self.assertEqual(obj.all_access, expected.all_access)
        self.assertEqual(obj.all_buses, expected.all_buses)
        self.assertEqual(obj.all_paths, expected.all_paths)
        self.assertEqual(obj.all_names, expected.all_names)
        self.assertEqual(obj.all_interfaces, expected.all_interfaces)
        self.assertEqual(obj.all_members, expected.all_members)
        self.assertEqual(obj.all_peernames, expected.all_peernames)
        self.assertEqual(obj.all_peerlabels, expected.all_peerlabels)

    def _assertEqual_aare(self, obj, expected):
        if obj:
            self.assertEqual(obj.regex, expected)
        else:
            self.assertEqual(obj, expected)

class DbusTestParse(DbusTest):
    tests = [
        # DbusRule object                                     audit  allow  deny   comment    access                all?    bus         all?    path            all?    name        all?    interface   all?    member  all?    peername    all?    peerlabel   all?
        ('dbus,'                                        , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus (   ),'                                  , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus ( , ),'                                  , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus send,'                                   , exp(False, False, False, '',        {'send'},             False,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus (send, receive),'                        , exp(False, False, False, '',        {'send', 'receive'},  False,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus send bus=session,'                       , exp(False, False, False, '',        {'send'},             False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('deny dbus send bus="session", # cmt'          , exp(False, False, True , ' # cmt',  {'send'},             False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('audit allow dbus peer=(label=foo),'           , exp(True , True , False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   'foo',      False)),
        ('dbus bus=session path=/foo/bar,'              , exp(False, False, False, '',        None  ,               True ,  'session',  False,  '/foo/bar',     False,  None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus send bus=(session),'                     , exp(False, False, False, '',        {'send'},             False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus name=(SomeService),'                     , exp(False, False, False, '',        None,                 True,   None,       True,   None,           True,  'SomeService',False, None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus send bus=session peer=(label="foo"),'    , exp(False, False, False, '',        {'send'},             False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   'foo',      False)),
        ('dbus send  bus = ( session ) , '              , exp(False, False, False, '',        {'send'},             False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus path=/foo,'                              , exp(False, False, False, '',        None  ,               True ,   None,      True,   '/foo',         False,  None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus eavesdrop bus=session,'                  , exp(False, False, False, '',        {'eavesdrop'},        False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus peer=(name=foo label=bar),'              , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus peer=(  name = foo    label = bar  ),'   , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus peer=(  name = foo ,  label = bar  ),'   , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus peer=(, name = foo ,  label = bar ,),'   , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus peer=(  name = foo,   label = bar  ),'   , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo,',     False,  'bar',      False)),  # XXX peername includes the comma
        ('dbus peer=(label=bar name=foo),'              , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus peer=(  label = bar   name = foo  ),'    , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus peer=(, label = bar , name = foo ,),'    , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus peer=(, label = bar,  name = foo  ),'    , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar,',     False)),  # XXX peerlabel includes the comma
        ('dbus peer=(  label = bar , name = foo  ),'    , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus peer=(  label = "bar"  name = "foo" ),'  , exp(False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   None,       True,   None,   True,   'foo',      False,  'bar',      False)),
        ('dbus path=/foo/bar bus=session,'              , exp(False, False, False, '',        None  ,               True ,  'session',  False,  '/foo/bar',     False,  None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),
        ('dbus bus=system path=/foo/bar bus=session,'   , exp(False, False, False, '',        None  ,               True ,  'session',  False,  '/foo/bar',     False,  None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),  # XXX bus= specified twice, last one wins
        ('dbus send peer=(label="foo") bus=session,'    , exp(False, False, False, '',        {'send'},             False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   'foo',      False)),
        ('dbus bus=1 bus=2 bus=3 bus=4 bus=5 bus=6,'    , exp(False, False, False, '',        None  ,               True ,  '6',        False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),  # XXX bus= specified multiple times, last one wins
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(DbusRule.match(rawrule))
        obj = DbusRule.parse(rawrule)
        self.assertEqual(rawrule.strip(), obj.raw_rule)
        self._compare_obj(obj, expected)

class DbusTestParseInvalid(DbusTest):
    tests = [
        ('dbus foo,'                     , AppArmorException),
        ('dbus foo bar,'                 , AppArmorException),
        ('dbus foo int,'                 , AppArmorException),
        ('dbus send bar,'                , AppArmorException),
        ('dbus send receive,'            , AppArmorException),
        ('dbus peer=,'                   , AppArmorException),
        ('dbus peer=(label=foo) path=,'  , AppArmorException),
        ('dbus (invalid),'               , AppArmorException),
        ('dbus peer=,'                   , AppArmorException),
        ('dbus bus=session bind bus=system,', AppArmorException),
        ('dbus bus=1 bus=2 bus=3 bus=4 bus=5 bus=6 bus=7,', AppArmorException),
    ]

    def _run_test(self, rawrule, expected):
        self.assertTrue(DbusRule.match(rawrule))  # the above invalid rules still match the main regex!
        with self.assertRaises(expected):
            DbusRule.parse(rawrule)

class DbusTestParseFromLog(DbusTest):
    def test_dbus_from_log(self):
        parser = ReadLog('', '', '', '')
        event = 'type=USER_AVC msg=audit(1375323372.644:157): pid=363 uid=102 auid=4294967295 ses=4294967295  msg=\'apparmor="DENIED" operation="dbus_method_call"  bus="system" name="org.freedesktop.DBus" path="/org/freedesktop/DBus" interface="org.freedesktop.DBus" member="Hello" mask="send" pid=2833 profile="/tmp/apparmor-2.8.0/tests/regression/apparmor/dbus_service" peer_profile="unconfined"  exe="/bin/dbus-daemon" sauid=102 hostname=? addr=? terminal=?\''

        parsed_event = parser.parse_event(event)

        self.assertEqual(parsed_event, {
            'request_mask': None,
            'denied_mask': 'send',
            'error_code': 0,
            'magic_token': 0,
            'parent': 0,
            'profile': '/tmp/apparmor-2.8.0/tests/regression/apparmor/dbus_service',
            'bus': 'system',
            'peer_profile': 'unconfined',
            'operation': 'dbus_method_call',
            'resource': None,
            'info': None,
            'aamode': 'REJECTING',
            'time': 1375323372,
            'active_hat': None,
            'pid': 2833,
            'task': 0,
            'attr': None,
            'name2': None,
            'name': 'org.freedesktop.DBus',
            'path': '/org/freedesktop/DBus',
            'interface': 'org.freedesktop.DBus',
            'member': 'Hello',
            'family': None,
            'protocol': None,
            'sock_type': None,
        })

# XXX send rules must not contain name conditional, but the log event includes it - how should we handle this in logparser.py?

#       #                            access                       bus                  path                  name                  interface                  member                  peername        peerlabel
#       obj = DbusRule(parsed_event['denied_mask'], parsed_event['bus'], parsed_event['path'], parsed_event['name'], parsed_event['interface'], parsed_event['member'], parsed_event['peer_profile'], DbusRule.ALL, log_event=parsed_event)

#       # DbusRule      audit  allow  deny   comment    access                all?    bus         all?    path            all?    name        all?    interface   all?    member  all?    peername    all?    peerlabel   all?
#       expected = exp( False, False, False, '',        {'send'},             False,  'system',   False, '/org/freedesktop/DBus', False,  'org.freedesktop.DBus', False, 'org.freedesktop.DBus', False,
#                                                                                                                                                                        'Hello', False, 'unconfined', False, None,      True)

#       self._compare_obj(obj, expected)

#       self.assertEqual(obj.get_raw(1), '  dbus send bus=system path=/org/freedesktop/DBus name=org.freedesktop.DBus member=Hello peer=(name=unconfined),')

class DbusFromInit(DbusTest):
    tests = [
        #DbusRule# access               bus             path            name            interface       member          peername        peerlabel   audit=, deny=, allow_keyword, comment=, log_event)
        (DbusRule( 'send' ,             'session',      DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL),
                    #exp# audit  allow  deny   comment    access                all?    bus         all?    path            all?    name        all?    interface   all?    member  all?    peername    all?    peerlabel   all?
                    exp(  False, False, False, '',        {'send'},             False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),

        #DbusRule# access               bus             path            name            interface       member          peername        peerlabel   audit=, deny=, allow_keyword, comment=, log_event)
        (DbusRule(('send', 'receive'),  'session',      DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL),
                    #exp# audit  allow  deny   comment    access                all?    bus         all?    path            all?    name        all?    interface   all?    member  all?    peername    all?    peerlabel   all?
                    exp(  False, False, False, '',        {'send', 'receive'},  False,  'session',  False,  None,           True,   None,       True,   None,       True,   None,   True,   None,       True,   None,       True)),

        #DbusRule# access               bus             path            name            interface       member          peername        peerlabel   audit=, deny=, allow_keyword, comment=, log_event)
        (DbusRule(DbusRule.ALL,         DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'),
                    #exp# audit  allow  deny   comment    access                all?    bus         all?    path            all?    name        all?    interface   all?    member      all?   peername      all?    peerlabel     all?
                    exp(  False, False, False, '',        None  ,               True ,  None,       True,   None,           True,   None,       True,   '/int/face',False,  '/mem/ber', False, '/peer/name', False, '/peer/label', False)),
    ]

    def _run_test(self, obj, expected):
        self._compare_obj(obj, expected)

class InvalidDbusInit(AATest):
    tests = [
        #         access        bus             path            name            interface       member          peername        peerlabel       expected exception

        # empty fields
        (        ('',           'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        ((),           'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, '',             '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '',             'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    '',             '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '',             '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '/int/face',    '',             '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '',             '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   ''           ), AppArmorBug),

        # whitespace fields
        (        ('  ',         'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, '  ',           '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '  ',           'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    '  ',           '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '  ',           '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '/int/face',    '  ',           '/peer/name',   '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '  ',           '/peer/label'), AppArmorBug),
        (        (DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '  '         ), AppArmorBug),

        # wrong type - dict()
        (        (dict(),       'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     dict(),         '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      dict(),         'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    dict(),         '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    'com.aa.test',  dict(),         '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face',    dict(),         '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     dict(),         '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   dict()       ), AppArmorBug),


        # wrong type - None
        (        (None,         'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        ((None),       'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     None,           '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      None,           'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    None,           '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    'com.aa.test',  None,           '/mem/ber',     '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face',    None,           '/peer/name',   '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     None,           '/peer/label'), AppArmorBug),
        (        (('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   None         ), AppArmorBug),

        # bind conflicts with path, interface, member, peer name and peer label
        (        (('bind'),     DbusRule.ALL,   '/org/test',    DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),
        (        (('bind'),     DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/int/face',    DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),
        (        (('bind'),     DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/mem/ber',     DbusRule.ALL,   DbusRule.ALL ), AppArmorException),
        (        (('bind'),     DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/peer/name',   DbusRule.ALL ), AppArmorException),
        (        (('bind'),     DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/peer/label'), AppArmorException),

        # eavesdrop conflcts with path, name, interface, member, peer name and peer label
        (        (('eavesdrop'),DbusRule.ALL,   '/org/test',    DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),
        (        (('eavesdrop'),DbusRule.ALL,   DbusRule.ALL,   'com.aa.test',  DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),
        (        (('eavesdrop'),DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/int/face',    DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),
        (        (('eavesdrop'),DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/mem/ber',     DbusRule.ALL,   DbusRule.ALL ), AppArmorException),
        (        (('eavesdrop'),DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/peer/name',   DbusRule.ALL ), AppArmorException),
        (        (('eavesdrop'),DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   '/peer/label'), AppArmorException),

        # send and receive conflict with name
        (        (('send'),     DbusRule.ALL,   DbusRule.ALL,   'com.aa.test',  DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),
        (        (('receive'),  DbusRule.ALL,   DbusRule.ALL,   'com.aa.test',  DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),

        # misc
        (        (DbusRule.ALL, DbusRule.ALL,   'foo/bar',      DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),  # path doesn't start with /
        (        (('foo'),      DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),  # invalid access keyword
        (        (('foo', 'send'), DbusRule.ALL, DbusRule.ALL,  DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL ), AppArmorException),  # valid + invalid access keyword
    ]

    def _run_test(self, params, expected):
        with self.assertRaises(expected):
            DbusRule(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7])

    def test_missing_params_1(self):
        with self.assertRaises(TypeError):
            DbusRule('send')

    def test_missing_params_2(self):
        with self.assertRaises(TypeError):
            DbusRule(('send'),     'session')

    def test_missing_params_3(self):
        with self.assertRaises(TypeError):
            DbusRule(('send'),     'session',      '/org/test')

    def test_missing_params_4(self):
        with self.assertRaises(TypeError):
            DbusRule(('send'),     'session',      '/org/test',    'com.aa.test')

    def test_missing_params_5(self):
        with self.assertRaises(TypeError):
            DbusRule(('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face')

    def test_missing_params_6(self):
        with self.assertRaises(TypeError):
            DbusRule(('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber')

    def test_missing_params_7(self):
        with self.assertRaises(TypeError):
            DbusRule(('send'),     'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name')

class InvalidDbusTest(AATest):
    def _check_invalid_rawrule(self, rawrule):
        obj = None
        self.assertFalse(DbusRule.match(rawrule))
        with self.assertRaises(AppArmorException):
            obj = DbusRule(DbusRule.parse(rawrule))

        self.assertIsNone(obj, 'DbusRule handed back an object unexpectedly')

    def test_invalid_dbus_missing_comma(self):
        self._check_invalid_rawrule('dbus')  # missing comma

    def test_invalid_non_DbusRule(self):
        self._check_invalid_rawrule('signal,')  # not a dbus rule

    def test_empty_data_1(self):
        #              access        bus             path            name            interface       member          peername        peerlabel       expected exception
        obj = DbusRule(('send'),     'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label')
        obj.access = ''
        # no access set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_2(self):
        obj = DbusRule(('send'),     'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label')
        obj.bus = ''
        # no bus set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_3(self):
        obj = DbusRule(('send'),     'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label')
        obj.path = ''
        # no path set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_4(self):
        obj = DbusRule(DbusRule.ALL, 'session',      '/org/test',    'com.aa.test',  '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label')
        obj.name = ''
        # no name set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_5(self):
        obj = DbusRule(('send'),     'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label')
        obj.interface = ''
        # no interface set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_6(self):
        obj = DbusRule(('send'),     'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label')
        obj.member = ''
        # no member set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_7(self):
        obj = DbusRule(('send'),     'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label')
        obj.peername = ''
        # no peername set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)

    def test_empty_data_8(self):
        obj = DbusRule(('send'),     'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label')
        obj.peerlabel = ''
        # no peerlabel set, and ALL not set
        with self.assertRaises(AppArmorBug):
            obj.get_clean(1)


class WriteDbusTest(AATest):
    def _run_test(self, rawrule, expected):
        self.assertTrue(DbusRule.match(rawrule), 'DbusRule.match() failed')
        obj = DbusRule.parse(rawrule)
        clean = obj.get_clean()
        raw = obj.get_raw()

        self.assertEqual(expected.strip(), clean, 'unexpected clean rule')
        self.assertEqual(rawrule.strip(), raw, 'unexpected raw rule')

    tests = [
        #  raw rule                                                           clean rule
        ('     dbus         ,    # foo        '                             , 'dbus, # foo'),
        ('    audit     dbus send,'                                         , 'audit dbus send,'),
        ('    audit     dbus (send  ),'                                     , 'audit dbus send,'),
        ('    audit     dbus (send , receive ),'                            , 'audit dbus (receive send),'),
        ('   deny dbus         send      bus=session,# foo bar'             , 'deny dbus send bus=session, # foo bar'),
        ('   deny dbus         send      bus=(session),  '                  , 'deny dbus send bus=session,'),
        ('   deny dbus         send      peer=(name=unconfined label=foo),' , 'deny dbus send peer=(name=unconfined label=foo),'),
        ('   deny dbus         send      interface = ( foo ),'              , 'deny dbus send interface=foo,'),
        ('   deny dbus         send      ,# foo bar'                        , 'deny dbus send, # foo bar'),
        ('   allow dbus             peer=(label=foo)    ,# foo bar'         , 'allow dbus peer=(label=foo), # foo bar'),
        ('dbus,'                                                            , 'dbus,'),
        ('dbus (receive),'                                                  , 'dbus receive,'),
        ('dbus (send),'                                                     , 'dbus send,'),
        ('dbus (send receive),'                                             , 'dbus (receive send),'),
        ('dbus receive,'                                                    , 'dbus receive,'),
        ('dbus eavesdrop,'                                                  , 'dbus eavesdrop,'),
        ('dbus bind bus = foo name = bar,'                                  , 'dbus bind bus=foo name=bar,'),
        ('dbus send peer=(  label = /foo ) ,'                               , 'dbus send peer=(label=/foo),'),
        ('dbus (receive) member=baz,'                                       , 'dbus receive member=baz,'),
        ('dbus send path = /foo,'                                           , 'dbus send path=/foo,'),
        ('dbus receive peer=(label=foo),'                                   , 'dbus receive peer=(label=foo),'),
        ('dbus (send receive) peer=(name=/usr/bin/bar),'                    , 'dbus (receive send) peer=(name=/usr/bin/bar),'),
        ('dbus (,  receive  ,,, send  ,) interface=/sbin/baz,'              , 'dbus (receive send) interface=/sbin/baz,'),  # XXX leading and trailing ',' inside (...) causes error
        # XXX add more complex rules
    ]

    def test_write_manually_1(self):
        #              access        bus             path            name            interface       member          peername        peerlabel       expected exception
        obj = DbusRule(('send'),     'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label', allow_keyword=True)

        expected = '    allow dbus send bus=session path=/org/test interface=/int/face member=/mem/ber peer=(name=/peer/name label=/peer/label),'

        self.assertEqual(expected, obj.get_clean(2), 'unexpected clean rule')
        self.assertEqual(expected, obj.get_raw(2), 'unexpected raw rule')

    def test_write_manually_2(self):
        #              access               bus             path            name            interface       member          peername        peerlabel       expected exception
        obj = DbusRule(('send', 'receive'), DbusRule.ALL,   '/org/test',    DbusRule.ALL,   DbusRule.ALL,   '/mem/ber',     '/peer/name',   DbusRule.ALL,  allow_keyword=True)

        expected = '    allow dbus (receive send) path=/org/test member=/mem/ber peer=(name=/peer/name),'

        self.assertEqual(expected, obj.get_clean(2), 'unexpected clean rule')
        self.assertEqual(expected, obj.get_raw(2), 'unexpected raw rule')


class DbusCoveredTest(AATest):
    def _run_test(self, param, expected):
        obj = DbusRule.parse(self.rule)
        check_obj = DbusRule.parse(param)

        self.assertTrue(DbusRule.match(param))

        self.assertEqual(obj.is_equal(check_obj), expected[0], 'Mismatch in is_equal, expected %s' % expected[0])
        self.assertEqual(obj.is_equal(check_obj, True), expected[1], 'Mismatch in is_equal/strict, expected %s' % expected[1])

        self.assertEqual(obj.is_covered(check_obj), expected[2], 'Mismatch in is_covered, expected %s' % expected[2])
        self.assertEqual(obj.is_covered(check_obj, True, True), expected[3], 'Mismatch in is_covered/exact, expected %s' % expected[3])

class DbusCoveredTest_01(DbusCoveredTest):
    rule = 'dbus send,'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        ('dbus,'                                        , [ False   , False         , False     , False     ]),
        ('dbus send,'                                   , [ True    , True          , True      , True      ]),
        ('dbus send member=unconfined,'                 , [ False   , False         , True      , True      ]),
        ('dbus send, # comment'                         , [ True    , False         , True      , True      ]),
        ('allow dbus send,'                             , [ True    , False         , True      , True      ]),
        ('dbus     send,'                               , [ True    , False         , True      , True      ]),
        ('dbus send bus=session,'                       , [ False   , False         , True      , True      ]),
        ('dbus send member=(label=foo),'                , [ False   , False         , True      , True      ]),
        ('audit dbus send,'                             , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('dbus receive,'                                , [ False   , False         , False     , False     ]),
        ('dbus member=(label=foo),'                     , [ False   , False         , False     , False     ]),
        ('audit deny dbus send,'                        , [ False   , False         , False     , False     ]),
        ('deny dbus send,'                              , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_02(DbusCoveredTest):
    rule = 'audit dbus send,'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        (      'dbus send,'                             , [ False   , False         , True      , False     ]),
        ('audit dbus send,'                             , [ True    , True          , True      , True      ]),
        (      'dbus send bus=session,'                 , [ False   , False         , True      , False     ]),
        ('audit dbus send bus=session,'                 , [ False   , False         , True      , True      ]),
        (      'dbus,'                                  , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('dbus receive,'                                , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_03(DbusCoveredTest):
    rule = 'dbus send bus=session,'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        (      'dbus send bus=session,'                 , [ True    , True          , True      , True      ]),
        ('allow dbus send bus=session,'                 , [ True    , False         , True      , True      ]),
        (      'dbus send,'                             , [ False   , False         , False     , False     ]),
        (      'dbus,'                                  , [ False   , False         , False     , False     ]),
        (      'dbus send member=(label=foo),'          , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('audit dbus send bus=session,'                 , [ False   , False         , False     , False     ]),
        ('audit dbus bus=session,'                      , [ False   , False         , False     , False     ]),
        (      'dbus send,'                             , [ False   , False         , False     , False     ]),
        (      'dbus,'                                  , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_04(DbusCoveredTest):
    rule = 'dbus,'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        (      'dbus,'                                  , [ True    , True          , True      , True      ]),
        ('allow dbus,'                                  , [ True    , False         , True      , True      ]),
        (      'dbus send,'                             , [ False   , False         , True      , True      ]),
        (      'dbus receive bus=session,'              , [ False   , False         , True      , True      ]),
        (      'dbus member=(label=foo),'               , [ False   , False         , True      , True      ]),
        (      'dbus send bus=session,'                 , [ False   , False         , True      , True      ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('deny  dbus,'                                  , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_05(DbusCoveredTest):
    rule = 'deny dbus send,'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        (      'deny dbus send,'                        , [ True    , True          , True      , True      ]),
        ('audit deny dbus send,'                        , [ False   , False         , False     , False     ]),
        (           'dbus send,'                        , [ False   , False         , False     , False     ]), # XXX should covered be true here?
        (      'deny dbus receive,'                     , [ False   , False         , False     , False     ]),
        (      'deny dbus,'                             , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_06(DbusCoveredTest):
    rule = 'dbus send peer=(name=unconfined),'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        ('dbus,'                                        , [ False   , False         , False     , False     ]),
        ('dbus send,'                                   , [ False   , False         , False     , False     ]),
        ('dbus send peer=(name=unconfined),'            , [ True    , True          , True      , True      ]),
        ('dbus peer=(name=unconfined),'                 , [ False   , False         , False     , False     ]),
        ('dbus send, # comment'                         , [ False   , False         , False     , False     ]),
        ('allow dbus send,'                             , [ False   , False         , False     , False     ]),
        ('allow dbus send peer=(name=unconfined),'      , [ True    , False         , True      , True      ]),
        ('allow dbus send peer=(name=/foo/bar),'        , [ False   , False         , False     , False     ]),
        ('allow dbus send peer=(name=/**),'             , [ False   , False         , False     , False     ]),
        ('allow dbus send peer=(name=**),'              , [ False   , False         , False     , False     ]),
        ('dbus    send,'                                , [ False   , False         , False     , False     ]),
        ('dbus    send peer=(name=unconfined),'         , [ True    , False         , True      , True      ]),
        ('dbus send bus=session,'                       , [ False   , False         , False     , False     ]),
        ('dbus send peer=(name=unconfined label=foo),'  , [ False   , False         , True      , True      ]),
        ('audit dbus send peer=(name=unconfined),'      , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('dbus receive,'                                , [ False   , False         , False     , False     ]),
        ('dbus peer=(label=foo),'                       , [ False   , False         , False     , False     ]),
        ('audit deny dbus send,'                        , [ False   , False         , False     , False     ]),
        ('deny dbus send,'                              , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_07(DbusCoveredTest):
    rule = 'dbus send peer=(label=unconfined),'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        ('dbus,'                                        , [ False   , False         , False     , False     ]),
        ('dbus send,'                                   , [ False   , False         , False     , False     ]),
        ('dbus send peer=(label=unconfined),'           , [ True    , True          , True      , True      ]),
        ('dbus peer=(label=unconfined),'                , [ False   , False         , False     , False     ]),
        ('dbus send, # comment'                         , [ False   , False         , False     , False     ]),
        ('allow dbus send,'                             , [ False   , False         , False     , False     ]),
        ('allow dbus send peer=(label=unconfined),'     , [ True    , False         , True      , True      ]),
        ('allow dbus send peer=(label=/foo/bar),'       , [ False   , False         , False     , False     ]),
        ('allow dbus send peer=(label=/**),'            , [ False   , False         , False     , False     ]),
        ('allow dbus send peer=(label=**),'             , [ False   , False         , False     , False     ]),
        ('dbus    send,'                                , [ False   , False         , False     , False     ]),
        ('dbus    send peer=(label=unconfined),'        , [ True    , False         , True      , True      ]),
        ('dbus send bus=session,'                       , [ False   , False         , False     , False     ]),
        ('dbus send peer=(label=unconfined name=foo),'  , [ False   , False         , True      , True      ]),
        ('audit dbus send peer=(label=unconfined),'     , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('dbus receive,'                                , [ False   , False         , False     , False     ]),
        ('dbus peer=(label=foo),'                       , [ False   , False         , False     , False     ]),
        ('audit deny dbus send,'                        , [ False   , False         , False     , False     ]),
        ('deny dbus send,'                              , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_08(DbusCoveredTest):
    rule = 'dbus send path=/foo/bar,'

    tests = [
        #   rule                                              equal     strict equal    covered     covered exact
        ('dbus,'                                        , [ False   , False         , False     , False     ]),
        ('dbus send,'                                   , [ False   , False         , False     , False     ]),
        ('dbus send path=/foo/bar,'                     , [ True    , True          , True      , True      ]),
        ('dbus send path=/foo/*,'                       , [ False   , False         , False     , False     ]),
        ('dbus send path=/**,'                          , [ False   , False         , False     , False     ]),
        ('dbus send path=/what/*,'                      , [ False   , False         , False     , False     ]),
        ('dbus path=/foo/bar,'                          , [ False   , False         , False     , False     ]),
        ('dbus send, # comment'                         , [ False   , False         , False     , False     ]),
        ('allow dbus send,'                             , [ False   , False         , False     , False     ]),
        ('allow dbus send path=/foo/bar,'               , [ True    , False         , True      , True      ]),
        ('dbus    send,'                                , [ False   , False         , False     , False     ]),
        ('dbus    send path=/foo/bar,'                  , [ True    , False         , True      , True      ]),
        ('dbus    send path=/what/ever,'                , [ False   , False         , False     , False     ]),
        ('dbus send bus=session,'                       , [ False   , False         , False     , False     ]),
        ('dbus send path=/foo/bar peer=(label=foo),'    , [ False   , False         , True      , True      ]),
        ('audit dbus send path=/foo/bar,'               , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('dbus receive,'                                , [ False   , False         , False     , False     ]),
        ('dbus peer=(label=foo),'                       , [ False   , False         , False     , False     ]),
        ('audit deny dbus send,'                        , [ False   , False         , False     , False     ]),
        ('deny dbus send,'                              , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_09(DbusCoveredTest):
    rule = 'dbus send member=**,'

    tests = [
        #   rule                                              equal     strict equal    covered     covered exact
        ('dbus,'                                        , [ False   , False         , False     , False     ]),
        ('dbus send,'                                   , [ False   , False         , False     , False     ]),
        ('dbus send member=/foo/bar,'                   , [ False   , False         , True      , True      ]),
        ('dbus send member=/foo/*,'                     , [ False   , False         , True      , True      ]),
        ('dbus send member=/**,'                        , [ False   , False         , True      , True      ]),
        ('dbus send member=/what/*,'                    , [ False   , False         , True      , True      ]),
        ('dbus member=/foo/bar,'                        , [ False   , False         , False     , False     ]),
        ('dbus send, # comment'                         , [ False   , False         , False     , False     ]),
        ('allow dbus send,'                             , [ False   , False         , False     , False     ]),
        ('allow dbus send member=/foo/bar,'             , [ False   , False         , True      , True      ]),
        ('dbus    send,'                                , [ False   , False         , False     , False     ]),
        ('dbus    send member=/foo/bar,'                , [ False   , False         , True      , True      ]),
        ('dbus    send member=/what/ever,'              , [ False   , False         , True      , True      ]),
        ('dbus send bus=session,'                       , [ False   , False         , False     , False     ]),
        ('dbus send member=/foo/bar peer=(label=foo),'  , [ False   , False         , True      , True      ]),
        ('audit dbus send member=/foo/bar,'             , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('dbus receive,'                                , [ False   , False         , False     , False     ]),
        ('dbus member=(label=foo),'                     , [ False   , False         , False     , False     ]),
        ('audit deny dbus send,'                        , [ False   , False         , False     , False     ]),
        ('deny dbus send,'                              , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_10(DbusCoveredTest):
    rule = 'dbus (send, receive) interface=foo,'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        ('dbus,'                                        , [ False   , False         , False     , False     ]),
        ('dbus send,'                                   , [ False   , False         , False     , False     ]),
        ('dbus send interface=foo,'                     , [ False   , False         , True      , True      ]),
        ('dbus receive bus=session interface=foo,'      , [ False   , False         , True      , True      ]),
        ('dbus (receive,send) interface=foo,'           , [ True    , False         , True      , True      ]),
        ('dbus (receive,send),'                         , [ False   , False         , False     , False     ]),
        ('dbus send bus=session,'                       , [ False   , False         , False     , False     ]),
        ('dbus send member=/foo/bar,'                   , [ False   , False         , False     , False     ]),
        ('dbus send member=/foo/*,'                     , [ False   , False         , False     , False     ]),
        ('dbus send member=/**,'                        , [ False   , False         , False     , False     ]),
        ('dbus send member=/what/*,'                    , [ False   , False         , False     , False     ]),
        ('dbus member=/foo/bar,'                        , [ False   , False         , False     , False     ]),
        ('dbus send, # comment'                         , [ False   , False         , False     , False     ]),
        ('allow dbus send,'                             , [ False   , False         , False     , False     ]),
        ('allow dbus send member=/foo/bar,'             , [ False   , False         , False     , False     ]),
        ('dbus    send,'                                , [ False   , False         , False     , False     ]),
        ('dbus    send member=/foo/bar,'                , [ False   , False         , False     , False     ]),
        ('dbus    send member=/what/ever,'              , [ False   , False         , False     , False     ]),
        ('dbus send bus=session,'                       , [ False   , False         , False     , False     ]),
        ('dbus send bus=session interface=foo,'         , [ False   , False         , True      , True      ]),
        ('dbus send member=/foo/bar peer=(label=foo),'  , [ False   , False         , False     , False     ]),
        ('audit dbus send member=/foo/bar,'             , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('dbus receive,'                                , [ False   , False         , False     , False     ]),
        ('dbus peer=(label=foo),'                       , [ False   , False         , False     , False     ]),
        ('audit deny dbus send,'                        , [ False   , False         , False     , False     ]),
        ('deny dbus send,'                              , [ False   , False         , False     , False     ]),
    ]

class DbusCoveredTest_11(DbusCoveredTest):
    rule = 'dbus name=/foo/bar,'

    tests = [
        #   rule                                            equal     strict equal    covered     covered exact
        ('dbus,'                                        , [ False   , False         , False     , False     ]),
        ('dbus name=/foo/bar,'                          , [ True    , True          , True      , True      ]),
        ('dbus name=/foo/*,'                            , [ False   , False         , False     , False     ]),
        ('dbus name=/**,'                               , [ False   , False         , False     , False     ]),
        ('dbus name=/what/*,'                           , [ False   , False         , False     , False     ]),
        ('dbus, # comment'                              , [ False   , False         , False     , False     ]),
        ('allow dbus,'                                  , [ False   , False         , False     , False     ]),
        ('allow dbus name=/foo/bar,'                    , [ True    , False         , True      , True      ]),
        ('dbus   ,'                                     , [ False   , False         , False     , False     ]),
        ('dbus    name=/foo/bar,'                       , [ True    , False         , True      , True      ]),
        ('dbus    name=/what/ever,'                     , [ False   , False         , False     , False     ]),
        ('dbus bus=session,'                            , [ False   , False         , False     , False     ]),
        ('dbus name=/foo/bar peer=(label=foo),'         , [ False   , False         , True      , True      ]),
        ('audit dbus name=/foo/bar,'                    , [ False   , False         , False     , False     ]),
        ('audit dbus,'                                  , [ False   , False         , False     , False     ]),
        ('dbus receive,'                                , [ False   , False         , False     , False     ]),
        ('dbus peer=(label=foo),'                       , [ False   , False         , False     , False     ]),
        ('audit deny dbus,'                             , [ False   , False         , False     , False     ]),
        ('deny dbus,'                                   , [ False   , False         , False     , False     ]),
    ]



class DbusCoveredTest_Invalid(AATest):
    def AASetup(self):
        #                        access               bus             path            name            interface       member          peername        peerlabel       expected exception
        self.obj     = DbusRule(('send', 'receive'), 'session',      '/org/test',    DbusRule.ALL,   '/int/face',    DbusRule.ALL,   '/peer/name',   '/peer/label', allow_keyword=True)
        self.testobj = DbusRule(('send'),            'session',      '/org/test',    DbusRule.ALL,   '/int/face',    '/mem/ber',     '/peer/name',   '/peer/label', allow_keyword=True)


    def test_borked_obj_is_covered_1(self):
        self.testobj.access = ''

        with self.assertRaises(AppArmorBug):
            self.obj.is_covered(self.testobj)

    def test_borked_obj_is_covered_2(self):
        self.testobj.bus = ''

        with self.assertRaises(AppArmorBug):
            self.obj.is_covered(self.testobj)

    def test_borked_obj_is_covered_3(self):
        self.testobj.path = ''

        with self.assertRaises(AppArmorBug):
            self.obj.is_covered(self.testobj)

    def test_borked_obj_is_covered_4(self):
        # we need a different 'victim' because dbus send doesn't allow the name conditional we want to test here
        self.obj     = DbusRule(('bind'),            'session',      DbusRule.ALL,   '/name',        DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,  allow_keyword=True)
        self.testobj = DbusRule(('bind'),            'session',      DbusRule.ALL,   '/name',        DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,   DbusRule.ALL,  allow_keyword=True)
        self.testobj.name = ''

        with self.assertRaises(AppArmorBug):
            self.obj.is_covered(self.testobj)

    def test_borked_obj_is_covered_5(self):
        self.testobj.interface = ''

        with self.assertRaises(AppArmorBug):
            self.obj.is_covered(self.testobj)

    def test_borked_obj_is_covered_6(self):
        self.testobj.member = ''

        with self.assertRaises(AppArmorBug):
            self.obj.is_covered(self.testobj)

    def test_borked_obj_is_covered_7(self):
        self.testobj.peername = ''

        with self.assertRaises(AppArmorBug):
            self.obj.is_covered(self.testobj)

    def test_borked_obj_is_covered_8(self):
        self.testobj.peerlabel = ''

        with self.assertRaises(AppArmorBug):
            self.obj.is_covered(self.testobj)


    def test_invalid_is_covered(self):
        obj = DbusRule.parse('dbus send,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_covered(testobj)

    def test_invalid_is_equal(self):
        obj = DbusRule.parse('dbus send,')

        testobj = BaseRule()  # different type

        with self.assertRaises(AppArmorBug):
            obj.is_equal(testobj)

class DbusLogprofHeaderTest(AATest):
    tests = [
        ('dbus,',                        [                               _('Access mode'), _('ALL'),    _('Bus'), _('ALL'),    _('Path'), _('ALL'), _('Name'), _('ALL'),    _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), _('ALL')]),
        ('dbus (send receive),',         [                               _('Access mode'), 'receive send', _('Bus'), _('ALL'), _('Path'), _('ALL'), _('Name'), _('ALL'),    _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), _('ALL')]),
        ('dbus send bus=session,',       [                               _('Access mode'), 'send',      _('Bus'), 'session',   _('Path'), _('ALL'), _('Name'), _('ALL'),    _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), _('ALL')]),
        ('deny dbus,',                   [_('Qualifier'), 'deny',        _('Access mode'), _('ALL'),    _('Bus'), _('ALL'),    _('Path'), _('ALL'), _('Name'), _('ALL'),    _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), _('ALL')]),
        ('allow dbus send,',             [_('Qualifier'), 'allow',       _('Access mode'), 'send',      _('Bus'), _('ALL'),    _('Path'), _('ALL'), _('Name'), _('ALL'),    _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), _('ALL')]),
        ('audit dbus send bus=session,', [_('Qualifier'), 'audit',       _('Access mode'), 'send',      _('Bus'), 'session',   _('Path'), _('ALL'), _('Name'), _('ALL'),    _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), _('ALL')]),
        ('audit deny dbus send,',        [_('Qualifier'), 'audit deny',  _('Access mode'), 'send',      _('Bus'), _('ALL'),    _('Path'), _('ALL'), _('Name'), _('ALL'),    _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), _('ALL')]),
        ('dbus bind name=bind.name,',    [                               _('Access mode'), 'bind',      _('Bus'), _('ALL'),    _('Path'), _('ALL'), _('Name'), 'bind.name', _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), _('ALL')]),
        ('dbus send bus=session path=/path interface=aa.test member=ExMbr peer=(name=(peer.name)),',
                                         [                               _('Access mode'), 'send',      _('Bus'), 'session',   _('Path'), '/path',  _('Name'), _('ALL'),    _('Interface'), 'aa.test', _('Member'), 'ExMbr',  _('Peer name'), 'peer.name',_('Peer label'), _('ALL')]),
        ('dbus send peer=(label=foo),',  [                               _('Access mode'), 'send',      _('Bus'), _('ALL'),    _('Path'), _('ALL'), _('Name'), _('ALL'),    _('Interface'), _('ALL'),  _('Member'), _('ALL'), _('Peer name'), _('ALL'),   _('Peer label'), 'foo'   ]),
   ]

    def _run_test(self, params, expected):
        obj = DbusRule._parse(params)
        self.assertEqual(obj.logprof_header(), expected)

## --- tests for DbusRuleset --- #

class DbusRulesTest(AATest):
    def test_empty_ruleset(self):
        ruleset = DbusRuleset()
        ruleset_2 = DbusRuleset()
        self.assertEqual([], ruleset.get_raw(2))
        self.assertEqual([], ruleset.get_clean(2))
        self.assertEqual([], ruleset_2.get_raw(2))
        self.assertEqual([], ruleset_2.get_clean(2))

    def test_ruleset_1(self):
        ruleset = DbusRuleset()
        rules = [
            'dbus peer=(label=foo),',
            'dbus send,',
        ]

        expected_raw = [
            'dbus peer=(label=foo),',
            'dbus send,',
            '',
        ]

        expected_clean = [
            'dbus peer=(label=foo),',
            'dbus send,',
            '',
        ]

        for rule in rules:
            ruleset.add(DbusRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw())
        self.assertEqual(expected_clean, ruleset.get_clean())

    def test_ruleset_2(self):
        ruleset = DbusRuleset()
        rules = [
            'dbus send peer=(label=foo),',
            'allow dbus send,',
            'deny dbus bus=session, # example comment',
        ]

        expected_raw = [
            '  dbus send peer=(label=foo),',
            '  allow dbus send,',
            '  deny dbus bus=session, # example comment',
            '',
        ]

        expected_clean = [
            '  deny dbus bus=session, # example comment',
            '',
            '  allow dbus send,',
            '  dbus send peer=(label=foo),',
            '',
        ]

        for rule in rules:
            ruleset.add(DbusRule.parse(rule))

        self.assertEqual(expected_raw, ruleset.get_raw(1))
        self.assertEqual(expected_clean, ruleset.get_clean(1))


class DbusGlobTest(AATest):
    def setUp(self):
        self.maxDiff = None
        self.ruleset = DbusRuleset()

    def test_glob_1(self):
        self.assertEqual(self.ruleset.get_glob('dbus send,'), 'dbus,')

    # not supported or used yet
    # def test_glob_2(self):
    #     self.assertEqual(self.ruleset.get_glob('dbus send raw,'), 'dbus send,')

    def test_glob_ext(self):
        with self.assertRaises(NotImplementedError):
            # get_glob_ext is not available for dbus rules
            self.ruleset.get_glob_ext('dbus send peer=(label=foo),')

#class DbusDeleteTest(AATest):
#    pass

setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
