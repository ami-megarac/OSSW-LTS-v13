/*
    libparted - a library for manipulating disk partitions
    Copyright (C) 1999 - 2009 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "config.h"

#include <parted/parted.h>
#include <parted/debug.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/disk.h>
#include <sys/ata.h>
#include <cam/cam.h>
#include <cam/scsi/scsi_pass.h>

#include "../architecture.h"

#if ENABLE_NLS
#  include <libintl.h>
#  define _(String) dgettext (PACKAGE, String)
#else
#  define _(String) (String)
#endif /* ENABLE_NLS */

#if !defined(__FreeBSD_version) && defined(__FreeBSD_kernel_version)
#define __FreeBSD_version __FreeBSD_kernel_version
#endif

#define FREEBSD_SPECIFIC(dev)	((FreeBSDSpecific*) (dev)->arch_specific)

typedef	struct _FreeBSDSpecific	FreeBSDSpecific;

struct _FreeBSDSpecific {
	int	fd;
	long long	phys_sector_size;
};

static char* _device_get_part_path (PedDevice* dev, int num);
static int _partition_is_mounted_by_path (const char* path);

static int
_device_stat (PedDevice* dev, struct stat * dev_stat)
{
	PED_ASSERT (dev != NULL);
	PED_ASSERT (!dev->external_mode);

	while (1) {
		if (!stat (dev->path, dev_stat)) {
			return 1;
		} else {
			if (ped_exception_throw (
				PED_EXCEPTION_ERROR,
				PED_EXCEPTION_RETRY_CANCEL,
				_("Could not stat device %s - %s."),
				dev->path,
				strerror (errno))
					!= PED_EXCEPTION_RETRY)
				return 0;
		}
	}
}

static int
_device_probe_type (PedDevice* dev)
{
	struct stat		dev_stat;
	char 			*np;

	if (!_device_stat (dev, &dev_stat))
		return 0;

	if (!S_ISCHR(dev_stat.st_mode)) {
		dev->type = PED_DEVICE_FILE;
		return 1;
	}

	np = strrchr(dev->path, '/');
	if (np == NULL) {
		dev->type = PED_DEVICE_UNKNOWN;
		return 0;
	}
	np += 1; /* advance past '/' */

	if (strncmp(np, "ada", 3) == 0) {
		dev->type = PED_DEVICE_SCSI;
	} else if (strncmp(np, "ad", 2) == 0) {
		dev->type = PED_DEVICE_IDE;
	} else if (strncmp(np, "da", 2) == 0) {
		dev->type = PED_DEVICE_SCSI;
	} else if (strncmp(np, "acd", 2) == 0 ||
		   strncmp(np, "cd", 2) == 0) {
		/* ignore CD-ROM drives */
		dev->type = PED_DEVICE_UNKNOWN;
		return 0;
	} else {
		dev->type = PED_DEVICE_UNKNOWN;
	}

	return 1;
}

static void
_device_set_sector_size (PedDevice* dev)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	int sector_size;

	dev->sector_size = PED_SECTOR_SIZE_DEFAULT;
	dev->phys_sector_size = PED_SECTOR_SIZE_DEFAULT;

	PED_ASSERT (dev->open_count);

	if (ioctl (arch_specific->fd, DIOCGSECTORSIZE, &sector_size)) {
		ped_exception_throw (
			PED_EXCEPTION_WARNING,
			PED_EXCEPTION_OK,
			_("Could not determine sector size for %s: %s.\n"
			  "Using the default sector size (%lld)."),
			dev->path, strerror (errno), PED_SECTOR_SIZE_DEFAULT);
	} else {
		dev->sector_size = (long long)sector_size;;
	}

	if (arch_specific->phys_sector_size)
		dev->phys_sector_size = arch_specific->phys_sector_size;

	if (sector_size != PED_SECTOR_SIZE_DEFAULT) {
		ped_exception_throw (
			PED_EXCEPTION_WARNING,
			PED_EXCEPTION_OK,
			_("Device %s has a logical sector size of %lld.  Not "
			  "all parts of GNU Parted support this at the moment, "
			  "and the working code is HIGHLY EXPERIMENTAL.\n"),
			dev->path, dev->sector_size);
	}
}

static PedSector
_device_get_length (PedDevice* dev)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	off_t bytes = 0;

	PED_ASSERT (dev->open_count > 0);
	PED_ASSERT (dev->sector_size % PED_SECTOR_SIZE_DEFAULT == 0);

	if(ioctl(arch_specific->fd, DIOCGMEDIASIZE, &bytes) != 0) {
		ped_exception_throw (
			PED_EXCEPTION_BUG,
			PED_EXCEPTION_CANCEL,
			_("Unable to determine the size of %s (%s)."),
			dev->path,
			strerror (errno));
		return 0;
	}

	return bytes / dev->sector_size;
}


