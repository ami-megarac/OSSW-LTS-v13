#! /bin/sh

##  This file is part of AutoGen.
##
##  AutoGen Copyright (C) 1992-2015 by Bruce Korb - all rights reserved
##
##  AutoGen is free software: you can redistribute it and/or modify it
##  under the terms of the GNU General Public License as published by the
##  Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##
##  AutoGen is distributed in the hope that it will be useful, but
##  WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
##  See the GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License along
##  with this program.  If not, see <http://www.gnu.org/licenses/>.

typeset -r prog=$(basename "$0" .sh)
typeset -r progdir=$(\cd $(dirname "$0") && pwd -P)
typeset -r program=${progdir}/$(basename "$0")
typeset -r progpid=$$

builddir=`pwd`
. ${top_builddir:-..}/config/shdefs

die()
{
  exec 1> ${TMPDIR}/err-report.txt 2>&1
  echo "mk-agen-texi FAILED: $*"
  echo
  cat ${LOG_FILE}
  exec 2>&8 1>&2 8>&-
  cat ${TMPDIR}/err-report.txt
  trap : EXIT
  echo leaving ${TMPDIR} in place
  kill -TERM ${progpid}
  exit 1
}

set_config_values()
{
  TMPDIR=`pwd`/ag-texi-$$.d
  rm -rf ag-texi-*.d
  mkdir ${TMPDIR} || die "cannot make ${TMPDIR} directory"
  export TMPDIR

  case "$-" in
  *x* ) trap "echo 'saved tmp dir:  ${TMPDIR}';chmod 777 ${TMPDIR}" EXIT
        VERBOSE=true ;;
  *   ) trap "rm -rf ${TMPDIR}" EXIT
        VERBOSE=false ;;
  esac

  LOG_FILE=${TMPDIR}/texi.log

  exec 8>&2 2> ${LOG_FILE}

  nl='
' ht='	'
  . ${top_builddir}/config/shdefs
  : ${MAKE=`which make`}
  : ${srcdir=`pwd`}
  srcdir=`cd ${srcdir} >/dev/null ; pwd`
  INCLUDES="${DEFS} "`

    for d in ${top_srcdir} ${top_builddir} \
             ${top_builddir}/autoopts ${top_srcdir}/autoopts
    do
      (\cd ${d} && pwd) 2>/dev/null
    done | \
      sort -u | \
      sed s/^/-I/ `

  CFLAGS="${INCLUDES} "`echo ${CFLAGS} | \
    ${SED:-sed} -e "s/-Werror[^ ${ht}]*//g;s/-Wextra//g"`

  LIBS=-L`
    test -d ${top_builddir}/autoopts/.libs \
      && echo ${top_builddir}/autoopts/.libs \
      || echo ${top_builddir}/autoopts
    `" $LIBS"

  export CC CFLAGS LIBS MAKE LOG_FILE TMPDIR
}

setup_exports()
{
  # Now auto-export variables:
  #
  set -a

  PATH=${top_builddir}/columns:${PATH}
  timer=`expr ${AG_TIMEOUT} '*' 5`
  d=`find ${top_builddir}/autoopts -type f -name libopts.a -print`
  test -f "$d" || die "Cannot locate libopts.a"
  LIBS="$d ${LIBS}"

  eval `${EGREP} '^AG_[A-Z_]*' ${top_srcdir}/VERSION`

  AGsrc=${top_srcdir}/agen5
  OPTIONS_DEF=${AGsrc}/opts.def
  GETDEF_SRC=`${FGREP} -l '/*=' ${AGsrc}/*.[ch] ${AGsrc}/*.scm`

  for d in "${top_builddir}" "${top_srcdir}"
  do test -f "$d/agen5/invoke-autogen.texi" || continue
     AGEN_TEXI=${d}/agen5/invoke-autogen.texi
     break
  done
  DOC_TEXT=${top_srcdir}/doc/autogen-texi.txt

  ADDON_TEXI="
    ${top_srcdir}/doc/bitmaps.texi
    ${top_builddir}/columns/invoke-columns.texi
    ${top_builddir}/getdefs/invoke-getdefs.texi
    ${top_builddir}/xml2ag/invoke-xml2ag.texi
    ${top_srcdir}/doc/snprintfv.texi"

  DOC_INCLUDES="
    ${AGsrc}/defParse-fsm.c
    ${AGsrc}/opts.h
    ${top_builddir}/autoopts/libopts.texi
    ${top_srcdir}/doc/autogen-intro.texi
    ${AGEN_TEXI}"

  DOC_TEMPLATE=${top_builddir}/doc/auto_gen.tpl

  DOC_DEPENDS=`
    echo ${DOC_TEMPLATE} ${OPTIONS_DEF} ${ADDON_MENU} ${ADDON_TEXI} \
         ${DOC_INCLUDES} ${GETDEF_SRC}  ${DOC_TEXT}`

  set +a
}

