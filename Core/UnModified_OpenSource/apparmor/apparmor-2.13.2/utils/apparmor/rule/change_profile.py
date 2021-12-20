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

from apparmor.regex import RE_PROFILE_CHANGE_PROFILE, strip_quotes
from apparmor.common import AppArmorBug, AppArmorException, type_is_str
from apparmor.rule import BaseRule, BaseRuleset, parse_modifiers, logprof_value_or_all, quote_if_needed

# setup module translations
from apparmor.translations import init_translation
_ = init_translation()


class ChangeProfileRule(BaseRule):
    '''Class to handle and store a single change_profile rule'''

    # Nothing external should reference this class, all external users
    # should reference the class field ChangeProfileRule.ALL
    class __ChangeProfileAll(object):
        pass

    ALL = __ChangeProfileAll

    rule_name = 'change_profile'

    equiv_execmodes = [ 'safe', '', None ]

    def __init__(self, execmode, execcond, targetprofile, audit=False, deny=False, allow_keyword=False,
                 comment='', log_event=None):

        '''
            CHANGE_PROFILE RULE = 'change_profile' [ [ EXEC MODE ] EXEC COND ] [ -> PROGRAMCHILD ]
        '''

        super(ChangeProfileRule, self).__init__(audit=audit, deny=deny,
                                             allow_keyword=allow_keyword,
                                             comment=comment,
                                             log_event=log_event)

        if execmode:
            if execmode != 'safe' and execmode != 'unsafe':
                raise AppArmorBug('Unknown exec mode (%s) in change_profile rule' % execmode)
            elif not execcond or execcond == ChangeProfileRule.ALL:
                raise AppArmorException('Exec condition is required when unsafe or safe keywords are present')
        self.execmode = execmode

        self.execcond = None
        self.all_execconds = False
        if execcond == ChangeProfileRule.ALL:
            self.all_execconds = True
        elif type_is_str(execcond):
            if not execcond.strip():
                raise AppArmorBug('Empty exec condition in change_profile rule')
            elif execcond.startswith('/') or execcond.startswith('@'):
                self.execcond = execcond
            else:
                raise AppArmorException('Exec condition in change_profile rule does not start with /: %s' % str(execcond))
        else:
            raise AppArmorBug('Passed unknown object to ChangeProfileRule: %s' % str(execcond))

        self.targetprofile = None
        self.all_targetprofiles = False
        if targetprofile == ChangeProfileRule.ALL:
            self.all_targetprofiles = True
        elif type_is_str(targetprofile):
            if targetprofile.strip():
                self.targetprofile = targetprofile
            else:
                raise AppArmorBug('Empty target profile in change_profile rule')
        else:
            raise AppArmorBug('Passed unknown object to ChangeProfileRule: %s' % str(targetprofile))

    @classmethod
    def _match(cls, raw_rule):
        return RE_PROFILE_CHANGE_PROFILE.search(raw_rule)

    @classmethod
    def _parse(cls, raw_rule):
        '''parse raw_rule and return ChangeProfileRule'''

        matches = cls._match(raw_rule)
        if not matches:
            raise AppArmorException(_("Invalid change_profile rule '%s'") % raw_rule)

        audit, deny, allow_keyword, comment = parse_modifiers(matches)

        execmode = matches.group('execmode')

        if matches.group('execcond'):
            execcond = strip_quotes(matches.group('execcond'))
        else:
            execcond = ChangeProfileRule.ALL

        if matches.group('targetprofile'):
            targetprofile = strip_quotes(matches.group('targetprofile'))
        else:
            targetprofile = ChangeProfileRule.ALL

        return ChangeProfileRule(execmode, execcond, targetprofile,
                           audit=audit, deny=deny, allow_keyword=allow_keyword, comment=comment)

    def get_clean(self, depth=0):
        '''return rule (in clean/default formatting)'''

        space = '  ' * depth

        if self.execmode:
            execmode = ' %s' % self.execmode
        else:
            execmode = ''

        if self.all_execconds:
            execcond = ''
        elif self.execcond:
            execcond = ' %s' % quote_if_needed(self.execcond)
        else:
            raise AppArmorBug('Empty execcond in change_profile rule')

        if self.all_targetprofiles:
            targetprofile = ''
        elif self.targetprofile:
            targetprofile = ' -> %s' % quote_if_needed(self.targetprofile)
        else:
            raise AppArmorBug('Empty target profile in change_profile rule')

        return('%s%schange_profile%s%s%s,%s' % (space, self.modifiers_str(), execmode, execcond, targetprofile, self.comment))

    def is_covered_localvars(self, other_rule):
        '''check if other_rule is covered by this rule object'''

        if self.execmode != other_rule.execmode and \
           (self.execmode not in ChangeProfileRule.equiv_execmodes or \
            other_rule.execmode not in ChangeProfileRule.equiv_execmodes):
            return False

        if not self._is_covered_plain(self.execcond, self.all_execconds, other_rule.execcond, other_rule.all_execconds, 'exec condition'):
            # TODO: honor globbing and variables
            return False

        if not self._is_covered_plain(self.targetprofile, self.all_targetprofiles, other_rule.targetprofile, other_rule.all_targetprofiles, 'target profile'):
            return False

        # still here? -> then it is covered
        return True

    def is_equal_localvars(self, rule_obj, strict):
        '''compare if rule-specific variables are equal'''

        if not type(rule_obj) == ChangeProfileRule:
            raise AppArmorBug('Passed non-change_profile rule: %s' % str(rule_obj))

        if self.execmode != rule_obj.execmode and \
           (self.execmode not in ChangeProfileRule.equiv_execmodes or \
            rule_obj.execmode not in ChangeProfileRule.equiv_execmodes):
            return False

        if (self.execcond != rule_obj.execcond
                or self.all_execconds != rule_obj.all_execconds):
            return False

        if (self.targetprofile != rule_obj.targetprofile
                or self.all_targetprofiles != rule_obj.all_targetprofiles):
            return False

        return True

    def logprof_header_localvars(self):
        headers = []

        if self.execmode:
            headers += [_('Exec Mode'), self.execmode]

        execcond_txt        = logprof_value_or_all(self.execcond,       self.all_execconds)
        targetprofiles_txt  = logprof_value_or_all(self.targetprofile,  self.all_targetprofiles)

        return headers + [
            _('Exec Condition'), execcond_txt,
            _('Target Profile'), targetprofiles_txt,
        ]

class ChangeProfileRuleset(BaseRuleset):
    '''Class to handle and store a collection of change_profile rules'''

    def get_glob(self, path_or_rule):
        '''Return the next possible glob. For change_profile rules, that can be "change_profile EXECCOND,",
           "change_profile -> TARGET_PROFILE," or "change_profile," (all change_profile).
           Also, EXECCOND filename can be globbed'''
        # XXX implement all options mentioned above ;-)
        return 'change_profile,'
