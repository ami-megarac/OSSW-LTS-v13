#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2014-2016 Christian Boltz
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import unittest
from common_test import AATest, setup_all_loops

import apparmor.aamode

from apparmor.aamode import split_log_mode, str_to_mode, sub_str_to_mode, validate_log_mode
from apparmor.common import AppArmorBug

class AamodeTest_split_log_mode(AATest):
    def test_split_log_mode_1(self):
        self.assertEqual(split_log_mode(''), ('', ''))
    def test_split_log_mode_2(self):
        self.assertEqual(split_log_mode('r'), ('r', 'r'))
    def test_split_log_mode_3(self):
        self.assertEqual(split_log_mode('r::'), ('r', ''))
    def test_split_log_mode_4(self):
        self.assertEqual(split_log_mode('::r'), ('', 'r'))
    def test_split_log_mode_5(self):
        self.assertEqual(split_log_mode('r::w'), ('r', 'w'))
    def test_split_log_mode_6(self):
        self.assertEqual(split_log_mode('rw::rw'), ('rw', 'rw'))
    def test_split_log_mode_invalid_1(self):
        with self.assertRaises(AppArmorBug):
            split_log_mode('r::w::r')

class AamodeTest_str_to_mode(AATest):
    tests = [
        ('x',   apparmor.aamode.AA_MAY_EXEC),
        ('w',   apparmor.aamode.AA_MAY_WRITE),
        ('r',   apparmor.aamode.AA_MAY_READ),
        ('a',   apparmor.aamode.AA_MAY_APPEND),
        ('l',   apparmor.aamode.AA_MAY_LINK),
        ('k',   apparmor.aamode.AA_MAY_LOCK),
        ('m',   apparmor.aamode.AA_EXEC_MMAP),
        ('i',   apparmor.aamode.AA_EXEC_INHERIT),
        ('u',   apparmor.aamode.AA_EXEC_UNCONFINED | apparmor.aamode.AA_EXEC_UNSAFE),
        ('U',   apparmor.aamode.AA_EXEC_UNCONFINED),
        ('p',   apparmor.aamode.AA_EXEC_PROFILE | apparmor.aamode.AA_EXEC_UNSAFE),
        ('P',   apparmor.aamode.AA_EXEC_PROFILE),
        ('c',   apparmor.aamode.AA_EXEC_CHILD | apparmor.aamode.AA_EXEC_UNSAFE),
        ('C',   apparmor.aamode.AA_EXEC_CHILD),
        (None,  set()),
    ]

    def _run_test(self, params, expected):
        mode = expected | apparmor.aamode.AA_OTHER(expected)
        #print("mode: %s params: %s str_to_mode(params): %s" % (mode, params,  apparmor.aamode.str_to_mode(params)))
        self.assertEqual(mode, str_to_mode(params), 'mode is %s and expected string is %s'%(mode, expected))

class AamodeTest_sub_str_to_mode(AATest):
    def test_sub_str_to_mode_1(self):
        self.assertEqual(sub_str_to_mode(''), set())
    def test_sub_str_to_mode_2(self):
        self.assertEqual(sub_str_to_mode('ix'), {'i', 'x'})
    def test_sub_str_to_mode_3(self):
        self.assertEqual(sub_str_to_mode('rw'), {'r', 'w'})
    def test_sub_str_to_mode_4(self):
        self.assertEqual(sub_str_to_mode('rPix'), {'i', 'P', 'r', 'x'})
    def test_sub_str_to_mode_5(self):
        self.assertEqual(sub_str_to_mode('rPUx'), {'P', 'r', 'U', 'x'})
    def test_sub_str_to_mode_6(self):
        self.assertEqual(sub_str_to_mode('cix'), {'i', 'x', 'C', 'execunsafe'})
    def test_sub_str_to_mode_7(self):
        self.assertEqual(sub_str_to_mode('rwlk'), {'k', 'r', 'l', 'w'})
    def test_sub_str_to_mode_dupes(self):
        self.assertEqual(sub_str_to_mode('rwrwrw'), {'r', 'w'})

    def test_sub_str_to_mode_invalid_1(self):
        with self.assertRaises(AppArmorBug):
            sub_str_to_mode('asdf42')

    def test_sub_str_to_mode_invalid_2(self):
        import apparmor.aamode
        apparmor.aamode.MODE_HASH = {'x': 'foo'}  # simulate MODE_HASH and MODE_MAP_SET getting out of sync

        with self.assertRaises(AppArmorBug):
            sub_str_to_mode('r')



class AamodeTest_validate_log_mode(AATest):
    def test_validate_log_mode_1(self):
        self.assertTrue(validate_log_mode('a'))
    def test_validate_log_mode_2(self):
        self.assertTrue(validate_log_mode('rw'))
    def test_validate_log_mode_3(self):
        self.assertTrue(validate_log_mode('Pixrw'))
    def test_validate_log_mode_4(self):
        self.assertTrue(validate_log_mode('rrrr'))

    def test_validate_log_mode_invalid_1(self):
        self.assertFalse(validate_log_mode('c'))  # 'c' (create) must be converted to 'a' before calling validate_log_mode()
    def test_validate_log_mode_invalid_2(self):
        self.assertFalse(validate_log_mode('R'))  # only lowercase 'r' is valid
    def test_validate_log_mode_invalid_3(self):
        self.assertFalse(validate_log_mode('foo'))
    def test_validate_log_mode_invalid_4(self):
        self.assertFalse(validate_log_mode(''))


setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
