# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
#    Copyright (C) 2015-2018 Christian Boltz <apparmor@cboltz.de>
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
import ctypes
import os
import re
import sys
import time
import LibAppArmor
from apparmor.common import AppArmorException, AppArmorBug, open_file_read, DebugLogger

from apparmor.aamode import validate_log_mode, log_str_to_mode, hide_log_mode, AA_MAY_EXEC

# setup module translations
from apparmor.translations import init_translation
_ = init_translation()

class ReadLog:
    RE_audit_time_id = '(msg=)?audit\([\d\.\:]+\):\s+'  # 'audit(1282626827.320:411): '
    RE_kernel_time = '\[[\d\.\s]+\]'    # '[ 1612.746129]'
    RE_type_num = '1[45][0-9][0-9]'     # 1400..1599
    RE_aa_or_op = '(apparmor=|operation=)'

    RE_log_parts = [
        'kernel:\s+(' + RE_kernel_time + '\s+)?(audit:\s+)?type=' + RE_type_num + '\s+' + RE_audit_time_id + RE_aa_or_op,  # v2_6 syslog
        'kernel:\s+(' + RE_kernel_time + '\s+)?' + RE_audit_time_id + 'type=' + RE_type_num + '\s+' + RE_aa_or_op,
        'type=(AVC|APPARMOR[_A-Z]*|' + RE_type_num + ')\s+' + RE_audit_time_id + '(type=' + RE_type_num + '\s+)?' + RE_aa_or_op,  # v2_6 audit and dmesg
        'type=USER_AVC\s+' + RE_audit_time_id + '.*apparmor=',  # dbus
        'type=UNKNOWN\[' + RE_type_num + '\]\s+' + RE_audit_time_id + RE_aa_or_op,
        'dbus\[[0-9]+\]:\s+apparmor=',  # dbus
    ]

    # used to pre-filter log lines so that we hand over only relevant lines to LibAppArmor parsing
    RE_LOG_ALL = re.compile('(' + '|'.join(RE_log_parts) + ')')

    def __init__(self, pid, filename, active_profiles, profile_dir):
        self.filename = filename
        self.profile_dir = profile_dir
        self.pid = pid
        self.active_profiles = active_profiles
        self.log = []
        self.debug_logger = DebugLogger('ReadLog')
        self.LOG = None
        self.logmark = ''
        self.seenmark = None
        self.next_log_entry = None

    def prefetch_next_log_entry(self):
        if self.next_log_entry:
            sys.stderr.out('A log entry already present: %s' % self.next_log_entry)
        self.next_log_entry = self.LOG.readline()
        while not self.RE_LOG_ALL.search(self.next_log_entry) and not (self.logmark and self.logmark in self.next_log_entry):
            self.next_log_entry = self.LOG.readline()
            if not self.next_log_entry:
                break

    def get_next_log_entry(self):
        # If no next log entry fetch it
        if not self.next_log_entry:
            self.prefetch_next_log_entry()
        log_entry = self.next_log_entry
        self.next_log_entry = None
        return log_entry

    def peek_at_next_log_entry(self):
        # Take a peek at the next log entry
        if not self.next_log_entry:
            self.prefetch_next_log_entry()
        return self.next_log_entry

    def throw_away_next_log_entry(self):
        self.next_log_entry = None

    def parse_log_record(self, record):
        self.debug_logger.debug('parse_log_record: %s' % record)

        record_event = self.parse_event(record)
        return record_event

    def parse_event(self, msg):
        """Parse the event from log into key value pairs"""
        msg = msg.strip()
        self.debug_logger.info('parse_event: %s' % msg)
        #print(repr(msg))
        if sys.version_info < (3, 0):
            # parse_record fails with u'foo' style strings hence typecasting to string
            msg = str(msg)
        event = LibAppArmor.parse_record(msg)
        ev = dict()
        ev['resource'] = event.info
        ev['active_hat'] = event.active_hat
        ev['aamode'] = event.event
        ev['time'] = event.epoch
        ev['operation'] = event.operation
        ev['profile'] = event.profile
        ev['name'] = event.name
        ev['name2'] = event.name2
        ev['attr'] = event.attribute
        ev['parent'] = event.parent
        ev['pid'] = event.pid
        ev['task'] = event.task
        ev['info'] = event.info
        ev['error_code'] = event.error_code
        ev['denied_mask'] = event.denied_mask
        ev['request_mask'] = event.requested_mask
        ev['magic_token'] = event.magic_token
        ev['family'] = event.net_family
        ev['protocol'] = event.net_protocol
        ev['sock_type'] = event.net_sock_type

        if event.ouid != ctypes.c_ulong(-1).value:  # ULONG_MAX
            ev['fsuid'] = event.fsuid
            ev['ouid'] = event.ouid

        if ev['operation'] and ev['operation'] == 'signal':
            ev['signal'] = event.signal
            ev['peer'] = event.peer
        elif ev['operation'] and ev['operation'] == 'ptrace':
            ev['peer'] = event.peer
        elif ev['operation'] and ev['operation'].startswith('dbus_'):
            ev['peer_profile'] = event.peer_profile
            ev['bus'] = event.dbus_bus
            ev['path'] = event.dbus_path
            ev['interface'] = event.dbus_interface
            ev['member'] = event.dbus_member

        LibAppArmor.free_record(event)

        if not ev['time']:
            ev['time'] = int(time.time())
        # Remove None keys
        #for key in ev.keys():
        #    if not ev[key] or not re.search('[\w]+', ev[key]):
        #        ev.pop(key)

        if ev['aamode']:
            # Convert aamode values to their counter-parts
            mode_convertor = {0: 'UNKNOWN',
                              1: 'ERROR',
                              2: 'AUDIT',
                              3: 'PERMITTING',
                              4: 'REJECTING',
                              5: 'HINT',
                              6: 'STATUS'
                              }
            try:
                ev['aamode'] = mode_convertor[ev['aamode']]
            except KeyError:
                ev['aamode'] = None

        # "translate" disconnected paths to errors, which means the event will be ignored.
        # XXX Ideally we should propose to add the attach_disconnected flag to the profile
        if ev['error_code'] == 13 and ev['info'] == 'Failed name lookup - disconnected path':
            ev['aamode'] = 'ERROR'

        if ev['aamode']:
            #debug_logger.debug(ev)
            return ev
        else:
            return None

    def add_to_tree(self, loc_pid, parent, type, event):
        self.debug_logger.info('add_to_tree: pid [%s] type [%s] event [%s]' % (loc_pid, type, event))
        if not self.pid.get(loc_pid, False):
            profile, hat = event[:2]
            if parent and self.pid.get(parent, False):
                if not hat:
                    hat = 'null-complain-profile'
                arrayref = []
                self.pid[parent].append(arrayref)
                self.pid[loc_pid] = arrayref
                for ia in ['fork', loc_pid, profile, hat]:
                    arrayref.append(ia)
