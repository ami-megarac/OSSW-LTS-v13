#! /bin/bash
#	Copyright (C) 2014 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME pivot_root
#=DESCRIPTION 
# This test verifies that the pivot_root syscall is indeed restricted for
# confined processes.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

disk_img=$tmpdir/disk_img
new_root=$tmpdir/new_root/
put_old=${new_root}put_old/
bad=$tmpdir/BAD/
proc=$new_root/proc
fstype="ext2"
root_was_shared="no"

pivot_root_cleanup() {
	mountpoint -q "$proc"
	if [ $? -eq 0 ] ; then
		umount "$proc"
	fi

	mountpoint -q "$new_root"
	if [ $? -eq 0 ] ; then
		umount "$new_root"
	fi

	if [ "${root_was_shared}" = "yes" ] ; then
		[ -n "$VERBOSE" ] && echo 'notice: re-mounting / as shared'
		mount --make-shared /
	fi
}
do_onexit="pivot_root_cleanup"

# systemd mounts / and everything under it MS_SHARED. This breaks
# pivot_root entirely, so attempt to detect it, and remount /
# MS_PRIVATE temporarily.
FINDMNT=/bin/findmnt
if [ -x "${FINDMNT}" ] && ${FINDMNT} -no PROPAGATION / > /dev/null 2>&1 ; then
	if [ "$(${FINDMNT} -no PROPAGATION /)" == "shared" ] ; then
		root_was_shared="yes"
	fi
elif [ "$(ps hp1  -ocomm)" = "systemd" ] ; then
	# no findmnt or findmnt doesn't know the PROPAGATION column,
	# but init is systemd so assume rootfs is shared
	root_was_shared="yes"
fi
if [ "${root_was_shared}" = "yes" ] ; then
	[ -n "$VERBOSE" ] && echo 'notice: re-mounting / as private'
	mount --make-private /
fi

# Create disk image since pivot_root doesn't allow old root and new root to be
# on the same filesystem
dd if=/dev/zero of="$disk_img" bs=1024 count=512 2> /dev/null
/sbin/mkfs -t "$fstype" -F "$disk_img" > /dev/null 2> /dev/null
/bin/mkdir "$new_root"
/bin/mount -o loop -t "$fstype" "$disk_img" "$new_root"

# Must mount proc because the pivot_root test program calls aa_getcon() after
# pivot_root() and aa_getcon() reads /proc/<PID>/attr/current
mkdir "$proc"
mount -t proc proc "$proc"

# Will be used for pivot_root()'s put_old parameter
mkdir "$put_old"

do_test()
{
	local desc="PIVOT_ROOT ($1)"
	shift

	runchecktest "$desc" "$@"
}

# Needed for aa_getcon()
cur="/proc/*/attr/current:r"

# Needed for clone(CLONE_NEWNS) and pivot_root()
cap=capability:sys_admin

# A profile name that'll be used to test AA's transitions during pivot_root()
new_prof=/sbin/init


# Ensure everything works as expected when unconfined
do_test "unconfined" pass "$put_old" "$new_root" unconfined

# Ensure the test binary is accurately doing post pivot_root profile verification
do_test "unconfined, bad context" fail "$put_old" "$new_root" "$bad"

# Ensure failure when no perms are granted
genprofile
do_test "no perms" fail "$put_old" "$new_root" "$test"

if [ "$(kernel_features mount)" != "true" -o "$(parser_supports 'mount,')" != "true" ] ; then
	# pivot_root mediation isn't supported by this kernel/parser, so verify that
	# capability sys_admin is sufficient and skip the remaining tests
	genprofile $cur $cap
	do_test "cap" pass "$put_old" "$new_root" "$test"

	exit
fi

# Ensure failure when no pivot_root perms are granted
genprofile $cur $cap
do_test "cap only" fail "$put_old" "$new_root" "$test"

