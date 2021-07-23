-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: libpsl
Binary: libpsl-dev, libpsl5, psl, psl-make-dafsa
Architecture: any all
Version: 0.20.2-2
Maintainer: Tim Rühsen <tim.ruehsen@gmx.de>
Uploaders:  Daniel Kahn Gillmor <dkg@fifthhorseman.net>,
Homepage: https://github.com/rockdaboot/libpsl
Standards-Version: 4.2.1
Vcs-Browser: https://salsa.debian.org/debian/libpsl
Vcs-Git: https://salsa.debian.org/debian/libpsl.git
Testsuite: autopkgtest
Testsuite-Triggers: diffutils, publicsuffix
Build-Depends: debhelper (>= 11~)
Build-Depends-Arch: autoconf-archive, automake, gtk-doc-tools, libidn2-0-dev (>= 0.16), libunistring-dev, pkg-config, publicsuffix (>= 20150507), python3:any
Package-List:
 libpsl-dev deb libdevel optional arch=any
 libpsl5 deb libs optional arch=any
 psl deb net optional arch=any
 psl-make-dafsa deb net optional arch=all
Checksums-Sha1:
 9e4e6d6035a483cadd8894f34080ae18a803182d 8590430 libpsl_0.20.2.orig.tar.gz
 34f78bddb5af71fe91ca6c7f02efc89b5f87427c 9920 libpsl_0.20.2-2.debian.tar.xz
Checksums-Sha256:
 94d2b5e00e9aa761ae7efbaa67edc00d5298487ed9706eb4789e349012993c31 8590430 libpsl_0.20.2.orig.tar.gz
 1f008454fdb973964202020fb700d5028e001b7eaa4e77eeab8ebc99b749ea51 9920 libpsl_0.20.2-2.debian.tar.xz
Files:
 e0161bb222758220d05ad7c4d16f8e4e 8590430 libpsl_0.20.2.orig.tar.gz
 1f00ea627709de6f9e108b0556f2b8bb 9920 libpsl_0.20.2-2.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iHUEARYKAB0WIQTTaP514aqS9uSbmdJsHx7ezFD6UwUCW8TmmwAKCRBsHx7ezFD6
U+PdAQDbh7fFm2SggN8bm75WwQ/Z+6hLpw2jw/4P8ZswbiJPPgD/bb4f/X5xrCpF
l/4CXlUQ8O9kZE1Dbu2zkMGyjAKI2AQ=
=PrBP
-----END PGP SIGNATURE-----
