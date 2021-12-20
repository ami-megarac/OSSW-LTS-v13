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
import os
import sys

import apparmor.aa as apparmor
import apparmor.ui as aaui
from apparmor.common import user_perm, cmd

# setup module translations
from apparmor.translations import init_translation
_ = init_translation()

class aa_tools:
    def __init__(self, tool_name, args):
        apparmor.init_aa()

        self.name = tool_name
        self.profiledir = args.dir
        self.profiling = args.program
        self.check_profile_dir()
        self.silent = None
        self.do_reload = args.do_reload

        if tool_name in ['audit']:
            self.remove = args.remove
        elif tool_name == 'autodep':
            self.force = args.force
            self.aa_mountpoint = apparmor.check_for_apparmor()
        elif tool_name == 'cleanprof':
            self.silent = args.silent

    def check_profile_dir(self):
        if self.profiledir:
            apparmor.profile_dir = apparmor.get_full_path(self.profiledir)
            if not os.path.isdir(apparmor.profile_dir):
                raise apparmor.AppArmorException("%s is not a directory." % self.profiledir)

        if not user_perm(apparmor.profile_dir):
            raise apparmor.AppArmorException("Cannot write to profile directory: %s" % (apparmor.profile_dir))

    def get_next_to_profile(self):
        '''Iterator function to walk the list of arguments passed'''

        for p in self.profiling:
            if not p:
                continue

            program = None
            profile = None
            if os.path.exists(p) or p.startswith('/'):
                fq_path = apparmor.get_full_path(p).strip()
                if os.path.commonprefix([apparmor.profile_dir, fq_path]) == apparmor.profile_dir:
                    program = None
                    profile = fq_path
                else:
                    program = fq_path
                    profile = apparmor.get_profile_filename_from_attachment(fq_path, True)
            else:
                which = apparmor.which(p)
                if which is not None:
                    program = apparmor.get_full_path(which)
                    profile = apparmor.get_profile_filename_from_attachment(program, True)
                elif os.path.exists(os.path.join(apparmor.profile_dir, p)):
                    program = None
                    profile = apparmor.get_full_path(os.path.join(apparmor.profile_dir, p)).strip()
                else:
                    if '/' not in p:
                        aaui.UI_Info(_("Can't find %(program)s in the system path list. If the name of the application\nis correct, please run 'which %(program)s' as a user with correct PATH\nenvironment set up in order to find the fully-qualified path and\nuse the full path as parameter.") % { 'program': p })
                    else:
                        aaui.UI_Info(_("%s does not exist, please double-check the path.") % p)
                    continue

            yield (program, profile)

    def cleanprof_act(self):
        # used by aa-cleanprof
        apparmor.read_profiles()

        for (program, profile) in self.get_next_to_profile():
            if program is None:
                program = profile

            if not program or not(os.path.exists(program) or apparmor.profile_exists(program)):
                if program and not program.startswith('/'):
                    program = aaui.UI_GetString(_('The given program cannot be found, please try with the fully qualified path name of the program: '), '')
                else:
                    aaui.UI_Info(_("%s does not exist, please double-check the path.") % program)
                    sys.exit(1)

            if program and apparmor.profile_exists(program):
                self.clean_profile(program)

            else:
                if '/' not in program:
                    aaui.UI_Info(_("Can't find %(program)s in the system path list. If the name of the application\nis correct, please run 'which %(program)s' as a user with correct PATH\nenvironment set up in order to find the fully-qualified path and\nuse the full path as parameter.") % { 'program': program })
                else:
                    aaui.UI_Info(_("%s does not exist, please double-check the path.") % program)
                    sys.exit(1)

    def cmd_disable(self):
        apparmor.read_profiles()

        for (program, profile) in self.get_next_to_profile():

            output_name = profile if program is None else program

            if not os.path.isfile(profile) or apparmor.is_skippable_file(profile):
                aaui.UI_Info(_('Profile for %s not found, skipping') % output_name)
                continue

            aaui.UI_Info(_('Disabling %s.') % output_name)
            self.disable_profile(profile)

            self.unload_profile(profile)

    def cmd_enforce(self):
        apparmor.read_profiles()

        for (program, profile) in self.get_next_to_profile():

            output_name = profile if program is None else program

            if not os.path.isfile(profile) or apparmor.is_skippable_file(profile):
                aaui.UI_Info(_('Profile for %s not found, skipping') % output_name)
                continue

            apparmor.set_enforce(profile, program)

            self.reload_profile(profile)

    def cmd_complain(self):
        apparmor.read_profiles()

        for (program, profile) in self.get_next_to_profile():

            output_name = profile if program is None else program

            if not os.path.isfile(profile) or apparmor.is_skippable_file(profile):
                aaui.UI_Info(_('Profile for %s not found, skipping') % output_name)
                continue

            apparmor.set_complain(profile, program)

            self.reload_profile(profile)

    def cmd_audit(self):
        apparmor.read_profiles()

        for (program, profile) in self.get_next_to_profile():

            output_name = profile if program is None else program

            if not os.path.isfile(profile) or apparmor.is_skippable_file(profile):
                aaui.UI_Info(_('Profile for %s not found, skipping') % output_name)
                continue

            # keep this to allow toggling 'audit' flags
            if not self.remove:
                aaui.UI_Info(_('Setting %s to audit mode.') % output_name)
            else:
                aaui.UI_Info(_('Removing audit mode from %s.') % output_name)
            apparmor.change_profile_flags(profile, program, 'audit', not self.remove)

            disable_link = '%s/disable/%s' % (apparmor.profile_dir, os.path.basename(profile))

            if os.path.exists(disable_link):
                aaui.UI_Info(_('\nWarning: the profile %s is disabled. Use aa-enforce or aa-complain to enable it.') % os.path.basename(profile))

            self.reload_profile(profile)

    def cmd_autodep(self):
        apparmor.read_profiles()

        for (program, profile) in self.get_next_to_profile():
            if not program:
                aaui.UI_Info(_('Please pass an application to generate a profile for, not a profile itself - skipping %s.') % profile)
                continue

            apparmor.check_qualifiers(program)

            if os.path.exists(apparmor.get_profile_filename_from_attachment(program, True)) and not self.force:
                aaui.UI_Info(_('Profile for %s already exists - skipping.') % program)
            else:
                apparmor.autodep(program)
                if self.aa_mountpoint:
                    apparmor.reload(program)

    def clean_profile(self, program):
        filename = apparmor.get_profile_filename_from_attachment(program, True)
        import apparmor.cleanprofile as cleanprofile
        prof = cleanprofile.Prof(filename)
        cleanprof = cleanprofile.CleanProf(True, prof, prof)
        deleted = cleanprof.remove_duplicate_rules(program)
        aaui.UI_Info(_("\nDeleted %s rules.") % deleted)
        apparmor.changed[program] = True

        if filename:
            if not self.silent:
                q = aaui.PromptQuestion()
                q.title = 'Changed Local Profiles'
                q.explanation = _('The local profile for %(program)s in file %(file)s was changed. Would you like to save it?') % { 'program': program, 'file': filename }
                q.functions = ['CMD_SAVE_CHANGES', 'CMD_VIEW_CHANGES', 'CMD_ABORT']
                q.default = 'CMD_VIEW_CHANGES'
                q.options = []
                q.selected = 0
                ans = ''
                arg = None
                while ans != 'CMD_SAVE_CHANGES':
                    ans, arg = q.promptUser()
                    if ans == 'CMD_SAVE_CHANGES':
                        apparmor.write_profile_ui_feedback(program, True)
                        self.reload_profile(filename)
                    elif ans == 'CMD_VIEW_CHANGES':
                        #oldprofile = apparmor.serialize_profile(apparmor.original_aa[program], program, '')
                        newprofile = apparmor.serialize_profile(apparmor.aa[program], program, {'is_attachment': True})
                        aaui.UI_Changes(filename, newprofile, comments=True)
            else:
                apparmor.write_profile_ui_feedback(program, True)
                self.reload_profile(filename)
        else:
            raise apparmor.AppArmorException(_('The profile for %s does not exists. Nothing to clean.') % program)

    def enable_profile(self, filename):
        apparmor.delete_symlink('disable', filename)

    def disable_profile(self, filename):
        apparmor.create_symlink('disable', filename)

    def unload_profile(self, profile):
        if not self.do_reload:
            return

        # FIXME: should ensure profile is loaded before unloading
        cmd_info = cmd([apparmor.parser, '-I%s' % apparmor.profile_dir, '--base', apparmor.profile_dir, '-R', profile])

        if cmd_info[0] != 0:
            raise apparmor.AppArmorException(cmd_info[1])

    def reload_profile(self, profile):
        if not self.do_reload:
            return

        cmd_info = cmd([apparmor.parser, '-I%s' % apparmor.profile_dir, '--base', apparmor.profile_dir, '-r', profile])

        if cmd_info[0] != 0:
            raise apparmor.AppArmorException(cmd_info[1])
