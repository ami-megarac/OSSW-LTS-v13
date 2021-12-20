# ----------------------------------------------------------------------
#    Copyright (c) 1999, 2004-2009 NOVELL (All rights reserved)
#    Copyright (c) 2010-2016 Canonical Ltd.
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
NAME 		= apparmor-utils
all:
COMMONDIR=../common/

include $(COMMONDIR)/Make.rules

PERLTOOLS = aa-notify
PYTOOLS = aa-easyprof aa-genprof aa-logprof aa-cleanprof aa-mergeprof \
          aa-autodep aa-audit aa-complain aa-enforce aa-disable \
	  aa-status aa-unconfined
TOOLS = ${PERLTOOLS} ${PYTOOLS} aa-decode aa-remove-unknown
PYSETUP = python-tools-setup.py
PYMODULES = $(wildcard apparmor/*.py apparmor/rule/*.py)

MANPAGES = ${TOOLS:=.8} logprof.conf.5

all: docs
	$(MAKE) -C po all
	$(MAKE) -C vim all

.PHONY: docs
docs: ${MANPAGES} ${HTMLMANPAGES}

# need some better way of determining this
DESTDIR=/
BINDIR=${DESTDIR}/usr/sbin
CONFDIR=${DESTDIR}/etc/apparmor
PYPREFIX=/usr

PYFLAKES=pyflakes

po/${NAME}.pot: ${TOOLS} ${PYMODULES}
	$(MAKE) -C po ${NAME}.pot NAME=${NAME} SOURCES="${TOOLS} ${PYMODULES}"

.PHONY: install
install: ${MANPAGES} ${HTMLMANPAGES}
	install -d ${CONFDIR}
	install -m 644 logprof.conf severity.db notify.conf ${CONFDIR}
	install -d ${BINDIR}
	ln -sf aa-status ${BINDIR}/apparmor_status
	# aa-easyprof is installed by python-tools-setup.py
	install -m 755 $(filter-out aa-easyprof, ${TOOLS}) ${BINDIR}
	$(MAKE) -C po install DESTDIR=${DESTDIR} NAME=${NAME}
	$(MAKE) install_manpages DESTDIR=${DESTDIR}
	$(MAKE) -C vim install DESTDIR=${DESTDIR}
	ln -sf aa-status.8 ${DESTDIR}/${MANDIR}/man8/apparmor_status.8
	${PYTHON} ${PYSETUP} install --prefix=${PYPREFIX} --root=${DESTDIR} --version=${VERSION}

.PHONY: clean
ifndef VERBOSE
.SILENT: clean
endif
clean: pod_clean
	rm -f core core.* *.o *.s *.a *~
	$(MAKE) -C po clean
	$(MAKE) -C vim clean
	$(MAKE) -C test clean
	rm -rf staging/ build/
	rm -f apparmor/*.pyc apparmor/rule/*.pyc
	rm -rf apparmor/__pycache__/ apparmor/rule/__pycache__/

# ${CAPABILITIES} is defined in common/Make.rules
.PHONY: check_severity_db
.SILENT: check_severity_db
check_severity_db: /usr/include/linux/capability.h severity.db
	# The sed statement is based on the one in the parser's makefile
	RC=0 ; for cap in ${CAPABILITIES} ; do \
	    if !  grep -q -w $${cap} severity.db ; then \
		echo "Warning! capability $${cap} not found in severity.db" ; \
		RC=1 ; \
	    fi ;\
	done ; \
	test "$$RC" -eq 0

# check_pod_files is defined in common/Make.rules
.PHONY: check
.SILENT: check
check: check_severity_db check_pod_files
	for i in ${PERLTOOLS} ; do \
		perl -c $$i || exit 1; \
	done
	for i in ${PYTOOLS} apparmor test/*.py; do \
		echo Checking $$i; \
		$(PYFLAKES) $$i || exit 1; \
	done
	$(MAKE) -C test check
	$(MAKE) -C vim check
