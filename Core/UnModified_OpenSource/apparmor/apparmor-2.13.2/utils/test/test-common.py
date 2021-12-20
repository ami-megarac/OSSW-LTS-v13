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

from apparmor.common import type_is_str

class TestIs_str_type(AATest):
    tests = [
        ('foo',     True),
        (u'foo',    True),
        (42,        False),
        (True,      False),
        ([],        False),
    ]

    def _run_test(self, params, expected):
        self.assertEqual(type_is_str(params), expected)


setup_all_loops(__name__)
if __name__ == '__main__':
    unittest.main(verbosity=1)
