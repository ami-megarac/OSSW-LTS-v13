#! /bin/sh

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

prog=`basename $0 .sh`

die() {
    echo "$prog failure:  $*"
    kill -TERM $progpid
    sleep 1
    exit 1
}

init() {
    PS4='>tpc-${FUNCNAME}> '
    progpid=$$
    prog=`basename $0`
    progdir=`\cd \`dirname $0\` >/dev/null ; pwd`
    readonly progpid progdir prog

    for d in top_srcdir srcdir top_builddir builddir
    do
        eval v=\${$d}
        test -d "$v" || die "$d does not reference a directory"
        v=`cd $v >/dev/null && pwd`
        eval ${d}=${v}
    done
    . ${top_builddir}/config/shdefs
}

collect_src() {
    exec 8>&1 1>&2
    cd ${builddir}
    sentinel_file=${1} ; shift
    cat 1>&8 <<- _EOF_
	#define  AUTOOPTS_INTERNAL 1
	#include "autoopts/project.h"
	#define  LOCAL static
	#include "ao-strs.h"
	static char const ao_ver_string[] =
	    "${AO_CURRENT}:${AO_REVISION}:${AO_AGE}\n";
	_EOF_

    for f in "$@"
    do  test "X$f" = "Xproject.h" || \
            printf '#include "%s"\n' $f
    done 1>&8
}

extension_defines() {
    cd ${builddir}/tpl

    test -f tpl-config.tlib || die "tpl-config.tlib not configured"
    test -f ${top_builddir}/config.h || die "config.h missing"
    ${GREP} 'extension-defines' tpl-config.tlib >/dev/null && return

    txt=`sed -n '/POSIX.*SOURCE/,/does not conform to ANSI C/{
	    /^#/p
	}
	/does not conform to ANSI C/q' ${top_builddir}/config.h`

    {
        sed '/define  *top-build-dir/d;/^;;;/d' tpl-config.tlib
        cat <<- _EOF_
	(define top-build-dir   "`cd ${top_builddir} >/dev/null
		pwd`")
	(define top-src-dir     "`cd ${top_srcdir} >/dev/null
		pwd`")
	(define extension-defines
	   "${txt}") \\=]
	_EOF_
    } > tpl-config.$$
    mv -f  tpl-config.$$  tpl-config.tlib
}

fix_scripts() {
    for f in ${srcdir}/tpl/*.sh ${srcdir}/tpl/*.pl
    do
        d=`basename $f | sed 's/\.[sp][hl]$//'`
        st=`sed 1q $f`

        case "$st" in
        *perl ) echo '#!' `which perl`
                 sed 1d $f
                 ;;

        */sh )   echo '#!' ${POSIX_SHELL}
                 sed 1d $f
                 ;;

        * )      die "Invalid script type: $st"
                 ;;
        esac > $d
        chmod 755 $d
    done

    for f in ${srcdir}/tpl/*.pm
    do
        test -f ${f##*/} && continue
        cp $f ${f##*/}
        chmod 644 ${f##*/}
    done
}

find_shell_prog() {
    case `uname -s` in
    SunOS )
      while : ; do
        POSIX_SHELL=`which bash`
        test -x "${POSIX_SHELL}" && break
        POSIX_SHELL=/usr/xpg4/bin/sh
        test -x "${POSIX_SHELL}" && break
        die "You are hosed.  You are on Solaris and have no usable shell."
      done
      ;;
    esac
}

find_cat_prog() {
    while :
    do
        \unalias -a
        unset -f command cat which
        POSIX_CAT=`which cat`
        test -x "$POSIX_CAT" && break
        POSIX_CAT=`
            PATH=\`command -p getconf CS_PATH\`
            command -v cat `
        test -x "${POSIX_CAT}" && break
        die "cannot locate 'cat' command"
    done

    formats='man mdoc texi'
    for f in $formats
    do
        for g in $formats
        do
            test -f ${f}2${g} || {
                printf "#! ${POSIX_SHELL}\nexec ${POSIX_CAT} "'${1+"$@"}\n' \
                    > ${f}2${g}
                chmod 755 ${f}2${g}
            }
        done
    done
}

scan_cflags() {
    libguiledir=
    while test $# -gt 0
    do
        case "$1" in
        -I )
            test -f "$2/libguile/__scm.h" && {
                libguiledir=$2
                return 0
            }
            ;;
        -I* )
            f=${1#-I}
            test -f "$f/libguile/__scm.h" && {
                libguiledir=$f
                return 0
            }
            ;;
        esac
        shift
    done

    libguiledir=/usr/include
    test -f $libguiledir/libguile/__scm.h && return 0
    die "The Guile header __scm.h cannot be found"
}

find_libguiledir() {
    guile_scm_h=
    libguiledir=`exec 2>/dev/null ; guile-config info includedir`

    if test -d "${libguiledir}"
    then
        test -f ${libguiledir}/libguile.h || {
            set -- ${libguiledir}/*guile*/.
            if test -d "${2}"
            then libguiledir=${2%/.}
            elif test -d "$1"
            then libguiledir=${1%/.}
            fi
        }

        v=`guile-config --version 2>&1 | sed 's/.* version //'`
        test -d ${libguiledir}/${v%.*} && v=${v%.*}
        test -d ${libguiledir}/${v} && libguiledir=${libguiledir}/$v

    else
        scan_cflags $*
    fi
    guile_scm_h=`find ${libguiledir} -type f -name __scm.h`
}

fix_guile() {
    cd ${builddir}
    find_libguiledir ${LGCFLAGS}

    list=`exec 2>/dev/null
        find ${libguiledir}/libguile* -type f | \
            xargs grep -l -E '\<noreturn\>'`

    test -z "$list" && exit 0

    test -d libguile || mkdir libguile || {
        echo "cannot make libguile directory"
        exit 1
    } 1>&2

    noret='\([^a-zA-Z0-9_]\)noreturn\([^a-zA-Z0-9_]\)'
    nores='\1__noreturn__\2'
    sedex="s@${noret}@${nores}@"

    for f in $list
    do
        g=libguile${f##*/libguile}
        sed "${sedex}" $f > $g
        diff -u $f $g >&2 || :
    done

    test -f libguile.h || cp ${libguiledir}/libguile.h .
}

init
collect_src "$@" > ${builddir}/libopts.c
extension_defines
fix_scripts
find_shell_prog
find_cat_prog
fix_guile
touch ${sentinel_file}
