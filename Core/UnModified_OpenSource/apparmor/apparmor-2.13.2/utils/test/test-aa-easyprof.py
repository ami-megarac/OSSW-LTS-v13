#! /usr/bin/python3
# ------------------------------------------------------------------
#
#    Copyright (C) 2011-2015 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

import glob
import json
import optparse
import os
import shutil
import sys
import tempfile
import unittest

import apparmor.easyprof as easyprof

topdir = None
debugging = False

def recursive_rm(dirPath, contents_only=False):
    '''recursively remove directory'''
    names = os.listdir(dirPath)
    for name in names:
        path = os.path.join(dirPath, name)
        if os.path.islink(path) or not os.path.isdir(path):
            os.unlink(path)
        else:
            recursive_rm(path)
    if contents_only == False:
        os.rmdir(dirPath)

# From Lib/test/test_optparse.py from python 2.7.4
class InterceptedError(Exception):
    def __init__(self,
                 error_message=None,
                 exit_status=None,
                 exit_message=None):
        self.error_message = error_message
        self.exit_status = exit_status
        self.exit_message = exit_message

    def __str__(self):
        return self.error_message or self.exit_message or "intercepted error"


class InterceptingOptionParser(optparse.OptionParser):
    def exit(self, status=0, msg=None):
        raise InterceptedError(exit_status=status, exit_message=msg)

    def error(self, msg):
        raise InterceptedError(error_message=msg)


class Manifest(object):
    def __init__(self, profile_name):
        self.security = dict()
        self.security['profiles'] = dict()
        self.profile_name = profile_name
        self.security['profiles'][self.profile_name] = dict()

    def add_policygroups(self, policy_list):
        self.security['profiles'][self.profile_name]['policy_groups'] = policy_list.split(",")

    def add_author(self, author):
        self.security['profiles'][self.profile_name]['author'] = author

    def add_copyright(self, copyright):
        self.security['profiles'][self.profile_name]['copyright'] = copyright

    def add_comment(self, comment):
        self.security['profiles'][self.profile_name]['comment'] = comment

    def add_binary(self, binary):
        self.security['profiles'][self.profile_name]['binary'] = binary

    def add_template(self, template):
        self.security['profiles'][self.profile_name]['template'] = template

    def add_template_variable(self, name, value):
        if not 'template_variables' in self.security['profiles'][self.profile_name]:
            self.security['profiles'][self.profile_name]['template_variables'] = dict()

        self.security['profiles'][self.profile_name]['template_variables'][name] = value

    def emit_json(self, use_security_prefix=True):
        manifest = dict()
        manifest['security'] = self.security
        if use_security_prefix:
            dumpee = manifest
        else:
            dumpee = self.security

        return json.dumps(dumpee, indent=2)

#
# Our test class
#
class T(unittest.TestCase):

    # work around UsrMove
    ls = os.path.realpath('/bin/ls')

    def setUp(self):
        '''Setup for tests'''
        global topdir

        self.tmpdir = os.path.realpath(tempfile.mkdtemp(prefix='test-aa-easyprof'))

        # Copy everything into place
        for d in ['easyprof/policygroups', 'easyprof/templates']:
            shutil.copytree(os.path.join(topdir, d),
                            os.path.join(self.tmpdir, os.path.basename(d)))

        # Create a test template
        self.test_template = "test-template"
        contents = '''# vim:syntax=apparmor
# %s
# AppArmor policy for ###NAME###
# ###AUTHOR###
# ###COPYRIGHT###
# ###COMMENT###

#include <tunables/global>

###VAR###

###PROFILEATTACH### {
  #include <abstractions/base>

  ###ABSTRACTIONS###

  ###POLICYGROUPS###

  ###READS###

  ###WRITES###
}

''' % (self.test_template)
        open(os.path.join(self.tmpdir, 'templates', self.test_template), 'w').write(contents)

        # Create a test policygroup
        self.test_policygroup = "test-policygroup"
        contents = '''
  # %s
  #include <abstractions/gnome>
  #include <abstractions/nameservice>
''' % (self.test_policygroup)
        open(os.path.join(self.tmpdir, 'policygroups', self.test_policygroup), 'w').write(contents)

        # setup our conffile
        self.conffile = os.path.join(self.tmpdir, 'easyprof.conf')
        contents = '''
POLICYGROUPS_DIR="%s/policygroups"
TEMPLATES_DIR="%s/templates"
''' % (self.tmpdir, self.tmpdir)
        open(self.conffile, 'w').write(contents)

        self.binary = "/opt/bin/foo"
        self.full_args = ['-c', self.conffile, self.binary]

        # Check __AA_BASEDIR, which may be set by the Makefile, to see if
        # we should use a non-default base directory path to find
        # abstraction files
        #
        # NOTE: Individual tests can append another --base path to the
        #       args list and override a base path set here
        base = os.getenv('__AA_BASEDIR')
        if base:
            self.full_args.append('--base=%s' % base)

        # Check __AA_PARSER, which may be set by the Makefile, to see if
        # we should use a non-default apparmor_parser path to verify
        # policy
        parser = os.getenv('__AA_PARSER')
        if parser:
            self.full_args.append('--parser=%s' % parser)

        if debugging:
            self.full_args.append('-d')

        (self.options, self.args) = easyprof.parse_args(self.full_args + [self.binary])

        # Now create some differently prefixed files in the include-dir
        self.test_include_dir = os.path.join(self.tmpdir, 'include-dir')
        os.mkdir(self.test_include_dir)
        os.mkdir(os.path.join(self.test_include_dir, "templates"))
        os.mkdir(os.path.join(self.test_include_dir, "policygroups"))
        for d in ['policygroups', 'templates']:
            for f in easyprof.get_directory_contents(os.path.join(
                                                     self.tmpdir, d)):
                shutil.copy(f, os.path.join(self.test_include_dir, d,
                                            "inc_%s" % os.path.basename(f)))

    def tearDown(self):
        '''Teardown for tests'''
        if os.path.exists(self.tmpdir):
            if debugging:
                sys.stdout.write("%s\n" % self.tmpdir)
            else:
                recursive_rm(self.tmpdir)

