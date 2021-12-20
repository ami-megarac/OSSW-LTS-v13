#!/bin/sh

set -e

BASE_REL=$(dpkg-parsechangelog 2>/dev/null | sed -ne 's/Version: \([0-9.]\+\)+\?.*/\1/p')
UPSTREAM_GIT="git://git.ffmpeg.org/rtmpdump"

if [ -z ${BASE_REL} ]; then
	echo 'Please run this script from the sources root directory.'
	exit 1
fi

echo "Fetching latest upstream code into FETCH_HEAD"
git fetch $UPSTREAM_GIT

RTMPDUMP_GIT_COMMIT=`git describe --always FETCH_HEAD`
RTMPDUMP_GIT_DATE=`git --no-pager log -1 --pretty=format:%cd --date=short FETCH_HEAD | sed s/-//g`
TARBALL="../rtmpdump_${BASE_REL}+${RTMPDUMP_GIT_DATE}.git${RTMPDUMP_GIT_COMMIT}.orig.tar.gz"

git archive --format=tar --prefix=rtmpdump-"${RTMPDUMP_GIT_DATE}/" FETCH_HEAD | \
	gzip -9fn > "$TARBALL"

echo "Suchessfully created $TARBALL"


