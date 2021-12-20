# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
#    Copyright (C) 2014-2018 Christian Boltz <apparmor@cboltz.de>
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License as published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
# ----------------------------------------------------------------------
# No old version logs, only 2.6 + supported
from __future__ import division, with_statement
import os
import re
import shutil
import subprocess
import sys
import time
import traceback
import atexit
import tempfile

import apparmor.config
import apparmor.logparser
import apparmor.severity

from copy import deepcopy

from apparmor.aare import AARE

from apparmor.common import (AppArmorException, AppArmorBug, open_file_read, valid_path, hasher,
                             open_file_write, DebugLogger)

import apparmor.ui as aaui

from apparmor.aamode import str_to_mode, split_mode

from apparmor.regex import (RE_PROFILE_START, RE_PROFILE_END, RE_PROFILE_LINK,
                            RE_ABI, RE_PROFILE_ALIAS,
                            RE_PROFILE_BOOLEAN, RE_PROFILE_VARIABLE, RE_PROFILE_CONDITIONAL,
                            RE_PROFILE_CONDITIONAL_VARIABLE, RE_PROFILE_CONDITIONAL_BOOLEAN,
                            RE_PROFILE_CHANGE_HAT,
                            RE_PROFILE_HAT_DEF, RE_PROFILE_MOUNT,
                            RE_PROFILE_PIVOT_ROOT,
                            RE_PROFILE_UNIX, RE_RULE_HAS_COMMA, RE_HAS_COMMENT_SPLIT,
                            strip_quotes, parse_profile_start_line, re_match_include )

from apparmor.profile_list import ProfileList

from apparmor.profile_storage import ProfileStorage, add_or_remove_flag, ruletypes, write_abi

import apparmor.rules as aarules

from apparmor.rule.capability       import CapabilityRule
from apparmor.rule.change_profile   import ChangeProfileRule
from apparmor.rule.dbus             import DbusRule
from apparmor.rule.file             import FileRule
from apparmor.rule.network          import NetworkRule
from apparmor.rule.ptrace           import PtraceRule
from apparmor.rule.rlimit           import RlimitRule
from apparmor.rule.signal           import SignalRule
from apparmor.rule import quote_if_needed

# setup module translations
from apparmor.translations import init_translation
_ = init_translation()

# Setup logging incase of debugging is enabled
debug_logger = DebugLogger('aa')

# The database for severity
sev_db = None
# The file to read log messages from
### Was our
logfile = None

CONFDIR = None
conf = None
cfg = None
repo_cfg = None

parser = None
profile_dir = None
extra_profile_dir = None
### end our
# To keep track of previously included profile fragments
include = dict()

active_profiles = ProfileList()
extra_profiles = ProfileList()

# To store the globs entered by users so they can be provided again
# format: user_globs['/foo*'] = AARE('/foo*')
user_globs = {}

## Variables used under logprof
transitions = hasher()

aa = hasher()  # Profiles originally in sd, replace by aa
original_aa = hasher()
extras = hasher()  # Inactive profiles from extras
### end our
log_pid = dict()  # handed over to ReadLog, gets filled in logparser.py. The only case the previous content of this variable _might_(?) be used is aa-genprof (multiple do_logprof_pass() runs)

profile_changes = dict()
prelog = hasher()
changed = dict()
created = []
helpers = dict()  # Preserve this between passes # was our
### logprof ends

filelist = hasher()    # File level variables and stuff in config files

def on_exit():
    """Shutdowns the logger and records exit if debugging enabled"""
    debug_logger.debug('Exiting..')
    debug_logger.shutdown()

# Register the on_exit method with atexit
atexit.register(on_exit)

def check_for_LD_XXX(file):
    """Returns True if specified program contains references to LD_PRELOAD or
    LD_LIBRARY_PATH to give the Px/Ux code better suggestions"""
    if not os.path.isfile(file):
        return False
    size = os.stat(file).st_size
    # Limit to checking files under 100k for the sake of speed
    if size > 100000:
        return False
    with open(file, 'rb') as f_in:
        for line in f_in:
            if b'LD_PRELOAD' in line or b'LD_LIBRARY_PATH' in line:
                return True
    return False

def fatal_error(message):
    # Get the traceback to the message
    tb_stack = traceback.format_list(traceback.extract_stack())
    tb_stack = ''.join(tb_stack)
    # Add the traceback to message
    message = tb_stack + '\n\n' + message
    debug_logger.error(message)

    # Else tell user what happened
    aaui.UI_Important(message)
    sys.exit(1)

def check_for_apparmor(filesystem='/proc/filesystems', mounts='/proc/mounts'):
    """Finds and returns the mountpoint for apparmor None otherwise"""
    support_securityfs = False
    aa_mountpoint = None
    if valid_path(filesystem):
        with open_file_read(filesystem) as f_in:
            for line in f_in:
                if 'securityfs' in line:
                    support_securityfs = True
                    break
    if valid_path(mounts) and support_securityfs:
        with open_file_read(mounts) as f_in:
            for line in f_in:
                split = line.split()
                if len(split) > 2 and split[2] == 'securityfs':
                    mountpoint = split[1] + '/apparmor'
                    # Check if apparmor is actually mounted there
                    # XXX valid_path() only checks the syntax, but not if the directory exists!
                    if valid_path(mountpoint) and valid_path(mountpoint + '/profiles'):
                        aa_mountpoint = mountpoint
                        break
    return aa_mountpoint

def which(file):
    """Returns the executable fullpath for the file, None otherwise"""
    if sys.version_info >= (3, 3):
        return shutil.which(file)
    env_dirs = os.getenv('PATH').split(':')
    for env_dir in env_dirs:
        env_path = env_dir + '/' + file
        # Test if the path is executable or not
        if os.access(env_path, os.X_OK):
            return env_path
    return None

def get_full_path(original_path):
    """Return the full path after resolving any symlinks"""
    path = original_path
    link_count = 0
    if not path.startswith('/'):
        path = os.getcwd() + '/' + path
    while os.path.islink(path):
        link_count += 1
        if link_count > 64:
            fatal_error(_("Followed too many links while resolving %s") % (original_path))
        direc, file = os.path.split(path)
        link = os.readlink(path)
        # If the link an absolute path
        if link.startswith('/'):
            path = link
        else:
            # Link is relative path
            path = direc + '/' + link
    return os.path.realpath(path)

def find_executable(bin_path):
    """Returns the full executable path for the given executable, None otherwise"""
    full_bin = None
    if os.path.exists(bin_path):
        full_bin = get_full_path(bin_path)
    else:
        if '/' not in bin_path:
            env_bin = which(bin_path)
            if env_bin:
                full_bin = get_full_path(env_bin)
    if full_bin and os.path.exists(full_bin):
        return full_bin
    return None

def get_profile_filename_from_profile_name(profile, get_new=False):
    """Returns the full profile name for the given profile name"""

    filename = active_profiles.filename_from_profile_name(profile)
    if filename:
        return filename

    if get_new:
        return get_new_profile_filename(profile)

def get_profile_filename_from_attachment(profile, get_new=False):
    """Returns the full profile name for the given attachment"""

    filename = active_profiles.filename_from_attachment(profile)
    if filename:
        return filename

    if get_new:
        return get_new_profile_filename(profile)

def get_new_profile_filename(profile):
    '''Compose filename for a new profile'''
    if profile.startswith('/'):
        # Remove leading /
        profile = profile[1:]
    else:
        profile = "profile_" + profile
    profile = profile.replace('/', '.')
    full_profilename = profile_dir + '/' + profile
    return full_profilename

def name_to_prof_filename(prof_filename):
    """Returns the profile"""
    if prof_filename.startswith(profile_dir):
        profile = prof_filename.split(profile_dir, 1)[1]
        return (prof_filename, profile)
    else:
        bin_path = find_executable(prof_filename)
        if bin_path:
            prof_filename = get_profile_filename_from_attachment(bin_path, True)
            if os.path.isfile(prof_filename):
                return (prof_filename, bin_path)

    return None, None

def complain(path):
    """Sets the profile to complain mode if it exists"""
    prof_filename, name = name_to_prof_filename(path)
    if not prof_filename:
        fatal_error(_("Can't find %s") % path)
    set_complain(prof_filename, name)

def enforce(path):
    """Sets the profile to enforce mode if it exists"""
    prof_filename, name = name_to_prof_filename(path)
    if not prof_filename:
        fatal_error(_("Can't find %s") % path)
    set_enforce(prof_filename, name)

def set_complain(filename, program):
    """Sets the profile to complain mode"""
    aaui.UI_Info(_('Setting %s to complain mode.') % (filename if program is None else program))
    # a force-complain symlink is more packaging-friendly, but breaks caching
    # create_symlink('force-complain', filename)
    delete_symlink('disable', filename)
    change_profile_flags(filename, program, 'complain', True)

def set_enforce(filename, program):
    """Sets the profile to enforce mode"""
    aaui.UI_Info(_('Setting %s to enforce mode.') % (filename if program is None else program))
    delete_symlink('force-complain', filename)
    delete_symlink('disable', filename)
    change_profile_flags(filename, program, 'complain', False)

def delete_symlink(subdir, filename):
    path = filename
    link = re.sub('^%s' % profile_dir, '%s/%s' % (profile_dir, subdir), path)
    if link != path and os.path.islink(link):
        os.remove(link)

def create_symlink(subdir, filename):
    path = filename
    bname = os.path.basename(filename)
    if not bname:
        raise AppArmorException(_('Unable to find basename for %s.') % filename)
    #print(filename)
    link = re.sub('^%s' % profile_dir, '%s/%s' % (profile_dir, subdir), path)
    #print(link)
    #link = link + '/%s'%bname
    #print(link)
    symlink_dir = os.path.dirname(link)
    if not os.path.exists(symlink_dir):
        # If the symlink directory does not exist create it
        os.makedirs(symlink_dir)

    if not os.path.exists(link):
        try:
            os.symlink(filename, link)
        except:
            raise AppArmorException(_('Could not create %(link)s symlink to %(file)s.') % { 'link': link, 'file': filename })

def head(file):
    """Returns the first/head line of the file"""
    first = ''
    if os.path.isfile(file):
        with open_file_read(file) as f_in:
            try:
                first = f_in.readline().rstrip()
            except UnicodeDecodeError:
                pass
            return first
    else:
        raise AppArmorException(_('Unable to read first line from %s: File Not Found') % file)

def get_output(params):
    '''Runs the program with the given args and returns the return code and stdout (as list of lines)'''
    try:
        # Get the output of the program
        output = subprocess.check_output(params)
        ret = 0
    except OSError as e:
        raise AppArmorException(_("Unable to fork: %(program)s\n\t%(error)s") % { 'program': params[0], 'error': str(e) })
    except subprocess.CalledProcessError as e:  # If exit code != 0
        output = e.output
        ret = e.returncode

    output = output.decode('utf-8').split('\n')

    # Remove the extra empty string caused due to \n if present
    if output[len(output) - 1] == '':
        output.pop()

    return (ret, output)

def get_reqs(file):
    """Returns a list of paths from ldd output"""
    pattern1 = re.compile('^\s*\S+ => (\/\S+)')
    pattern2 = re.compile('^\s*(\/\S+)')
    reqs = []

    ldd = conf.find_first_file(cfg['settings'].get('ldd')) or '/usr/bin/ldd'
    if not os.path.isfile(ldd) or not os.access(ldd, os.EX_OK):
        raise AppArmorException('Can\'t find ldd')

    ret, ldd_out = get_output([ldd, file])
    if ret == 0 or ret == 1:
        for line in ldd_out:
            if 'not a dynamic executable' in line:  # comes with ret == 1
                break
            if 'cannot read header' in line:
                break
            if 'statically linked' in line:
                break
            match = pattern1.search(line)
            if match:
                reqs.append(match.groups()[0])
            else:
                match = pattern2.search(line)
                if match:
                    reqs.append(match.groups()[0])
    return reqs

def handle_binfmt(profile, path):
    """Modifies the profile to add the requirements"""
    reqs_processed = dict()
    reqs = get_reqs(path)
    while reqs:
        library = reqs.pop()
        library = get_full_path(library)  # resolve symlinks
        if not reqs_processed.get(library, False):
            if get_reqs(library):
                reqs += get_reqs(library)
            reqs_processed[library] = True

        library_rule = FileRule(library, 'mr', None, FileRule.ALL, owner=False, log_event=True)

        if not is_known_rule(profile, 'file', library_rule):
            globbed_library = glob_common(library)
            if globbed_library:
                # glob_common returns a list, just use the first element (typically '/lib/libfoo.so.*')
                library_rule = FileRule(globbed_library[0], 'mr', None, FileRule.ALL, owner=False)

            profile['file'].add(library_rule)

def get_interpreter_and_abstraction(exec_target):
    '''Check if exec_target is a script.
       If a hashbang is found, check if we have an abstraction for it.

       Returns (interpreter_path, abstraction)
       - interpreter_path is none if exec_target is not a script or doesn't have a hashbang line
       - abstraction is None if no matching abstraction exists'''

    if not os.path.exists(exec_target):
        aaui.UI_Important(_('Execute target %s does not exist!') % exec_target)
        return None, None

    if not os.path.isfile(exec_target):
        aaui.UI_Important(_('Execute target %s is not a file!') % exec_target)
        return None, None

    hashbang = head(exec_target)
    if not hashbang.startswith('#!'):
        return None, None

    # get the interpreter (without parameters)
    interpreter = hashbang[2:].strip().split()[0]
    interpreter_path = get_full_path(interpreter)
    interpreter = re.sub('^(/usr)?/bin/', '', interpreter_path)

    if interpreter in ['bash', 'dash', 'sh']:
        abstraction = 'abstractions/bash'
    elif interpreter == 'perl':
        abstraction = 'abstractions/perl'
    elif re.search('^python([23]|[23]\.[0-9]+)?$', interpreter):
        abstraction = 'abstractions/python'
    elif re.search('^ruby([0-9]+(\.[0-9]+)*)?$', interpreter):
        abstraction = 'abstractions/ruby'
    else:
        abstraction = None

    return interpreter_path, abstraction

def get_inactive_profile(local_profile):
    if extras.get(local_profile, False):
        return {local_profile: extras[local_profile]}
    return dict()