static int
_device_probe_geometry (PedDevice* dev)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	struct stat		dev_stat;
	struct ata_params 	params;

	if (!_device_stat (dev, &dev_stat))
		return 0;
	PED_ASSERT (S_ISCHR (dev_stat.st_mode));

	_device_set_sector_size (dev);

	dev->length = _device_get_length (dev);
	if (!dev->length)
		return 0;

	dev->bios_geom.sectors = 63;
	dev->bios_geom.heads = 255;
	dev->bios_geom.cylinders
		= dev->length / (63 * 255);

	if (ioctl (arch_specific->fd, IOCATAGPARM, &params) != 0) {
		dev->hw_geom.sectors = params.sectors;
		dev->hw_geom.heads = params.heads;
		dev->hw_geom.cylinders = params.cylinders;
	} else {
		dev->hw_geom = dev->bios_geom;
	}

	return 1;
}

static char*
strip_name(char* str)
{
	int	i;
	int	end = 0;

	for (i = 0; str[i] != 0; i++) {
		if (!isspace (str[i])
		    || (isspace (str[i]) && !isspace (str[i+1]) && str[i+1])) {
			str [end] = str[i];
			end++;
		}
	}
	str[end] = 0;
	return strdup (str);
}

static int
init_ide (PedDevice* dev)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	struct stat		dev_stat;
	struct ata_params 	params;
	PedExceptionOption 	ex_status;
	char 			vendor_buf[64];

	if (!_device_stat (dev, &dev_stat))
		goto error;

	if (!ped_device_open (dev))
		goto error;

	if (ioctl (arch_specific->fd, IOCATAGPARM, &params) != 0) {
		ex_status = ped_exception_throw (
			PED_EXCEPTION_WARNING,
			PED_EXCEPTION_IGNORE_CANCEL,
			_("Could not get identity of device %s - %s"),
			dev->path, strerror (errno));
		switch (ex_status) {
			case PED_EXCEPTION_CANCEL:
				goto error_close_dev;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_IGNORE:
				dev->model = strdup(_("Generic IDE"));
				break;
			default:
				PED_ASSERT (0);
				break;
		}
	} else {
		snprintf(vendor_buf, 64, "%.40s/%.8s", params.model, params.revision);
		dev->model = strip_name (vendor_buf);
	}

	if (!_device_probe_geometry (dev))
		goto error_close_dev;

	ped_device_close (dev);
	return 1;

error_close_dev:
	ped_device_close (dev);
error:
	return 0;
}

static char *
_scsi_pass_dev (PedDevice* dev)
{
	union ccb	ccb;
	int		fd;
	char		result[64];

	if (sscanf (dev->path, "/dev/%2s%u", ccb.cgdl.periph_name, &ccb.cgdl.unit_number) != 2 &&
		sscanf (dev->path, "/dev/%3s%u", ccb.cgdl.periph_name, &ccb.cgdl.unit_number) != 2)
		goto error;

	if ((fd = open("/dev/xpt0", O_RDWR)) < 0)
		goto error;

        ccb.ccb_h.func_code = XPT_GDEVLIST;
	if (ioctl(fd, CAMGETPASSTHRU, &ccb) != 0)
		goto error_close_dev;

	snprintf(result, sizeof(result), "/dev/%s%d", ccb.cgdl.periph_name, ccb.cgdl.unit_number);
        close(fd);
	return strdup(result);

error_close_dev:
	close(fd);
error:
	return NULL;
}

static uint32_t
local_ata_logical_sector_size(struct ata_params *ident_data)
{
        if ((ident_data->pss & 0xc000) == 0x4000 &&
            (ident_data->pss & ATA_PSS_LSSABOVE512)) {
                return ((u_int32_t)ident_data->lss_1 |
                    ((u_int32_t)ident_data->lss_2 << 16));
        }
        return (512);
}

static uint64_t
local_ata_physical_sector_size(struct ata_params *ident_data)
{
        if ((ident_data->pss & 0xc000) == 0x4000 &&
            (ident_data->pss & ATA_PSS_MULTLS)) {
                return ((uint64_t)local_ata_logical_sector_size(ident_data) *
                    (1 << (ident_data->pss & ATA_PSS_LSPPS)));
        }
        return (512);
}

