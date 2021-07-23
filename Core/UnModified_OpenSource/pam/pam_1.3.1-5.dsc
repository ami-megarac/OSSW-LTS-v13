-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: pam
Binary: libpam0g, libpam-modules, libpam-modules-bin, libpam-runtime, libpam0g-dev, libpam-cracklib, libpam-doc
Architecture: any all
Version: 1.3.1-5
Maintainer: Steve Langasek <vorlon@debian.org>
Uploaders: Sam Hartman <hartmans@debian.org>
Homepage: http://www.linux-pam.org/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/vorlon/pam
Vcs-Git: https://salsa.debian.org/vorlon/pam.git
Build-Depends: libcrack2-dev (>= 2.8), bzip2, debhelper (>= 9), quilt (>= 0.48-1), flex, libdb-dev, libselinux1-dev [linux-any], po-debconf, dh-autoreconf, autopoint, libaudit-dev [linux-any] <!stage1>, pkg-config, libfl-dev, libfl-dev:native, docbook-xsl, docbook-xml, xsltproc, libxml2-utils, w3m
Build-Conflicts: libdb4.2-dev, libxcrypt-dev
Build-Conflicts-Indep: fop
Package-List:
 libpam-cracklib deb admin optional arch=any
 libpam-doc deb doc optional arch=all
 libpam-modules deb admin required arch=any
 libpam-modules-bin deb admin required arch=any
 libpam-runtime deb admin required arch=all
 libpam0g deb libs optional arch=any
 libpam0g-dev deb libdevel optional arch=any
Checksums-Sha1:
 e89b6d279c9bf8cb495dfc0b3f3931eb50f818e9 912332 pam_1.3.1.orig.tar.xz
 946e02d9b7c0d66c8aec714bec0b9358c13aec6f 114384 pam_1.3.1-5.debian.tar.xz
Checksums-Sha256:
 eff47a4ecd833fbf18de9686632a70ee8d0794b79aecb217ebd0ce11db4cd0db 912332 pam_1.3.1.orig.tar.xz
 be2c2b27efd6bea02f9d102d7d8c58374557beb7245b2a9d75ecc829e9449f62 114384 pam_1.3.1-5.debian.tar.xz
Files:
 558ff53b0fc0563ca97f79e911822165 912332 pam_1.3.1.orig.tar.xz
 efe2fa4a833a9260a9c422abc54e953b 114384 pam_1.3.1-5.debian.tar.xz
Dgit: aa2142277bf5fb4a884c6119180e41258817705b debian archive/debian/1.3.1-5 https://git.dgit.debian.org/pam

-----BEGIN PGP SIGNATURE-----

iQJGBAEBCgAwFiEErEg/aN5yj0PyIC/KVo0w8yGyEz0FAlxlFIASHHZvcmxvbkBk
ZWJpYW4ub3JnAAoJEFaNMPMhshM9D2UP/RpnIfrdq0yJKJpBGnqwzuNiHvLzu4FZ
MPNAem0fl1SFfHAHOz99ZdXau3hSKQOi10tWNDliSmw7aHarB2c2sPNoK771g6vA
0wqd75GHfza3avVnmMsQvBaeW6b3LZdi+NW9CDUqUCQ/tLn8Uo04cfYKgC/GV/jE
pBYw2xBkNwFxESXUSU1wkjVQ1+OUkDpyNqaw4/4UKywjgz7Q26BMS3xiZQNGnlTB
gtla31a27At9O7Y1vU8J6NAkpUyUFVIEan6bILE7xOvqMu2/WV5edw4UjYQJ47o4
2NjzqOXhrXKpij0HJSGrkmdcagYDAYTTsihC8BZqB8tE6nQrbpXT6RaTSTrlARJZ
ujqEK8fdjEvQZGhevM1B0mIi4DCoQTf8I+0cyo6n6Ays79rmU/ktUg0MOq6SI4Yq
Jkrzk5FaPmBG8qIHx1mWOhFAzrAiwBaOBlhasva8IM1D5Y6gKDagJnY7UGC5PFeE
fPGAmZVLo040Gjr5qBitXcEQ2108Koe+Do2D2HuH68YnkvvM1VGY11EnzpBbPBSw
/+FdNUGQjxEUnqdgsLOHsSc8M74xbvsVXGkld8Zlc3K5+xrOxBY5yQhWprL4cAcj
uei7h3G7BNW2a60dT7Bmqb5BZHofCewiOxp3qzZSNadrSRRXMaKrPtLSWZEhZV44
KW+0MbkZ/Bdj
=P56w
-----END PGP SIGNATURE-----