#                 self.pid[parent].append(array_ref)
#                 self.pid[loc_pid] = array_ref
            else:
                arrayref = []
                self.log.append(arrayref)
                self.pid[loc_pid] = arrayref
#                 self.log.append(array_ref)
#                 self.pid[loc_pid] = array_ref
        self.pid[loc_pid].append([type, loc_pid] + event)
        #print("\n\npid",self.pid)
        #print("log",self.log)

    def add_event_to_tree(self, e):
        e = self.parse_event_for_tree(e)
        if e is not None:
            (pid, parent, mode, details) = e
            self.add_to_tree(pid, parent, mode, details)

    def parse_event_for_tree(self, e):
        aamode = e.get('aamode', 'UNKNOWN')

        if aamode == 'UNKNOWN':
            raise AppArmorBug('aamode is UNKNOWN - %s' % e['type'])  # should never happen

        if aamode in ['AUDIT', 'STATUS', 'ERROR']:
            return None

        if 'profile_set' in e['operation']:
            return None

        # Skip if AUDIT event was issued due to a change_hat in unconfined mode
        if not e.get('profile', False):
            return None

        # Convert new null profiles to old single level null profile
        if '//null-' in e['profile']:
            e['profile'] = 'null-complain-profile'

        profile = e['profile']
        hat = None

        if '//' in e['profile']:
            profile, hat = e['profile'].split('//')[:2]

        # Filter out change_hat events that aren't from learning
        if e['operation'] == 'change_hat':
            if aamode != 'HINT' and aamode != 'PERMITTING':
                return None
            if e['error_code'] == 1 and e['info'] == 'unconfined can not change_hat':
                return None
            profile = e['name2']
            #hat = None
            if '//' in e['name2']:
                profile, hat = e['name2'].split('//')[:2]

        if not hat:
            hat = profile

        # prog is no longer passed around consistently
        prog = 'HINT'

        if profile != 'null-complain-profile' and not self.profile_exists(profile):
            return None
        if e['operation'] == 'exec':
            # convert rmask and dmask to mode arrays
            e['denied_mask'],  e['name2'] = log_str_to_mode(e['profile'], e['denied_mask'], e['name2'])
            e['request_mask'], e['name2'] = log_str_to_mode(e['profile'], e['request_mask'], e['name2'])

            if e.get('info', False) and e['info'] == 'mandatory profile missing':
                return(e['pid'], e['parent'], 'exec',
                                 [profile, hat, aamode, 'PERMITTING', e['denied_mask'], e['name'], e['name2']])
            elif (e.get('name2', False) and '//null-' in e['name2']) or e.get('name', False):
                return(e['pid'], e['parent'], 'exec',
                                 [profile, hat, prog, aamode, e['denied_mask'], e['name'], ''])
            else:
                self.debug_logger.debug('parse_event_for_tree: dropped exec event in %s' % e['profile'])

        elif self.op_type(e) == 'file':
            # Map c (create) and d (delete) to w (logging is more detailed than the profile language)
            rmask = e['request_mask']
            rmask = rmask.replace('c', 'w')
            rmask = rmask.replace('d', 'w')
            if not validate_log_mode(hide_log_mode(rmask)):
                raise AppArmorException(_('Log contains unknown mode %s') % rmask)

            dmask = e['denied_mask']
            dmask = dmask.replace('c', 'w')
            dmask = dmask.replace('d', 'w')
            if not validate_log_mode(hide_log_mode(dmask)):
                raise AppArmorException(_('Log contains unknown mode %s') % dmask)

            if e.get('ouid') is not None and e['fsuid'] == e['ouid']:
                # mark as "owner" event
                if '::' not in rmask:
                    rmask = '%s::' % rmask
                if '::' not in dmask:
                    dmask = '%s::' % dmask

            # convert rmask and dmask to mode arrays
            e['denied_mask'],  e['name2'] = log_str_to_mode(e['profile'], dmask, e['name2'])
            e['request_mask'], e['name2'] = log_str_to_mode(e['profile'], rmask, e['name2'])

            # check if this is an exec event
            is_domain_change = False
            if e['operation'] == 'inode_permission' and (e['denied_mask'] & AA_MAY_EXEC) and aamode == 'PERMITTING':
                following = self.peek_at_next_log_entry()
                if following:
                    entry = self.parse_log_record(following)
                    if entry and entry.get('info', False) == 'set profile':
                        is_domain_change = True
                        self.throw_away_next_log_entry()

            if is_domain_change:
                return(e['pid'], e['parent'], 'exec',
                                 [profile, hat, prog, aamode, e['denied_mask'], e['name'], e['name2']])
            else:
                return(e['pid'], e['parent'], 'path',
                                 [profile, hat, prog, aamode, e['denied_mask'], e['name'], ''])

        elif e['operation'] == 'capable':
            return(e['pid'], e['parent'], 'capability',
                             [profile, hat, prog, aamode, e['name'], ''])

        elif e['operation'] == 'clone':
            parent, child = e['pid'], e['task']
            if not parent:
                parent = 'null-complain-profile'
            if not hat:
                hat = 'null-complain-profile'
            arrayref = []
            if self.pid.get(parent, False):
                self.pid[parent].append(arrayref)
            else:
                self.log.append(arrayref)
            self.pid[child].append(arrayref)
            for ia in ['fork', child, profile, hat]:
                arrayref.append(ia)
