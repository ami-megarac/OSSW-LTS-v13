#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME link
#=DESCRIPTION
# Link requires 'l' permission on the link and that permissions on the
#links rwmx perms are a subset of the targets perms, and if x is present
#that the link and target have the same x qualifiers.
# This test verifies matching, non-matching and missing link
# permissions in a profile.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

target=$tmpdir/target_
linkfile=$tmpdir/link_

tfiles=`$bin/link_subset --filenames $target`
lfiles=`$bin/link_subset --filenames $linkfile`

# unconfined test - no target file
#runchecktest "unconfined - no target" fail $target $linkfile

#touch $target
# unconfined test
#runchecktest "unconfined" pass $target $linkfile

#rm -rf $target
# Link no perms on link or target - no target file
#genprofile
#runchecktest "link no target (no perms) -> target (no perms)" fail $target $linkfile
#rm -rf $linkfile

touch $target
for f in $tfiles ; do touch ${f%%:*} ; done

# Link no perms on link or target
#runchecktest "link (no perms) -> target (no perms)" fail $target $linkfile
#rm -rf $linkfile

genprofile $tfiles $lfiles hat:remove_link /**:rw
runchecktest "link_subset" pass $target $linkfile
rm -rf $linkfile
