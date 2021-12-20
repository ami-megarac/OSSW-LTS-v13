/*
    libparted - a library for manipulating disk partitions
    Copyright (C) 2000, 2007, 2009-2010 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include <parted/parted.h>
#include <parted/endian.h>

#if ENABLE_NLS
#  include <libintl.h>
#  define _(String) dgettext (PACKAGE, String)
#else
#  define _(String) (String)
#endif /* ENABLE_NLS */

#include <unistd.h>

#define ZFS_SIGNATURE		0x00bab10c

struct zfs_uberblock
{
  uint64_t signature;
  uint64_t version;
};

static PedGeometry*
zfs_probe (PedGeometry* geom)
{
	struct zfs_uberblock *uber = alloca (geom->dev->sector_size);

	if (!ped_geometry_read (geom, uber, 256, 1))
		return 0;

	if ((le64toh (uber->signature) == ZFS_SIGNATURE
		|| be64toh (uber->signature) == ZFS_SIGNATURE)
		&& uber->version != 0)
		return ped_geometry_new (geom->dev, geom->start, geom->length);
	else
		return NULL;
}

static PedFileSystemOps zfs_ops = {
	probe:		zfs_probe,
};

static PedFileSystemType zfs_type = {
	next:	NULL,
	ops:	&zfs_ops,
	name:	"zfs",
};

void
ped_file_system_zfs_init ()
{
	ped_file_system_type_register (&zfs_type);
}

void
ped_file_system_zfs_done ()
{
	ped_file_system_type_unregister (&zfs_type);
}
