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
from common_test import AAParseTest, setup_regex_tests, setup_aa

class AAParsePivotRootTest(AAParseTest):
    def setUp(self):
        self.parse_function = aa.parse_pivot_root_rule

    tests = [
        ('pivot_root,', 'pivot_root base keyword'),
        ('pivot_root /old,', 'pivot_root oldroot rule'),
        ('pivot_root /old /new,', 'pivot_root old and new root rule'),
        ('pivot_root /old /new -> /usr/bin/child,', 'pivot_root child rule'),
    ]

setup_aa(aa)
if __name__ == '__main__':
    setup_regex_tests(AAParsePivotRootTest)
    unittest.main(verbosity=1)