static int
init_scsi (PedDevice* dev)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	PedExceptionOption 	ex_status;
	struct stat		dev_stat;
	char*			pass_dev;
	int			pass_fd;
	union ccb		ccb;

	if (!_device_stat (dev, &dev_stat))
		goto error;

	if (!ped_device_open (dev))
		goto error;

	pass_dev = _scsi_pass_dev(dev);
	if (!pass_dev)
		goto error_close_dev;

	pass_fd = open(pass_dev, O_RDWR);
	if (pass_fd < 0) {
		dev->host = 0;
		dev->did = 0;
		if (ped_exception_throw (
			PED_EXCEPTION_ERROR,
			PED_EXCEPTION_IGNORE_CANCEL,
		        _("Error initialising SCSI device %s - %s"),
			dev->path, strerror (errno))
				!= PED_EXCEPTION_IGNORE)
			goto error_close_dev;
		if (!_device_probe_geometry (dev))
		        goto error_close_dev;
                ped_device_close (dev);
                return 1;
	}

        ccb.ccb_h.func_code = XPT_GDEVLIST;
        if (ioctl(pass_fd, CAMGETPASSTHRU, &ccb) != 0) {
		ex_status = ped_exception_throw (
			PED_EXCEPTION_WARNING,
			PED_EXCEPTION_IGNORE_CANCEL,
		        _("Could not get ID of devices %s - %s"),
			dev->path, strerror (errno));
		switch (ex_status) {
			case PED_EXCEPTION_CANCEL:
				goto error_close_fd_dev;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_IGNORE:
				dev->host = 0;
				dev->did = 0;
				break;
			default:
				PED_ASSERT (0);
				break;
			}
        }

	dev->host = ccb.ccb_h.target_id;
	dev->did = ccb.ccb_h.target_lun;

        ccb.ccb_h.func_code = XPT_GDEV_TYPE;
        if (ioctl(pass_fd, CAMIOCOMMAND, &ccb) != 0) {
		ex_status = ped_exception_throw (
			PED_EXCEPTION_WARNING,
			PED_EXCEPTION_IGNORE_CANCEL,
			_("Could not get identity of device %s - %s"),
			dev->path, strerror (errno));
		switch (ex_status) {
			case PED_EXCEPTION_CANCEL:
				goto error_close_fd_dev;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_IGNORE:
				dev->model = strdup(_("Generic SCSI"));
				break;
			default:
				PED_ASSERT (0);
				break;
		}
	} else {
		size_t model_length = (8 + 16 + 2);
		dev->model = (char*) ped_malloc (model_length);
		if (!dev->model)
			goto error_close_fd_dev;
		if (ccb.cgd.protocol == PROTO_ATA && *ccb.cgd.ident_data.model) {
			snprintf (dev->model, model_length, "%s", ccb.cgd.ident_data.model);
			arch_specific->phys_sector_size = local_ata_physical_sector_size(&ccb.cgd.ident_data);
		} else {
			snprintf (dev->model, model_length, "%.8s %.16s", ccb.cgd.inq_data.vendor, ccb.cgd.inq_data.product);
		}
	}

	if (!_device_probe_geometry (dev))
		goto error_close_fd_dev;

	close (pass_fd);
	ped_device_close (dev);
	return 1;

error_close_fd_dev:
	close (pass_fd);
error_close_dev:
	ped_device_close (dev);
error:
	return 0;
}

static int
init_file (PedDevice* dev)
{
	struct stat	dev_stat;

	if (!_device_stat (dev, &dev_stat))
		goto error;
	if (!ped_device_open (dev))
		goto error;

	if (S_ISCHR(dev_stat.st_mode))
		dev->length = _device_get_length (dev);
	else
		dev->length = dev_stat.st_size / 512;
	if (dev->length <= 0) {
		ped_exception_throw (
			PED_EXCEPTION_ERROR,
			PED_EXCEPTION_CANCEL,
			_("The device %s is zero-length, and can't possibly "
			  "store a file system or partition table.  Perhaps "
			  "you selected the wrong device?"),
			dev->path);
		goto error_close_dev;
	}

	ped_device_close (dev);

	dev->bios_geom.cylinders = dev->length / 4 / 32;
	dev->bios_geom.heads = 4;
	dev->bios_geom.sectors = 32;
	dev->hw_geom = dev->bios_geom;
	dev->sector_size = PED_SECTOR_SIZE_DEFAULT;
	dev->phys_sector_size = PED_SECTOR_SIZE_DEFAULT;
	dev->model = strdup ("");

	return 1;

error_close_dev:
	ped_device_close (dev);
error:
	return 0;
}

