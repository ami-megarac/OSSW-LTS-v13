# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
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

from apparmor.regex import RE_PROFILE_RLIMIT, strip_quotes
from apparmor.common import AppArmorBug, AppArmorException, type_is_str
from apparmor.rule import BaseRule, BaseRuleset, parse_comment, quote_if_needed

# setup module translations
from apparmor.translations import init_translation
_ = init_translation()

rlimit_size     = ['fsize', 'data', 'stack', 'core', 'rss', 'as', 'memlock', 'msgqueue']  # NUMBER ( 'K' | 'M' | 'G' )
rlimit_number   = ['ofile', 'nofile', 'locks', 'sigpending', 'nproc', 'rtprio']
rlimit_time     = ['cpu', 'rttime']  # number + time unit (cpu in seconds+, rttime in us+)
rlimit_nice     = ['nice']  # a number between -20 and 19.

rlimit_all      = rlimit_size + rlimit_number + rlimit_time + rlimit_nice

RE_NUMBER_UNIT  = re.compile('^(?P<number>[0-9]+)\s*(?P<unit>[a-zA-Z]*)$')
RE_NUMBER       = re.compile('^[0-9]+$')
RE_UNIT_SIZE    = re.compile('^[0-9]+\s*([KMG]B?)?$')
RE_NICE         = re.compile('^(-20|-[01]?[0-9]|[01]?[0-9])$')


