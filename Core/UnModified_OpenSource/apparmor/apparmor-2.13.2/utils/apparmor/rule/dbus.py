# ----------------------------------------------------------------------
#    Copyright (C) 2015 Christian Boltz <apparmor@cboltz.de>
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

import re

from apparmor.regex import RE_PROFILE_DBUS, RE_PROFILE_NAME, strip_parenthesis, strip_quotes
from apparmor.common import AppArmorBug, AppArmorException
from apparmor.rule import BaseRule, BaseRuleset, check_and_split_list, logprof_value_or_all, parse_modifiers, quote_if_needed

# setup module translations
from apparmor.translations import init_translation
_ = init_translation()


message_keywords = ['send', 'receive', 'r', 'read', 'w', 'write', 'rw']
access_keywords  = [ 'bind', 'eavesdrop' ] + message_keywords

# XXX joint_access_keyword and RE_ACCESS_KEYWORDS exactly as in SignalRule - move to function?
joint_access_keyword = '(' + '(\s|,)*' + '(' + '|'.join(access_keywords) + ')(\s|,)*' + ')'
RE_ACCESS_KEYWORDS = ( joint_access_keyword +  # one of the access_keyword or
                       '|' +                                           # or
                       '\(' + '(\s|,)*' + joint_access_keyword + '?' + '(' + '(\s|,)+' + joint_access_keyword + ')*' + '\)'  # one or more access_keyword in (...)
                     )


RE_FLAG         = '(?P<%s>(\S+|"[^"]+"|\(\s*\S+\s*\)|\(\s*"[^"]+"\)\s*))'    # string without spaces, or quoted string, optionally wrapped in (...). %s is the match group name
# plaintext version:      | * | "* "  | (    *    ) | (  " *   " )    |

# XXX this regex will allow repeated parameters, last one wins
# XXX (the parser will reject such rules)
RE_DBUS_DETAILS  = re.compile(
    '^' +
    '(\s+(?P<access>'       + RE_ACCESS_KEYWORDS + '))?' +  # optional access keyword(s)
    '((\s+(bus\s*=\s*'      + RE_FLAG % 'bus'       + '))?|' +  # optional bus= system | session | AARE, (...) optional
    '(\s+(path\s*=\s*'      + RE_FLAG % 'path'      + '))?|' +  # optional path=AARE, (...) optional
    '(\s+(name\s*=\s*'      + RE_FLAG % 'name'      + '))?|' +  # optional name=AARE, (...) optional
    '(\s+(interface\s*=\s*' + RE_FLAG % 'interface' + '))?|' +  # optional interface=AARE, (...) optional
    '(\s+(member\s*=\s*'    + RE_FLAG % 'member'    + '))?|' +  # optional member=AARE, (...) optional
    '(\s+(peer\s*=\s*\((,|\s)*'    +  # optional peer=( name=AARE and/or label=AARE ), (...) required
            '(' +
                '(' + '(,|\s)*' + ')' +  # empty peer=()
                '|'  # or
                '(' + 'name\s*=\s*' + RE_PROFILE_NAME % 'peername1' + ')' +  # only peer name (match group peername1)
                '|'  # or
                '(' + 'label\s*=\s*' + RE_PROFILE_NAME % 'peerlabel1' + ')' +  # only peer label (match group peerlabel1)
                '|'  # or
                '(' + 'name\s*=\s*'  + RE_PROFILE_NAME % 'peername2'  + '(,|\s)+' + 'label\s*=\s*' + RE_PROFILE_NAME % 'peerlabel2' + ')' +  # peer name + label (match name peername2/peerlabel2)
                '|'  # or
                '(' + 'label\s*=\s*' + RE_PROFILE_NAME % 'peerlabel3' + '(,|\s)+' + 'name\s*=\s*'  + RE_PROFILE_NAME % 'peername3'  + ')' +  # peer label + name (match name peername3/peerlabel3)
            ')'
        '(,|\s)*\)))?){0,6}'
    '\s*$')


