-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: apparmor
Binary: apparmor, apparmor-utils, apparmor-profiles, libapparmor-dev, libapparmor1, libapparmor-perl, libapache2-mod-apparmor, libpam-apparmor, apparmor-notify, python3-libapparmor, python3-apparmor, dh-apparmor, apparmor-easyprof
Architecture: any all
Version: 2.13.2-10
Maintainer: Debian AppArmor Team <pkg-apparmor-team@lists.alioth.debian.org>
Uploaders: intrigeri <intrigeri@debian.org>
Homepage: http://apparmor.net/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/apparmor-team/apparmor
Vcs-Git: https://salsa.debian.org/apparmor-team/apparmor.git
Testsuite: autopkgtest
Testsuite-Triggers: @builddeps@, apparmor-profiles-extra, bind9, cups-browsed, cups-daemon, evince, haveged, libreoffice-common, libvirt-daemon-system, linux-image-amd64, man-db, ntp, onioncircuits, tcpdump, thunderbird, tor
Build-Depends: apache2-dev, autoconf, automake, bison, bzip2, chrpath, debhelper (>= 11), dejagnu, dh-apache2, flex, liblocale-gettext-perl, libpam-dev, libtool, perl, pkg-config, po-debconf, python3, python3-all-dev, swig
Package-List:
 apparmor deb admin optional arch=any
 apparmor-easyprof deb admin optional arch=all
 apparmor-notify deb admin optional arch=all
 apparmor-profiles deb admin optional arch=all
 apparmor-utils deb admin optional arch=any
 dh-apparmor deb devel optional arch=all
 libapache2-mod-apparmor deb httpd optional arch=any
 libapparmor-dev deb libdevel optional arch=any
 libapparmor-perl deb perl optional arch=any
 libapparmor1 deb libs optional arch=any
 libpam-apparmor deb admin optional arch=any
 python3-apparmor deb python optional arch=any
 python3-libapparmor deb python optional arch=any
Checksums-Sha1:
 e3ba457792f42178be5cd18192dc1af53e336503 7369240 apparmor_2.13.2.orig.tar.gz
 6dbe36c348b43dbcad5c7d6c382f05ef85ccd8da 870 apparmor_2.13.2.orig.tar.gz.asc
 5ca751a41ecbbf10d661af93619708ba966f90f3 106724 apparmor_2.13.2-10.debian.tar.xz
Checksums-Sha256:
 844def9926dfda5c7858428d06e44afc80573f9706458b6e7282edbb40b11a30 7369240 apparmor_2.13.2.orig.tar.gz
 5b0fb153a28a29c0d300b390ab62b9a19a3d23634c8c3d08292181d68d8b0e8a 870 apparmor_2.13.2.orig.tar.gz.asc
 2777537b493f5e3aea89aa41ba9e7664615d3e36be2d87d5ddc63bd9c1f4bc43 106724 apparmor_2.13.2-10.debian.tar.xz
Files:
 2439b35266b5a3a461b0a2dba6e863c3 7369240 apparmor_2.13.2.orig.tar.gz
 dc758be36fdcf429f14a7048d90a3f82 870 apparmor_2.13.2.orig.tar.gz.asc
 e502da89e89963573abc5198c2cb35f1 106724 apparmor_2.13.2-10.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEHRt1x6SY9ZXd6PxEAfr306of05kFAlyfd2YACgkQAfr306of
05lgDw//bZZCJ5574qZrZqmc0rTyaAmx9oJFOqTdasV88I0L3yC9uFtVcLoQYhTy
m4eg9nvfAOkLhmwCtuTyh4CmdiQTaLK0i6skmgArr4QLK+Rh03lYjZCTtMEn1iaU
0dFBnjDSS0xIyQjpXi7wI077ZK22QNL1YPfSnZWo5UTS2NG/m88Gr5txVsb2Yvpo
RX84331n9+X4pRj2jqqMOOkKMCtWTxqZWgSifb1LEgqoD/P621/fQZ+A9rDX17DQ
Zkkj+8ur8DEV/0ZCFkcde1Q9v3+ZZWU4/Bg6AZm0folLN1cnztg3TDsGADVDzOej
4pXB5RXPe+do6ttlY0s0N34v75LgK0fFpkTgwiBrpUBCoy9mPrYGqE9/6U8wYdx3
e7RHxii5S7grze3EMuzkY0mflkqInt0hYm8IkA+cQONQMQnRM5NMzErp7GR2cv0S
eEd6Olwed15dd1bbFEL8zkqSj30sVck0SQuBTzBz8Hsm5F1CWhthkubIRhQdo+tK
grvTyqvqVf7M2JpPQ+M3PvgoZlloMMhu2sGm+Jme7ZAjCWVisai7WntdGfcR6lDH
t5FDOQp3hfD0OFWmqA/YkTpoQQYbRJ9QehtxzwDaPBWbGToC25woMPk41F9qsX2Q
zMvP1dOF+OFxR4g4p1pTPAoAx+cR141SrNYW/ajQoiCONbTDE54=
=0MiY
-----END PGP SIGNATURE-----
