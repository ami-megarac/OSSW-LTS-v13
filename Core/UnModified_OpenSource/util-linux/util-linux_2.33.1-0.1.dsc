-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: util-linux
Binary: util-linux, util-linux-locales, mount, bsdutils, fdisk, fdisk-udeb, libblkid1, libblkid1-udeb, libblkid-dev, libfdisk1, libfdisk1-udeb, libfdisk-dev, libmount1, libmount1-udeb, libmount-dev, libsmartcols1, libsmartcols1-udeb, libsmartcols-dev, libuuid1, uuid-runtime, libuuid1-udeb, uuid-dev, util-linux-udeb, rfkill
Architecture: any all
Version: 2.33.1-0.1
Maintainer: LaMont Jones <lamont@debian.org>
Uploaders: Adam Conrad <adconrad@0c3.net>
Standards-Version: 4.1.2
Vcs-Browser: https://salsa.debian.org/debian/util-linux
Vcs-Git: https://salsa.debian.org/debian/util-linux.git
Testsuite: autopkgtest
Testsuite-Triggers: bash, bc, bsdmainutils, dpkg, grep
Build-Depends: bc <!stage1 !nocheck>, debhelper (>= 11), dh-exec (>= 0.13), dpkg-dev (>= 1.17.14), gettext, libaudit-dev [linux-any] <!stage1>, libcap-ng-dev [linux-any] <!stage1>, libncurses5-dev, libncursesw5-dev, libpam0g-dev <!stage1>, libselinux1-dev (>= 2.6-3~) [linux-any], libsystemd-dev [linux-any] <!stage1>, libtool, libudev-dev [linux-any] <!stage1>, netbase <!stage1 !nocheck>, pkg-config, po-debconf, socat <!stage1 !nocheck>, systemd [linux-any] <!stage1>, bison, zlib1g-dev
Package-List:
 bsdutils deb utils required arch=any profile=!stage1 essential=yes
 fdisk deb utils important arch=any profile=!stage1
 fdisk-udeb udeb debian-installer optional arch=hurd-any,linux-any profile=!stage1
 libblkid-dev deb libdevel optional arch=any
 libblkid1 deb libs optional arch=any
 libblkid1-udeb udeb debian-installer optional arch=any
 libfdisk-dev deb libdevel optional arch=any
 libfdisk1 deb libs optional arch=any
 libfdisk1-udeb udeb debian-installer optional arch=any
 libmount-dev deb libdevel optional arch=linux-any
 libmount1 deb libs optional arch=any
 libmount1-udeb udeb debian-installer optional arch=linux-any
 libsmartcols-dev deb libdevel optional arch=any
 libsmartcols1 deb libs optional arch=any
 libsmartcols1-udeb udeb debian-installer optional arch=any
 libuuid1 deb libs optional arch=any
 libuuid1-udeb udeb debian-installer optional arch=any
 mount deb admin required arch=linux-any profile=!stage1
 rfkill deb utils optional arch=linux-any profile=!stage1
 util-linux deb utils required arch=any profile=!stage1 essential=yes
 util-linux-locales deb localization optional arch=all profile=!stage1
 util-linux-udeb udeb debian-installer optional arch=any profile=!stage1
 uuid-dev deb libdevel optional arch=any
 uuid-runtime deb utils optional arch=any profile=!stage1
Checksums-Sha1:
 f57232d9594d23e7c20b5728b24bf4e5d977accc 4650936 util-linux_2.33.1.orig.tar.xz
 cb1387efead38e5c18f62fc034e9222f5aeb8d6e 81780 util-linux_2.33.1-0.1.debian.tar.xz
Checksums-Sha256:
 c14bd9f3b6e1792b90db87696e87ec643f9d63efa0a424f092a5a6b2f2dbef21 4650936 util-linux_2.33.1.orig.tar.xz
 07bfeb8298fab559dec2091463cab343785853bcae6c92c0806b7639e105913a 81780 util-linux_2.33.1-0.1.debian.tar.xz
Files:
 6fcfea2043b5ac188fd3eed56aeb5d90 4650936 util-linux_2.33.1.orig.tar.xz
 ad5d638c9cfa042cd79de5b7313140d9 81780 util-linux_2.33.1-0.1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQJFBAEBCgAvFiEE+uHltkZSvnmOJ4zCC8R9xk0TUwYFAlw3BxgRHGFuZHJlYXNA
ZmF0YWwuc2UACgkQC8R9xk0TUwbx3w/+JxC+G+PmdwI+9ajvn58QSqmJ2KpFWZX4
HYgPfmvooQKVsKcZJvMfmIaF1YPVopzXs/dmzG7ER2InvHMZJxL8kJ75LSlpnAUt
XN9yI3ZTGOQvagWqhw6DaSwD3MzUjA1BIMEXtbCyvHKjq6y3BTdlkcK0ghXaXHV5
XN0cxVqwWFv7LntvrWIYM5edxGt1RveC1Mgu0KbhIFNHFsRRIAYQ1CwX5Eool1RM
5MVvv8zPoFUeGxhHNejq5Vo/rizFp0x2h35ty1Uvg0+28Rl0qoXF1T2AHn/4lae3
8L2Bl9dr5eKBF1OLBwfPgNUlyWJkLB5Z76ATxmVXdyN+mLmfbdCeqaTi/rJQ4oc3
3znxRvEMFIZjpHg/hGlNFSFF73/doyA8FbIuSdCg+N3CXeRo02IU7uIwRRvs4W42
bLx87VH7cRXgD+UMwVjbFrE3SmDKx9RHxAQzDPYMyI26sQxB7U0eQSB4gcYO6jce
j5S7NV/IVpRNgthg6S1ANOan1Dw75O7Ve8xa805h3ZrhozuqyE/71OsJhkOcuXSh
VhUy7ihewBk3oDl8BSstp0uxGC8YXvKLs8DEHBN7gKSC/Nli2853WnowMnM4RcUh
6BmJrbvcV9Kbct6skxgF0/dDPSVDV/oB7w+wfG6uDySHYO3uoc3dVUA7c3A+Hjnp
FGTfRoETaPs=
=oh1E
-----END PGP SIGNATURE-----
