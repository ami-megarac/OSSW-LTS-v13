-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: systemd
Binary: systemd, systemd-sysv, systemd-container, systemd-journal-remote, systemd-coredump, systemd-tests, libpam-systemd, libnss-myhostname, libnss-mymachines, libnss-resolve, libnss-systemd, libsystemd0, libsystemd-dev, udev, libudev1, libudev-dev, udev-udeb, libudev1-udeb
Architecture: linux-any
Version: 241-7~deb10u8
Maintainer: Debian systemd Maintainers <pkg-systemd-maintainers@lists.alioth.debian.org>
Uploaders: Michael Biebl <biebl@debian.org>, Marco d'Itri <md@linux.it>, Sjoerd Simons <sjoerd@debian.org>, Martin Pitt <mpitt@debian.org>, Felipe Sateler <fsateler@debian.org>
Homepage: https://www.freedesktop.org/wiki/Software/systemd
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/systemd-team/systemd
Vcs-Git: https://salsa.debian.org/systemd-team/systemd.git
Testsuite: autopkgtest
Testsuite-Triggers: acl, apparmor, build-essential, busybox-static, cron, cryptsetup-bin, dbus-user-session, dnsmasq-base, e2fsprogs, evemu-tools, fdisk, gcc, gdm3, iproute2, iputils-ping, isc-dhcp-client, kbd, less, libc-dev, libc6-dev, libcap2-bin, liblz4-tool, locales, make, net-tools, netcat-openbsd, network-manager, perl, pkg-config, plymouth, policykit-1, python3, qemu-system-arm, qemu-system-s390x, qemu-system-x86, quota, rsyslog, socat, strace, tree, util-linux, xserver-xorg, xserver-xorg-video-dummy, xz-utils
Build-Depends: debhelper (>= 10.4~), pkg-config, xsltproc, docbook-xsl, docbook-xml, m4, meson (>= 0.49), gettext, gperf, gnu-efi [amd64 i386 arm64], libcap-dev (>= 1:2.24-9~), libpam0g-dev, libapparmor-dev (>= 2.9.0-3+exp2) <!stage1>, libidn11-dev <!stage1>, libiptc-dev <!stage1>, libaudit-dev <!stage1>, libdbus-1-dev (>= 1.3.2) <!nocheck>, libcryptsetup-dev (>= 2:1.6.0) <!stage1>, libselinux1-dev (>= 2.1.9), libacl1-dev, liblzma-dev, liblz4-dev (>= 0.0~r125), liblz4-tool <!nocheck>, libbz2-dev <!stage1>, zlib1g-dev <!stage1> | libz-dev <!stage1>, libcurl4-gnutls-dev <!stage1> | libcurl-dev <!stage1>, libmicrohttpd-dev <!stage1>, libgnutls28-dev <!stage1>, libgcrypt20-dev, libkmod-dev (>= 15), libblkid-dev (>= 2.24), libmount-dev (>= 2.30), libseccomp-dev (>= 2.3.1) [amd64 arm64 armel armhf i386 mips mipsel mips64 mips64el x32 powerpc ppc64 ppc64el s390x], libdw-dev (>= 0.158) <!stage1>, libpolkit-gobject-1-dev <!stage1>, linux-base <!nocheck>, acl <!nocheck>, python3:native, python3-lxml:native, python3-pyparsing <!nocheck>, python3-evdev <!nocheck>, tzdata <!nocheck>, libcap2-bin <!nocheck>, iproute2 <!nocheck>
Package-List:
 libnss-myhostname deb admin optional arch=linux-any
 libnss-mymachines deb admin optional arch=linux-any
 libnss-resolve deb admin optional arch=linux-any
 libnss-systemd deb admin optional arch=linux-any
 libpam-systemd deb admin standard arch=linux-any
 libsystemd-dev deb libdevel optional arch=linux-any
 libsystemd0 deb libs optional arch=linux-any
 libudev-dev deb libdevel optional arch=linux-any
 libudev1 deb libs optional arch=linux-any
 libudev1-udeb udeb debian-installer optional arch=linux-any profile=!noudeb
 systemd deb admin important arch=linux-any
 systemd-container deb admin optional arch=linux-any profile=!stage1
 systemd-coredump deb admin optional arch=linux-any profile=!stage1
 systemd-journal-remote deb admin optional arch=linux-any profile=!stage1
 systemd-sysv deb admin important arch=linux-any
 systemd-tests deb admin optional arch=linux-any
 udev deb admin important arch=linux-any
 udev-udeb udeb debian-installer optional arch=linux-any profile=!noudeb
Checksums-Sha1:
 66378cd752881489006dde6ae5faab971af8c0e5 7640538 systemd_241.orig.tar.gz
 c7a8700c956ccbe7bab7ceb158efc9779e180e34 182616 systemd_241-7~deb10u8.debian.tar.xz
Checksums-Sha256:
 b2561a8e1d10a2c248253f0dda31a85dd6d69f2b54177de55e02cd1d2778316e 7640538 systemd_241.orig.tar.gz
 6f5419d06f917a0565a55b9c9e7b9b55c094623a32b722f24431c20f7b48491b 182616 systemd_241-7~deb10u8.debian.tar.xz
Files:
 c5953c24c850b44fcf714326e567dc37 7640538 systemd_241.orig.tar.gz
 a516260db1d405c71425b97636af396e 182616 systemd_241-7~deb10u8.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQKmBAEBCgCQFiEERkRAmAjBceBVMd3uBUy48xNDz0QFAmDz33NfFIAAAAAALgAo
aXNzdWVyLWZwckBub3RhdGlvbnMub3BlbnBncC5maWZ0aGhvcnNlbWFuLm5ldDQ2
NDQ0MDk4MDhDMTcxRTA1NTMxRERFRTA1NENCOEYzMTM0M0NGNDQSHGNhcm5pbEBk
ZWJpYW4ub3JnAAoJEAVMuPMTQ89EcroP/11pBsTSAyMkAkRLb7jXn4KpD3dvhpn8
zCBsZoOOVmr9mEPXG6DFA0uPQvbcwpyLzMMuXbFTBladMmP/QcVjBYsTYy4H32Kc
yniCVb417/BS3/wtnBz4Afi+zYGvWJ0JK2gDWZMsSFpU+/VTkyAAO6DRgJ3pkRp4
dtEtxm0LwYCV9+fp9MdWgQ4kt8dWwuyWh8NU3TdVeaAO1yPz9LDLW6s7rLr2zOaH
Aa0v1TRP7+WD3+lIpNj1rHHlt+uq/bx6AxNXO/d50rgBz2l7bqcgnuX7eOyxRjlo
curQTSB6kW+V+nRRLh629TtLdoDHnvXCULMNO01bITza6lNA0K85nGwWFAfOmMvO
R/lsHhizaKo6T3K1FtuIGs4YSjBCZV0ONbui6KZn0eAWNkw0/MR7ueUuoX9D/duM
s1f8oY5U5Rv3F+HTQseqfPS2i+C+fEYtW8NB163cXjZd+bifRP6e20409K8NsVxz
H95HClWCurfYVOF1xlzn23QVIikJrv8vuqWKmVFQ/pyzBnVlGPiyUx7sEPC5UlX+
QE45344g6h2moZknhzuFvMH1SR6SCBcK8H/XqIpCdNHOwUunItGuGCmvFkFGLP5p
SXacqy/k/GYYjSwtN4mxKLoksno6NvalCNcC8QXWYZwZQiRJBoCYH2Nl0FHRT06/
7DrMPYdpYiC8
=3uzl
-----END PGP SIGNATURE-----
