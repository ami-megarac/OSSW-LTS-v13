-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: openldap
Binary: slapd, slapd-contrib, slapd-smbk5pwd, ldap-utils, libldap-2.4-2, libldap-common, libldap2-dev, slapi-dev
Architecture: any all
Version: 2.4.47+dfsg-3+deb10u6
Maintainer: Debian OpenLDAP Maintainers <pkg-openldap-devel@lists.alioth.debian.org>
Uploaders: Steve Langasek <vorlon@debian.org>, Torsten Landschoff <torsten@debian.org>, Ryan Tandy <ryan@nardis.ca>
Homepage: http://www.openldap.org/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/openldap-team/openldap
Vcs-Git: https://salsa.debian.org/openldap-team/openldap.git
Build-Depends: debhelper (>= 10), dpkg-dev (>= 1.17.14), groff-base, heimdal-multidev (>= 7.4.0.dfsg.1-1~) <!stage1>, libdb5.3-dev <!stage1>, libgnutls28-dev, libltdl-dev <!stage1>, libperl-dev (>= 5.8.0) <!stage1>, libsasl2-dev, libwrap0-dev <!stage1>, nettle-dev <!stage1>, perl:any, po-debconf, unixodbc-dev <!stage1>
Build-Conflicts: autoconf2.13, bind-dev, libbind-dev, libicu-dev
Package-List:
 ldap-utils deb net optional arch=any
 libldap-2.4-2 deb libs optional arch=any
 libldap-common deb libs optional arch=all
 libldap2-dev deb libdevel optional arch=any
 slapd deb net optional arch=any profile=!stage1
 slapd-contrib deb net optional arch=any profile=!stage1
 slapd-smbk5pwd deb oldlibs optional arch=all profile=!stage1
 slapi-dev deb libdevel optional arch=any profile=!stage1
Checksums-Sha1:
 e2465bd56a2a35f78537831ca06a6a839200bb89 4872293 openldap_2.4.47+dfsg.orig.tar.gz
 81522bf135ee377992f51260d15a6727adcb6613 173600 openldap_2.4.47+dfsg-3+deb10u6.debian.tar.xz
Checksums-Sha256:
 8f1ac7a4be7dd8ef158361efbfe16509756d3d9b396f5f378c3cf5c727807651 4872293 openldap_2.4.47+dfsg.orig.tar.gz
 d21ccc7d2fc3b38dd68e8f4dd73bcff51d377e4ad47e6372ea4f806729856b79 173600 openldap_2.4.47+dfsg-3+deb10u6.debian.tar.xz
Files:
 b734b63b740333d7d6bd02cb9ce09380 4872293 openldap_2.4.47+dfsg.orig.tar.gz
 4d504020bfe6e92f432bdba373e633a3 173600 openldap_2.4.47+dfsg-3+deb10u6.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQKmBAEBCgCQFiEERkRAmAjBceBVMd3uBUy48xNDz0QFAmArWHlfFIAAAAAALgAo
aXNzdWVyLWZwckBub3RhdGlvbnMub3BlbnBncC5maWZ0aGhvcnNlbWFuLm5ldDQ2
NDQ0MDk4MDhDMTcxRTA1NTMxRERFRTA1NENCOEYzMTM0M0NGNDQSHGNhcm5pbEBk
ZWJpYW4ub3JnAAoJEAVMuPMTQ89E5jIP/3rUhhrq//PUVdo/5M9iUy6byuSfoNbQ
g9pN+pD9VTmErmWzIGpFu8fH1uux8EAxnfDHY2k7y/yi7q5v2hiiIC9H8p0I9CmE
tpvS5clPLGBB/BXjSQJ24yzoHXdk4qgWVQT7xCmXzgegG8wraC7DWrdAx8yKa/6H
JPbr0wkdTJf44KQuDn0rp/bvjrmxRIVDcQ+4qmRbZnXBxqUgZCpRxkv249dhEQSE
xdg4thGMzRNbOnb4lNTZL/witco9XkSMr7zTQ2BTTqpFCHMVGvxw23F5hcHoC9DQ
tA/QViGVvk/4GwU63ZEz0UI1Efl7WL+Ra03AAvMWHIynDdnWjuz7EW6r9+32SFsj
lbL0r9urFA7BCeHl0fakr6jNeNp9vXiHZMrqldwzQwWbv2yzXzuHQsokR9q/xleJ
aqkCb2nCoa1AGy1U+4tZHV83RTDafNffsvu702IVBo7mACganC5HQQISGpMA2CQ3
x6EV2t7+CzVUp7TJdx8ZCJyU2L6cAkqXN6wxejVXXedTQL/JSysnbxWaL+GaT2DR
5f8xspzzTwrvasHt3uqzI2MkVYA5v2j1uWP8kYHqH7rXuukqgAZggdYXIT550ja4
4sYbP2R4ZWgrPdAm5KJ3jWcbtmt++ZxeTpLGqM/VOHLPWjwFFPRg+sPbZAt2uD2D
+o5L7lUmrDRL
=9TQt
-----END PGP SIGNATURE-----
