#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME mount
#=DESCRIPTION 
# This test verifies that the mount syscall is indeed restricted for confined 
# processes.
#=END

# I made this a seperate test script because of the need to make a
# loopfile before the tests run.

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

##
## A. MOUNT
##

mount_file=$tmpdir/mountfile
mount_point=$tmpdir/mountpoint
mount_bad=$tmpdir/mountbad
loop_device="unset" 
fstype="ext2"

setup_mnt() {
	/bin/mount -n -t${fstype} ${loop_device} ${mount_point}
#	/bin/mount -n -t${fstype} ${loop_device} ${mount_bad}
}
remove_mnt() {
	mountpoint -q "${mount_point}"
	if [ $? -eq 0 ] ; then
		/bin/umount -t${fstype} ${mount_point}
	fi
	mountpoint -q "${mount_bad}"
	if [ $? -eq 0 ] ; then
		/bin/umount -t${fstype} ${mount_bad}
	fi
}

mount_cleanup() {
	remove_mnt &> /dev/null
	if [ "$loop_device" != "unset" ]
	then
		/sbin/losetup -d ${loop_device} &> /dev/null
	fi
}
do_onexit="mount_cleanup"

dd if=/dev/zero of=${mount_file} bs=1024 count=512 2> /dev/null
/sbin/mkfs -t${fstype} -F ${mount_file} > /dev/null 2> /dev/null
/bin/mkdir ${mount_point}
/bin/mkdir ${mount_bad}

# in a modular udev world, the devices won't exist until the loopback
# module is loaded.
if [ ! -b /dev/loop0 ] ; then 
	modprobe loop
fi

# kinda ugly way of atomically finding a free loop device
for i in $(seq 0 15) 
do
	if [ "$loop_device" = "unset" ] 
	then
		if /sbin/losetup /dev/loop$i ${mount_file} > /dev/null 2> /dev/null
		then 
			loop_device=/dev/loop$i;
		fi
	fi
done
if [ "$loop_device" = "unset" ] 
then
	fatalerror 'Unable to find a free loop device'
fi


# TEST 1.  Make sure can mount and umount unconfined
runchecktest "MOUNT (unconfined)" pass mount ${loop_device} ${mount_point}
remove_mnt

setup_mnt
runchecktest "UMOUNT (unconfined)" pass umount ${loop_device} ${mount_point}
remove_mnt

# TEST A2.  confine MOUNT no perms
genprofile
runchecktest "MOUNT (confined no perm)" fail mount ${loop_device} ${mount_point}
remove_mnt

setup_mnt
runchecktest "UMOUNT (confined no perm)" fail umount ${loop_device} ${mount_point}
remove_mnt


if [ "$(kernel_features mount)" != "true" -o "$(parser_supports 'mount,')" != "true" ] ; then
	genprofile capability:sys_admin
	runchecktest "MOUNT (confined cap)" pass mount ${loop_device} ${mount_point}
	remove_mnt

	setup_mnt
	runchecktest "UMOUNT (confined cap)" pass umount ${loop_device} ${mount_point}
	remove_mnt
else
	echo "    using mount rules ..."

	genprofile capability:sys_admin
	runchecktest "MOUNT (confined cap)" fail mount ${loop_device} ${mount_point}
	remove_mnt

	setup_mnt
	runchecktest "UMOUNT (confined cap)" fail umount ${loop_device} ${mount_point}
	remove_mnt


	genprofile mount:ALL
	runchecktest "MOUNT (confined mount:ALL)" fail mount ${loop_device} ${mount_point}
	remove_mnt


	genprofile "mount:-> ${mount_point}/"
	runchecktest "MOUNT (confined bad mntpnt mount -> mntpnt)" fail mount ${loop_device} ${mount_bad}
	remove_mnt

	runchecktest "MOUNT (confined mount -> mntpnt)" fail mount ${loop_device} ${mount_point}
	remove_mnt



	genprofile umount:ALL
	setup_mnt
	runchecktest "UMOUNT (confined umount:ALL)" fail umount ${loop_device} ${mount_point}
	remove_mnt


	genprofile mount:ALL cap:sys_admin
	runchecktest "MOUNT (confined cap mount:ALL)" pass mount ${loop_device} ${mount_point}
	remove_mnt


	genprofile cap:sys_admin "mount:-> ${mount_point}/"
	runchecktest "MOUNT (confined bad mntpnt cap mount -> mntpnt)" fail mount ${loop_device} ${mount_bad}
	remove_mnt

	runchecktest "MOUNT (confined cap mount -> mntpnt)" pass mount ${loop_device} ${mount_point}
	remove_mnt


	genprofile cap:sys_admin "mount:fstype=${fstype}XXX"
	runchecktest "MOUNT (confined cap mount bad fstype)" fail mount ${loop_device} ${mount_point}
	remove_mnt

	genprofile cap:sys_admin "mount:fstype=${fstype}"
	runchecktest "MOUNT (confined cap mount fstype)" pass mount ${loop_device} ${mount_point}
	remove_mnt


	genprofile cap:sys_admin umount:ALL
	setup_mnt
	runchecktest "UMOUNT (confined cap umount:ALL)" pass umount ${loop_device} ${mount_point}
	remove_mnt

fi

#need tests for move mount, remount, bind mount, chroot