static int
init_generic (PedDevice* dev, char* model_name)
{
	struct stat		dev_stat;
	PedExceptionOption	ex_status;

	if (!_device_stat (dev, &dev_stat))
		goto error;

	if (!ped_device_open (dev))
		goto error;

	ped_exception_fetch_all ();
	if (_device_probe_geometry (dev)) {
		ped_exception_leave_all ();
	} else {
		/* hack to allow use of files, for testing */
		ped_exception_catch ();
		ped_exception_leave_all ();

		ex_status = ped_exception_throw (
				PED_EXCEPTION_WARNING,
				PED_EXCEPTION_IGNORE_CANCEL,
				_("Unable to determine geometry of "
				"file/device.  You should not use Parted "
				"unless you REALLY know what you're doing!"));
		switch (ex_status) {
			case PED_EXCEPTION_CANCEL:
				goto error_close_dev;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_IGNORE:
				break;
			default:
				PED_ASSERT (0);
				break;
		}

		/* what should we stick in here? */
		dev->length = dev_stat.st_size / PED_SECTOR_SIZE_DEFAULT;
		dev->bios_geom.cylinders = dev->length / 4 / 32;
		dev->bios_geom.heads = 4;
		dev->bios_geom.sectors = 32;
		dev->sector_size = PED_SECTOR_SIZE_DEFAULT;
		dev->phys_sector_size = PED_SECTOR_SIZE_DEFAULT;
	}

	dev->model = strdup (model_name);

	ped_device_close (dev);
	return 1;

error_close_dev:
	ped_device_close (dev);
error:
	return 0;
}

static PedDevice*
freebsd_new (const char* path)
{
	PedDevice*	dev;

	PED_ASSERT (path != NULL);

	dev = (PedDevice*) ped_malloc (sizeof (PedDevice));
	if (!dev)
		goto error;

	dev->path = strdup (path);
	if (!dev->path)
		goto error_free_dev;

	dev->arch_specific
		= (FreeBSDSpecific*) ped_malloc (sizeof (FreeBSDSpecific));
	if (!dev->arch_specific)
		goto error_free_path;

	memset(dev->arch_specific, 0, sizeof(FreeBSDSpecific));

	dev->open_count = 0;
	dev->read_only = 0;
	dev->external_mode = 0;
	dev->dirty = 0;
	dev->boot_dirty = 0;

	if (!_device_probe_type (dev))
		goto error_free_arch_specific;

	switch (dev->type) {
	case PED_DEVICE_IDE:
		if (!init_ide (dev))
			goto error_free_arch_specific;
		break;
	case PED_DEVICE_SCSI:
		if (!init_scsi (dev))
			goto error_free_arch_specific;
		break;
	case PED_DEVICE_FILE:
		if (!init_file (dev))
			goto error_free_arch_specific;
		break;
	case PED_DEVICE_UNKNOWN:
		if (!init_generic (dev, _("Unknown")))
			goto error_free_arch_specific;
		break;

	default:
		ped_exception_throw (PED_EXCEPTION_NO_FEATURE,
				PED_EXCEPTION_CANCEL,
				_("ped_device_new()  Unsupported device type"));
		goto error_free_arch_specific;
	}
	return dev;

error_free_arch_specific:
	free (dev->arch_specific);
error_free_path:
	free (dev->path);
error_free_dev:
	free (dev);
error:
	return NULL;
}

static void
freebsd_destroy (PedDevice* dev)
{
	free (dev->arch_specific);
	free (dev->path);
	free (dev->model);
	free (dev);
}

static int
freebsd_is_busy (PedDevice* dev)
{
	int	i;
	char*	part_name;

	if (_partition_is_mounted_by_path (dev->path))
		return 1;

	for (i = 0; i < 32; i++) {
		int status;

		part_name = _device_get_part_path (dev, i);
		if (!part_name)
			return 1;
		status = _partition_is_mounted_by_path (part_name);
		free (part_name);

		if (status)
			return 1;
	}

	return 0;
}

static void
_flush_cache (PedDevice* dev)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);

	if (dev->read_only)
		return;
	dev->dirty = 0;

	if (dev->type == PED_DEVICE_FILE) {
		if (fsync(arch_specific->fd) != 0) {
			ped_exception_throw (
				PED_EXCEPTION_WARNING,
				PED_EXCEPTION_OK,
				_("Could not flush cache of file %s - %s."),
				dev->path,
				strerror (errno));
			return;
		}
	} else {
		if (ioctl (arch_specific->fd, DIOCGFLUSH) != 0) {
			ped_exception_throw (
				PED_EXCEPTION_WARNING,
				PED_EXCEPTION_OK,
				_("Could not flush cache of device %s - %s."),
				dev->path,
				strerror (errno));
			return;
		}
	}

	return;
}

