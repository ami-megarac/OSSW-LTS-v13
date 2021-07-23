-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: readline
Binary: libreadline7, lib64readline7, readline-common, libreadline-dev, lib64readline-dev, rlfe, lib32readline7, lib32readline-dev, readline-doc
Architecture: any all
Version: 7.0-5
Maintainer: Matthias Klose <doko@debian.org>
Standards-Version: 4.1.4
Build-Depends: debhelper (>= 9), libncurses-dev, lib32ncurses-dev [amd64 ppc64], lib64ncurses-dev [i386 powerpc sparc s390], mawk | awk, texinfo, autotools-dev, gcc-multilib [amd64 i386 kfreebsd-amd64 powerpc ppc64 s390 sparc]
Package-List:
 lib32readline-dev deb libdevel optional arch=amd64,ppc64
 lib32readline7 deb libs optional arch=amd64,ppc64
 lib64readline-dev deb libdevel optional arch=i386,powerpc,s390,sparc
 lib64readline7 deb libs optional arch=i386,powerpc,s390,sparc
 libreadline-dev deb libdevel optional arch=any
 libreadline7 deb libs important arch=any
 readline-common deb utils important arch=all
 readline-doc deb doc optional arch=all
 rlfe deb utils optional arch=any
Checksums-Sha1:
 d9095fa14a812495052357e1d678b3f2ac635463 2910016 readline_7.0.orig.tar.gz
 0a330772ead95b8d374b6877dabc681ce9eee6bc 29992 readline_7.0-5.debian.tar.xz
Checksums-Sha256:
 750d437185286f40a369e1e4f4764eda932b9459b5ec9a731628393dd3d32334 2910016 readline_7.0.orig.tar.gz
 5c1cc7396a670ce7e6e4c0bc36e8d3067b7642bea5b30fc3ff22bf8e65d2ee80 29992 readline_7.0-5.debian.tar.xz
Files:
 205b03a87fc83dab653b628c59b9fc91 2910016 readline_7.0.orig.tar.gz
 3df1db59f65051a4ce79069a6475d9ea 29992 readline_7.0-5.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQJEBAEBCAAuFiEE1WVxuIqLuvFAv2PWvX6qYHePpvUFAlrtxVAQHGRva29AZGVi
aWFuLm9yZwAKCRC9fqpgd4+m9fy5D/4oPKVumMr+qMlRCc9vzCMw5wYjplZZXCvc
j1Cebp1CHHf48ujWd/5NTzKescoypOltWOA2rde2bNfXgNCbLGBwE3SK6r5clzrC
EoZPQAO0+Tb2ipy28gomF0hVG/lTebbHQd0aUw4OqXvuzBaljj062mcV2Kbt8pP7
6PuDNKQk12wdFT3a7SSbZhHd6wq4G6alq1bIBup+b3m6o5v+2NbrjY+FO2Z+A14O
lq0V6XX39/2Qr5ETukgHT7Fg+CAGWOiekNfiK+Sgb3HEFOroBzxA/K0E1Pkl/Wmz
/FmOU/e2Vyzv6O6WQAP8f9DJcct07OaZqqABT7qDt8PdCQ1Q6c9fetM38ef7MULU
NGHPVlty9RW1HweBYbG0AP98CyVLteJn8Mm4yZ17tTd7v0Lj8DRHDdgJ32/7L/MF
j8VAITv2+frteTidW8rwuKtpMbf+DvhDGH/Npsn00JWXZlXEeQcnbP9UTWMzjx5T
C4HS2wPC4oV8hRo7/sQbyafWUZbUEWdiB7mzFujHEtIZpU+qMdeuCvKrwik+SNMt
fqVppazppd0KjgNhHu7rXGfNceu3k/LJjuthDQKSyQn4n7SM5veV7xgfU6+1BeQs
Cqrrbokm1Tqgd5fqn/T/8t6y4AUsddcwIKhhdnx856tz4O0uzjbySyrqGq5/BiOo
VNX8VyeMdQ==
=XB9k
-----END PGP SIGNATURE-----