class DbusRule(BaseRule):
    '''Class to handle and store a single dbus rule'''

    # Nothing external should reference this class, all external users
    # should reference the class field DbusRule.ALL
    class __DbusAll(object):
        pass

    ALL = __DbusAll

    rule_name = 'dbus'

    def __init__(self, access, bus, path, name, interface, member, peername, peerlabel,
                audit=False, deny=False, allow_keyword=False, comment='', log_event=None):

        super(DbusRule, self).__init__(audit=audit, deny=deny,
                                             allow_keyword=allow_keyword,
                                             comment=comment,
                                             log_event=log_event)

        self.access, self.all_access, unknown_items = check_and_split_list(access, access_keywords, DbusRule.ALL, 'DbusRule', 'access')
        if unknown_items:
            raise AppArmorException(_('Passed unknown access keyword to DbusRule: %s') % ' '.join(unknown_items))

        #                                                       rulepart        partname        is_path log_event
        self.bus, self.all_buses            = self._aare_or_all(bus,            'bus',          False,  log_event)
        self.path, self.all_paths           = self._aare_or_all(path,           'path',         True,   log_event)
        self.name, self.all_names           = self._aare_or_all(name,           'name',         False,  log_event)
        self.interface, self.all_interfaces = self._aare_or_all(interface,      'interface',    False,  log_event)
        self.member, self.all_members       = self._aare_or_all(member,         'member',       False,  log_event)
        self.peername, self.all_peernames   = self._aare_or_all(peername,       'peer name',    False,  log_event)
        self.peerlabel, self.all_peerlabels = self._aare_or_all(peerlabel,      'peer label',   False,  log_event)

        # not all combinations are allowed
        if self.access and 'bind' in self.access and (self.path or self.interface or self.member or self.peername or self.peerlabel):
                raise AppArmorException(_('dbus bind rules must not contain a path, interface, member or peer conditional'))
        elif self.access and 'eavesdrop' in self.access and (self.name or self.path or self.interface or self.member or self.peername or self.peerlabel):
                raise AppArmorException(_('dbus eavesdrop rules must not contain a name, path, interface, member or peer conditional'))
        elif self.access and self.name:
            for msg in message_keywords:
                if msg in self.access:
                    raise AppArmorException(_('dbus %s rules must not contain a name conditional') % '/'.join(self.access))

    @classmethod
    def _match(cls, raw_rule):
        return RE_PROFILE_DBUS.search(raw_rule)

    @classmethod
    def _parse(cls, raw_rule):
        '''parse raw_rule and return DbusRule'''

        matches = cls._match(raw_rule)
        if not matches:
            raise AppArmorException(_("Invalid dbus rule '%s'") % raw_rule)

        audit, deny, allow_keyword, comment = parse_modifiers(matches)

        rule_details = ''
        if matches.group('details'):
            rule_details = matches.group('details')

        if rule_details:
            details = RE_DBUS_DETAILS.search(rule_details)
            if not details:
                raise AppArmorException(_("Invalid or unknown keywords in 'dbus %s" % rule_details))

            if details.group('access'):
                # XXX move to function _split_access()?
                access = strip_parenthesis(details.group('access'))
                access = access.replace(',', ' ').split()  # split by ',' or whitespace
                if access == []:  # XXX that happens for "dbus ( )," rules - correct behaviour? (also: same for signal rules?)
                    access = DbusRule.ALL
            else:
                access = DbusRule.ALL

            if details.group('bus'):
                bus = strip_parenthesis(strip_quotes(details.group('bus')))
            else:
                bus = DbusRule.ALL

            if details.group('path'):
                path = strip_parenthesis(strip_quotes(details.group('path')))
            else:
                path = DbusRule.ALL

            if details.group('name'):
                name = strip_parenthesis(strip_quotes(details.group('name')))
            else:
                name = DbusRule.ALL

            if details.group('interface'):
                interface = strip_parenthesis(strip_quotes(details.group('interface')))
            else:
                interface = DbusRule.ALL

            if details.group('member'):
                member = strip_parenthesis(strip_quotes(details.group('member')))
            else:
                member = DbusRule.ALL

            if details.group('peername1'):
                peername = strip_parenthesis(strip_quotes(details.group('peername1')))
            elif details.group('peername2'):
                peername = strip_parenthesis(strip_quotes(details.group('peername2')))
            elif details.group('peername3'):
                peername = strip_parenthesis(strip_quotes(details.group('peername3')))
            else:
                peername = DbusRule.ALL

            if details.group('peerlabel1'):
                peerlabel = strip_parenthesis(strip_quotes(details.group('peerlabel1')))
            elif details.group('peerlabel2'):
                peerlabel = strip_parenthesis(strip_quotes(details.group('peerlabel2')))
            elif details.group('peerlabel3'):
                peerlabel = strip_parenthesis(strip_quotes(details.group('peerlabel3')))
            else:
                peerlabel = DbusRule.ALL

        else:
            access = DbusRule.ALL
            bus = DbusRule.ALL
            path = DbusRule.ALL
            name = DbusRule.ALL
            interface = DbusRule.ALL
            member = DbusRule.ALL
            peername = DbusRule.ALL
            peerlabel = DbusRule.ALL

        return DbusRule(access, bus, path, name, interface, member, peername, peerlabel,
                           audit=audit, deny=deny, allow_keyword=allow_keyword, comment=comment)

    def get_clean(self, depth=0):
        '''return rule (in clean/default formatting)'''

        space = '  ' * depth

        # XXX split off _get_access_rule_part? (also needed in PtraceRule)
        if self.all_access:
            access = ''
        elif len(self.access) == 1:
            access = ' %s' % ' '.join(self.access)
        elif self.access:
            access = ' (%s)' % ' '.join(sorted(self.access))
        else:
            raise AppArmorBug('Empty access in dbus rule')

        bus         = self._get_aare_rule_part('bus',       self.bus,       self.all_buses)
        path        = self._get_aare_rule_part('path',      self.path,      self.all_paths)
        name        = self._get_aare_rule_part('name',      self.name,      self.all_names)
        interface   = self._get_aare_rule_part('interface', self.interface, self.all_interfaces)
        member      = self._get_aare_rule_part('member',    self.member,    self.all_members)

        peername    = self._get_aare_rule_part('name',      self.peername,  self.all_peernames)
        peerlabel   = self._get_aare_rule_part('label',     self.peerlabel, self.all_peerlabels)
        peer = peername + peerlabel
        if peer:
            peer = ' peer=(%s)' % peer.strip()

        return('%s%sdbus%s%s%s%s%s%s%s,%s' % (space, self.modifiers_str(), access, bus, path, name, interface, member, peer, self.comment))

    def _get_aare_rule_part(self, prefix, value, all_values):
        '''helper function to write a rule part
           value is expected to be a AARE'''
        if all_values:
            return ''
        elif value:
            return ' %(prefix)s=%(value)s' % {'prefix': prefix, 'value': quote_if_needed(value.regex)}
        else:
            raise AppArmorBug('Empty %(prefix_name)s in %(rule_name)s rule' % {'prefix_name': prefix, 'rule_name': self.rule_name})


    def is_covered_localvars(self, other_rule):
        '''check if other_rule is covered by this rule object'''

        if not self._is_covered_list(self.access,       self.all_access,        other_rule.access,      other_rule.all_access,      'access'):
            return False

        if not self._is_covered_aare_compat(self.bus,   self.all_buses,         other_rule.bus,         other_rule.all_buses,       'bus'):
            return False

        if not self._is_covered_aare_compat(self.path,  self.all_paths,         other_rule.path,        other_rule.all_paths,       'path'):
            return False

        if not self._is_covered_aare_compat(self.name,  self.all_names,         other_rule.name,        other_rule.all_names,       'name'):
            return False

        if not self._is_covered_aare_compat(self.interface, self.all_interfaces, other_rule.interface,  other_rule.all_interfaces,  'interface'):
            return False

        if not self._is_covered_aare_compat(self.member, self.all_members,      other_rule.member,      other_rule.all_members,     'member'):
            return False

        if not self._is_covered_aare_compat(self.peername, self.all_peernames,  other_rule.peername,    other_rule.all_peernames,   'peername'):
            return False

        if not self._is_covered_aare_compat(self.peerlabel, self.all_peerlabels, other_rule.peerlabel,  other_rule.all_peerlabels,  'peerlabel'):
            return False

        # still here? -> then it is covered
        return True


    def is_equal_localvars(self, rule_obj, strict):
        '''compare if rule-specific variables are equal'''

        if not type(rule_obj) == DbusRule:
            raise AppArmorBug('Passed non-dbus rule: %s' % str(rule_obj))

        if (self.access != rule_obj.access
                or self.all_access != rule_obj.all_access):
            return False

        if not self._is_equal_aare(self.bus,        self.all_buses,         rule_obj.bus,           rule_obj.all_buses,         'bus'):
            return False

        if not self._is_equal_aare(self.path,       self.all_paths,         rule_obj.path,          rule_obj.all_paths,         'path'):
            return False

        if not self._is_equal_aare(self.name,       self.all_names,         rule_obj.name,          rule_obj.all_names,         'name'):
            return False

        if not self._is_equal_aare(self.interface,  self.all_interfaces,    rule_obj.interface,     rule_obj.all_interfaces,    'interface'):
            return False

        if not self._is_equal_aare(self.member,     self.all_members,       rule_obj.member,        rule_obj.all_members,       'member'):
            return False

        if not self._is_equal_aare(self.peername,   self.all_peernames,     rule_obj.peername,      rule_obj.all_peernames,     'peername'):
            return False

        if not self._is_equal_aare(self.peerlabel,  self.all_peerlabels,    rule_obj.peerlabel,     rule_obj.all_peerlabels,    'peerlabel'):
            return False

        return True

    def logprof_header_localvars(self):
        access      = logprof_value_or_all(self.access,     self.all_access)
        bus         = logprof_value_or_all(self.bus,        self.all_buses)
        path        = logprof_value_or_all(self.path,       self.all_paths)
        name        = logprof_value_or_all(self.name,       self.all_names)
        interface   = logprof_value_or_all(self.interface,  self.all_interfaces)
        member      = logprof_value_or_all(self.member,     self.all_members)
        peername    = logprof_value_or_all(self.peername,   self.all_peernames)
        peerlabel   = logprof_value_or_all(self.peerlabel,  self.all_peerlabels)

        return [
            _('Access mode'),   access,
            _('Bus'),           bus,
            _('Path'),          path,
            _('Name'),          name,
            _('Interface'),     interface,
            _('Member'),        member,
            _('Peer name'),     peername,
            _('Peer label'),    peerlabel,
        ]


class DbusRuleset(BaseRuleset):
    '''Class to handle and store a collection of dbus rules'''

    def get_glob(self, path_or_rule):
        '''Return the next possible glob. For dbus rules, that means removing access or removing/globbing bus'''
        # XXX only remove one part, not all
        return 'dbus,'
