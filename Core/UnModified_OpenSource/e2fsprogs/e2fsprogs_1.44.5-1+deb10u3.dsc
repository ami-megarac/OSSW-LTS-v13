-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: e2fsprogs
Binary: fuse2fs, e2fsck-static, e2fsprogs-l10n, libcomerr2, libcom-err2, comerr-dev, libss2, ss-dev, e2fsprogs-udeb, e2fslibs, e2fslibs-dev, libext2fs2, libext2fs-dev, e2fsprogs
Architecture: any all
Version: 1.44.5-1+deb10u3
Maintainer: Theodore Y. Ts'o <tytso@mit.edu>
Homepage: http://e2fsprogs.sourceforge.net
Standards-Version: 4.2.1
Vcs-Browser: https://git.kernel.org/pub/scm/fs/ext2/e2fsprogs.git
Vcs-Git: https://git.kernel.org/pub/scm/fs/ext2/e2fsprogs.git -b debian/master
Build-Depends: gettext, texinfo, pkg-config, libfuse-dev [linux-any kfreebsd-any] <!pkg.e2fsprogs.no-fuse2fs>, libattr1-dev, debhelper (>= 9.0), libblkid-dev, uuid-dev, m4
Package-List:
 comerr-dev deb libdevel optional arch=any
 e2fsck-static deb admin optional arch=any profile=!pkg.e2fsprogs.no-static-e2fsck
 e2fslibs deb oldlibs optional arch=any
 e2fslibs-dev deb oldlibs optional arch=all
 e2fsprogs deb admin required arch=any
 e2fsprogs-l10n deb localization optional arch=all
 e2fsprogs-udeb udeb debian-installer optional arch=any profile=!noudeb
 fuse2fs deb admin optional arch=linux-any,kfreebsd-any profile=!pkg.e2fsprogs.no-fuse2fs
 libcom-err2 deb libs optional arch=any
 libcomerr2 deb oldlibs optional arch=any
 libext2fs-dev deb libdevel optional arch=any
 libext2fs2 deb libs optional arch=any
 libss2 deb libs optional arch=any
 ss-dev deb libdevel optional arch=any
Checksums-Sha1:
 c3f64d10b6ef1a268a077838a5cafb6aaebe2986 7619237 e2fsprogs_1.44.5.orig.tar.gz
 db05093049f5f2788a5b6fd23a6746bbcfe05ed9 488 e2fsprogs_1.44.5.orig.tar.gz.asc
 4d02f7e775f06bdf8ed5c526df2776fb1290ad68 82412 e2fsprogs_1.44.5-1+deb10u3.debian.tar.xz
Checksums-Sha256:
 2e211fae27ef74d5af4a4e40b10b8df7f87c655933bd171aab4889bfc4e6d1cc 7619237 e2fsprogs_1.44.5.orig.tar.gz
 c0e3e4e51f46c005890963b005015b784b2f19e291a16a15681b9906528f557e 488 e2fsprogs_1.44.5.orig.tar.gz.asc
 0114857448922a218613f369f665f03f1b1435004c9d79ce5ee1a8a8a6cec53f 82412 e2fsprogs_1.44.5-1+deb10u3.debian.tar.xz
Files:
 8d78b11d04d26c0b2dd149529441fa80 7619237 e2fsprogs_1.44.5.orig.tar.gz
 dde8ecabaf0f5082ef7de90e8bc9b8c6 488 e2fsprogs_1.44.5.orig.tar.gz.asc
 06b5d0e4023d94add08971e8e58c8264 82412 e2fsprogs_1.44.5-1+deb10u3.debian.tar.xz
Dgit: d28e48c4a50b433b6c71d4f81ff69a3294ee6b24 debian archive/debian/1.44.5-1+deb10u3 https://git.dgit.debian.org/e2fsprogs

-----BEGIN PGP SIGNATURE-----

iQEzBAEBCAAdFiEEK2m5VNv+CHkogTfJ8vlZVpUNgaMFAl4sbPgACgkQ8vlZVpUN
gaNsVQf/fcD+BCtoZcwXYxyFZJSDJlKzLy7bKsdHpTwKb8INrC2uxeCfyHBNKaOm
8hkZN4nYPjG5+4573r4kvLCZvQ9tku4Jh5rOE5X7bfw/CEoh9yQReF1IkyinnmqQ
Fknrp5ll66MTfVTohkbnF9VoMEiF5O6vrbBIsWNmcMXX62xmOVlK2+43yYACVWMX
/6Q06/eSeg38n1lLdv385BHtKYsK4dOTkYC+V5jAzj0jEFAd/Wxhm+kiQyqRQXmg
lHrSjBSuCxt7+Y/L3I+XJ1lXgzoAD3+qtpIHgSojYJQzlgOfMxwSu1zHiOt1X8jL
iUMfa2A0FvAnoQMeKbmDpa/dcEZ6Aw==
=DHI6
-----END PGP SIGNATURE-----
