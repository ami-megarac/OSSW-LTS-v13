# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
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
from __future__ import with_statement
import os
import re
from apparmor.common import AppArmorException, open_file_read, warn, convert_regexp  # , msg, error, debug
from apparmor.regex import re_match_include

class Severity(object):
    def __init__(self, dbname=None, default_rank=10):
        """Initialises the class object"""
        self.PROF_DIR = '/etc/apparmor.d'  # The profile directory
        self.NOT_IMPLEMENTED = '_-*not*implemented*-_'  # used for rule types that don't have severity ratings
        self.severity = dict()
        self.severity['DATABASENAME'] = dbname
        self.severity['CAPABILITIES'] = {}
        self.severity['FILES'] = {}
        self.severity['REGEXPS'] = {}
        self.severity['DEFAULT_RANK'] = default_rank
        # For variable expansions for the profile
        self.severity['VARIABLES'] = dict()
        if not dbname:
            raise AppArmorException("No severity db file given")

        with open_file_read(dbname) as database:  # open(dbname, 'r')
            for lineno, line in enumerate(database, start=1):
                line = line.strip()  # or only rstrip and lstrip?
                if line == '' or line.startswith('#'):
                    continue
                if line.startswith('/'):
                    try:
                        path, read, write, execute = line.split()
                        read, write, execute = int(read), int(write), int(execute)
                    except ValueError:
                        raise AppArmorException("Insufficient values for permissions in file: %s\n\t[Line %s]: %s" % (dbname, lineno, line))
                    else:
                        if read not in range(0, 11) or write not in range(0, 11) or execute not in range(0, 11):
                            raise AppArmorException("Inappropriate values for permissions in file: %s\n\t[Line %s]: %s" % (dbname, lineno, line))
                        path = path.lstrip('/')
                        if '*' not in path:
                            self.severity['FILES'][path] = {'r': read, 'w': write, 'x': execute}
                        else:
                            ptr = self.severity['REGEXPS']
                            pieces = path.split('/')
                            for index, piece in enumerate(pieces):
                                if '*' in piece:
                                    path = '/'.join(pieces[index:])
                                    regexp = convert_regexp(path)
                                    ptr[regexp] = {'AA_RANK': {'r': read, 'w': write, 'x': execute}}
                                    break
                                else:
                                    ptr[piece] = ptr.get(piece, {})
                                    ptr = ptr[piece]
                elif line.startswith('CAP_'):
                    try:
                        resource, severity = line.split()
                        severity = int(severity)
                    except ValueError:
                        error_message = 'No severity value present in file: %s\n\t[Line %s]: %s' % (dbname, lineno, line)
                        #error(error_message)
                        raise AppArmorException(error_message)  # from None
                    else:
                        if severity not in range(0, 11):
                            raise AppArmorException("Inappropriate severity value present in file: %s\n\t[Line %s]: %s" % (dbname, lineno, line))
                        self.severity['CAPABILITIES'][resource] = severity
                else:
                    raise AppArmorException("Unexpected line in file: %s\n\t[Line %s]: %s" % (dbname, lineno, line))

    def rank_capability(self, resource):
        """Returns the severity of for the capability resource, default value if no match"""
        cap = 'CAP_%s' % resource.upper()
        if resource == '__ALL__':
            return max(self.severity['CAPABILITIES'].values())
        if cap in self.severity['CAPABILITIES'].keys():
            return self.severity['CAPABILITIES'][cap]
        # raise ValueError("unexpected capability rank input: %s"%resource)
        warn("unknown capability: %s" % resource)
        return self.severity['DEFAULT_RANK']

    def rank_path(self, path, mode=None):
        """Returns the rank for the given path"""
        if '@' in path:    # path contains variable
            return self.handle_variable_rank(path, mode)
        elif path[0] == '/':    # file resource
            return self.handle_file(path, mode)
        else:
            raise AppArmorException("Unexpected path input: %s" % path)

    def check_subtree(self, tree, mode, sev, segments):
        """Returns the max severity from the regex tree"""
        if len(segments) == 0:
            first = ''
        else:
            first = segments[0]
        rest = segments[1:]
        path = '/'.join([first] + rest)
        # Check if we have a matching directory tree to descend into
        if tree.get(first, False):
            sev = self.check_subtree(tree[first], mode, sev, rest)
        # If severity still not found, match against globs
        if sev is None:
            # Match against all globs at this directory level
            for chunk in tree.keys():
                if '*' in chunk:
                    # Match rest of the path
                    if re.search("^" + chunk, path):
                        # Find max rank
                        if "AA_RANK" in tree[chunk].keys():
                            for m in mode:
                                if sev is None or tree[chunk]["AA_RANK"].get(m, -1) > sev:
                                    sev = tree[chunk]["AA_RANK"].get(m, None)
        return sev

    def handle_file(self, resource, mode):
        """Returns the severity for the file, default value if no match found"""
        resource = resource[1:]    # remove initial / from path
        pieces = resource.split('/')    # break path into directory level chunks
        sev = None
        # Check for an exact match in the db
        if resource in self.severity['FILES'].keys():
            # Find max value among the given modes
            for m in mode:
                if sev is None or self.severity['FILES'][resource].get(m, -1) > sev:
                    sev = self.severity['FILES'][resource].get(m, None)
        else:
            # Search regex tree for matching glob
            sev = self.check_subtree(self.severity['REGEXPS'], mode, sev, pieces)
        if sev is None:
            # Return default rank if severity cannot be found
            return self.severity['DEFAULT_RANK']
        else:
            return sev

    def handle_variable_rank(self, resource, mode):
        """Returns the max possible rank for file resources containing variables"""
        regex_variable = re.compile('@{([^{.]*)}')
        matches = regex_variable.search(resource)
        if matches:
            rank = self.severity['DEFAULT_RANK']
            variable = '@{%s}' % matches.groups()[0]
            #variables = regex_variable.findall(resource)
            for replacement in self.severity['VARIABLES'][variable]:
                resource_replaced = self.variable_replace(variable, replacement, resource)
                rank_new = self.handle_variable_rank(resource_replaced, mode)
                if rank == self.severity['DEFAULT_RANK']:
                    rank = rank_new
                elif rank_new != self.severity['DEFAULT_RANK'] and rank_new > rank:
                    rank = rank_new
            return rank
        else:
            return self.handle_file(resource, mode)

    def variable_replace(self, variable, replacement, resource):
        """Returns the expanded path for the passed variable"""
        leading = False
        trailing = False
        # Check for leading or trailing / that may need to be collapsed
        if resource.find("/" + variable) != -1 and resource.find("//" + variable) == -1:  # find that a single / exists before variable or not
            leading = True
        if resource.find(variable + "/") != -1 and resource.find(variable + "//") == -1:
            trailing = True
        if replacement[0] == '/' and replacement[:2] != '//' and leading:  # finds if the replacement has leading / or not
            replacement = replacement[1:]
        if replacement[-1] == '/' and replacement[-2:] != '//' and trailing:
            replacement = replacement[:-1]
        return resource.replace(variable, replacement)

    def load_variables(self, prof_path):
        """Loads the variables for the given profile"""
        if os.path.isfile(prof_path):
            with open_file_read(prof_path) as f_in:
                for line in f_in:
                    line = line.strip()
                    # If any includes, load variables from them first
                    match = re_match_include(line)
                    if match:
                        new_path = match
                        if not new_path.startswith('/'):
                            new_path = self.PROF_DIR + '/' + match
                        self.load_variables(new_path)
                    else:
                        # Remove any comments
                        if '#' in line:
                            line = line.split('#')[0].rstrip()
                        # Expected format is @{Variable} = value1 value2 ..
                        if line.startswith('@') and '=' in line:
                            if '+=' in line:
                                line = line.split('+=')
                                try:
                                    self.severity['VARIABLES'][line[0]] += [i.strip('"') for i in line[1].split()]
                                except KeyError:
                                    raise AppArmorException("Variable %s was not previously declared, but is being assigned additional value in file: %s" % (line[0], prof_path))
                            else:
                                line = line.split('=')
                                if line[0] in self.severity['VARIABLES'].keys():
                                    raise AppArmorException("Variable %s was previously declared in file: %s" % (line[0], prof_path))
                                self.severity['VARIABLES'][line[0]] = [i.strip('"') for i in line[1].split()]

    def unload_variables(self):
        """Clears all loaded variables"""
        self.severity['VARIABLES'] = dict()