/* By default, kernel of FreeBSD does not allow overwriting MBR */
#define GEOM_SYSCTL    "kern.geom.debugflags"

static int
freebsd_open (PedDevice* dev)
{
	int old_flags, flags;
	size_t flagssize;
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);

retry:
	flagssize = sizeof (old_flags);

	if (sysctlbyname (GEOM_SYSCTL, &old_flags, &flagssize, NULL, 0) != 0) {
		ped_exception_throw (
			PED_EXCEPTION_WARNING,
			PED_EXCEPTION_OK,
			_("Unable to get %s sysctl (%s)."),
			GEOM_SYSCTL,
			strerror (errno));
	}

	if ((old_flags & 0x10) == 0) {
		/* "allow foot shooting", see geom(4) */
		flags = old_flags | 0x10;

		if (sysctlbyname (GEOM_SYSCTL, NULL, NULL, &flags, sizeof (int)) != 0) {
			flags = old_flags;
			ped_exception_throw (
				PED_EXCEPTION_WARNING,
				PED_EXCEPTION_OK,
				_("Unable to set %s sysctl (%s)."),
				GEOM_SYSCTL,
				strerror (errno));
		}
	} else
		flags = old_flags;

	arch_specific->fd = open (dev->path, O_RDWR);

	if (flags != old_flags) {
		if (sysctlbyname (GEOM_SYSCTL, NULL, NULL, &old_flags, sizeof (int)) != 0) {
			ped_exception_throw (
				PED_EXCEPTION_WARNING,
				PED_EXCEPTION_OK,
				_("Unable to set %s sysctl (%s)."),
				GEOM_SYSCTL,
				strerror (errno));
		}
	}

	if (arch_specific->fd == -1) {
		char*	rw_error_msg = strerror (errno);

		arch_specific->fd = open (dev->path, O_RDONLY);

		if (arch_specific->fd == -1) {
			if (ped_exception_throw (
				PED_EXCEPTION_ERROR,
				PED_EXCEPTION_RETRY_CANCEL,
				_("Error opening %s: %s"),
				dev->path, strerror (errno))
					!= PED_EXCEPTION_RETRY) {
				return 0;
			} else {
				goto retry;
			}
		} else {
			ped_exception_throw (
				PED_EXCEPTION_WARNING,
				PED_EXCEPTION_OK,
				_("Unable to open %s read-write (%s).  %s has "
				  "been opened read-only."),
				dev->path, rw_error_msg, dev->path);
			dev->read_only = 1;
		}
	} else {
		dev->read_only = 0;
	}

	_flush_cache (dev);

	return 1;
}

static int
freebsd_refresh_open (PedDevice* dev)
{
	return 1;
}

static int
freebsd_close (PedDevice* dev)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);

	if (dev->dirty)
		_flush_cache (dev);
	close (arch_specific->fd);
	return 1;
}

static int
freebsd_refresh_close (PedDevice* dev)
{
	if (dev->dirty)
		_flush_cache (dev);
	return 1;
}

static int
_device_seek (const PedDevice* dev, PedSector sector)
{
	FreeBSDSpecific*	arch_specific;
	off_t	pos;

	PED_ASSERT (dev->sector_size % PED_SECTOR_SIZE_DEFAULT == 0);
	PED_ASSERT (dev != NULL);
	PED_ASSERT (!dev->external_mode);

	arch_specific = FREEBSD_SPECIFIC (dev);

	pos = sector * dev->sector_size;
	return lseek (arch_specific->fd, pos, SEEK_SET) == pos;
}

