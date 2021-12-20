# ----------------------------------------------------------------------
#    Copyright (c) 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007
#    NOVELL (All rights reserved)
#
#    Copyright (c) Christian Boltz 2018
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
NAME=apparmor-parser
all:
COMMONDIR=../common/

include $(COMMONDIR)/Make.rules

DESTDIR=/
APPARMOR_BIN_PREFIX=${DESTDIR}/lib/apparmor
SYSTEMD_UNIT_DIR=${DESTDIR}/usr/lib/systemd/system
CONFDIR=/etc/apparmor
INSTALL_CONFDIR=${DESTDIR}${CONFDIR}
LOCALEDIR=/usr/share/locale
MANPAGES=apparmor.d.5 apparmor.7 apparmor_parser.8 subdomain.conf.5 aa-teardown.8

YACC	:= bison
YFLAGS	:= -d
LEX	:= flex
LEXFLAGS = -B -v
WARNINGS = -Wall
EXTRA_WARNINGS = -Wsign-compare -Wmissing-field-initializers -Wformat-security -Wunused-parameter
CXX_WARNINGS = ${WARNINGS} $(shell for warning in ${EXTRA_WARNINGS} ; do \
			if ${CXX} $${warning} -S -o /dev/null -xc /dev/null >/dev/null 2>&1; then \
				echo "$${warning}"; \
			fi ; \
		done)
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

EXTRA_CXXFLAGS = ${CFLAGS} ${CPPFLAGS} ${CXX_WARNINGS} -std=gnu++0x -D_GNU_SOURCE
EXTRA_CFLAGS = ${EXTRA_CXXFLAGS} ${CPP_WARNINGS}

#LEXLIB	:= -lfl

# override this on the make command to point to where the immunix.h file is
# (yeah this is lame, but since we are tied to the kernel so tightly...)
#INCLUDEDIR = /usr/src/linux/include
INCLUDEDIR =

ifdef INCLUDEDIR
	CFLAGS += -I$(INCLUDEDIR)
endif

# Internationalization support. Define a package and a LOCALEDIR
EXTRA_CFLAGS+=-DPACKAGE=\"${NAME}\" -DLOCALEDIR=\"${LOCALEDIR}\"

# Compile-time configuration of the location of the config file
EXTRA_CFLAGS+=-DSUBDOMAIN_CONFDIR=\"${CONFDIR}\"

SRCS = parser_common.c parser_include.c parser_interface.c parser_lex.c \
       parser_main.c parser_misc.c parser_merge.c parser_symtab.c \
       parser_yacc.c parser_regex.c parser_variable.c parser_policy.c \
       parser_alias.c common_optarg.c lib.c network.c \
       mount.cc dbus.cc profile.cc rule.cc signal.cc ptrace.cc \
       af_rule.cc af_unix.cc policy_cache.c
HDRS = parser.h parser_include.h immunix.h mount.h dbus.h lib.h profile.h \
       rule.h common_optarg.h signal.h ptrace.h network.h af_rule.h af_unix.h \
       policy_cache.h
TOOLS = apparmor_parser

OBJECTS = $(patsubst %.cc, %.o, $(SRCS:.c=.o))

AAREDIR= libapparmor_re
AAREOBJECT = ${AAREDIR}/libapparmor_re.a
AAREOBJECTS = $(AAREOBJECT)
AARE_LDFLAGS = -static-libgcc -static-libstdc++ -L. $(LDFLAGS)
AALIB = -Wl,-Bstatic -lapparmor -Wl,-Bdynamic -lpthread

ifdef USE_SYSTEM
  # Using the system libapparmor so Makefile dependencies can't be used
  LIBAPPARMOR_A =
  INCLUDE_APPARMOR =
  APPARMOR_H =