def create_new_profile(localfile, is_stub=False):
    local_profile = hasher()
    local_profile[localfile] = ProfileStorage('NEW', localfile, 'create_new_profile()')
    local_profile[localfile]['flags'] = 'complain'
    local_profile[localfile]['include']['abstractions/base'] = 1

    if os.path.exists(localfile) and os.path.isfile(localfile):
        interpreter_path, abstraction = get_interpreter_and_abstraction(localfile)

        if interpreter_path:
            local_profile[localfile]['file'].add(FileRule(localfile,        'r',  None, FileRule.ALL, owner=False))
            local_profile[localfile]['file'].add(FileRule(interpreter_path, None, 'ix', FileRule.ALL, owner=False))

            if abstraction:
                local_profile[localfile]['include'][abstraction] = True

            handle_binfmt(local_profile[localfile], interpreter_path)
        else:
            local_profile[localfile]['file'].add(FileRule(localfile,        'mr', None, FileRule.ALL, owner=False))

            handle_binfmt(local_profile[localfile], localfile)
    # Add required hats to the profile if they match the localfile
    for hatglob in cfg['required_hats'].keys():
        if re.search(hatglob, localfile):
            for hat in sorted(cfg['required_hats'][hatglob].split()):
                if not local_profile.get(hat, False):
                    local_profile[hat] = ProfileStorage('NEW', hat, 'create_new_profile() required_hats')
                local_profile[hat]['flags'] = 'complain'

    if not is_stub:
        created.append(localfile)
        changed[localfile] = True

    debug_logger.debug("Profile for %s:\n\t%s" % (localfile, local_profile.__str__()))
    return {localfile: local_profile}

def delete_profile(local_prof):
    """Deletes the specified file from the disk and remove it from our list"""
    profile_file = get_profile_filename_from_profile_name(local_prof, True)
    if os.path.isfile(profile_file):
        os.remove(profile_file)
    if aa.get(local_prof, False):
        aa.pop(local_prof)

    #prof_unload(local_prof)

def confirm_and_abort():
    ans = aaui.UI_YesNo(_('Are you sure you want to abandon this set of profile changes and exit?'), 'n')
    if ans == 'y':
        aaui.UI_Info(_('Abandoning all changes.'))
        for prof in created:
            delete_profile(prof)
        sys.exit(0)

def get_profile(prof_name):
    profile_data = None
    distro = cfg['repository']['distro']
    repo_url = cfg['repository']['url']
    # local_profiles = []
    profile_hash = hasher()
    if repo_is_enabled():
        aaui.UI_BusyStart(_('Connecting to repository...'))
        status_ok, ret = fetch_profiles_by_name(repo_url, distro, prof_name)
        aaui.UI_BusyStop()
        if status_ok:
            profile_hash = ret
        else:
            aaui.UI_Important(_('WARNING: Error fetching profiles from the repository'))
    inactive_profile = get_inactive_profile(prof_name)
    if inactive_profile:
        uname = 'Inactive local profile for %s' % prof_name
        inactive_profile[prof_name][prof_name]['flags'] = 'complain'
        orig_filename = inactive_profile[prof_name][prof_name]['filename']  # needed for CMD_VIEW_PROFILE
        inactive_profile[prof_name][prof_name]['filename'] = ''
        profile_hash[uname]['username'] = uname
        profile_hash[uname]['profile_type'] = 'INACTIVE_LOCAL'
        profile_hash[uname]['profile'] = serialize_profile(inactive_profile[prof_name], prof_name, None)
        profile_hash[uname]['profile_data'] = inactive_profile

        # no longer necessary after splitting active and extra profiles
        # existing_profiles.pop(prof_name)  # remove profile filename from list to force storing in /etc/apparmor.d/ instead of extra_profile_dir

    # If no profiles in repo and no inactive profiles
    if not profile_hash.keys():
        return None
    options = []
    tmp_list = []
    preferred_present = False
    preferred_user = cfg['repository'].get('preferred_user', 'NOVELL')

    for p in profile_hash.keys():
        if profile_hash[p]['username'] == preferred_user:
            preferred_present = True
        else:
            tmp_list.append(profile_hash[p]['username'])

    if preferred_present:
        options.append(preferred_user)
    options += tmp_list

    q = aaui.PromptQuestion()
    q.headers = ['Profile', prof_name]
    q.functions = ['CMD_VIEW_PROFILE', 'CMD_USE_PROFILE', 'CMD_CREATE_PROFILE', 'CMD_ABORT']
    q.default = "CMD_VIEW_PROFILE"
    q.options = options
    q.selected = 0

    ans = ''
    while 'CMD_USE_PROFILE' not in ans and 'CMD_CREATE_PROFILE' not in ans:
        ans, arg = q.promptUser()
        p = profile_hash[options[arg]]
        q.selected = options.index(options[arg])
        if ans == 'CMD_VIEW_PROFILE':
            pager = get_pager()
            subprocess.call([pager, orig_filename])
        elif ans == 'CMD_USE_PROFILE':
            if p['profile_type'] == 'INACTIVE_LOCAL':
                profile_data = p['profile_data']
                created.append(prof_name)
            else:
                profile_data = parse_repo_profile(prof_name, repo_url, p)
    return profile_data

def activate_repo_profiles(url, profiles, complain):
    read_profiles()
    try:
        for p in profiles:
            pname = p[0]
            profile_data = parse_repo_profile(pname, url, p[1])
            attach_profile_data(aa, profile_data)
            write_profile(pname)
            if complain:
                fname = get_profile_filename_from_profile_name(pname, True)
                change_profile_flags(profile_dir + fname, None, 'complain', True)
                aaui.UI_Info(_('Setting %s to complain mode.') % pname)
    except Exception as e:
            sys.stderr.write(_("Error activating profiles: %s") % e)

def autodep(bin_name, pname=''):
    bin_full = None
    global repo_cfg
    if not repo_cfg and not cfg['repository'].get('url', False):
        repo_conf = apparmor.config.Config('shell', CONFDIR)
        repo_cfg = repo_conf.read_config('repository.conf')
        if not repo_cfg.get('repository', False) or repo_cfg['repository']['enabled'] == 'later':
            UI_ask_to_enable_repo()
    if bin_name:
        bin_full = find_executable(bin_name)
        #if not bin_full:
        #    bin_full = bin_name
        #if not bin_full.startswith('/'):
            #return None
        # Return if exectuable path not found
        if not bin_full:
            return None
    else:
        bin_full = pname  # for named profiles

    pname = bin_full
    read_inactive_profiles()
    profile_data = get_profile(pname)
    # Create a new profile if no existing profile
    if not profile_data:
        profile_data = create_new_profile(pname)
    file = get_profile_filename_from_profile_name(pname, True)
    profile_data[pname][pname]['filename'] = None  # will be stored in /etc/apparmor.d when saving, so it shouldn't carry the extra_profile_dir filename
    attach_profile_data(aa, profile_data)
    attach_profile_data(original_aa, profile_data)
    if os.path.isfile(profile_dir + '/tunables/global'):
        if not filelist.get(file, False):
            filelist[file] = hasher()
        filelist[file]['include']['tunables/global'] = True
        filelist[file]['profiles'][pname] = hasher()
        filelist[file]['profiles'][pname][pname] = True
    write_profile_ui_feedback(pname)

def get_profile_flags(filename, program):
    # To-Do
    # XXX If more than one profile in a file then second one is being ignored XXX
    # Do we return flags for both or
    flags = ''
    with open_file_read(filename) as f_in:
        for line in f_in:
            if RE_PROFILE_START.search(line):
                matches = parse_profile_start_line(line, filename)
                if (matches['attachment'] is not None):
                    profile_glob = AARE(matches['attachment'], True)
                else:
                    profile_glob = AARE(matches['profile'], True)
                flags = matches['flags']
                if (program is not None and profile_glob.match(program)) or program is None or program == matches['profile']:
                    return flags

    raise AppArmorException(_('%s contains no profile') % filename)

def change_profile_flags(prof_filename, program, flag, set_flag):
    """Reads the old profile file and updates the flags accordingly"""
    # TODO: count the number of matching lines (separated by profile and hat?) and return it
    #       so that code calling this function can make sure to only report success if there was a match
    # TODO: change child profile flags even if program is specified

    found = False

    if not flag or flag.strip() == '':
        raise AppArmorBug('New flag for %s is empty' % prof_filename)

    with open_file_read(prof_filename) as f_in:
        temp_file = tempfile.NamedTemporaryFile('w', prefix=prof_filename, suffix='~', delete=False, dir=profile_dir)
        shutil.copymode(prof_filename, temp_file.name)
        with open_file_write(temp_file.name) as f_out:
            for line in f_in:
                if RE_PROFILE_START.search(line):
                    matches = parse_profile_start_line(line, prof_filename)
                    space = matches['leadingspace'] or ''
                    profile = matches['profile']
                    old_flags = matches['flags']
                    newflags = ', '.join(add_or_remove_flag(old_flags, flag, set_flag))

                    if (matches['attachment'] is not None):
                        profile_glob = AARE(matches['attachment'], True)
                    else:
                        profile_glob = AARE(matches['profile'], False)  # named profiles can come without an attachment path specified ("profile foo {...}")

                    if (program is not None and profile_glob.match(program)) or program is None or program == matches['profile']:
                        found = True
                        if program is not None and program != profile:
                            aaui.UI_Info(_('Warning: profile %s represents multiple programs') % profile)

                        header_data = {
                            'attachment': matches['attachment'] or '',
                            'flags': newflags,
                            'profile_keyword': matches['profile_keyword'],
                            'header_comment': matches['comment'] or '',
                        }
                        line = write_header(header_data, len(space)/2, profile, False, True)
                        line = '%s\n' % line[0]
                elif RE_PROFILE_HAT_DEF.search(line):
                    matches = RE_PROFILE_HAT_DEF.search(line)
                    space = matches.group('leadingspace') or ''
                    hat_keyword = matches.group('hat_keyword')
                    hat = matches.group('hat')
                    old_flags = matches['flags']
                    newflags = ', '.join(add_or_remove_flag(old_flags, flag, set_flag))
                    comment = matches.group('comment') or ''
                    if comment:
                        comment = ' %s' % comment

                    if newflags:
                        line = '%s%s%s flags=(%s) {%s\n' % (space, hat_keyword, hat, newflags, comment)
                    else:
                        line = '%s%s%s {%s\n' % (space, hat_keyword, hat, comment)
                f_out.write(line)
    os.rename(temp_file.name, prof_filename)

    if not found:
        if program is None:
            raise AppArmorException("%(file)s doesn't contain a valid profile (syntax error?)" % {'file': prof_filename})
        else:
            raise AppArmorException("%(file)s doesn't contain a valid profile for %(profile)s (syntax error?)" % {'file': prof_filename, 'profile': program})

def profile_exists(program):
    """Returns True if profile exists, False otherwise"""
    # Check cache of profiles

    if active_profiles.filename_from_attachment(program):
        return True
    # Check the disk for profile
    prof_path = get_profile_filename_from_attachment(program, True)
    #print(prof_path)
    if os.path.isfile(prof_path):
        # Add to cache of profile
        raise AppArmorBug('Reached strange condition in profile_exists(), please open a bugreport!')
        # active_profiles[program] = prof_path
        # return True
    return False

def sync_profile():
    user, passw = get_repo_user_pass()
    if not user or not passw:
        return None
    repo_profiles = []
    changed_profiles = []
    new_profiles = []
    serialize_opts = dict()
    status_ok, ret = fetch_profiles_by_user(cfg['repository']['url'],
                                            cfg['repository']['distro'], user)
    if not status_ok:
        if not ret:
            ret = 'UNKNOWN ERROR'
        aaui.UI_Important(_('WARNING: Error synchronizing profiles with the repository:\n%s\n') % ret)
    else:
        users_repo_profiles = ret
        serialize_opts['NO_FLAGS'] = True
        for prof in sorted(aa.keys()):
            if is_repo_profile([aa[prof][prof]]):
                repo_profiles.append(prof)
            if prof in created:
                p_local = serialize_profile(aa[prof], prof, serialize_opts)
                if not users_repo_profiles.get(prof, False):
                    new_profiles.append(prof)
                    new_profiles.append(p_local)
                    new_profiles.append('')
                else:
                    p_repo = users_repo_profiles[prof]['profile']
                    if p_local != p_repo:
                        changed_profiles.append(prof)
                        changed_profiles.append(p_local)
                        changed_profiles.append(p_repo)
        if repo_profiles:
            for prof in repo_profiles:
                p_local = serialize_profile(aa[prof], prof, serialize_opts)
                if not users_repo_profiles.get(prof, False):
                    new_profiles.append(prof)
                    new_profiles.append(p_local)
                    new_profiles.append('')
                else:
                    p_repo = ''
                    if aa[prof][prof]['repo']['user'] == user:
                        p_repo = users_repo_profiles[prof]['profile']
                    else:
                        status_ok, ret = fetch_profile_by_id(cfg['repository']['url'],
                                                             aa[prof][prof]['repo']['id'])
                        if status_ok:
                            p_repo = ret['profile']
                        else:
                            if not ret:
                                ret = 'UNKNOWN ERROR'
                            aaui.UI_Important(_('WARNING: Error synchronizing profiles with the repository\n%s') % ret)
                            continue
                    if p_repo != p_local:
                        changed_profiles.append(prof)
                        changed_profiles.append(p_local)
                        changed_profiles.append(p_repo)
        if changed_profiles:
            submit_changed_profiles(changed_profiles)
        if new_profiles:
            submit_created_profiles(new_profiles)

def fetch_profile_by_id(url, id):
    #To-Do
    return None, None

def fetch_profiles_by_name(url, distro, user):
    #to-Do
    return None, None

def fetch_profiles_by_user(url, distro, user):
    #to-Do
    return None, None

def submit_created_profiles(new_profiles):
    #url = cfg['repository']['url']
    if new_profiles:
        title = 'Submit newly created profiles to the repository'
        message = 'Would you like to upload newly created profiles?'
        console_select_and_upload_profiles(title, message, new_profiles)

def submit_changed_profiles(changed_profiles):
    #url = cfg['repository']['url']
    if changed_profiles:
        title = 'Submit changed profiles to the repository'
        message = 'The following profiles from the repository were changed.\nWould you like to upload your changes?'
        console_select_and_upload_profiles(title, message, changed_profiles)

def upload_profile(url, user, passw, distro, p, profile_string, changelog):
    # To-Do
    return None, None

