-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: krb5
Binary: krb5-user, krb5-kdc, krb5-kdc-ldap, krb5-admin-server, krb5-kpropd, krb5-multidev, libkrb5-dev, libkrb5-dbg, krb5-pkinit, krb5-otp, krb5-k5tls, krb5-doc, libkrb5-3, libgssapi-krb5-2, libgssrpc4, libkadm5srv-mit11, libkadm5clnt-mit11, libk5crypto3, libkdb5-9, libkrb5support0, libkrad0, krb5-gss-samples, krb5-locales, libkrad-dev
Architecture: any all
Version: 1.17-3+deb10u1
Maintainer: Sam Hartman <hartmans@debian.org>
Uploaders: Russ Allbery <rra@debian.org>, Benjamin Kaduk <kaduk@mit.edu>
Homepage: http://web.mit.edu/kerberos/
Standards-Version: 4.1.1
Vcs-Browser: https://salsa.debian.org/debian/krb5
Vcs-Git: https://salsa.debian.org/debian/krb5
Testsuite: autopkgtest
Testsuite-Triggers: ldap-utils, libsasl2-modules-gssapi-mit, slapd
Build-Depends: debhelper (>= 10), byacc | bison, comerr-dev, docbook-to-man, libkeyutils-dev [linux-any], libldap2-dev <!stage1>, libsasl2-dev <!stage1>, libncurses5-dev, libssl-dev, ss-dev, libverto-dev (>= 0.2.4), pkg-config
Build-Depends-Indep: python, python-cheetah, python-lxml, python-sphinx, doxygen, doxygen-latex, texlive-generic-extra
Package-List:
 krb5-admin-server deb net optional arch=any
 krb5-doc deb doc optional arch=all
 krb5-gss-samples deb net optional arch=any
 krb5-k5tls deb net optional arch=any
 krb5-kdc deb net optional arch=any
 krb5-kdc-ldap deb net optional arch=any profile=!stage1
 krb5-kpropd deb net optional arch=any
 krb5-locales deb localization optional arch=all
 krb5-multidev deb libdevel optional arch=any
 krb5-otp deb net optional arch=any
 krb5-pkinit deb net optional arch=any
 krb5-user deb net optional arch=any
 libgssapi-krb5-2 deb libs optional arch=any
 libgssrpc4 deb libs optional arch=any
 libk5crypto3 deb libs optional arch=any
 libkadm5clnt-mit11 deb libs optional arch=any
 libkadm5srv-mit11 deb libs optional arch=any
 libkdb5-9 deb libs optional arch=any
 libkrad-dev deb libdevel optional arch=any
 libkrad0 deb libs optional arch=any
 libkrb5-3 deb libs optional arch=any
 libkrb5-dbg deb debug optional arch=any
 libkrb5-dev deb libdevel optional arch=any
 libkrb5support0 deb libs optional arch=any
Checksums-Sha1:
 0c404b081db9c996c581f636ce450ee28778f338 8761763 krb5_1.17.orig.tar.gz
 2cecc10c5ef730a1d63ee97f128233954cbcb094 100584 krb5_1.17-3+deb10u1.debian.tar.xz
Checksums-Sha256:
 5a6e2284a53de5702d3dc2be3b9339c963f9b5397d3fbbc53beb249380a781f5 8761763 krb5_1.17.orig.tar.gz
 396ecf9ec5b4ac91d2ce8527d7f6b2309e70fc5b14ea1158eb7e367c48e9c5ca 100584 krb5_1.17-3+deb10u1.debian.tar.xz
Files:
 3b729d89eb441150e146780c4138481b 8761763 krb5_1.17.orig.tar.gz
 2722ae5fcf04dddf2684a5966f0b825e 100584 krb5_1.17-3+deb10u1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEtuYvPRKsOElcDakFEMKTtsN8TjYFAl+2u2IACgkQEMKTtsN8
TjYOAxAAvCFmvkHy8EEasqqbys5LGwMH9wovcBTRIyMcGWZ835yuENQOApJRvf4T
8Lr41ciWVygwv1gDMlJ/i0p6ABXY32HXL+ilIOguUJPRd8iDnXu75l6u91WmYAVo
gviuG3hoeA2O9HmkxYYcueJTFV4usroLA86O01/hRMOft6ia9qN2cV187W+qPIOz
kre0Lv7rXpei303Yk+iL90KIFaXYfnKoZGNAo0SuJLkir1vVyrvEK734XJnsFzdf
/iIkmmQIOJBqAaFxDJxzXj+uFjZV9Tzh9zuCwMqpE+xNuB6fjRcfradx7wTd0DJN
h3JLBC+GR0GunGe1GNoJ4M7mxqsio/LPud9MYgZ8xPVZSk3nG1ZAn62G/Ve+304N
Q/eebH7Jz40UcEb5/gAp0am6DfZ+yA0fJzdyaa76aQiENWMPjcbIQoexOMbu8hqW
pbcciIkisNUgExmWGQmnP+NYTJeIj2KPlRIIVa6bNI0Y4zGc1I+5PBxvtkD0A7T5
vd6UufNW6By+2MS7VYpZRTdCSc0gnt32JfjfHOtRQX2/RPPaD1dXGWohJoMi5/C9
YH+hHHNwKkZWGZzZdrefUz2X5g5sDzm1Z8MH2vJwYMeetQ+XJkm1jQGIZdDb+K8J
3pDp7kpSs5wggKZ9iUsNgCCbwxIoTCUHjIaClvN0i8VJ8tYGZsc=
=OuoW
-----END PGP SIGNATURE-----
