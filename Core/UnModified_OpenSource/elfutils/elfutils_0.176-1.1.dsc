-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: elfutils
Binary: elfutils, libelf1, libelf-dev, libdw-dev, libdw1, libasm1, libasm-dev
Architecture: any
Version: 0.176-1.1
Maintainer: Kurt Roeckx <kurt@roeckx.be>
Homepage: https://sourceware.org/elfutils/
Standards-Version: 4.1.0
Build-Depends: debhelper (>= 9), autotools-dev, autoconf, automake, bzip2, zlib1g-dev, zlib1g-dev:native, libbz2-dev, liblzma-dev, m4, gettext, gawk, dpkg-dev (>= 1.16.1~), gcc-multilib [any-amd64 sparc64] <!nocheck>, libc6-dbg [powerpc powerpcspe ppc64 ppc64el armel armhf arm64 sparc64 riscv64], flex, bison
Build-Conflicts: autoconf2.13, automake1.4
Package-List:
 elfutils deb utils optional arch=any
 libasm-dev deb libdevel optional arch=any
 libasm1 deb libs optional arch=any
 libdw-dev deb libdevel optional arch=any
 libdw1 deb libs optional arch=any
 libelf-dev deb libdevel optional arch=any
 libelf1 deb libs optional arch=any
Checksums-Sha1:
 6511203cae7225ae780501834a7ccd234b14889a 8646075 elfutils_0.176.orig.tar.bz2
 6012c37ad5eeb16add7e5e1f0929c383ce0e00d4 455 elfutils_0.176.orig.tar.bz2.asc
 93fd45f0f144ed658801eb8b8e691a4d6fd6005a 31644 elfutils_0.176-1.1.debian.tar.xz
Checksums-Sha256:
 eb5747c371b0af0f71e86215a5ebb88728533c3a104a43d4231963f308cd1023 8646075 elfutils_0.176.orig.tar.bz2
 51474b579b25fc799de0777e241c83605427d2903f8d28524ef6af42f75931fd 455 elfutils_0.176.orig.tar.bz2.asc
 06d7057e744d3a6138cf43d30237e2b327b6bfe3041a9a4b210414429c1267f1 31644 elfutils_0.176-1.1.debian.tar.xz
Files:
 077e4f49320cad82bf17a997068b1db9 8646075 elfutils_0.176.orig.tar.bz2
 5296badecd902a6bf8fc7eb778cea932 455 elfutils_0.176.orig.tar.bz2.asc
 bb7fe1dd5985d32a18b7838dcce7ec80 31644 elfutils_0.176-1.1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCAAdFiEEn93aiCIaVkMmfHVORqBfr9bZ1iQFAlztnaUACgkQRqBfr9bZ
1iTy3g/+PrbQLkfwUbhL/tlbfxWxk2r/zmO23s+hLX2QjyOLrEAZlQDTnUpkMRSy
UHsLLeNaM+qoAwByqJiNhxIq0kMi+NaMWHRVi/BBPsf2f0+OuWmkdI7pGcjKK+61
6CPPs+IlWqk+aagmcki23P84LNNbmqCihM6jJt1OOj79FGQ6wIKh5MNlsZRGW2LZ
EakdnOxouCSuYyd5anC2XyhNNyRKUL+YUoyyyZFafzFo1qWX4GD1oUiAsOxxAYx3
zFcJsqujLmGZ0OQ/L5qCHidGH54bKN71/OvUkwIYSTM/jIb7hylWsBCXjhDZa5cx
cAdPMgJMj9qFIrWAO6BBd71LPajuU4DRrR4rr8VOnzBmdTPjbJWHeduSKcUNox8n
8choOXGIvBdWFpWWWMH9FRININmLUVndYiEehtBtbJMeAdX+oQhMqCo6TvSkNOWW
5LUiH9PF05KnvejMKra1bhOI7ZhWUFj47w/msOVzj9571cHmWP3VvkfIuCFrMvlz
u8YAWp//Ho4/Cs+Twl+eOseKcd4iEshSbzYI76izEVil5rujmk8YyDKDfF7n9rEw
JrmFwq222CY3w7+CoRwncdQZtTwFQ70qQtrMmM/b5LYDdKotReS13jJeAOdH+3lj
4OBM1zSIeSD3fsk0RP5ULiKyEwuGoGc/i/ZgMTEld5s5fgW7z4A=
=ZaON
-----END PGP SIGNATURE-----
