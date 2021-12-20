# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
#    Copyright (C) 2014 Christian Boltz <apparmor@cboltz.de>
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

from apparmor.aare import AARE
from apparmor.common import AppArmorBug, type_is_str

# setup module translations
from apparmor.translations import init_translation
_ = init_translation()


class BaseRule(object):
    '''Base class to handle and store a single rule'''

    # type specific rules should inherit from this class.
    # Methods that subclasses need to implement:
    #   __init__
    #   _match(cls, raw_rule) (as a class method)
    #     - parses a raw rule and returns a regex match object
    #   _parse(cls, raw_rule) (as a class method)
    #     - parses a raw rule and returns an object of the Rule subclass
    #   get_clean(depth)
    #     - return rule in clean format
    #   is_covered(self, other_rule)
    #     - check if other_rule is covered by this rule (i.e. is a
    #       subset of this rule's permissions)
    #   is_equal_localvars(self, other_rule)
    #     - equality check for the rule-specific fields

    # decides if the (G)lob and Glob w/ (E)xt options are displayed
    can_glob = False
    can_glob_ext = False

    # defines if the (N)ew option is displayed
    can_edit = False

    # defines if the '(O)wner permissions on/off' option is displayed
    can_owner = False

    def __init__(self, audit=False, deny=False, allow_keyword=False,
                 comment='', log_event=None):
        '''initialize variables needed by all rule types'''
        self.audit = audit
        self.deny = deny
        self.allow_keyword = allow_keyword
        self.comment = comment
        self.log_event = log_event

        # Set only in the parse() class method
        self.raw_rule = None

    def _aare_or_all(self, rulepart, partname, is_path, log_event):
        '''checks rulepart and returns
           - (AARE, False) if rulepart is a (non-empty) string
           - (None, True) if rulepart is all_obj (typically *Rule.ALL)
           - raises AppArmorBug if rulepart is an empty string or has a wrong type

           Parameters:
           - rulepart: the rule part to check (string or *Rule.ALL object)
           - partname: the name of the rulepart (for example 'peer', used for exception messages)
           - is_path (passed through to AARE)
           - log_event (passed through to AARE)
           '''

        if rulepart == self.ALL:
            return None, True
        elif type_is_str(rulepart):
            if len(rulepart.strip()) == 0:
                raise AppArmorBug('Passed empty %(partname)s to %(classname)s: %(rulepart)s' %
                        {'partname': partname, 'classname': self.__class__.__name__, 'rulepart': str(rulepart)})
            return AARE(rulepart, is_path=is_path, log_event=log_event), False
        else:
            raise AppArmorBug('Passed unknown %(partname)s to %(classname)s: %(rulepart)s'
                    % {'partname': partname, 'classname': self.__class__.__name__, 'rulepart': str(rulepart)})

    def __repr__(self):
        classname = self.__class__.__name__
        try:
            raw_content = self.get_raw()  # will fail for BaseRule
            return '<%s> %s' % (classname, raw_content)
        except NotImplementedError:
            return '<%s (NotImplementedError - get_clean() not implemented?)>' % classname

    @classmethod
    def match(cls, raw_rule):
        '''return True if raw_rule matches the class (main) regex, False otherwise
           Note: This function just provides an answer to "is this your job?".
                 It does not guarantee that the rule is completely valid.'''

        if cls._match(raw_rule):
            return True
        else:
            return False

    # @abstractmethod  FIXME - uncomment when python3 only
    @classmethod
    def _match(cls, raw_rule):
        '''parse raw_rule and return regex match object'''
        raise NotImplementedError("'%s' needs to implement _match(), but didn't" % (str(cls)))

    @classmethod
    def parse(cls, raw_rule):
        '''parse raw_rule and return a rule object'''
        rule = cls._parse(raw_rule)
        rule.raw_rule = raw_rule.strip()
        return rule

    # @abstractmethod  FIXME - uncomment when python3 only
    @classmethod
    def _parse(cls, raw_rule):
        '''returns a Rule object created from parsing the raw rule.
           required to be implemented by subclasses; raise exception if not'''
        raise NotImplementedError("'%s' needs to implement _parse(), but didn't" % (str(cls)))

    # @abstractmethod  FIXME - uncomment when python3 only
    def get_clean(self, depth=0):
        '''return clean rule (with default formatting, and leading whitespace as specified in the depth parameter)'''
        raise NotImplementedError("'%s' needs to implement get_clean(), but didn't" % (str(self.__class__)))

    def get_raw(self, depth=0):
        '''return raw rule (with original formatting, and leading whitespace in the depth parameter)'''
        if self.raw_rule:
            return '%s%s' % ('  ' * depth, self.raw_rule)
        else:
            return self.get_clean(depth)

    def is_covered(self, other_rule, check_allow_deny=True, check_audit=False):
        '''check if other_rule is covered by this rule object'''

        if not type(other_rule) == type(self):
            raise AppArmorBug('Passes %s instead of %s' % (str(other_rule),self.__class__.__name__))

        if check_allow_deny and self.deny != other_rule.deny:
            return False

        if other_rule.deny and not self.deny:
            return False

        if check_audit and other_rule.audit != self.audit:
            return False

        if other_rule.audit and not self.audit:
            return False

        # still here? -> then the common part is covered, check rule-specific things now
        return self.is_covered_localvars(other_rule)

    # @abstractmethod  FIXME - uncomment when python3 only
    def is_covered_localvars(self, other_rule):
        '''check if the rule-specific parts of other_rule is covered by this rule object'''
        raise NotImplementedError("'%s' needs to implement is_covered_localvars(), but didn't" % (str(self)))

    def _is_covered_plain(self, self_value, self_all, other_value, other_all, cond_name):
        '''check if other_* is covered by self_* - for plain str, int etc.'''

        if not other_value and not other_all:
            raise AppArmorBug('No %(cond_name)s specified in other %(rule_name)s rule' % {'cond_name': cond_name, 'rule_name': self.rule_name})

        if not self_all:
            if other_all:
                return False
            if self_value != other_value:
                return False

        # still here? -> then it is covered
        return True

    def _is_covered_list(self, self_value, self_all, other_value, other_all, cond_name, sanity_check=True):
        '''check if other_* is covered by self_* - for lists'''

        if sanity_check and not other_value and not other_all:
            raise AppArmorBug('No %(cond_name)s specified in other %(rule_name)s rule' % {'cond_name': cond_name, 'rule_name': self.rule_name})

        if not self_all:
            if other_all:
                return False
            if not other_value.issubset(self_value):
                return False

        # still here? -> then it is covered
        return True

    def _is_covered_aare_compat(self, self_value, self_all, other_value, other_all, cond_name):
        '''check if other_* is covered by self_* - for AARE
           Note: this function checks against other_value.regex, which is not really correct, but avoids overly strict results when matching one regex against another
        '''
        if type(other_value) == AARE:
           other_value = other_value.regex

        return self._is_covered_aare(self_value, self_all, other_value, other_all, cond_name)

    def _is_covered_aare(self, self_value, self_all, other_value, other_all, cond_name):
        '''check if other_* is covered by self_* - for AARE'''

        if not other_value and not other_all:
            raise AppArmorBug('No %(cond_name)s specified in other %(rule_name)s rule' % {'cond_name': cond_name, 'rule_name': self.rule_name})

        if not self_all:
            if other_all:
                return False
            if not self_value.match(other_value):
                return False

        # still here? -> then it is covered
        return True

    def is_equal(self, rule_obj, strict=False):
        '''compare if rule_obj == self
           Calls is_equal_localvars() to compare rule-specific variables'''

        if self.audit != rule_obj.audit or self.deny != rule_obj.deny:
            return False

        if strict and (
            self.allow_keyword != rule_obj.allow_keyword
            or self.comment != rule_obj.comment
            or self.raw_rule != rule_obj.raw_rule
        ):
            return False

        return self.is_equal_localvars(rule_obj, strict)

    def _is_equal_aare(self, self_value, self_all, other_value, other_all, cond_name):
        '''check if other_* is the same as self_* - for AARE'''

        if not other_value and not other_all:
            raise AppArmorBug('No %(cond_name)s specified in other %(rule_name)s rule' % {'cond_name': cond_name, 'rule_name': self.rule_name})

        if self_all != other_all:
            return False

        if self_value and not self_value.is_equal(other_value):
            return False

        # still here? -> then it is equal
        return True

    # @abstractmethod  FIXME - uncomment when python3 only
    def is_equal_localvars(self, other_rule, strict):
        '''compare if rule-specific variables are equal'''
        raise NotImplementedError("'%s' needs to implement is_equal_localvars(), but didn't" % (str(self)))

    def severity(self, sev_db):
        '''return severity of this rule, which can be:
           - a number between 0 and 10, where 0 means harmless and 10 means critical,
           - "unknown" (to be exact: the value specified for "unknown" as set when loading the severity database), or
           - sev_db.NOT_IMPLEMENTED if no severity check is implemented for this rule type.
           sev_db must be an apparmor.severity.Severity object.'''
        return sev_db.NOT_IMPLEMENTED

    def logprof_header(self):
        '''return the headers (human-readable version of the rule) to display in aa-logprof for this rule object
           returns {'label1': 'value1', 'label2': 'value2'} '''

        headers = []
        qualifier = []

        if self.audit:
            qualifier += ['audit']

        if self.deny:
            qualifier += ['deny']
        elif self.allow_keyword:
            qualifier += ['allow']

        if qualifier:
            headers += [_('Qualifier'), ' '.join(qualifier)]

        headers += self.logprof_header_localvars()

        return headers

    # @abstractmethod  FIXME - uncomment when python3 only
    def logprof_header_localvars(self):
        '''return the headers (human-readable version of the rule) to display in aa-logprof for this rule object
           returns {'label1': 'value1', 'label2': 'value2'} '''
        raise NotImplementedError("'%s' needs to implement logprof_header(), but didn't" % (str(self)))

    # @abstractmethod  FIXME - uncomment when python3 only
    def edit_header(self):
        '''return the prompt for, and the path to edit when using '(N)ew' '''
        raise NotImplementedError("'%s' needs to implement edit_header(), but didn't" % (str(self)))

    # @abstractmethod  FIXME - uncomment when python3 only
    def validate_edit(self, newpath):
        '''validate the new path.
           Returns True if it covers the previous path, False if it doesn't.'''
        raise NotImplementedError("'%s' needs to implement validate_edit(), but didn't" % (str(self)))

    # @abstractmethod  FIXME - uncomment when python3 only
    def store_edit(self, newpath):
        '''store the changed path.
           This is done even if the new path doesn't match the original one.'''
        raise NotImplementedError("'%s' needs to implement store_edit(), but didn't" % (str(self)))

    def modifiers_str(self):
        '''return the allow/deny and audit keyword as string, including whitespace'''

        if self.audit:
            auditstr = 'audit '
        else:
            auditstr = ''

        if self.deny:
            allowstr = 'deny '
        elif self.allow_keyword:
            allowstr = 'allow '
        else:
            allowstr = ''

        return '%s%s' % (auditstr, allowstr)


