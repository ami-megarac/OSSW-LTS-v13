AC_DEFUN([PROG_POD2MAN],[
   AC_CHECK_PROG(POD2MAN,pod2man,pod2man,no)
   if test "$POD2MAN" = "no"; then
      AC_MSG_ERROR([
The pod2man program was not found in the default path.  pod2man is part of
Perl, which can be retrieved from:

    https://www.perl.org
])
   fi
])