static int
freebsd_read (const PedDevice* dev, void* buffer, PedSector start,
	      PedSector count)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	PedExceptionOption	ex_status;
	void*			diobuf = NULL;

        PED_ASSERT (dev != NULL);
        PED_ASSERT (dev->sector_size % PED_SECTOR_SIZE_DEFAULT == 0);

	while (1) {
		if (_device_seek (dev, start))
			break;

		ex_status = ped_exception_throw (
			PED_EXCEPTION_ERROR,
			PED_EXCEPTION_RETRY_IGNORE_CANCEL,
			_("%s during seek for read on %s"),
			strerror (errno), dev->path);

		switch (ex_status) {
			case PED_EXCEPTION_IGNORE:
				return 1;

			case PED_EXCEPTION_RETRY:
				break;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_CANCEL:
				return 0;
			default:
				PED_ASSERT (0);
				break;
		}
	}

	size_t read_length = count * dev->sector_size;
	if (posix_memalign (&diobuf, dev->sector_size, read_length) != 0)
		return 0;

	while (1) {
		ssize_t status = read (arch_specific->fd, diobuf, read_length);
		if (status > 0) {
			memcpy(buffer, diobuf, status);
		}
		if (status == (ssize_t) read_length)
			break;
		if (status > 0) {
			read_length -= status;
			buffer = (char *) buffer + status;
			continue;
		}

		ex_status = ped_exception_throw (
			PED_EXCEPTION_ERROR,
			PED_EXCEPTION_RETRY_IGNORE_CANCEL,
			_("%s during read on %s"),
			strerror (errno),
			dev->path);

		switch (ex_status) {
			case PED_EXCEPTION_IGNORE:
				return 1;

			case PED_EXCEPTION_RETRY:
				break;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_CANCEL:
				return 0;
			default:
				PED_ASSERT (0);
				break;
		}
	}

	free (diobuf);

	return 1;
}

static int
freebsd_write (PedDevice* dev, const void* buffer, PedSector start,
	     PedSector count)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	PedExceptionOption	ex_status;
	void*			diobuf;
	void*			diobuf_start;

	PED_ASSERT(dev->sector_size % PED_SECTOR_SIZE_DEFAULT == 0);

	if (dev->read_only) {
		if (ped_exception_throw (
			PED_EXCEPTION_ERROR,
			PED_EXCEPTION_IGNORE_CANCEL,
			_("Can't write to %s, because it is opened read-only."),
			dev->path)
				!= PED_EXCEPTION_IGNORE)
			return 0;
		else
			return 1;
	}

	while (1) {
		if (_device_seek (dev, start))
			break;

		ex_status = ped_exception_throw (
			PED_EXCEPTION_ERROR, PED_EXCEPTION_RETRY_IGNORE_CANCEL,
			_("%s during seek for write on %s"),
			strerror (errno), dev->path);

		switch (ex_status) {
			case PED_EXCEPTION_IGNORE:
				return 1;

			case PED_EXCEPTION_RETRY:
				break;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_CANCEL:
				return 0;
			default:
				PED_ASSERT (0);
				break;
		}
	}

#ifdef READ_ONLY
	printf ("ped_device_write (\"%s\", %p, %d, %d)\n",
		dev->path, buffer, (int) start, (int) count);
#else
	size_t write_length = count * dev->sector_size;
	dev->dirty = 1;
	if (posix_memalign(&diobuf, dev->sector_size, write_length) != 0)
		return 0;
	memcpy(diobuf, buffer, write_length);
	diobuf_start = diobuf;
	while (1) {
		ssize_t status = write (arch_specific->fd, diobuf, write_length);
		if (status == count * PED_SECTOR_SIZE_DEFAULT) break;
		if (status > 0) {
			write_length -= status;
			diobuf = (char *) diobuf + status;
			continue;
		}

		ex_status = ped_exception_throw (
			PED_EXCEPTION_ERROR,
			PED_EXCEPTION_RETRY_IGNORE_CANCEL,
			_("%s during write on %s"),
			strerror (errno), dev->path);

		switch (ex_status) {
			case PED_EXCEPTION_IGNORE:
				return 1;

			case PED_EXCEPTION_RETRY:
				break;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_CANCEL:
				return 0;
			default:
				PED_ASSERT (0);
				break;
		}
	}
	free(diobuf_start);
#endif /* !READ_ONLY */
	return 1;
}

/* returns the number of sectors that are ok.
 */
static PedSector
freebsd_check (PedDevice* dev, void* buffer, PedSector start, PedSector count)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	PedSector		done = 0;
	int			status;
	void*			diobuf;

	if (!_device_seek (dev, start))
		return 0;

	if (posix_memalign(&diobuf, PED_SECTOR_SIZE_DEFAULT,
			   count * PED_SECTOR_SIZE_DEFAULT) != 0)
		return 0;

	for (done = 0; done < count; done += status / dev->sector_size) {
		status = read (arch_specific->fd, diobuf,
				(size_t) ((count-done) * dev->sector_size));
		if (status > 0)
			memcpy(buffer, diobuf, status);
		if (status < 0)
			break;
	}
	free(diobuf);

	return done;
}