def console_select_and_upload_profiles(title, message, profiles_up):
    url = cfg['repository']['url']
    profiles = profiles_up[:]
    q = aaui.PromptQuestion()
    q.title = title
    q.headers = ['Repository', url]
    q.explanation = message
    q.functions = ['CMD_UPLOAD_CHANGES', 'CMD_VIEW_CHANGES', 'CMD_ASK_LATER',
                      'CMD_ASK_NEVER', 'CMD_ABORT']
    q.default = 'CMD_VIEW_CHANGES'
    q.options = [i[0] for i in profiles]
    q.selected = 0
    ans = ''
    while 'CMD_UPLOAD_CHANGES' not in ans and 'CMD_ASK_NEVER' not in ans and 'CMD_ASK_LATER' not in ans:
        ans, arg = q.promptUser()
        if ans == 'CMD_VIEW_CHANGES':
            aaui.UI_Changes(profiles[arg][2], profiles[arg][1])
    if ans == 'CMD_NEVER_ASK':
        set_profiles_local_only([i[0] for i in profiles])
    elif ans == 'CMD_UPLOAD_CHANGES':
        changelog = aaui.UI_GetString(_('Changelog Entry: '), '')
        user, passw = get_repo_user_pass()
        if user and passw:
            for p_data in profiles:
                prof = p_data[0]
                prof_string = p_data[1]
                status_ok, ret = upload_profile(url, user, passw,
                                                cfg['repository']['distro'],
                                                prof, prof_string, changelog)
                if status_ok:
                    newprof = ret
                    newid = newprof['id']
                    set_repo_info(aa[prof][prof], url, user, newid)
                    write_profile_ui_feedback(prof)
                    aaui.UI_Info('Uploaded %s to repository' % prof)
                else:
                    if not ret:
                        ret = 'UNKNOWN ERROR'
                    aaui.UI_Important(_('WARNING: An error occurred while uploading the profile %(profile)s\n%(ret)s') % { 'profile': prof, 'ret': ret })
        else:
            aaui.UI_Important(_('Repository Error\nRegistration or Signin was unsuccessful. User login\ninformation is required to upload profiles to the repository.\nThese changes could not be sent.'))

def set_profiles_local_only(profiles):
    for p in profiles:
        aa[profiles][profiles]['repo']['neversubmit'] = True
        write_profile_ui_feedback(profiles)


def build_x_functions(default, options, exec_toggle):
    ret_list = []
    fallback_toggle = False
    if exec_toggle:
        if 'i' in options:
            ret_list.append('CMD_ix')
            if 'p' in options:
                ret_list.append('CMD_pix')
                fallback_toggle = True
            if 'c' in options:
                ret_list.append('CMD_cix')
                fallback_toggle = True
            if 'n' in options:
                ret_list.append('CMD_nix')
                fallback_toggle = True
            if fallback_toggle:
                ret_list.append('CMD_EXEC_IX_OFF')
        if 'u' in options:
            ret_list.append('CMD_ux')

    else:
        if 'i' in options:
            ret_list.append('CMD_ix')
        if 'c' in options:
            ret_list.append('CMD_cx')
            fallback_toggle = True
        if 'p' in options:
            ret_list.append('CMD_px')
            fallback_toggle = True
        if 'n' in options:
            ret_list.append('CMD_nx')
            fallback_toggle = True
        if 'u' in options:
            ret_list.append('CMD_ux')

        if fallback_toggle:
            ret_list.append('CMD_EXEC_IX_ON')

    ret_list += ['CMD_DENY', 'CMD_ABORT', 'CMD_FINISHED']
    return ret_list

def handle_children(profile, hat, root):
    entries = root[:]
    pid = None
    p = None
    h = None
    prog = None
    aamode = None
    mode = None
    detail = None
    to_name = None
    uhat = None
    capability = None
    family = None
    sock_type = None
    protocol = None
    regex_nullcomplain = re.compile('^null(-complain)*-profile$')

    for entry in entries:
        if type(entry[0]) != str:
            handle_children(profile, hat, entry)
        else:
            typ = entry.pop(0)
            if typ == 'fork':
                # If type is fork then we (should) have pid, profile and hat
                pid, p, h = entry[:3]
                if not regex_nullcomplain.search(p) and not regex_nullcomplain.search(h):
                    profile = p
                    hat = h
                if hat:
                    profile_changes[pid] = profile + '//' + hat
                else:
                    profile_changes[pid] = profile
            elif typ == 'unknown_hat':
                # If hat is not known then we (should) have pid, profile, hat, mode and unknown hat in entry
                pid, p, h, aamode, uhat = entry[:5]
                if not regex_nullcomplain.search(p):
                    profile = p
                if aa[profile].get(uhat, False):
                    hat = uhat
                    continue
                new_p = update_repo_profile(aa[profile][profile])
                if new_p and UI_SelectUpdatedRepoProfile(profile, new_p) and aa[profile].get(uhat, False):
                    hat = uhat
                    continue

                default_hat = None
                for hatglob in cfg.options('defaulthat'):
                    if re.search(hatglob, profile):
                        default_hat = cfg['defaulthat'][hatglob]

                context = profile
                context = context + ' -> ^%s' % uhat
                ans = transitions.get(context, 'XXXINVALIDXXX')

                while ans not in ['CMD_ADDHAT', 'CMD_USEDEFAULT', 'CMD_DENY']:
                    q = aaui.PromptQuestion()
                    q.headers += [_('Profile'), profile]

                    if default_hat:
                        q.headers += [_('Default Hat'), default_hat]

                    q.headers += [_('Requested Hat'), uhat]

                    q.functions.append('CMD_ADDHAT')
                    if default_hat:
                        q.functions.append('CMD_USEDEFAULT')
                    q.functions += ['CMD_DENY', 'CMD_ABORT', 'CMD_FINISHED']

                    q.default = 'CMD_DENY'
                    if aamode == 'PERMITTING':
                        q.default = 'CMD_ADDHAT'

                    ans = q.promptUser()[0]

                    if ans == 'CMD_FINISHED':
                        save_profiles()
                        return

                transitions[context] = ans

                if ans == 'CMD_ADDHAT':
                    hat = uhat
                    aa[profile][hat] = ProfileStorage(profile, hat, 'handle_children addhat')
                    aa[profile][hat]['flags'] = aa[profile][profile]['flags']
                    changed[profile] = True
                elif ans == 'CMD_USEDEFAULT':
                    hat = default_hat
                elif ans == 'CMD_DENY':
                    # As unknown hat is denied no entry for it should be made
                    return None

            elif typ == 'capability':
                # If capability then we (should) have pid, profile, hat, program, mode, capability
                pid, p, h, prog, aamode, capability = entry[:6]
                if not regex_nullcomplain.search(p) and not regex_nullcomplain.search(h):
                    profile = p
                    hat = h
                if not profile or not hat:
                    continue
                prelog[aamode][profile][hat]['capability'][capability] = True

            elif typ == 'dbus':
                # If dbus then we (should) have pid, profile, hat, program, mode, access, bus, name, path, interface, member, peer_profile
                pid, p, h, prog, aamode, access, bus, path, name, interface, member, peer_profile = entry
                if not regex_nullcomplain.search(p) and not regex_nullcomplain.search(h):
                    profile = p
                    hat = h
                if not profile or not hat:
                    continue
                prelog[aamode][profile][hat]['dbus'][access][bus][path][name][interface][member][peer_profile] = True

            elif typ == 'ptrace':
                # If ptrace then we (should) have pid, profile, hat, program, mode, access and peer
                pid, p, h, prog, aamode, access, peer = entry
                if not regex_nullcomplain.search(p) and not regex_nullcomplain.search(h):
                    profile = p
                    hat = h
                if not profile or not hat:
                    continue
                prelog[aamode][profile][hat]['ptrace'][peer][access] = True

            elif typ == 'signal':
                # If signal then we (should) have pid, profile, hat, program, mode, access, signal and peer
                pid, p, h, prog, aamode, access, signal, peer = entry
                if not regex_nullcomplain.search(p) and not regex_nullcomplain.search(h):
                    profile = p
                    hat = h
                if not profile or not hat:
                    continue
                prelog[aamode][profile][hat]['signal'][peer][access][signal] = True

            elif typ == 'path' or typ == 'exec':
                # If path or exec then we (should) have pid, profile, hat, program, mode, details and to_name
                pid, p, h, prog, aamode, mode, detail, to_name = entry[:8]
                if not mode:
                    mode = set()
                if not regex_nullcomplain.search(p) and not regex_nullcomplain.search(h):
                    profile = p
                    hat = h
                if not profile or not hat or not detail:
                    continue

                # Give Execute dialog if x access requested for something that's not a directory
                # For directories force an 'ix' Path dialog
                do_execute = False
                exec_target = detail

                if mode & str_to_mode('x'):
                    if os.path.isdir(exec_target):
                        raise AppArmorBug('exec permissions requested for directory %s. This should not happen - please open a bugreport!' % exec_target)
                    elif typ != 'exec':
                        raise AppArmorBug('exec permissions requested for %(exec_target)s, but mode is %(mode)s instead of exec. This should not happen - please open a bugreport!' % {'exec_target': exec_target, 'mode':mode})
                    else:
                        do_execute = True
                        domainchange = 'change'

                if mode and mode != str_to_mode('x'):  # x is already handled in handle_children, so it must not become part of prelog
                    path = detail

                    if prelog[aamode][profile][hat]['path'].get(path, False):
                        mode |= prelog[aamode][profile][hat]['path'][path]
                    prelog[aamode][profile][hat]['path'][path] = mode

                if do_execute:
                    if not aa[profile][hat]:
                        continue  # ignore log entries for non-existing profiles

                    exec_event = FileRule(exec_target, None, FileRule.ANY_EXEC, FileRule.ALL, owner=False, log_event=True)
                    if is_known_rule(aa[profile][hat], 'file', exec_event):
                        continue

                    p = update_repo_profile(aa[profile][profile])
                    if to_name:
                        if UI_SelectUpdatedRepoProfile(profile, p) and is_known_rule(aa[profile][hat], 'file', exec_event):  # we need an exec_event with target=to_name here
                            continue
                    else:
                        if UI_SelectUpdatedRepoProfile(profile, p) and is_known_rule(aa[profile][hat], 'file', exec_event):  # we need an exec_event with target=exec_target here
                            continue

                    context_new = profile
                    if profile != hat:
                        context_new = context_new + '^%s' % hat
                    context_new = context_new + ' -> %s' % exec_target

                    # nx is not used in profiles but in log files.
                    # Log parsing methods will convert it to its profile form
                    # nx is internally cx/px/cix/pix + to_name
                    exec_mode = False
                    file_perm = None

                    if True:
                        options = cfg['qualifiers'].get(exec_target, 'ipcnu')
                        if to_name:
                            fatal_error(_('%s has transition name but not transition mode') % entry)

                        ### If profiled program executes itself only 'ix' option
                        ##if exec_target == profile:
                            ##options = 'i'

                        # Don't allow hats to cx?
                        options.replace('c', '')
                        # Add deny to options
                        options += 'd'
                        # Define the default option
                        default = None
                        if 'p' in options and os.path.exists(get_profile_filename_from_attachment(exec_target, True)):
                            default = 'CMD_px'
                            sys.stdout.write(_('Target profile exists: %s\n') % get_profile_filename_from_attachment(exec_target, True))
                        elif 'i' in options:
                            default = 'CMD_ix'
                        elif 'c' in options:
                            default = 'CMD_cx'
                        elif 'n' in options:
                            default = 'CMD_nx'
                        else:
                            default = 'DENY'

                        #
                        parent_uses_ld_xxx = check_for_LD_XXX(profile)

                        sev_db.unload_variables()
                        sev_db.load_variables(get_profile_filename_from_profile_name(profile, True))
                        severity = sev_db.rank_path(exec_target, 'x')

                        # Prompt portion starts
                        q = aaui.PromptQuestion()

                        q.headers += [_('Profile'), combine_name(profile, hat)]
                        if prog and prog != 'HINT':
                            q.headers += [_('Program'), prog]

                        # to_name should not exist here since, transitioning is already handeled
                        q.headers += [_('Execute'), exec_target]
                        q.headers += [_('Severity'), severity]

                        # prompt = '\n%s\n' % context_new  # XXX
                        exec_toggle = False
                        q.functions += build_x_functions(default, options, exec_toggle)

                        # ask user about the exec mode to use
                        ans = ''
                        while ans not in ['CMD_ix', 'CMD_px', 'CMD_cx', 'CMD_nx', 'CMD_pix', 'CMD_cix', 'CMD_nix', 'CMD_ux', 'CMD_DENY']:  # add '(I)gnore'? (hotkey conflict with '(i)x'!)
                            ans = q.promptUser()[0]

                            if ans.startswith('CMD_EXEC_IX_'):
                                exec_toggle = not exec_toggle
                                q.functions = build_x_functions(default, options, exec_toggle)
                                ans = ''
                                continue

                            if ans == 'CMD_FINISHED':
                                save_profiles()
                                return

                            if ans == 'CMD_nx' or ans == 'CMD_nix':
                                arg = exec_target
                                ynans = 'n'
                                if profile == hat:
                                    ynans = aaui.UI_YesNo(_('Are you specifying a transition to a local profile?'), 'n')
                                if ynans == 'y':
                                    if ans == 'CMD_nx':
                                        ans = 'CMD_cx'
                                    else:
                                        ans = 'CMD_cix'
                                else:
                                    if ans == 'CMD_nx':
                                        ans = 'CMD_px'
                                    else:
                                        ans = 'CMD_pix'

                                to_name = aaui.UI_GetString(_('Enter profile name to transition to: '), arg)

                            if ans == 'CMD_ix':
                                exec_mode = 'ix'
                            elif ans in ['CMD_px', 'CMD_cx', 'CMD_pix', 'CMD_cix']:
                                exec_mode = ans.replace('CMD_', '')
                                px_msg = _("Should AppArmor sanitise the environment when\nswitching profiles?\n\nSanitising environment is more secure,\nbut some applications depend on the presence\nof LD_PRELOAD or LD_LIBRARY_PATH.")
                                if parent_uses_ld_xxx:
                                    px_msg = _("Should AppArmor sanitise the environment when\nswitching profiles?\n\nSanitising environment is more secure,\nbut this application appears to be using LD_PRELOAD\nor LD_LIBRARY_PATH and sanitising the environment\ncould cause functionality problems.")

                                ynans = aaui.UI_YesNo(px_msg, 'y')
                                if ynans == 'y':
                                    # Disable the unsafe mode
                                    exec_mode = exec_mode.capitalize()
                            elif ans == 'CMD_ux':
                                exec_mode = 'ux'
                                ynans = aaui.UI_YesNo(_("Launching processes in an unconfined state is a very\ndangerous operation and can cause serious security holes.\n\nAre you absolutely certain you wish to remove all\nAppArmor protection when executing %s ?") % exec_target, 'n')
                                if ynans == 'y':
                                    ynans = aaui.UI_YesNo(_("Should AppArmor sanitise the environment when\nrunning this program unconfined?\n\nNot sanitising the environment when unconfining\na program opens up significant security holes\nand should be avoided if at all possible."), 'y')
                                    if ynans == 'y':
                                        # Disable the unsafe mode
                                        exec_mode = exec_mode.capitalize()
                                else:
                                    ans = 'INVALID'

                        if exec_mode and 'i' in exec_mode:
                            # For inherit we need mr
                            file_perm = 'mr'
                        else:
                            if ans == 'CMD_DENY':
                                aa[profile][hat]['file'].add(FileRule(exec_target, None, 'x', FileRule.ALL, owner=False, log_event=True, deny=True))
                                changed[profile] = True
                                # Skip remaining events if they ask to deny exec
                                if domainchange == 'change':
                                    return None

                        if ans != 'CMD_DENY':
                            if to_name:
                                rule_to_name = to_name
                            else:
                                rule_to_name = FileRule.ALL

                            aa[profile][hat]['file'].add(FileRule(exec_target, file_perm, exec_mode, rule_to_name, owner=False, log_event=True))

                            changed[profile] = True

                            if 'i' in exec_mode:
                                interpreter_path, abstraction = get_interpreter_and_abstraction(exec_target)

                                if interpreter_path:
                                    aa[profile][hat]['file'].add(FileRule(exec_target,      'r',  None, FileRule.ALL, owner=False))
                                    aa[profile][hat]['file'].add(FileRule(interpreter_path, None, 'ix', FileRule.ALL, owner=False))

                                    if abstraction:
                                        aa[profile][hat]['include'][abstraction] = True

                                    handle_binfmt(aa[profile][hat], interpreter_path)

                    # Update tracking info based on kind of change

                    if ans == 'CMD_ix':
                        if hat:
                            profile_changes[pid] = '%s//%s' % (profile, hat)
                        else:
                            profile_changes[pid] = '%s//' % profile
                    elif re.search('^CMD_(px|nx|pix|nix)', ans):
                        if to_name:
                            exec_target = to_name
                        if aamode == 'PERMITTING':
                            if domainchange == 'change':
                                profile = exec_target
                                hat = exec_target
                                profile_changes[pid] = '%s' % profile

                        # Check profile exists for px
                        if not os.path.exists(get_profile_filename_from_attachment(exec_target, True)):
                            ynans = 'y'
                            if 'i' in exec_mode:
                                ynans = aaui.UI_YesNo(_('A profile for %s does not exist.\nDo you want to create one?') % exec_target, 'n')
                            if ynans == 'y':
                                helpers[exec_target] = 'enforce'
                                if to_name:
                                    autodep('', exec_target)
                                else:
                                    autodep(exec_target, '')
                                reload_base(exec_target)
                    elif ans.startswith('CMD_cx') or ans.startswith('CMD_cix'):
                        if to_name:
                            exec_target = to_name
                        if aamode == 'PERMITTING':
                            if domainchange == 'change':
                                profile_changes[pid] = '%s//%s' % (profile, exec_target)

                        if not aa[profile].get(exec_target, False):
                            ynans = 'y'
                            if 'i' in exec_mode:
                                ynans = aaui.UI_YesNo(_('A profile for %s does not exist.\nDo you want to create one?') % exec_target, 'n')
                            if ynans == 'y':
                                hat = exec_target
                                if not aa[profile].get(hat, False):
                                    stub_profile = create_new_profile(hat, True)
                                    aa[profile][hat] = stub_profile[hat][hat]

                                aa[profile][hat]['profile'] = True

                                if profile != hat:
                                    aa[profile][hat]['flags'] = aa[profile][profile]['flags']

                                aa[profile][hat]['flags'] = 'complain'

                                file_name = aa[profile][profile]['filename']
                                filelist[file_name]['profiles'][profile][hat] = True

                    elif ans.startswith('CMD_ux'):
                        profile_changes[pid] = 'unconfined'
                        if domainchange == 'change':
                            return None

            elif typ == 'netdomain':
                # If netdomain we (should) have pid, profile, hat, program, mode, network family, socket type and protocol
                pid, p, h, prog, aamode, family, sock_type, protocol = entry[:8]

                if not regex_nullcomplain.search(p) and not regex_nullcomplain.search(h):
                    profile = p
                    hat = h
                if not hat or not profile:
                    continue
                if family and sock_type:
                    prelog[aamode][profile][hat]['netdomain'][family][sock_type] = True

    return None

