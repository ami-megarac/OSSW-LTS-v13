-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: parted
Binary: parted, parted-udeb, libparted2, libparted-fs-resize0, libparted2-udeb, libparted-fs-resize0-udeb, libparted-i18n, libparted-dev, parted-doc
Architecture: any all
Version: 3.2-25
Maintainer: Parted Maintainer Team <parted-maintainers@alioth-lists.debian.net>
Uploaders: Bastian Blank <waldi@debian.org>, Colin Watson <cjwatson@debian.org>
Homepage: http://www.gnu.org/software/parted
Standards-Version: 3.9.8
Vcs-Browser: https://salsa.debian.org/parted-team/parted
Vcs-Git: https://salsa.debian.org/parted-team/parted.git
Build-Depends: dpkg-dev (>= 1.15.7~), debhelper (>= 9.20150501~), dh-exec, libncurses-dev | libncurses5-dev, libreadline-dev | libreadline6-dev, libdevmapper-dev (>= 2:1.02.39) [linux-any], uuid-dev, gettext, texinfo (>= 4.2), debianutils (>= 1.13.1), libblkid-dev, pkg-config, check, dh-autoreconf, autoconf (>= 2.63), automake (>= 1:1.11.6), autopoint, gperf
Package-List:
 libparted-dev deb libdevel optional arch=any
 libparted-fs-resize0 deb libs optional arch=any
 libparted-fs-resize0-udeb udeb debian-installer optional arch=any
 libparted-i18n deb localization optional arch=all
 libparted2 deb libs optional arch=any
 libparted2-udeb udeb debian-installer optional arch=any
 parted deb admin optional arch=any
 parted-doc deb doc optional arch=all
 parted-udeb udeb debian-installer optional arch=any
Checksums-Sha1:
 78db6ca8dd6082c5367c8446bf6f7ae044091959 1655244 parted_3.2.orig.tar.xz
 d1118229cd6585c0fccb1ab1bac365ede1515e6b 88524 parted_3.2-25.debian.tar.xz
Checksums-Sha256:
 858b589c22297cacdf437f3baff6f04b333087521ab274f7ab677cb8c6bb78e4 1655244 parted_3.2.orig.tar.xz
 0bc8d0df18b83a806df08c364634fecc474fa24a8b367820bd2315489427b623 88524 parted_3.2-25.debian.tar.xz
Files:
 0247b6a7b314f8edeb618159fa95f9cb 1655244 parted_3.2.orig.tar.xz
 40781620101be15edb3174391a43a2db 88524 parted_3.2-25.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCAAdFiEErApP8SYRtvzPAcEROTWH2X2GUAsFAly0mGsACgkQOTWH2X2G
UAulTg/5AdX6WSjFtolZq50tnBQYVHbXK5NnftwbkZ3Bgew9DSbIko5cPCM7yJVr
4tXstoVt6uDdlNpP9/gEQ4iE9gUjQaCTIu/6A7ANtIIXdq4XNOdfrxW8XnRtEDN6
n/q7hsEdj+IahQr3MviQSMAt8/94UaJDK1dEpgDcM9eq+6KGHEG/jGM8hG6fF4jW
kiw1dBsU63nEpHlshkL5axup4X372Krso+z2ZoSrWcd/9Znd1Pfwn+isuL/0V6Ip
Rqb5cKB873cd21LoTwnz9KRcz3+/VGjB0eJ+/ug9yjNF2xM0t5UwufvOWQlvU6zh
evMsRDpbDutNpOaFMh0EwsQbkaD/4BZFATa4SSp0JAViWFtvrgOBPZq0jvHh9rxl
1wUKId2WTnQt1QDkbdN1yIOIECBSeYSWF2+1X7xF299RBvS2nkMVB6ShyhPTiMo+
YmqtAe/gDE2KnIQSRKYkoELrzglCfifb9JzRi4S8i0R2I+m3XYoEgH7Sz6zqx9my
nUfB02gLNEAtuQBYZgYoIahV9ALC0B8pvwnAbVpGLdVk9bPSmtZ4rAuvlOyegtC9
lt9xgHLlH7qOEsl65VJwHLRAzwW5iehCxyAHzg/Y/NlI+IybCZtAxvZ5IVUr3mX/
nXYqeFvWEafntssbTPGtg83c3Xk/XG7y/LtYMLrDHnaYGz2lR0c=
=MZnm
-----END PGP SIGNATURE-----
