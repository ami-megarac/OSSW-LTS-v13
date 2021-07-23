-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: gnutls28
Binary: libgnutls28-dev, libgnutls30, gnutls-bin, gnutls-doc, libgnutlsxx28, libgnutls-openssl27, libgnutls-dane0
Architecture: any all
Version: 3.6.7-4+deb10u6
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
 fbf94198f7055f3c3159c97ff07e2d718079b12f 90436 gnutls28_3.6.7-4+deb10u6.debian.tar.xz
Checksums-Sha256:
 5b3409ad5aaf239808730d1ee12fdcd148c0be00262c7edf157af655a8a188e2 8153728 gnutls28_3.6.7.orig.tar.xz
 a14d0a7b9295b65ae797a70f8e765024a2e363dca03d008bfce0aec2b3f292b0 534 gnutls28_3.6.7.orig.tar.xz.asc
 6158e0ebb12a23ca44ce90d8e677af355d04bdf66d97ca1d5a59049c1b455b3c 90436 gnutls28_3.6.7-4+deb10u6.debian.tar.xz
Files:
 c4ac669c500df939d4fbfea722367929 8153728 gnutls28_3.6.7.orig.tar.xz
 13b4d4d680c451c29129191ae9250529 534 gnutls28_3.6.7.orig.tar.xz.asc
 7a92f337c6e9dbf33aa29d350bbe3a98 90436 gnutls28_3.6.7-4+deb10u6.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIyBAEBCgAdFiEE0uCSA5741Jbt9PpepU8BhUOCFIQFAl/wrroACgkQpU8BhUOC
FITgPg/2Nmf42Kzpo3pPtyvWXJI66NM6Q9JDkGN4BlFlo6Vj/q5+0T5iLbJBVs+m
z89plb7fDMsSwszzQ6UmC80wtgmIQgMwBmoA4xS0aL+nJR6RFFb44lNuO/Xhdg2k
B5V5B40L3F1u0KAo8GPlAYwglj0clsXbH/H30Ee3G/PYRmlGGmnwNfomXsoeBHdk
qjgWM2K1cElXFQoEFtT7mwOjwbL7nJAUDIcrTrhyBJ56yYvaknxGmSIOaTKWrDmQ
0GjBgpOSPJZTG6YU+zZSeP4kX40IhxdXqIqN4GzgTYjr1DsjLnM8zLscQsJfCGV/
DCpZU+m/ToF/xEvBFkSlnJRQ5qwzt7Sdmde+dk15uOzMvCPLnPlLTBtfL7lLzyCl
JeLVhl4fjykOsYFzm7O4VtIxohYip88QTHtnlM63HN1igdTXqhKnyFvEbAYldyoS
NymtKKIbBQNCK1X+GITfKTdNs3pzVB5LL/8yPo/idPXtdNcuvzIJpiyweTbUtdYc
o6PNoJoOLlWbXBgJmvE3gYjZeqF+F+w2+72LhmiuBVN8Sby3TTal2J5o0Jtv/sRw
ihhH9tjQaFF+EWyQaHyMXQgyqQsgtbqUv72L6uRd1grTyukyW4LcCCI1xdPGbJJ4
l4G1ktLabjXP1h4X7vylB+5CtqgPJTx3xA9W6bhUEbRWrHNcKw==
=DP/s
-----END PGP SIGNATURE-----
