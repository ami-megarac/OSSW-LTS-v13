# ----------------------------------------------------------------------
#    Copyright (c) 2004, 2005 NOVELL (All rights reserved)
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
# ----------------------------------------------------------------------
all:

# As translations get added, they will automatically be included, unless
# the lang is explicitly added to DISABLED_LANGS; e.g. DISABLED_LANGS=en es

DISABLED_LANGS=

COMMONDIR=../../common
include $(COMMONDIR)/Make-po.rules

XGETTEXT_ARGS+=--language=C --keyword=_ $(shell if [ -f ${NAME}.pot ] ; then echo -n -j ; fi)

