-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: avahi
Binary: avahi-daemon, avahi-dnsconfd, avahi-autoipd, python-avahi, avahi-utils, avahi-discover, libavahi-common3, libavahi-common-data, libavahi-common-dev, libavahi-core7, libavahi-core-dev, libavahi-client3, libavahi-client-dev, libavahi-glib1, libavahi-glib-dev, libavahi-gobject0, libavahi-gobject-dev, libavahi-compat-libdnssd1, libavahi-compat-libdnssd-dev, libavahi-ui-gtk3-0, libavahi-ui-gtk3-dev, avahi-ui-utils, gir1.2-avahi-0.6
Architecture: any all
Version: 0.7-4+deb10u1
Maintainer: Utopia Maintenance Team <pkg-utopia-maintainers@lists.alioth.debian.org>
Uploaders: Sjoerd Simons <sjoerd@debian.org>, Sebastian Dröge <slomo@debian.org>, Loic Minier <lool@dooz.org>, Michael Biebl <biebl@debian.org>
Homepage: http://avahi.org/
Standards-Version: 4.1.4
Vcs-Browser: https://salsa.debian.org/utopia-team/avahi
Vcs-Git: https://salsa.debian.org/utopia-team/avahi.git
Build-Depends: debhelper (>= 10.3), dh-python, pkg-config, libcap-dev (>= 1:2.16) [linux-any], libgdbm-dev, libglib2.0-dev (>= 2.4), libgtk-3-dev <!stage1>, libexpat-dev, libdaemon-dev (>= 0.11), libdbus-1-dev (>= 0.60), python-all-dev (>= 2.6.6-3~), python-gdbm (>= 2.4.3), python-dbus <!stage1>, python-gi-dev <!stage1>, gobject-introspection, libgirepository1.0-dev, xmltoman, intltool (>= 0.35.0)
Package-List:
 avahi-autoipd deb net optional arch=linux-any
 avahi-daemon deb net optional arch=any
 avahi-discover deb net optional arch=all profile=!stage1
 avahi-dnsconfd deb net optional arch=any
 avahi-ui-utils deb utils optional arch=any profile=!stage1
 avahi-utils deb net optional arch=any
 gir1.2-avahi-0.6 deb introspection optional arch=any
 libavahi-client-dev deb libdevel optional arch=any
 libavahi-client3 deb libs optional arch=any
 libavahi-common-data deb libs optional arch=any
 libavahi-common-dev deb libdevel optional arch=any
 libavahi-common3 deb libs optional arch=any
 libavahi-compat-libdnssd-dev deb libdevel optional arch=any
 libavahi-compat-libdnssd1 deb libs optional arch=any
 libavahi-core-dev deb libdevel optional arch=any
 libavahi-core7 deb libs optional arch=any
 libavahi-glib-dev deb libdevel optional arch=any
 libavahi-glib1 deb libs optional arch=any
 libavahi-gobject-dev deb libdevel optional arch=any
 libavahi-gobject0 deb libs optional arch=any
 libavahi-ui-gtk3-0 deb libs optional arch=any profile=!stage1
 libavahi-ui-gtk3-dev deb libdevel optional arch=any profile=!stage1
 python-avahi deb python optional arch=any profile=!stage1
Checksums-Sha1:
 8a062878968c0f8e083046429647ad33b122542f 1333400 avahi_0.7.orig.tar.gz
 b627d6c1e4e85ff2f196e71edbddf77a156790a5 30532 avahi_0.7-4+deb10u1.debian.tar.xz
Checksums-Sha256:
 57a99b5dfe7fdae794e3d1ee7a62973a368e91e414bd0dfa5d84434de5b14804 1333400 avahi_0.7.orig.tar.gz
 7f7244a5728b58565192004e2f98b88e03d2e66a0f6320f885e53eaa41cfc61e 30532 avahi_0.7-4+deb10u1.debian.tar.xz
Files:
 d76c59d0882ac6c256d70a2a585362a6 1333400 avahi_0.7.orig.tar.gz
 f74489d63432eaa68266238f4df48a64 30532 avahi_0.7-4+deb10u1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEKHCjG+qdvPJ0cjEIwnTbZMIwD3sFAmApK88ACgkQwnTbZMIw
D3tLEhAAp2MSOYndC5B7I2E90lnpc4vHmMZWS+DbJAdlcSIYTLbwFnala7Tl8c3p
fv7DGM7ltOpRY8NDPtBMOyUUIak+2h9JJbKhREHGV8W7y7aEUfI2XJgL+gU5Xsk6
uLvIYkb+ml4xBpjSwzJZ2W34/YGvwbn64HWu1UjnCIvM0QID23/wwBRM4rXbYWlX
NWfZ4Tgl3FFRcGEPXjwLNwWUxcmaoRagtEeQuoP0IxWCGUR/AxzySfEAgZAFO1SG
0vVbdQvbinhIIGdeECeTBXJuBMyPmnyxCfy+BoxAz5GpNGLBAgSSf32AVUJvkjB/
oyXbJP1o3VKn7KFNITJUZd9uZtTW0Shz4B3TDcSt/cy7aRWZ9brgfqYKApoE76c5
8k8HWRPyegShFPP9GOLeNN2nEdNO1gBA2OS/PHJGXvsrPru6VUC9nFaTFsKOPFMm
G6s6GNjNUbVcDCpsniNSf+Euf5s5IaspFCa1C53O4ec9VkspBsmucyVuaW1RmxmG
I9kItuDGB2YD9cg451LsEgDPVy32A1oc9t/HEjyF09VYtKenJmMQb7C035eCWs2D
qd7RAJ+NCGx676YjFZoIdaThu1UKgq8fpEWunOkQtATv8p08eCNVtYf0vkhk8/sh
PcsjqL4FNTlBn/A/9jx/8QiAIZpnFKmLvaRB2LVyA1+M49gbuiA=
=Bdwo
-----END PGP SIGNATURE-----
