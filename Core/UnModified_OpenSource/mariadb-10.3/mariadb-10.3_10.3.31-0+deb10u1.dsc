-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: mariadb-10.3
Binary: libmariadb-dev, libmariadbclient-dev, libmariadb-dev-compat, libmariadb3, libmariadbd19, libmariadbd-dev, mariadb-common, mariadb-client-core-10.3, mariadb-client-10.3, mariadb-server-core-10.3, mariadb-server-10.3, mariadb-server, mariadb-client, mariadb-backup, mariadb-plugin-connect, mariadb-plugin-rocksdb, mariadb-plugin-oqgraph, mariadb-plugin-tokudb, mariadb-plugin-mroonga, mariadb-plugin-spider, mariadb-plugin-gssapi-server, mariadb-plugin-gssapi-client, mariadb-plugin-cracklib-password-check, mariadb-test, mariadb-test-data
Architecture: any all
Version: 1:10.3.31-0+deb10u1
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
 db808430f5c759e6a90e84c89fe81ad08f0aa022 73298653 mariadb-10.3_10.3.31.orig.tar.gz
 0609f46f60c0290d18d10300d7ab8ccaec0b432d 195 mariadb-10.3_10.3.31.orig.tar.gz.asc
 e084a3898a0b32e42faf0c4da183a54a2035c10f 222020 mariadb-10.3_10.3.31-0+deb10u1.debian.tar.xz
Checksums-Sha256:
 20421dfe5750f510ab0ee23420337332e6799cd38fa31332e2841dfa956eb771 73298653 mariadb-10.3_10.3.31.orig.tar.gz
 f215e0b11511b28b4f185fffad92cca3508531afa62279a0e30e84ce9953ea9d 195 mariadb-10.3_10.3.31.orig.tar.gz.asc
 9ea979da970b6fef5874db889f0f78f9c301dc5d0a2e60780be0bcdc635d7b32 222020 mariadb-10.3_10.3.31-0+deb10u1.debian.tar.xz
Files:
 154f9abd0b21ec5f1c89ba0fba474267 73298653 mariadb-10.3_10.3.31.orig.tar.gz
 43c44ed8a05c4c13fb1f13b3dc4dc79a 195 mariadb-10.3_10.3.31.orig.tar.gz.asc
 e87b46111c1fcb9b3aac00797b59ef60 222020 mariadb-10.3_10.3.31-0+deb10u1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEmbRSsR88dMO0U+RvvthEn87o2ogFAmEvwYQACgkQvthEn87o
2ogQNhAAuQmlNwjYDIksGFR/4gqZyJsP1lHLwdeFAJvzFJRsVx6tW/dIbRcLJsiM
e0jHnwV3mM7IIB/OqZuGpOScbjN7Q7OX919WTATqefi1AeefBAG7dJyBpi40j8TE
aTf16lyMG+Y5/VgUG9HoPGF3ITvFIwEhimiC6e+W8nTiGQ9wuYM8MVqRHHBEwsNr
TCVh+jWFIS7vt7szgkPtrFiTsXsU1UjJCfwOzJ3alLvoJWlVC0lTkl2g1n5uMqZA
D12Z3ff/E/J8sDz0Lpwr1WmsYCYq3rOMN7T18k8NjjiygcZhkMiimj1irzAIwuAU
4ZO5ClOWMpNSiI5tiCBIrJ+HAl8zUCMo+dQTKjoeI7nsBSpeogLy9r6RzPc/M5M/
FzWwKbi14OSTeKnRynXQF4uZZoUAHYEJhfi2XLhBCzx3DwQugdODBOb2gyzbmaXh
jD1ZNMXDzqGQO/bIhoFiuXK8FRh84n0KIL6yhJKO8RzshlKsXVYrI9Pevu4gKx/W
QhCLFaFpzzTEaUQ7HSmOp8phxpyayXP77wlVdtMY28yZQhx8vkDQZC6Pr3PN1YBv
jc2A7gVs36OxPYyCjlamsjq/pjDQMD62qa69n519/Yd27NOECbnpcbY4fJ9IK8CE
+hXtfqUk3EkyEA1D+mV2XUQ2I6uRCyoax7TjZrbbdm9yGHySBJU=
=NCrm
-----END PGP SIGNATURE-----
