#!/bin/sh
# test GPT -> hybrid MBR syncing for Apple systems
# http://www.rodsbooks.com/gdisk/hybrid.html

# Copyright (C) 2012 Canonical Ltd.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

if test "$VERBOSE" = yes; then
  set -x
  parted --version
fi

: ${srcdir=.}
. $srcdir/t-lib.sh

require_root_
require_scsi_debug_module_

ss=$sector_size_
# must be big enough for a 32MiB partition in order to be big enough for FAT16
n_sectors=131072
bootcode_size=446
mbr_table_size=$((512 - $bootcode_size))

dump_mbr_table () {
  dd if=$dev bs=1 skip=$bootcode_size count=$mbr_table_size 2>/dev/null | od -v -An -tx1
}

# create memory-backed device
sectors_per_MiB=$((1024 * 1024 / $ss))
n_MiB=$((($n_sectors + $sectors_per_MiB - 1) / $sectors_per_MiB))
scsi_debug_setup_ dev_size_mb=$n_MiB > dev-name ||
  skip_test_ 'failed to create scsi_debug device'
dev=$(cat dev-name)

# force Apple mode
export PARTED_GPT_APPLE=1

# create gpt label
parted -s $dev mklabel gpt > empty 2>&1 || fail=1
compare /dev/null empty || fail=1

# print the empty table
parted -m -s $dev unit s print > t 2>&1 || fail=1
sed "s,.*/$dev:,$dev:," t > out || fail=1

# check for expected output
printf "BYT;\n$dev:${n_sectors}s:scsi:$sector_size_:$sector_size_:gpt:Linux scsi_debug;\n" \
  > exp || fail=1
compare exp out || fail=1

# the empty table should have a MBR containing only a protective entry
dump_mbr_table > out || fail=1
cat <<EOF > exp || fail=1
 00 00 01 00 ee fe ff ff 01 00 00 00 ff ff 01 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 55 aa
EOF
compare exp out || fail=1

# create a 32MiB FAT16 EFI System Partition
parted -s $dev mkpart p1 fat16 2048s 67583s set 1 boot on > empty 2>&1 || fail=1
compare /dev/null empty || fail=1
mkfs.vfat -F 16 ${dev}1 >/dev/null || skip_ "mkfs.vfat failed"

# this is represented as a protective partition, but now it only extends as
# far as the end of the first partition rather than covering the whole disk
# (matching refit gptsync's strategy); it still starts at sector one
dump_mbr_table > out || fail=1
cat <<EOF > exp || fail=1
 00 00 01 00 ee fe ff ff 01 00 00 00 ff 07 01 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 55 aa
EOF
compare exp out || fail=1

# add a 2MiB ext3 partition
parted -s $dev mkpart p2 ext3 67584s 71679s > empty 2>&1 || fail=1
compare /dev/null empty || fail=1
mkfs.ext3 ${dev}2 >/dev/null 2>&1 || skip_ "mkfs.ext3 failed"

# this should have an MBR representing both partitions; the second partition
# should be marked bootable
dump_mbr_table > out || fail=1
cat <<EOF > exp || fail=1
 00 00 01 00 ee fe ff ff 01 00 00 00 ff 07 01 00
 80 fe ff ff 83 fe ff ff 00 08 01 00 00 10 00 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 55 aa
EOF
compare exp out || fail=1

# add a 1MiB partition with no filesystem
parted -s $dev mkpart p3 71680s 73727s > empty 2>&1 || fail=1
compare /dev/null empty || fail=1

# the new partition should be represented as 0xda (Non-FS data)
dump_mbr_table > out || fail=1
cat <<EOF > exp || fail=1
 00 00 01 00 ee fe ff ff 01 00 00 00 ff 07 01 00
 80 fe ff ff 83 fe ff ff 00 08 01 00 00 10 00 00
 00 fe ff ff da fe ff ff 00 18 01 00 00 08 00 00
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 55 aa
EOF
compare exp out || fail=1

# add two more 1MiB partitions
parted -s $dev mkpart p4 73728s 75775s > empty 2>&1 || fail=1
parted -s $dev mkpart p5 75776s 77823s > empty 2>&1 || fail=1

# only the first four partitions will be represented
dump_mbr_table > out || fail=1
cat <<EOF > exp || fail=1
 00 00 01 00 ee fe ff ff 01 00 00 00 ff 07 01 00
 80 fe ff ff 83 fe ff ff 00 08 01 00 00 10 00 00
 00 fe ff ff da fe ff ff 00 18 01 00 00 08 00 00
 00 fe ff ff da fe ff ff 00 20 01 00 00 08 00 00
 55 aa
EOF
compare exp out || fail=1

# convert first partition to a BIOS Boot Partition
parted -s $dev set 1 boot off set 1 bios_grub on > empty 2>&1 || fail=1
compare /dev/null empty || fail=1

# this should be represented in the same way as an EFI System Partition
dump_mbr_table > out || fail=1
compare exp out || fail=1

# convert first partition to an ordinary FAT partition
parted -s $dev set 1 bios_grub off > empty 2>&1 || fail=1
compare /dev/null empty || fail=1

# this should result in a protective partition covering the GPT data up to
# the start of the first partition, and then representations of the first
# three partitions
dump_mbr_table > out || fail=1
cat <<EOF > exp || fail=1
 00 00 01 00 ee fe ff ff 01 00 00 00 ff 07 00 00
 00 fe ff ff 0b fe ff ff 00 08 00 00 00 00 01 00
 80 fe ff ff 83 fe ff ff 00 08 01 00 00 10 00 00
 00 fe ff ff da fe ff ff 00 18 01 00 00 08 00 00
 55 aa
EOF
compare exp out || fail=1

# convert third partition to a BIOS Boot Partition
parted -s $dev set 3 bios_grub on > empty 2>&1 || fail=1
compare /dev/null empty || fail=1

# since this isn't the first partition, it shouldn't become a protective
# partition or have its starting LBA address set to 1 (and GRUB doesn't care
# whether it's in the hybrid MBR anyway)
dump_mbr_table > out || fail=1
compare exp out || fail=1

Exit $fail