else
  LIBAPPARMOR_SRC = ../libraries/libapparmor
  LOCAL_LIBAPPARMOR_INCLUDE = $(LIBAPPARMOR_SRC)/include
  LOCAL_LIBAPPARMOR_LDPATH = $(LIBAPPARMOR_SRC)/src/.libs

  LIBAPPARMOR_A = $(LOCAL_LIBAPPARMOR_LDPATH)/libapparmor.a
  INCLUDE_APPARMOR = -I$(LOCAL_LIBAPPARMOR_INCLUDE)
  AARE_LDFLAGS += -L$(LOCAL_LIBAPPARMOR_LDPATH)
  APPARMOR_H = $(LOCAL_LIBAPPARMOR_INCLUDE)/sys/apparmor.h
endif
EXTRA_CFLAGS += $(INCLUDE_APPARMOR)

LEX_C_FILES	= parser_lex.c
YACC_C_FILES	= parser_yacc.c parser_yacc.h

TESTS = tst_regex tst_misc tst_symtab tst_variable tst_lib
TEST_CFLAGS = $(EXTRA_CFLAGS) -DUNIT_TEST -Wno-unused-result
TEST_OBJECTS = $(filter-out \
			parser_lex.o \
			parser_yacc.o \
			common_optarg.o \
			parser_main.o \
			policy_cache.o, ${OBJECTS}) \
               $(AAREOBJECTS)
TEST_LDFLAGS = $(AARE_LDFLAGS)
TEST_LDLIBS = $(AALIB)

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

po/${NAME}.pot: ${SRCS} ${HDRS}
	$(MAKE) -C po ${NAME}.pot NAME=${NAME} SOURCES="${SRCS} ${HDRS}"

techdoc.pdf: techdoc.tex
	timestamp=$(shell date --utc "+%Y%m%d%H%M%S%z" -r $< );\
	while pdflatex "\def\fixedpdfdate{$$timestamp}\input $<" ${BUILD_OUTPUT} || exit 1 ; \
		grep -q "Label(s) may have changed" techdoc.log; \
	do :; done

techdoc/index.html: techdoc.pdf
	latex2html -show_section_numbers -split 0 -noinfo -nonavigation -noaddress techdoc.tex ${BUILD_OUTPUT}

techdoc.txt: techdoc/index.html
	w3m -dump $< > $@

# targets arranged this way so that people who don't want full docs can
# pick specific targets they want.
arch: 	$(TOOLS)

manpages:	$(MANPAGES)

htmlmanpages:	$(HTMLMANPAGES)

pdf:	techdoc.pdf

docs:	manpages htmlmanpages
extra_docs: pdf

indep: docs
	$(Q)$(MAKE) -C po all

all:	arch indep

.PHONY: coverage
coverage:
	$(MAKE) clean apparmor_parser COVERAGE=1

ifndef USE_SYSTEM
$(LIBAPPARMOR_A):
	@if [ ! -f $@ ]; then \
		echo "error: $@ is missing. Pick one of these possible solutions:" 1>&2; \
		echo "  1) Build against the in-tree libapparmor by building it first and then trying again. See the top-level README for help." 1>&2; \
		echo "  2) Build against the system libapparmor by adding USE_SYSTEM=1 to your make command." 1>&2;\
		exit 1; \
	fi
endif

apparmor_parser: $(OBJECTS) $(AAREOBJECTS) $(LIBAPPARMOR_A)
	$(CXX) $(LDFLAGS) $(EXTRA_CFLAGS) -o $@ $(OBJECTS) $(LIBS) \
	      ${LEXLIB}  $(AAREOBJECTS) $(AARE_LDFLAGS) $(AALIB)

parser_yacc.c parser_yacc.h: parser_yacc.y parser.h profile.h
	$(YACC) $(YFLAGS) -o parser_yacc.c parser_yacc.y

parser_lex.c: parser_lex.l parser_yacc.h parser.h profile.h mount.h dbus.h policy_cache.h
	$(LEX) ${LEXFLAGS} -o$@ $<

