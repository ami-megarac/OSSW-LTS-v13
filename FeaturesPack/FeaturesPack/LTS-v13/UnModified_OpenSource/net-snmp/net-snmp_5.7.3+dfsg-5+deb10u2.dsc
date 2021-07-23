-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: net-snmp
Binary: snmpd, snmptrapd, snmp, libsnmp-base, libsnmp30, libsnmp30-dbg, libsnmp-dev, libsnmp-perl, python-netsnmp, tkmib
Architecture: any all
Version: 5.7.3+dfsg-5+deb10u2
Maintainer: Net-SNMP Packaging Team <pkg-net-snmp-devel@lists.alioth.debian.org>
Uploaders: Craig Small <csmall@debian.org>, Thomas Anders <tanders@users.sourceforge.net>, Noah Meyerhans <noahm@debian.org>
Homepage: http://net-snmp.sourceforge.net/
Standards-Version: 4.1.3
Vcs-Browser: https://salsa.debian.org/debian/net-snmp
Vcs-Git: https://salsa.debian.org/debian/net-snmp.git
Build-Depends: debhelper (>= 11), libtool, libwrap0-dev, libssl-dev, perl (>= 5.8), libperl-dev, python-all (>= 2.6.6-3~), python-setuptools (>= 0.6b3), python2.7-dev, autoconf, automake, debianutils (>= 1.13.1), dh-python, bash (>= 2.05), findutils (>= 4.1.20), procps, pkg-config [kfreebsd-i386 kfreebsd-amd64], libbsd-dev [kfreebsd-i386 kfreebsd-amd64], libkvm-dev [kfreebsd-i386 kfreebsd-amd64], libsensors4-dev [!hurd-i386 !kfreebsd-i386 !kfreebsd-amd64], default-libmysqlclient-dev, libpci-dev
Build-Conflicts: libsnmp-dev
Package-List:
 libsnmp-base deb libs optional arch=all
 libsnmp-dev deb libdevel optional arch=any
 libsnmp-perl deb perl optional arch=any
 libsnmp30 deb libs optional arch=any
 libsnmp30-dbg deb debug optional arch=any
 python-netsnmp deb python optional arch=any
 snmp deb net optional arch=any
 snmpd deb net optional arch=any
 snmptrapd deb net optional arch=any
 tkmib deb net optional arch=all
Checksums-Sha1:
 ebbbc5e9fc5006edd3e62d595366497592d964a2 3371224 net-snmp_5.7.3+dfsg.orig.tar.xz
 a7a8702c5e1a21855402f8cb57ecad82d2da7911 83716 net-snmp_5.7.3+dfsg-5+deb10u2.debian.tar.xz
Checksums-Sha256:
 073eb05b926a9d23a2eba3270c4e52dd94c0aa27e8b7cf7f1a4e59a4d3da3fb5 3371224 net-snmp_5.7.3+dfsg.orig.tar.xz
 a176612ac83e2b80e64a17a605c74b259b0eca3eaeb77ab6b17dd04d23e1b7be 83716 net-snmp_5.7.3+dfsg-5+deb10u2.debian.tar.xz
Files:
 6391ae27eb1ae34ff5530712bb1c4209 3371224 net-snmp_5.7.3+dfsg.orig.tar.xz
 b479f583dd9695948cffc87b5f09c2da 83716 net-snmp_5.7.3+dfsg-5+deb10u2.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEXT3w9TizJ8CqeneiAiFmwP88hOMFAmASiy4ACgkQAiFmwP88
hOMkgg//daczGaqX0THQs5OXnzbYWeCoN/PiVfgOJUlWt++forhiBqi8a85iUa4J
A++gA1N9IE1CnYDzLUnsyECR/f4p/dUmowZ/uJGa08Bl24Ce5F42Cg+pKemmGkmS
jPS9t3K5qUZfeETJ2joJgu+woYkxDHk8gpVjP/0MQmh41OHzE4m+7YWlcnlIHuqm
cobqsknlLLNSbXwQ4O57xkmKGPpA1/ajKZHwxjutpa3m34S0X1yZR0xs7yyUudJ/
Ph8wcs2uCTjs7pXltCFRn7WH8aD610arflZKk5am4sq4G3UgR434S53Dq/YtcKq9
HYozRfCYw+alqOTUJehc5I1C6WqtRonYh7asbcEsfMwCA2poFvef5AjuTAb52Gh7
TLSuYjKMpC/THf62bm5W+MaIqIZ42QSOGFZR7vxUSN6Vg/R2rFsJikC9Yw19Q17V
ZOAmbwtaHsPrlAcPZpSp6lf5FjPG+cM2AVRKSxedUqZ8gYw45VcWp7EAnpN57PIF
bFtbo581dMGZ4BBRhn+PQ2XmLcVTX06M6P2dSl44Mn5NqQdhFgCgkaJzx0Q17alX
XMXeYINsyMiw4XpWuc5/vVIuFymHjvxaWmwUw7oG4Lb1mceF5cYszOw1+qsGI1kA
fBY3zsXeQ27+2hQq+GFXqqz8w5uJYSs7DFbjhl4aSFzZrXp8nt8=
=+cgt
-----END PGP SIGNATURE-----