class BaseRuleset(object):
    '''Base class to handle and store a collection of rules'''

    # decides if the (G)lob and Glob w/ (E)xt options are displayed
    # XXX TODO: remove in all *Ruleset classes (moved to *Rule)
    can_glob = True
    can_glob_ext = False

    def __init__(self):
        '''initialize variables needed by all ruleset types
           Do not override in child class unless really needed - override _init_vars() instead'''
        self.rules = []
        self._init_vars()

    def _init_vars(self):
        '''called by __init__() and delete_all_rules() - override in child class to initialize more variables'''
        pass

    def __repr__(self):
        classname = self.__class__.__name__
        if self.rules:
            return '<%s>\n' % classname + '\n'.join(self.get_raw(1)) + '</%s>' % classname
        else:
            return '<%s (empty) />' % classname

    def add(self, rule, cleanup=False):
        '''add a rule object
           if cleanup is specified, delete rules that are covered by the new rule
           (the difference to delete_duplicates() is: cleanup only deletes rules that
           are covered by the new rule, but keeps other, unrelated superfluous rules)
        '''
        deleted = 0

        if cleanup:
            oldrules = self.rules
            self.rules = []

            for oldrule in oldrules:
                if not rule.is_covered(oldrule):
                    self.rules.append(oldrule)
                else:
                    deleted += 1

        self.rules.append(rule)

        return deleted

    def get_raw(self, depth=0):
        '''return all raw rules (if possible/not modified in their original formatting).
           Returns an array of lines, with depth * leading whitespace'''

        data = []
        for rule in self.rules:
            data.append(rule.get_raw(depth))

        if data:
            data.append('')

        return data

    def get_clean(self, depth=0):
        '''return all rules (in clean/default formatting)
           Returns an array of lines, with depth * leading whitespace'''

        allow_rules = []
        deny_rules = []

        for rule in self.rules:
            if rule.deny:
                deny_rules.append(rule.get_clean(depth))
            else:
                allow_rules.append(rule.get_clean(depth))

        allow_rules.sort()
        deny_rules.sort()

        cleandata = []

        if deny_rules:
            cleandata += deny_rules
            cleandata.append('')

        if allow_rules:
            cleandata += allow_rules
            cleandata.append('')

        return cleandata

    def is_covered(self, rule, check_allow_deny=True, check_audit=False):
        '''return True if rule is covered by existing rules, otherwise False'''

        for r in self.rules:
            if r.is_covered(rule, check_allow_deny, check_audit):
                return True

        return False

