-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: cyrus-sasl2
Binary: sasl2-bin, cyrus-sasl2-doc, libsasl2-2, libsasl2-modules, libsasl2-modules-db, libsasl2-modules-ldap, libsasl2-modules-otp, libsasl2-modules-sql, libsasl2-modules-gssapi-mit, libsasl2-dev, libsasl2-modules-gssapi-heimdal
Architecture: any all
Version: 2.1.27+dfsg-1+deb10u1
Maintainer: Debian Cyrus Team <team+cyrus@tracker.debian.org>
Uploaders: Fabian Fagerholm <fabbe@debian.org>, Roberto C. Sanchez <roberto@debian.org>, Ondřej Surý <ondrej@debian.org>, Adam Conrad <adconrad@0c3.net>
Homepage: https://www.cyrusimap.org/sasl/
Standards-Version: 4.3.0.1
Vcs-Browser: https://salsa.debian.org/debian/cyrus-sasl2
Vcs-Git: https://salsa.debian.org/debian/cyrus-sasl2.git
Build-Depends: chrpath, debhelper-compat (= 12), default-libmysqlclient-dev <!pkg.cyrus-sasl2.nosql> | libmysqlclient-dev <!pkg.cyrus-sasl2.nosql>, docbook-to-man, groff-base, heimdal-multidev <!pkg.cyrus-sasl2.nogssapi>, krb5-multidev <!pkg.cyrus-sasl2.nogssapi>, libdb-dev, libkrb5-dev <!pkg.cyrus-sasl2.nogssapi>, libldap2-dev <!pkg.cyrus-sasl2.noldap>, libpam0g-dev, libpod-pom-view-restructured-perl, libpq-dev <!pkg.cyrus-sasl2.nosql>, libsqlite3-dev, libssl-dev, po-debconf, python3-sphinx, quilt
Build-Conflicts: heimdal-dev
Package-List:
 cyrus-sasl2-doc deb doc optional arch=all
 libsasl2-2 deb libs standard arch=any
 libsasl2-dev deb libdevel optional arch=any
 libsasl2-modules deb libs optional arch=any
 libsasl2-modules-db deb libs standard arch=any
 libsasl2-modules-gssapi-heimdal deb libs optional arch=any profile=!pkg.cyrus-sasl2.nogssapi
 libsasl2-modules-gssapi-mit deb libs optional arch=any profile=!pkg.cyrus-sasl2.nogssapi
 libsasl2-modules-ldap deb libs optional arch=any profile=!pkg.cyrus-sasl2.noldap
 libsasl2-modules-otp deb libs optional arch=any
 libsasl2-modules-sql deb libs optional arch=any profile=!pkg.cyrus-sasl2.nosql
 sasl2-bin deb utils optional arch=any
Checksums-Sha1:
 6da3baff1685e96b93b46cdd47e13ecc34a632df 2058596 cyrus-sasl2_2.1.27+dfsg.orig.tar.xz
 7535cdb01b04cfa4b2a5d9619aa2e837f0291dc9 99972 cyrus-sasl2_2.1.27+dfsg-1+deb10u1.debian.tar.xz
Checksums-Sha256:
 108b0c691c423837264f05abb559ea76c3dfdd91246555e8abe87c129a6e37cd 2058596 cyrus-sasl2_2.1.27+dfsg.orig.tar.xz
 df71d3cd6c623702c5daeab440c91899c8d4e7955cf632e6bd07de3a65cb8538 99972 cyrus-sasl2_2.1.27+dfsg-1+deb10u1.debian.tar.xz
Files:
 ce30955361d1cdde3c31d0ee742e338d 2058596 cyrus-sasl2_2.1.27+dfsg.orig.tar.xz
 ba6707c9b3f82742a8b25d5d95fd6dd3 99972 cyrus-sasl2_2.1.27+dfsg-1+deb10u1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQKmBAEBCgCQFiEERkRAmAjBceBVMd3uBUy48xNDz0QFAl379AFfFIAAAAAALgAo
aXNzdWVyLWZwckBub3RhdGlvbnMub3BlbnBncC5maWZ0aGhvcnNlbWFuLm5ldDQ2
NDQ0MDk4MDhDMTcxRTA1NTMxRERFRTA1NENCOEYzMTM0M0NGNDQSHGNhcm5pbEBk
ZWJpYW4ub3JnAAoJEAVMuPMTQ89EQ6AP/i7NbOh4pQfKmmio4vy/7tj2hpo6cZKU
86Nt9aWV6YzclS9h5lTOGO3zoubwxyVVkOWyBGKghT1V+5S6g3DvCAc95cMBn8WL
cjvxDFlnhjB9kX9foFc11iexq2pokQses/gvXfohIXhuRU4T5doAxz4T10qxY1or
/du9U6QZulGA+A1GvL0jPzS+Ru38xUUd3c6+PQ0bzqbp3Itp57iMTLPCdDDIbsSy
eCut6mXPW9bNIkEmwRZrQ0jA7C2Y9r3FfmNhpc8TH5M4zpWhroJ51prWNSC8aRhQ
R4DmEr2gG9bk8QDYW3KQGQrlijSekXjFVxtJ+VL8kBiA1bJ1cjVBl1S1LDEh3/sk
9nqPSyrihQ/s8b2vvEvG/2tVX/r6q+WHGy0waKCkUeUId7RMcrgdsNiW7IdhnWqx
dOnf+zcP3gZjMrYMfKx3+D515pWpbYTmSGTfmMbZO8TNIzuKu7g4uNEsm0uEwIDy
vgKtuZBZSSoQ7c47aNxKKC8i6UAXDi36uMfy4i2js8rfzskQPS6fku1iU/6qFA65
DJB3RIcORUq8Q20xiZTypl3WATqzvTaZOa39IJ/L9GNQ/u5GfsHv0hhSqecg245x
O9C2FPpSScA19QEMf7TpAu9nG7cXEC89DjyHSJAfAWvn7iuM5Ma/9nWXKedY0jk6
G8yFEVNv/UUk
=h4cq
-----END PGP SIGNATURE-----
