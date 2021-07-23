-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: perl
Binary: perl-base, perl-doc, perl-debug, libperl5.28, libperl-dev, perl-modules-5.28, perl
Architecture: any all
Version: 5.28.1-6+deb10u1
Maintainer: Niko Tyni <ntyni@debian.org>
Uploaders: Dominic Hargreaves <dom@earth.li>
Homepage: http://dev.perl.org/perl5/
Standards-Version: 3.9.8
Vcs-Browser: https://salsa.debian.org/perl-team/interpreter/perl
Vcs-Git: https://salsa.debian.org/perl-team/interpreter/perl.git
Testsuite: autopkgtest
Testsuite-Triggers: build-essential, db-util, dpkg-dev, gdbmtool, libapt-pkg-perl, pkg-perl-autopkgtest
Build-Depends: file, cpio, libdb-dev, libgdbm-dev (>= 1.18-3), libgdbm-compat-dev, netbase <!nocheck>, procps [!hurd-any] <!nocheck>, zlib1g-dev | libz-dev, libbz2-dev, dpkg-dev (>= 1.17.14), dist (>= 3.5-236), libc6-dev (>= 2.19-9) [s390x]
Package-List:
 libperl-dev deb libdevel optional arch=any
 libperl5.28 deb libs optional arch=any
 perl deb perl standard arch=any
 perl-base deb perl required arch=any essential=yes
 perl-debug deb devel optional arch=any
 perl-doc deb doc optional arch=all
 perl-modules-5.28 deb perl standard arch=all
Checksums-Sha1:
 21339f5f1bcacbaed5cdfe97368eacbc5e55da35 411944 perl_5.28.1.orig-regen-configure.tar.xz
 5fc239bebb8c484c3f5c58e663274ce668981651 12372080 perl_5.28.1.orig.tar.xz
 5373cc70c69ab08aac031cbf0618ba3f29f7fd37 185004 perl_5.28.1-6+deb10u1.debian.tar.xz
Checksums-Sha256:
 5873b81af4514d3910ab1a8267b15ff8c0e2100dbae4edfd10b65ef72cd31ef8 411944 perl_5.28.1.orig-regen-configure.tar.xz
 fea7162d4cca940a387f0587b93f6737d884bf74d8a9d7cfd978bc12cd0b202d 12372080 perl_5.28.1.orig.tar.xz
 e531c2d8c85b28b34c2122175a8e8f6cfe56b8a0708972fc4beae9876549d815 185004 perl_5.28.1-6+deb10u1.debian.tar.xz
Files:
 fbf2e774fdcc55c92afe713db38e5e25 411944 perl_5.28.1.orig-regen-configure.tar.xz
 fbb590c305f2f88578f448581b8cf9c4 12372080 perl_5.28.1.orig.tar.xz
 254ec057be58387a6b1fd7bdaef36d9c 185004 perl_5.28.1-6+deb10u1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQJBBAEBCAArFiEEy0llJ/kAnyscGnbawAV+cU1pT7IFAl8XRuoNHGRvbUBlYXJ0
aC5saQAKCRDABX5xTWlPsqYYEADWBOo8+OsHEZ+qSMtrbMlhLDTQc3Czwqr/Mprb
qN9E5FdcbTG1CkSIJC6wzkrdUrKVAquO0Q6+aHUpvfll36u3AP62PbyFntgwJX+r
4nuGqIYjd7ConG3d0RcBdd+NAu4YKffjw7U6+w7cZHPq/HorjIudo5VydLVyiiV3
/TxTc85Vf6++OtRIS+vjl8XmRUtuPaNdI5QzBsFjwUCKWcbkvD8W6WDftcOp9WEr
lr07ZaE8vxg+E+zHazt/mLORVVrvqhl+5cBeTP+p8xnJZu4PdvQ9+W23UTDKvcs/
aLNnSeySd2Vv+xx1OQQwW1YKvymr0yVLPH/m7bl5NgXIBM0RUD+q0YZUlzPRvBnH
f+VRW5xoaWRkU8dCHp1asdtmbLEdDNoB4cUV+Pno6iiOoanIXg6Ztcpg8yv++/Em
0W98yLtUbygiPZFGoBuajfN9FPrIffaYbxHPDEQtXzvF/VmdANA4YHEDqbD59a+2
gxVgONsBL8ZVgpkj28XYkEu1mkYfZDgmMZD032a4xKD5ew3sFh+StPdc+6dNo3V5
L8VYRNbCcwl7uyuif5bhL2CrxP1vVcvLfx/HFRNWZ6t+Mqk2apKwJtBCzuK51+nm
/kU985GthrbAA55YfQKpOdh9CXz0XBH2RTPx78SOasivqBwAYoGy06LuYyTXiptU
cE29jg==
=0n7n
-----END PGP SIGNATURE-----
