INCLUDES = $(all_includes)

# variables to set the library versions used by libtool
# Step through these rules IN ORDER to update the library version.
# 1. Update the version information only immediately before a public release
#    of your software. More frequent updates are unnecessary, and only
#    guarantee that the current interface number gets larger faster.
# 2. If the library source code has changed at all since the last update,
#    then
#    - increment AA_LIB_REVISION
# 3. If any interfaces have been added, removed, or changed since the last
#    update,
#    - increment AA_LIB_CURRENT
#    - set AA_LIB_REVISION to 0.
# 4. If any interfaces have been added since the last public release, then
#    - increment AA_LIB_AGE.
# 5. If any interfaces have been removed or changed since the last public
#    release, then
#    - set AA_LIB_AGE to 0.
#
# The resulting library version triplet will be:
#   (AA_LIB_CURRENT - AA_LIB_AGE).AA_LIB_AGE.AA_LIB_REVISION
# and the SONAME will be:
#   libapparmor.so.(AA_LIB_CURRENT - AA_LIB_AGE)
#
# For more information, see:
# http://www.gnu.org/software/libtool/manual/html_node/Libtool-versioning.html
#
AA_LIB_CURRENT = 7
AA_LIB_REVISION = 0
AA_LIB_AGE = 6

SUFFIXES = .pc.in .pc

BUILT_SOURCES = grammar.h scanner.h af_protos.h
AM_LFLAGS = -v
AM_YFLAGS = -d -p aalogparse_
AM_CFLAGS = -Wall
AM_CPPFLAGS = -D_GNU_SOURCE -I$(top_srcdir)/include/
scanner.h: scanner.l
	$(LEX) -v $<

scanner.c: scanner.l

af_protos.h:
	 echo '#include <netinet/in.h>' | $(CC) -E -dM - | LC_ALL=C  sed  -n -e "/IPPROTO_MAX/d"  -e "s/^\#define[ \\t]\\+IPPROTO_\\([A-Z0-9_]\\+\\)\\(.*\\)$$/AA_GEN_PROTO_ENT(\\UIPPROTO_\\1, \"\\L\\1\")/p" > $@

lib_LTLIBRARIES = libapparmor.la
noinst_HEADERS = grammar.h parser.h scanner.h af_protos.h private.h PMurHash.h

libapparmor_la_SOURCES = grammar.y libaalogparse.c kernel.c scanner.c private.c features.c kernel_interface.c policy_cache.c PMurHash.c
libapparmor_la_LDFLAGS = -version-info $(AA_LIB_CURRENT):$(AA_LIB_REVISION):$(AA_LIB_AGE) -XCClinker -dynamic -pthread \
	-Wl,--version-script=$(top_srcdir)/src/libapparmor.map

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libapparmor.pc

CLEANFILES = libapparmor.pc

%.pc: %.pc.in $(top_builddir)/config.status
	$(AM_V_GEN)cd "$(top_builddir)" && \
	$(SHELL) ./config.status --file="src/$@"

tst_aalogmisc_SOURCES = tst_aalogmisc.c
tst_aalogmisc_LDADD = .libs/libapparmor.a

tst_features_SOURCES = tst_features.c
tst_features_LDADD = .libs/libapparmor.a

tst_kernel_SOURCES = tst_kernel.c
tst_kernel_LDADD = .libs/libapparmor.a
tst_kernel_LDFLAGS = -pthread

check_PROGRAMS = tst_aalogmisc tst_features tst_kernel
TESTS = $(check_PROGRAMS)

EXTRA_DIST = grammar.y scanner.l libapparmor.map libapparmor.pc