##### Repo related functions

def UI_SelectUpdatedRepoProfile(profile, p):
    # To-Do
    return False

def UI_repo_signup():
    # To-Do
    return None, None

def UI_ask_to_enable_repo():
    # To-Do
    pass

def UI_ask_to_upload_profiles():
    # To-Do
    pass

def parse_repo_profile(fqdbin, repo_url, profile):
    # To-Do
    pass

def set_repo_info(profile_data, repo_url, username, iden):
    # To-Do
    pass

def is_repo_profile(profile_data):
    # To-Do
    pass

def get_repo_user_pass():
    # To-Do
    pass
def get_preferred_user(repo_url):
    # To-Do
    pass
def repo_is_enabled():
    # To-Do
    return False

def update_repo_profile(profile):
    # To-Do
    return None

def order_globs(globs, original_path):
    """Returns the globs in sorted order, more specific behind"""
    # To-Do
    # ATM its lexicographic, should be done to allow better matches later

    globs = sorted(globs)

    # make sure the original path is always the last option
    if original_path in globs:
        globs.remove(original_path)
    globs.append(original_path)

    return globs

def ask_the_questions(log_dict):
    for aamode in sorted(log_dict.keys()):
        # Describe the type of changes
        if aamode == 'PERMITTING':
            aaui.UI_Info(_('Complain-mode changes:'))
        elif aamode == 'REJECTING':
            aaui.UI_Info(_('Enforce-mode changes:'))
        elif aamode == 'merge':
            pass  # aa-mergeprof
        else:
            raise AppArmorBug(_('Invalid mode found: %s') % aamode)

        for profile in sorted(log_dict[aamode].keys()):
            # Update the repo profiles
            p = update_repo_profile(aa[profile][profile])
            if p:
                UI_SelectUpdatedRepoProfile(profile, p)

            sev_db.unload_variables()
            sev_db.load_variables(get_profile_filename_from_profile_name(profile, True))

            # Sorted list of hats with the profile name coming first
            hats = list(filter(lambda key: key != profile, sorted(log_dict[aamode][profile].keys())))
            if log_dict[aamode][profile].get(profile, False):
                hats = [profile] + hats

            for hat in hats:

                if not aa[profile].get(hat, {}).get('file'):
                    if aamode != 'merge':
                        # Ignore log events for a non-existing profile or child profile. Such events can occour
                        # after deleting a profile or hat manually, or when processing a foreign log.
                        # (Checking for 'file' is a simplified way to check if it's a ProfileStorage.)
                        debug_logger.debug("Ignoring events for non-existing profile %s" % combine_name(profile, hat))
                        continue

                    ans = ''
                    while ans not in ['CMD_ADDHAT', 'CMD_ADDSUBPROFILE', 'CMD_DENY']:
                        q = aaui.PromptQuestion()
                        q.headers += [_('Profile'), profile]

                        if log_dict[aamode][profile][hat]['profile']:
                            q.headers += [_('Requested Subprofile'), hat]
                            q.functions.append('CMD_ADDSUBPROFILE')
                        else:
                            q.headers += [_('Requested Hat'), hat]
                            q.functions.append('CMD_ADDHAT')

                        q.functions += ['CMD_DENY', 'CMD_ABORT', 'CMD_FINISHED']

                        q.default = 'CMD_DENY'

                        ans = q.promptUser()[0]

                        if ans == 'CMD_FINISHED':
                            return

                    if ans == 'CMD_DENY':
                        continue  # don't ask about individual rules if the user doesn't want the additional subprofile/hat

                    if log_dict[aamode][profile][hat]['profile']:
                        aa[profile][hat] = ProfileStorage(profile, hat, 'mergeprof ask_the_questions() - missing subprofile')
                        aa[profile][hat]['profile'] = True
                    else:
                        aa[profile][hat] = ProfileStorage(profile, hat, 'mergeprof ask_the_questions() - missing hat')
                        aa[profile][hat]['profile'] = False

                #Add the includes from the other profile to the user profile
                done = False

                options = []
                for inc in log_dict[aamode][profile][hat]['include'].keys():
                    if not inc in aa[profile][hat]['include'].keys():
                        if inc.startswith('/'):
                            options.append('#include "%s"' %inc)
                        else:
                            options.append('#include <%s>' %inc)

                default_option = 1

                q = aaui.PromptQuestion()
                q.options = options
                q.selected = default_option - 1
                q.headers = [_('File includes'), _('Select the ones you wish to add')]
                q.functions = ['CMD_ALLOW', 'CMD_IGNORE_ENTRY', 'CMD_ABORT', 'CMD_FINISHED']
                q.default = 'CMD_ALLOW'

                while not done and options:
                    ans, selected = q.promptUser()
                    if ans == 'CMD_IGNORE_ENTRY':
                        done = True
                    elif ans == 'CMD_ALLOW':
                        selection = options[selected]
                        inc = re_match_include(selection)
                        deleted = apparmor.aa.delete_duplicates(aa[profile][hat], inc)
                        aa[profile][hat]['include'][inc] = True
                        options.pop(selected)
                        aaui.UI_Info(_('Adding %s to the file.') % selection)
                        if deleted:
                            aaui.UI_Info(_('Deleted %s previous matching profile entries.') % deleted)
                    elif ans == 'CMD_FINISHED':
                        return

                # check for and ask about conflicting exec modes
                ask_conflict_mode(profile, hat, aa[profile][hat], log_dict[aamode][profile][hat])

                for ruletype in ruletypes:
                    for rule_obj in log_dict[aamode][profile][hat][ruletype].rules:

                        if is_known_rule(aa[profile][hat], ruletype, rule_obj):
                            continue

                        default_option = 1
                        options = []
                        newincludes = match_includes(aa[profile][hat], ruletype, rule_obj)
                        q = aaui.PromptQuestion()
                        if newincludes:
                            options += list(map(lambda inc: '#include <%s>' % inc, sorted(set(newincludes))))

                        if ruletype == 'file' and rule_obj.path:
                            options += propose_file_rules(aa[profile][hat], rule_obj)
                        else:
                            options.append(rule_obj.get_clean())

                        done = False
                        while not done:
                            q.options = options
                            q.selected = default_option - 1
                            q.headers = [_('Profile'), combine_name(profile, hat)]
                            q.headers += rule_obj.logprof_header()

                            # Load variables into sev_db? Not needed/used for capabilities and network rules.
                            severity = rule_obj.severity(sev_db)
                            if severity != sev_db.NOT_IMPLEMENTED:
                                q.headers += [_('Severity'), severity]

                            q.functions = available_buttons(rule_obj)

                            # In complain mode: events default to allow
                            # In enforce mode: events default to deny
                            # XXX does this behaviour really make sense, except for "historical reasons"[tm]?
                            q.default = 'CMD_DENY'
                            if rule_obj.log_event == 'PERMITTING':
                                q.default = 'CMD_ALLOW'

                            ans, selected = q.promptUser()
                            selection = options[selected]

                            if ans == 'CMD_IGNORE_ENTRY':
                                done = True
                                break

                            elif ans == 'CMD_FINISHED':
                                return

                            elif ans.startswith('CMD_AUDIT'):
                                if ans == 'CMD_AUDIT_NEW':
                                    rule_obj.audit = True
                                    rule_obj.raw_rule = None
                                else:
                                    rule_obj.audit = False
                                    rule_obj.raw_rule = None

                                options = set_options_audit_mode(rule_obj, options)

                            elif ans.startswith('CMD_USER_'):
                                if ans == 'CMD_USER_ON':
                                    rule_obj.owner = True
                                    rule_obj.raw_rule = None
                                else:
                                    rule_obj.owner = False
                                    rule_obj.raw_rule = None

                                options = set_options_owner_mode(rule_obj, options)

                            elif ans == 'CMD_ALLOW':
                                done = True
                                changed[profile] = True

                                inc = re_match_include(selection)
                                if inc:
                                    deleted = delete_duplicates(aa[profile][hat], inc)

                                    aa[profile][hat]['include'][inc] = True

                                    aaui.UI_Info(_('Adding %s to profile.') % selection)
                                    if deleted:
                                        aaui.UI_Info(_('Deleted %s previous matching profile entries.') % deleted)

                                else:
                                    rule_obj = selection_to_rule_obj(rule_obj, selection)
                                    deleted = aa[profile][hat][ruletype].add(rule_obj, cleanup=True)

                                    aaui.UI_Info(_('Adding %s to profile.') % rule_obj.get_clean())
                                    if deleted:
                                        aaui.UI_Info(_('Deleted %s previous matching profile entries.') % deleted)

                            elif ans == 'CMD_DENY':
                                if re_match_include(selection):
                                    aaui.UI_Important("Denying via an include file isn't supported by the AppArmor tools")

                                else:
                                    done = True
                                    changed[profile] = True

                                    rule_obj = selection_to_rule_obj(rule_obj, selection)
                                    rule_obj.deny = True
                                    rule_obj.raw_rule = None  # reset raw rule after manually modifying rule_obj
                                    deleted = aa[profile][hat][ruletype].add(rule_obj, cleanup=True)
                                    aaui.UI_Info(_('Adding %s to profile.') % rule_obj.get_clean())
                                    if deleted:
                                        aaui.UI_Info(_('Deleted %s previous matching profile entries.') % deleted)

                            elif ans == 'CMD_GLOB':
                                if not re_match_include(selection):
                                    globbed_rule_obj = selection_to_rule_obj(rule_obj, selection)
                                    globbed_rule_obj.glob()
                                    options, default_option = add_to_options(options, globbed_rule_obj.get_raw())

                            elif ans == 'CMD_GLOBEXT':
                                if not re_match_include(selection):
                                    globbed_rule_obj = selection_to_rule_obj(rule_obj, selection)
                                    globbed_rule_obj.glob_ext()
                                    options, default_option = add_to_options(options, globbed_rule_obj.get_raw())

                            elif ans == 'CMD_NEW':
                                if not re_match_include(selection):
                                    edit_rule_obj = selection_to_rule_obj(rule_obj, selection)
                                    prompt, oldpath = edit_rule_obj.edit_header()

                                    newpath = aaui.UI_GetString(prompt, oldpath)
                                    if newpath:
                                        try:
                                            input_matches_path = rule_obj.validate_edit(newpath)  # note that we check against the original rule_obj here, not edit_rule_obj (which might be based on a globbed path)
                                        except AppArmorException:
                                            aaui.UI_Important(_('The path you entered is invalid (not starting with / or a variable)!'))
                                            continue

                                        if not input_matches_path:
                                            ynprompt = _('The specified path does not match this log entry:\n\n  Log Entry: %(path)s\n  Entered Path:  %(ans)s\nDo you really want to use this path?') % { 'path': oldpath, 'ans': newpath }
                                            key = aaui.UI_YesNo(ynprompt, 'n')
                                            if key == 'n':
                                                continue

                                        edit_rule_obj.store_edit(newpath)
                                        options, default_option = add_to_options(options, edit_rule_obj.get_raw())
                                        user_globs[newpath] = AARE(newpath, True)

                            else:
                                done = False

