#! /bin/sh
##  -*- Mode: shell-script -*-
## mklibsrc.sh --   make the libopts tear-off library source tarball
##
##  This file is part of AutoGen.
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

top_builddir=`cd $top_builddir ; pwd`
top_srcdir=`cd $top_srcdir ; pwd`

test -x ${top_builddir}/agen5/autogen   || exit 0
test -x ${top_builddir}/columns/columns || exit 0

ao_rev=${AO_CURRENT}.${AO_REVISION}.${AO_AGE}
tag=libopts-${ao_rev}

cd ${top_builddir}/pkg
[ ! -d ${tag} ] || rm -rf ${tag}
mkdir ${tag} ${tag}/compat ${tag}/autoopts ${tag}/m4
tagd=`pwd`/${tag}

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#
#  WORKING IN SOURCE DIRECTORY
#
cd ${top_builddir}/autoopts
files='libopts.c gettext.h parse-duration.c parse-duration.h
	stdnoreturn.in.h '$(
    fgrep '#include' libopts.c | \
       sed -e 's,"$,,;s,#.*",,' )

for f in ${files} intprops.h
do
  test -f ${f} &&
    cp -f ${f} ${tagd}/${f} && continue

  test -f ${top_srcdir}/autoopts/${f} &&
    cp -f ${top_srcdir}/autoopts/${f} ${tagd}/${f} && continue

  test -f ${top_srcdir}/${f} &&
    cp -f ${top_srcdir}/${f} ${tagd}/${f} && continue
done

cp -f ${top_srcdir}/pkg/libopts/COPYING.* ${tagd}/.

cd ${top_srcdir}/compat
cp windows-config.h compat.h pathfind.c snprintf.c strdup.c strchr.c \
   ../config/snippet/_Noreturn.h ${tagd}/compat/.
#
#  END WORK IN SOURCE DIRECTORY
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

cd ${tagd}

cp ${top_srcdir}/config/libopts*.m4 ${top_srcdir}/config/stdnoret*.m4 m4/.
chmod u+w m4/libopts.m4
cat ${top_srcdir}/pkg/libopts/libopts-add.m4 >> m4/libopts.m4
test ! -f Makefile.am || rm -f Makefile.am

sed s,'\${tag}',"${tag}",g ${top_srcdir}/pkg/libopts/README > README

touch MakeDefs.inc

vers=${AO_CURRENT}:${AO_REVISION}:${AO_AGE}
{
  sed $'s/^\t//' << EOMakefile
	## LIBOPTS Makefile
	MAINTAINERCLEANFILES    = Makefile.in
	if INSTALL_LIBOPTS
	lib_LTLIBRARIES         = libopts.la
	else
	noinst_LTLIBRARIES      = libopts.la
	endif
	libopts_la_SOURCES      = libopts.c
	libopts_la_CPPFLAGS     = -I\$(srcdir)
	libopts_la_LDFLAGS      = -version-info ${AM_LDFLAGS} ${vers}
	EXTRA_DIST		=
	BUILT_SOURCES		=
	MOSTLYCLEANFILES	=

	libopts.c:		\$(BUILT_SOURCES)
		@: do-nothing rule to avoid default SCCS get

EOMakefile

  printf '\n# Makefile fragment from gnulib-s stdnoreturn module:\n#\n'
  sed '/^#/d;/^$/d;s/top_srcdir/srcdir/' \
    ${top_srcdir}/pkg/libopts/stdnoreturn.mk
  printf '\nEXTRA_DIST += \\\n'
  find $(ls -A) -type f \
    | env LC_COLLATE=C sort \
    | egrep -v '^(libopts\.c|Makefile\.am)$' \
    | ${CLexe} -I4 --spread=1 --line-sep="  \\"
} > Makefile.am

gz='gzip --best'
sfx=tar.gz

cd ..
echo ! cd `pwd`
echo ! tar cvf ${tag}.${sfx} ${tag}
tar cvf - ${tag} | $gz > ${top_builddir}/autoopts/${tag}.${sfx}
rm -rf ${tag}

## Local Variables:
## mode: shell-script
## indent-tabs-mode: nil
## sh-indentation: 2
## sh-basic-offset: 2
## End:

## end of mklibsrc.sh
