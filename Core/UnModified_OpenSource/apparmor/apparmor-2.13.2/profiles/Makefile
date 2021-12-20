# ------------------------------------------------------------------
#
#    Copyright (C) 2002-2009 Novell/SUSE
#    Copyright (C) 2010-2016 Canonical Ltd.
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
#
# ------------------------------------------------------------------

# Makefile for LSM-based AppArmor profiles

NAME=apparmor-profiles
all: local docs
COMMONDIR=../common/

include $(COMMONDIR)/Make.rules

DESTDIR=/
PROFILES_DEST=${DESTDIR}/etc/apparmor.d
EXTRAS_DEST=${DESTDIR}/usr/share/apparmor/extra-profiles/
PROFILES_SOURCE=./apparmor.d
ABSTRACTIONS_SOURCE=./apparmor.d/abstractions
EXTRAS_SOURCE=./apparmor/profiles/extras/

SUBDIRS=$(shell find ${PROFILES_SOURCE} -type d -print)
TOPLEVEL_PROFILES=$(filter-out ${SUBDIRS}, $(wildcard ${PROFILES_SOURCE}/*))

# $(PWD) is wrong when using "make -C profiles" - explicitely set it here to get the right value
PWD=$(shell pwd)

local:
	for profile in ${TOPLEVEL_PROFILES}; do \
		fn=$$(basename $$profile); \
		echo "# Site-specific additions and overrides for '$$fn'" > ${PROFILES_SOURCE}/local/$$fn; \
		grep "include[[:space:]]\\+<local/$$fn>" "$$profile" >/dev/null || { echo "$$profile doesn't contain #include <local/$$fn>" ; exit 1; } ; \
	done; \

.PHONY: install
install: local
	install -m 755 -d ${PROFILES_DEST}
	install -m 755 -d ${PROFILES_DEST}/disable
	for dir in ${SUBDIRS} ; do \
	    	install -m 755 -d "${PROFILES_DEST}/$${dir#${PROFILES_SOURCE}}" ; \
	done
	for file in $$(find ${PROFILES_SOURCE} -type f -print) ; do \
	    	install -m 644 "$${file}" "${PROFILES_DEST}/$$(dirname $${file#${PROFILES_SOURCE}})" ; \
	done
	install -m 755 -d ${EXTRAS_DEST}
	install -m 644 ${EXTRAS_SOURCE}/* ${EXTRAS_DEST}

LOCAL_ADDITIONS=$(filter-out ${PROFILES_SOURCE}/local/README, $(wildcard ${PROFILES_SOURCE}/local/*))
.PHONY: clean
clean:
	-rm -f ${LOCAL_ADDITIONS}

ifndef VERBOSE
  Q=@
else
  Q=
endif

ifndef PARSER
# use system parser
PARSER=../parser/apparmor_parser
endif

ifndef LOGPROF
# use ../utils logprof
LOGPROF=PYTHONPATH=../utils $(PYTHON) ../utils/aa-logprof
endif

.PHONY: docs
# docs: should we have some here?
docs:

IGNORE_FILES=${EXTRAS_SOURCE}/README
CHECK_PROFILES=$(filter-out ${IGNORE_FILES} ${SUBDIRS}, $(wildcard ${PROFILES_SOURCE}/*) $(wildcard ${EXTRAS_SOURCE}/*))
# use find because Make wildcard is not recursive:
CHECK_ABSTRACTIONS=$(shell find ${ABSTRACTIONS_SOURCE} -type f -print)

.PHONY: check
check: check-parser check-logprof

.PHONY: check-parser
check-parser: local
	@echo "*** Checking profiles from ${PROFILES_SOURCE} and ${EXTRAS_SOURCE} against apparmor_parser"
	$(Q)for profile in ${CHECK_PROFILES} ; do \
	        [ -n "${VERBOSE}" ] && echo "Testing $${profile}" ; \
		${PARSER} --config-file=../parser/tst/parser.conf -S -b ${PWD}/apparmor.d $${profile} > /dev/null || exit 1; \
	done

	@echo "*** Checking abstractions from ${ABSTRACTIONS_SOURCE} against apparmor_parser"
	$(Q)for abstraction in ${CHECK_ABSTRACTIONS} ; do \
	        [ -n "${VERBOSE}" ] && echo "Testing $${abstraction}" ; \
		echo "#include <tunables/global> profile test { #include <$${abstraction}> }" \
		| ${PARSER} --config-file=../parser/tst/parser.conf -S -b ${PWD}/apparmor.d -I ${PWD} > /dev/null \
		|| exit 1; \
	done

.PHONY: check-logprof
check-logprof: local
	@echo "*** Checking profiles from ${PROFILES_SOURCE} against logprof"
	$(Q)${LOGPROF} -d ${PROFILES_SOURCE} -f /dev/null || exit 1
