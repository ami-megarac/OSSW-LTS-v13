##  -*- Mode: Makefile -*-
##
## Makefile.am -- process this file with automake to produce Makefile.in
##
## Author:          Bruce Korb <bkorb@gnu.org>
##
## This file is part of AutoGen.
## AutoGen Copyright (C) 1992-2016 by Bruce Korb - all rights reserved
##
## AutoGen is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by the
## Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## AutoGen is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
## See the GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program.  If not, see <http://www.gnu.org/licenses/>.

SHELL_TESTS = define.test directives.test error.test expr.test extract.test \
	include.test opts.test output.test snarf.test suffix.test shell.test

NOSHELL_TESTS = return.test \
    alist.test  case.test   columns.test  debug.test    defref.test \
    dynref.test endmac.test for.test      forfrom.test  forin.test  \
    format.test get.test    gperf.test    heredef.test  html.test   \
    in.test     leave.test  license.test  line.test     loop.test   \
    make.test   match.test  pseudo.test   reorder.test  stack.test  \
    stress.test string.test strtable.test strxform.test time.test   \
    str2m.test

UNREADY_TESTS   = daemon.test
TESTS       	= @AGEN5_TESTS@
EXTRA_DIST  	= $(TESTS) daemon.test
FUNCLIST    	= funcCase.c funcDef.c funcEval.c funcFor.c funcIf.c functions.c
EXPRLIST    	= expFormat.c expGuile.c expOutput.c  expPrint.c  expState.c \
	    	  expString.c expGperf.c expExtract.c $(FUNCLIST)

TESTS_ENVIRONMENT = TERM='' \
	CFLAGS='$(CFLAGS)' \
	COMPILE='$(COMPILE)' \
	EXPRLIST="$(EXPRLIST)" \
	LDFLAGS='$(LDFLAGS)' \
	SHELL="$(POSIX_SHELL)" \
	srcdir="$(srcdir)" \
	top_builddir="$(top_builddir)" \
	top_srcdir="$(top_srcdir)"

distclean-local:
	-rm -rf *-testd FAILURES defs

$(TESTS)    : defs

defs        : ${top_builddir}/autoopts/test/defs
	$(TESTS_ENVIRONMENT) $(SHELL) \
		$(top_srcdir)/config/install-defs.sh \
			$(top_builddir)/autoopts/test/defs

${top_builddir}/autoopts/test/defs :
	cd ${top_builddir}/autoopts/test ; $(MAKE) defs

verbose     : defs
	rm -rf FAILURES *-testd ; \
	VERBOSE=true $(MAKE) check TESTS="$(TESTS)"

# end of Makefile.am
