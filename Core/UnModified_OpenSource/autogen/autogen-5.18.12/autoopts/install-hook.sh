#! /bin/sh

##  This file is part of AutoOpts, a companion to AutoGen.
##  AutoOpts is free software.
##  AutoOpts is Copyright (C) 1992-2016 by Bruce Korb - all rights reserved
##
##  AutoOpts is available under either of two licenses.  The license
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

egrep '#undef +AUTOOPTS_ENABLED' ${top_builddir}/config.h >/dev/null && \
    exit 0

srcdir=`dirname $0`
srcdir=`cd ${srcdir} ; pwd`

. ${top_builddir}/config/shdefs

rm -f ${DESTdestdir}/options.h ${DESTdestdir}/usage-txt.h
opthdrsrc=${srcdir}/autoopts/options.h
usehdrsrc=${srcdir}/autoopts/usage-txt.h
cfgf=${top_builddir}/config.h

emit_options_h() {
    sed '/^#include <stdio/q' ${opthdrsrc}

    if ${EGREP} 'define +HAVE_STDINT_H' ${cfgf} >/dev/null
    then echo '#include <stdint.h>'
    else echo '#include <inttypes.h>' ; fi

    if ${EGREP} 'define +HAVE_LIMITS_H' ${cfgf} >/dev/null
    then echo '#include <limits.h>'
    else echo '#include <sys/limits.h>' ; fi

    if ${EGREP} 'define +HAVE_STDBOOL_H' ${cfgf} >/dev/null
    then echo '#include <stdbool.h>'
    else cat <<- _EOF_
	typedef enum { false = 0, true = 1 } _Bool;
	#define bool   _Bool
	#define true   1
	#define false  0
	_EOF_
    fi

    ${EGREP} 'define +NO_OPTIONAL_OPT_ARGS' ${cfgf}

    if ${EGREP} 'define +HAVE_INTPTR_T' ${cfgf} >/dev/null
    then :
    else
        sizeof_charp=`${EGREP} 'define +SIZEOF_CHARP ' ${cfgf} | \
            sed 's/.*SIZEOF_CHARP *//'`
        sizeof_long=` ${EGREP} 'define +SIZEOF_LONG '  ${cfgf} | \
            sed 's/.*SIZEOF_LONG *//'`
        if test "X${sizeof_charp}" = "X${sizeof_long}"
        then ptrtype=long
        else ptrtype=int
        fi
        cat <<- _EOF_
		#ifndef HAVE_UINTPTR_T
		#define HAVE_UINTPTR_T 1
		#define HAVE_INTPTR_T  1
		typedef $ptrtype intptr_t;
		typedef unsigned $ptrtype uintptr_t;
		#endif /* HAVE_UINTPTR_T */
		_EOF_
    fi

    sedcmd='1,/END-CONFIGURED-HEADERS/d'

    if ${EGREP} 'define +HAVE_PATHFIND' ${cfgf} >/dev/null
    then nopathfind='/From:.*pathfind\.c/,/#endif.*HAVE_PATHFIND/d'
    else nopathfind='/HAVE_PATHFIND/d' ; fi

    sed "${sedcmd};${nopathfind}" ${opthdrsrc}
}
emit_options_h > ${DESTdestdir}/options.h

sed '/#if.*AUTOOPTS_INTERNAL/,/#endif.*AUTOOPTS_INTERNAL/d' \
  ${usehdrsrc} > ${DESTdestdir}/usage-txt.h

test -d "${DESTpkgdatadir}" && {
    rmbuild='/# *START-BUILDTREE-ISMS/,/# *END-BUILDTREE-ISMS/d
	/# *END-INSTALL-ONLY-CODE/d
	/^##/d'

    cd ${DESTpkgdatadir}
    for f in *
    do  case "$f" in
        agtexi-cmd.tpl | cmd-doc.tlib | def2pot.tpl   | \
        getopt.tpl     | options.tpl  | str2init.tlib | usage.tlib)
            sed "${rmbuild}" $f > $f.tmp
            mv -f $f.tmp $f
            ;;

        tpl-config.tlib )
            sed '/define top-[a-z]*-dir/d' $f > $f.tmp
            mv -f $f.tmp $f
            test -n "${DESTlibdatadir}" && {
                test -d ${DESTlibdatadir} || \
                    mkdir -p ${DESTlibdatadir} || \
                    continue

                cp $f ${DESTlibdatadir}/.
            }
            ;;

        *.* ) : ;;
        * )
            chmod a+x $f
            ;;
        esac
    done
}

## Local Variables:
## Mode: shell-script
## indent-tabs-mode:       nil
## sh-basic-offset:          4
## sh-indent-after-do:       4
## sh-indentation:           4
## sh-indent-for-case-label: 0
## sh-indent-for-case-alt:   4
## End:

# end of install-hook.sh
