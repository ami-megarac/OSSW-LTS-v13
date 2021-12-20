# ----------------------------------------------------------------------
#    Copyright (C) 2018 Christian Boltz <apparmor@cboltz.de>
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
from apparmor.common import AppArmorBug, AppArmorException

# setup module translations
from apparmor.translations import init_translation
_ = init_translation()


class ProfileList:
    ''' Stores the list of profiles (both name and attachment) and in which files they live '''

    def __init__(self):
        self.profile_names = {}     # profile name -> filename
        self.attachments = {}       # attachment -> filename
        self.attachments_AARE = {}  # AARE(attachment) -> filename

    def add(self, filename, profile_name, attachment):
        ''' Add the given profile and attachment to the list '''

        if not filename:
            raise AppArmorBug('Empty filename given to ProfileList')

        if not profile_name and not attachment:
            raise AppArmorBug('Neither profile name or attachment given')

        if profile_name in self.profile_names:
            raise AppArmorException(_('Profile %(profile_name)s exists in %(filename)s and %(filename2)s' % {'profile_name': profile_name, 'filename': filename, 'filename2': self.profile_names[profile_name]}))

        if attachment in self.attachments:
            raise AppArmorException(_('Profile for %(profile_name)s exists in %(filename)s and %(filename2)s' % {'profile_name': attachment, 'filename': filename, 'filename2': self.attachments[attachment]}))

        if profile_name:
            self.profile_names[profile_name] = filename

        if attachment:
            self.attachments[attachment] = filename
            self.attachments_AARE[attachment] = AARE(attachment, True)

    def filename_from_profile_name(self, name):
        ''' Return profile filename for the given profile name, or None '''

        return self.profile_names.get(name, None)

    def filename_from_attachment(self, attachment):
        ''' Return profile filename for the given attachment/executable path, or None '''

        if not attachment.startswith( ('/', '@', '{') ):
            raise AppArmorBug('Called filename_from_attachment with non-path attachment: %s' % attachment)

        # plain path
        if self.attachments.get(attachment):
            return self.attachments[attachment]

        # try AARE matches to cover profile names with alternations and wildcards
        for path in self.attachments.keys():
            if self.attachments_AARE[path].match(attachment):
                return self.attachments[path]  # XXX this returns the first match, not necessarily the best one

        return None  # nothing found
