-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: nghttp2
Binary: libnghttp2-dev, libnghttp2-doc, libnghttp2-14, nghttp2-client, nghttp2-proxy, nghttp2-server, nghttp2
Architecture: any all
Version: 1.36.0-2+deb10u1
Maintainer: Tomasz Buchert <tomasz@debian.org>
Uploaders: Ondřej Surý <ondrej@debian.org>
Homepage: https://nghttp2.org/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/debian/nghttp2
Vcs-Git: https://salsa.debian.org/debian/nghttp2.git
Testsuite: autopkgtest
Testsuite-Triggers: nginx
Build-Depends: debhelper (>= 12), debhelper-compat (= 12), dpkg-dev (>= 1.17.14), libc-ares-dev, libcunit1-dev <!nocheck>, libev-dev (>= 1:4.15), libevent-dev, libjansson-dev (>= 2.5), libjemalloc-dev [!hurd-i386], libssl-dev, libsystemd-dev, libxml2-dev, pkg-config, zlib1g-dev
Build-Depends-Indep: python3-sphinx
Package-List:
 libnghttp2-14 deb libs optional arch=any
 libnghttp2-dev deb libdevel optional arch=any
 libnghttp2-doc deb doc optional arch=all
 nghttp2 deb httpd optional arch=all
 nghttp2-client deb httpd optional arch=any
 nghttp2-proxy deb httpd optional arch=any
 nghttp2-server deb httpd optional arch=any
Checksums-Sha1:
 487edc0ae98bb35cec0bb2dcac3745eaf5fd0109 1919021 nghttp2_1.36.0.orig.tar.bz2
 0626e05346394ef600fab255f480f168b6d4e0f4 13132 nghttp2_1.36.0-2+deb10u1.debian.tar.xz
Checksums-Sha256:
 16a734d7414062911e23989e243ca76e7722cb3c60273723e3e3ae4c21e71ceb 1919021 nghttp2_1.36.0.orig.tar.bz2
 f4fb4dd2385d158efba2ec3d3ce1b13c24ecb05c75f353f370f7cb0f080c7537 13132 nghttp2_1.36.0-2+deb10u1.debian.tar.xz
Files:
 c859aaab080a82d68f89204768b280f3 1919021 nghttp2_1.36.0.orig.tar.bz2
 be1c38a563df9d41e9b5b8f2c0c9761f 13132 nghttp2_1.36.0-2+deb10u1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQJGBAEBCgAwFiEEzmRl4OZ9N8ZVV+eKAK40x+rZ6zgFAl1o0PESHHRvbWFzekBk
ZWJpYW4ub3JnAAoJEACuNMfq2es4ZjIP/0sFe8CDBYZ6NcwnAnNC7Rv8u0AQvB+u
nlB3WdngtZ1KZezuOLSDmCHAJ6hTqXWnPmZ54Kij9eP8NKXjQNJ7BAyJHMrVbTPR
6vNtfn39Ja2OB1fo2t7hSjTYRHggXVbtNq+f2eqaeto2TwZZ4plZ36fThiZBmnlh
0K4b7DD0YONOB89rqldf7KdJMquQ+0F1RlKjnblyhbFtffDxZ5O2aW2t8YL/VBen
XOwzs1U4LKGDo8p/54AoLS54cDPfitx6hqGoCTfpF4ff4qxtybHhcyAXShShHE4w
p7wpDjCu8ExEl3fJIhVhtobJUl0v9GWXokiN6IRFJZERcnJo18lRw6yNK+6xniE1
LLaK0+Bsfcq8Ci8A8SqlF0Ax9GBblboJBNy15f9xYP0PjicGPpXis45htJVl6HlK
vAE4cba/ydBNWDvQXWKM6mkSDOW5gV50ejTuR0i8qNfX5SbMtvTIenBvzB16LTmL
EcFSTi4tvupQCGR5BOKX48yZ7tPlGC2NJLhYPSfNZ/KlBtIcE9PU4pVofR7yDhtC
KhafBjMBSllnG36LKr864qV2ml+S//LFi+xyXOEnUaFHvKLOvVY/TwayPMcBpQnT
r8wUvnMOf8VLn0TPkrhCMvkG/MB4fpjRNoBUw/xQYt1TAkcaZuQienyq5z9cms06
a+ierq0l7VsZ
=x3RT
-----END PGP SIGNATURE-----