def selection_to_rule_obj(rule_obj, selection):
    rule_type = type(rule_obj)
    return rule_type.parse(selection)

def set_options_audit_mode(rule_obj, options):
    '''change audit state in options (proposed rules) to audit state in rule_obj.
       #include options will be kept unchanged
    '''
    return set_options_mode(rule_obj, options, 'audit')

def set_options_owner_mode(rule_obj, options):
    '''change owner state in options (proposed rules) to owner state in rule_obj.
       #include options will be kept unchanged
    '''
    return set_options_mode(rule_obj, options, 'owner')

def set_options_mode(rule_obj, options, what):
    ''' helper function for set_options_audit_mode() and set_options_owner_mode'''
    new_options = []

    for rule in options:
        if re_match_include(rule):
            new_options.append(rule)
        else:
            parsed_rule = selection_to_rule_obj(rule_obj, rule)
            if what == 'audit':
                parsed_rule.audit = rule_obj.audit
            elif what == 'owner':
                parsed_rule.owner = rule_obj.owner
            else:
                raise AppArmorBug('Unknown "what" value given to set_options_mode: %s' % what)

            parsed_rule.raw_rule = None
            new_options.append(parsed_rule.get_raw())

    return new_options

def available_buttons(rule_obj):
    buttons = []

    if not rule_obj.deny:
        buttons += ['CMD_ALLOW']

    buttons += ['CMD_DENY', 'CMD_IGNORE_ENTRY']

    if rule_obj.can_glob:
        buttons += ['CMD_GLOB']

    if rule_obj.can_glob_ext:
        buttons += ['CMD_GLOBEXT']

    if rule_obj.can_edit:
        buttons += ['CMD_NEW']

    if rule_obj.audit:
        buttons += ['CMD_AUDIT_OFF']
    else:
        buttons += ['CMD_AUDIT_NEW']

    if rule_obj.can_owner:
        if rule_obj.owner:
            buttons += ['CMD_USER_OFF']
        else:
            buttons += ['CMD_USER_ON']

    buttons += ['CMD_ABORT', 'CMD_FINISHED']

    return buttons

def add_to_options(options, newpath):
    if newpath not in options:
        options.append(newpath)

    default_option = options.index(newpath) + 1
    return (options, default_option)

def delete_duplicates(profile, incname):
    deleted = 0
    # Allow rules covered by denied rules shouldn't be deleted
    # only a subset allow rules may actually be denied

    if include.get(incname, False):
        for rule_type in ruletypes:
            deleted += profile[rule_type].delete_duplicates(include[incname][incname][rule_type])

    elif filelist.get(incname, False):
        for rule_type in ruletypes:
            deleted += profile[rule_type].delete_duplicates(filelist[incname][incname][rule_type])

    return deleted

def ask_conflict_mode(profile, hat, old_profile, merge_profile):
    '''ask user about conflicting exec rules'''
    for oldrule in old_profile['file'].rules:
        conflictingrules = merge_profile['file'].get_exec_conflict_rules(oldrule)

        if conflictingrules.rules:
            q = aaui.PromptQuestion()
            q.headers = [_('Path'), oldrule.path.regex]
            q.headers += [_('Select the appropriate mode'), '']
            options = []
            options.append(oldrule.get_clean())
            for rule in conflictingrules.rules:
                options.append(rule.get_clean())
            q.options = options
            q.functions = ['CMD_ALLOW', 'CMD_ABORT']
            done = False
            while not done:
                ans, selected = q.promptUser()
                if ans == 'CMD_ALLOW':
                    if selected == 0:
                        pass  # just keep the existing rule
                    elif selected > 0:
                        # replace existing rule with merged one
                        old_profile['file'].delete(oldrule)
                        old_profile['file'].add(conflictingrules.rules[selected - 1])
                    else:
                        raise AppArmorException(_('Unknown selection'))

                    for rule in conflictingrules.rules:
                        merge_profile['file'].delete(rule)  # make sure aa-mergeprof doesn't ask to add conflicting rules later

                    done = True

def get_include_path(incname):
    if incname.startswith('/'):
        return incname
    return profile_dir + '/' + incname

def match_includes(profile, rule_type, rule_obj):
    newincludes = []
    for incname in include.keys():
        # XXX type check should go away once we init all profiles correctly
        if valid_include(profile, incname) and include[incname][incname].get(rule_type, False) and include[incname][incname][rule_type].is_covered(rule_obj):
            newincludes.append(incname)

    return newincludes

def valid_include(profile, incname):
    if profile and profile['include'].get(incname, False):
        return False

    if cfg['settings']['custom_includes']:
        for incm in cfg['settings']['custom_includes'].split():
            if incm == incname:
                return True

    if incname.startswith('abstractions/') and os.path.isfile(profile_dir + '/' + incname):
        return True
    elif incname.startswith('/') and os.path.isfile(incname):
        return True

    return False

def set_logfile(filename):
    ''' set logfile to a) the specified filename or b) if not given, the first existing logfile from logprof.conf'''

    global logfile

    if filename:
        logfile = filename
    else:
        logfile = conf.find_first_file(cfg['settings']['logfiles']) or '/var/log/syslog'

    if not os.path.exists(logfile):
        if filename:
            raise AppArmorException(_('The logfile %s does not exist. Please check the path') % logfile)
        else:
            raise AppArmorException('Can\'t find system log "%s".' % (logfile))
    elif os.path.isdir(logfile):
        raise AppArmorException(_('%s is a directory. Please specify a file as logfile') % logfile)

def do_logprof_pass(logmark='', passno=0, log_pid=log_pid):
    # set up variables for this pass
#    transitions = hasher()
    global active_profiles
    global sev_db
#    aa = hasher()
#    profile_changes = hasher()
#     prelog = hasher()
#     changed = dict()
#    filelist = hasher()

    aaui.UI_Info(_('Reading log entries from %s.') % logfile)

    if not passno:
        aaui.UI_Info(_('Updating AppArmor profiles in %s.') % profile_dir)
        read_profiles()

    if not sev_db:
        sev_db = apparmor.severity.Severity(CONFDIR + '/severity.db', _('unknown'))
    #print(pid)
    #print(active_profiles)
    ##if not repo_cf and cfg['repostory']['url']:
    ##    repo_cfg = read_config('repository.conf')
    ##    if not repo_cfg['repository'].get('enabled', False) or repo_cfg['repository]['enabled'] not in ['yes', 'no']:
    ##    UI_ask_to_enable_repo()

    log_reader = apparmor.logparser.ReadLog(log_pid, logfile, active_profiles, profile_dir)
    log = log_reader.read_log(logmark)
    #read_log(logmark)

    for root in log:
        handle_children('', '', root)
    #for root in range(len(log)):
        #log[root] = handle_children('', '', log[root])
    #print(log)
    for pid in sorted(profile_changes.keys()):
        set_process(pid, profile_changes[pid])

    log_dict = collapse_log()

    ask_the_questions(log_dict)

    finishing = False
    # Check for finished
    save_profiles()

    ##if not repo_cfg['repository'].get('upload', False) or repo['repository']['upload'] == 'later':
    ##    UI_ask_to_upload_profiles()
    ##if repo_enabled():
    ##    if repo_cgf['repository']['upload'] == 'yes':
    ##        sync_profiles()
    ##    created = []

    # If user selects 'Finish' then we want to exit logprof
    if finishing:
        return 'FINISHED'
    else:
        return 'NORMAL'


def save_profiles():
    # Ensure the changed profiles are actual active profiles
    for prof_name in changed.keys():
        if not is_active_profile(prof_name):
            print("*** save_profiles(): removing %s" % prof_name)
            print('*** This should not happen. Please open a bugreport!')
            changed.pop(prof_name)

    changed_list = sorted(changed.keys())

    if changed_list:
        q = aaui.PromptQuestion()
        q.title = 'Changed Local Profiles'
        q.explanation = _('The following local profiles were changed. Would you like to save them?')
        q.functions = ['CMD_SAVE_CHANGES', 'CMD_SAVE_SELECTED', 'CMD_VIEW_CHANGES', 'CMD_VIEW_CHANGES_CLEAN', 'CMD_ABORT']
        q.default = 'CMD_VIEW_CHANGES'
        q.selected = 0
        ans = ''
        arg = None
        while ans != 'CMD_SAVE_CHANGES':
            if not changed:
                return

            options = sorted(changed.keys())
            q.options = options

            ans, arg = q.promptUser()

            q.selected = arg  # remember selection
            which = options[arg]

            if ans == 'CMD_SAVE_SELECTED':
                write_profile_ui_feedback(which)
                reload_base(which)
                q.selected = 0  # saving the selected profile removes it from the list, therefore reset selection

            elif ans == 'CMD_VIEW_CHANGES':
                oldprofile = None
                if aa[which][which].get('filename', False):
                    oldprofile = aa[which][which]['filename']
                else:
                    oldprofile = get_profile_filename_from_attachment(which, True)

                serialize_options = {}
                serialize_options['METADATA'] = True
                newprofile = serialize_profile(aa[which], which, serialize_options)

                aaui.UI_Changes(oldprofile, newprofile, comments=True)

            elif ans == 'CMD_VIEW_CHANGES_CLEAN':
                oldprofile = serialize_profile(original_aa[which], which, '')
                newprofile = serialize_profile(aa[which], which, '')

                aaui.UI_Changes(oldprofile, newprofile)

        for profile_name in sorted(changed.keys()):
            write_profile_ui_feedback(profile_name)
            reload_base(profile_name)

def get_pager():
    return 'less'

def set_process(pid, profile):
    # If process not running don't do anything
    if not os.path.exists('/proc/%s/attr/current' % pid):
        return None

    process = None
    try:
        process = open_file_read('/proc/%s/attr/current' % pid)
    except IOError:
        return None
    current = process.readline().strip()
    process.close()

    if not re.search('^null(-complain)*-profile$', current):
        return None

    stats = None
    try:
        stats = open_file_read('/proc/%s/stat' % pid)
    except IOError:
        return None
    stat = stats.readline().strip()
    stats.close()

    match = re.search('^\d+ \((\S+)\) ', stat)
    if not match:
        return None

    try:
        process = open_file_write('/proc/%s/attr/current' % pid)
    except IOError:
        return None
    process.write('setprofile %s' % profile)
    process.close()

def collapse_log():
    log_dict = hasher()
    for aamode in prelog.keys():
        for profile in prelog[aamode].keys():
            for hat in prelog[aamode][profile].keys():

                log_dict[aamode][profile][hat] = ProfileStorage(profile, hat, 'collapse_log()')

                for path in prelog[aamode][profile][hat]['path'].keys():
                    mode = prelog[aamode][profile][hat]['path'][path]

                    user, other = split_mode(mode)

                    # logparser.py doesn't preserve 'owner' information, see https://bugs.launchpad.net/apparmor/+bug/1538340
                    # XXX re-check this code after fixing this bug
                    if other:
                        owner = False
                        mode = other
                    else:
                        owner = True
                        mode = user

                    # python3 aa-logprof -f <(echo '[55826.822365] audit: type=1400 audit(1454355221.096:85479): apparmor="ALLOWED" operation="file_receive" profile="/usr/sbin/smbd" name="/foo.png" pid=28185 comm="smbd" requested_mask="w" denied_mask="w" fsuid=100 ouid=100')
                    # happens via log_str_to_mode() called in logparser.py parse_event_for_tree()
                    # XXX fix this in the log parsing!
                    if 'a' in mode and 'w' in mode:
                        mode.remove('a')

                    file_event = FileRule(path, mode, None, FileRule.ALL, owner=owner, log_event=True)

                    if not is_known_rule(aa[profile][hat], 'file', file_event):
                        log_dict[aamode][profile][hat]['file'].add(file_event)

                for cap in prelog[aamode][profile][hat]['capability'].keys():
                    cap_event = CapabilityRule(cap, log_event=True)
                    if not is_known_rule(aa[profile][hat], 'capability', cap_event):
                        log_dict[aamode][profile][hat]['capability'].add(cap_event)

                dbus = prelog[aamode][profile][hat]['dbus']
                for access in                               dbus:
                    for bus in                              dbus[access]:
                        for path in                         dbus[access][bus]:
                            for name in                     dbus[access][bus][path]:
                                for interface in            dbus[access][bus][path][name]:
                                    for member in           dbus[access][bus][path][name][interface]:
                                        for peer_profile in dbus[access][bus][path][name][interface][member]:
                                            # Depending on the access type, not all parameters are allowed.
                                            # Ignore them, even if some of them appear in the log.
                                            # Also, the log doesn't provide a peer name, therefore always use ALL.
                                            if access in ['send', 'receive']:
                                                dbus_event = DbusRule(access, bus, path,            DbusRule.ALL,   interface,   member,        DbusRule.ALL,   peer_profile, log_event=True)
                                            elif access == 'bind':
                                                dbus_event = DbusRule(access, bus, DbusRule.ALL,    name,           DbusRule.ALL, DbusRule.ALL, DbusRule.ALL,   DbusRule.ALL, log_event=True)
                                            elif access == 'eavesdrop':
                                                dbus_event = DbusRule(access, bus, DbusRule.ALL,    DbusRule.ALL,   DbusRule.ALL, DbusRule.ALL, DbusRule.ALL,   DbusRule.ALL, log_event=True)
                                            else:
                                                raise AppArmorBug('unexpected dbus access: %s')

                                            log_dict[aamode][profile][hat]['dbus'].add(dbus_event)

                nd = prelog[aamode][profile][hat]['netdomain']
                for family in nd.keys():
                    for sock_type in nd[family].keys():
                        net_event = NetworkRule(family, sock_type, log_event=True)
                        if not is_known_rule(aa[profile][hat], 'network', net_event):
                            log_dict[aamode][profile][hat]['network'].add(net_event)

                ptrace = prelog[aamode][profile][hat]['ptrace']
                for peer in ptrace.keys():
                    for access in ptrace[peer].keys():
                        ptrace_event = PtraceRule(access, peer, log_event=True)
                        if not is_known_rule(aa[profile][hat], 'ptrace', ptrace_event):
                            log_dict[aamode][profile][hat]['ptrace'].add(ptrace_event)

                sig = prelog[aamode][profile][hat]['signal']
                for peer in sig.keys():
                    for access in sig[peer].keys():
                        for signal in sig[peer][access].keys():
                            signal_event = SignalRule(access, signal, peer, log_event=True)
                            if not is_known_rule(aa[profile][hat], 'signal', signal_event):
                                log_dict[aamode][profile][hat]['signal'].add(signal_event)

    return log_dict