parser_lex.o: parser_lex.c parser.h parser_yacc.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_misc.o: parser_misc.c parser.h parser_yacc.h profile.h cap_names.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_yacc.o: parser_yacc.c parser_yacc.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_main.o: parser_main.c parser.h parser_version.h policy_cache.h libapparmor_re/apparmor_re.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_interface.o: parser_interface.c parser.h profile.h libapparmor_re/apparmor_re.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_include.o: parser_include.c parser.h parser_include.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_merge.o: parser_merge.c parser.h profile.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_regex.o: parser_regex.c parser.h profile.h libapparmor_re/apparmor_re.h libapparmor_re/aare_rules.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_symtab.o: parser_symtab.c parser.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_variable.o: parser_variable.c parser.h profile.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_policy.o: parser_policy.c parser.h parser_yacc.h profile.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_alias.o: parser_alias.c parser.h profile.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_common.o: parser_common.c parser.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

mount.o: mount.cc mount.h parser.h immunix.h rule.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

common_optarg.o: common_optarg.c common_optarg.h parser.h libapparmor_re/apparmor_re.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

policy_cache.o: policy_cache.c policy_cache.h parser.h lib.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

lib.o: lib.c lib.h parser.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

dbus.o: dbus.cc dbus.h parser.h immunix.h parser_yacc.h rule.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

signal.o: signal.cc signal.h parser.h immunix.h parser_yacc.h rule.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

ptrace.o: ptrace.cc ptrace.h parser.h immunix.h parser_yacc.h rule.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

network.o: network.c network.h parser.h immunix.h parser_yacc.h rule.h af_names.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

af_rule.o: af_rule.cc af_rule.h network.h parser.h profile.h immunix.h parser_yacc.h rule.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

af_unix.o: af_unix.cc af_unix.h network.h af_rule.h parser.h profile.h immunix.h parser_yacc.h $(APPARMOR_H)
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

profile.o: profile.cc profile.h parser.h network.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

rule.o: rule.cc rule.h policydb.h
	$(CXX) $(EXTRA_CFLAGS) -c -o $@ $<

parser_version.h: Makefile
	@echo \#define PARSER_VERSION \"$(VERSION)\" > .ver
	@mv -f .ver $@

# af_names and capabilities generation has moved to common/Make.rules,
# as well as the filtering that occurs for network protocols that
# apparmor should not mediate.

.PHONY: af_names.h
af_names.h:
	echo "$(AF_NAMES)" | LC_ALL=C sed -n -e 's/[ \t]\?AF_MAX[ \t]\+[0-9]\+,//g'  -e 's/[ \t]\+\?AF_\([A-Z0-9_]\+\)[ \t]\+\([0-9]\+\),/#ifndef AF_\1\n#  define AF_\1 \2\n#endif\nAA_GEN_NET_ENT("\L\1", \UAF_\1)\n\n/pg' > $@
	echo "$(AF_NAMES)" | LC_ALL=C sed -n -e 's/.*,[ \t]\+AF_MAX[ \t]\+\([0-9]\+\),\?.*/#define AA_AF_MAX \1\n/p' >> $@
	# cat $@

cap_names.h: /usr/include/linux/capability.h
	echo "$(CAPABILITIES)" | LC_ALL=C sed -n -e "s/[ \\t]\\?CAP_\\([A-Z0-9_]\\+\\)/\{\"\\L\\1\", \\UCAP_\\1\},\\n/pg" > $@

tst_lib: lib.c parser.h $(filter-out lib.o, ${TEST_OBJECTS})
	$(CXX) $(TEST_CFLAGS) -o $@ $< $(filter-out $(<:.c=.o), ${TEST_OBJECTS}) $(TEST_LDFLAGS) $(TEST_LDLIBS)
tst_%: parser_%.c parser.h $(filter-out parser_%.o, ${TEST_OBJECTS})
	$(CXX) $(TEST_CFLAGS) -o $@ $< $(filter-out $(<:.c=.o), ${TEST_OBJECTS}) $(TEST_LDFLAGS) $(TEST_LDLIBS)

.SILENT: check
.PHONY: check
check: check_pod_files tests

.SILENT: tests
tests: apparmor_parser ${TESTS}
	sh -e -c 'for test in ${TESTS} ; do echo "*** running $${test}" && ./$${test}; done'
	$(Q)$(MAKE) -s -C tst tests

# always need to rebuild.
.SILENT: $(AAREOBJECT)
.PHONY: $(AAREOBJECT)
$(AAREOBJECT):
	$(MAKE) -C $(AAREDIR) CFLAGS="$(EXTRA_CXXFLAGS)"

