# ----------------------------------------------------------------------
#    Copyright (c) 2012 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, contact Canonical, Ltd.
# ----------------------------------------------------------------------
#
# Usage:
# $ python ./python-tools-setup.py install --root=... --version=...
#
# Note: --version=... must be the last argument to this script
#

from distutils.command.install import install as _install
from distutils.core import setup
import os
import shutil
import sys

class Install(_install, object):
    '''Override distutils to install the files where we want them.'''
    def run(self):
        # Now byte-compile everything
        super(Install, self).run()

        prefix = self.prefix
        if self.root != None:
            prefix = self.root

        # Install scripts, configuration files and data
        scripts = ['/usr/bin/aa-easyprof']
        self.mkpath(prefix + os.path.dirname(scripts[0]))
        for s in scripts:
            f = prefix + s
            self.copy_file(os.path.basename(s), f)

        configs = ['easyprof/easyprof.conf']
        self.mkpath(prefix + "/etc/apparmor")
        for c in configs:
            self.copy_file(c, os.path.join(prefix + "/etc/apparmor", os.path.basename(c)))

        data = ['easyprof/templates', 'easyprof/policygroups']
        self.mkpath(prefix + "/usr/share/apparmor/easyprof")
        for d in data:
            self.copy_tree(d, os.path.join(prefix + "/usr/share/apparmor/easyprof", os.path.basename(d)))


if os.path.exists('staging'):
    shutil.rmtree('staging')
shutil.copytree('apparmor', 'staging')

# Support the --version=... since this will be part of a Makefile
version = "unknown-version"
if "--version=" in sys.argv[-1]:
    version=sys.argv[-1].split('=')[1]
    sys.argv = sys.argv[0:-1]

setup (name='apparmor',
       version=version,
       description='Python libraries for AppArmor utilities',
       long_description='Python libraries for AppArmor utilities',
       author='AppArmor Developers',
       author_email='apparmor@lists.ubuntu.com',
       url='https://launchpad.net/apparmor',
       license='GPL-2',
       cmdclass={'install': Install},
       package_dir={'apparmor': 'staging'},
       packages=['apparmor', 'apparmor.rule'],
       py_modules=['apparmor.easyprof']
)

shutil.rmtree('staging')
