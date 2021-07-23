-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: audit
Binary: auditd, libauparse0, libauparse-dev, libaudit1, libaudit-common, libaudit-dev, python-audit, python3-audit, golang-redhat-audit-dev, audispd-plugins
Architecture: linux-any all
Version: 1:2.8.4-3
Maintainer: Laurent Bigonville <bigon@debian.org>
Homepage: https://people.redhat.com/sgrubb/audit/
Standards-Version: 4.2.1
Vcs-Browser: https://salsa.debian.org/debian/audit
Vcs-Git: https://salsa.debian.org/debian/audit.git
Build-Depends: debhelper (>= 10), dh-python <!nopython>, dpkg-dev (>= 1.16.1~), intltool, libcap-ng-dev, libkrb5-dev, libldap2-dev <!pkg.audit.noldap>, libprelude-dev, libwrap0-dev, python-all-dev:any (>= 2.6.6-3~) <!nopython>, libpython-all-dev (>= 2.6.6-3~) <!nopython>, python3-all-dev:any <!nopython>, libpython3-all-dev <!nopython>, swig
Build-Depends-Indep: golang-go
Package-List:
 audispd-plugins deb admin optional arch=linux-any profile=!pkg.audit.noldap
 auditd deb admin optional arch=linux-any
 golang-redhat-audit-dev deb devel optional arch=all
 libaudit-common deb libs optional arch=all
 libaudit-dev deb libdevel optional arch=linux-any
 libaudit1 deb libs optional arch=linux-any
 libauparse-dev deb libdevel optional arch=linux-any
 libauparse0 deb libs optional arch=linux-any
 python-audit deb python optional arch=linux-any profile=!nopython
 python3-audit deb python optional arch=linux-any profile=!nopython
Checksums-Sha1:
 026235ab9e8b19f6c2b1112ce13d180f35cf0ff4 1123889 audit_2.8.4.orig.tar.gz
 8d300675549adba418c84e6900d108b7ad156849 16712 audit_2.8.4-3.debian.tar.xz
Checksums-Sha256:
 a410694d09fc5708d980a61a5abcb9633a591364f1ecc7e97ad5daef9c898c38 1123889 audit_2.8.4.orig.tar.gz
 2b4b16cf58c3a6180d380bd4ad1d30a38fa22826ca3c1233c5298138427e29d0 16712 audit_2.8.4-3.debian.tar.xz
Files:
 ec9510312564c3d9483bccf8dbda4779 1123889 audit_2.8.4.orig.tar.gz
 34b4b854e5811833b1e67ee92a1af596 16712 audit_2.8.4-3.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQFFBAEBCAAvFiEEmRrdqQAhuF2x31DwH8WJHrqwQ9UFAlzByq4RHGJpZ29uQGRl
Ymlhbi5vcmcACgkQH8WJHrqwQ9WwIgf/b5ljY6Zb1vYHcLHFD8sLJud/pEXVDuln
HTwo8sewbsluuRL93mP+20dr2bQLpVh2YXMDEHuxxonBxZLcrvQ0s9g17xLQT232
X0glxP/g7a3a5W1iITnwNIci4l4B4MedtQqLNrUlpMd6i63z1WkB9utcOSj/5pGM
M1k2BPYtrSQdQgkMbixW6wahBqSi+T2hCsCPUyUyHED6B1d259HFsK1QDt/bK6ch
06+WNyGgs6a9YMe5LMDy84e8QU5sEh60O8gK0dDV7mjqkMowNzMCpUHKZ34vqZ6k
U0nsiVQkVujrK465Uv67CF5JygNMLAs0FHhhlq1Q57a0ITqIkKuzaQ==
=g6aY
-----END PGP SIGNATURE-----
