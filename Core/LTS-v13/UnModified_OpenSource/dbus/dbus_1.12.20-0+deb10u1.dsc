-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: dbus
Binary: dbus, dbus-1-doc, dbus-tests, dbus-udeb, dbus-user-session, dbus-x11, libdbus-1-3, libdbus-1-3-udeb, libdbus-1-dev
Architecture: any all
Version: 1.12.20-0+deb10u1
Maintainer: Utopia Maintenance Team <pkg-utopia-maintainers@lists.alioth.debian.org>
Uploaders: Sjoerd Simons <sjoerd@debian.org>, Sebastian Dröge <slomo@debian.org>, Michael Biebl <biebl@debian.org>, Loic Minier <lool@dooz.org>, Simon McVittie <smcv@debian.org>,
Homepage: http://dbus.freedesktop.org/
Standards-Version: 4.2.1
Vcs-Browser: https://salsa.debian.org/utopia-team/dbus
Vcs-Git: https://salsa.debian.org/utopia-team/dbus.git
Testsuite: autopkgtest
Testsuite-Triggers: apparmor, build-essential, gnome-desktop-testing, init, xauth, xvfb
Build-Depends: autoconf-archive (>= 20150224), automake, debhelper (>= 11.1~), dh-exec, libapparmor-dev [linux-any] <!pkg.dbus.minimal>, libaudit-dev [linux-any] <!pkg.dbus.minimal>, libcap-ng-dev [linux-any] <!pkg.dbus.minimal>, libexpat-dev, libglib2.0-dev <!pkg.dbus.minimal>, libnss-wrapper <!nocheck>, libselinux1-dev [linux-any] <!pkg.dbus.minimal>, libsystemd-dev [linux-any] <!pkg.dbus.minimal>, libx11-dev <!pkg.dbus.minimal>, python3 <!nocheck !pkg.dbus.minimal>, python3-dbus <!nocheck !pkg.dbus.minimal>, python3-gi <!nocheck !pkg.dbus.minimal>, valgrind [amd64 arm64 armhf i386 mips64 mips64el mips mipsel powerpc ppc64 ppc64el s390x] <!pkg.dbus.minimal>, xmlto <!nodoc !pkg.dbus.minimal>
Build-Depends-Indep: doxygen <!nodoc>, ducktype <!nodoc>, xsltproc <!nodoc>, yelp-tools <!nodoc>
Package-List:
 dbus deb admin standard arch=any
 dbus-1-doc deb doc optional arch=all profile=!nodoc,!pkg.dbus.minimal
 dbus-tests deb misc optional arch=any profile=!pkg.dbus.minimal
 dbus-udeb udeb debian-installer optional arch=any profile=!noudeb
 dbus-user-session deb admin optional arch=linux-any profile=!pkg.dbus.minimal
 dbus-x11 deb x11 optional arch=any profile=!pkg.dbus.minimal
 libdbus-1-3 deb libs optional arch=any
 libdbus-1-3-udeb udeb debian-installer optional arch=any profile=!noudeb
 libdbus-1-dev deb libdevel optional arch=any
Checksums-Sha1:
 f7fe130511aeeac40270af38d6892ed63392c7f6 2095511 dbus_1.12.20.orig.tar.gz
 598f9a0e8017ddea5838e9b9e9074f9d09c4f116 833 dbus_1.12.20.orig.tar.gz.asc
 b826e25a3b7eb5727f1310e5445bc7e4a0fadf28 63300 dbus_1.12.20-0+deb10u1.debian.tar.xz
Checksums-Sha256:
 f77620140ecb4cdc67f37fb444f8a6bea70b5b6461f12f1cbe2cec60fa7de5fe 2095511 dbus_1.12.20.orig.tar.gz
 a5f4d51c9c95a6cf7270abb6548894d91d51eebc0e9f996d0951c8ee925894e7 833 dbus_1.12.20.orig.tar.gz.asc
 52dafb74ae52ab16159635db7762bdd41c584e292d3e93f84872b47df6004f49 63300 dbus_1.12.20-0+deb10u1.debian.tar.xz
Files:
 dfe8a71f412e0b53be26ed4fbfdc91c4 2095511 dbus_1.12.20.orig.tar.gz
 0a4179092214077fdf79d2fd23eb8f41 833 dbus_1.12.20.orig.tar.gz.asc
 06dbb10f66d9e5633c8b6903027754d4 63300 dbus_1.12.20-0+deb10u1.debian.tar.xz
Dgit: 692b80b30f5ba03103be8a0f0b590237fc8afb54 debian archive/debian/1.12.20-0+deb10u1 https://git.dgit.debian.org/dbus

-----BEGIN PGP SIGNATURE-----

iQJEBAEBCAAuFiEENuxaZEik9e95vv6Y4FrhR4+BTE8FAl8EQBsQHHNtY3ZAZGVi
aWFuLm9yZwAKCRDgWuFHj4FMT27oD/99+2PnpEFG9Mw6/fLTLnahNL/a1bkKmHzN
49n68gnUN03bl4xKVOB+QyfmwLjm31ddh0fC1uQFUzlKuhy6JcqJK/dra576E1JZ
Ntafb4Y2ZOsORD1pcHcut+IFxg9gQSCfq1gJQlHbnOwZv57lJIwnz3k+eLMKEOh+
uuMszMqf0uxj5czHoTWkxt6/onR5qzp/bpBHnzSo0CbQRo8TDLJLQg/0MzvyfSrA
5OjMXQeDAf6hD3KIVgFMeEmOe4TCafR24WenI8Jl4TY6gF7txGAuHlp8pj/O19Bt
g/NQMS0JbfvZ656EI34Cq0U6KGWki26f38tQPk3nh/Mvm8KtzuCQ1h+7Kwb+IPOJ
su2iUOJD4jkJAmZ8ZnPyJu0ehRaxElgnH2jSsQ5c2nno/HkkTY3kK5fJlQxpOkpk
CrW4CbpJ38i5ypn1tigiVrZle2MHNzgWY5LSzJZjpY57JRoOPsbjl44qzZNtHFrR
nG6zRk1yiHMcqHoxb8D0qnQNtpF2mEHhf5vIXfqbOJBS5wvL5F3uwGlj0jwGM85i
RpKG63yrLtekh7MDlyTQo0CO6U/jEDXXeSWqza9UE+Ux/UqZ3HZ4tWP8T9gSn808
DHwLSx17aK+OgOZaEdwcSjWO6NIBfL8nQtIKo/yteYG5hTbnu1/azs1g+3s55l+V
KPEZikYp1A==
=zVZC
-----END PGP SIGNATURE-----
