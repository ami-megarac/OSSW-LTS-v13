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

class AAParseUnixTest(AAParseTest):

    def setUp(self):
        self.parse_function = aa.parse_unix_rule

    tests = [
        ('unix,', 'unix base keyword'),
        ('unix r,', 'unix r rule'),
        ('unix w,', 'unix w rule'),
        ('unix rw,', 'unix rw rule'),
        ('unix send,', 'unix send rule'),
        ('unix receive,', 'unix receive rule'),
        ('unix (r),', 'unix (r) rule'),
        ('unix (w),', 'unix (w) rule'),
        ('unix (rw),', 'unix (rw) rule'),
        ('unix (send),', 'unix (send) rule'),
        ('unix (receive),', 'unix (receive) rule'),
        ('unix (connect, receive, send) type=stream peer=(label=unconfined,addr="@/tmp/.X11-unix/X[0-9]*"),',
            'complex unix rule'),
    ]

setup_aa(aa)
if __name__ == '__main__':
    setup_regex_tests(AAParseUnixTest)
    unittest.main(verbosity=1)
