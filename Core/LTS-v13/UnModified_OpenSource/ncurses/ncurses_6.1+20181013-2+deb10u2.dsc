-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: ncurses
Binary: libtinfo6, libtinfo6-udeb, libncurses6, libncurses-dev, libtinfo-dev, libtinfo6-dbg, libncurses5-dev, libncurses6-dbg, libncursesw6, libncursesw5-dev, libncursesw6-dbg, lib64tinfo6, lib64ncurses6, lib64ncursesw6, lib64ncurses-dev, lib32tinfo6, lib32ncurses6, lib32ncursesw6, lib32ncurses-dev, ncurses-bin, ncurses-base, ncurses-term, ncurses-examples, ncurses-doc, libtinfo5, libncurses5, libncursesw5
Architecture: any all
Version: 6.1+20181013-2+deb10u2
Maintainer: Craig Small <csmall@debian.org>
Uploaders: Sven Joachim <svenjoac@gmx.de>
Homepage: https://invisible-island.net/ncurses/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/debian/ncurses
Vcs-Git: https://salsa.debian.org/debian/ncurses.git
Build-Depends: debhelper (>= 11~), libgpm-dev [linux-any], pkg-config, autoconf-dickey (>= 2.52+20170501)
Build-Depends-Arch: g++-multilib [amd64 i386 powerpc ppc64 s390 sparc] <!nobiarch>
Package-List:
 lib32ncurses-dev deb libdevel optional arch=amd64,ppc64 profile=!nobiarch
 lib32ncurses6 deb libs optional arch=amd64,ppc64 profile=!nobiarch
 lib32ncursesw6 deb libs optional arch=amd64,ppc64 profile=!nobiarch
 lib32tinfo6 deb libs optional arch=amd64,ppc64 profile=!nobiarch
 lib64ncurses-dev deb libdevel optional arch=i386,powerpc,sparc,s390 profile=!nobiarch
 lib64ncurses6 deb libs optional arch=i386,powerpc,sparc,s390 profile=!nobiarch
 lib64ncursesw6 deb libs optional arch=i386,powerpc,sparc,s390 profile=!nobiarch
 lib64tinfo6 deb libs optional arch=i386,powerpc,sparc,s390 profile=!nobiarch
 libncurses-dev deb libdevel optional arch=any
 libncurses5 deb oldlibs optional arch=any
 libncurses5-dev deb oldlibs optional arch=any
 libncurses6 deb libs optional arch=any
 libncurses6-dbg deb debug optional arch=any
 libncursesw5 deb oldlibs optional arch=any
 libncursesw5-dev deb oldlibs optional arch=any
 libncursesw6 deb libs optional arch=any
 libncursesw6-dbg deb debug optional arch=any
 libtinfo-dev deb oldlibs optional arch=any
 libtinfo5 deb oldlibs optional arch=any
 libtinfo6 deb libs optional arch=any
 libtinfo6-dbg deb debug optional arch=any
 libtinfo6-udeb udeb debian-installer optional arch=any
 ncurses-base deb misc required arch=all essential=yes
 ncurses-bin deb utils required arch=any essential=yes
 ncurses-doc deb doc optional arch=all
 ncurses-examples deb misc optional arch=any
 ncurses-term deb misc standard arch=all
Checksums-Sha1:
 4af288c253634e3056a3646554a8b138fd6353be 3411288 ncurses_6.1+20181013.orig.tar.gz
 eb1f4c538d0c64d83d8575bbb4c7d1f2996fc567 251 ncurses_6.1+20181013.orig.tar.gz.asc
 2f6d909f968686b2cd51ddd899fe2c4a6f898bda 61664 ncurses_6.1+20181013-2+deb10u2.debian.tar.xz
Checksums-Sha256:
 aeb1d098ee90b39a763b57b00da19ff5bbb573dea077f98fbd85d59444bb3b59 3411288 ncurses_6.1+20181013.orig.tar.gz
 865931406e519909a4d0ab87b14d0c6d3ebccb7b3e0dac5c6095f0dfce5e14cf 251 ncurses_6.1+20181013.orig.tar.gz.asc
 4574ec11ce2577e76f30f8d40cc2a9ebf94d8208f47247021da88b7b09e77df9 61664 ncurses_6.1+20181013-2+deb10u2.debian.tar.xz
Files:
 ea0fcd870f98479c49aac1c83e2533da 3411288 ncurses_6.1+20181013.orig.tar.gz
 ddd6d2f7aa0aee770cdecd4fde0bbe58 251 ncurses_6.1+20181013.orig.tar.gz.asc
 3f10bbd22130474b1719151f030a997d 61664 ncurses_6.1+20181013-2+deb10u2.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEKF8heKgv5Jai5p4QOxBucY1rMawFAl29yOIACgkQOxBucY1r
Maw9NQ//Vv87uq6E1krpqKjwDIHr7E3rtxZDBIir1Sda1C50SrnpFhAzKw1tXbQv
OGREPH2RIG5xfBgI+KrOwIyeCSVhMzHI4JUYLhauQHvTwIJNlI4DWi8HZhKQOSI0
kZxK293p5jKOeng1RxRAExqKbPMBAxJlZYQIbH5z7+L01Rbf+e0pCk9tlWKC9nHm
SrXx+hmNhNtJ8ak6f/agbBDBgE37hoziGixRGC53hJD/eMnqnOZS/fSHheI719Ef
aBC7wOWFBmiwSisQXNUkn8Y+8b++Fyeb0XzUSMDts9DOvUWD33F9i0yFWJysWHZO
Lu7O1SEgoldw44Eq0DxR2JVX5mrnroJxSVHofNIYsF5EPowAzkYB62PWgZSbGtsG
d2zF56+AlXtAILvuPzD19aNHUD3vhgS6W1BDI+N1kMZLqBtCJC4+lppyM7OL9sBf
1cQsqT+yxQXe0we8cWFRkT9N6gbrAMHYNXCATLid3WnFVSTyCTAIHiTCaSO57Obi
hzQMH5P8Py9bLIyxVxYrRBA4TPDJsPdbx5iEUMH0UrsvLlU45/5yMmHkzJEpnlGo
8Gz0Gvl4zMkXs+UkydHf1xzj2bIKpXh0vQM9VRTuu0BKVYXhIWVjoJUUTy2byGX+
pbzvUk4y14pIi5Hnn8IYrYtPdUdReMjxQAe0uBZBA70+WKZ15YY=
=MkHT
-----END PGP SIGNATURE-----
