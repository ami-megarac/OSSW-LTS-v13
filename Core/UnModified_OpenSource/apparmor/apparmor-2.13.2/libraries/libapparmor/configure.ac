m4_define([__apparmor_version], m4_sinclude(common/Version))
m4_ifdef(__apparmor_version, , m4_define([__apparmor_version], m4_sinclude(../../common/Version)))
m4_define([__aalen], decr(len(__apparmor_version)))
m4_define([apparmor_version], m4_substr(__apparmor_version, 0, __aalen))

AC_INIT(configure.ac)

AM_INIT_AUTOMAKE(libapparmor1, apparmor_version)

AM_PROG_LEX
AC_PROG_YACC
AC_PROG_SED
PKG_PROG_PKG_CONFIG

AC_PATH_PROG([SWIG], [swig])

AC_MSG_CHECKING([whether the libapparmor debug output should be enabled])
AC_ARG_ENABLE([debug_output],
[AS_HELP_STRING([--enable-debug-output], [generate the libapparmor debug output [[default=no]]])],
[AC_MSG_RESULT([$enableval])],
[enable_debug_output=no]
[AC_MSG_RESULT([$enable_debug_output])])
AS_IF([test "$enable_debug_output" = "yes"], [AC_DEFINE([ENABLE_DEBUG_OUTPUT], [1], [debug output])])

AC_MSG_CHECKING([whether the libapparmor man pages should be generated])
AC_ARG_ENABLE(man_pages,
[AS_HELP_STRING([--enable-man-pages], [generate the libapparmor man pages [[default=yes]]])],
[AC_MSG_RESULT($enableval)],
[enable_man_pages=yes]
[AC_MSG_RESULT($enable_man_pages)])
if test "$enable_man_pages" = "yes"; then
   sinclude(m4/ac_podchecker.m4)
   PROG_PODCHECKER

   sinclude(m4/ac_pod2man.m4)
   PROG_POD2MAN
fi

AC_MSG_CHECKING([whether python bindings are enabled])
AC_ARG_WITH(python,
[  --with-python           enable the python wrapper  [[default=no]]],
[AC_MSG_RESULT($withval)], [AC_MSG_RESULT(no)])
if test "$with_python" = "yes"; then
   test -z "$SWIG" && AC_MSG_ERROR([swig is required when enabling python bindings])
   AC_PATH_PROG(PYTHON, python)
   test -z "$PYTHON" && AC_MSG_ERROR([python is required when enabling python bindings])
   sinclude(m4/ac_python_devel.m4)
   AC_PYTHON_DEVEL
   AM_PATH_PYTHON
fi

AC_MSG_CHECKING([whether perl bindings are enabled])
AC_ARG_WITH(perl,
[  --with-perl             enable the perl wrapper [[default=no]]],
[AC_MSG_RESULT($withval)], [AC_MSG_RESULT(no)])
if test "$with_perl" = "yes"; then
   test -z "$SWIG" && AC_MSG_ERROR([swig is required when enabling perl bindings])
   AC_PATH_PROG(PERL, perl)
   test -z "$PERL" && AC_MSG_ERROR([perl is required when enabling perl bindings])
   perl_includedir="`$PERL -e 'use Config; print $Config{archlib}'`/CORE"
   AC_CHECK_FILE($perl_includedir/perl.h, enable_perl=yes, enable_perl=no)
fi


AC_MSG_CHECKING([whether ruby bindings are enabled])
AC_ARG_WITH(ruby,
[  --with-ruby             enable the ruby wrapper [[default=no]]],
[AC_MSG_RESULT($withval)], [AC_MSG_RESULT(no)])
if test "$with_ruby" = "yes"; then
   test -z "$SWIG" && AC_MSG_ERROR([swig is required when enabling ruby bindings])
   AC_PATH_PROG([RUBY], [ruby])
   test -z "$RUBY" && AC_MSG_ERROR([ruby is required when enabling ruby bindings])
fi


AM_CONDITIONAL(ENABLE_MAN_PAGES, test x$enable_man_pages = xyes)
AM_CONDITIONAL(HAVE_PYTHON, test x$with_python = xyes)
AM_CONDITIONAL(HAVE_PERL, test x$with_perl = xyes) 
AM_CONDITIONAL(HAVE_RUBY, test x$with_ruby = xyes)

AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h stdint.h syslog.h)

AC_CHECK_FUNCS([asprintf __secure_getenv secure_getenv reallocarray])

AM_PROG_CC_C_O
AC_C_CONST
AM_PROG_LIBTOOL

AC_PROG_CC_C99
if test "$ac_cv_prog_cc_c99" = "no"; then
  AC_MSG_ERROR([C99 mode is required to build libapparmor])
fi

AC_OUTPUT(
Makefile
doc/Makefile
src/Makefile
swig/Makefile
swig/perl/Makefile
swig/perl/Makefile.PL
swig/python/Makefile
swig/python/setup.py
swig/python/test/Makefile
swig/ruby/Makefile
testsuite/Makefile
testsuite/config/Makefile
testsuite/libaalogparse.test/Makefile
testsuite/lib/Makefile
include/Makefile
include/sys/Makefile
)