def is_skippable_file(path):
    """Returns True if filename matches something to be skipped (rpm or dpkg backup files, hidden files etc.)
        The list of skippable files needs to be synced with apparmor initscript and libapparmor _aa_is_blacklisted()
        path: filename (with or without directory)"""

    basename = os.path.basename(path)

    if not basename or basename[0] == '.' or basename == 'README':
        return True

    skippable_suffix = ('.dpkg-new', '.dpkg-old', '.dpkg-dist', '.dpkg-bak', '.dpkg-remove', '.pacsave', '.pacnew', '.rpmnew', '.rpmsave', '.orig', '.rej', '~')
    if basename.endswith(skippable_suffix):
        return True

    return False

def is_skippable_dir(path):
    if re.search('^(.*/)?(disable|cache|cache\.d|force-complain|lxc|\.git)/?$', path):
        return True
    return False

def read_profiles():
    # we'll read all profiles from disk, so reset the storage first (autodep() might have created/stored
    # a profile already, which would cause a 'Conflicting profile' error in attach_profile_data())
    global aa, original_aa
    aa = hasher()
    original_aa = hasher()

    try:
        os.listdir(profile_dir)
    except:
        fatal_error(_("Can't read AppArmor profiles in %s") % profile_dir)

    for file in os.listdir(profile_dir):
        if os.path.isfile(profile_dir + '/' + file):
            if is_skippable_file(file):
                continue
            else:
                read_profile(profile_dir + '/' + file, True)

def read_inactive_profiles():
    if hasattr(read_inactive_profiles, 'already_read'):
        # each autodep() run calls read_inactive_profiles, but that's a) superfluous and b) triggers a conflict because the inactive profiles are already loaded
        # therefore don't do anything if the inactive profiles were already loaded
        return

    read_inactive_profiles.already_read = True

    if not os.path.exists(extra_profile_dir):
        return None
    try:
        os.listdir(profile_dir)
    except:
        fatal_error(_("Can't read AppArmor profiles in %s") % extra_profile_dir)

    for file in os.listdir(extra_profile_dir):
        if os.path.isfile(extra_profile_dir + '/' + file):
            if is_skippable_file(file):
                continue
            else:
                read_profile(extra_profile_dir + '/' + file, False)

def read_profile(file, active_profile):
    data = None
    try:
        with open_file_read(file) as f_in:
            data = f_in.readlines()
    except IOError:
        debug_logger.debug("read_profile: can't read %s - skipping" % file)
        return None

    profile_data = parse_profile_data(data, file, 0)

    if profile_data and active_profile:
        attach_profile_data(aa, profile_data)
        attach_profile_data(original_aa, profile_data)

        for profile in profile_data:  # TODO: also honor hats
            name = profile_data[profile][profile]['name']
            attachment = profile_data[profile][profile]['attachment']
            filename = profile_data[profile][profile]['filename']

            if not attachment and name.startswith('/'):
                active_profiles.add(filename, name, name)  # use name as name and attachment
            else:
                active_profiles.add(filename, name, attachment)

    elif profile_data:
        attach_profile_data(extras, profile_data)

        for profile in profile_data:  # TODO: also honor hats
            name = profile_data[profile][profile]['name']
            attachment = profile_data[profile][profile]['attachment']
            filename = profile_data[profile][profile]['filename']

            if not attachment and name.startswith('/'):
                extra_profiles.add(filename, name, name)  # use name as name and attachment
            else:
                extra_profiles.add(filename, name, attachment)

def attach_profile_data(profiles, profile_data):
    # Make deep copy of data to avoid changes to
    # arising due to mutables
    for p in profile_data.keys():
        if profiles.get(p, False):
            for hat in profile_data[p].keys():
                if profiles[p].get(hat, False):
                    raise AppArmorException(_("Conflicting profiles for %s defined in two files:\n- %s\n- %s") %
                            (combine_name(p, hat), profiles[p][hat]['filename'], profile_data[p][hat]['filename']))

        profiles[p] = deepcopy(profile_data[p])


def parse_profile_start(line, file, lineno, profile, hat):
    matches = parse_profile_start_line(line, file)

    if profile:  # we are inside a profile, so we expect a child profile
        if not matches['profile_keyword']:
            raise AppArmorException(_('%(profile)s profile in %(file)s contains syntax errors in line %(line)s: missing "profile" keyword.') % {
                    'profile': profile, 'file': file, 'line': lineno + 1 })
        if profile != hat:
            # nesting limit reached - a child profile can't contain another child profile
            raise AppArmorException(_('%(profile)s profile in %(file)s contains syntax errors in line %(line)s: a child profile inside another child profile is not allowed.') % {
                    'profile': profile, 'file': file, 'line': lineno + 1 })

        hat = matches['profile']
        in_contained_hat = True
        pps_set_profile = True
        pps_set_hat_external = False

    else:  # stand-alone profile
        profile = matches['profile']
        if len(profile.split('//')) > 2:
            raise AppArmorException("Nested child profiles ('%(profile)s', found in %(file)s) are not supported by the AppArmor tools yet." % {'profile': profile, 'file': file})
        elif len(profile.split('//')) == 2:
            profile, hat = profile.split('//')
            pps_set_hat_external = True
        else:
            hat = profile
            pps_set_hat_external = False

        in_contained_hat = False
        pps_set_profile = False

    attachment = matches['attachment']
    flags = matches['flags']

    return (profile, hat, attachment, flags, in_contained_hat, pps_set_profile, pps_set_hat_external)

def parse_profile_data(data, file, do_include):
    profile_data = hasher()
    profile = None
    hat = None
    in_contained_hat = None
    repo_data = None
    parsed_profiles = []
    initial_comment = ''
    lastline = None

    if do_include:
        profile = file
        hat = file
        profile_data[profile][hat] = ProfileStorage(profile, hat, 'parse_profile_data() do_include')
        profile_data[profile][hat]['filename'] = file

    for lineno, line in enumerate(data):
        line = line.strip()
        if not line:
            continue
        # we're dealing with a multiline statement
        if lastline:
            line = '%s %s' % (lastline, line)
            lastline = None
        # Starting line of a profile
        if RE_PROFILE_START.search(line):
            (profile, hat, attachment, flags, in_contained_hat, pps_set_profile, pps_set_hat_external) = parse_profile_start(line, file, lineno, profile, hat)

            if profile_data[profile].get(hat, False):
                raise AppArmorException('Profile %(profile)s defined twice in %(file)s, last found in line %(line)s' %
                    { 'file': file, 'line': lineno + 1, 'profile': combine_name(profile, hat) })

            profile_data[profile][hat] = ProfileStorage(profile, hat, 'parse_profile_data() profile_start')

            if attachment:
                profile_data[profile][hat]['attachment'] = attachment
            if pps_set_profile:
                profile_data[profile][hat]['profile'] = True
            if pps_set_hat_external:
                profile_data[profile][hat]['external'] = True

            # save profile name and filename
            profile_data[profile][hat]['name'] = profile
            profile_data[profile][hat]['filename'] = file
            filelist[file]['profiles'][profile][hat] = True

            profile_data[profile][hat]['flags'] = flags

            # Save the initial comment
            if initial_comment:
                profile_data[profile][hat]['initial_comment'] = initial_comment

            initial_comment = ''

            if repo_data:
                profile_data[profile][profile]['repo']['url'] = repo_data['url']
                profile_data[profile][profile]['repo']['user'] = repo_data['user']

        elif RE_PROFILE_END.search(line):
            # If profile ends and we're not in one
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected End of Profile reached in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            if in_contained_hat:
                hat = profile
                in_contained_hat = False
            else:
                parsed_profiles.append(profile)
                profile = None

            initial_comment = ''

        elif CapabilityRule.match(line):
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected capability entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            profile_data[profile][hat]['capability'].add(CapabilityRule.parse(line))

        elif RE_PROFILE_LINK.search(line):
            matches = RE_PROFILE_LINK.search(line).groups()

            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected link entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            audit = False
            if matches[0]:
                audit = True

            allow = 'allow'
            if matches[1] and matches[1].strip() == 'deny':
                allow = 'deny'

            subset = matches[3]
            link = strip_quotes(matches[6])
            value = strip_quotes(matches[7])
            profile_data[profile][hat][allow]['link'][link]['to'] = value
            profile_data[profile][hat][allow]['link'][link]['mode'] = profile_data[profile][hat][allow]['link'][link].get('mode', set()) | apparmor.aamode.AA_MAY_LINK

            if subset:
                profile_data[profile][hat][allow]['link'][link]['mode'] |= apparmor.aamode.AA_LINK_SUBSET

            if audit:
                profile_data[profile][hat][allow]['link'][link]['audit'] = profile_data[profile][hat][allow]['link'][link].get('audit', set()) | apparmor.aamode.AA_LINK_SUBSET
            else:
                profile_data[profile][hat][allow]['link'][link]['audit'] = set()

        elif ChangeProfileRule.match(line):
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected change profile entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            profile_data[profile][hat]['change_profile'].add(ChangeProfileRule.parse(line))

        elif RE_PROFILE_ALIAS.search(line):
            matches = RE_PROFILE_ALIAS.search(line).groups()

            from_name = strip_quotes(matches[0])
            to_name = strip_quotes(matches[1])

            if profile:
                profile_data[profile][hat]['alias'][from_name] = to_name
            else:
                if not filelist.get(file, False):
                    filelist[file] = hasher()
                filelist[file]['alias'][from_name] = to_name

        elif RlimitRule.match(line):
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected rlimit entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            profile_data[profile][hat]['rlimit'].add(RlimitRule.parse(line))

        elif RE_PROFILE_BOOLEAN.search(line):
            matches = RE_PROFILE_BOOLEAN.search(line).groups()

            if profile and not do_include:
                raise AppArmorException(_('Syntax Error: Unexpected boolean definition found inside profile in file: %(file)s line: %(line)s') % {
                        'file': file, 'line': lineno + 1 })

            bool_var = matches[0]
            value = matches[1]

            profile_data[profile][hat]['lvar'][bool_var] = value

        elif RE_PROFILE_VARIABLE.search(line):
            # variable additions += and =
            matches = RE_PROFILE_VARIABLE.search(line).groups()

            list_var = strip_quotes(matches[0])
            var_operation = matches[1]
            value = matches[2]

            if profile:
                if not profile_data[profile][hat].get('lvar', False):
                    profile_data[profile][hat]['lvar'][list_var] = []
                store_list_var(profile_data[profile]['lvar'], list_var, value, var_operation, file)
            else:
                if not filelist[file].get('lvar', False):
                    filelist[file]['lvar'][list_var] = []
                store_list_var(filelist[file]['lvar'], list_var, value, var_operation, file)

        elif RE_PROFILE_CONDITIONAL.search(line):
            # Conditional Boolean
            pass

        elif RE_PROFILE_CONDITIONAL_VARIABLE.search(line):
            # Conditional Variable defines
            pass

        elif RE_PROFILE_CONDITIONAL_BOOLEAN.search(line):
            # Conditional Boolean defined
            pass

        elif RE_ABI.search(line):
            if profile:
                profile_data[profile][hat]['abi'].append(line)
            else:
                if not filelist.get(file):
                    filelist[file] = hasher()
                if not filelist[file].get('abi'):
                    filelist[file]['abi'] = []
                filelist[file]['abi'].append(line)

        elif re_match_include(line):
            # Include files
            include_name = re_match_include(line)
            if include_name.startswith('local/'):
                profile_data[profile][hat]['localinclude'][include_name] = True

            if profile:
                profile_data[profile][hat]['include'][include_name] = True
            else:
                if not filelist.get(file):
                    filelist[file] = hasher()
                filelist[file]['include'][include_name] = True
            # If include is a directory
            if os.path.isdir(get_include_path(include_name)):
                for file_name in include_dir_filelist(profile_dir, include_name):
                    if not include.get(file_name, False):
                        load_include(file_name)
            else:
                if not include.get(include_name, False):
                    load_include(include_name)

        elif NetworkRule.match(line):
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected network entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            profile_data[profile][hat]['network'].add(NetworkRule.parse(line))

        elif DbusRule.match(line):
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected dbus entry found in file: %(file)s line: %(line)s') % {'file': file, 'line': lineno + 1 })

            profile_data[profile][hat]['dbus'].add(DbusRule.parse(line))

        elif RE_PROFILE_MOUNT.search(line):
            matches = RE_PROFILE_MOUNT.search(line).groups()

            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected mount entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            audit = False
            if matches[0]:
                audit = True
            allow = 'allow'
            if matches[1] and matches[1].strip() == 'deny':
                allow = 'deny'
            mount = matches[2]

            mount_rule = parse_mount_rule(mount)
            mount_rule.audit = audit
            mount_rule.deny = (allow == 'deny')

            mount_rules = profile_data[profile][hat][allow].get('mount', list())
            mount_rules.append(mount_rule)
            profile_data[profile][hat][allow]['mount'] = mount_rules

        elif SignalRule.match(line):
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected signal entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            profile_data[profile][hat]['signal'].add(SignalRule.parse(line))

        elif PtraceRule.match(line):
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected ptrace entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            profile_data[profile][hat]['ptrace'].add(PtraceRule.parse(line))

        elif RE_PROFILE_PIVOT_ROOT.search(line):
            matches = RE_PROFILE_PIVOT_ROOT.search(line).groups()

            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected pivot_root entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            audit = False
            if matches[0]:
                audit = True
            allow = 'allow'
            if matches[1] and matches[1].strip() == 'deny':
                allow = 'deny'
            pivot_root = matches[2].strip()

            pivot_root_rule = parse_pivot_root_rule(pivot_root)
            pivot_root_rule.audit = audit
            pivot_root_rule.deny = (allow == 'deny')

            pivot_root_rules = profile_data[profile][hat][allow].get('pivot_root', list())
            pivot_root_rules.append(pivot_root_rule)
            profile_data[profile][hat][allow]['pivot_root'] = pivot_root_rules

        elif RE_PROFILE_UNIX.search(line):
            matches = RE_PROFILE_UNIX.search(line).groups()

            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected unix entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            audit = False
            if matches[0]:
                audit = True
            allow = 'allow'
            if matches[1] and matches[1].strip() == 'deny':
                allow = 'deny'
            unix = matches[2].strip()

            unix_rule = parse_unix_rule(unix)
            unix_rule.audit = audit
            unix_rule.deny = (allow == 'deny')

            unix_rules = profile_data[profile][hat][allow].get('unix', list())
            unix_rules.append(unix_rule)
            profile_data[profile][hat][allow]['unix'] = unix_rules

        elif RE_PROFILE_CHANGE_HAT.search(line):
            matches = RE_PROFILE_CHANGE_HAT.search(line).groups()

            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected change hat declaration found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            aaui.UI_Important(_('Ignoring no longer supported change hat declaration "^%(hat)s," found in file: %(file)s line: %(line)s') % {
                    'hat': matches[0], 'file': file, 'line': lineno + 1 })

        elif RE_PROFILE_HAT_DEF.search(line):
            # An embedded hat syntax definition starts
            matches = RE_PROFILE_HAT_DEF.search(line)
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected hat definition found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            in_contained_hat = True
            hat = matches.group('hat')
            hat = strip_quotes(hat)

            # if hat is already known, the filelist check some lines below will error out.
            # nevertheless, just to be sure, don't overwrite existing profile_data.
            if not profile_data[profile].get(hat, False):
                profile_data[profile][hat] = ProfileStorage(profile, hat, 'parse_profile_data() hat_def')
                profile_data[profile][hat]['filename'] = file

            flags = matches.group('flags')

            profile_data[profile][hat]['flags'] = flags

            if initial_comment:
                profile_data[profile][hat]['initial_comment'] = initial_comment
            initial_comment = ''
            if filelist[file]['profiles'][profile].get(hat, False) and not do_include:
                raise AppArmorException(_('Error: Multiple definitions for hat %(hat)s in profile %(profile)s.') % { 'hat': hat, 'profile': profile })
            filelist[file]['profiles'][profile][hat] = True

        elif line[0] == '#':
            # Handle initial comments
            if not profile:
                if line.startswith('# Last Modified:'):
                    continue
                elif line.startswith('# REPOSITORY:'): # TODO: allow any number of spaces/tabs
                    parts = line.split()
                    if len(parts) == 3 and parts[2] == 'NEVERSUBMIT':
                        repo_data = {'neversubmit': True}
                    elif len(parts) == 5:
                        repo_data = {'url': parts[2],
                                     'user': parts[3],
                                     'id': parts[4]}
                    else:
                        aaui.UI_Important(_('Warning: invalid "REPOSITORY:" line in %s, ignoring.') % file)
                        initial_comment = initial_comment + line + '\n'
                else:
                    initial_comment = initial_comment + line + '\n'

        elif FileRule.match(line):
            # leading permissions could look like a keyword, therefore handle file rules after everything else
            if not profile:
                raise AppArmorException(_('Syntax Error: Unexpected path entry found in file: %(file)s line: %(line)s') % { 'file': file, 'line': lineno + 1 })

            profile_data[profile][hat]['file'].add(FileRule.parse(line))

        elif not RE_RULE_HAS_COMMA.search(line):
            # Bah, line continues on to the next line
            if RE_HAS_COMMENT_SPLIT.search(line):
                # filter trailing comments
                lastline = RE_HAS_COMMENT_SPLIT.search(line).group('not_comment')
            else:
                lastline = line
        else:
            raise AppArmorException(_('Syntax Error: Unknown line found in file %(file)s line %(lineno)s:\n    %(line)s') % { 'file': file, 'lineno': lineno + 1, 'line': line })

    if lastline:
        # lastline gets merged into line (and reset to None) when reading the next line.
        # If it isn't empty, this means there's something unparseable at the end of the profile
        raise AppArmorException(_('Syntax Error: Unknown line found in file %(file)s line %(lineno)s:\n    %(line)s') % { 'file': file, 'lineno': lineno + 1, 'line': lastline })

    # Below is not required I'd say
    if not do_include:
        for hatglob in cfg['required_hats'].keys():
            for parsed_prof in sorted(parsed_profiles):
                if re.search(hatglob, parsed_prof):
                    for hat in cfg['required_hats'][hatglob].split():
                        if not profile_data[parsed_prof].get(hat, False):
                            profile_data[parsed_prof][hat] = ProfileStorage(parsed_prof, hat, 'parse_profile_data() required_hats')

    # End of file reached but we're stuck in a profile
    if profile and not do_include:
        raise AppArmorException(_("Syntax Error: Missing '}' or ','. Reached end of file %(file)s while inside profile %(profile)s") % { 'file': file, 'profile': profile })

    return profile_data