#    def is_log_covered(self, parsed_log_event, check_allow_deny=True, check_audit=False):
#        '''return True if parsed_log_event is covered by existing rules, otherwise False'''
#
#        rule_obj = self.new_rule()
#        rule_obj.set_log(parsed_log_event)
#
#        return self.is_covered(rule_obj, check_allow_deny, check_audit)

    def delete(self, rule):
        '''Delete rule from rules'''

        rule_to_delete = False
        i = 0
        for r in self.rules:
            if r.is_equal(rule):
                rule_to_delete = True
                break
            i = i + 1

        if rule_to_delete:
            self.rules.pop(i)
        else:
            raise AppArmorBug('Attempt to delete non-existing rule %s' % rule.get_raw(0))

    def delete_duplicates(self, include_rules):
        '''Delete duplicate rules.
           include_rules must be a *_rules object or None'''

        deleted = 0

        # delete rules that are covered by include files
        if include_rules:
            oldrules = self.rules
            self.rules = []
            for rule in oldrules:
                if include_rules.is_covered(rule, True, False):
                    deleted += 1
                else:
                    self.rules.append(rule)

        # de-duplicate rules inside the profile
        deleted += self.delete_in_profile_duplicates()
        self.rules.reverse()
        deleted += self.delete_in_profile_duplicates()  # search again in reverse order - this will find the remaining duplicates
        self.rules.reverse()  # restore original order for raw output

        return deleted

    def delete_in_profile_duplicates(self):
        '''Delete duplicate rules inside a profile'''

        deleted = 0
        oldrules = self.rules
        self.rules = []

        for rule in oldrules:
            if not self.is_covered(rule, True, False):
                self.rules.append(rule)
            else:
                deleted += 1

        return deleted

    def get_glob_ext(self, path_or_rule):
        '''returns the next possible glob with extension (for file rules only).
           For all other rule types, raise an exception'''
        raise NotImplementedError("get_glob_ext is not available for this rule type!")