#             if self.pid.get(parent, False):
#                 self.pid[parent] += [arrayref]
#             else:
#                 self.log += [arrayref]
#             self.pid[child] = arrayref

        elif self.op_type(e) == 'net':
            return(e['pid'], e['parent'], 'netdomain',
                             [profile, hat, prog, aamode, e['family'], e['sock_type'], e['protocol']])
        elif e['operation'] == 'change_hat':
            return(e['pid'], e['parent'], 'unknown_hat',
                             [profile, hat, aamode, hat])
        elif e['operation'] == 'ptrace':
            if not e['peer']:
                self.debug_logger.debug('ignored garbage ptrace event with empty peer')
                return None
            if not e['denied_mask']:
                self.debug_logger.debug('ignored garbage ptrace event with empty denied_mask')
                return None

            return(e['pid'], e['parent'], 'ptrace',
                             [profile, hat, prog, aamode, e['denied_mask'], e['peer']])
        elif e['operation'] == 'signal':
            return(e['pid'], e['parent'], 'signal',
                             [profile, hat, prog, aamode, e['denied_mask'], e['signal'], e['peer']])
        elif e['operation'].startswith('dbus_'):
            return(e['pid'], e['parent'], 'dbus',
                             [profile, hat, prog, aamode, e['denied_mask'], e['bus'], e['path'], e['name'], e['interface'], e['member'], e['peer_profile']])
        else:
            self.debug_logger.debug('UNHANDLED: %s' % e)

    def read_log(self, logmark):
        self.logmark = logmark
        seenmark = True
        if self.logmark:
            seenmark = False
        #last = None
        #event_type = None
        try:
            #print(self.filename)
            self.LOG = open_file_read(self.filename)
        except IOError:
            raise AppArmorException('Can not read AppArmor logfile: ' + self.filename)
        #LOG = open_file_read(log_open)
        line = True
        while line:
            line = self.get_next_log_entry()
            if not line:
                break
            line = line.strip()
            self.debug_logger.debug('read_log: %s' % line)
            if self.logmark in line:
                seenmark = True

            self.debug_logger.debug('read_log: seenmark = %s' % seenmark)
            if not seenmark:
                continue

            event = self.parse_log_record(line)
            #print(event)
            if event:
                try:
                    self.add_event_to_tree(event)
                except AppArmorException as e:
                    ex_msg = ('%(msg)s\n\nThis error was caused by the log line:\n%(logline)s' %
                            {'msg': e.value, 'logline': line})
                    # when py3 only: Drop the original AppArmorException by passing None as the parent exception
                    raise AppArmorBug(ex_msg)  # py3-only: from None

        self.LOG.close()
        self.logmark = ''
        return self.log

    # operation types that can be network or file operations
    # (used by op_type() which checks some event details to decide)
    OP_TYPE_FILE_OR_NET = {
        # Note: op_type() also uses some startswith() checks which are not listed here!
       'create',
       'post_create',
       'bind',
       'connect',
       'listen',
       'accept',
       'sendmsg',
       'recvmsg',
       'getsockname',
       'getpeername',
       'getsockopt',
       'setsockopt',
       'socket_create',
       'sock_shutdown',
       'open',
       'truncate',
       'mkdir',
       'mknod',
       'chmod',
       'chown',
       'rename_src',
       'rename_dest',
       'unlink',
       'rmdir',
       'symlink_create',
       'link',
       'sysctl',
       'getattr',
       'setattr',
       'xattr',
    }

    def op_type(self, event):
        """Returns the operation type if known, unkown otherwise"""

        if ( event['operation'].startswith('file_') or event['operation'].startswith('inode_') or event['operation'] in self.OP_TYPE_FILE_OR_NET ):
            # file or network event?
            if event['family'] and event['protocol'] and event['sock_type']:
                # 'unix' events also use keywords like 'connect', but protocol is 0 and should therefore be filtered out
                return 'net'
            elif event['denied_mask']:
                return 'file'
            else:
                raise AppArmorException('unknown file or network event type')

        else:
            return 'unknown'

    def profile_exists(self, program):
        """Returns True if profile exists, False otherwise"""
        # Check cache of profiles
        if self.active_profiles.filename_from_profile_name(program):
            return True
        # Check the disk for profile
        prof_path = self.get_profile_filename(program)
        #print(prof_path)
        if os.path.isfile(prof_path):
            # Add to cache of profile
            raise AppArmorBug('This should never happen, please open a bugreport!')
            # self.active_profiles[program] = prof_path
            # return True
        return False

    def get_profile_filename(self, profile):
        """Returns the full profile name"""
        if profile.startswith('/'):
            # Remove leading /
            profile = profile[1:]
        else:
            profile = "profile_" + profile
        profile = profile.replace('/', '.')
        full_profilename = self.profile_dir + '/' + profile
        return full_profilename
