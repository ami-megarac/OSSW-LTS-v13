-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: openssl
Binary: openssl, libssl1.1, libcrypto1.1-udeb, libssl1.1-udeb, libssl-dev, libssl-doc
Architecture: any all
Version: 1.1.1d-0+deb10u6
Maintainer: Debian OpenSSL Team <pkg-openssl-devel@lists.alioth.debian.org>
Uploaders: Christoph Martin <christoph.martin@uni-mainz.de>, Kurt Roeckx <kurt@roeckx.be>, Sebastian Andrzej Siewior <sebastian@breakpoint.cc>
Homepage: https://www.openssl.org/
Standards-Version: 4.2.1
Vcs-Browser: https://salsa.debian.org/debian/openssl
Vcs-Git: https://salsa.debian.org/debian/openssl.git
Testsuite: autopkgtest
Testsuite-Triggers: perl
Build-Depends: debhelper (>= 11), m4, bc, dpkg-dev (>= 1.15.7)
Package-List:
 libcrypto1.1-udeb udeb debian-installer optional arch=any
 libssl-dev deb libdevel optional arch=any
 libssl-doc deb doc optional arch=all
 libssl1.1 deb libs optional arch=any
 libssl1.1-udeb udeb debian-installer optional arch=any
 openssl deb utils optional arch=any
Checksums-Sha1:
 056057782325134b76d1931c48f2c7e6595d7ef4 8845861 openssl_1.1.1d.orig.tar.gz
 d3bbfe1db19cc36bb17f2b6dc39fa8ade6a8cdd3 488 openssl_1.1.1d.orig.tar.gz.asc
 6b2418348d24186953ed819b627fe4b442a887e8 99740 openssl_1.1.1d-0+deb10u6.debian.tar.xz
Checksums-Sha256:
 1e3a91bc1f9dfce01af26026f856e064eab4c8ee0a8f457b5ae30b40b8b711f2 8845861 openssl_1.1.1d.orig.tar.gz
 f3fd3299a79421fffd51d35f62636b8e987dab1d3033d93a19d7685868e15395 488 openssl_1.1.1d.orig.tar.gz.asc
 617063d8e99e888198f9aeae9cfc363b5799c4712881a8e525a6339b921580ff 99740 openssl_1.1.1d-0+deb10u6.debian.tar.xz
Files:
 3be209000dbc7e1b95bcdf47980a3baa 8845861 openssl_1.1.1d.orig.tar.gz
 56a525b2d934330e1c2de3bc9b55e4e2 488 openssl_1.1.1d.orig.tar.gz.asc
 09be261093ba4b2496d5b47f40a8a85f 99740 openssl_1.1.1d-0+deb10u6.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQGzBAEBCgAdFiEEV4kucFIzBRM39v3RBWQfF1cS+lsFAmBaUWEACgkQBWQfF1cS
+lv5AQwAuFTA1CuB+FFBeh6egj/OsAILyCisdKujtBTW7Sie47TwGI72R5uUmdQM
xi/NCe9gbeUgmv2m79CAdPuKIRriGNxMvRzgkFT2mi+4b7hO+bENny+9MDtYCnmK
yYhUCIli76vrkYenBaA/UY9cU9E/Sen0UjNYThtX0lTvMXG0ypiDd587nhQA/ySq
bFVpciFCgwJXEkjSeHPoT6XwTa3/JrfTbvR5fjf9faybm0Ys7E5UINMxVR+pJJBh
Xw9kGwT+WrdDhCHjqYscyOI0vxJwolomM/wmJfKhBiOW+c6YIbm4EsdAggjD/hn5
WraQ0L3z5F5/EUWqIkJeANK0oXMypnNXm6T/nE0XecMyeq5tOcV2xgXpTvw0bs4P
0hE7e/a2+KvRv1oPNzDp2z/f1X5E/9MCDC/l4mtlBBjfJarxqWRS4RNcierFVf0H
8/3nZmZRY5Dz7M8mgagBsMWG4w7hFZzZ4pWa16sBFJ6n9YyZ15cKUQJpGQMbKYvH
A34SdVoj
=4KX7
-----END PGP SIGNATURE-----
