SUBDIRS = lib config libaalogparse.test
PACKAGE = libaalogparse
AUTOMAKE_OPTIONS = dejagnu

INCLUDES = -I. -I$(top_srcdir)/include

AM_CPPFLAGS = $(DEBUG_FLAGS) -DLOCALEDIR=\"${localedir}\"
AM_CFLAGS = -Wall

noinst_PROGRAMS = test_multi.multi

test_multi_multi_SOURCES	= test_multi.c
test_multi_multi_CFLAGS		= -Wall
test_multi_multi_LDADD		= -L../src/.libs -lapparmor

clean-local:
	rm -rf tmp.err.* tmp.out.* site.exp site.bak test_multi/out
	rm -f libaalogparse.log libaalogparse.sum

check-local: check-DEJAGNU
	@if ! test -f libaalogparse.log ; then echo '*** libaalogparse.log not found - is dejagnu installed? ***'; exit 1; fi
	@if grep ERROR libaalogparse.log ; then exit 1 ; fi

EXTRA_DIST = test_multi/*.in test_multi/*.out test_multi/*.err