def parse_mount_rule(line):
    # XXX Do real parsing here
    return aarules.Raw_Mount_Rule(line)

def parse_pivot_root_rule(line):
    # XXX Do real parsing here
    return aarules.Raw_Pivot_Root_Rule(line)

def parse_unix_rule(line):
    # XXX Do real parsing here
    return aarules.Raw_Unix_Rule(line)

def separate_vars(vs):
    """Returns a list of all the values for a variable"""
    data = set()
    vs = vs.strip()

    RE_VARS = re.compile('^(("[^"]*")|([^"\s]+))\s*(.*)$')
    while RE_VARS.search(vs):
        matches = RE_VARS.search(vs).groups()
        data.add(strip_quotes(matches[0]))
        vs = matches[3].strip()

    if vs:
        raise AppArmorException('Variable assignments contains invalid parts (unbalanced quotes?): %s' % vs)

    return data

def is_active_profile(pname):
    if aa.get(pname, False):
        return True
    else:
        return False

def store_list_var(var, list_var, value, var_operation, filename):
    """Store(add new variable or add values to variable) the variables encountered in the given list_var
       - the 'var' parameter will be modified
       - 'list_var' is the variable name, for example '@{foo}'
        """
    vlist = separate_vars(value)
    if var_operation == '=':
        if not var.get(list_var, False):
            var[list_var] = set(vlist)
        else:
            raise AppArmorException(_('Redefining existing variable %(variable)s: %(value)s in %(file)s') % { 'variable': list_var, 'value': value, 'file': filename })
    elif var_operation == '+=':
        if var.get(list_var, False):
            var[list_var] |= vlist
        else:
            raise AppArmorException(_('Values added to a non-existing variable %(variable)s: %(value)s in %(file)s') % { 'variable': list_var, 'value': value, 'file': filename })
    else:
        raise AppArmorException(_('Unknown variable operation %(operation)s for variable %(variable)s in %(file)s') % { 'operation': var_operation, 'variable': list_var, 'file': filename })

def write_header(prof_data, depth, name, embedded_hat, write_flags):
    pre = ' ' * int(depth * 2)
    data = []
    unquoted_name = name
    name = quote_if_needed(name)

    attachment = ''
    if prof_data['attachment']:
        attachment = ' %s' % quote_if_needed(prof_data['attachment'])

    comment = ''
    if prof_data['header_comment']:
        comment = ' %s' % prof_data['header_comment']

    if (not embedded_hat and re.search('^[^/]', unquoted_name)) or (embedded_hat and re.search('^[^^]', unquoted_name)) or prof_data['attachment'] or prof_data['profile_keyword']:
        name = 'profile %s%s' % (name, attachment)

    flags = ''
    if write_flags and prof_data['flags']:
        flags = ' flags=(%s)' % prof_data['flags']

    data.append('%s%s%s {%s' % (pre, name, flags, comment))

    return data

def set_allow_str(allow):
    if allow == 'deny':
        return 'deny '
    elif allow == 'allow':
        return ''
    elif allow == '':
        return ''
    else:
        raise AppArmorException(_("Invalid allow string: %(allow)s"))

def set_ref_allow(prof_data, allow):
    if allow:
        return prof_data[allow], set_allow_str(allow)
    else:
        return prof_data, ''


def write_pair(prof_data, depth, allow, name, prefix, sep, tail, fn):
    pre = '  ' * depth
    data = []
    ref, allow = set_ref_allow(prof_data, allow)

    if ref.get(name, False):
        for key in sorted(ref[name].keys()):
            value = fn(ref[name][key])  # eval('%s(%s)' % (fn, ref[name][key]))
            data.append('%s%s%s%s%s%s%s' % (pre, allow, prefix, key, sep, value, tail))
        if ref[name].keys():
            data.append('')

    return data

def write_includes(prof_data, depth):
    pre = '  ' * depth
    data = []

    for key in sorted(prof_data['include'].keys()):
        if key.startswith('/'):
            qkey = '"%s"' % key
        else:
            qkey = '<%s>' % quote_if_needed(key)

        data.append('%s#include %s' % (pre, qkey))

    if data:
        data.append('')

    return data

def write_change_profile(prof_data, depth):
    data = []
    if prof_data.get('change_profile', False):
        data = prof_data['change_profile'].get_clean(depth)
    return data

def write_alias(prof_data, depth):
    return write_pair(prof_data, depth, '', 'alias', 'alias ', ' -> ', ',', quote_if_needed)

def write_rlimits(prof_data, depth):
    data = []
    if prof_data.get('rlimit', False):
        data = prof_data['rlimit'].get_clean(depth)
    return data

def var_transform(ref):
    data = []
    for value in ref:
        if not value:
            value = '""'
        data.append(quote_if_needed(value))
    return ' '.join(data)

def write_list_vars(prof_data, depth):
    return write_pair(prof_data, depth, '', 'lvar', '', ' = ', '', var_transform)

def write_capabilities(prof_data, depth):
    data = []
    if prof_data.get('capability', False):
        data = prof_data['capability'].get_clean(depth)
    return data

def write_netdomain(prof_data, depth):
    data = []
    if prof_data.get('network', False):
        data = prof_data['network'].get_clean(depth)
    return data

def write_dbus(prof_data, depth):
    data = []
    if prof_data.get('dbus', False):
        data = prof_data['dbus'].get_clean(depth)
    return data

def write_mount_rules(prof_data, depth, allow):
    pre = '  ' * depth
    data = []

    # no mount rules, so return
    if not prof_data[allow].get('mount', False):
        return data

    for mount_rule in prof_data[allow]['mount']:
        data.append('%s%s' % (pre, mount_rule.serialize()))
    data.append('')
    return data

def write_mount(prof_data, depth):
    data = write_mount_rules(prof_data, depth, 'deny')
    data += write_mount_rules(prof_data, depth, 'allow')
    return data

def write_signal(prof_data, depth):
    data = []
    if prof_data.get('signal', False):
        data = prof_data['signal'].get_clean(depth)
    return data

def write_ptrace(prof_data, depth):
    data = []
    if prof_data.get('ptrace', False):
        data = prof_data['ptrace'].get_clean(depth)
    return data

def write_pivot_root_rules(prof_data, depth, allow):
    pre = '  ' * depth
    data = []

    # no pivot_root rules, so return
    if not prof_data[allow].get('pivot_root', False):
        return data

    for pivot_root_rule in prof_data[allow]['pivot_root']:
        data.append('%s%s' % (pre, pivot_root_rule.serialize()))
    data.append('')
    return data

def write_pivot_root(prof_data, depth):
    data = write_pivot_root_rules(prof_data, depth, 'deny')
    data += write_pivot_root_rules(prof_data, depth, 'allow')
    return data

def write_unix_rules(prof_data, depth, allow):
    pre = '  ' * depth
    data = []

    # no unix rules, so return
    if not prof_data[allow].get('unix', False):
        return data

    for unix_rule in prof_data[allow]['unix']:
        data.append('%s%s' % (pre, unix_rule.serialize()))
    data.append('')
    return data

def write_unix(prof_data, depth):
    data = write_unix_rules(prof_data, depth, 'deny')
    data += write_unix_rules(prof_data, depth, 'allow')
    return data

def write_link_rules(prof_data, depth, allow):
    pre = '  ' * depth
    data = []
    allowstr = set_allow_str(allow)

    if prof_data[allow].get('link', False):
        for path in sorted(prof_data[allow]['link'].keys()):
            to_name = prof_data[allow]['link'][path]['to']
            subset = ''
            if prof_data[allow]['link'][path]['mode'] & apparmor.aamode.AA_LINK_SUBSET:
                subset = 'subset '
            audit = ''
            if prof_data[allow]['link'][path].get('audit', False):
                audit = 'audit '
            path = quote_if_needed(path)
            to_name = quote_if_needed(to_name)
            data.append('%s%s%slink %s%s -> %s,' % (pre, audit, allowstr, subset, path, to_name))
        data.append('')

    return data

def write_links(prof_data, depth):
    data = write_link_rules(prof_data, depth, 'deny')
    data += write_link_rules(prof_data, depth, 'allow')

    return data

def write_file(prof_data, depth):
    data = []
    if prof_data.get('file', False):
        data = prof_data['file'].get_clean(depth)
    return data

def write_rules(prof_data, depth):
    data = write_abi(prof_data, depth)
    data += write_alias(prof_data, depth)
    data += write_list_vars(prof_data, depth)
    data += write_includes(prof_data, depth)
    data += write_rlimits(prof_data, depth)
    data += write_capabilities(prof_data, depth)
    data += write_netdomain(prof_data, depth)
    data += write_dbus(prof_data, depth)
    data += write_mount(prof_data, depth)
    data += write_signal(prof_data, depth)
    data += write_ptrace(prof_data, depth)
    data += write_pivot_root(prof_data, depth)
    data += write_unix(prof_data, depth)
    data += write_links(prof_data, depth)
    data += write_file(prof_data, depth)
    data += write_change_profile(prof_data, depth)

    return data

