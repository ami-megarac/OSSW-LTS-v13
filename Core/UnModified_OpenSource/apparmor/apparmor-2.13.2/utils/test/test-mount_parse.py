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

class BaseAAParseMountTest(AAParseTest):
    def setUp(self):
        self.parse_function = aa.parse_mount_rule

class AAParseMountTest(BaseAAParseMountTest):
    tests = [
        ('mount,', 'mount base keyword rule'),
        ('mount -o ro,', 'mount ro rule'),
        ('mount -o rw /dev/sdb1 -> /mnt/external,', 'mount rw with mount point'),
    ]

class AAParseRemountTest(BaseAAParseMountTest):
    tests = [
        ('remount,', 'remount base keyword rule'),
        ('remount -o ro,', 'remount ro rule'),
        ('remount -o ro /,', 'remount ro with mountpoint'),
    ]

class AAParseUmountTest(BaseAAParseMountTest):
    tests = [
        ('umount,', 'umount base keyword rule'),
        ('umount /mnt/external,', 'umount with mount point'),
        ('unmount,', 'unmount base keyword rule'),
        ('unmount /mnt/external,', 'unmount with mount point'),
    ]

setup_aa(aa)
if __name__ == '__main__':
    setup_regex_tests(AAParseMountTest)
    setup_regex_tests(AAParseRemountTest)
    setup_regex_tests(AAParseUmountTest)
    unittest.main(verbosity=1)