def check_and_split_list(lst, allowed_keywords, all_obj, classname, keyword_name, allow_empty_list=False):
    '''check if lst is all_obj or contains only items listed in allowed_keywords'''

    if lst == all_obj:
        return None, True, None
    elif type_is_str(lst):
        result_list = {lst}
    elif type(lst) in [list, tuple, set] and (len(lst) > 0 or allow_empty_list):
        result_list = set(lst)
    else:
        raise AppArmorBug('Passed unknown %(type)s object to %(classname)s: %(unknown_object)s' %
                {'type': type(lst), 'classname': classname, 'unknown_object': str(lst)})

    unknown_items = set()
    for item in result_list:
        if not item.strip():
            raise AppArmorBug('Passed empty %(keyword_name)s to %(classname)s' %
                    {'keyword_name': keyword_name, 'classname': classname})
        if item not in allowed_keywords:
            unknown_items.add(item)

    return result_list, False, unknown_items

def logprof_value_or_all(value, all_values):
    '''helper for logprof_header() to return 'all' (if all_values is True) or the specified value.
       For some types, the value is made more readable.'''

    if all_values:
        return _('ALL')

    if type(value) == AARE:
        return value.regex
    elif type(value) == set or type(value) == list or type(value) == tuple:
        return ' '.join(sorted(value))
    else:
        return value

def parse_comment(matches):
    '''returns the comment (with a leading space) from the matches object'''
    comment = ''
    if matches.group('comment'):
        # include a space so that we don't need to add it everywhere when writing the rule
        comment = ' %s' % matches.group('comment')
    return comment

def parse_modifiers(matches):
    '''returns audit, deny, allow_keyword and comment from the matches object
       - audit, deny and allow_keyword are True/False
       - comment is the comment with a leading space'''
    audit = False
    if matches.group('audit'):
        audit = True

    deny = False
    allow_keyword = False

    allowstr = matches.group('allow')
    if allowstr:
        if allowstr.strip() == 'allow':
            allow_keyword = True
        elif allowstr.strip() == 'deny':
            deny = True
        else:
            raise AppArmorBug("Invalid allow/deny keyword %s" % allowstr)

    comment = parse_comment(matches)

    return (audit, deny, allow_keyword, comment)

def quote_if_needed(data):
    '''quote data if it contains whitespace'''
    if ' ' in data:
        data = '"' + data + '"'
    return data