# We have our executables and texi's.  Collect the definitions:
#
run_getdefs()
{
  gd_cfg=${TMPDIR}/getdefs.cfg
  exec 3> ${gd_cfg}
  cat >&3 <<-  EOCat
	output      ${TMPDIR}/${GEN_BASE}.def
	copy        ${OPTIONS_DEF}
	srcfile
	linenum
	template    auto_gen.tpl
	assign      ag-texi = invoke-autogen.texi
	subblock    exparg  = arg_name,arg_desc,arg_optional,arg_list
	EOCat

  tf=invoke-autogen.texi
  test -f ${tf} || ln -s ${AGEN_TEXI} ${tf}

  for f in ${ADDON_TEXI}
  do
    tf=`basename ${f}`
    case "$tf" in
    invoke-* ) : ;;
    * ) tf=invoke-$tf ;;
    esac
    test -f ${tf} || ln -s ${f} ${tf}
    echo "assign      addon-texi = ${tf}"
  done >&3

  for f in ${GETDEF_SRC}
  do
    echo "input      " ${f}
  done >&3
  exec 3>&-
  echo + ${GDexe} load-opt=${gd_cfg} >&8
  ${GDexe} load-opt=${gd_cfg} || die cannot run ${GDexe}
}

sanity_check()
{
  # Make sure the executables are there
  #
  test -x ${AGexe} || (cd `dirname ${AGexe}` ; ${MAKE}) || exit 0
  test -x ${GDexe} || (cd `dirname ${GDexe}` ; ${MAKE}) || exit 0
  test -x ${CLexe} || (cd `dirname ${CLexe}` ; ${MAKE}) || exit 0
  PATH="`dirname ${AGexe}`:`dirname ${CLexe}`:$PATH"

  # See to it that the .texi files have been generated, too.
  #
  for f in ${ADDON_TEXI} ${AGEN_TEXI} \
           ${top_builddir}/autoopts/libopts.texi
  do
    test -f "${f}" || (
      cd `dirname "${f}"`
      ${MAKE} `basename "${f}"` >&2
      test $? -ne 0 && die MAKE of ${f} failed.
    )
  done

  # Make sure we have all our sources and generate the doc
  #
  for f in ${DOC_DEPENDS}
  do test -f "${f}" || die cannot find doc file: ${f}
     test -f `basename $f` || ln -s "${f}" .
  done
}

build_agdoc() {
  #  Validate everything:
  #
  set_config_values
  setup_exports
  sanity_check
  run_getdefs

  {
    sh -c set | sed '/^BASH/d'
  } >&2
  {
    cat <<- _EOF_
	timeout     ${timer}
	templ-dirs  ${srcdir}
	templ-dirs  ${top_srcdir}/autoopts/tpl
	base-name   ${GEN_BASE}
	make-dep    F ${GEN_BASE}.dep
	make-dep    P
	_EOF_
    ${VERBOSE} && {
      echo 'trace       every'
      echo "trace-out   >>${TMPDIR}/ag.log"
      export VERBOSE
    }
  } > ${TMPDIR}/ag.ini

  opts="--load-opts=${TMPDIR}/ag.ini"
  cmd=`echo ${AGexe} ${opts} ${TMPDIR}/${GEN_BASE}.def`
  echo "${PS4:-+} " ${cmd} >&8

  timeout ${timer}s ${cmd} || {
    head -n999999 ${TMPDIR}/ag.ini ${TMPDIR}/*.log
    die could not regenerate doc
  } >&2

  test -f ${GEN_BASE}.texi || die "MISSING: ${GEN_BASE}.texi"

  exec 2>&8 8>&-
}

build_gnudocs()
{
  local sedcmd='/^@author @email/ {
    s/.*{//
    s/}.*//
    s/@@*/@/g
    p
    q
  }'

  case "X$-" in
    *x* ) local dashx=-x ;;
    *   ) local dashx=   ;;
  esac

  title=`sed -n 's/^@title  *//p' agdoc.texi`
  email=--email' '`sed -n "$sedcmd" agdoc.texi`
  opts="--texi2html ${email}"
  PS4='>${FUNCNAME:-gd}> ' ${SHELL} ${dashx} \
    ${top_srcdir}/config/gendocs.sh $opts autogen "$title"
}

mk_autogen_texi() {
  tfile=autogen.texi
  page_style=\
'\internalpagesizes{46\baselineskip}{6in}{-.25in}{-.25in}{\bindingoffset}{36pt}%'

  cat > ${tfile}$$ <<- _EOF_
	\\input texinfo
	@ignore
	${page_style}
	@end ignore
	@c %**start of header
	@setfilename ${tfile%.texi}.info
	@include ${GEN_BASE}.texi
	_EOF_

  if test -f ${tfile} && cmp -s ${tfile} ${tfile}$$
  then rm -f ${tfile}$$
  else mv -f ${tfile}$$ ${tfile}
  fi
}

PS4='>agt-${FUNCNAME}> '
set -x
GEN_BASE=agdoc
test "X$1" = X--force && {
  rm -f agdoc.texi
  shift
}
test -f agdoc.texi || build_agdoc
mk_autogen_texi

case "$1" in
gnudocs | gnudoc ) build_gnudocs ;;
* )
esac

exit 0

## Local Variables:
## mode: shell-script
## indent-tabs-mode: nil
## sh-indentation: 2
## sh-basic-offset: 2
## End:
