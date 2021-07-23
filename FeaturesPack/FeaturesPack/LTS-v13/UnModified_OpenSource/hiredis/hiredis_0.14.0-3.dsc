-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: hiredis
Binary: libhiredis0.14, libhiredis-dev
Architecture: any
Version: 0.14.0-3
Maintainer: Chris Lamb <lamby@debian.org>
Homepage: https://github.com/redis/hiredis
Standards-Version: 4.2.1
Vcs-Browser: https://salsa.debian.org/lamby/pkg-hiredis
Vcs-Git: https://salsa.debian.org/lamby/pkg-hiredis.git
Testsuite: autopkgtest
Testsuite-Triggers: gcc, libc-dev, pkg-config
Build-Depends: debhelper-compat (= 11), procps <!nocheck>, redis-server (>= 2:2.4.2-2) [linux-any kfreebsd-any] <!nocheck>
Package-List:
 libhiredis-dev deb libdevel optional arch=any
 libhiredis0.14 deb libs optional arch=any
Checksums-Sha1:
 d668b86756d2c68f0527e845dc10ace5a053bbd9 63061 hiredis_0.14.0.orig.tar.gz
 3fe3435cf67130ae33acd9f841f01d05cdb588d6 8384 hiredis_0.14.0-3.debian.tar.xz
Checksums-Sha256:
 042f965e182b80693015839a9d0278ae73fae5d5d09d8bf6d0e6a39a8c4393bd 63061 hiredis_0.14.0.orig.tar.gz
 b41014e417c02d089f7bc7ce4b598debad284aadf8c07f39f7447f08d14ed078 8384 hiredis_0.14.0-3.debian.tar.xz
Files:
 6d565680a4af0d2e261abbc3e3431b2b 63061 hiredis_0.14.0.orig.tar.gz
 d51e565e927bd84278a1114c677a4659 8384 hiredis_0.14.0-3.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCAAdFiEEwv5L0nHBObhsUz5GHpU+J9QxHlgFAlvffpoACgkQHpU+J9Qx
HlgTURAAjBTtYPFM6s4NI8vjidztIT+poyw+f/Io70JoH1Arrf/gsM2n6NNf0LjN
sCKtZI+TgnLxtIpoo9BWJqldjH00gXyPRGaPNX3yxY9HmjpAPVHjppMG1fKwPZsB
7VkBNNprAvTFrTCXpan58br3yfClINsqhawKtQb4+gjGTzx5Yw8DRLiUqW15rd4n
gXtSUfbvbmJWLRrPLxyl7pKnyvqEtEoEsdYEft/8ejqiNvb8+BrjIwvhdTaLmPHf
CdA9gek52UlpF/1uzG4vI1KDirZAKkswlRGj5VURzkhpOnlzhDfP+stkGaDTm6Vy
SmKGxIzLoLehzXEfx8KLVzCIt0Bj1jvrdGsY3lviUpJS/2bzJUXLIO4goVY1SgVW
8ELiTX9LrHQwZufXz1zh1EhZfsVg1sRiYqwEgxRlFKggkmWmcfg6X23IZ8hSqtvw
47o6mVmO9BpwB6R9k3JKHWiXq3vZcqOiZ/Cz1EYp550SKcytEyu/TR2cKq90WRo3
hdzpWLW6T79BekCIh3u14sY0f62uxjz76bSoTrqVEw8g9o7namWtTISUHvBoCHZi
UaKi2jf7cEmAZ1V4T84zQKtHYcESqxsWuf9mDzwlFaiS6rxL9UGOa34LSLw0sh5x
kXmwDP4qYFtdcwZwyb/rL+UHI9pCtBwIbWh1xxB1IH9IIDEKuSs=
=XhOz
-----END PGP SIGNATURE-----
