# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
#    Copyright (C) 2014-2017 Christian Boltz <apparmor@cboltz.de>
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


from apparmor.common import AppArmorBug, hasher, type_is_str

from apparmor.rule.capability       import CapabilityRuleset
from apparmor.rule.change_profile   import ChangeProfileRuleset
from apparmor.rule.dbus             import DbusRuleset
from apparmor.rule.file             import FileRuleset
from apparmor.rule.network          import NetworkRuleset
from apparmor.rule.ptrace           import PtraceRuleset
from apparmor.rule.rlimit           import RlimitRuleset
from apparmor.rule.signal           import SignalRuleset

ruletypes = {
    'capability':       {'ruleset': CapabilityRuleset},
    'change_profile':   {'ruleset': ChangeProfileRuleset},
    'dbus':             {'ruleset': DbusRuleset},
    'file':             {'ruleset': FileRuleset},
    'network':          {'ruleset': NetworkRuleset},
    'ptrace':           {'ruleset': PtraceRuleset},
    'rlimit':           {'ruleset': RlimitRuleset},
    'signal':           {'ruleset': SignalRuleset},
}

class ProfileStorage:
    '''class to store the content (header, rules, comments) of a profilename

       Acts like a dict(), but has some additional checks.
    '''

    def __init__(self, profilename, hat, calledby):
        data = dict()

        # self.data['info'] isn't used anywhere, but can be helpful in debugging.
        data['info'] = {'profile': profilename, 'hat': hat, 'calledby': calledby}

        for rule in ruletypes:
            data[rule] = ruletypes[rule]['ruleset']()

        data['alias']            = dict()
        data['abi']              = []
        data['include']          = dict()
        data['localinclude']     = dict()
        data['lvar']             = dict()
        data['repo']             = dict()

        data['filename']         = ''
        data['name']             = ''
        data['attachment']       = ''
        data['flags']            = ''
        data['external']         = False
        data['header_comment']   = ''  # currently only set by change_profile_flags()
        data['initial_comment']  = ''
        data['profile_keyword']  = False  # currently only set by change_profile_flags()
        data['profile']          = False  # profile or hat?

        data['allow'] = dict()
        data['deny'] = dict()

        data['allow']['link']    = hasher()
        data['deny']['link']     = hasher()

        # mount, pivot_root, unix have a .get() fallback to list() - initialize them nevertheless
        data['allow']['mount']   = list()
        data['deny']['mount']    = list()
        data['allow']['pivot_root'] = list()
        data['deny']['pivot_root']  = list()
        data['allow']['unix']    = list()
        data['deny']['unix']     = list()

        self.data = data

    def __getitem__(self, key):
        if key in self.data:
            return self.data[key]
        else:
            raise AppArmorBug('attempt to read unknown key %s' % key)

    def __setitem__(self, key, value):
        # TODO: Most of the keys (containing *Ruleset, dict(), list() or hasher()) should be read-only.
        #       Their content needs to be changed, but the container shouldn't
        #       Note: serialize_profile_from_old_profile.write_prior_segments() and write_prior_segments() expect the container to be writeable!
        # TODO: check if value has the expected type
        if key in self.data:
            self.data[key] = value
        else:
            raise AppArmorBug('attempt to set unknown key %s' % key)

    def get(self, key, fallback=None):
        if key in self.data:
            return self.data.get(key, fallback)
        else:
            raise AppArmorBug('attempt to read unknown key %s' % key)


def split_flags(flags):
    '''split the flags given as string into a sorted, de-duplicated list'''

    if flags is None:
        flags = ''

    # Flags may be whitespace and/or comma separated
    flags_list = flags.replace(',', ' ').split()
    # sort and remove duplicates
    return sorted(set(flags_list))

def add_or_remove_flag(flags, flag_to_change, set_flag):
    '''add (if set_flag == True) or remove the given flag_to_change to flags'''

    if type_is_str(flags) or flags is None:
        flags = split_flags(flags)

    if set_flag:
        if flag_to_change not in flags:
            flags.append(flag_to_change)
    else:
        if flag_to_change in flags:
            flags.remove(flag_to_change)

    return sorted(flags)

def write_abi(ref, depth):
    pre = '  ' * depth
    data = []

    if ref.get('abi'):
        for line in ref.get('abi'):
            data.append('%s%s' % (pre, line))
        data.append('')

    return data
