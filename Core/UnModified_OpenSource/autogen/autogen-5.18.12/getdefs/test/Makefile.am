##  -*- Mode: Makefile -*-
##
## Makefile.am -- process this file with automake to produce Makefile.in
##
## Author:          Bruce Korb <bkorb@gnu.org>
##
## This file is part of AutoGen.
## AutoGen Copyright (C) 1992-2015 by Bruce Korb - all rights reserved
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

TESTS      = cfg.test index.test option.test subblock.test
EXTRA_DIST = defs $(TESTS)

TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) \
	srcdir=$(srcdir) top_builddir=$(top_builddir) \
	GDexe=$(GDexe) AGexe=$(AGexe) CLexe=$(CLexe)

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