#
# config file tests
#
    def test_configuration_file_p_invalid(self):
        '''Test config parsing (invalid POLICYGROUPS_DIR)'''
        contents = '''
POLICYGROUPS_DIR=
TEMPLATES_DIR="%s/templates"
''' % (self.tmpdir)

        open(self.conffile, 'w').write(contents)
        try:
            easyprof.AppArmorEasyProfile(self.binary, self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise

        raise Exception ("File should have been invalid")

    def test_configuration_file_p_empty(self):
        '''Test config parsing (empty POLICYGROUPS_DIR)'''
        contents = '''
POLICYGROUPS_DIR="%s"
TEMPLATES_DIR="%s/templates"
''' % ('', self.tmpdir)

        open(self.conffile, 'w').write(contents)
        try:
            easyprof.AppArmorEasyProfile(self.binary, self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise

        raise Exception ("File should have been invalid")

    def test_configuration_file_p_nonexistent(self):
        '''Test config parsing (nonexistent POLICYGROUPS_DIR)'''
        contents = '''
POLICYGROUPS_DIR="%s/policygroups"
TEMPLATES_DIR="%s/templates"
''' % ('/nonexistent', self.tmpdir)

        open(self.conffile, 'w').write(contents)
        try:
            easyprof.AppArmorEasyProfile(self.binary, self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise

        raise Exception ("File should have been invalid")

    def test_policygroups_dir_relative(self):
        '''Test --policy-groups-dir (relative DIR)'''
        os.chdir(self.tmpdir)
        rel = os.path.join(self.tmpdir, 'relative')
        os.mkdir(rel)
        shutil.copy(os.path.join(self.tmpdir, 'policygroups', self.test_policygroup), os.path.join(rel, self.test_policygroup))

        args = self.full_args
        args += ['--policy-groups-dir', './relative', '--show-policy-group', '--policy-groups=%s' % self.test_policygroup]
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        # no fallback
        self.assertTrue(easyp.dirs['policygroups'] == rel, "Not using specified --policy-groups-dir\n" +
                                                           "Specified dir: %s\nActual dir: %s" % (rel, str(easyp.dirs['policygroups'])))
        self.assertFalse(easyp.get_policy_groups() == None, "Could not find policy-groups")

    def test_policygroups_dir_nonexistent(self):
        '''Test --policy-groups-dir (nonexistent DIR)'''
        os.chdir(self.tmpdir)
        rel = os.path.join(self.tmpdir, 'nonexistent')

        args = self.full_args
        args += ['--policy-groups-dir', rel, '--show-policy-group', '--policy-groups=%s' % self.test_policygroup]
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        # test if using fallback
        self.assertFalse(easyp.dirs['policygroups'] == rel, "Using nonexistent --policy-groups-dir")

        # test fallback
        self.assertTrue(easyp.get_policy_groups() != None, "Found policy-groups when shouldn't have")

    def test_policygroups_dir_valid(self):
        '''Test --policy-groups-dir (valid DIR)'''
        os.chdir(self.tmpdir)
        valid = os.path.join(self.tmpdir, 'valid')
        os.mkdir(valid)
        shutil.copy(os.path.join(self.tmpdir, 'policygroups', self.test_policygroup), os.path.join(valid, self.test_policygroup))

        args = self.full_args
        args += ['--policy-groups-dir', valid, '--show-policy-group', '--policy-groups=%s' % self.test_policygroup]
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        # no fallback
        self.assertTrue(easyp.dirs['policygroups'] == valid, "Not using specified --policy-groups-dir")
        self.assertFalse(easyp.get_policy_groups() == None, "Could not find policy-groups")

    def test_policygroups_dir_valid_with_vendor(self):
        '''Test --policy-groups-dir (valid DIR with vendor)'''
        os.chdir(self.tmpdir)
        valid = os.path.join(self.tmpdir, 'valid')
        os.mkdir(valid)
        shutil.copy(os.path.join(self.tmpdir, 'policygroups', self.test_policygroup), os.path.join(valid, self.test_policygroup))

        vendor = "ubuntu"
        version = "1.0"
        valid_distro = os.path.join(valid, vendor, version)
        os.mkdir(os.path.join(valid, vendor))
        os.mkdir(valid_distro)
        shutil.copy(os.path.join(self.tmpdir, 'policygroups', self.test_policygroup), valid_distro)

        args = self.full_args
        args += ['--policy-groups-dir', valid, '--show-policy-group', '--policy-groups=%s' % self.test_policygroup]
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        self.assertTrue(easyp.dirs['policygroups'] == valid, "Not using specified --policy-groups-dir")
        self.assertFalse(easyp.get_policy_groups() == None, "Could not find policy-groups")
        for f in easyp.get_policy_groups():
            self.assertFalse(os.path.basename(f) == vendor, "Found '%s' in %s" % (vendor, f))

    def test_configuration_file_t_invalid(self):
        '''Test config parsing (invalid TEMPLATES_DIR)'''
        contents = '''
TEMPLATES_DIR=
POLICYGROUPS_DIR="%s/templates"
''' % (self.tmpdir)

        open(self.conffile, 'w').write(contents)
        try:
            easyprof.AppArmorEasyProfile(self.binary, self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise

        raise Exception ("File should have been invalid")

    def test_configuration_file_t_empty(self):
        '''Test config parsing (empty TEMPLATES_DIR)'''
        contents = '''
TEMPLATES_DIR="%s"
POLICYGROUPS_DIR="%s/templates"
''' % ('', self.tmpdir)

        open(self.conffile, 'w').write(contents)
        try:
            easyprof.AppArmorEasyProfile(self.binary, self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise

        raise Exception ("File should have been invalid")

    def test_configuration_file_t_nonexistent(self):
        '''Test config parsing (nonexistent TEMPLATES_DIR)'''
        contents = '''
TEMPLATES_DIR="%s/policygroups"
POLICYGROUPS_DIR="%s/templates"
''' % ('/nonexistent', self.tmpdir)

        open(self.conffile, 'w').write(contents)
        try:
            easyprof.AppArmorEasyProfile(self.binary, self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise

        raise Exception ("File should have been invalid")

    def test_templates_dir_relative(self):
        '''Test --templates-dir (relative DIR)'''
        os.chdir(self.tmpdir)
        rel = os.path.join(self.tmpdir, 'relative')
        os.mkdir(rel)
        shutil.copy(os.path.join(self.tmpdir, 'templates', self.test_template), os.path.join(rel, self.test_template))

        args = self.full_args
        args += ['--templates-dir', './relative', '--show-template', '--template=%s' % self.test_template]
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        # no fallback
        self.assertTrue(easyp.dirs['templates'] == rel, "Not using specified --template-dir\n" +
                                                        "Specified dir: %s\nActual dir: %s" % (rel, str(easyp.dirs['templates'])))
        self.assertFalse(easyp.get_templates() == None, "Could not find templates")

    def test_templates_dir_nonexistent(self):
        '''Test --templates-dir (nonexistent DIR)'''
        os.chdir(self.tmpdir)
        rel = os.path.join(self.tmpdir, 'nonexistent')

        args = self.full_args
        args += ['--templates-dir', rel, '--show-template', '--template=%s' % self.test_template]
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        # test if using fallback
        self.assertFalse(easyp.dirs['templates'] == rel, "Using nonexistent --template-dir")

        # test fallback
        self.assertTrue(easyp.get_templates() != None, "Found templates when shouldn't have")

    def test_templates_dir_valid(self):
        '''Test --templates-dir (valid DIR)'''
        os.chdir(self.tmpdir)
        valid = os.path.join(self.tmpdir, 'valid')
        os.mkdir(valid)
        shutil.copy(os.path.join(self.tmpdir, 'templates', self.test_template), os.path.join(valid, self.test_template))

        args = self.full_args
        args += ['--templates-dir', valid, '--show-template', '--template=%s' % self.test_template]
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        # no fallback
        self.assertTrue(easyp.dirs['templates'] == valid, "Not using specified --template-dir")
        self.assertFalse(easyp.get_templates() == None, "Could not find templates")

    def test_templates_dir_valid_with_vendor(self):
        '''Test --templates-dir (valid DIR with vendor)'''
        os.chdir(self.tmpdir)
        valid = os.path.join(self.tmpdir, 'valid')
        os.mkdir(valid)
        shutil.copy(os.path.join(self.tmpdir, 'templates', self.test_template), os.path.join(valid, self.test_template))

        vendor = "ubuntu"
        version = "1.0"
        valid_distro = os.path.join(valid, vendor, version)
        os.mkdir(os.path.join(valid, vendor))
        os.mkdir(valid_distro)
        shutil.copy(os.path.join(self.tmpdir, 'templates', self.test_template), valid_distro)

        args = self.full_args
        args += ['--templates-dir', valid, '--show-template', '--template=%s' % self.test_template]
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        self.assertTrue(easyp.dirs['templates'] == valid, "Not using specified --template-dir")
        self.assertFalse(easyp.get_templates() == None, "Could not find templates")
        for f in easyp.get_templates():
            self.assertFalse(os.path.basename(f) == vendor, "Found '%s' in %s" % (vendor, f))

#
# Binary file tests
#
    def test_binary_without_profile_name(self):
        '''Test binary (<binary> { })'''
        easyprof.AppArmorEasyProfile(self.ls, self.options)

    def test_binary_with_profile_name(self):
        '''Test binary (profile <name> <binary> { })'''
        args = self.full_args
        args += ['--profile-name=some-profile-name']
        (self.options, self.args) = easyprof.parse_args(args)
        easyprof.AppArmorEasyProfile(self.ls, self.options)

    def test_binary_omitted_with_profile_name(self):
        '''Test binary (profile <name> { })'''
        args = self.full_args
        args += ['--profile-name=some-profile-name']
        (self.options, self.args) = easyprof.parse_args(args)
        easyprof.AppArmorEasyProfile(None, self.options)

    def test_binary_nonexistent(self):
        '''Test binary (nonexistent)'''
        easyprof.AppArmorEasyProfile(os.path.join(self.tmpdir, 'nonexistent'), self.options)

    def test_binary_relative(self):
        '''Test binary (relative)'''
        try:
            easyprof.AppArmorEasyProfile('./foo', self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("Binary should have been invalid")

    def test_binary_symlink(self):
        '''Test binary (symlink)'''
        exe = os.path.join(self.tmpdir, 'exe')
        open(exe, 'a').close()
        symlink = exe + ".lnk"
        os.symlink(exe, symlink)

        try:
            easyprof.AppArmorEasyProfile(symlink, self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("Binary should have been invalid")

#
# Templates tests
#
    def test_templates_list(self):
        '''Test templates (list)'''
        args = self.full_args
        args.append('--list-templates')
        (self.options, self.args) = easyprof.parse_args(args)

        easyp = easyprof.AppArmorEasyProfile(None, self.options)
        for i in easyp.get_templates():
            self.assertTrue(os.path.exists(i), "Could not find '%s'" % i)

    def test_templates_show(self):
        '''Test templates (show)'''
        files = []
        for f in glob.glob("%s/templates/*" % self.tmpdir):
            files.append(f)

        for f in files:
            args = self.full_args
            args += ['--show-template', '--template', f]
            (self.options, self.args) = easyprof.parse_args(args)
            easyp = easyprof.AppArmorEasyProfile(None, self.options)

            path = os.path.join(easyp.dirs['templates'], f)
            self.assertTrue(os.path.exists(path), "Could not find '%s'" % path)
            open(path).read()

    def test_templates_list_include(self):
        '''Test templates (list with --include-templates-dir)'''
        args = self.full_args
        args.append('--list-templates')
        (self.options, self.args) = easyprof.parse_args(args)

        easyp = easyprof.AppArmorEasyProfile(None, self.options)
        orig_templates = easyp.get_templates()

        args = self.full_args
        args.append('--list-templates')
        args.append('--include-templates-dir=%s' %
                    os.path.join(self.test_include_dir, 'templates'))
        (self.options, self.args) = easyprof.parse_args(args)

        easyp = easyprof.AppArmorEasyProfile(None, self.options)
        inc_templates = easyp.get_templates()
        self.assertTrue(len(inc_templates) == len(orig_templates) * 2,
                        "templates missing: %s" % inc_templates)

        for i in inc_templates:
            self.assertTrue(os.path.exists(i), "Could not find '%s'" % i)

    def test_templates_show_include(self):
        '''Test templates (show with --include-templates-dir)'''
        files = []
        for f in glob.glob("%s/templates/*" % self.test_include_dir):
            files.append(f)

        for f in files:
            args = self.full_args
            args += ['--show-template',
                     '--template', f,
                     '--include-templates-dir=%s' %
                     os.path.join(self.test_include_dir, 'templates')]
            (self.options, self.args) = easyprof.parse_args(args)
            easyp = easyprof.AppArmorEasyProfile(None, self.options)

            path = os.path.join(easyp.dirs['templates_include'], f)
            self.assertTrue(os.path.exists(path), "Could not find '%s'" % path)
            open(path).read()

            bn = os.path.basename(f)
            # setup() copies everything in the include prefixed with inc_
            self.assertTrue(bn.startswith('inc_'),
                            "'%s' does not start with 'inc_'" % bn)

#
# Policygroups tests
#
    def test_policygroups_list(self):
        '''Test policygroups (list)'''
        args = self.full_args
        args.append('--list-policy-groups')
        (self.options, self.args) = easyprof.parse_args(args)

        easyp = easyprof.AppArmorEasyProfile(None, self.options)
        for i in easyp.get_policy_groups():
            self.assertTrue(os.path.exists(i), "Could not find '%s'" % i)

    def test_policygroups_show(self):
        '''Test policygroups (show)'''
        files = []
        for f in glob.glob("%s/policygroups/*" % self.tmpdir):
            files.append(f)

        for f in files:
            args = self.full_args
            args += ['--show-policy-group',
                     '--policy-groups', os.path.basename(f)]
            (self.options, self.args) = easyprof.parse_args(args)
            easyp = easyprof.AppArmorEasyProfile(None, self.options)

            path = os.path.join(easyp.dirs['policygroups'], f)
            self.assertTrue(os.path.exists(path), "Could not find '%s'" % path)
            open(path).read()

    def test_policygroups_list_include(self):
        '''Test policygroups (list with --include-policy-groups-dir)'''
        args = self.full_args
        args.append('--list-policy-groups')
        (self.options, self.args) = easyprof.parse_args(args)

        easyp = easyprof.AppArmorEasyProfile(None, self.options)
        orig_policy_groups = easyp.get_policy_groups()

        args = self.full_args
        args.append('--list-policy-groups')
        args.append('--include-policy-groups-dir=%s' %
                    os.path.join(self.test_include_dir, 'policygroups'))
        (self.options, self.args) = easyprof.parse_args(args)

        easyp = easyprof.AppArmorEasyProfile(None, self.options)
        inc_policy_groups = easyp.get_policy_groups()
        self.assertTrue(len(inc_policy_groups) == len(orig_policy_groups) * 2,
                        "policy_groups missing: %s" % inc_policy_groups)

        for i in inc_policy_groups:
            self.assertTrue(os.path.exists(i), "Could not find '%s'" % i)

    def test_policygroups_show_include(self):
        '''Test policygroups (show with --include-policy-groups-dir)'''
        files = []
        for f in glob.glob("%s/policygroups/*" % self.test_include_dir):
            files.append(f)

        for f in files:
            args = self.full_args
            args += ['--show-policy-group',
                     '--policy-groups', os.path.basename(f),
                     '--include-policy-groups-dir=%s' %
                     os.path.join(self.test_include_dir, 'policygroups')]
            (self.options, self.args) = easyprof.parse_args(args)
            easyp = easyprof.AppArmorEasyProfile(None, self.options)

            path = os.path.join(easyp.dirs['policygroups_include'], f)
            self.assertTrue(os.path.exists(path), "Could not find '%s'" % path)
            open(path).read()

            bn = os.path.basename(f)
            # setup() copies everything in the include prefixed with inc_
            self.assertTrue(bn.startswith('inc_'),
                            "'%s' does not start with 'inc_'" % bn)

#
# Manifest file argument tests
#
    def test_manifest_argument(self):
        '''Test manifest argument'''

        # setup our manifest
        self.manifest = os.path.join(self.tmpdir, 'manifest.json')
        contents = '''
{"security": {"domain.reverse.appname": {"name": "simple-app"}}}
'''
        open(self.manifest, 'w').write(contents)

        args = self.full_args
        args.extend(['--manifest', self.manifest])
        easyprof.parse_args(args)

    def _manifest_conflicts(self, opt, value):
        '''Helper for conflicts tests'''
        # setup our manifest
        self.manifest = os.path.join(self.tmpdir, 'manifest.json')
        contents = '''
{"security": {"domain.reverse.appname": {"binary": /nonexistent"}}}
'''
        open(self.manifest, 'w').write(contents)

        # opt first
        args = self.full_args
        args.extend([opt, value, '--manifest', self.manifest])
        raised = False
        try:
            easyprof.parse_args(args, InterceptingOptionParser())
        except InterceptedError:
            raised = True

        self.assertTrue(raised, msg="%s and manifest arguments did not " \
                                    "raise a parse error" % opt)

        # manifest first
        args = self.full_args
        args.extend(['--manifest', self.manifest, opt, value])
        raised = False
        try:
            easyprof.parse_args(args, InterceptingOptionParser())
        except InterceptedError:
            raised = True

        self.assertTrue(raised, msg="%s and manifest arguments did not " \
                                    "raise a parse error" % opt)

    def test_manifest_conflicts_profilename(self):
        '''Test manifest arg conflicts with profile_name arg'''
        self._manifest_conflicts("--profile-name", "simple-app")

    def test_manifest_conflicts_copyright(self):
        '''Test manifest arg conflicts with copyright arg'''
        self._manifest_conflicts("--copyright", "2013-01-01")

    def test_manifest_conflicts_author(self):
        '''Test manifest arg conflicts with author arg'''
        self._manifest_conflicts("--author", "Foo Bar")

    def test_manifest_conflicts_comment(self):
        '''Test manifest arg conflicts with comment arg'''
        self._manifest_conflicts("--comment", "some comment")

    def test_manifest_conflicts_abstractions(self):
        '''Test manifest arg conflicts with abstractions arg'''
        self._manifest_conflicts("--abstractions", "base")

    def test_manifest_conflicts_read_path(self):
        '''Test manifest arg conflicts with read-path arg'''
        self._manifest_conflicts("--read-path", "/etc/passwd")

    def test_manifest_conflicts_write_path(self):
        '''Test manifest arg conflicts with write-path arg'''
        self._manifest_conflicts("--write-path", "/tmp/foo")

    def test_manifest_conflicts_policy_groups(self):
        '''Test manifest arg conflicts with policy-groups arg'''
        self._manifest_conflicts("--policy-groups", "opt-application")

    def test_manifest_conflicts_name(self):
        '''Test manifest arg conflicts with name arg'''
        self._manifest_conflicts("--name", "foo")

    def test_manifest_conflicts_template_var(self):
        '''Test manifest arg conflicts with template-var arg'''
        self._manifest_conflicts("--template-var", "foo")

    def test_manifest_conflicts_policy_version(self):
        '''Test manifest arg conflicts with policy-version arg'''
        self._manifest_conflicts("--policy-version", "1.0")

    def test_manifest_conflicts_policy_vendor(self):
        '''Test manifest arg conflicts with policy-vendor arg'''
        self._manifest_conflicts("--policy-vendor", "somevendor")


#
# Test genpolicy
#

    def _gen_policy(self, name=None, template=None, extra_args=[]):
        '''Generate a policy'''
        # Build up our args
        args = self.full_args

        if template == None:
            args.append('--template=%s' % self.test_template)
        else:
            args.append('--template=%s' % template)

        if name != None:
            args.append('--name=%s' % name)

        if len(extra_args) > 0:
            args += extra_args

        args.append(self.binary)

        # Now parse our args
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)
        params = easyprof.gen_policy_params(self.binary, self.options)
        p = easyp.gen_policy(**params)

        # We always need to check for these
        search_terms = [self.binary]
        if name != None:
            search_terms.append(name)

        if template == None:
            search_terms.append(self.test_template)

        for s in search_terms:
            self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))

        # ###NAME### should be replaced with self.binary or 'name'. Check for that
        inv_s = '###NAME###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

        if debugging:
            sys.stdout.write("%s\n" % p)

        return p

    def _gen_manifest_policy(self, manifest, use_security_prefix=True):
        # Build up our args
        args = self.full_args
        args.append("--manifest=/dev/null")

        (self.options, self.args) = easyprof.parse_args(args)
        (binary, self.options) = easyprof.parse_manifest(manifest.emit_json(use_security_prefix), self.options)[0]
        easyp = easyprof.AppArmorEasyProfile(binary, self.options)
        params = easyprof.gen_policy_params(binary, self.options)
        p = easyp.gen_policy(**params)

        # ###NAME### should be replaced with self.binary or 'name'. Check for that
        inv_s = '###NAME###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

        if debugging:
            sys.stdout.write("%s\n" % p)

        return p

    def test__is_safe(self):
        '''Test _is_safe()'''
        bad = [
               "/../../../../etc/passwd",
               "abstraction with spaces",
               "semicolon;bad",
               "bad\x00baz",
               "foo/bar",
               "foo'bar",
               'foo"bar',
              ]
        for s in bad:
            self.assertFalse(easyprof._is_safe(s), "'%s' should be bad" %s)

    def test_genpolicy_templates_abspath(self):
        '''Test genpolicy (abspath to template)'''
        # create a new template
        template = os.path.join(self.tmpdir, "test-abspath-template")
        shutil.copy(os.path.join(self.tmpdir, 'templates', self.test_template), template)
        contents = open(template).read()
        test_string = "#teststring"
        open(template, 'w').write(contents + "\n%s\n" % test_string)

        p = self._gen_policy(template=template)

        for s in [self.test_template, test_string]:
            self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))

    def test_genpolicy_templates_system(self):
        '''Test genpolicy (system template)'''
        self._gen_policy()

    def test_genpolicy_templates_nonexistent(self):
        '''Test genpolicy (nonexistent template)'''
        try:
            self._gen_policy(template=os.path.join(self.tmpdir, "/nonexistent"))
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("template should be invalid")

    def test_genpolicy_name(self):
        '''Test genpolicy (name)'''
        self._gen_policy(name='test-foo')

    def test_genpolicy_comment(self):
        '''Test genpolicy (comment)'''
        s = "test comment"
        p = self._gen_policy(extra_args=['--comment=%s' % s])
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###COMMENT###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_author(self):
        '''Test genpolicy (author)'''
        s = "Archibald Poindexter"
        p = self._gen_policy(extra_args=['--author=%s' % s])
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###AUTHOR###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_copyright(self):
        '''Test genpolicy (copyright)'''
        s = "2112/01/01"
        p = self._gen_policy(extra_args=['--copyright=%s' % s])
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###COPYRIGHT###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_abstractions(self):
        '''Test genpolicy (single abstraction)'''
        s = "nameservice"
        p = self._gen_policy(extra_args=['--abstractions=%s' % s])
        search = "#include <abstractions/%s>" % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###ABSTRACTIONS###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_abstractions_multiple(self):
        '''Test genpolicy (multiple abstractions)'''
        abstractions = "authentication,X,user-tmp"
        p = self._gen_policy(extra_args=['--abstractions=%s' % abstractions])
        for s in abstractions.split(','):
            search = "#include <abstractions/%s>" % s
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###ABSTRACTIONS###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_abstractions_bad(self):
        '''Test genpolicy (abstractions - bad values)'''
        bad = [
               "nonexistent",
               "/../../../../etc/passwd",
               "abstraction with spaces",
              ]
        for s in bad:
            try:
                self._gen_policy(extra_args=['--abstractions=%s' % s])
            except easyprof.AppArmorException:
                continue
            except Exception:
                raise
            raise Exception ("abstraction '%s' should be invalid" % s)

    def _create_tmp_base_dir(self, prefix='', abstractions=[], tunables=[]):
        '''Create a temporary base dir layout'''
        base_name = 'apparmor.d'
        if prefix:
            base_name = '%s-%s' % (prefix, base_name)
        base_dir = os.path.join(self.tmpdir, base_name)
        abstractions_dir = os.path.join(base_dir, 'abstractions')
        tunables_dir = os.path.join(base_dir, 'tunables')

        os.mkdir(base_dir)
        os.mkdir(abstractions_dir)
        os.mkdir(tunables_dir)

        for f in abstractions:
            contents = '''
  # Abstraction file for testing
  /%s r,
''' % (f)
            open(os.path.join(abstractions_dir, f), 'w').write(contents)

        for f in tunables:
            contents = '''
# Tunable file for testing
@{AA_TEST_%s}=foo
''' % (f)
            open(os.path.join(tunables_dir, f), 'w').write(contents)

        return base_dir

    def test_genpolicy_abstractions_custom_base(self):
        '''Test genpolicy (custom base dir)'''
        abstraction = "custom-base-dir-test-abstraction"
        # The default template #includes the base abstraction and global
        # tunable so we need to create placeholders
        base = self._create_tmp_base_dir(abstractions=['base', abstraction], tunables=['global'])
        args = ['--abstractions=%s' % abstraction, '--base=%s' % base]

        p = self._gen_policy(extra_args=args)
        search = "#include <abstractions/%s>" % abstraction
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###ABSTRACTIONS###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_abstractions_custom_base_bad(self):
        '''Test genpolicy (custom base dir - bad base dirs)'''
        abstraction = "custom-base-dir-test-abstraction"
        bad = [ None, '/etc/apparmor.d', '/' ]
        for base in bad:
            try:
                args = ['--abstractions=%s' % abstraction]
                if base:
                    args.append('--base=%s' % base)
                self._gen_policy(extra_args=args)
            except easyprof.AppArmorException:
                continue
            except Exception:
                raise
            raise Exception ("abstraction '%s' should be invalid" % abstraction)

    def test_genpolicy_abstractions_custom_include(self):
        '''Test genpolicy (custom include dir)'''
        abstraction = "custom-include-dir-test-abstraction"
        # No need to create placeholders for the base abstraction or global
        # tunable since we're not adjusting the base directory
        include = self._create_tmp_base_dir(abstractions=[abstraction])
        args = ['--abstractions=%s' % abstraction, '--Include=%s' % include]
        p = self._gen_policy(extra_args=args)
        search = "#include <abstractions/%s>" % abstraction
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###ABSTRACTIONS###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_abstractions_custom_include_bad(self):
        '''Test genpolicy (custom include dir - bad include dirs)'''
        abstraction = "custom-include-dir-test-abstraction"
        bad = [ None, '/etc/apparmor.d', '/' ]
        for include in bad:
            try:
                args = ['--abstractions=%s' % abstraction]
                if include:
                    args.append('--Include=%s' % include)
                self._gen_policy(extra_args=args)
            except easyprof.AppArmorException:
                continue
            except Exception:
                raise
            raise Exception ("abstraction '%s' should be invalid" % abstraction)

    def test_genpolicy_profile_name_bad(self):
        '''Test genpolicy (profile name - bad values)'''
        bad = [
               "/../../../../etc/passwd",
               "../../../../etc/passwd",
               "profile name with spaces",
              ]
        for s in bad:
            try:
                self._gen_policy(extra_args=['--profile-name=%s' % s])
            except easyprof.AppArmorException:
                continue
            except Exception:
                raise
            raise Exception ("profile_name '%s' should be invalid" % s)

    def test_genpolicy_policy_group_bad(self):
        '''Test genpolicy (policy group - bad values)'''
        bad = [
               "/../../../../etc/passwd",
               "../../../../etc/passwd",
               "profile name with spaces",
              ]
        for s in bad:
            try:
                self._gen_policy(extra_args=['--policy-groups=%s' % s])
            except easyprof.AppArmorException:
                continue
            except Exception:
                raise
            raise Exception ("policy group '%s' should be invalid" % s)

    def test_genpolicy_policygroups(self):
        '''Test genpolicy (single policygroup)'''
        groups = self.test_policygroup
        p = self._gen_policy(extra_args=['--policy-groups=%s' % groups])

        for s in ['#include <abstractions/nameservice>', '#include <abstractions/gnome>']:
            self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###POLICYGROUPS###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_policygroups_multiple(self):
        '''Test genpolicy (multiple policygroups)'''
        test_policygroup2 = "test-policygroup2"
        contents = '''
  # %s
  #include <abstractions/kde>
  #include <abstractions/openssl>
''' % (self.test_policygroup)
        open(os.path.join(self.tmpdir, 'policygroups', test_policygroup2), 'w').write(contents)

        groups = "%s,%s" % (self.test_policygroup, test_policygroup2)
        p = self._gen_policy(extra_args=['--policy-groups=%s' % groups])

        for s in ['#include <abstractions/nameservice>',
                  '#include <abstractions/gnome>',
                  '#include <abstractions/kde>',
                  '#include <abstractions/openssl>']:
            self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###POLICYGROUPS###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_policygroups_nonexistent(self):
        '''Test genpolicy (nonexistent policygroup)'''
        try:
            self._gen_policy(extra_args=['--policy-groups=nonexistent'])
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("policygroup should be invalid")

    def test_genpolicy_readpath_file(self):
        '''Test genpolicy (read-path file)'''
        s = "/opt/test-foo"
        p = self._gen_policy(extra_args=['--read-path=%s' % s])
        search = "%s rk," % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_readpath_home_file(self):
        '''Test genpolicy (read-path file in /home)'''
        s = "/home/*/test-foo"
        p = self._gen_policy(extra_args=['--read-path=%s' % s])
        search = "owner %s rk," % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_readpath_homevar_file(self):
        '''Test genpolicy (read-path file in @{HOME})'''
        s = "@{HOME}/test-foo"
        p = self._gen_policy(extra_args=['--read-path=%s' % s])
        search = "owner %s rk," % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_readpath_homedirs_file(self):
        '''Test genpolicy (read-path file in @{HOMEDIRS})'''
        s = "@{HOMEDIRS}/test-foo"
        p = self._gen_policy(extra_args=['--read-path=%s' % s])
        search = "owner %s rk," % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_readpath_dir(self):
        '''Test genpolicy (read-path directory/)'''
        s = "/opt/test-foo-dir/"
        p = self._gen_policy(extra_args=['--read-path=%s' % s])
        search_terms = ["%s rk," % s, "%s** rk," % s]
        for search in search_terms:
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_readpath_dir_glob(self):
        '''Test genpolicy (read-path directory/*)'''
        s = "/opt/test-foo-dir/*"
        p = self._gen_policy(extra_args=['--read-path=%s' % s])
        search_terms = ["%s rk," % os.path.dirname(s), "%s rk," % s]
        for search in search_terms:
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_readpath_dir_glob_all(self):
        '''Test genpolicy (read-path directory/**)'''
        s = "/opt/test-foo-dir/**"
        p = self._gen_policy(extra_args=['--read-path=%s' % s])
        search_terms = ["%s rk," % os.path.dirname(s), "%s rk," % s]
        for search in search_terms:
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_readpath_multiple(self):
        '''Test genpolicy (read-path multiple)'''
        paths = ["/opt/test-foo",
                 "/home/*/test-foo",
                 "@{HOME}/test-foo",
                 "@{HOMEDIRS}/test-foo",
                 "/opt/test-foo-dir/",
                 "/opt/test-foo-dir/*",
                 "/opt/test-foo-dir/**"]
        args = []
        search_terms = []
        for s in paths:
            args.append('--read-path=%s' % s)
            # This mimics easyprof.gen_path_rule()
            owner = ""
            if s.startswith('/home/') or s.startswith("@{HOME"):
                owner = "owner "
            if s.endswith('/'):
                search_terms.append("%s rk," % (s))
                search_terms.append("%s%s** rk," % (owner, s))
            elif s.endswith('/**') or s.endswith('/*'):
                search_terms.append("%s rk," % (os.path.dirname(s)))
                search_terms.append("%s%s rk," % (owner, s))
            else:
                search_terms.append("%s%s rk," % (owner, s))

        p = self._gen_policy(extra_args=args)
        for search in search_terms:
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_readpath_bad(self):
        '''Test genpolicy (read-path bad)'''
        s = "bar"
        try:
            self._gen_policy(extra_args=['--read-path=%s' % s])
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("read-path should be invalid")

    def test_genpolicy_writepath_file(self):
        '''Test genpolicy (write-path file)'''
        s = "/opt/test-foo"
        p = self._gen_policy(extra_args=['--write-path=%s' % s])
        search = "%s rwk," % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_writepath_home_file(self):
        '''Test genpolicy (write-path file in /home)'''
        s = "/home/*/test-foo"
        p = self._gen_policy(extra_args=['--write-path=%s' % s])
        search = "owner %s rwk," % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_writepath_homevar_file(self):
        '''Test genpolicy (write-path file in @{HOME})'''
        s = "@{HOME}/test-foo"
        p = self._gen_policy(extra_args=['--write-path=%s' % s])
        search = "owner %s rwk," % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_writepath_homedirs_file(self):
        '''Test genpolicy (write-path file in @{HOMEDIRS})'''
        s = "@{HOMEDIRS}/test-foo"
        p = self._gen_policy(extra_args=['--write-path=%s' % s])
        search = "owner %s rwk," % s
        self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_writepath_dir(self):
        '''Test genpolicy (write-path directory/)'''
        s = "/opt/test-foo-dir/"
        p = self._gen_policy(extra_args=['--write-path=%s' % s])
        search_terms = ["%s rwk," % s, "%s** rwk," % s]
        for search in search_terms:
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_writepath_dir_glob(self):
        '''Test genpolicy (write-path directory/*)'''
        s = "/opt/test-foo-dir/*"
        p = self._gen_policy(extra_args=['--write-path=%s' % s])
        search_terms = ["%s rwk," % os.path.dirname(s), "%s rwk," % s]
        for search in search_terms:
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_writepath_dir_glob_all(self):
        '''Test genpolicy (write-path directory/**)'''
        s = "/opt/test-foo-dir/**"
        p = self._gen_policy(extra_args=['--write-path=%s' % s])
        search_terms = ["%s rwk," % os.path.dirname(s), "%s rwk," % s]
        for search in search_terms:
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_writepath_multiple(self):
        '''Test genpolicy (write-path multiple)'''
        paths = ["/opt/test-foo",
                 "/home/*/test-foo",
                 "@{HOME}/test-foo",
                 "@{HOMEDIRS}/test-foo",
                 "/opt/test-foo-dir/",
                 "/opt/test-foo-dir/*",
                 "/opt/test-foo-dir/**"]
        args = []
        search_terms = []
        for s in paths:
            args.append('--write-path=%s' % s)
            # This mimics easyprof.gen_path_rule()
            owner = ""
            if s.startswith('/home/') or s.startswith("@{HOME"):
                owner = "owner "
            if s.endswith('/'):
                search_terms.append("%s rwk," % (s))
                search_terms.append("%s%s** rwk," % (owner, s))
            elif s.endswith('/**') or s.endswith('/*'):
                search_terms.append("%s rwk," % (os.path.dirname(s)))
                search_terms.append("%s%s rwk," % (owner, s))
            else:
                search_terms.append("%s%s rwk," % (owner, s))

        p = self._gen_policy(extra_args=args)
        for search in search_terms:
            self.assertTrue(search in p, "Could not find '%s' in:\n%s" % (search, p))
        inv_s = '###READPATH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_writepath_bad(self):
        '''Test genpolicy (write-path bad)'''
        s = "bar"
        try:
            self._gen_policy(extra_args=['--write-path=%s' % s])
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("write-path should be invalid")

    def test_genpolicy_templatevar(self):
        '''Test genpolicy (template-var single)'''
        s = "@{FOO}=bar"
        p = self._gen_policy(extra_args=['--template-var=%s' % s])
        k, v = s.split('=')
        s = '%s="%s"' % (k, v)
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###TEMPLATEVAR###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_templatevar_multiple(self):
        '''Test genpolicy (template-var multiple)'''
        variables = ['@{FOO}=bar', '@{BAR}=baz']
        args = []
        for s in variables:
            args.append('--template-var=%s' % s)

        p = self._gen_policy(extra_args=args)
        for s in variables:
            k, v = s.split('=')
            s = '%s="%s"' % (k, v)
            self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
            inv_s = '###TEMPLATEVAR###'
            self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_templatevar_bad(self):
        '''Test genpolicy (template-var - bad values)'''
        bad = [
               "{FOO}=bar",
               "@FOO}=bar",
               "@{FOO=bar",
               "FOO=bar",
               "@FOO=bar",
               "@{FOO}=/../../../etc/passwd",
               "@{FOO}=bar=foo",
               "@{FOO;BAZ}=bar",
               '@{FOO}=bar"baz',
              ]
        for s in bad:
            try:
                self._gen_policy(extra_args=['--template-var=%s' % s])
            except easyprof.AppArmorException:
                continue
            except Exception:
                raise
            raise Exception ("template-var should be invalid")

    def test_genpolicy_invalid_template_policy(self):
        '''Test genpolicy (invalid template policy)'''
        # create a new template
        template = os.path.join(self.tmpdir, "test-invalid-template")
        shutil.copy(os.path.join(self.tmpdir, 'templates', self.test_template), template)
        contents = open(template).read()
        bad_pol = ""
        bad_string = "bzzzt"
        for line in contents.splitlines():
            if '}' in line:
                bad_pol += bad_string
            else:
                bad_pol += line
            bad_pol += "\n"
        open(template, 'w').write(bad_pol)
        try:
            self._gen_policy(template=template)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("policy should be invalid")

    def test_genpolicy_no_binary_without_profile_name(self):
        '''Test genpolicy (no binary with no profile name)'''
        try:
            easyprof.gen_policy_params(None, self.options)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("No binary or profile name should have been invalid")

    def test_genpolicy_with_binary_with_profile_name(self):
        '''Test genpolicy (binary with profile name)'''
        profile_name = "some-profile-name"
        p = self._gen_policy(extra_args=['--profile-name=%s' % profile_name])
        s = 'profile "%s" "%s" {' % (profile_name, self.binary)
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###PROFILEATTACH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_with_binary_without_profile_name(self):
        '''Test genpolicy (binary without profile name)'''
        p = self._gen_policy()
        s = '"%s" {' % (self.binary)
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###PROFILEATTACH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_genpolicy_without_binary_with_profile_name(self):
        '''Test genpolicy (no binary with profile name)'''
        profile_name = "some-profile-name"
        args = self.full_args
        args.append('--profile-name=%s' % profile_name)
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(None, self.options)
        params = easyprof.gen_policy_params(None, self.options)
        p = easyp.gen_policy(**params)
        s = 'profile "%s" {' % (profile_name)
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###PROFILEATTACH###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

# manifest tests

    def test_gen_manifest_policy_with_binary_with_profile_name(self):
        '''Test gen_manifest_policy (binary with profile name)'''
        m = Manifest("test_gen_manifest_policy")
        m.add_binary(self.ls)
        self._gen_manifest_policy(m)

    def test_gen_manifest_policy_without_binary_with_profile_name(self):
        '''Test gen_manifest_policy (no binary with profile name)'''
        m = Manifest("test_gen_manifest_policy")
        self._gen_manifest_policy(m)

    def test_gen_manifest_policy_templates_system(self):
        '''Test gen_manifest_policy (system template)'''
        m = Manifest("test_gen_manifest_policy")
        m.add_template(self.test_template)
        self._gen_manifest_policy(m)

    def test_gen_manifest_policy_templates_system_noprefix(self):
        '''Test gen_manifest_policy (system template, no security prefix)'''
        m = Manifest("test_gen_manifest_policy")
        m.add_template(self.test_template)
        self._gen_manifest_policy(m, use_security_prefix=False)

    def test_gen_manifest_abs_path_template(self):
        '''Test gen_manifest_policy (abs path template)'''
        m = Manifest("test_gen_manifest_policy")
        m.add_template("/etc/shadow")
        try:
            self._gen_manifest_policy(m)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("abs path template name should be invalid")

    def test_gen_manifest_escape_path_templates(self):
        '''Test gen_manifest_policy (esc path template)'''
        m = Manifest("test_gen_manifest_policy")
        m.add_template("../../../../../../../../etc/shadow")
        try:
            self._gen_manifest_policy(m)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("../ template name should be invalid")

    def test_gen_manifest_policy_templates_nonexistent(self):
        '''Test gen manifest policy (nonexistent template)'''
        m = Manifest("test_gen_manifest_policy")
        m.add_template("nonexistent")
        try:
            self._gen_manifest_policy(m)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("template should be invalid")

    def test_gen_manifest_policy_comment(self):
        '''Test gen manifest policy (comment)'''
        s = "test comment"
        m = Manifest("test_gen_manifest_policy")
        m.add_comment(s)
        p = self._gen_manifest_policy(m)
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###COMMENT###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_gen_manifest_policy_author(self):
        '''Test gen manifest policy (author)'''
        s = "Archibald Poindexter"
        m = Manifest("test_gen_manifest_policy")
        m.add_author(s)
        p = self._gen_manifest_policy(m)
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###AUTHOR###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_gen_manifest_policy_copyright(self):
        '''Test genpolicy (copyright)'''
        s = "2112/01/01"
        m = Manifest("test_gen_manifest_policy")
        m.add_copyright(s)
        p = self._gen_manifest_policy(m)
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###COPYRIGHT###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_gen_manifest_policy_policygroups(self):
        '''Test gen manifest policy (single policygroup)'''
        groups = self.test_policygroup
        m = Manifest("test_gen_manifest_policy")
        m.add_policygroups(groups)
        p = self._gen_manifest_policy(m)

        for s in ['#include <abstractions/nameservice>', '#include <abstractions/gnome>']:
            self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###POLICYGROUPS###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_gen_manifest_policy_policygroups_multiple(self):
        '''Test genpolicy (multiple policygroups)'''
        test_policygroup2 = "test-policygroup2"
        contents = '''
  # %s
  #include <abstractions/kde>
  #include <abstractions/openssl>
''' % (self.test_policygroup)
        open(os.path.join(self.tmpdir, 'policygroups', test_policygroup2), 'w').write(contents)

        groups = "%s,%s" % (self.test_policygroup, test_policygroup2)
        m = Manifest("test_gen_manifest_policy")
        m.add_policygroups(groups)
        p = self._gen_manifest_policy(m)

        for s in ['#include <abstractions/nameservice>',
                  '#include <abstractions/gnome>',
                  '#include <abstractions/kde>',
                  '#include <abstractions/openssl>']:
            self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###POLICYGROUPS###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_gen_manifest_policy_policygroups_nonexistent(self):
        '''Test gen manifest policy (nonexistent policygroup)'''
        groups = "nonexistent"
        m = Manifest("test_gen_manifest_policy")
        m.add_policygroups(groups)
        try:
            self._gen_manifest_policy(m)
        except easyprof.AppArmorException:
            return
        except Exception:
            raise
        raise Exception ("policygroup should be invalid")

    def test_gen_manifest_policy_templatevar(self):
        '''Test gen manifest policy (template-var single)'''
        m = Manifest("test_gen_manifest_policy")
        m.add_template_variable("FOO", "bar")
        p = self._gen_manifest_policy(m)
        s = '@{FOO}="bar"'
        self.assertTrue(s in p, "Could not find '%s' in:\n%s" % (s, p))
        inv_s = '###TEMPLATEVAR###'
        self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_gen_manifest_policy_templatevar_multiple(self):
        '''Test gen manifest policy (template-var multiple)'''
        variables = [["FOO", "bar"], ["BAR", "baz"]]
        m = Manifest("test_gen_manifest_policy")
        for s in variables:
            m.add_template_variable(s[0], s[1])

        p = self._gen_manifest_policy(m)
        for s in variables:
            str_s = '@{%s}="%s"' % (s[0], s[1])
            self.assertTrue(str_s in p, "Could not find '%s' in:\n%s" % (str_s, p))
            inv_s = '###TEMPLATEVAR###'
            self.assertFalse(inv_s in p, "Found '%s' in :\n%s" % (inv_s, p))

    def test_gen_manifest_policy_invalid_keys(self):
        '''Test gen manifest policy (invalid keys)'''
        keys = ['config_file',
                'debug',
                'help',
                'list-templates',
                'list_templates',
                'show-template',
                'show_template',
                'list-policy-groups',
                'list_policy_groups',
                'show-policy-group',
                'show_policy_group',
                'templates-dir',
                'templates_dir',
                'policy-groups-dir',
                'policy_groups_dir',
                'nonexistent',
                'no_verify',
               ]

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        for k in keys:
            security = dict()
            security["profile_name"] = "test-app"
            security[k] = "bad"
            j = json.dumps(security, indent=2)
            try:
                easyprof.parse_manifest(j, self.options)
            except easyprof.AppArmorException:
                continue
            raise Exception ("'%s' should be invalid" % k)

    def test_gen_manifest(self):
        '''Test gen_manifest'''
        #  this should come from manpage
        m = '''{
  "security": {
    "profiles": {
      "com.example.foo": {
        "abstractions": [
          "audio",
          "gnome"
        ],
        "author": "Your Name",
        "binary": "/opt/foo/**",
        "comment": "Unstructured single-line comment",
        "copyright": "Unstructured single-line copyright statement",
        "name": "My Foo App",
        "policy_groups": [
          "opt-application",
          "user-application"
        ],
        "policy_vendor": "somevendor",
        "policy_version": 1.0,
        "read_path": [
          "/tmp/foo_r",
          "/tmp/bar_r/"
        ],
        "template": "user-application",
        "template_variables": {
          "APPNAME": "foo",
          "VAR1": "bar",
          "VAR2": "baz"
        },
        "write_path": [
          "/tmp/foo_w",
          "/tmp/bar_w/"
        ]
      }
    }
  }
}'''

        for d in ['policygroups', 'templates']:
            shutil.copytree(os.path.join(self.tmpdir, d),
                            os.path.join(self.tmpdir, d, "somevendor/1.0"))

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        (binary, self.options) = easyprof.parse_manifest(m, self.options)[0]
        easyp = easyprof.AppArmorEasyProfile(binary, self.options)
        params = easyprof.gen_policy_params(binary, self.options)

        # verify we get the same manifest back
        man_new = easyp.gen_manifest(params)
        self.assertEquals(m, man_new)

    def test_gen_manifest_ubuntu(self):
        '''Test gen_manifest (ubuntu)'''
        # this should be based on the manpage (but use existing policy_groups
        # and template
        m = '''{
  "security": {
    "profiles": {
      "com.ubuntu.developer.myusername.MyCoolApp": {
        "name": "MyCoolApp",
        "policy_groups": [
          "opt-application",
          "user-application"
        ],
        "policy_vendor": "ubuntu",
        "policy_version": 1.0,
        "template": "user-application",
        "template_variables": {
          "APPNAME": "MyCoolApp",
          "APPVERSION": "0.1.2"
        }
      }
    }
  }
}'''

        for d in ['policygroups', 'templates']:
            shutil.copytree(os.path.join(self.tmpdir, d),
                            os.path.join(self.tmpdir, d, "ubuntu/1.0"))

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        (binary, self.options) = easyprof.parse_manifest(m, self.options)[0]
        easyp = easyprof.AppArmorEasyProfile(binary, self.options)
        params = easyprof.gen_policy_params(binary, self.options)

        # verify we get the same manifest back
        man_new = easyp.gen_manifest(params)
        self.assertEquals(m, man_new)

    def test_parse_manifest_no_version(self):
        '''Test parse_manifest (vendor with no version)'''
        #  this should come from manpage
        m = '''{
  "security": {
    "profiles": {
      "com.ubuntu.developer.myusername.MyCoolApp": {
        "policy_groups": [
          "opt-application",
          "user-application"
        ],
        "policy_vendor": "ubuntu",
        "template": "user-application",
        "template_variables": {
          "APPNAME": "MyCoolApp",
          "APPVERSION": "0.1.2"
        }
      }
    }
  }
}'''

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        (binary, self.options) = easyprof.parse_manifest(m, self.options)[0]
        try:
            easyprof.AppArmorEasyProfile(binary, self.options)
        except easyprof.AppArmorException:
            return
        raise Exception ("Should have failed on missing version")

    def test_parse_manifest_no_vendor(self):
        '''Test parse_manifest (version with no vendor)'''
        #  this should come from manpage
        m = '''{
  "security": {
    "profiles": {
      "com.ubuntu.developer.myusername.MyCoolApp": {
        "policy_groups": [
          "opt-application",
          "user-application"
        ],
        "policy_version": 1.0,
        "template": "user-application",
        "template_variables": {
          "APPNAME": "MyCoolApp",
          "APPVERSION": "0.1.2"
        }
      }
    }
  }
}'''

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        (binary, self.options) = easyprof.parse_manifest(m, self.options)[0]
        try:
            easyprof.AppArmorEasyProfile(binary, self.options)
        except easyprof.AppArmorException:
            return
        raise Exception ("Should have failed on missing vendor")

    def test_parse_manifest_multiple(self):
        '''Test parse_manifest_multiple'''
        m = '''{
  "security": {
    "profiles": {
      "com.example.foo": {
        "abstractions": [
          "audio",
          "gnome"
        ],
        "author": "Your Name",
        "binary": "/opt/foo/**",
        "comment": "Unstructured single-line comment",
        "copyright": "Unstructured single-line copyright statement",
        "name": "My Foo App",
        "policy_groups": [
          "opt-application",
          "user-application"
        ],
        "read_path": [
          "/tmp/foo_r",
          "/tmp/bar_r/"
        ],
        "template": "user-application",
        "template_variables": {
          "APPNAME": "foo",
          "VAR1": "bar",
          "VAR2": "baz"
        },
        "write_path": [
          "/tmp/foo_w",
          "/tmp/bar_w/"
        ]
      },
      "com.ubuntu.developer.myusername.MyCoolApp": {
        "policy_groups": [
          "opt-application"
        ],
        "policy_vendor": "ubuntu",
        "policy_version": 1.0,
        "template": "user-application",
        "template_variables": {
          "APPNAME": "MyCoolApp",
          "APPVERSION": "0.1.2"
        }
      }
    }
  }
}'''

        for d in ['policygroups', 'templates']:
            shutil.copytree(os.path.join(self.tmpdir, d),
                            os.path.join(self.tmpdir, d, "ubuntu/1.0"))

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        profiles = easyprof.parse_manifest(m, self.options)
        for (binary, options) in profiles:
            easyp = easyprof.AppArmorEasyProfile(binary, options)
            params = easyprof.gen_policy_params(binary, options)
            easyp.gen_manifest(params)
            easyp.gen_policy(**params)


# verify manifest tests
    def _verify_manifest(self, m, expected, invalid=False):
        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        try:
            (binary, options) = easyprof.parse_manifest(m, self.options)[0]
        except easyprof.AppArmorException:
            if invalid:
                return
            raise
        params = easyprof.gen_policy_params(binary, options)
        if expected:
            self.assertTrue(easyprof.verify_manifest(params, args), "params=%s\nmanifest=%s" % (params,m))
        else:
            self.assertFalse(easyprof.verify_manifest(params, args), "params=%s\nmanifest=%s" % (params,m))

    def test_verify_manifest_full(self):
        '''Test verify_manifest (full)'''
        m = '''{
  "security": {
    "profiles": {
      "com.example.foo": {
        "abstractions": [
          "base"
        ],
        "author": "Your Name",
        "binary": "/opt/com.example/foo/**",
        "comment": "some free-form single-line comment",
        "copyright": "Unstructured single-line copyright statement",
        "name": "foo",
        "policy_groups": [
          "user-application",
          "opt-application"
        ],
        "template": "user-application",
        "template_variables": {
          "OK1": "foo",
          "OK2": "com.example.foo"
        }
      }
    }
  }
}'''
        self._verify_manifest(m, expected=True)

    def test_verify_manifest_full_bad(self):
        '''Test verify_manifest (full bad)'''
        m = '''{
  "security": {
    "profiles": {
      "/com.example.foo": {
         "abstractions": [
          "audio",
          "gnome"
        ],
        "author": "Your Name",
        "binary": "/usr/foo/**",
        "comment": "some free-form single-line comment",
        "copyright": "Unstructured single-line copyright statement",
        "name": "foo",
        "policy_groups": [
          "user-application",
          "opt-application"
        ],
        "read_path": [
          "/tmp/foo_r",
          "/tmp/bar_r/"
        ],
        "template": "user-application",
        "template_variables": {
          "VAR1": "f*o",
          "VAR2": "*foo",
          "VAR3": "fo*",
          "VAR4": "b{ar",
          "VAR5": "b{a,r}",
          "VAR6": "b}ar",
          "VAR7": "bar[0-9]",
          "VAR8": "b{ar",
          "VAR9": "/tmp/../etc/passwd"
        },
        "write_path": [
          "/tmp/foo_w",
          "/tmp/bar_w/"
        ]
      }
    }
  }
}'''

        self._verify_manifest(m, expected=False, invalid=True)

    def test_verify_manifest_binary(self):
        '''Test verify_manifest (binary in /usr)'''
        m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "binary": "/usr/foo/**",
     "template": "user-application"
    }
   }
  }
}'''
        self._verify_manifest(m, expected=True)

    def test_verify_manifest_profile_profile_name_bad(self):
        '''Test verify_manifest (bad profile_name)'''
        m = '''{
  "security": {
   "profiles": {
    "/foo": {
     "binary": "/opt/com.example/foo/**",
     "template": "user-application"
    }
   }
  }
}'''
        self._verify_manifest(m, expected=False, invalid=True)

        m = '''{
  "security": {
   "profiles": {
    "bin/*": {
     "binary": "/opt/com.example/foo/**",
     "template": "user-application"
    }
   }
  }
}'''
        self._verify_manifest(m, expected=False)

    def test_verify_manifest_profile_profile_name(self):
        '''Test verify_manifest (profile_name)'''
        m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "binary": "/opt/com.example/foo/**",
     "template": "user-application"
    }
   }
  }
}'''
        self._verify_manifest(m, expected=True)

    def test_verify_manifest_profile_abstractions(self):
        '''Test verify_manifest (abstractions)'''
        m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "binary": "/opt/com.example/foo/**",
     "template": "user-application",
     "abstractions": [
       "base"
     ]
    }
   }
  }
}'''
        self._verify_manifest(m, expected=True)

    def test_verify_manifest_profile_abstractions_bad(self):
        '''Test verify_manifest (bad abstractions)'''
        m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "binary": "/opt/com.example/foo/**",
     "template": "user-application",
     "abstractions": [
       "user-tmp"
     ]
    }
   }
  }
}'''
        self._verify_manifest(m, expected=False)

    def test_verify_manifest_profile_template_var(self):
        '''Test verify_manifest (good template_var)'''
        m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "binary": "/opt/com.example/something with spaces/**",
     "template": "user-application",
     "template_variables": {
       "OK1": "foo",
       "OK2": "com.example.foo",
       "OK3": "something with spaces"
     }
    }
   }
  }
}'''
        self._verify_manifest(m, expected=True)

    def test_verify_manifest_profile_template_var_bad(self):
        '''Test verify_manifest (bad template_var)'''
        for v in ['"VAR1": "f*o"',
                  '"VAR2": "*foo"',
                  '"VAR3": "fo*"',
                  '"VAR4": "b{ar"',
                  '"VAR5": "b{a,r}"',
                  '"VAR6": "b}ar"',
                  '"VAR7": "bar[0-9]"',
                  '"VAR8": "b{ar"',
                  '"VAR9": "foo/bar"' # this is valid, but potentially unsafe
                  ]:
            m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "binary": "/opt/com.example/foo/**",
    "template": "user-application",
     "template_variables": {
       %s
     }
    }
   }
  }
}''' % v
            self._verify_manifest(m, expected=False)

    def test_manifest_invalid(self):
        '''Test invalid manifest (parse error)'''
        m = '''{
  "security": {
   "com.example.foo": {
    "binary": "/opt/com.example/foo/**",
    "template": "user-application",
    "abstractions": [
      "base"
    ]
}'''
        self._verify_manifest(m, expected=False, invalid=True)

    def test_manifest_invalid2(self):
        '''Test invalid manifest (profile_name is not key)'''
        m = '''{
  "security": {
    "binary": "/opt/com.example/foo/**",
    "template": "user-application",
    "abstractions": [
      "base"
    ]
  }
}'''
        self._verify_manifest(m, expected=False, invalid=True)

    def test_manifest_invalid3(self):
        '''Test invalid manifest (profile_name in dict)'''
        m = '''{
  "security": {
    "binary": "/opt/com.example/foo/**",
    "template": "user-application",
    "abstractions": [
      "base"
    ],
    "profile_name": "com.example.foo"
  }
}'''
        self._verify_manifest(m, expected=False, invalid=True)

    def test_manifest_invalid4(self):
        '''Test invalid manifest (bad path in template var)'''
        for v in ['"VAR1": "/tmp/../etc/passwd"',
                  '"VAR2": "./"',
                  '"VAR3": "foo\"bar"',
                  '"VAR4": "foo//bar"',
                  ]:
            m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "binary": "/opt/com.example/foo/**",
     "template": "user-application",
     "template_variables": {
       %s
     }
    }
   }
  }
}''' % v
            args = self.full_args
            args.append("--manifest=/dev/null")
            (self.options, self.args) = easyprof.parse_args(args)
            (binary, options) = easyprof.parse_manifest(m, self.options)[0]
            params = easyprof.gen_policy_params(binary, options)
            try:
                easyprof.verify_manifest(params)
            except easyprof.AppArmorException:
                return

            raise Exception ("Should have failed with invalid variable declaration")


# policy version tests
    def test_policy_vendor_manifest_nonexistent(self):
        '''Test policy vendor via manifest (nonexistent)'''
        m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "policy_vendor": "nonexistent",
     "policy_version": 1.0,
     "binary": "/opt/com.example/foo/**",
     "template": "user-application"
    }
   }
  }
}'''
        # Build up our args
        args = self.full_args
        args.append("--manifest=/dev/null")

        (self.options, self.args) = easyprof.parse_args(args)
        (binary, self.options) = easyprof.parse_manifest(m, self.options)[0]
        try:
            easyprof.AppArmorEasyProfile(binary, self.options)
        except easyprof.AppArmorException:
            return

        raise Exception ("Should have failed with non-existent directory")

    def test_policy_version_manifest(self):
        '''Test policy version via manifest (good)'''
        policy_vendor = "somevendor"
        policy_version = "1.0"
        policy_subdir = "%s/%s" % (policy_vendor, policy_version)
        m = '''{
  "security": {
   "profiles": {
    "com.example.foo": {
     "policy_vendor": "%s",
     "policy_version": %s,
     "binary": "/opt/com.example/foo/**",
     "template": "user-application"
    }
   }
  }
}''' % (policy_vendor, policy_version)
        for d in ['policygroups', 'templates']:
            shutil.copytree(os.path.join(self.tmpdir, d),
                            os.path.join(self.tmpdir, d, policy_subdir))

        # Build up our args
        args = self.full_args
        args.append("--manifest=/dev/null")

        (self.options, self.args) = easyprof.parse_args(args)
        (binary, self.options) = easyprof.parse_manifest(m, self.options)[0]
        easyp = easyprof.AppArmorEasyProfile(binary, self.options)

        tdir = os.path.join(self.tmpdir, 'templates', policy_subdir)
        for t in easyp.get_templates():
            self.assertTrue(t.startswith(tdir))

        pdir = os.path.join(self.tmpdir, 'policygroups', policy_subdir)
        for p in easyp.get_policy_groups():
            self.assertTrue(p.startswith(pdir))

        params = easyprof.gen_policy_params(binary, self.options)
        easyp.gen_policy(**params)

    def test_policy_vendor_version_args(self):
        '''Test policy vendor and version via command line args (good)'''
        policy_version = "1.0"
        policy_vendor = "somevendor"
        policy_subdir = "%s/%s" % (policy_vendor, policy_version)

        # Create the directories
        for d in ['policygroups', 'templates']:
            shutil.copytree(os.path.join(self.tmpdir, d),
                            os.path.join(self.tmpdir, d, policy_subdir))

        # Build up our args
        args = self.full_args
        args.append("--policy-version=%s" % policy_version)
        args.append("--policy-vendor=%s" % policy_vendor)

        (self.options, self.args) = easyprof.parse_args(args)
        (self.options, self.args) = easyprof.parse_args(self.full_args + [self.binary])
        easyp = easyprof.AppArmorEasyProfile(self.binary, self.options)

        tdir = os.path.join(self.tmpdir, 'templates', policy_subdir)
        for t in easyp.get_templates():
            self.assertTrue(t.startswith(tdir), \
                            "'%s' does not start with '%s'" % (t, tdir))

        pdir = os.path.join(self.tmpdir, 'policygroups', policy_subdir)
        for p in easyp.get_policy_groups():
            self.assertTrue(p.startswith(pdir), \
                            "'%s' does not start with '%s'" % (p, pdir))

        params = easyprof.gen_policy_params(self.binary, self.options)
        easyp.gen_policy(**params)

    def test_policy_vendor_args_nonexistent(self):
        '''Test policy vendor via command line args (nonexistent)'''
        policy_vendor = "nonexistent"
        policy_version = "1.0"
        args = self.full_args
        args.append("--policy-version=%s" % policy_version)
        args.append("--policy-vendor=%s" % policy_vendor)

        (self.options, self.args) = easyprof.parse_args(args)
        (self.options, self.args) = easyprof.parse_args(self.full_args + [self.binary])
        try:
            easyprof.AppArmorEasyProfile(self.binary, self.options)
        except easyprof.AppArmorException:
            return

        raise Exception ("Should have failed with non-existent directory")

    def test_policy_version_args_bad(self):
        '''Test policy version via command line args (bad)'''
        bad = [
               "../../../../../../etc",
               "notanumber",
               "v1.0a",
               "-1",
              ]
        for policy_version in bad:
            args = self.full_args
            args.append("--policy-version=%s" % policy_version)
            args.append("--policy-vendor=somevendor")

            (self.options, self.args) = easyprof.parse_args(args)
            (self.options, self.args) = easyprof.parse_args(self.full_args + [self.binary])
            try:
                easyprof.AppArmorEasyProfile(self.binary, self.options)
            except easyprof.AppArmorException:
                continue

            raise Exception ("Should have failed with bad version")

    def test_policy_vendor_args_bad(self):
        '''Test policy vendor via command line args (bad)'''
        bad = [
               "../../../../../../etc",
               "vendor with space",
               "semicolon;isbad",
              ]
        for policy_vendor in bad:
            args = self.full_args
            args.append("--policy-vendor=%s" % policy_vendor)
            args.append("--policy-version=1.0")

            (self.options, self.args) = easyprof.parse_args(args)
            (self.options, self.args) = easyprof.parse_args(self.full_args + [self.binary])
            try:
                easyprof.AppArmorEasyProfile(self.binary, self.options)
            except easyprof.AppArmorException:
                continue

            raise Exception ("Should have failed with bad vendor")

