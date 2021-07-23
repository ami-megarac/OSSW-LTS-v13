-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: db5.3
Binary: db5.3-doc, libdb5.3-dev, libdb5.3, db5.3-util, db5.3-sql-util, libdb5.3++, libdb5.3++-dev, libdb5.3-tcl, libdb5.3-dbg, libdb5.3-java-jni, libdb5.3-java, libdb5.3-java-dev, libdb5.3-sql-dev, libdb5.3-sql, libdb5.3-stl-dev, libdb5.3-stl
Architecture: any all
Version: 5.3.28+dfsg1-0.5
Maintainer: Debian Berkeley DB Team <team+bdb@tracker.debian.org>
Uploaders: Ondřej Surý <ondrej@debian.org>, Dmitrijs Ledkovs <xnox@debian.org>
Homepage: http://www.oracle.com/technetwork/database/database-technologies/berkeleydb/overview/index.html
Standards-Version: 3.9.6
Vcs-Browser: https://salsa.debian.org/debian/db5.3
Vcs-Git: https://salsa.debian.org/debian/db5.3.git
Build-Depends: debhelper (>= 10), autotools-dev, dh-autoreconf, tcl <cross !pkg.db5.3.notcl>, tcl-dev <!pkg.db5.3.notcl>, procps [!hurd-i386] <!nocheck>, javahelper <!nojava>, default-jdk <!nojava>
Package-List:
 db5.3-doc deb doc optional arch=all
 db5.3-sql-util deb database extra arch=any
 db5.3-util deb database optional arch=any
 libdb5.3 deb libs standard arch=any
 libdb5.3++ deb libs optional arch=any
 libdb5.3++-dev deb libdevel extra arch=any
 libdb5.3-dbg deb debug extra arch=any
 libdb5.3-dev deb libdevel extra arch=any
 libdb5.3-java deb java optional arch=all profile=!nojava
 libdb5.3-java-dev deb libdevel optional arch=any profile=!nojava
 libdb5.3-java-jni deb java optional arch=any profile=!nojava
 libdb5.3-sql deb libs extra arch=any profile=!pkg.db5.3.nosql
 libdb5.3-sql-dev deb libdevel extra arch=any
 libdb5.3-stl deb libs extra arch=any
 libdb5.3-stl-dev deb libdevel extra arch=any
 libdb5.3-tcl deb interpreters extra arch=any profile=!pkg.db5.3.notcl
Checksums-Sha1:
 98d30e5ba942cc4a818ac29270ac72a3ffc2c374 19723860 db5.3_5.3.28+dfsg1.orig.tar.xz
 ab66f947a8b1f3ee25bce4f25419a1c546c7d238 29128 db5.3_5.3.28+dfsg1-0.5.debian.tar.xz
Checksums-Sha256:
 b19bf3dd8ce74b95a7b215be9a7c8489e8e8f18da60d64d6340a06e75f497749 19723860 db5.3_5.3.28+dfsg1.orig.tar.xz
 682c1736c1b5f3afbd90cf24e085a0437821ae595dc54aeef8c09ddd1c3d05fe 29128 db5.3_5.3.28+dfsg1-0.5.debian.tar.xz
Files:
 1dd7f0f45b985b661dc3c6edbd646d80 19723860 db5.3_5.3.28+dfsg1.orig.tar.xz
 bb05add3358c3d1c2dcf1dfec52b7f02 29128 db5.3_5.3.28+dfsg1-0.5.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQEzBAEBCAAdFiEEeuS9ZL8A0js0NGiOXkCM2RzYOdIFAlx0/SgACgkQXkCM2RzY
OdKb8gf+J6XcIIP5p9Eh5QEd1P8ATFgbeXqQUAjedHpGomYWqqcvlPVnDK+uytB5
kLNNmCjhCFsavdBjK2Yvniw4/ug5R271ZR0euzhLoQg5DjSEJVKRjZOBHKbY3GKM
A9BpxzLN+eDreiSQ6yDrtUAxKY1fNYkx+vBlbcc3VwsRwdi4BNJkoGo+MviWXBtv
WNe/UxaSRexafmdtbKyZJQkOjO/shZfGIs0QdMU5Z5hC9z2THvoWhq3DPGZCRaDe
KoaGUJM6KAkpbSR3WUyfF+y8XdnnKkULDGYlhUofzYcIGTKpChke1vcAIceAekq0
Ce8X3gdCbjlJLD6rp2sjc6RzdeHl3Q==
=e66K
-----END PGP SIGNATURE-----
