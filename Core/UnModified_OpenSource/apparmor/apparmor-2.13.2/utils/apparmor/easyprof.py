# ------------------------------------------------------------------
#
#    Copyright (C) 2011-2015 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

from __future__ import with_statement

import codecs
import copy
import glob
import json
import optparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

#
# TODO: move this out to the common library
#
#from apparmor import AppArmorException
class AppArmorException(Exception):
    '''This class represents AppArmor exceptions'''
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)
#
# End common
#

DEBUGGING = False

#
# TODO: move this out to a utilities library
#
def error(out, exit_code=1, do_exit=True):
    '''Print error message and exit'''
    try:
        sys.stderr.write("ERROR: %s\n" % (out))
    except IOError:
        pass

    if do_exit:
        sys.exit(exit_code)


def warn(out):
    '''Print warning message'''
    try:
        sys.stderr.write("WARN: %s\n" % (out))
    except IOError:
        pass


def msg(out, output=sys.stdout):
    '''Print message'''
    try:
        sys.stdout.write("%s\n" % (out))
    except IOError:
        pass


def cmd(command):
    '''Try to execute the given command.'''
    debug(command)
    try:
        sp = subprocess.Popen(command, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
    except OSError as ex:
        return [127, str(ex)]

    out = sp.communicate()[0]
    return [sp.returncode, out]


def cmd_pipe(command1, command2):
    '''Try to pipe command1 into command2.'''
    try:
        sp1 = subprocess.Popen(command1, stdout=subprocess.PIPE)
        sp2 = subprocess.Popen(command2, stdin=sp1.stdout)
    except OSError as ex:
        return [127, str(ex)]

    out = sp2.communicate()[0]
    return [sp2.returncode, out]


def debug(out):
    '''Print debug message'''
    if DEBUGGING:
        try:
            sys.stderr.write("DEBUG: %s\n" % (out))
        except IOError:
            pass


def valid_binary_path(path):
    '''Validate name'''
    try:
        a_path = os.path.abspath(path)
    except Exception:
        debug("Could not find absolute path for binary")
        return False

    if path != a_path:
        debug("Binary should use a normalized absolute path")
        return False

    if not os.path.exists(a_path):
        return True

    r_path = os.path.realpath(path)
    if r_path != a_path:
        debug("Binary should not be a symlink")
        return False

    return True


def valid_variable(v):
    '''Validate variable name'''
    debug("Checking '%s'" % v)
    try:
        (key, value) = v.split('=')
    except Exception:
        return False

    if not re.search(r'^@\{[a-zA-Z0-9_]+\}$', key):
        return False

    if '/' in value:
        rel_ok = False
        if not value.startswith('/'):
            rel_ok = True
        if not valid_path(value, relative_ok=rel_ok):
            return False

    if '"' in value:
        return False

    # If we made it here, we are safe
    return True


def valid_path(path, relative_ok=False):
    '''Valid path'''
    m = "Invalid path: %s" % (path)
    if not relative_ok and not path.startswith('/'):
        debug("%s (relative)" % (m))
        return False

    if '"' in path: # We double quote elsewhere
        debug("%s (quote)" % (m))
        return False

    if '../' in path:
        debug("%s (../ path escape)" % (m))
        return False

    try:
        p = os.path.normpath(path)
    except Exception:
        debug("%s (could not normalize)" % (m))
        return False

    if p != path:
        debug("%s (normalized path != path (%s != %s))" % (m, p, path))
        return False

    # If we made it here, we are safe
    return True


def _is_safe(s):
    '''Known safe regex'''
    if re.search(r'^[a-zA-Z_0-9\-\.]+$', s):
        return True
    return False


def valid_policy_vendor(s):
    '''Verify the policy vendor'''
    return _is_safe(s)


def valid_policy_version(v):
    '''Verify the policy version'''
    try:
        float(v)
    except ValueError:
        return False
    if float(v) < 0:
        return False
    return True


def valid_template_name(s, strict=False):
    '''Verify the template name'''
    if not strict and s.startswith('/'):
        if not valid_path(s):
            return False
        return True
    return _is_safe(s)


def valid_abstraction_name(s):
    '''Verify the template name'''
    return _is_safe(s)


def valid_profile_name(s):
    '''Verify the profile name'''
    # profile name specifies path
    if s.startswith('/'):
        if not valid_path(s):
            return False
        return True

    # profile name does not specify path
    # alpha-numeric and Debian version, plus '_'
    if re.search(r'^[a-zA-Z0-9][a-zA-Z0-9_\+\-\.:~]+$', s):
        return True
    return False


def valid_policy_group_name(s):
    '''Verify policy group name'''
    return _is_safe(s)


def get_directory_contents(path):
    '''Find contents of the given directory'''
    if not valid_path(path):
        return None

    files = []
    for f in glob.glob(path + "/*"):
        files.append(f)

    files.sort()
    return files

def open_file_read(path):
    '''Open specified file read-only'''
    try:
        orig = codecs.open(path, 'r', "UTF-8")
    except Exception:
        raise

    return orig


def verify_policy(policy, exe, base=None, include=None):
    '''Verify policy compiles'''
    if not exe:
        warn("Could not find apparmor_parser. Skipping verify")
        return True

    fn = ""
    # if policy starts with '/' and is one line, assume it is a path
    if len(policy.splitlines()) == 1 and valid_path(policy):
        fn = policy
    else:
        f, fn = tempfile.mkstemp(prefix='aa-easyprof')
        if not isinstance(policy, bytes):
            policy = policy.encode('utf-8')
        os.write(f, policy)
        os.close(f)

    command = [exe, '-QTK']
    if base:
        command.extend(['-b', base])
    if include:
        command.extend(['-I', include])
    command.append(fn)

    rc, out = cmd(command)
    os.unlink(fn)
    if rc == 0:
        return True
    return False

#
# End utility functions
#


class AppArmorEasyProfile:
    '''Easy profile class'''
    def __init__(self, binary, opt):
        verify_options(opt)
        opt.ensure_value("conffile", "/etc/apparmor/easyprof.conf")
        self.conffile = os.path.abspath(opt.conffile)
        debug("Examining confile=%s" % (self.conffile))

        self.dirs = dict()
        if os.path.isfile(self.conffile):
            self._get_defaults()

        self.parser_path = '/sbin/apparmor_parser'
        if opt.parser_path:
            self.parser_path = opt.parser_path
        elif not os.path.exists(self.parser_path):
            rc, self.parser_path = cmd(['which', 'apparmor_parser'])
            if rc != 0:
                self.parser_path = None

        self.parser_base = "/etc/apparmor.d"
        if opt.parser_base:
            self.parser_base = opt.parser_base

        self.parser_include = None
        if opt.parser_include:
            self.parser_include = opt.parser_include

        if opt.templates_dir and os.path.isdir(opt.templates_dir):
            self.dirs['templates'] = os.path.abspath(opt.templates_dir)
        elif not opt.templates_dir and \
             opt.template and \
             os.path.isfile(opt.template) and \
             valid_path(opt.template):
	    # If we specified the template and it is an absolute path, just set
	    # the templates directory to the parent of the template so we don't
            # have to require --template-dir with absolute paths.
            self.dirs['templates'] = os.path.abspath(os.path.dirname(opt.template))

        if opt.include_templates_dir and \
           os.path.isdir(opt.include_templates_dir):
            self.dirs['templates_include'] = os.path.abspath(opt.include_templates_dir)

        if opt.policy_groups_dir and os.path.isdir(opt.policy_groups_dir):
            self.dirs['policygroups'] = os.path.abspath(opt.policy_groups_dir)

        if opt.include_policy_groups_dir and \
           os.path.isdir(opt.include_policy_groups_dir):
            self.dirs['policygroups_include'] = os.path.abspath(opt.include_policy_groups_dir)

        self.policy_version = None
        self.policy_vendor = None
        if (opt.policy_version and not opt.policy_vendor) or \
           (opt.policy_vendor and not opt.policy_version):
            raise AppArmorException("Must specify both policy version and vendor")

        # If specified --policy-version and --policy-vendor, use
        #  templates_dir/policy_vendor/policy_version
        if opt.policy_version and opt.policy_vendor:
            self.policy_vendor = opt.policy_vendor
            self.policy_version = str(opt.policy_version)

            for i in ['templates', 'policygroups']:
                d = os.path.join(self.dirs[i], \
                                 self.policy_vendor, \
                                 self.policy_version)
                if not os.path.isdir(d):
                    raise AppArmorException(
                            "Could not find %s directory '%s'" % (i, d))
                self.dirs[i] = d

        if not 'templates' in self.dirs:
            raise AppArmorException("Could not find templates directory")
        if not 'policygroups' in self.dirs:
            raise AppArmorException("Could not find policygroups directory")

        self.binary = binary
        if binary:
            if not valid_binary_path(binary):
                raise AppArmorException("Invalid path for binary: '%s'" % binary)

        if opt.manifest:
            self.set_template(opt.template, allow_abs_path=False)
        else:
            self.set_template(opt.template)

        self.set_policygroup(opt.policy_groups)
        if opt.name:
            self.set_name(opt.name)
        elif self.binary != None:
            self.set_name(self.binary)

        self.templates = []
        for f in get_directory_contents(self.dirs['templates']):
            if os.path.isfile(f):
                self.templates.append(f)

        if 'templates_include' in self.dirs:
            for f in get_directory_contents(self.dirs['templates_include']):
                if os.path.isfile(f) and f not in self.templates:
                    self.templates.append(f)

        self.policy_groups = []
        for f in get_directory_contents(self.dirs['policygroups']):
            if os.path.isfile(f):
                self.policy_groups.append(f)

        if 'policygroups_include' in self.dirs:
            for f in get_directory_contents(self.dirs['policygroups_include']):
                if os.path.isfile(f) and f not in self.policy_groups:
                    self.policy_groups.append(f)

    def _get_defaults(self):
        '''Read in defaults from configuration'''
        if not os.path.exists(self.conffile):
            raise AppArmorException("Could not find '%s'" % self.conffile)

        # Read in the configuration
        f = open_file_read(self.conffile)

        pat = re.compile(r'^\w+=".*"?')
        for line in f:
            if not pat.search(line):
                continue
            if line.startswith("POLICYGROUPS_DIR="):
                d = re.split(r'=', line.strip())[1].strip('["\']')
                self.dirs['policygroups'] = d
            elif line.startswith("TEMPLATES_DIR="):
                d = re.split(r'=', line.strip())[1].strip('["\']')
                self.dirs['templates'] = d
        f.close()

        keys = self.dirs.keys()
        if 'templates' not in keys:
            raise AppArmorException("Could not find TEMPLATES_DIR in '%s'" % self.conffile)
        if 'policygroups' not in keys:
            raise AppArmorException("Could not find POLICYGROUPS_DIR in '%s'" % self.conffile)

        for k in self.dirs.keys():
            if not os.path.isdir(self.dirs[k]):
                raise AppArmorException("Could not find '%s'" % self.dirs[k])

    def set_name(self, name):
        '''Set name of policy'''
        self.name = name

    def get_template(self):
        '''Get contents of current template'''
        return open(self.template).read()

    def set_template(self, template, allow_abs_path=True):
        '''Set current template'''
        if "../" in template:
            raise AppArmorException('template "%s" contains "../" escape path' % (template))
        elif template.startswith('/') and not allow_abs_path:
            raise AppArmorException("Cannot use an absolute path template '%s'" % template)

        # If have an abs path, just use it
        if template.startswith('/'):
            if not os.path.exists(template):
                raise AppArmorException('%s does not exist' % (template))
            self.template = template
            return

        # Find the template since we don't have an abs path
        sys_t = os.path.join(self.dirs['templates'], template)
        inc_t = None
        if 'templates_include' in self.dirs:
            inc_t = os.path.join(self.dirs['templates_include'], template)

        if os.path.exists(sys_t):
            self.template = sys_t
        elif inc_t is not None and os.path.exists(inc_t):
            self.template = inc_t
        else:
            raise AppArmorException('%s does not exist' % (template))

    def get_templates(self):
        '''Get list of all available templates by filename'''
        return self.templates

    def get_policygroup(self, policygroup):
        '''Get contents of specific policygroup'''
        p = policygroup
        if not p.startswith('/'):
            sys_p = os.path.join(self.dirs['policygroups'], p)
            inc_p = None
            if 'policygroups_include' in self.dirs:
                inc_p = os.path.join(self.dirs['policygroups_include'], p)

            if os.path.exists(sys_p):
                p = sys_p
            elif inc_p is not None and os.path.exists(inc_p):
                p = inc_p

        if self.policy_groups == None or not p in self.policy_groups:
            raise AppArmorException("Policy group '%s' does not exist" % p)
        return open(p).read()

    def set_policygroup(self, policygroups):
        '''Set policygroups'''
        self.policy_groups = []
        if policygroups != None:
            for p in policygroups.split(','):
                # If have abs path, just use it
                if p.startswith('/'):
                    if not os.path.exists(p):
                        raise AppArmorException('%s does not exist' % (p))
                    self.policy_groups.append(p)
                    continue

                # Find the policy group since we don't have and abs path
                sys_p = os.path.join(self.dirs['policygroups'], p)
                inc_p = None
                if 'policygroups_include' in self.dirs:
                    inc_p = os.path.join(self.dirs['policygroups_include'], p)

                if os.path.exists(sys_p):
                    self.policy_groups.append(sys_p)
                elif inc_p is not None and os.path.exists(inc_p):
                    self.policy_groups.append(inc_p)
                else:
                    raise AppArmorException('%s does not exist' % (p))

    def get_policy_groups(self):
        '''Get list of all policy groups by filename'''
        return self.policy_groups

    def gen_abstraction_rule(self, abstraction):
        '''Generate an abstraction rule'''
        base = os.path.join(self.parser_base, "abstractions", abstraction)
        if not os.path.exists(base):
            if not self.parser_include:
                raise AppArmorException("%s does not exist" % base)

            include = os.path.join(self.parser_include, "abstractions", abstraction)
            if not os.path.exists(include):
                raise AppArmorException("Neither %s nor %s exist" % (base, include))

        return "#include <abstractions/%s>" % abstraction

    def gen_variable_declaration(self, dec):
        '''Generate a variable declaration'''
        if not valid_variable(dec):
            raise AppArmorException("Invalid variable declaration '%s'" % dec)
        # Make sure we always quote
        k, v = dec.split('=')
        return '%s="%s"' % (k, v)

    def gen_path_rule(self, path, access):
        rule = []
        if not path.startswith('/') and not path.startswith('@'):
            raise AppArmorException("'%s' should not be relative path" % path)

        owner = ""
        if path.startswith('/home/') or path.startswith("@{HOME"):
            owner = "owner "

        if path.endswith('/'):
            rule.append("%s %s," % (path, access))
            rule.append("%s%s** %s," % (owner, path, access))
        elif path.endswith('/**') or path.endswith('/*'):
            rule.append("%s %s," % (os.path.dirname(path), access))
            rule.append("%s%s %s," % (owner, path, access))
        else:
            rule.append("%s%s %s," % (owner, path, access))

        return rule


    def gen_policy(self, name,
                         binary=None,
                         profile_name=None,
                         template_var=[],
                         abstractions=None,
                         policy_groups=None,
                         read_path=[],
                         write_path=[],
                         author=None,
                         comment=None,
                         copyright=None,
                         no_verify=False):
        def find_prefix(t, s):
            '''Calculate whitespace prefix based on occurrence of s in t'''
            pat = re.compile(r'^ *%s' % s)
            p = ""
            for line in t.splitlines():
                if pat.match(line):
                    p = " " * (len(line) - len(line.lstrip()))
                    break
            return p

        policy = self.get_template()
        if '###ENDUSAGE###' in policy:
            found = False
            tmp = ""
            for line in policy.splitlines():
                if not found:
                    if line.startswith('###ENDUSAGE###'):
                        found = True
                    continue
                tmp += line + "\n"
            policy = tmp

        attachment = ""
        if binary:
            if not valid_binary_path(binary):
                raise AppArmorException("Invalid path for binary: '%s'" % \
                                        binary)
            if profile_name:
                attachment = 'profile "%s" "%s"' % (profile_name, binary)
            else:
                attachment = '"%s"' % binary
        elif profile_name:
            attachment = 'profile "%s"' % profile_name
        else:
            raise AppArmorException("Must specify binary and/or profile name")
        policy = re.sub(r'###PROFILEATTACH###', attachment, policy)

        policy = re.sub(r'###NAME###', name, policy)

        # Fill-in various comment fields
        if comment != None:
            policy = re.sub(r'###COMMENT###', "Comment: %s" % comment, policy)

        if author != None:
            policy = re.sub(r'###AUTHOR###', "Author: %s" % author, policy)

        if copyright != None:
            policy = re.sub(r'###COPYRIGHT###', "Copyright: %s" % copyright, policy)

        # Fill-in rules and variables with proper indenting
        search = '###ABSTRACTIONS###'
        prefix = find_prefix(policy, search)
        s = "%s# No abstractions specified" % prefix
        if abstractions != None:
            s = "%s# Specified abstractions" % (prefix)
            t = abstractions.split(',')
            t.sort()
            for i in t:
                s += "\n%s%s" % (prefix, self.gen_abstraction_rule(i))
        policy = re.sub(r' *%s' % search, s, policy)

        search = '###POLICYGROUPS###'
        prefix = find_prefix(policy, search)
        s = "%s# No policy groups specified" % prefix
        if policy_groups != None:
            s = "%s# Rules specified via policy groups" % (prefix)
            t = policy_groups.split(',')
            t.sort()
            for i in t:
                for line in self.get_policygroup(i).splitlines():
                    s += "\n%s%s" % (prefix, line)
                if i != policy_groups.split(',')[-1]:
                    s += "\n"
        policy = re.sub(r' *%s' % search, s, policy)

        search = '###VAR###'
        prefix = find_prefix(policy, search)
        s = "%s# No template variables specified" % prefix
        if len(template_var) > 0:
            s = "%s# Specified profile variables" % (prefix)
            template_var.sort()
            for i in template_var:
                s += "\n%s%s" % (prefix, self.gen_variable_declaration(i))
        policy = re.sub(r' *%s' % search, s, policy)

        search = '###READS###'
        prefix = find_prefix(policy, search)
        s = "%s# No read paths specified" % prefix
        if len(read_path) > 0:
            s = "%s# Specified read permissions" % (prefix)
            read_path.sort()
            for i in read_path:
                for r in self.gen_path_rule(i, 'rk'):
                    s += "\n%s%s" % (prefix, r)
        policy = re.sub(r' *%s' % search, s, policy)

        search = '###WRITES###'
        prefix = find_prefix(policy, search)
        s = "%s# No write paths specified" % prefix
        if len(write_path) > 0:
            s = "%s# Specified write permissions" % (prefix)
            write_path.sort()
            for i in write_path:
                for r in self.gen_path_rule(i, 'rwk'):
                    s += "\n%s%s" % (prefix, r)
        policy = re.sub(r' *%s' % search, s, policy)

        if no_verify:
            debug("Skipping policy verification")
        elif not verify_policy(policy, self.parser_path, self.parser_base, self.parser_include):
            msg("\n" + policy)
            raise AppArmorException("Invalid policy")

        return policy

    def output_policy(self, params, count=0, dir=None):
        '''Output policy'''
        policy = self.gen_policy(**params)
        if not dir:
            if count:
                sys.stdout.write('### aa-easyprof profile #%d ###\n' % count)
            sys.stdout.write('%s\n' % policy)
        else:
            out_fn = ""
            if 'profile_name' in params:
                out_fn = params['profile_name']
            elif 'binary' in params:
                out_fn = params['binary']
            else: # should not ever reach this
                raise AppArmorException("Could not determine output filename")

            # Generate an absolute path, convertng any path delimiters to '.'
            out_fn = os.path.join(dir, re.sub(r'/', '.', out_fn.lstrip('/')))
            if os.path.exists(out_fn):
                raise AppArmorException("'%s' already exists" % out_fn)

            if not os.path.exists(dir):
                os.mkdir(dir)

            if not os.path.isdir(dir):
                raise AppArmorException("'%s' is not a directory" % dir)

            f, fn = tempfile.mkstemp(prefix='aa-easyprof')
            if not isinstance(policy, bytes):
                policy = policy.encode('utf-8')
            os.write(f, policy)
            os.close(f)

            shutil.move(fn, out_fn)

    def gen_manifest(self, params):
        '''Take params list and output a JSON file'''
        d = dict()
        d['security'] = dict()
        d['security']['profiles'] = dict()

        pkey = ""
        if 'profile_name' in params:
            pkey = params['profile_name']
        elif 'binary' in params:
            # when profile_name is not specified, the binary (path attachment)
            # also functions as the profile name
            pkey = params['binary']
        else:
            raise AppArmorException("Must supply binary or profile name")

        d['security']['profiles'][pkey] = dict()

        # Add the template since it isn't part of 'params'
        template = os.path.basename(self.template)
        if template != 'default':
            d['security']['profiles'][pkey]['template'] = template

        # Add the policy_version since it isn't part of 'params'
        if self.policy_version:
            d['security']['profiles'][pkey]['policy_version'] = float(self.policy_version)
        if self.policy_vendor:
            d['security']['profiles'][pkey]['policy_vendor'] = self.policy_vendor

        for key in params:
            if key == 'profile_name' or \
               (key == 'binary' and not 'profile_name' in params):
                continue # don't re-add the pkey
            elif key == 'binary' and not params[key]:
                continue # binary can by None when specifying --profile-name
            elif key == 'template_var':
                d['security']['profiles'][pkey]['template_variables'] = dict()
                for tvar in params[key]:
                    if not self.gen_variable_declaration(tvar):
                        raise AppArmorException("Malformed template_var '%s'" % tvar)
                    (k, v) = tvar.split('=')
                    k = k.lstrip('@').lstrip('{').rstrip('}')
                    d['security']['profiles'][pkey]['template_variables'][k] = v
            elif key == 'abstractions' or key == 'policy_groups':
                d['security']['profiles'][pkey][key] = params[key].split(",")
                d['security']['profiles'][pkey][key].sort()
            else:
                d['security']['profiles'][pkey][key] = params[key]
        json_str = json.dumps(d,
                              sort_keys=True,
                              indent=2,
                              separators=(',', ': ')
                             )
        return json_str

def print_basefilenames(files):
    for i in files:
        sys.stdout.write("%s\n" % (os.path.basename(i)))

def print_files(files):
    for i in files:
        with open(i) as f:
            sys.stdout.write(f.read()+"\n")

def check_manifest_conflict_args(option, opt_str, value, parser):
    '''Check for -m/--manifest with conflicting args'''
    conflict_args = ['abstractions',
                     'read_path',
                     'write_path',
                     # template always get set to 'default', can't conflict
                     # 'template',
                     'policy_groups',
                     'policy_version',
                     'policy_vendor',
                     'name',
                     'profile_name',
                     'comment',
                     'copyright',
                     'author',
                     'template_var']
    for conflict in conflict_args:
        if getattr(parser.values, conflict, False):
            raise optparse.OptionValueError("can't use --%s with --manifest " \
                                            "argument" % conflict)
    setattr(parser.values, option.dest, value)

def check_for_manifest_arg(option, opt_str, value, parser):
    '''Check for -m/--manifest with conflicting args'''
    if parser.values.manifest:
        raise optparse.OptionValueError("can't use --%s with --manifest " \
                                        "argument" % opt_str.lstrip('-'))
    setattr(parser.values, option.dest, value)

def check_for_manifest_arg_append(option, opt_str, value, parser):
    '''Check for -m/--manifest with conflicting args (with append)'''
    if parser.values.manifest:
         raise optparse.OptionValueError("can't use --%s with --manifest " \
                                         "argument" % opt_str.lstrip('-'))
    parser.values.ensure_value(option.dest, []).append(value)

def add_parser_policy_args(parser):
    '''Add parser arguments'''
    parser.add_option("--parser",
                      dest="parser_path",
                      help="The path to the profile parser used for verification",
                      metavar="PATH")
    parser.add_option("-a", "--abstractions",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      dest="abstractions",
                      help="Comma-separated list of abstractions",
                      metavar="ABSTRACTIONS")
    parser.add_option("-b", "--base",
                      dest="parser_base",
                      help="Set the base directory for resolving abstractions",
                      metavar="DIR")
    parser.add_option("-I", "--Include",
                      dest="parser_include",
                      help="Add a directory to the search path when resolving abstractions",
                      metavar="DIR")
    parser.add_option("--read-path",
                      action="callback",
                      callback=check_for_manifest_arg_append,
                      type=str,
                      dest="read_path",
                      help="Path allowing owner reads",
                      metavar="PATH")
    parser.add_option("--write-path",
                      action="callback",
                      callback=check_for_manifest_arg_append,
                      type=str,
                      dest="write_path",
                      help="Path allowing owner writes",
                      metavar="PATH")
    parser.add_option("-t", "--template",
                      dest="template",
                      help="Use non-default policy template",
                      metavar="TEMPLATE",
                      default='default')
    parser.add_option("--templates-dir",
                      dest="templates_dir",
                      help="Use non-default templates directory",
                      metavar="DIR")
    parser.add_option("--include-templates-dir",
                      dest="include_templates_dir",
                      help="Also search DIR for templates",
                      metavar="DIR")
    parser.add_option("-p", "--policy-groups",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      help="Comma-separated list of policy groups",
                      metavar="POLICYGROUPS")
    parser.add_option("--policy-groups-dir",
                      dest="policy_groups_dir",
                      help="Use non-default policy-groups directory",
                      metavar="DIR")
    parser.add_option("--include-policy-groups-dir",
                      dest="include_policy_groups_dir",
                      help="Also search DIR for policy groups",
                      metavar="DIR")
    parser.add_option("--policy-version",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      dest="policy_version",
                      help="Specify version for templates and policy groups",
                      metavar="VERSION")
    parser.add_option("--policy-vendor",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      dest="policy_vendor",
                      help="Specify vendor for templates and policy groups",
                      metavar="VENDOR")
    parser.add_option("--profile-name",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      dest="profile_name",
                      help="AppArmor profile name",
                      metavar="PROFILENAME")

def parse_args(args=None, parser=None):
    '''Parse arguments'''
    global DEBUGGING

    if parser == None:
        parser = optparse.OptionParser()

    parser.add_option("-c", "--config-file",
                      dest="conffile",
                      help="Use alternate configuration file",
                      metavar="FILE")
    parser.add_option("-d", "--debug",
                      help="Show debugging output",
                      action='store_true',
                      default=False)
    parser.add_option("--no-verify",
                      help="Don't verify policy using 'apparmor_parser -p'",
                      action='store_true',
                      default=False)
    parser.add_option("--list-templates",
                      help="List available templates",
                      action='store_true',
                      default=False)
    parser.add_option("--show-template",
                      help="Show specified template",
                      action='store_true',
                      default=False)
    parser.add_option("--list-policy-groups",
                      help="List available policy groups",
                      action='store_true',
                      default=False)
    parser.add_option("--show-policy-group",
                      help="Show specified policy groups",
                      action='store_true',
                      default=False)
    parser.add_option("-n", "--name",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      dest="name",
                      help="Name of policy (not AppArmor profile name)",
                      metavar="COMMENT")
    parser.add_option("--comment",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      dest="comment",
                      help="Comment for policy",
                      metavar="COMMENT")
    parser.add_option("--author",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      dest="author",
                      help="Author of policy",
                      metavar="COMMENT")
    parser.add_option("--copyright",
                      action="callback",
                      callback=check_for_manifest_arg,
                      type=str,
                      dest="copyright",
                      help="Copyright for policy",
                      metavar="COMMENT")
    parser.add_option("--template-var",
                      action="callback",
                      callback=check_for_manifest_arg_append,
                      type=str,
                      dest="template_var",
                      help="Declare AppArmor variable",
                      metavar="@{VARIABLE}=VALUE")
    parser.add_option("--output-format",
                      action="store",
                      dest="output_format",
                      help="Specify output format as text (default) or json",
                      metavar="FORMAT",
                      default="text")
    parser.add_option("--output-directory",
                      action="store",
                      dest="output_directory",
                      help="Output policy to this directory",
                      metavar="DIR")
    # This option conflicts with any of the value arguments, e.g. name,
    # author, template-var, etc.
    parser.add_option("-m", "--manifest",
                      action="callback",
                      callback=check_manifest_conflict_args,
                      type=str,
                      dest="manifest",
                      help="JSON manifest file",
                      metavar="FILE")
    parser.add_option("--verify-manifest",
                      action="store_true",
                      default=False,
                      dest="verify_manifest",
                      help="Verify JSON manifest file")


    # add policy args now
    add_parser_policy_args(parser)

    (my_opt, my_args) = parser.parse_args(args)

    if my_opt.debug:
        DEBUGGING = True
    return (my_opt, my_args)

def gen_policy_params(binary, opt):
    '''Generate parameters for gen_policy'''
    params = dict(binary=binary)

    if not binary and not opt.profile_name:
        raise AppArmorException("Must specify binary and/or profile name")

    if opt.profile_name:
        params['profile_name'] = opt.profile_name

    if opt.name:
        params['name'] = opt.name
    else:
        if opt.profile_name:
            params['name'] = opt.profile_name
        elif binary:
            params['name'] = os.path.basename(binary)

    if opt.template_var: # What about specified multiple times?
        params['template_var'] = opt.template_var
    if opt.abstractions:
        params['abstractions'] = opt.abstractions
    if opt.policy_groups:
        params['policy_groups'] = opt.policy_groups
    if opt.read_path:
        params['read_path'] = opt.read_path
    if opt.write_path:
        params['write_path'] = opt.write_path
    if opt.comment:
        params['comment'] = opt.comment
    if opt.author:
        params['author'] = opt.author
    if opt.copyright:
        params['copyright'] = opt.copyright
    if opt.policy_version and opt.output_format == "json":
        params['policy_version'] = opt.policy_version
    if opt.policy_vendor and opt.output_format == "json":
        params['policy_vendor'] = opt.policy_vendor

    return params

def parse_manifest(manifest, opt_orig):
    '''Take a JSON manifest as a string and updates options, returning an
       updated binary. Note that a JSON file may contain multiple profiles.'''

    try:
        m = json.loads(manifest)
    except ValueError:
        raise AppArmorException("Could not parse manifest")

    if 'security' in m:
        top_table = m['security']
    else:
        top_table = m

    if 'profiles' not in top_table:
        raise AppArmorException("Could not parse manifest (could not find 'profiles')")
    table = top_table['profiles']

    # generally mirrors what is settable in gen_policy_params()
    valid_keys = ['abstractions',
                  'author',
                  'binary',
                  'comment',
                  'copyright',
                  'name',
                  'policy_groups',
                  'policy_version',
                  'policy_vendor',
                  'profile_name',
                  'read_path',
                  'template',
                  'template_variables',
                  'write_path',
                 ]

    profiles = []

    for profile_name in table:
        if not isinstance(table[profile_name], dict):
            raise AppArmorException("Wrong JSON structure")
        opt = copy.deepcopy(opt_orig)

        # The JSON structure is:
        # {
        #   "security": {
        #     <profile_name>: {
        #       "binary": ...
        #       ...
        # but because binary can be the profile name, we need to handle
        # 'profile_name' and 'binary' special. If a profile_name starts with
        # '/', then it is considered the binary. Otherwise, set the
        # profile_name and set the binary if it is in the JSON.
        binary = None
        if profile_name.startswith('/'):
            if 'binary' in table[profile_name]:
                raise AppArmorException("Profile name should not specify path with binary")
            binary = profile_name
        else:
            setattr(opt, 'profile_name', profile_name)
            if 'binary' in table[profile_name]:
                binary = table[profile_name]['binary']
                setattr(opt, 'binary', binary)

        for key in table[profile_name]:
            if key not in valid_keys:
                raise AppArmorException("Invalid key '%s'" % key)

            if key == 'binary':
                continue #  handled above
            elif key == 'abstractions' or key == 'policy_groups':
                setattr(opt, key, ",".join(table[profile_name][key]))
            elif key == "template_variables":
                t = table[profile_name]['template_variables']
                vlist = []
                for v in t.keys():
                    vlist.append("@{%s}=%s" % (v, t[v]))
                    setattr(opt, 'template_var', vlist)
            else:
                if hasattr(opt, key):
                    setattr(opt, key, table[profile_name][key])

        profiles.append( (binary, opt) )

    return profiles


def verify_options(opt, strict=False):
    '''Make sure our options are valid'''
    if hasattr(opt, 'binary') and opt.binary and not valid_path(opt.binary):
        raise AppArmorException("Invalid binary '%s'" % opt.binary)
    if hasattr(opt, 'profile_name') and opt.profile_name != None and \
       not valid_profile_name(opt.profile_name):
        raise AppArmorException("Invalid profile name '%s'" % opt.profile_name)
    if hasattr(opt, 'binary') and opt.binary and \
       hasattr(opt, 'profile_name') and opt.profile_name != None and \
       opt.profile_name.startswith('/'):
        raise AppArmorException("Profile name should not specify path with binary")
    if hasattr(opt, 'policy_vendor') and opt.policy_vendor and \
       not valid_policy_vendor(opt.policy_vendor):
        raise AppArmorException("Invalid policy vendor '%s'" % \
                                opt.policy_vendor)
    if hasattr(opt, 'policy_version') and opt.policy_version and \
       not valid_policy_version(opt.policy_version):
        raise AppArmorException("Invalid policy version '%s'" % \
                                opt.policy_version)
    if hasattr(opt, 'template') and opt.template and \
       not valid_template_name(opt.template, strict):
        raise AppArmorException("Invalid template '%s'" % opt.template)
    if hasattr(opt, 'template_var') and opt.template_var:
        for i in opt.template_var:
            if not valid_variable(i):
                raise AppArmorException("Invalid variable '%s'" % i)
    if hasattr(opt, 'policy_groups') and opt.policy_groups:
        for i in opt.policy_groups.split(','):
            if not valid_policy_group_name(i):
                raise AppArmorException("Invalid policy group '%s'" % i)
    if hasattr(opt, 'abstractions') and opt.abstractions:
        for i in opt.abstractions.split(','):
            if not valid_abstraction_name(i):
                raise AppArmorException("Invalid abstraction '%s'" % i)
    if hasattr(opt, 'read_paths') and opt.read_paths:
        for i in opt.read_paths:
            if not valid_path(i):
                raise AppArmorException("Invalid read path '%s'" % i)
    if hasattr(opt, 'write_paths') and opt.write_paths:
        for i in opt.write_paths:
            if not valid_path(i):
                raise AppArmorException("Invalid write path '%s'" % i)


def verify_manifest(params, args=None):
    '''Verify manifest for safe and unsafe options'''
    err_str = ""
    (opt, args) = parse_args(args)
    fake_easyp = AppArmorEasyProfile(None, opt)

    unsafe_keys = ['read_path', 'write_path']
    safe_abstractions = ['base']
    for k in params:
        debug("Examining %s=%s" % (k, params[k]))
        if k in unsafe_keys:
            err_str += "\nfound %s key" % k
        elif k == 'profile_name':
            if params['profile_name'].startswith('/') or \
               '*' in params['profile_name']:
                err_str += "\nprofile_name '%s'" % params['profile_name']
        elif k == 'abstractions':
            for a in params['abstractions'].split(','):
                if not a in safe_abstractions:
                    err_str += "\nfound '%s' abstraction" % a
        elif k == "template_var":
            pat = re.compile(r'[*/\{\}\[\]]')
            for tv in params['template_var']:
                if not fake_easyp.gen_variable_declaration(tv):
                    err_str += "\n%s" % tv
                    continue
                tv_val = tv.split('=')[1]
                debug("Examining %s" % tv_val)
                if '..' in tv_val or pat.search(tv_val):
                     err_str += "\n%s" % tv

    if err_str:
        warn("Manifest definition is potentially unsafe%s" % err_str)
        return False

    return True

