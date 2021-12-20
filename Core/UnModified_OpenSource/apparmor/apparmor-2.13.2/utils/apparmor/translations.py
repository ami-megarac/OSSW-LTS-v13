# ------------------------------------------------------------------
#
#    Copyright (C) 2014 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------
import gettext

TRANSLATION_DOMAIN = 'apparmor-utils'

__apparmor_gettext__ = None

def init_translation():
    global __apparmor_gettext__
    if __apparmor_gettext__ is None:
        t = gettext.translation(TRANSLATION_DOMAIN, fallback=True)
        __apparmor_gettext__ = t.gettext
    return __apparmor_gettext__
