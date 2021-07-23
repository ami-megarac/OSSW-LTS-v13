-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: iptables
Binary: iptables, iptables-dev, libxtables12, libxtables-dev, libiptc0, libiptc-dev, libip4tc0, libip4tc-dev, libip6tc0, libip6tc-dev
Architecture: any
Version: 1.8.2-4
Maintainer: Debian Netfilter Packaging Team <pkg-netfilter-team@lists.alioth.debian.org>
Uploaders: Arturo Borrero Gonzalez <arturo@debian.org>, Laurence J. Lane <ljlane@debian.org>
Homepage: http://www.netfilter.org/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/pkg-netfilter-team/pkg-iptables
Vcs-Git: https://salsa.debian.org/pkg-netfilter-team/pkg-iptables
Testsuite: autopkgtest
Build-Depends: autoconf, automake, bison, debhelper (>= 10), flex, libmnl-dev, libnetfilter-conntrack-dev, libnetfilter-conntrack3, libnfnetlink-dev, libnftnl-dev, libtool (>= 2.2.6)
Package-List:
 iptables deb net important arch=linux-any
 iptables-dev deb oldlibs optional arch=any
 libip4tc-dev deb libdevel optional arch=linux-any
 libip4tc0 deb libs optional arch=linux-any
 libip6tc-dev deb libdevel optional arch=linux-any
 libip6tc0 deb libs optional arch=linux-any
 libiptc-dev deb libdevel optional arch=linux-any
 libiptc0 deb libs optional arch=linux-any
 libxtables-dev deb libdevel optional arch=linux-any
 libxtables12 deb libs optional arch=linux-any
Checksums-Sha1:
 215c4ef4c6cd29ef0dd265b4fa5ec51a4f930c92 679858 iptables_1.8.2.orig.tar.bz2
 37719b6ae51f60b2f3a8aec61450b3613f457794 65300 iptables_1.8.2-4.debian.tar.xz
Checksums-Sha256:
 a3778b50ed1a3256f9ca975de82c2204e508001fc2471238c8c97f3d1c4c12af 679858 iptables_1.8.2.orig.tar.bz2
 e6562e368ed7bff8378c1a31ca0d283f15be3a4c68165786dfaa38cc5e9e9e09 65300 iptables_1.8.2-4.debian.tar.xz
Files:
 944558e88ddcc3b9b0d9550070fa3599 679858 iptables_1.8.2.orig.tar.bz2
 c27e499611c48ba307792518d29cdcc7 65300 iptables_1.8.2-4.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEE3ZhhqyPcMzOJLgepaOcTmB0VFfgFAlx5KscACgkQaOcTmB0V
FfhPSw//fAVicWk289alGJznc3z9UGhiq1WkjNYDgCJF6rIhsyONh4lMWrlPz9h6
r77qU8RQFnJz0lyUlIJiCyG/YwPrfd27i0k9cktAXKcQmZL4gvdA0MqouDJXMpLP
IFjKtnfsGrmDmlJGrk77e8dukOOCAISrQ9yh/Z6Ct+UbigQyIg2M678P3qY6Q6Ii
6VQdr+OUqqTZrJpgDrpm1+5CouR+vgLXdcup2Y2XkBbK9WVQd2ByQ86LtjdUXVMR
kdV1kEgLfazHrQ8zoXF5314OY2m4MuU51sVFDgqva8N8htBMiBvXPfpslGPq2t3r
JznFXo8UT6Ilzn3GNd012S1XZopEaebB5pBKT/r3iV0Swcdh9PIrTY0DrKMFMtZL
trng5XpExqDJUKD/O/TXeA5FEI68WGC1oeTiw2Xow6emEAibdDcPJOS/6mnGVMyp
ufnez7xppvTUG4pdnD9r7x9LDH9FresuRpC9FF40RsC4AqtbbnWR1XcgvLkH9R5i
AKN0w8nsaHXnA5UJfe3RhIVTfqBWWYvDtmf158W1SQwdcac9+qeHMqN633oa9oeU
JZ58cuMtQa6OXoyiOjWgN0nXzx+3xmER0MbGVQ5EmPZ0wC7QodRCMTjTwxlahAWC
4y6b+oSPSkFjDdrJ+vpp8/yLIhoVjXrFjuw10Aos8eO/Cew1/x8=
=SXfJ
-----END PGP SIGNATURE-----