static int
_do_fsync (PedDevice* dev)
{
	FreeBSDSpecific*	arch_specific = FREEBSD_SPECIFIC (dev);
	int			status;
	PedExceptionOption	ex_status;

	while (1) {
		status = fsync (arch_specific->fd);
		if (status >= 0) break;

		ex_status = ped_exception_throw (
			PED_EXCEPTION_ERROR,
			PED_EXCEPTION_RETRY_IGNORE_CANCEL,
			_("%s during write on %s"),
			strerror (errno), dev->path);

		switch (ex_status) {
			case PED_EXCEPTION_IGNORE:
				return 1;

			case PED_EXCEPTION_RETRY:
				break;

			case PED_EXCEPTION_UNHANDLED:
				ped_exception_catch ();
			case PED_EXCEPTION_CANCEL:
				return 0;
			default:
				PED_ASSERT (0);
				break;
		}
	}
	return 1;
}

static int
freebsd_sync (PedDevice* dev)
{
	PED_ASSERT (dev != NULL);
	PED_ASSERT (!dev->external_mode);

	if (dev->read_only)
		return 1;
	if (!_do_fsync (dev))
		return 0;
	_flush_cache (dev);
	return 1;
}

static int
freebsd_sync_fast (PedDevice* dev)
{
	PED_ASSERT (dev != NULL);
	PED_ASSERT (!dev->external_mode);

	if (dev->read_only)
		return 1;
	if (!_do_fsync (dev))
		return 0;
	/* no cache flush... */
	return 1;
}

static int
_probe_standard_devices ()
{
	/* Add standard devices that are not autodetected here. */
	return 1;
}

static int
_probe_kern_disks ()
{
	size_t listsize;
	char *disklist, *pdisklist, *psave;
	char buf[PATH_MAX];
	struct stat st;

	if (sysctlbyname("kern.disks", NULL, &listsize, NULL, 0) != 0) {
		ped_exception_throw (
			PED_EXCEPTION_WARNING,
			PED_EXCEPTION_OK,
			_("Could not get the list of devices through kern.disks sysctl."));
		return 0;
	}

	if (listsize == 0)
		return 0;

	disklist = ped_malloc(listsize + 1);
	if (!disklist)
		return 0;

	if (sysctlbyname("kern.disks", disklist, &listsize, NULL, 0) != 0) {
		free(disklist);
		return 0;
	}

	for (pdisklist = disklist ; ; pdisklist = NULL) {
		char dev_name [256];
		char *token;

		token = strtok_r(pdisklist, " ", &psave);
		if (token == NULL)
			break;

		strncpy (dev_name, _PATH_DEV, sizeof(dev_name));
		strncat (dev_name, token, sizeof(dev_name) - strlen(_PATH_DEV) - 1);
		dev_name[sizeof(dev_name) - 1] = '\0';
		_ped_device_probe (dev_name);

		snprintf (buf, sizeof (buf), "%s.eli", dev_name);
		if (stat (buf, &st) == 0)
			_ped_device_probe (buf);
	}

	free(disklist);
	return 1;
}

static int
_probe_zfs_volumes ()
{
	DIR*	pool_dir;
	DIR*	zvol_dir;
	struct dirent*  pool_dent;
	struct dirent*	zvol_dent;
	char            buf[PATH_MAX];
	struct stat     st;

	pool_dir = opendir ("/dev/zvol");
	if (!pool_dir)
		return 0;

	while ((pool_dent = readdir (pool_dir))) {
		if (strcmp (pool_dent->d_name, ".")  == 0 || strcmp (pool_dent->d_name, "..") == 0)
			continue;

		snprintf (buf, sizeof (buf), "/dev/zvol/%s", pool_dent->d_name);
		zvol_dir = opendir (buf);

		while ((zvol_dent = readdir (zvol_dir))) {
			if (strcmp (zvol_dent->d_name, ".")  == 0 || strcmp (zvol_dent->d_name, "..") == 0)
				continue;

			snprintf (buf, sizeof (buf), "/dev/zvol/%s/%s", pool_dent->d_name, zvol_dent->d_name);
			if (stat (buf, &st) != 0)
				continue;
			_ped_device_probe (buf);
		}
		closedir (zvol_dir);
	}
	closedir (pool_dir);

	return 1;
}

