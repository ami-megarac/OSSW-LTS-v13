#!/bin/bash
#
# Build MariaDB .deb packages
#
# This file is run by BuildBot, Travis-CI and other CI tools. This file is not
# used to build the official Debian/Ubuntu packages.
#

# Exit immediately on any error
set -e

# Debug script and command lines
#set -x

# Don't run the mysql-test-run test suite as part of build.
# It takes a lot of time, and we will do a better test anyway in
# BuildBot, running the test suite from installed .debs on a clean VM.
export DEB_BUILD_OPTIONS="nocheck"

# Automatically use major.minor version strings from the sources
#
source ./VERSION
UPSTREAM="${MYSQL_VERSION_MAJOR}.${MYSQL_VERSION_MINOR}.${MYSQL_VERSION_PATCH}${MYSQL_VERSION_EXTRA}"
RELEASE_EXTRA=""

RELEASE_NAME=""
PATCHLEVEL="+deb" # Differentiate from MariaDB.org repositories that use '+maria' 
LOGSTRING="MariaDB build"

# Look up distro-version specific stuff
CODENAME="$(lsb_release -sc)"

# Adjust changelog, add new version
echo "Incrementing changelog and starting build scripts"
dch -b -D ${CODENAME} -v "${UPSTREAM}${PATCHLEVEL}~${CODENAME}" "Automatic build with ${LOGSTRING}."

# Build the package
# Pass -I so that .git and other unnecessary temporary and source control files
# will be ignored by dpkg-source when creating the tar.gz source package.
# Use -b to build binary only packages as there is no need to waste time on
# generating the source package.
echo "Creating package version ${UPSTREAM}${PATCHLEVEL}~${CODENAME} ... "
fakeroot dpkg-buildpackage -us -uc -I -b

# Don't log package contents on Travis-CI to save time and log size
if [[ ! $TRAVIS ]]
then
  echo "List package contents ..."
  cd ..
  for package in `ls *.deb`
  do
    echo $package | cut -d '_' -f 1
    dpkg-deb -c $package | awk '{print $1 " " $2 " " $6}' | sort -k 3
    echo "------------------------------------------------"
  done
fi

echo "Build complete"
