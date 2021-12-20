#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2015 Christian Boltz <apparmor@cboltz.de>
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import unittest
from common_test import AATest, setup_all_loops

from apparmor.common import AppArmorBug
from apparmor.rule import BaseRule, parse_modifiers
import apparmor.severity as severity

import re

class TestBaserule(AATest):
    def test_abstract__parse(self):
        with self.assertRaises(NotImplementedError):
            BaseRule._parse('foo')

    def test_abstract__parse_2(self):
        with self.assertRaises(NotImplementedError):
            BaseRule.parse('foo')

    def test_abstract__match(self):
        with self.assertRaises(NotImplementedError):
            BaseRule._match('foo')

    def test_abstract__match2(self):
        with self.assertRaises(NotImplementedError):
            BaseRule.match('foo')

    def test_abstract_get_clean(self):
        obj = BaseRule()
        with self.assertRaises(NotImplementedError):
            obj.get_clean()

    def test_is_equal_localvars(self):
        obj = BaseRule()
        with self.assertRaises(NotImplementedError):
            obj.is_equal_localvars(BaseRule(), False)

    def test_is_covered_localvars(self):
        obj = BaseRule()
        with self.assertRaises(NotImplementedError):
            obj.is_covered_localvars(None)

    def test_parse_modifiers_invalid(self):
        regex = re.compile('^\s*(?P<audit>audit\s+)?(?P<allow>allow\s+|deny\s+|invalid\s+)?')
        matches = regex.search('audit invalid ')

        with self.assertRaises(AppArmorBug):
            parse_modifiers(matches)

    def test_default_severity(self):
        sev_db = severity.Severity('severity.db', 'unknown')
        obj = BaseRule()
        rank = obj.severity(sev_db)
        self.assertEqual(rank, sev_db.NOT_IMPLEMENTED)

    def test_logprof_header_localvars(self):
        obj = BaseRule()
        with self.assertRaises(NotImplementedError):
            obj.logprof_header_localvars()

    def test_edit_header_localvars(self):
        obj = BaseRule()
        with self.assertRaises(NotImplementedError):
            obj.edit_header()

    def test_validate_edit_localvars(self):
        obj = BaseRule()
        with self.assertRaises(NotImplementedError):
            obj.validate_edit('/foo')

    def test_store_edit_localvars(self):
        obj = BaseRule()
        with self.assertRaises(NotImplementedError):
            obj.store_edit('/foo')


setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