static int
_probe_lvm_volumes ()
{
	DIR*	lvm_dir;
	struct dirent*	lvm_dent;
	char            buf[PATH_MAX];
	struct stat     st;

	lvm_dir = opendir ("/dev/linux_lvm");
	if (!lvm_dir)
		return 0;

	while ((lvm_dent = readdir (lvm_dir))) {
		if (strcmp (lvm_dent->d_name, ".")  == 0 || strcmp (lvm_dent->d_name, "..") == 0)
			continue;
		snprintf (buf, sizeof (buf), "/dev/linux_lvm/%s", lvm_dent->d_name);
		if (stat (buf, &st) != 0)
			continue;
		_ped_device_probe (buf);
	}
	closedir (lvm_dir);

	return 1;
}

static void
freebsd_probe_all ()
{
	_probe_standard_devices ();

	_probe_kern_disks ();

	_probe_zfs_volumes ();

	_probe_lvm_volumes ();
}

static char*
_device_get_part_path (PedDevice* dev, int num)
{
	int		path_len = strlen (dev->path);
	int		result_len = path_len + 16;
	int		is_gpt;
	char*		result;
	PedDisk*	disk;

	disk = ped_disk_new (dev);
	if (!disk)
		return NULL;

	result = (char*) ped_malloc (result_len);
	if (!result)
		return NULL;

	is_gpt = !strcmp (disk->type->name, "gpt");

	ped_disk_destroy (disk);

	/* append slice number (ad0, partition 1 => ad0s1)*/
	snprintf (result, result_len, is_gpt ? "%sp%d" : "%ss%d", dev->path, num);

	return result;
}

static char*
freebsd_partition_get_path (const PedPartition* part)
{
	return _device_get_part_path (part->disk->dev, part->num);
}

static int
_partition_is_mounted_by_dev (dev_t dev)
{
	struct stat mntdevstat;
	struct statfs *mntbuf, *statfsp;
	char *devname;
	char device[256];
	int mntsize, i;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = 0; i < mntsize; i++) {
		statfsp = &mntbuf[i];
		devname = statfsp->f_mntfromname;
		if (*devname != '/') {
			strcpy(device, _PATH_DEV);
			strcat(device, devname);
			strcpy(statfsp->f_mntfromname, device);
		}
		if (stat(devname, &mntdevstat) == 0 &&
			mntdevstat.st_rdev == dev)
			return 1;
	}

	return 0;
}

static int
_partition_is_mounted_by_path (const char *path)
{
	struct stat part_stat;
	if (stat (path, &part_stat) != 0)
		return 0;
	if (!S_ISCHR(part_stat.st_mode))
		return 0;
	return _partition_is_mounted_by_dev (part_stat.st_rdev);
}

static int
_partition_is_mounted (const PedPartition *part)
{
	int status;
	char* part_name;

	if (!ped_partition_is_active (part))
		return 0;
	part_name = _device_get_part_path (part->disk->dev, part->num);
	if (!part_name)
		return 0;
	status = _partition_is_mounted_by_path (part_name);
	free (part_name);
	return status;
}

static int
freebsd_partition_is_busy (const PedPartition* part)
{
	PedPartition*	walk;

	PED_ASSERT (part != NULL);

	if (_partition_is_mounted (part))
		return 1;
	if (part->type == PED_PARTITION_EXTENDED) {
		for (walk = part->part_list; walk; walk = walk->next) {
			if (freebsd_partition_is_busy (walk))
				return 1;
		}
	}
	return 0;
}

static int
_kernel_reread_part_table (PedDevice* dev)
{
	/* The FreeBSD kernel (at least the 7.x series) automatically
	   monitors the partition tables and re-read them if they
	   change. */
	return 1;
}

static int
freebsd_disk_commit (PedDisk* disk)
{
	if (disk->dev->type != PED_DEVICE_FILE)
		return _kernel_reread_part_table (disk->dev);

	return 1;
}

static PedDeviceArchOps freebsd_dev_ops = {
	_new:		freebsd_new,
	destroy:	freebsd_destroy,
	is_busy:	freebsd_is_busy,
	open:		freebsd_open,
	refresh_open:	freebsd_refresh_open,
	close:		freebsd_close,
	refresh_close:	freebsd_refresh_close,
	read:		freebsd_read,
	write:		freebsd_write,
	check:		freebsd_check,
	sync:		freebsd_sync,
	sync_fast:	freebsd_sync_fast,
	probe_all:	freebsd_probe_all
};

PedDiskArchOps freebsd_disk_ops =  {
	partition_get_path:	freebsd_partition_get_path,
	partition_is_busy:	freebsd_partition_is_busy,
	disk_commit:		freebsd_disk_commit
};

PedArchitecture ped_freebsd_arch = {
	dev_ops:	&freebsd_dev_ops,
	disk_ops:	&freebsd_disk_ops
};