# Ensure failure when everything except capability sys_admin is granted
genprofile $cur "pivot_root:ALL"
do_test "bare rule, no cap" fail "$put_old" "$new_root" "$test"

# Give sufficient perms with full pivot_root access
genprofile $cur $cap "pivot_root:ALL"
do_test "bare rule" pass "$put_old" "$new_root" "$test"

# Give sufficient perms and specify new_root
genprofile $cur $cap "pivot_root:$new_root"
do_test "new_root" pass "$put_old" "$new_root" "$test"

# Ensure failure when new_root is bad
genprofile $cur $cap "pivot_root:$bad"
do_test "bad new_root" fail "$put_old" "$new_root" "$test"

# Give sufficient perms and specify put_old
genprofile $cur $cap "pivot_root:oldroot=$put_old"
do_test "put_old" pass "$put_old" "$new_root" "$test"

# Ensure failure when put_old is bad
genprofile $cur $cap "pivot_root:oldroot=$bad"
do_test "bad put_old" fail "$put_old" "$new_root" "$test"

# Give sufficient perms and specify put_old and new_root
genprofile $cur $cap "pivot_root:oldroot=$put_old $new_root"
do_test "put_old, new_root" pass "$put_old" "$new_root" "$test"

# Ensure failure when put_old is bad
genprofile $cur $cap "pivot_root:oldroot=$bad $new_root"
do_test "bad put_old, new_root" fail "$put_old" "$new_root" "$test"

# Ensure failure when new_root is bad
genprofile $cur $cap "pivot_root:oldroot=$put_old $bad"
do_test "put_old, bad new_root" fail "$put_old" "$new_root" "$test"

if [ "$(kernel_features_istrue namespaces/pivot_root)" != "true" ] ; then
    echo "	kernel does not support pivot_root domain transitions - skipping tests ..."
elif [ "$(parser_supports 'pivot_root -> foo,')"  != "true" ] ; then
    #pivot_root domain transitions not supported
    echo "	parser does not support pivot root domain transitions - skipping tests ..."
else
    # Give sufficient perms and perform a profile transition
    genprofile $cap "pivot_root:-> $new_prof" -- image=$new_prof $cur
    do_test "transition" pass "$put_old" "$new_root" "$new_prof"

    # Ensure failure when the the new profile can't read /proc/<PID>/attr/current
    genprofile $cap "pivot_root:-> $new_prof" -- image=$new_prof
    do_test "transition, no perms" fail "$put_old" "$new_root" "$new_prof"

    # Ensure failure when the new profile doesn't exist
    genprofile $cap "pivot_root:-> $bad" -- image=$new_prof $cur
    do_test "bad transition" fail "$put_old" "$new_root" "$new_prof"

    # Ensure the test binary is accurately doing post pivot_root profile verification
    genprofile $cap "pivot_root:-> $new_prof" -- image=$new_prof $cur
    do_test "bad transition comparison" fail "$put_old" "$new_root" "$test"

    # Give sufficient perms with new_root and a transition
    genprofile $cap "pivot_root:$new_root -> $new_prof" -- image=$new_prof $cur
    do_test "new_root, transition" pass "$put_old" "$new_root" "$new_prof"

    # Ensure failure when the new profile doesn't exist and new_root is specified
    genprofile $cap "pivot_root:$new_root -> $bad" -- image=$new_prof $cur
    do_test "new_root, bad transition" fail "$put_old" "$new_root" "$new_prof"

    # Give sufficient perms with new_root, put_old, and a transition
    genprofile $cap "pivot_root:oldroot=$put_old $new_root -> $new_prof" -- image=$new_prof $cur
    do_test "put_old, new_root, transition" pass "$put_old" "$new_root" "$new_prof"

    # Ensure failure when the new profile doesn't exist and new_root and put_old are specified
    genprofile $cap "pivot_root:oldroot=$put_old $new_root -> $bad" -- image=$new_prof $cur
    do_test "put_old, new_root, bad transition" fail "$put_old" "$new_root" "$new_prof"

fi
