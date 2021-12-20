-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: gnutls28
Binary: libgnutls28-dev, libgnutls30, gnutls-bin, gnutls-doc, libgnutlsxx28, libgnutls-openssl27, libgnutls-dane0
Architecture: any all
Version: 3.6.7-4+deb10u7
Maintainer: Debian GnuTLS Maintainers <pkg-gnutls-maint@lists.alioth.debian.org>
Uploaders:  Andreas Metzler <ametzler@debian.org>, Eric Dorland <eric@debian.org>, James Westby <jw+debian@jameswestby.net>, Simon Josefsson <simon@josefsson.org>
Homepage: https://www.gnutls.org/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/gnutls-team/gnutls
Vcs-Git: https://salsa.debian.org/gnutls-team/gnutls.git
Testsuite: autopkgtest
Testsuite-Triggers: ca-certificates, datefudge, freebsd-net-tools, net-tools, openssl, softhsm2
Build-Depends: autogen (>= 1:5.16-0), bison, ca-certificates <!nocheck>, chrpath, datefudge <!nocheck>, debhelper (>= 10), freebsd-net-tools [kfreebsd-i386 kfreebsd-amd64] <!nocheck>, libcmocka-dev <!nocheck>, libgmp-dev (>= 2:6), libidn2-dev, libopts25-dev, libp11-kit-dev (>= 0.23.10), libssl-dev <!nocheck>, libtasn1-6-dev (>= 4.9), libunbound-dev (>= 1.5.10-1), libunistring-dev (>= 0.9.7), net-tools [!kfreebsd-i386 !kfreebsd-amd64] <!nocheck>, nettle-dev (>= 3.4.1~rc1), openssl <!nocheck>, pkg-config, softhsm2 <!nocheck>
Build-Depends-Indep: gtk-doc-tools, texinfo (>= 4.8)
Build-Conflicts: libgnutls-dev
Package-List:
 gnutls-bin deb net optional arch=any
 gnutls-doc deb doc optional arch=all
 libgnutls-dane0 deb libs optional arch=any
 libgnutls-openssl27 deb libs optional arch=any
 libgnutls28-dev deb libdevel optional arch=any
 libgnutls30 deb libs optional arch=any
 libgnutlsxx28 deb libs optional arch=any
Checksums-Sha1:
 71f73b9829e44c947bb668b25b8b2e594a065345 8153728 gnutls28_3.6.7.orig.tar.xz
 5911d8f00c70e65d27f8d5244c37ae3b04b6cae7 534 gnutls28_3.6.7.orig.tar.xz.asc
 825902146b9c4327a6c2c463f069923ec2acf6e0 94000 gnutls28_3.6.7-4+deb10u7.debian.tar.xz
Checksums-Sha256:
 5b3409ad5aaf239808730d1ee12fdcd148c0be00262c7edf157af655a8a188e2 8153728 gnutls28_3.6.7.orig.tar.xz
 a14d0a7b9295b65ae797a70f8e765024a2e363dca03d008bfce0aec2b3f292b0 534 gnutls28_3.6.7.orig.tar.xz.asc
 4f399badd85387e1dd42c811e16d10c4c22196e57142a7325ec44c52b3c6a168 94000 gnutls28_3.6.7-4+deb10u7.debian.tar.xz
Files:
 c4ac669c500df939d4fbfea722367929 8153728 gnutls28_3.6.7.orig.tar.xz
 13b4d4d680c451c29129191ae9250529 534 gnutls28_3.6.7.orig.tar.xz.asc
 e485ece5bac5eca4d5d183943953e515 94000 gnutls28_3.6.7-4+deb10u7.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEE0uCSA5741Jbt9PpepU8BhUOCFIQFAmCf9DsACgkQpU8BhUOC
FIR7xw//UQaqSgvttAaan86bbsxIt6E83CnKn8G5CGr5nKuCszSFYfaK2oNBS3Kv
4bt5UzMk+EeTMW5EATca1AIJhtZmhZc7eYwq0otB6noFonboTcczzG3NKmmnmPIH
3b3pyoM4hwNjdW4S2E0pmTUSVmSXIC0PFHGAKtZfAZl+Pcn1OtCgOaLCVg1cd6FH
lcA+O+uFAafo/pOJQxtuk4um0+gc6nz/CXI9Txdrv5BAe+Z0ivbZZs50lCE/slGz
XS0Hqtq/PTzGDHbSA2mfAF9mg91JSv6JJAActjkO6FLcTNsOgz5Dnh9ZdHUxItrn
/QMufJjEy1Rfje8I0WhOJmnqKDscChli8Un2RmVPULUnu+r7LYiMVD9kNJinLMh+
7intxHbH3m0btnaDbK+/S8RS/ipXjsooVegZ3NO3OYXOGBj1B0Gs1DwDTR3oNF/z
poaYHLy+/Hnbz5uquvvaNnzQJIg2RiHKABv+F9GKGmjcnX+Wd2va1Y+s/tWk84yp
oTinc4/g4YORCDk1j9zoPdR8Uzo6yFQdJj96J2w153kCGnmFokkQQS/DO43JXRwg
9LTnrohLMXGVjqI41ov/pEW5A7ED8oJEESOB6asBMlnks/a8LGvDAONusI4fovit
imlGbhbFuNZef0dU+LQgGGPk+Gmm0gdAj7O92U2IWbpCglPaaUg=
=nk2K
-----END PGP SIGNATURE-----
