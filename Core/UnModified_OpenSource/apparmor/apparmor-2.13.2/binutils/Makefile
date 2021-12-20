# ----------------------------------------------------------------------
#    Copyright (c) 2015
#    Canonical Ltd. (All rights reserved)
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# ----------------------------------------------------------------------
NAME=aa-binutils
all:
COMMONDIR=../common/

include $(COMMONDIR)/Make.rules

DESTDIR=/
BINDIR=${DESTDIR}/usr/bin
LOCALEDIR=/usr/share/locale
MANPAGES=aa-enabled.1 aa-exec.1

WARNINGS = -Wall
EXTRA_WARNINGS = -Wsign-compare -Wmissing-field-initializers -Wformat-security -Wunused-parameter
CPP_WARNINGS =
ifndef CFLAGS
CFLAGS	= -g -O2 -pipe

ifdef DEBUG
CFLAGS += -pg -D DEBUG
endif
ifdef COVERAGE
CFLAGS = -g -pg -fprofile-arcs -ftest-coverage
endif
endif #CFLAGS

EXTRA_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${EXTRA_CXXFLAGS} ${CPP_WARNINGS}

#INCLUDEDIR = /usr/src/linux/include
INCLUDEDIR =

ifdef INCLUDEDIR
	CFLAGS += -I$(INCLUDEDIR)
endif

# Internationalization support. Define a package and a LOCALEDIR
EXTRA_CFLAGS+=-DPACKAGE=\"${NAME}\" -DLOCALEDIR=\"${LOCALEDIR}\"

SRCS = aa_enabled.c
HDRS =
TOOLS = aa-enabled aa-exec

AALIB = -Wl,-Bstatic -lapparmor  -Wl,-Bdynamic -lpthread

ifdef USE_SYSTEM
  # Using the system libapparmor so Makefile dependencies can't be used
  LIBAPPARMOR_A =
  INCLUDE_APPARMOR =
  APPARMOR_H =
  LIBAPPARMOR_LDFLAGS =
else
  LIBAPPARMOR_SRC = ../libraries/libapparmor/
  LOCAL_LIBAPPARMOR_INCLUDE = $(LIBAPPARMOR_SRC)/include
  LOCAL_LIBAPPARMOR_LDPATH = $(LIBAPPARMOR_SRC)/src/.libs

  LIBAPPARMOR_A = $(LOCAL_LIBAPPARMOR_LDPATH)/libapparmor.a
  INCLUDE_APPARMOR = -I$(LOCAL_LIBAPPARMOR_INCLUDE)
  APPARMOR_H = $(LOCAL_LIBAPPARMOR_INCLUDE)/sys/apparmor.h
  LIBAPPARMOR_LDFLAGS = -L$(LOCAL_LIBAPPARMOR_LDPATH)
endif
EXTRA_CFLAGS += $(INCLUDE_APPARMOR)
LDFLAGS += $(LIBAPPARMOR_LDFLAGS)

ifdef V
  VERBOSE = 1
endif
ifndef VERBOSE
  VERBOSE = 0
endif
ifeq ($(VERBOSE),1)
  BUILD_OUTPUT =
  Q =
else
  BUILD_OUTPUT = > /dev/null 2>&1
  Q = @
endif
export Q VERBOSE BUILD_OUTPUT

po/%.pot: %.c
	$(MAKE) -C po $(@F) NAME=$* SOURCES=$*.c

# targets arranged this way so that people who don't want full docs can
# pick specific targets they want.
arch: 	$(TOOLS)

manpages:	$(MANPAGES)

docs:	manpages

indep: docs
	$(Q)$(MAKE) -C po all

all:	arch indep

.PHONY: coverage
coverage:
	$(MAKE) clean $(TOOLS) COVERAGE=1

ifndef USE_SYSTEM
$(LIBAPPARMOR_A):
	@if [ ! -f $@ ]; then \
		echo "error: $@ is missing. Pick one of these possible solutions:" 1>&2; \
		echo "  1) Build against the in-tree libapparmor by building it first and then trying again. See the top-level README for help." 1>&2; \
		echo "  2) Build against the system libapparmor by adding USE_SYSTEM=1 to your make command." 1>&2;\
		exit 1; \
	fi
endif

aa-enabled: aa_enabled.c $(LIBAPPARMOR_A)
	$(CC) $(LDFLAGS) $(EXTRA_CFLAGS) -o $@ $< $(LIBS) $(AALIB) 

aa-exec: aa_exec.c $(LIBAPPARMOR_A)
	$(CC) $(LDFLAGS) $(EXTRA_CFLAGS) -o $@ $< $(LIBS) $(AALIB)

.SILENT: check
.PHONY: check
check: check_pod_files tests

.SILENT: tests
tests: $(TOOLS) $(TESTS)
	echo "no tests atm"

.PHONY: install
install: install-indep install-arch

.PHONY: install-arch
install-arch: arch
	install -m 755 -d ${BINDIR}
	install -m 755 ${TOOLS} ${BINDIR}

.PHONY: install-indep
install-indep: indep
	$(MAKE) -C po install NAME=${NAME} DESTDIR=${DESTDIR}
	$(MAKE) install_manpages DESTDIR=${DESTDIR}

ifndef VERBOSE
.SILENT: clean
endif
.PHONY: clean
clean: pod_clean
	rm -f core core.* *.o *.s *.a *~ *.gcda *.gcno
	rm -f gmon.out
	rm -f $(TOOLS) $(TESTS)
	$(MAKE) -s -C po clean

