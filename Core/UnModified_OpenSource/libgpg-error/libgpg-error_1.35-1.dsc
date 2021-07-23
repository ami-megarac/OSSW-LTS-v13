-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: libgpg-error
Binary: libgpg-error-dev, libgpg-error0, gpgrt-tools, libgpg-error0-udeb, libgpg-error-mingw-w64-dev, libgpg-error-l10n
Architecture: any all
Version: 1.35-1
Maintainer: Debian GnuPG Maintainers <pkg-gnupg-maint@lists.alioth.debian.org>
Uploaders:  Daniel Kahn Gillmor <dkg@fifthhorseman.net>,
Homepage: https://www.gnupg.org/related_software/libgpg-error/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/debian/libgpg-error
Vcs-Git: https://salsa.debian.org/debian/libgpg-error.git
Testsuite: autopkgtest
Testsuite-Triggers: build-essential, wine, wine32, wine64
Build-Depends: automake (>= 1.14), debhelper-compat (= 12), gettext (>= 0.19.3), texinfo
Build-Depends-Indep: mingw-w64
Package-List:
 gpgrt-tools deb devel optional arch=any
 libgpg-error-dev deb libdevel optional arch=any
 libgpg-error-l10n deb localization optional arch=all
 libgpg-error-mingw-w64-dev deb libdevel optional arch=all
 libgpg-error0 deb libs optional arch=any
 libgpg-error0-udeb udeb debian-installer optional arch=any
Checksums-Sha1:
 1ffc6aaac4a4eecf16132c1b4eb500c1765d7190 918408 libgpg-error_1.35.orig.tar.bz2
 b1fa253f2c1c52f38ceda2cb75037a14a55e5bdc 534 libgpg-error_1.35.orig.tar.bz2.asc
 b7a2f7d51969ad8573528889bd060121cb43cdd5 16056 libgpg-error_1.35-1.debian.tar.xz
Checksums-Sha256:
 cbd5ee62a8a8c88d48c158fff4fc9ead4132aacd1b4a56eb791f9f997d07e067 918408 libgpg-error_1.35.orig.tar.bz2
 f6bfdc64a84245437c443f83faea85407d051d0487550515a4a279573589944d 534 libgpg-error_1.35.orig.tar.bz2.asc
 e600a34c09e6a3e8ec63d6145f4a11b16d92dc0ddeff1ba94cba08a8fecf0b66 16056 libgpg-error_1.35-1.debian.tar.xz
Files:
 2808a9e044f883f7554c5ba6a380b711 918408 libgpg-error_1.35.orig.tar.bz2
 a4e623fd84248b4196e61b74ded2e106 534 libgpg-error_1.35.orig.tar.bz2.asc
 b67aee4a998e32b12702ec00e1e781c4 16056 libgpg-error_1.35-1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iHUEARYKAB0WIQTJDm02IAobkioVCed2GBllKa5f+AUCXFOESwAKCRB2GBllKa5f
+POSAQCqbLz+1W/y4yz6UCmGDWze4kDnly2Q+lm48KVd50E6QwEAtPo8kpAz10dT
RgiaUPEqyIWMU/qjqmg6njWJ0mQwOwY=
=AbyY
-----END PGP SIGNATURE-----