class RlimitRule(BaseRule):
    '''Class to handle and store a single rlimit rule'''

    # Nothing external should reference this class, all external users
    # should reference the class field RlimitRule.ALL
    class __RlimitAll(object):
        pass

    ALL = __RlimitAll

    rule_name = 'rlimit'

    def __init__(self, rlimit, value, audit=False, deny=False, allow_keyword=False,
                 comment='', log_event=None):

        super(RlimitRule, self).__init__(audit=audit, deny=deny,
                                             allow_keyword=allow_keyword,
                                             comment=comment,
                                             log_event=log_event)

        if audit or deny or allow_keyword:
            raise AppArmorBug('The audit, allow or deny keywords are not allowed in rlimit rules.')

        if type_is_str(rlimit):
            if rlimit in rlimit_all:
                self.rlimit = rlimit
            else:
                raise AppArmorException('Unknown rlimit keyword in rlimit rule: %s' % rlimit)
        else:
            raise AppArmorBug('Passed unknown object to RlimitRule: %s' % str(rlimit))

        self.value = None
        self.value_as_int = None
        self.all_values = False
        if value == RlimitRule.ALL:
            self.all_values = True
        elif type_is_str(value):
            if not value.strip():
                raise AppArmorBug('Empty value in rlimit rule')

            elif rlimit in rlimit_size:
                if not RE_UNIT_SIZE.match(value):
                    raise AppArmorException('Invalid value or unit in rlimit %s %s rule' % (rlimit, value))
                self.value_as_int = self.size_to_int(value)

            elif rlimit in rlimit_number:
                if not RE_NUMBER.match(value):
                    raise AppArmorException('Invalid value in rlimit %s %s rule' % (rlimit, value))
                self.value_as_int = int(value)

            elif rlimit in rlimit_time:
                if not RE_NUMBER_UNIT.match(value):
                    raise AppArmorException('Invalid value in rlimit %s %s rule' % (rlimit, value))
                number, unit = split_unit(value)

                if rlimit == 'rttime':
                    self.value_as_int = self.time_to_int(value, 'us')
                else:
                    self.value_as_int = self.time_to_int(value, 'seconds')

            elif rlimit in rlimit_nice:  # pragma: no branch - "if rlimit in rlimit_all:" above avoids the need for an "else:" branch
                if not RE_NICE.match(value):
                    raise AppArmorException('Invalid value or unit in rlimit %s %s rule' % (rlimit, value))
                self.value_as_int = 0 - int(value)  # lower numbers mean a higher limit for nice

            # still here? fine :-)
            self.value = value
        else:
            raise AppArmorBug('Passed unknown object to RlimitRule: %s' % str(value))

    @classmethod
    def _match(cls, raw_rule):
        return RE_PROFILE_RLIMIT.search(raw_rule)

    @classmethod
    def _parse(cls, raw_rule):
        '''parse raw_rule and return RlimitRule'''

        matches = cls._match(raw_rule)
        if not matches:
            raise AppArmorException(_("Invalid rlimit rule '%s'") % raw_rule)

        comment = parse_comment(matches)

        if matches.group('rlimit'):
            rlimit = strip_quotes(matches.group('rlimit'))
        else:
            raise AppArmorException(_("Invalid rlimit rule '%s' - keyword missing") % raw_rule)  # pragma: no cover - would need breaking the regex

        if matches.group('value'):
            if matches.group('value') == 'infinity':
                value = RlimitRule.ALL
            else:
                value = strip_quotes(matches.group('value'))
        else:
            raise AppArmorException(_("Invalid rlimit rule '%s' - value missing") % raw_rule)  # pragma: no cover - would need breaking the regex

        return RlimitRule(rlimit, value,
                           comment=comment)

    def get_clean(self, depth=0):
        '''return rule (in clean/default formatting)'''

        space = '  ' * depth

        if self.rlimit:
            rlimit = ' %s' % quote_if_needed(self.rlimit)
        else:
            raise AppArmorBug('Empty rlimit in rlimit rule')

        if self.all_values:
            value = ' <= infinity'
        elif self.value:
            value = ' <= %s' % quote_if_needed(self.value)
        else:
            raise AppArmorBug('Empty value in rlimit rule')

        return('%s%sset rlimit%s%s,%s' % (space, self.modifiers_str(), rlimit, value, self.comment))

    def size_to_int(self, value):
        number, unit = split_unit(value)

        if unit == '':
            pass
        elif unit == 'K' or unit == 'KB':
            number = number * 1024
        elif unit == 'M' or unit == 'MB':
            number = number * 1024 * 1024
        elif unit == 'G' or unit == 'GB':
            number = number * 1024 * 1024 * 1024
        else:
            raise AppArmorException('Unknown unit %s in rlimit %s %s' % (unit, self.rlimit, value))

        return number

    def time_to_int(self, value, default_unit):
        number, unit = split_unit(value)

        if unit == '':
            unit = default_unit

        if unit in ['us', 'microsecond', 'microseconds']:
            number = number / 1000000.0
            if default_unit == 'seconds':
                raise AppArmorException(_('Invalid unit in rlimit cpu %s rule') % value)
        elif unit in ['ms', 'millisecond', 'milliseconds']:
            number = number / 1000.0
            if default_unit == 'seconds':
                raise AppArmorException(_('Invalid unit in rlimit cpu %s rule') % value)
        elif unit in ['s', 'sec', 'second', 'seconds']: # manpage doesn't list sec
            pass
        elif unit in ['min', 'minute', 'minutes']:
            number = number * 60
        elif unit in ['h', 'hour', 'hours']:
            number = number * 60 * 60
        elif unit in ['d', 'day', 'days']: # manpage doesn't list 'd'
            number = number * 60 * 60 * 24
        elif unit in ['week', 'weeks']:
            number = number * 60 * 60 * 24 * 7
        else:
            raise AppArmorException('Unknown unit %s in rlimit %s %s' % (unit, self.rlimit, value))

        return number

    def is_covered_localvars(self, other_rule):
        '''check if other_rule is covered by this rule object'''

        if not self._is_covered_plain(self.rlimit, False, other_rule.rlimit, False, 'rlimit'):  # rlimit can't be ALL, therefore using False
            return False

        if not other_rule.value and not other_rule.all_values:
            raise AppArmorBug('No target profile specified in other rlimit rule')

        if not self.all_values:
            if other_rule.all_values:
                return False
            if other_rule.value_as_int > self.value_as_int:
                return False

        # still here? -> then it is covered
        return True

    def is_equal_localvars(self, rule_obj, strict):
        '''compare if rule-specific variables are equal'''

        if not type(rule_obj) == RlimitRule:
            raise AppArmorBug('Passed non-rlimit rule: %s' % str(rule_obj))

        if (self.rlimit != rule_obj.rlimit):
            return False

        if (self.value_as_int != rule_obj.value_as_int
                or self.all_values != rule_obj.all_values):
            return False

        return True

    def logprof_header_localvars(self):
        rlimit_txt = self.rlimit

        if self.all_values:
            values_txt = 'infinity'
        else:
            values_txt = self.value

        return [
            _('Rlimit'), rlimit_txt,
            _('Value'),  values_txt,
        ]

class RlimitRuleset(BaseRuleset):
    '''Class to handle and store a collection of rlimit rules'''

    def get_glob(self, path_or_rule):
        '''Return the next possible glob. For rlimit rules, that can mean changing the value to 'infinity' '''
        # XXX implement all options mentioned above ;-)
        raise AppArmorBug('get_glob() is not (yet) available for this rule type')


def split_unit(value):
    matches = RE_NUMBER_UNIT.match(value)
    if not matches:
        raise AppArmorBug("Invalid value given to split_unit: %s" % value)

    number = int(matches.group('number'))
    unit = matches.group('unit') or ''

    return number, unit


