/* SPDX-License-Identifier: LGPL-2.1+ */

#include <sys/mount.h>

#include "alloc-util.h"
#include "fs-util.h"
#include "main-func.h"
#include "mkdir.h"
#include "mount-util.h"
#include "mountpoint-util.h"
#include "path-util.h"
#include "stat-util.h"
#include "string-util.h"
#include "volatile-util.h"

static int make_volatile(const char *path) {
        _cleanup_free_ char *old_usr = NULL;
        int r;

        r = path_is_mount_point(path, NULL, AT_SYMLINK_FOLLOW);
        if (r < 0)
                return log_error_errno(r, "Couldn't determine whether %s is a mount point: %m", path);
        if (r == 0)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "%s is not a mount point.", path);

        r = path_is_temporary_fs(path);
        if (r < 0)
                return log_error_errno(r, "Couldn't determine whether %s is a temporary file system: %m", path);
        if (r > 0) {
                log_info("%s already is a temporary file system.", path);
                return 0;
        }

        r = chase_symlinks("/usr", path, CHASE_PREFIX_ROOT, &old_usr);
        if (r < 0)
                return log_error_errno(r, "/usr not available in old root: %m");

        r = mkdir_p("/run/systemd/volatile-sysroot", 0700);
        if (r < 0)
                return log_error_errno(r, "Couldn't generate volatile sysroot directory: %m");

        r = mount_verbose(LOG_ERR, "tmpfs", "/run/systemd/volatile-sysroot", "tmpfs", MS_STRICTATIME, "mode=755");
        if (r < 0)
                goto finish_rmdir;

        if (mkdir("/run/systemd/volatile-sysroot/usr", 0755) < 0) {
                r = -errno;
                goto finish_umount;
        }

        r = mount_verbose(LOG_ERR, old_usr, "/run/systemd/volatile-sysroot/usr", NULL, MS_BIND|MS_REC, NULL);
        if (r < 0)
                goto finish_umount;

        r = bind_remount_recursive("/run/systemd/volatile-sysroot/usr", true, NULL);
        if (r < 0)
                goto finish_umount;

        r = umount_recursive(path, 0);
        if (r < 0) {
                log_error_errno(r, "Failed to unmount %s: %m", path);
                goto finish_umount;
        }

        if (mount(NULL, "/", NULL, MS_SLAVE|MS_REC, NULL) < 0)
                log_warning_errno(errno, "Failed to remount %s MS_SLAVE|MS_REC: %m", path);

        r = mount_verbose(LOG_ERR, "/run/systemd/volatile-sysroot", path, NULL, MS_MOVE, NULL);

finish_umount:
        (void) umount_recursive("/run/systemd/volatile-sysroot", 0);

finish_rmdir:
        (void) rmdir("/run/systemd/volatile-sysroot");

        return r;
}

static int run(int argc, char *argv[]) {
        VolatileMode m = _VOLATILE_MODE_INVALID;
        const char *path;
        int r;

        log_setup_service();

        if (argc > 3)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "Too many arguments. Expected directory and mode.");

        r = query_volatile_mode(&m);
        if (r < 0)
                return log_error_errno(r, "Failed to determine volatile mode from kernel command line.");
        if (r == 0 && argc >= 2) {
                /* The kernel command line always wins. However if nothing was set there, the argument passed here wins instead. */
                m = volatile_mode_from_string(argv[1]);
                if (m < 0) {
                        log_error("Couldn't parse volatile mode: %s", argv[1]);
                        r = -EINVAL;
                }
        }

        if (argc < 3)
                path = "/sysroot";
        else {
                path = argv[2];

                if (isempty(path))
                        return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                               "Directory name cannot be empty.");
                if (!path_is_absolute(path))
                        return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                               "Directory must be specified as absolute path.");
                if (path_equal(path, "/"))
                        return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                               "Directory cannot be the root directory.");
        }

        if (m != VOLATILE_YES)
                return 0;

        return make_volatile(path);
}

DEFINE_MAIN_FUNCTION(run);
