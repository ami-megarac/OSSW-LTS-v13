-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: mariadb-10.3
Binary: libmariadb-dev, libmariadbclient-dev, libmariadb-dev-compat, libmariadb3, libmariadbd19, libmariadbd-dev, mariadb-common, mariadb-client-core-10.3, mariadb-client-10.3, mariadb-server-core-10.3, mariadb-server-10.3, mariadb-server, mariadb-client, mariadb-backup, mariadb-plugin-connect, mariadb-plugin-rocksdb, mariadb-plugin-oqgraph, mariadb-plugin-tokudb, mariadb-plugin-mroonga, mariadb-plugin-spider, mariadb-plugin-gssapi-server, mariadb-plugin-gssapi-client, mariadb-plugin-cracklib-password-check, mariadb-test, mariadb-test-data
Architecture: any all
Version: 1:10.3.27-0+deb10u1
Maintainer: Debian MySQL Maintainers <pkg-mysql-maint@lists.alioth.debian.org>
Uploaders: Otto Kekäläinen <otto@debian.org>
Homepage: https://mariadb.org/
Standards-Version: 4.2.1
Vcs-Browser: https://salsa.debian.org/mariadb-team/mariadb-10.3
Vcs-Git: https://salsa.debian.org/mariadb-team/mariadb-10.3.git
Testsuite: autopkgtest
Build-Depends: bison, chrpath, cmake, cracklib-runtime, debhelper (>= 9.20160709), dh-apparmor, dh-exec, gdb, libaio-dev [linux-any], libarchive-dev, libboost-dev, libcrack2-dev (>= 2.9.0), libgnutls28-dev (>= 3.3.24), libjemalloc-dev [linux-any], libjudy-dev, libkrb5-dev, liblz4-dev, libncurses5-dev (>= 5.0-6~), libpam0g-dev, libpcre3-dev (>= 2:8.35-3.2~), libreadline-gplv2-dev, libreadline-gplv2-dev:native, libsnappy-dev, libsystemd-dev [linux-any], libxml2-dev, libzstd-dev (>= 1.3.3), lsb-release, perl, po-debconf, psmisc, unixodbc-dev, zlib1g-dev (>= 1:1.1.3-5~)
Package-List:
 libmariadb-dev deb libdevel optional arch=any
 libmariadb-dev-compat deb libdevel optional arch=any
 libmariadb3 deb libs optional arch=any
 libmariadbclient-dev deb oldlibs optional arch=any
 libmariadbd-dev deb libdevel optional arch=any
 libmariadbd19 deb libs optional arch=any
 mariadb-backup deb database optional arch=any
 mariadb-client deb database optional arch=all
 mariadb-client-10.3 deb database optional arch=any
 mariadb-client-core-10.3 deb database optional arch=any
 mariadb-common deb database optional arch=all
 mariadb-plugin-connect deb database optional arch=any
 mariadb-plugin-cracklib-password-check deb database optional arch=any
 mariadb-plugin-gssapi-client deb database optional arch=any
 mariadb-plugin-gssapi-server deb database optional arch=any
 mariadb-plugin-mroonga deb database optional arch=any-alpha,any-amd64,any-arm,any-arm64,any-i386,any-ia64,any-mips64el,any-mips64r6el,any-mipsel,any-mipsr6el,any-nios2,any-powerpcel,any-ppc64el,any-sh3,any-sh4,any-tilegx
 mariadb-plugin-oqgraph deb database optional arch=any
 mariadb-plugin-rocksdb deb database optional arch=amd64,arm64,mips64el,ppc64el
 mariadb-plugin-spider deb database optional arch=any
 mariadb-plugin-tokudb deb database optional arch=amd64
 mariadb-server deb database optional arch=all
 mariadb-server-10.3 deb database optional arch=any
 mariadb-server-core-10.3 deb database optional arch=any
 mariadb-test deb database optional arch=any
 mariadb-test-data deb database optional arch=all
Checksums-Sha1:
 8bc164b1a2ad77cd410169ae5b4df12cc97bbd6c 72875692 mariadb-10.3_10.3.27.orig.tar.gz
 9384649f3da023a4b96cac50261d914d68236fda 195 mariadb-10.3_10.3.27.orig.tar.gz.asc
 9f1c3ff82a5bcf8606f8d7cc36007360caf2d9fa 221444 mariadb-10.3_10.3.27-0+deb10u1.debian.tar.xz
Checksums-Sha256:
 0dadc1650ab2ff40caab58210e93b106ae1e3d1a82e5b0fd92c795b8b43e4619 72875692 mariadb-10.3_10.3.27.orig.tar.gz
 8f09058bc136ccb5fc4a4d1832e4fdc80db12ba012c90887dd7364b0a305b3bf 195 mariadb-10.3_10.3.27.orig.tar.gz.asc
 30d75f0d95eaa877757415825d0ac2a8ccffb315350a433bad636a6029074d88 221444 mariadb-10.3_10.3.27-0+deb10u1.debian.tar.xz
Files:
 6ab2934a671191d8ca8730e9a626c5c9 72875692 mariadb-10.3_10.3.27.orig.tar.gz
 0b45c27bca50485b0ea81531d36b1209 195 mariadb-10.3_10.3.27.orig.tar.gz.asc
 27a6a2e642146a5cf09fd3f99e73ff18 221444 mariadb-10.3_10.3.27-0+deb10u1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEmbRSsR88dMO0U+RvvthEn87o2ogFAl/A7FQACgkQvthEn87o
2ohvWA/7Be01GBFVQS75fCHIuWUukDKuKZqWqvMQIWf2YwbGyOKUysyGJ0xBl6Gf
TGVSNmfgeYxDW6+nZrfqXhJXbLmWLvXVKJ9b6Z1YdbpG7dkJtMA6hK3zvLq8559u
YzaU8sYvoN/TybVLZKSWDDNWhwwkqpo5A4jLV9OK/yCn9CMJDDDAL26thu+qPYmZ
HfTYS/FLAb89qgkFhyoVVpZvuZf3MIiH6qP8MHh8oC3s/y7GSoWeUKU2au5Pypsw
eOyf69TMu7WOM7ELApUI5WwRMDRQECH9z3Sqboa94asODZys82Y05GrwwPhBt6Mx
8BerXqmtDpRhJR9p9nHOttg9upZ8XhYbUqioLsTNJdHgyl5JKnMsNQB4gAD0qc3i
/hYw1fBFwRnmMR8saL+ZMq79ILuOxEVOeJi5b/FicCAdEX6Fm3fJInuCuzgdlhmC
8CDrF0hDJVXA2TL/A4z26sAO/kkNZK93XS5JU/M5X0ICp+Wvrs4ViaHBFctUzESi
WaLksJD2SEmlglrgdtJZToOxA//cdwYtf/7zE/YqjmT2pnTc2kLuO7MfjvpZQnk3
fTi6J4TrLIa7lD1rynvbg6J5uy7rq4Zasgiv5gZJNEtS+KS/f+m3cBUVDDj+73AN
UTyyunXuNBi+hX/xUrvY4xPPZ9DXvSmKBM1sFUjghYY6q91JCDE=
=GPY1
-----END PGP SIGNATURE-----