# output_directory tests
    def test_output_directory_multiple(self):
        '''Test output_directory (multiple)'''
        files = dict()
        files["com.example.foo"] = "com.example.foo"
        files["com.ubuntu.developer.myusername.MyCoolApp"] = "com.ubuntu.developer.myusername.MyCoolApp"
        files["usr.bin.baz"] = "/usr/bin/baz"
        m = '''{
  "security": {
   "profiles": {
    "%s": {
      "abstractions": [
        "audio",
        "gnome"
      ],
      "author": "Your Name",
      "binary": "/opt/foo/**",
      "comment": "Unstructured single-line comment",
      "copyright": "Unstructured single-line copyright statement",
      "name": "My Foo App",
      "policy_groups": [
        "opt-application",
        "user-application"
      ],
      "read_path": [
        "/tmp/foo_r",
        "/tmp/bar_r/"
      ],
      "template": "user-application",
      "template_variables": {
        "APPNAME": "foo",
        "VAR1": "bar",
        "VAR2": "baz"
      },
      "write_path": [
        "/tmp/foo_w",
        "/tmp/bar_w/"
      ]
    },
    "%s": {
      "policy_groups": [
        "opt-application",
        "user-application"
      ],
      "template": "user-application",
      "template_variables": {
        "APPNAME": "MyCoolApp",
        "APPVERSION": "0.1.2"
      }
    },
    "%s": {
      "abstractions": [
        "gnome"
      ],
      "policy_groups": [
        "user-application"
      ],
      "template_variables": {
        "APPNAME": "baz"
      }
    }
   }
  }
}''' % (files["com.example.foo"],
        files["com.ubuntu.developer.myusername.MyCoolApp"],
        files["usr.bin.baz"])

        out_dir = os.path.join(self.tmpdir, "output")

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        profiles = easyprof.parse_manifest(m, self.options)
        for (binary, options) in profiles:
            easyp = easyprof.AppArmorEasyProfile(binary, options)
            params = easyprof.gen_policy_params(binary, options)
            easyp.output_policy(params, dir=out_dir)

        for fn in files:
            f = os.path.join(out_dir, fn)
            self.assertTrue(os.path.exists(f), "Could not find '%s'" % f)

    def test_output_directory_single(self):
        '''Test output_directory (single)'''
        files = dict()
        files["com.example.foo"] = "com.example.foo"
        m = '''{
  "security": {
   "profiles": {
    "%s": {
      "abstractions": [
        "audio",
        "gnome"
      ],
      "author": "Your Name",
      "binary": "/opt/foo/**",
      "comment": "Unstructured single-line comment",
      "copyright": "Unstructured single-line copyright statement",
      "name": "My Foo App",
      "policy_groups": [
        "opt-application",
        "user-application"
      ],
      "read_path": [
        "/tmp/foo_r",
        "/tmp/bar_r/"
      ],
      "template": "user-application",
      "template_variables": {
        "APPNAME": "foo",
        "VAR1": "bar",
        "VAR2": "baz"
      },
      "write_path": [
        "/tmp/foo_w",
        "/tmp/bar_w/"
      ]
    }
   }
  }
}''' % (files["com.example.foo"])

        out_dir = os.path.join(self.tmpdir, "output")

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        profiles = easyprof.parse_manifest(m, self.options)
        for (binary, options) in profiles:
            easyp = easyprof.AppArmorEasyProfile(binary, options)
            params = easyprof.gen_policy_params(binary, options)
            easyp.output_policy(params, dir=out_dir)

        for fn in files:
            f = os.path.join(out_dir, fn)
            self.assertTrue(os.path.exists(f), "Could not find '%s'" % f)




    def test_output_directory_invalid(self):
        '''Test output_directory (output directory exists as file)'''
        files = dict()
        files["usr.bin.baz"] = "/usr/bin/baz"
        m = '''{
  "security": {
    "profiles": {
      "%s": {
        "abstractions": [
          "gnome"
        ],
        "policy_groups": [
          "user-application"
        ],
        "template_variables": {
          "APPNAME": "baz"
        }
      }
    }
  }
}''' % files["usr.bin.baz"]


        out_dir = os.path.join(self.tmpdir, "output")
        open(out_dir, 'w').close()

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        (binary, options) = easyprof.parse_manifest(m, self.options)[0]
        easyp = easyprof.AppArmorEasyProfile(binary, options)
        params = easyprof.gen_policy_params(binary, options)
        try:
            easyp.output_policy(params, dir=out_dir)
        except easyprof.AppArmorException:
            return
        raise Exception ("Should have failed with 'is not a directory'")

    def test_output_directory_invalid_params(self):
        '''Test output_directory (no binary or profile_name)'''
        files = dict()
        files["usr.bin.baz"] = "/usr/bin/baz"
        m = '''{
  "security": {
    "profiles": {
      "%s": {
        "abstractions": [
          "gnome"
        ],
        "policy_groups": [
          "user-application"
        ],
        "template_variables": {
          "APPNAME": "baz"
        }
      }
    }
  }
}''' % files["usr.bin.baz"]

        out_dir = os.path.join(self.tmpdir, "output")

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        (binary, options) = easyprof.parse_manifest(m, self.options)[0]
        easyp = easyprof.AppArmorEasyProfile(binary, options)
        params = easyprof.gen_policy_params(binary, options)
        del params['binary']
        try:
            easyp.output_policy(params, dir=out_dir)
        except easyprof.AppArmorException:
            return
        raise Exception ("Should have failed with 'Must specify binary and/or profile name'")

    def test_output_directory_invalid2(self):
        '''Test output_directory (profile exists)'''
        files = dict()
        files["usr.bin.baz"] = "/usr/bin/baz"
        m = '''{
  "security": {
    "profiles": {
      "%s": {
        "abstractions": [
          "gnome"
        ],
        "policy_groups": [
          "user-application"
        ],
        "template_variables": {
          "APPNAME": "baz"
        }
      }
    }
  }
}''' % files["usr.bin.baz"]

        out_dir = os.path.join(self.tmpdir, "output")
        os.mkdir(out_dir)
        open(os.path.join(out_dir, "usr.bin.baz"), 'w').close()

        args = self.full_args
        args.append("--manifest=/dev/null")
        (self.options, self.args) = easyprof.parse_args(args)
        (binary, options) = easyprof.parse_manifest(m, self.options)[0]
        easyp = easyprof.AppArmorEasyProfile(binary, options)
        params = easyprof.gen_policy_params(binary, options)
        try:
            easyp.output_policy(params, dir=out_dir)
        except easyprof.AppArmorException:
            return
        raise Exception ("Should have failed with 'already exists'")

    def test_output_directory_args(self):
        '''Test output_directory (args)'''
        files = dict()
        files["usr.bin.baz"] = "/usr/bin/baz"

        # Build up our args
        args = self.full_args
        args.append('--template=%s' % self.test_template)
        args.append('--name=%s' % 'foo')
        args.append(files["usr.bin.baz"])

        out_dir = os.path.join(self.tmpdir, "output")

        # Now parse our args
        (self.options, self.args) = easyprof.parse_args(args)
        easyp = easyprof.AppArmorEasyProfile(files["usr.bin.baz"], self.options)
        params = easyprof.gen_policy_params(files["usr.bin.baz"], self.options)
        easyp.output_policy(params, dir=out_dir)

        for fn in files:
            f = os.path.join(out_dir, fn)
            self.assertTrue(os.path.exists(f), "Could not find '%s'" % f)

