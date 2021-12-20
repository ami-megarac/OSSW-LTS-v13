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

target=$tmpdir/target
linkfile=$tmpdir/linkfile
okperm=rwixl
badperm=rwl
nolinkperm=rwix


PERMS="r w m ix px ux Px Ux l rw rm rix rpx rux rPx rUx rl wm wix wpx wux \
	wPx wUx wl mix mpx mux mPx mUx ml ixl pxl uxl Pxl Uxl rwm rwix rwpx \
	rwux rwPx rwUx rwl rmix rmpx rmux rmPx rmUx rml wmix wmpx wmux wmPx \
	wmUx wml mixl mpxl muxl mPxl mUxl rwmix rwmpx rwmux rwmPx rwmUx \
	rwml wmixl wmpxl wmuxl wmPxl wmUxl rwmixl rwmpxl rwmuxl rwmPxl \
	rwmUxl"


# unconfined test - no target file
runchecktest "unconfined - no target" fail $target $linkfile

touch $target
# unconfined test
runchecktest "unconfined" pass $target $linkfile

rm -rf $target
# Link no perms on link or target - no target file
genprofile
runchecktest "link no target (no perms) -> target (no perms)" fail $target $linkfile
rm -rf $linkfile

touch $target
# Link no perms on link or target 
runchecktest "link (no perms) -> target (no perms)" fail $target $linkfile
rm -rf $linkfile