def write_piece(profile_data, depth, name, nhat, write_flags):
    pre = '  ' * depth
    data = []
    wname = None
    inhat = False
    if name == nhat:
        wname = name
    else:
        wname = name + '//' + nhat
        name = nhat
        inhat = True
    data += write_header(profile_data[name], depth, wname, False, write_flags)
    data += write_rules(profile_data[name], depth + 1)

    pre2 = '  ' * (depth + 1)

    if not inhat:
        # Embedded hats
        for hat in list(filter(lambda x: x != name, sorted(profile_data.keys()))):
            if not profile_data[hat]['external']:
                data.append('')
                if profile_data[hat]['profile']:
                    data += list(map(str, write_header(profile_data[hat], depth + 1, hat, True, write_flags)))
                else:
                    data += list(map(str, write_header(profile_data[hat], depth + 1, '^' + hat, True, write_flags)))

                data += list(map(str, write_rules(profile_data[hat], depth + 2)))

                data.append('%s}' % pre2)

        data.append('%s}' % pre)

        # External hats
        for hat in list(filter(lambda x: x != name, sorted(profile_data.keys()))):
            if name == nhat and profile_data[hat].get('external', False):
                data.append('')
                data += list(map(lambda x: '  %s' % x, write_piece(profile_data, depth - 1, name, nhat, write_flags)))
                data.append('  }')

    return data

def serialize_profile(profile_data, name, options):
    string = ''
    include_metadata = False
    include_flags = True
    data = []

    if options:  # and type(options) == dict:
        if options.get('METADATA', False):
            include_metadata = True
        if options.get('NO_FLAGS', False):
            include_flags = False

    if include_metadata:
        string = '# Last Modified: %s\n' % time.asctime()

        if (profile_data[name].get('repo', False) and
                profile_data[name]['repo']['url'] and
                profile_data[name]['repo']['user'] and
                profile_data[name]['repo']['id']):
            repo = profile_data[name]['repo']
            string += '# REPOSITORY: %s %s %s\n' % (repo['url'], repo['user'], repo['id'])
        elif profile_data[name]['repo'].get('neversubmit'):
            string += '# REPOSITORY: NEVERSUBMIT\n'

#     if profile_data[name].get('initial_comment', False):
#         comment = profile_data[name]['initial_comment']
#         comment.replace('\\n', '\n')
#         string += comment + '\n'

    if options and options.get('is_attachment'):
        prof_filename = get_profile_filename_from_attachment(name, True)
    else:
        prof_filename = get_profile_filename_from_profile_name(name, True)

    if filelist.get(prof_filename, False):
        data += write_abi(filelist[prof_filename], 0)
        data += write_alias(filelist[prof_filename], 0)
        data += write_list_vars(filelist[prof_filename], 0)
        data += write_includes(filelist[prof_filename], 0)

    #Here should be all the profiles from the files added write after global/common stuff
    for prof in sorted(filelist[prof_filename]['profiles'].keys()):
        if prof != name:
            if original_aa[prof][prof].get('initial_comment', False):
                comment = original_aa[prof][prof]['initial_comment']
                comment.replace('\\n', '\n')
                data += [comment + '\n']
            data += write_piece(original_aa[prof], 0, prof, prof, include_flags)
        else:
            if profile_data[name].get('initial_comment', False):
                comment = profile_data[name]['initial_comment']
                comment.replace('\\n', '\n')
                data += [comment + '\n']
            data += write_piece(profile_data, 0, name, name, include_flags)

    string += '\n'.join(data)

    return string + '\n'

def serialize_parse_profile_start(line, file, lineno, profile, hat, prof_data_profile, prof_data_external, correct):
    (profile, hat, attachment, flags, in_contained_hat, pps_set_profile, pps_set_hat_external) = parse_profile_start(line, file, lineno, profile, hat)

    if hat and profile != hat and '%s//%s'%(profile, hat) in line and not prof_data_external:
        correct = False

    return (profile, hat, attachment, flags, in_contained_hat, correct)

def write_profile_ui_feedback(profile, is_attachment=False):
    aaui.UI_Info(_('Writing updated profile for %s.') % profile)
    write_profile(profile, is_attachment)

def write_profile(profile, is_attachment=False):
    prof_filename = None
    if aa[profile][profile].get('filename', False):
        prof_filename = aa[profile][profile]['filename']
    elif is_attachment:
        prof_filename = get_profile_filename_from_attachment(profile, True)
    else:
        prof_filename = get_profile_filename_from_profile_name(profile, True)

    newprof = tempfile.NamedTemporaryFile('w', suffix='~', delete=False, dir=profile_dir)
    if os.path.exists(prof_filename):
        shutil.copymode(prof_filename, newprof.name)
    else:
        #permission_600 = stat.S_IRUSR | stat.S_IWUSR    # Owner read and write
        #os.chmod(newprof.name, permission_600)
        pass

    serialize_options = {'METADATA': True, 'is_attachment': is_attachment}

    profile_string = serialize_profile(aa[profile], profile, serialize_options)
    newprof.write(profile_string)
    newprof.close()

    os.rename(newprof.name, prof_filename)

    if profile in changed:
        changed.pop(profile)
    else:
        debug_logger.info("Unchanged profile written: %s (not listed in 'changed' list)" % profile)

    original_aa[profile] = deepcopy(aa[profile])

def is_known_rule(profile, rule_type, rule_obj):
    # XXX get rid of get() checks after we have a proper function to initialize a profile
    if profile.get(rule_type, False):
        if profile[rule_type].is_covered(rule_obj, False):
            return True

    includelist = list(profile['include'].keys())
    checked = []

    while includelist:
        incname = includelist.pop(0)
        checked.append(incname)

        if os.path.isdir(get_include_path(incname)):
            includelist += include_dir_filelist(profile_dir, incname)
        else:
            if include[incname][incname].get(rule_type, False):
                if include[incname][incname][rule_type].is_covered(rule_obj, False):
                    return True

            for childinc in include[incname][incname]['include'].keys():
                if childinc not in checked:
                    includelist += [childinc]

    return False

def get_file_perms(profile, path, audit, deny):
    '''get the current permissions for the given path'''

    perms = profile['file'].get_perms_for_path(path, audit, deny)

    includelist = list(profile['include'].keys())
    checked = []

    while includelist:
        incname = includelist.pop(0)

        if incname in checked:
            continue
        checked.append(incname)

        if os.path.isdir(get_include_path(incname)):
            includelist += include_dir_filelist(profile_dir, incname)
        else:
            incperms = include[incname][incname]['file'].get_perms_for_path(path, audit, deny)

            for allow_or_deny in ['allow', 'deny']:
                for owner_or_all in ['all', 'owner']:
                    for perm in incperms[allow_or_deny][owner_or_all]:
                        perms[allow_or_deny][owner_or_all].add(perm)

                    if 'a' in perms[allow_or_deny][owner_or_all] and 'w' in perms[allow_or_deny][owner_or_all]:
                        perms[allow_or_deny][owner_or_all].remove('a')  # a is a subset of w, so remove it

            for incpath in incperms['paths']:
                perms['paths'].add(incpath)

            for childinc in include[incname][incname]['include'].keys():
                if childinc not in checked:
                    includelist += [childinc]

    return perms

def propose_file_rules(profile_obj, rule_obj):
    '''Propose merged file rules based on the existing profile and the log events
       - permissions get merged
       - matching paths from existing rules, common_glob() and user_globs get proposed
       - IMPORTANT: modifies rule_obj.original_perms and rule_obj.perms'''
    options = []
    original_path = rule_obj.path.regex

    merged_rule_obj = deepcopy(rule_obj)   # make sure not to modify the original rule object (with exceptions, see end of this function)

    existing_perms = get_file_perms(profile_obj, rule_obj.path, False, False)
    for perm in existing_perms['allow']['all']:  # XXX also handle owner-only perms
        merged_rule_obj.perms.add(perm)
        merged_rule_obj.raw_rule = None

    if 'a' in merged_rule_obj.perms and 'w' in merged_rule_obj.perms:
        merged_rule_obj.perms.remove('a')  # a is a subset of w, so remove it

    pathlist = {original_path} | existing_perms['paths'] | set(glob_common(original_path))

    for user_glob in user_globs:
        if user_globs[user_glob].match(original_path):
            pathlist.add(user_glob)

    pathlist = order_globs(pathlist, original_path)

    # paths in existing rules that match the original path
    for path in pathlist:
        merged_rule_obj.store_edit(path)
        merged_rule_obj.raw_rule = None
        options.append(merged_rule_obj.get_clean())

    merged_rule_obj.exec_perms = None

    rule_obj.original_perms = existing_perms
    if rule_obj.perms != merged_rule_obj.perms:
        rule_obj.perms = merged_rule_obj.perms
        rule_obj.raw_rule = None

    return options

def reload_base(bin_path):
    if not check_for_apparmor():
        return None

    prof_filename = get_profile_filename_from_profile_name(bin_path, True)

    # XXX use reload_profile() from tools.py instead (and don't hide output in /dev/null)
    subprocess.call("cat '%s' | %s -I%s -r >/dev/null 2>&1" % (prof_filename, parser, profile_dir), shell=True)

def reload(bin_path):
    bin_path = find_executable(bin_path)
    if not bin_path:
        return None

    return reload_base(bin_path)

def get_include_data(filename):
    data = []
    if not filename.startswith('/'):
        filename = profile_dir + '/' + filename
    if os.path.exists(filename):
        with open_file_read(filename) as f_in:
            data = f_in.readlines()
    else:
        raise AppArmorException(_('File Not Found: %s') % filename)
    return data

def include_dir_filelist(profile_dir, include_name):
    '''returns a list of files in the given profile_dir/include_name directory,
       except skippable files. If include_name is an absolute path, ignore
       profile_dir.
    '''
    files = []
    include_name_abs = get_include_path(include_name)
    for path in os.listdir(include_name_abs):
        path = path.strip()
        if is_skippable_file(path):
            continue
        if os.path.isfile(include_name_abs + '/' + path):
            file_name = include_name + '/' + path
            # strip off profile_dir for non-absolute paths
            if not include_name.startswith('/'):
                file_name = file_name.replace(profile_dir + '/', '')
            files.append(file_name)

    return files

def load_include(incname):
    load_includeslist = [incname]
    while load_includeslist:
        incfile = load_includeslist.pop(0)
        incfile_abs = get_include_path(incfile)
        if include.get(incfile, {}).get(incfile, False):
            pass  # already read, do nothing
        elif os.path.isfile(incfile_abs):
            data = get_include_data(incfile_abs)
            incdata = parse_profile_data(data, incfile, True)
            attach_profile_data(include, incdata)
        #If the include is a directory means include all subfiles
        elif os.path.isdir(incfile_abs):
            load_includeslist += include_dir_filelist(profile_dir, incfile)
        else:
            raise AppArmorException("Include file %s not found" % (incfile_abs))

    return 0

def check_qualifiers(program):
    if cfg['qualifiers'].get(program, False):
        if cfg['qualifiers'][program] != 'p':
            fatal_error(_("%s is currently marked as a program that should not have its own\nprofile.  Usually, programs are marked this way if creating a profile for \nthem is likely to break the rest of the system.  If you know what you\'re\ndoing and are certain you want to create a profile for this program, edit\nthe corresponding entry in the [qualifiers] section in /etc/apparmor/logprof.conf.") % program)
    return False

def get_subdirectories(current_dir):
    """Returns a list of all directories directly inside given directory"""
    if sys.version_info < (3, 0):
        return os.walk(current_dir).next()[1]
    else:
        return os.walk(current_dir).__next__()[1]

def loadincludes():
    incdirs = get_subdirectories(profile_dir)
    for idir in incdirs:
        if is_skippable_dir(idir):
            continue
        for dirpath, dirname, files in os.walk(profile_dir + '/' + idir):
            if is_skippable_dir(dirpath):
                continue
            for fi in files:
                if is_skippable_file(fi):
                    continue
                else:
                    fi = dirpath + '/' + fi
                    fi = fi.replace(profile_dir + '/', '', 1)
                    load_include(fi)

def glob_common(path):
    globs = []

    if re.search('[\d\.]+\.so$', path) or re.search('\.so\.[\d\.]+$', path):
        libpath = path
        libpath = re.sub('[\d\.]+\.so$', '*.so', libpath)
        libpath = re.sub('\.so\.[\d\.]+$', '.so.*', libpath)
        if libpath != path:
            globs.append(libpath)

    for glob in cfg['globs']:
        if re.search(glob, path):
            globbedpath = path
            globbedpath = re.sub(glob, cfg['globs'][glob], path)
            if globbedpath != path:
                globs.append(globbedpath)

    return sorted(set(globs))

def combine_name(name1, name2):
    if name1 == name2:
        return name1
    else:
        return '%s^%s' % (name1, name2)

def logger_path():
    logger = conf.find_first_file(cfg['settings']['logger']) or '/bin/logger'
    if not os.path.isfile(logger) or not os.access(logger, os.EX_OK):
        raise AppArmorException("Can't find logger!\nPlease make sure %s exists, or update the 'logger' path in logprof.conf." % logger)
    return logger

######Initialisations######

def init_aa(confdir="/etc/apparmor"):
    global CONFDIR
    global conf
    global cfg
    global profile_dir
    global extra_profile_dir
    global parser

    if CONFDIR:
        return  # config already initialized (and possibly changed afterwards), so don't overwrite the config variables

    CONFDIR = confdir
    conf = apparmor.config.Config('ini', CONFDIR)
    cfg = conf.read_config('logprof.conf')

    # prevent various failures if logprof.conf doesn't exist
    if not cfg.sections():
        cfg.add_section('settings')
        cfg.add_section('required_hats')

    if cfg['settings'].get('default_owner_prompt', False):
        cfg['settings']['default_owner_prompt'] = ''

    profile_dir = conf.find_first_dir(cfg['settings'].get('profiledir')) or '/etc/apparmor.d'
    if not os.path.isdir(profile_dir):
        raise AppArmorException('Can\'t find AppArmor profiles in %s' % (profile_dir))

    extra_profile_dir = conf.find_first_dir(cfg['settings'].get('inactive_profiledir')) or '/usr/share/apparmor/extra-profiles/'

    parser = conf.find_first_file(cfg['settings'].get('parser')) or '/sbin/apparmor_parser'
    if not os.path.isfile(parser) or not os.access(parser, os.EX_OK):
        raise AppArmorException('Can\'t find apparmor_parser at %s' % (parser))
