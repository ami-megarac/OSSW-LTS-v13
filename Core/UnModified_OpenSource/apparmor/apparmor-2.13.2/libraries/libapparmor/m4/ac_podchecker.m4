AC_DEFUN([PROG_PODCHECKER],[
   AC_CHECK_PROG(PODCHECKER,podchecker,podchecker,no)
   if test "$PODCHECKER" = "no"; then
      AC_MSG_ERROR([
The podchecker program was not found in the default path.  podchecker is part of
Perl, which can be retrieved from:

    https://www.perl.org
])
   fi
])