.PHONY: install-rhel4
install-rhel4: install-redhat

.PHONY: install-redhat
install-redhat:
	install -m 755 -d $(DESTDIR)/etc/init.d
	install -m 755 rc.apparmor.$(subst install-,,$@) $(DESTDIR)/etc/init.d/apparmor

.PHONY: install-suse
install-suse: install-systemd
	install -m 755 -d $(DESTDIR)/sbin
	ln -sf service $(DESTDIR)/sbin/rcapparmor

.PHONY: install-slackware
install-slackware:
	install -m 755 -d $(APPARMOR_BIN_PREFIX)/install
	install -m 755 frob_slack_rc $(APPARMOR_BIN_PREFIX)/install
	install -m 755 -d $(DESTDIR)/etc/rc.d
	install -m 755 rc.apparmor.$(subst install-,,$(@)) $(DESTDIR)/etc/rc.d/rc.apparmor

.PHONY: install-debian
install-debian:

.PHONY: install-unknown
install-unknown:

INSTALLDEPS=arch

ifndef DISTRO
DISTRO=$(shell if [ -f /etc/slackware-version ] ; then \
	         echo slackware ; \
	       elif [ -f /etc/debian_version ] ; then \
	         echo debian ;\
	       elif which rpm > /dev/null ; then \
	         if [ "$(rpm --eval '0%{?suse_version}')" != "0" ] ; then \
	             echo suse ;\
	         elif [ "$(rpm --eval '%{_host_vendor}')" = redhat ] ; then \
	            echo rhel4 ;\
	         elif [ "$(rpm --eval '0%{?fedora}')" != "0" ] ; then \
	            echo rhel4 ;\
	         else \
	            echo unknown ;\
	         fi ;\
	       else \
	          echo unknown ;\
	       fi)
endif

ifdef DISTRO
INSTALLDEPS+=install-$(DISTRO)
endif

.PHONY: install
install: install-indep install-arch

.PHONY: install-arch
install-arch: $(INSTALLDEPS)
	install -m 755 -d $(DESTDIR)/sbin
	install -m 755 ${TOOLS} $(DESTDIR)/sbin

.PHONY: install-indep
install-indep: indep
	install -m 755 -d $(INSTALL_CONFDIR)
	install -m 644 subdomain.conf $(INSTALL_CONFDIR)
	install -m 644 parser.conf $(INSTALL_CONFDIR)
	install -m 755 -d ${DESTDIR}/var/lib/apparmor
	install -m 755 -d $(APPARMOR_BIN_PREFIX)
	install -m 755 rc.apparmor.functions $(APPARMOR_BIN_PREFIX)
	$(MAKE) -C po install NAME=${NAME} DESTDIR=${DESTDIR}
	$(MAKE) install_manpages DESTDIR=${DESTDIR}

.PHONY: install-systemd
install-systemd:
	install -m 755 -d $(SYSTEMD_UNIT_DIR)
	install -m 644 apparmor.service $(SYSTEMD_UNIT_DIR)
	install -m 755 apparmor.systemd $(APPARMOR_BIN_PREFIX)
	install -m 755 -d $(DESTDIR)/usr/sbin
	install -m 755 aa-teardown $(DESTDIR)/usr/sbin

ifndef VERBOSE
.SILENT: clean
endif
.PHONY: clean
clean: pod_clean
	rm -f core core.* *.o *.s *.a *~ *.gcda *.gcno
	rm -f gmon.out
	rm -f $(TOOLS) $(TESTS)
	rm -f $(LEX_C_FILES)
	rm -f $(YACC_C_FILES)
	rm -f parser_version.h
	rm -f $(NAME)*.tar.gz $(NAME)*.tgz
	rm -f af_names.h
	rm -f cap_names.h
	rm -rf techdoc.aux techdoc.out techdoc.log techdoc.pdf techdoc.toc techdoc.txt techdoc/
	$(MAKE) -s -C $(AAREDIR) clean
	$(MAKE) -s -C po clean
	$(MAKE) -s -C tst clean

