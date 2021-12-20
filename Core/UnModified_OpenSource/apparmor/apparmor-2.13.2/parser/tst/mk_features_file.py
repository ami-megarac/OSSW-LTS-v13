#!/usr/bin/env python3
# ------------------------------------------------------------------
#
#   Copyright (C) 2014 Canonical Ltd.
#   Author: Steve Beattie <steve@nxnw.org>
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of version 2 of the GNU General Public
#   License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

from testlib import read_features_dir
from argparse import ArgumentParser
import os
from sys import stderr, exit

DEFAULT_FEATURES_DIR='/sys/kernel/security/apparmor/features'

def main():
    p = ArgumentParser()

    p.add_argument('fdir', action="store", nargs='?', metavar="features_dir",
                   default=DEFAULT_FEATURES_DIR, help="path to features directory")
    config = p.parse_args()

    if not os.path.exists(config.fdir):
        print('Unable to find apparmor features directory "%s"' % config.fdir, file=stderr)
        return 1

    features = read_features_dir(config.fdir)
    print(features)

    return 0

if __name__ == "__main__":
    exit(main())