#
# utility classes
#
    def test_valid_profile_name(self):
        '''Test valid_profile_name'''
        names = ['foo',
                 'com.example.foo',
                 '/usr/bin/foo',
                 'com.example.app_myapp_1:2.3+ab12~foo',
                ]
        for n in names:
            self.assertTrue(easyprof.valid_profile_name(n), "'%s' should be valid" % n)

    def test_valid_profile_name_invalid(self):
        '''Test valid_profile_name (invalid)'''
        names = ['fo/o',
                 '/../../etc/passwd',
                 '../../etc/passwd',
                 './../etc/passwd',
                 './etc/passwd',
                 '/usr/bin//foo',
                 '/usr/bin/./foo',
                 'foo`',
                 'foo!',
                 'foo@',
                 'foo$',
                 'foo#',
                 'foo%',
                 'foo^',
                 'foo&',
                 'foo*',
                 'foo(',
                 'foo)',
                 'foo=',
                 'foo{',
                 'foo}',
                 'foo[',
                 'foo]',
                 'foo|',
                 'foo/',
                 'foo\\',
                 'foo;',
                 'foo\'',
                 'foo"',
                 'foo<',
                 'foo>',
                 'foo?',
                 'foo\/',
                 'foo,',
                 '_foo',
                ]
        for n in names:
            self.assertFalse(easyprof.valid_profile_name(n), "'%s' should be invalid" % n)

    def test_valid_path(self):
        '''Test valid_path'''
        names = ['/bin/bar',
                 '/etc/apparmor.d/com.example.app_myapp_1:2.3+ab12~foo',
                ]
        names_rel = ['bin/bar',
                     'apparmor.d/com.example.app_myapp_1:2.3+ab12~foo',
                     'com.example.app_myapp_1:2.3+ab12~foo',
                    ]
        for n in names:
            self.assertTrue(easyprof.valid_path(n), "'%s' should be valid" % n)
        for n in names_rel:
            self.assertTrue(easyprof.valid_path(n, relative_ok=True), "'%s' should be valid" % n)

    def test_zz_valid_path_invalid(self):
        '''Test valid_path (invalid)'''
        names = ['/bin//bar',
                 'bin/bar',
                 '/../etc/passwd',
                 './bin/bar',
                 './',
                ]
        names_rel = ['bin/../bar',
                     'apparmor.d/../passwd',
                     'com.example.app_"myapp_1:2.3+ab12~foo',
                    ]
        for n in names:
            self.assertFalse(easyprof.valid_path(n, relative_ok=False), "'%s' should be invalid" % n)
        for n in names_rel:
            self.assertFalse(easyprof.valid_path(n, relative_ok=True), "'%s' should be invalid" % n)


#
# End test class
#

#
# Main
#
if __name__ == '__main__':
    absfn = os.path.abspath(sys.argv[0])
    topdir = os.path.dirname(os.path.dirname(absfn))

    if len(sys.argv) > 1 and (sys.argv[1] == '-d' or sys.argv[1] == '--debug'):
        debugging = True

    # run the tests
    suite = unittest.TestSuite()
    suite.addTest(unittest.TestLoader().loadTestsFromTestCase(T))
    rc = unittest.TextTestRunner(verbosity=1).run(suite)

    if not rc.wasSuccessful():
        sys.exit(1)
