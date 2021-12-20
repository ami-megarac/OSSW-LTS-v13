##  -*- Mode: Makefile -*-
##
## Makefile.am
##
## Author: Bruce Korb <bkorb@gnu.org>
##
##  This file is part of AutoOpts, a companion to AutoGen.
##  AutoOpts is free software.
##  AutoOpts is Copyright (C) 1992-2016 by Bruce Korb - all rights reserved
##
##  AutoOpts is available under any one of two licenses.  The license
##  in use must be one of these two and the choice is under the control
##  of the user of the license.
##
##   The GNU Lesser General Public License, version 3 or later
##      See the files "COPYING.lgplv3" and "COPYING.gplv3"
##
##   The Modified Berkeley Software Distribution License
##      See the file "COPYING.mbsd"
##
##  These files have the following sha256 sums:
##
##  8584710e9b04216a394078dc156b781d0b47e1729104d666658aecef8ee32e95  COPYING.gplv3
##  4379e7444a0e2ce2b12dd6f5a52a27a4d02d39d247901d3285c88cf0d37f477b  COPYING.lgplv3
##  13aa749a5b0a454917a944ed8fffc530b784f5ead522b1aacaf4ec8aa55a6239  COPYING.mbsd

libsrc     = libopts-@AO_CURRENT@.@AO_REVISION@.@AO_AGE@.tar.gz
EXTRA_DIST = defs.in stdopts.def $(TESTS)
TESTS      = \
    alias.test   argument.test cfg-edit.test  cond.test      \
    config.test  doc.test      enums.test     equiv.test     \
    errors.test  getopt.test   handler.test   immediate.test \
    keyword.test library.test  main.test      nested.test    \
    nls.test     rc.test       shell.test     stdopts.test   \
    time.test    usage.test    vendor.test    vers.test

getopt.test : ../$(libsrc)
../$(libsrc):
	cd .. ; echo $(MAKE) $(libsrc) ; $(MAKE) $(libsrc) >/dev/null 2>&1

distclean-local:
	@-rm -rf *-testd FAILURES

check : perm-stamp

perm-stamp :
	@-cd $(srcdir) ; chmod +x *.test 2>/dev/null

verbose : distclean-local
	VERBOSE=true ; export VERBOSE ; \
	$(MAKE) check TESTS="$(TESTS)"

.PHONY : distclean-local verbose perm-stamp

# Makefile.am ends here
