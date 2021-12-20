#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2017 Christian Boltz <apparmor@cboltz.de>
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import unittest
from common_test import AATest, setup_all_loops

from apparmor.common import AppArmorBug
from apparmor.profile_storage import ProfileStorage, add_or_remove_flag, split_flags

class TestUnknownKey(AATest):
    def AASetup(self):
        self.storage = ProfileStorage('/test/foo', 'hat', 'TEST')

    def test_read(self):
        with self.assertRaises(AppArmorBug):
            self.storage['foo']

    def test_get(self):
        with self.assertRaises(AppArmorBug):
            self.storage.get('foo')

    def test_get_with_fallback(self):
        with self.assertRaises(AppArmorBug):
            self.storage.get('foo', 'bar')

    def test_set(self):
        with self.assertRaises(AppArmorBug):
            self.storage['foo'] = 'bar'

class AaTest_add_or_remove_flag(AATest):
    tests = [
        #  existing flag(s)     flag to change  add or remove?      expected flags
        ([ [],                  'complain',     True            ],  ['complain']                ),
        ([ [],                  'complain',     False           ],  []                          ),
        ([ ['complain'],        'complain',     True            ],  ['complain']                ),
        ([ ['complain'],        'complain',     False           ],  []                          ),
        ([ [],                  'audit',        True            ],  ['audit']                   ),
        ([ [],                  'audit',        False           ],  []                          ),
        ([ ['complain'],        'audit',        True            ],  ['audit', 'complain']       ),
        ([ ['complain'],        'audit',        False           ],  ['complain']                ),
        ([ '',                  'audit',        True            ],  ['audit']                   ),
        ([ None,                'audit',        False           ],  []                          ),
        ([ 'complain',          'audit',        True            ],  ['audit', 'complain']       ),
        ([ '  complain  ',      'audit',        False           ],  ['complain']                ),
    ]

    def _run_test(self, params, expected):
        new_flags = add_or_remove_flag(params[0], params[1], params[2])
        self.assertEqual(new_flags, expected)

class AaTest_split_flags(AATest):
    tests = [
        (None                               , []                                    ),
        (''                                 , []                                    ),
        ('       '                          , []                                    ),
        ('  ,       '                       , []                                    ),
        ('complain'                         , ['complain']                          ),
        ('  complain   attach_disconnected' , ['attach_disconnected', 'complain']   ),
        ('  complain , attach_disconnected' , ['attach_disconnected', 'complain']   ),
        ('  complain , , audit , , '        , ['audit', 'complain']                 ),
    ]

    def _run_test(self, params, expected):
        split = split_flags(params)
        self.assertEqual(split, expected)


setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
