-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: libselinux
Binary: selinux-utils, libselinux1, libselinux1-dev, libselinux1-udeb, ruby-selinux, python-selinux, python3-selinux
Architecture: linux-any
Version: 2.8-1
Maintainer: Debian SELinux maintainers <selinux-devel@lists.alioth.debian.org>
Uploaders: Laurent Bigonville <bigon@debian.org>, Russell Coker <russell@coker.com.au>
Homepage: http://userspace.selinuxproject.org/
Standards-Version: 4.1.4
Vcs-Browser: https://salsa.debian.org/selinux-team/libselinux
Vcs-Git: https://salsa.debian.org/selinux-team/libselinux.git
Build-Depends: debhelper (>= 10), dh-python <!nopython>, file, gem2deb (>= 0.5.0~) <!noruby>, libsepol1-dev (>= 2.8), libpcre3-dev, pkg-config, python-all-dev (>= 2.6.6-3~) <!nopython>, python3-all-dev <!nopython>, swig <!nopython> <!noruby>
Package-List:
 libselinux1 deb libs optional arch=linux-any
 libselinux1-dev deb libdevel optional arch=linux-any
 libselinux1-udeb udeb debian-installer optional arch=linux-any
 python-selinux deb python optional arch=linux-any profile=!nopython
 python3-selinux deb python optional arch=linux-any profile=!nopython
 ruby-selinux deb ruby optional arch=linux-any profile=!noruby
 selinux-utils deb admin optional arch=linux-any
Checksums-Sha1:
 d45f2db91dbec82ef5a153aca247acc04234e8af 187759 libselinux_2.8.orig.tar.gz
 f23ed1bbd3246313a9b42a0f84dc7aa5651e2e3e 23052 libselinux_2.8-1.debian.tar.xz
Checksums-Sha256:
 31db96ec7643ce10912b3c3f98506a08a9116dcfe151855fd349c3fda96187e1 187759 libselinux_2.8.orig.tar.gz
 a0b150e870a3da7e1d7b0fec7c1a5ae6988a0985e545c69cfe8fe05363c5bf64 23052 libselinux_2.8-1.debian.tar.xz
Files:
 56057e60192b21122c1aede8ff723ca2 187759 libselinux_2.8.orig.tar.gz
 61bd1e8f673d3f161e4eb1a0cb9f3970 23052 libselinux_2.8-1.debian.tar.xz
Ruby-Versions: all

-----BEGIN PGP SIGNATURE-----

iQFFBAEBCAAvFiEEmRrdqQAhuF2x31DwH8WJHrqwQ9UFAlsMUbcRHGJpZ29uQGRl
Ymlhbi5vcmcACgkQH8WJHrqwQ9WjsQf9ERWSX7z9wTiSAKbr0Iqr7/DN9WTZAitD
Hx5EDsyF2mLBoRUmZq9/roDSB0J6P82cST+wRar29/FfBXV6FTTsNKAe97sMWmya
mNtmFgbQNawUS4N8x2ptS5JPSTypDG//iWM8i/ixdvTV2Z9biUFiahUx+Z4flQAp
+wZGH1EEyJeHdoOOASDllDPoPC2RyyP8Kl+8nKI8FWyfp/tzwmexpWYegto/bLoq
VlRUAuyqRDs+GzklkIW0Lbg3ZWPODpid/meyLvcwdVTE08/29xzm+RJE0v8sC/EW
uwp/Z3o1DODh8GPV3XgQTmPhGcozAhuvEHeDnMtZh215X97zS6T/iw==
=GseU
-----END PGP SIGNATURE-----
