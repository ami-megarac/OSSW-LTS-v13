# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
#    Copyright (C) 2014-2015 Christian Boltz <apparmor@cboltz.de>
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
import apparmor.aa as apparmor

class Prof(object):
    def __init__(self, filename):
        apparmor.init_aa()
        self.aa = apparmor.aa
        self.filelist = apparmor.filelist
        self.include = apparmor.include
        self.filename = filename

class CleanProf(object):
    def __init__(self, same_file, profile, other):
        #If same_file we're basically comparing the file against itself to check superfluous rules
        self.same_file = same_file
        self.profile = profile
        self.other = other

    def compare_profiles(self):
        deleted = 0
        other_file_includes = list(self.other.filelist[self.other.filename]['include'].keys())

        #Remove the duplicate file-level includes from other
        for rule in self.profile.filelist[self.profile.filename]['include'].keys():
            if rule in other_file_includes:
                self.other.filelist[self.other.filename]['include'].pop(rule)

        for profile in self.profile.aa.keys():
            deleted += self.remove_duplicate_rules(profile)

        return deleted

    def remove_duplicate_rules(self, program):
        #Process the profile of the program
        #Process every hat in the profile individually
        file_includes = list(self.profile.filelist[self.profile.filename]['include'].keys())
        deleted = 0
        for hat in sorted(self.profile.aa[program].keys()):
            #The combined list of includes from profile and the file
            includes = list(self.profile.aa[program][hat]['include'].keys()) + file_includes

            #If different files remove duplicate includes in the other profile
            if not self.same_file:
                if self.other.aa[program].get(hat):  # carefully avoid to accidently initialize self.other.aa[program][hat]
                    for inc in includes:
                        if self.other.aa[program][hat]['include'].get(inc, False):
                            self.other.aa[program][hat]['include'].pop(inc)
                            deleted += 1

            #Clean up superfluous rules from includes in the other profile
            for inc in includes:
                if not self.profile.include.get(inc, {}).get(inc, False):
                    apparmor.load_include(inc)
                if self.other.aa[program].get(hat):  # carefully avoid to accidently initialize self.other.aa[program][hat]
                    deleted += apparmor.delete_duplicates(self.other.aa[program][hat], inc)

            #Clean duplicate rules in other profile
            for ruletype in apparmor.ruletypes:
                if not self.same_file:
                    if self.other.aa[program].get(hat):  # carefully avoid to accidently initialize self.other.aa[program][hat]
                        deleted += self.other.aa[program][hat][ruletype].delete_duplicates(self.profile.aa[program][hat][ruletype])
                else:
                    deleted += self.other.aa[program][hat][ruletype].delete_duplicates(None)

        return deleted
