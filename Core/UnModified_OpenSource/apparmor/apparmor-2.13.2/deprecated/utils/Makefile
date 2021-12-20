# ----------------------------------------------------------------------
#    Copyright (c) 1999, 2004-2009 NOVELL (All rights reserved)
#    Copyright (c) 2010-2011, 2014 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, contact Novell, Inc.
# ----------------------------------------------------------------------

# NOTE: this Makefile has been adjusted from the original to assist in
# the installation of the Immunix perl modules, if they're still needed
# by users. Because the utilities conflict with their replacments, make
# install *will* *not* install them.

NAME 		= apparmor-utils
all:
COMMONDIR=../../common/

include $(COMMONDIR)/Make.rules

MODDIR = Immunix
PERLTOOLS = aa-genprof aa-logprof aa-autodep aa-audit aa-complain aa-enforce \
	aa-unconfined aa-disable
MODULES = ${MODDIR}/AppArmor.pm ${MODDIR}/Repository.pm \
	${MODDIR}/Config.pm ${MODDIR}/Severity.pm

all:

# need some better way of determining this
DESTDIR=/
BINDIR=${DESTDIR}/usr/sbin
CONFDIR=${DESTDIR}/etc/apparmor
VENDOR_PERL=$(shell perl -e 'use Config; print $$Config{"vendorlib"};')
PERLDIR=${DESTDIR}${VENDOR_PERL}/${MODDIR}

.PHONY: install
install:
	install -d ${PERLDIR}
	install -m 644 ${MODULES} ${PERLDIR}

.PHONY: clean
ifndef VERBOSE
.SILENT: clean
endif
clean: pod_clean
	rm -f core core.* *.o *.s *.a *~
	rm -rf staging/ build/

.PHONY: check
.SILENT: check
check:
	for i in ${MODULES} ${PERLTOOLS} ; do \
		perl -c $$i || exit 1; \
	done
