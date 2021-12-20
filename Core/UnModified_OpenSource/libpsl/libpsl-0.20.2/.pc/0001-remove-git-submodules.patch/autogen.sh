#!/bin/sh

AUTORECONF=$(which autoreconf 2>/dev/null)
if test $? -ne 0; then
  echo "No 'autoreconf' found. You must install the autoconf package."
  exit 1
fi

GIT=$(which git 2>/dev/null)
if test $? -ne 0; then
  echo "No 'git' found. You must install the git package."
  exit 1
fi

#  create m4 before gtkdocize
mkdir -p m4 2>/dev/null

GTKDOCIZE=$(which gtkdocize 2>/dev/null)
if test $? -ne 0; then
  echo "No gtk-doc support found. You can't build the docs."
  # rm because gtk-doc.make might be a link to a protected file
  rm -f gtk-doc.make 2>/dev/null
  echo "EXTRA_DIST =" >gtk-doc.make
  echo "CLEANFILES =" >>gtk-doc.make
  GTKDOCIZE=""
else
  $GTKDOCIZE
fi

$GIT submodule init
$GIT submodule update
$AUTORECONF --install --force --symlink

echo
echo "----------------------------------------------------------------"
echo "Initialized build system. For a common configuration please run:"
echo "----------------------------------------------------------------"
echo
if test -z $GTKDOCIZE; then
  echo "./configure"
else
  echo "./configure --enable-gtk-doc"
fi
echo
