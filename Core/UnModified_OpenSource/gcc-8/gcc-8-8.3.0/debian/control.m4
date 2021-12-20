divert(-1)

define(`checkdef',`ifdef($1, , `errprint(`error: undefined macro $1
')m4exit(1)')')
define(`errexit',`errprint(`error: undefined macro `$1'
')m4exit(1)')

dnl The following macros must be defined, when called:
dnl ifdef(`SRCNAME',	, errexit(`SRCNAME'))
dnl ifdef(`PV',		, errexit(`PV'))
dnl ifdef(`ARCH',		, errexit(`ARCH'))

dnl The architecture will also be defined (-D__i386__, -D__powerpc__, etc.)

define(`PN', `$1')
ifdef(`PRI', `', `
    define(`PRI', `$1')
')
define(`MAINTAINER', `Debian GCC Maintainers <debian-gcc@lists.debian.org>')

define(`depifenabled', `ifelse(index(enabled_languages, `$1'), -1, `', `$2')')
define(`ifenabled', `ifelse(index(enabled_languages, `$1'), -1, `dnl', `$2')')

ifdef(`TARGET',`ifdef(`CROSS_ARCH',`',`undefine(`MULTIARCH')')')
define(`CROSS_ARCH', ifdef(`CROSS_ARCH', CROSS_ARCH, `all'))
define(`libdep', `lib$2$1`'LS`'AQ (ifelse(`$3',`',`>=',`$3') ifelse(`$4',`',`${gcc:Version}',`$4'))')
define(`libdevdep', `lib$2$1`'LS`'AQ (ifelse(`$3',`',`=',`$3') ifelse(`$4',`',`${gcc:Version}',`$4'))')
define(`libidevdep', `lib$2$1`'LS`'AQ (ifelse(`$3',`',`=',`$3') ifelse(`$4',`',`${gcc:Version}',`$4'))')
ifdef(`TARGET',`ifelse(CROSS_ARCH,`all',`
define(`libidevdep', `lib$2$1`'LS`'AQ (>= ifelse(`$4',`',`${gcc:SoftVersion}',`$4'))')
')')
define(`libdbgdep', `lib$2$1`'LS`'AQ (ifelse(`$3',`',`>=',`$3') ifelse(`$4',`',`${gcc:Version}',`$4'))')

define(`BUILT_USING', ifelse(add_built_using,yes,`Built-Using: ${Built-Using}
'))
define(`TARGET_PACKAGE',`X-DH-Build-For-Type: target
')

divert`'dnl
dnl --------------------------------------------------------------------------
Source: SRCNAME
Section: devel
Priority: PRI(optional)
ifelse(DIST,`Ubuntu',`dnl
ifelse(regexp(SRCNAME, `gnat\|gdc-'),0,`dnl
Maintainer: Ubuntu MOTU Developers <ubuntu-motu@lists.ubuntu.com>
', `dnl
Maintainer: Ubuntu Core developers <ubuntu-devel-discuss@lists.ubuntu.com>
')dnl SRCNAME
XSBC-Original-Maintainer: MAINTAINER
', `dnl
Maintainer: MAINTAINER
')dnl DIST
ifelse(regexp(SRCNAME, `gnat'),0,`dnl
Uploaders: Ludovic Brenta <lbrenta@debian.org>
', regexp(SRCNAME, `gdc'),0,`dnl
Uploaders: Iain Buclaw <ibuclaw@ubuntu.com>, Matthias Klose <doko@debian.org>
', `dnl
Uploaders: Matthias Klose <doko@debian.org>
')dnl SRCNAME
Standards-Version: 4.3.0
ifdef(`TARGET',`dnl cross
Build-Depends: DEBHELPER_BUILD_DEP DPKG_BUILD_DEP
  LIBC_BUILD_DEP, LIBC_BIARCH_BUILD_DEP
  kfreebsd-kernel-headers (>= 0.84) [kfreebsd-any], linux-libc-dev [m68k],
  dwz, LIBUNWIND_BUILD_DEP LIBATOMIC_OPS_BUILD_DEP AUTO_BUILD_DEP
  SOURCE_BUILD_DEP CROSS_BUILD_DEP
  ISL_BUILD_DEP MPC_BUILD_DEP MPFR_BUILD_DEP GMP_BUILD_DEP,
  zlib1g-dev, gawk, lzma, xz-utils, patchutils,
  pkg-config, libgc-dev,
  zlib1g-dev, SDT_BUILD_DEP
  bison (>= 1:2.3), flex, coreutils (>= 2.26) | realpath (>= 1.9.12), lsb-release, quilt
',`dnl native
Build-Depends: DEBHELPER_BUILD_DEP DPKG_BUILD_DEP GCC_MULTILIB_BUILD_DEP
  LIBC_BUILD_DEP, LIBC_BIARCH_BUILD_DEP LIBC_DBG_DEP
  kfreebsd-kernel-headers (>= 0.84) [kfreebsd-any], linux-libc-dev [m68k],
  AUTO_BUILD_DEP BASE_BUILD_DEP
  dwz, libunwind8-dev [ia64], libatomic-ops-dev [ia64],
  gawk, lzma, xz-utils, patchutils,
  zlib1g-dev, SDT_BUILD_DEP
  BINUTILS_BUILD_DEP,
  gperf (>= 3.0.1), bison (>= 1:2.3), flex, gettext,
  gdb`'NT [!riscv64], OFFLOAD_BUILD_DEP
  texinfo (>= 4.3), locales-all, sharutils,
  procps, FORTRAN_BUILD_DEP GNAT_BUILD_DEP GO_BUILD_DEP GDC_BUILD_DEP
  ISL_BUILD_DEP MPC_BUILD_DEP MPFR_BUILD_DEP GMP_BUILD_DEP PHOBOS_BUILD_DEP
  CHECK_BUILD_DEP coreutils (>= 2.26) | realpath (>= 1.9.12), chrpath, lsb-release, quilt,
  pkg-config, libgc-dev,
  TARGET_TOOL_BUILD_DEP
Build-Depends-Indep: LIBSTDCXX_BUILD_INDEP
')dnl
ifelse(regexp(SRCNAME, `gnat'),0,`dnl
Homepage: http://gcc.gnu.org/
', regexp(SRCNAME, `gdc'),0,`dnl
Homepage: http://gdcproject.org/
', `dnl
Homepage: http://gcc.gnu.org/
')dnl SRCNAME
Vcs-Browser: https://salsa.debian.org/toolchain-team/gcc/tree/gcc-8-debian
Vcs-Git: https://salsa.debian.org/toolchain-team/gcc.git -b gcc-8-debian
XS-Testsuite: autopkgtest

ifelse(regexp(SRCNAME, `gcc-snapshot'),0,`dnl
Package: gcc-snapshot`'TS
Architecture: any
Section: devel
Priority: optional
Depends: binutils`'TS (>= ${binutils:Version}), ${dep:libcbiarchdev}, ${dep:libcdev}, ${dep:libunwinddev}, ${snap:depends}, ${shlibs:Depends}, ${dep:ecj}, python, ${misc:Depends}
Recommends: ${snap:recommends}
Suggests: ${dep:gold}
Provides: c++-compiler`'TS`'ifdef(`TARGET',`',`, c++abi2-dev')
BUILT_USING`'dnl
Description: SNAPSHOT of the GNU Compiler Collection
 This package contains a recent development SNAPSHOT of all files
 contained in the GNU Compiler Collection (GCC).
 .
 The source code for this package has been exported from SVN trunk.
 .
 DO NOT USE THIS SNAPSHOT FOR BUILDING DEBIAN PACKAGES!
 .
 This package will NEVER hit the testing distribution. It is used for
 tracking gcc bugs submitted to the Debian BTS in recent development
 versions of gcc.
',`dnl gcc-X.Y

dnl default base package dependencies
define(`BASEDEP', `gcc`'PV`'TS-base (= ${gcc:Version})')
define(`SOFTBASEDEP', `gcc`'PV`'TS-base (>= ${gcc:SoftVersion})')

ifdef(`TARGET',`
define(`BASELDEP', `gcc`'PV`'ifelse(CROSS_ARCH,`all',`-cross')-base`'GCC_PORTS_BUILD (= ${gcc:Version})')
define(`SOFTBASELDEP', `gcc`'PV`'ifelse(CROSS_ARCH, `all',`-cross')-base`'GCC_PORTS_BUILD (>= ${gcc:SoftVersion})')
',`dnl
define(`BASELDEP', `BASEDEP')
define(`SOFTBASELDEP', `SOFTBASEDEP')
')

ifelse(index(SRCNAME, `gnat'), 0, `
define(`BASEDEP', `gnat`'PV-base (= ${gnat:Version})')
define(`SOFTBASEDEP', `gnat`'PV-base (>= ${gnat:SoftVersion})')
')

ifenabled(`gccbase',`
Package: gcc`'PV`'TS-base
Architecture: any
Multi-Arch: same
Section: ifdef(`TARGET',`devel',`libs')
Priority: ifdef(`TARGET',`optional',`PRI(required)')
Depends: ${misc:Depends}
Replaces: ${base:Replaces}
Breaks: ${base:Breaks}
BUILT_USING`'dnl
Description: GCC, the GNU Compiler Collection (base package)
 This package contains files common to all languages and libraries
 contained in the GNU Compiler Collection (GCC).
ifdef(`BASE_ONLY', `dnl
 .
 This version of GCC is not yet available for this architecture.
 Please use the compilers from the gcc-snapshot package for testing.
')`'dnl
')`'dnl gccbase

ifenabled(`gcclbase',`
Package: gcc`'PV-cross-base`'GCC_PORTS_BUILD
Architecture: all
Section: ifdef(`TARGET',`devel',`libs')
Priority: ifdef(`TARGET',`optional',`PRI(required)')
Depends: ${misc:Depends}
BUILT_USING`'dnl
Description: GCC, the GNU Compiler Collection (library base package)
 This empty package contains changelog and copyright files common to
 all libraries contained in the GNU Compiler Collection (GCC).
ifdef(`BASE_ONLY', `dnl
 .
 This version of GCC is not yet available for this architecture.
 Please use the compilers from the gcc-snapshot package for testing.
')`'dnl
')`'dnl gcclbase

ifenabled(`gnatbase',`
Package: gnat`'PV-base`'TS
Architecture: any
# "all" causes build instabilities for "any" dependencies (see #748388).
Section: libs
Priority: PRI(optional)
Depends: ${misc:Depends}
Breaks: gcc-4.6 (<< 4.6.1-8~)
BUILT_USING`'dnl
Description: GNU Ada compiler (common files)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 This package contains files common to all GNAT related packages.
')`'dnl gnatbase

ifenabled(`libgcc',`
Package: libgcc1`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Provides: ifdef(`TARGET',`libgcc1-TARGET-dcv1',`libgcc1-armel [armel], libgcc1-armhf [armhf]')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
Breaks: ${multiarch:breaks}
')`'dnl
BUILT_USING`'dnl
Description: GCC support library`'ifdef(`TARGET',` (TARGET)', `')
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libgcc1-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc1,,=,${gcc:EpochVersion}), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libgcc1-dbg-armel [armel], libgcc1-dbg-armhf [armhf]
')dnl
ifdef(`MULTIARCH',`Multi-Arch: same
')dnl
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libgcc2`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`m68k')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libgcc2-TARGET-dcv1
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
Breaks: ${multiarch:breaks}
')`'dnl
BUILT_USING`'dnl
Description: GCC support library`'ifdef(`TARGET',` (TARGET)', `')
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libgcc2-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`m68k')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc2,,=,${gcc:EpochVersion}), ${misc:Depends}
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libgcc

ifenabled(`cdev',`
Package: libgcc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: libdevel
Priority: optional
Recommends: ${dep:libcdev}
Depends: BASELDEP, ${dep:libgcc}, ${dep:libssp}, ${dep:libgomp}, ${dep:libitm},
 ${dep:libatomic}, ${dep:libbtrace}, ${dep:libasan}, ${dep:liblsan},
 ${dep:libtsan}, ${dep:libubsan}, ${dep:libvtv},
 ${dep:libmpx},
 ${dep:libqmath}, ${dep:libunwinddev}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Replaces: gccgo-8 (<< ${gcc:Version})
BUILT_USING`'dnl
Description: GCC support library (development files)
 This package contains the headers and static library files necessary for
 building C programs which use libgcc, libgomp, libquadmath, libssp or libitm.
')`'dnl libgcc

Package: libgcc4`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`hppa')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
Breaks: ${multiarch:breaks}
')`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC support library`'ifdef(`TARGET',` (TARGET)', `')
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libgcc4-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`hppa')
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc4,,=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

ifenabled(`lib64gcc',`
Package: lib64gcc1`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${misc:Depends}
ifdef(`TARGET',`Provides: lib64gcc1-TARGET-dcv1
',`')`'dnl
Conflicts: libdep(gcc`'GCC_SO,,<=,1:3.3-0pre9)
BUILT_USING`'dnl
Description: GCC support library`'ifdef(`TARGET',` (TARGET)', `') (64bit)
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: lib64gcc1-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc1,64,=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl lib64gcc

ifenabled(`cdev',`
Package: lib64gcc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: libdevel
Priority: optional
Recommends: ${dep:libcdev}
Depends: BASELDEP, ${dep:libgccbiarch}, ${dep:libsspbiarch},
 ${dep:libgompbiarch}, ${dep:libitmbiarch}, ${dep:libatomicbiarch},
 ${dep:libbtracebiarch}, ${dep:libasanbiarch}, ${dep:liblsanbiarch},
 ${dep:libtsanbiarch}, ${dep:libubsanbiarch},
 ${dep:libvtvbiarch}, ${dep:libmpxbiarch},
 ${dep:libqmathbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: gccgo-8-multilib (<< ${gcc:Version})
BUILT_USING`'dnl
Description: GCC support library (64bit development files)
 This package contains the headers and static library files necessary for
 building C programs which use libgcc, libgomp, libquadmath, libssp or libitm.
')`'dnl cdev

ifenabled(`lib32gcc',`
Package: lib32gcc1`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${misc:Depends}
Conflicts: ${confl:lib32}
ifdef(`TARGET',`Provides: lib32gcc1-TARGET-dcv1
',`')`'dnl
BUILT_USING`'dnl
Description: GCC support library (32 bit Version)
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: lib32gcc1-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc1,32,=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl lib32gcc1

ifenabled(`cdev',`
Package: lib32gcc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: libdevel
Priority: optional
Recommends: ${dep:libcdev}
Depends: BASELDEP, ${dep:libgccbiarch}, ${dep:libsspbiarch},
 ${dep:libgompbiarch}, ${dep:libitmbiarch}, ${dep:libatomicbiarch},
 ${dep:libbtracebiarch}, ${dep:libasanbiarch}, ${dep:liblsanbiarch},
 ${dep:libtsanbiarch}, ${dep:libubsanbiarch},
 ${dep:libvtvbiarch}, ${dep:libmpxbiarch},
 ${dep:libqmathbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: gccgo-8-multilib (<< ${gcc:Version})
BUILT_USING`'dnl
Description: GCC support library (32 bit development files)
 This package contains the headers and static library files necessary for
 building C programs which use libgcc, libgomp, libquadmath, libssp or libitm.
')`'dnl cdev

ifenabled(`libneongcc',`
Package: libgcc1-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC support library [neon optimized]
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneongcc1

ifenabled(`libhfgcc',`
Package: libhfgcc1`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${misc:Depends}
ifdef(`TARGET',`Provides: libhfgcc1-TARGET-dcv1
',`Conflicts: libgcc1-armhf [biarchhf_archs]
')`'dnl
BUILT_USING`'dnl
Description: GCC support library`'ifdef(`TARGET',` (TARGET)', `') (hard float ABI)
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libhfgcc1-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc1,hf,=,${gcc:EpochVersion}), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgcc1-dbg-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libhfgcc

ifenabled(`cdev',`
ifenabled(`armml',`
Package: libhfgcc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: libdevel
Priority: optional
Recommends: ${dep:libcdev}
Depends: BASELDEP, ${dep:libgccbiarch}, ${dep:libsspbiarch},
 ${dep:libgompbiarch}, ${dep:libitmbiarch}, ${dep:libatomicbiarch},
 ${dep:libbtracebiarch}, ${dep:libasanbiarch}, ${dep:liblsanbiarch},
 ${dep:libtsanbiarch}, ${dep:libubsanbiarch},
 ${dep:libvtvbiarch}, ${dep:libmpxbiarch},
 ${dep:libqmathbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: gccgo-8-multilib (<< ${gcc:Version})
BUILT_USING`'dnl
Description: GCC support library (hard float ABI development files)
 This package contains the headers and static library files necessary for
 building C programs which use libgcc, libgomp, libquadmath, libssp or libitm.
')`'dnl armml
')`'dnl cdev

ifenabled(`libsfgcc',`
Package: libsfgcc1`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${misc:Depends}
ifdef(`TARGET',`Provides: libsfgcc1-TARGET-dcv1
',`Conflicts: libgcc1-armel [biarchsf_archs]
')`'dnl
BUILT_USING`'dnl
Description: GCC support library`'ifdef(`TARGET',` (TARGET)', `') (soft float ABI)
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libsfgcc1-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc1,sf,=,${gcc:EpochVersion}), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgcc1-dbg-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libsfgcc

ifenabled(`cdev',`
ifenabled(`armml',`
Package: libsfgcc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: libdevel
Priority: optional
Recommends: ${dep:libcdev}
Depends: BASELDEP, ${dep:libgccbiarch}, ${dep:libsspbiarch},
 ${dep:libgompbiarch}, ${dep:libitmbiarch}, ${dep:libatomicbiarch},
 ${dep:libbtracebiarch}, ${dep:libasanbiarch}, ${dep:liblsanbiarch},
 ${dep:libtsanbiarch}, ${dep:libubsanbiarch},
 ${dep:libvtvbiarch}, ${dep:libmpxbiarch},
 ${dep:libqmathbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: gccgo-8-multilib (<< ${gcc:Version})
BUILT_USING`'dnl
Description: GCC support library (soft float ABI development files)
 This package contains the headers and static library files necessary for
 building C programs which use libgcc, libgomp, libquadmath, libssp or libitm.
')`'dnl armml
')`'dnl cdev

ifenabled(`libn32gcc',`
Package: libn32gcc1`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${misc:Depends}
Conflicts: libdep(gcc`'GCC_SO,,<=,1:3.3-0pre9)
ifdef(`TARGET',`Provides: libn32gcc1-TARGET-dcv1
',`')`'dnl
BUILT_USING`'dnl
Description: GCC support library`'ifdef(`TARGET',` (TARGET)', `') (n32)
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libn32gcc1-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc1,n32,=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libn32gcc

ifenabled(`cdev',`
Package: libn32gcc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: libdevel
Priority: optional
Recommends: ${dep:libcdev}
Depends: BASELDEP, ${dep:libgccbiarch}, ${dep:libsspbiarch},
 ${dep:libgompbiarch}, ${dep:libitmbiarch}, ${dep:libatomicbiarch},
 ${dep:libbtracebiarch}, ${dep:libasanbiarch}, ${dep:liblsanbiarch},
 ${dep:libtsanbiarch}, ${dep:libubsanbiarch},
 ${dep:libvtvbiarch}, ${dep:libmpxbiarch},
 ${dep:libqmathbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: gccgo-8-multilib (<< ${gcc:Version})
BUILT_USING`'dnl
Description: GCC support library (n32 development files)
 This package contains the headers and static library files necessary for
 building C programs which use libgcc, libgomp, libquadmath, libssp or libitm.
')`'dnl cdev

ifenabled(`libx32gcc',`
Package: libx32gcc1`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${misc:Depends}
ifdef(`TARGET',`Provides: libx32gcc1-TARGET-dcv1
',`')`'dnl
BUILT_USING`'dnl
Description: GCC support library`'ifdef(`TARGET',` (TARGET)', `') (x32)
 Shared version of the support library, a library of internal subroutines
 that GCC uses to overcome shortcomings of particular machines, or
 special needs for some languages.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libx32gcc1-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gcc1,x32,=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC support library (debug symbols)`'ifdef(`TARGET',` (TARGET)', `')
 Debug symbols for the GCC support library.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libx32gcc

ifenabled(`cdev',`
ifenabled(`x32dev',`
Package: libx32gcc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: libdevel
Priority: optional
Recommends: ${dep:libcdev}
Depends: BASELDEP, ${dep:libgccbiarch}, ${dep:libsspbiarch},
 ${dep:libgompbiarch}, ${dep:libitmbiarch}, ${dep:libatomicbiarch},
 ${dep:libbtracebiarch}, ${dep:libasanbiarch}, ${dep:liblsanbiarch},
 ${dep:libtsanbiarch}, ${dep:libubsanbiarch},
 ${dep:libvtvbiarch}, ${dep:libmpxbiarch},
 ${dep:libqmathbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: gccgo-8-multilib (<< ${gcc:Version})
BUILT_USING`'dnl
Description: GCC support library (x32 development files)
 This package contains the headers and static library files necessary for
 building C programs which use libgcc, libgomp, libquadmath, libssp or libitm.
')`'dnl x32dev
')`'dnl cdev

ifenabled(`cdev',`
Package: gcc`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: cpp`'PV`'TS (= ${gcc:Version}),ifenabled(`gccbase',` BASEDEP,')
  ifenabled(`gccxbase',` BASEDEP,')
  ${dep:libcc1},
  binutils`'TS (>= ${binutils:Version}),
  ${dep:libgccdev}, ${shlibs:Depends}, ${misc:Depends}
Recommends: ${dep:libcdev}
Replaces: gccgo-8 (<< ${gcc:Version}), cpp`'PV`'TS (<< 7.1.1-8)
Suggests: ${gcc:multilib}, gcc`'PV-doc (>= ${gcc:SoftVersion}),
 gcc`'PV-locales (>= ${gcc:SoftVersion}),
 libdbgdep(gcc`'GCC_SO-dbg,,>=,${libgcc:Version}),
 libdbgdep(gomp`'GOMP_SO-dbg,),
 libdbgdep(itm`'ITM_SO-dbg,),
 libdbgdep(atomic`'ATOMIC_SO-dbg,),
 libdbgdep(asan`'ASAN_SO-dbg,),
 libdbgdep(lsan`'LSAN_SO-dbg,),
 libdbgdep(tsan`'TSAN_SO-dbg,),
 libdbgdep(ubsan`'UBSAN_SO-dbg,),
ifenabled(`libvtv',`',`
 libdbgdep(vtv`'VTV_SO-dbg,),
')`'dnl
 libdbgdep(mpx`'MPX_SO-dbg,),
 libdbgdep(quadmath`'QMATH_SO-dbg,)
Provides: c-compiler`'TS
ifdef(`TARGET',`Conflicts: gcc-multilib
')`'dnl
BUILT_USING`'dnl
Description: GNU C compiler`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU C compiler, a fairly portable optimizing compiler for C.
ifdef(`TARGET', `dnl
 .
 This package contains C cross-compiler for TARGET architecture.
')`'dnl

ifenabled(`multilib',`
Package: gcc`'PV-multilib`'TS
Architecture: ifdef(`TARGET',`any',MULTILIB_ARCHS)
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, gcc`'PV`'TS (= ${gcc:Version}), ${dep:libcbiarchdev}, ${dep:libgccbiarchdev}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU C compiler (multilib support)`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU C compiler, a fairly portable optimizing compiler for C.
 .
 This is a dependency package, depending on development packages
 for the non-default multilib architecture(s).
')`'dnl multilib

ifenabled(`testresults',`
Package: gcc`'PV-test-results
Architecture: any
Section: devel
Priority: optional
Depends: BASEDEP, ${misc:Depends}
Replaces: g++-5 (<< 5.2.1-28)
BUILT_USING`'dnl
Description: Test results for the GCC test suite
 This package contains the test results for running the GCC test suite
 for a post build analysis.
')`'dnl testresults

ifenabled(`plugindev',`
Package: gcc`'PV-plugin-dev`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, gcc`'PV`'TS (= ${gcc:Version}), GMP_BUILD_DEP MPC_BUILD_DEP ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Files for GNU GCC plugin development.
 This package contains (header) files for GNU GCC plugin development. It
 is only used for the development of GCC plugins, but not needed to run
 plugins.
')`'dnl plugindev
')`'dnl cdev

ifenabled(`cdev',`
Package: gcc`'PV-hppa64-linux-gnu
Architecture: ifdef(`TARGET',`any',hppa amd64 i386 x32)
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: PRI(optional)
Depends: BASEDEP, gcc`'PV`'TS (= ${gcc:Version}),
  binutils-hppa64-linux-gnu | binutils-hppa64,
  ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU C compiler (cross compiler for hppa64)
 This is the GNU C compiler, a fairly portable optimizing compiler for C.
')`'dnl cdev

ifenabled(`cdev',`
Package: cpp`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: ifdef(`TARGET',`devel',`interpreters')
Priority: optional
Depends: BASEDEP, ${shlibs:Depends}, ${misc:Depends}
Suggests: gcc`'PV-locales (>= ${gcc:SoftVersion})
Replaces: gccgo-8 (<< ${gcc:Version})
Breaks: libmagics++-dev (<< 2.28.0-4)ifdef(`TARGET',`',`, hardening-wrapper (<< 2.8+nmu3)')
BUILT_USING`'dnl
Description: GNU C preprocessor
 A macro processor that is used automatically by the GNU C compiler
 to transform programs before actual compilation.
 .
 This package has been separated from gcc for the benefit of those who
 require the preprocessor but not the compiler.
ifdef(`TARGET', `dnl
 .
 This package contains preprocessor configured for TARGET architecture.
')`'dnl

ifdef(`TARGET', `', `
ifenabled(`gfdldoc',`
Package: cpp`'PV-doc
Architecture: all
Section: doc
Priority: PRI(optional)
Depends: gcc`'PV-base (>= ${gcc:SoftVersion}), dpkg (>= 1.15.4) | install-info, ${misc:Depends}
Description: Documentation for the GNU C preprocessor (cpp)
 Documentation for the GNU C preprocessor in info `format'.
')`'dnl gfdldoc
')`'dnl native

ifdef(`TARGET', `', `
Package: gcc`'PV-locales
Architecture: all
Section: devel
Priority: PRI(optional)
Depends: SOFTBASEDEP, cpp`'PV (>= ${gcc:SoftVersion}), ${misc:Depends}
Recommends: gcc`'PV (>= ${gcc:SoftVersion})
Description: GCC, the GNU compiler collection (native language support files)
 Native language support for GCC. Lets GCC speak your language,
 if translations are available.
 .
 Please do NOT submit bug reports in other languages than "C".
 Always reset your language settings to use the "C" locales.
')`'dnl native
')`'dnl cdev

ifenabled(`c++',`
ifenabled(`c++dev',`
Package: g++`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, gcc`'PV`'TS (= ${gcc:Version}), libidevdep(stdc++`'PV-dev,,=), ${shlibs:Depends}, ${misc:Depends}
Provides: c++-compiler`'TS`'ifdef(`TARGET',`',`, c++abi2-dev')
Suggests: ${gxx:multilib}, gcc`'PV-doc (>= ${gcc:SoftVersion}), libdbgdep(stdc++CXX_SO`'PV-dbg,)
BUILT_USING`'dnl
Description: GNU C++ compiler`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU C++ compiler, a fairly portable optimizing compiler for C++.
ifdef(`TARGET', `dnl
 .
 This package contains C++ cross-compiler for TARGET architecture.
')`'dnl

ifenabled(`multilib',`
Package: g++`'PV-multilib`'TS
Architecture: ifdef(`TARGET',`any',MULTILIB_ARCHS)
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, g++`'PV`'TS (= ${gcc:Version}), gcc`'PV-multilib`'TS (= ${gcc:Version}), ${dep:libcxxbiarchdev}, ${shlibs:Depends}, ${misc:Depends}
Suggests: ${dep:libcxxbiarchdbg}
BUILT_USING`'dnl
Description: GNU C++ compiler (multilib support)`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU C++ compiler, a fairly portable optimizing compiler for C++.
 .
 This is a dependency package, depending on development packages
 for the non-default multilib architecture(s).
')`'dnl multilib
')`'dnl c++dev
')`'dnl c++

ifdef(`TARGET', `', `
ifenabled(`ssp',`
Package: libssp`'SSP_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: any
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Section: libs
Priority: PRI(optional)
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC stack smashing protection library
 GCC can now emit code for protecting applications from stack-smashing attacks.
 The protection is realized by buffer overflow detection and reordering of
 stack variables to avoid pointer corruption.

Package: lib32ssp`'SSP_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: biarch32_archs
Section: libs
Priority: PRI(optional)
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: libssp0 (<< 4.1)
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: GCC stack smashing protection library (32bit)
 GCC can now emit code for protecting applications from stack-smashing attacks.
 The protection is realized by buffer overflow detection and reordering of
 stack variables to avoid pointer corruption.

Package: lib64ssp`'SSP_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: biarch64_archs
Section: libs
Priority: PRI(optional)
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: libssp0 (<< 4.1)
BUILT_USING`'dnl
Description: GCC stack smashing protection library (64bit)
 GCC can now emit code for protecting applications from stack-smashing attacks.
 The protection is realized by buffer overflow detection and reordering of
 stack variables to avoid pointer corruption.

Package: libn32ssp`'SSP_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: biarchn32_archs
Section: libs
Priority: PRI(optional)
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: libssp0 (<< 4.1)
BUILT_USING`'dnl
Description: GCC stack smashing protection library (n32)
 GCC can now emit code for protecting applications from stack-smashing attacks.
 The protection is realized by buffer overflow detection and reordering of
 stack variables to avoid pointer corruption.

Package: libx32ssp`'SSP_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: biarchx32_archs
Section: libs
Priority: PRI(optional)
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: libssp0 (<< 4.1)
BUILT_USING`'dnl
Description: GCC stack smashing protection library (x32)
 GCC can now emit code for protecting applications from stack-smashing attacks.
 The protection is realized by buffer overflow detection and reordering of
 stack variables to avoid pointer corruption.

Package: libhfssp`'SSP_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: biarchhf_archs
Section: libs
Priority: PRI(optional)
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC stack smashing protection library (hard float ABI)
 GCC can now emit code for protecting applications from stack-smashing attacks.
 The protection is realized by buffer overflow detection and reordering of
 stack variables to avoid pointer corruption.

Package: libsfssp`'SSP_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: biarchsf_archs
Section: libs
Priority: PRI(optional)
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC stack smashing protection library (soft float ABI)
 GCC can now emit code for protecting applications from stack-smashing attacks.
 The protection is realized by buffer overflow detection and reordering of
 stack variables to avoid pointer corruption.
')`'dnl
')`'dnl native

ifenabled(`libgomp',`
Package: libgomp`'GOMP_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libgomp'GOMP_SO`-armel [armel], libgomp'GOMP_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
Breaks: ${multiarch:breaks}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: libgomp`'GOMP_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gomp`'GOMP_SO,,=), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libgomp'GOMP_SO`-dbg-armel [armel], libgomp'GOMP_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (debug symbols)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: lib32gomp`'GOMP_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (32bit)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: lib32gomp`'GOMP_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gomp`'GOMP_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (32 bit debug symbols)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: lib64gomp`'GOMP_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (64bit)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: lib64gomp`'GOMP_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gomp`'GOMP_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (64bit debug symbols)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: libn32gomp`'GOMP_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (n32)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: libn32gomp`'GOMP_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gomp`'GOMP_SO,n32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (n32 debug symbols)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers

ifenabled(`libx32gomp',`
Package: libx32gomp`'GOMP_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (x32)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: libx32gomp`'GOMP_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gomp`'GOMP_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (x32 debug symbols)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
')`'dnl libx32gomp

ifenabled(`libhfgomp',`
Package: libhfgomp`'GOMP_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgomp'GOMP_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (hard float ABI)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: libhfgomp`'GOMP_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gomp`'GOMP_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgomp'GOMP_SO`-dbg-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (hard float ABI debug symbols)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
')`'dnl libhfgomp

ifenabled(`libsfgomp',`
Package: libsfgomp`'GOMP_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgomp'GOMP_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (soft float ABI)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.

Package: libsfgomp`'GOMP_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(gomp`'GOMP_SO,sf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgomp'GOMP_SO`-dbg-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library (soft float ABI debug symbols)
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
')`'dnl libsfgomp

ifenabled(`libneongomp',`
Package: libgomp`'GOMP_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC OpenMP (GOMP) support library [neon optimized]
 GOMP is an implementation of OpenMP for the C, C++, and Fortran compilers
 in the GNU Compiler Collection.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneongomp
')`'dnl libgomp

ifenabled(`libitm',`
Package: libitm`'ITM_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libitm'ITM_SO`-armel [armel], libitm'ITM_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.

Package: libitm`'ITM_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(itm`'ITM_SO,,=), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libitm'ITM_SO`-dbg-armel [armel], libitm'ITM_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (debug symbols)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.

Package: lib32itm`'ITM_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (32bit)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.

Package: lib32itm`'ITM_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(itm`'ITM_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (32 bit debug symbols)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.

Package: lib64itm`'ITM_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (64bit)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.

Package: lib64itm`'ITM_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(itm`'ITM_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (64bit debug symbols)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.

#Package: libn32itm`'ITM_SO`'LS
#Section: ifdef(`TARGET',`devel',`libs')
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Priority: optional
#Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
#BUILT_USING`'dnl
#Description: GNU Transactional Memory Library (n32)
# GNU Transactional Memory Library (libitm) provides transaction support for
# accesses to the memory of a process, enabling easy-to-use synchronization of
# accesses to shared memory by several threads.

#Package: libn32itm`'ITM_SO-dbg`'LS
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Section: debug
#Priority: optional
#Depends: BASELDEP, libdep(itm`'ITM_SO,n32,=), ${misc:Depends}
#BUILT_USING`'dnl
#Description: GNU Transactional Memory Library (n32 debug symbols)
# GNU Transactional Memory Library (libitm) provides transaction support for
# accesses to the memory of a process, enabling easy-to-use synchronization of
# accesses to shared memory by several threads.

ifenabled(`libx32itm',`
Package: libx32itm`'ITM_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (x32)
 This manual documents the usage and internals of libitm. It provides
 transaction support for accesses to the memory of a process, enabling
 easy-to-use synchronization of accesses to shared memory by several threads.

Package: libx32itm`'ITM_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(itm`'ITM_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (x32 debug symbols)
 This manual documents the usage and internals of libitm. It provides
 transaction support for accesses to the memory of a process, enabling
 easy-to-use synchronization of accesses to shared memory by several threads.
')`'dnl libx32itm

ifenabled(`libhfitm',`
Package: libhfitm`'ITM_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libitm'ITM_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (hard float ABI)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.

Package: libhfitm`'ITM_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(itm`'ITM_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libitm'ITM_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (hard float ABI debug symbols)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.
')`'dnl libhfitm

ifenabled(`libsfitm',`
Package: libsfitm`'ITM_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (soft float ABI)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.

Package: libsfitm`'ITM_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(itm`'ITM_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library (soft float ABI debug symbols)
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.
')`'dnl libsfitm

ifenabled(`libneonitm',`
Package: libitm`'ITM_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Transactional Memory Library [neon optimized]
 GNU Transactional Memory Library (libitm) provides transaction support for
 accesses to the memory of a process, enabling easy-to-use synchronization of
 accesses to shared memory by several threads.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneonitm
')`'dnl libitm

ifenabled(`libatomic',`
Package: libatomic`'ATOMIC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libatomic'ATOMIC_SO`-armel [armel], libatomic'ATOMIC_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: libatomic`'ATOMIC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(atomic`'ATOMIC_SO,,=), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libatomic'ATOMIC_SO`-dbg-armel [armel], libatomic'ATOMIC_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (debug symbols)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: lib32atomic`'ATOMIC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (32bit)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: lib32atomic`'ATOMIC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(atomic`'ATOMIC_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (32 bit debug symbols)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: lib64atomic`'ATOMIC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (64bit)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: lib64atomic`'ATOMIC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(atomic`'ATOMIC_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (64bit debug symbols)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: libn32atomic`'ATOMIC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (n32)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: libn32atomic`'ATOMIC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(atomic`'ATOMIC_SO,n32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (n32 debug symbols)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

ifenabled(`libx32atomic',`
Package: libx32atomic`'ATOMIC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (x32)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: libx32atomic`'ATOMIC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(atomic`'ATOMIC_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (x32 debug symbols)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.
')`'dnl libx32atomic

ifenabled(`libhfatomic',`
Package: libhfatomic`'ATOMIC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libatomic'ATOMIC_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (hard float ABI)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: libhfatomic`'ATOMIC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(atomic`'ATOMIC_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libatomic'ATOMIC_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (hard float ABI debug symbols)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.
')`'dnl libhfatomic

ifenabled(`libsfatomic',`
Package: libsfatomic`'ATOMIC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (soft float ABI)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.

Package: libsfatomic`'ATOMIC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(atomic`'ATOMIC_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions (soft float ABI debug symbols)
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.
')`'dnl libsfatomic

ifenabled(`libneonatomic',`
Package: libatomic`'ATOMIC_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: support library providing __atomic built-in functions [neon optimized]
 library providing __atomic built-in functions. When an atomic call cannot
 be turned into lock-free instructions, GCC will make calls into this library.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneonatomic
')`'dnl libatomic

ifenabled(`libasan',`
Package: libasan`'ASAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libasan'ASAN_SO`-armel [armel], libasan'ASAN_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

Package: libasan`'ASAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(asan`'ASAN_SO,,=), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libasan'ASAN_SO`-dbg-armel [armel], libasan'ASAN_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (debug symbols)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

Package: lib32asan`'ASAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (32bit)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

Package: lib32asan`'ASAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(asan`'ASAN_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (32 bit debug symbols)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

Package: lib64asan`'ASAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (64bit)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

Package: lib64asan`'ASAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(asan`'ASAN_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (64bit debug symbols)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

#Package: libn32asan`'ASAN_SO`'LS
#Section: ifdef(`TARGET',`devel',`libs')
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Priority: optional
#Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
#BUILT_USING`'dnl
#Description: AddressSanitizer -- a fast memory error detector (n32)
# AddressSanitizer (ASan) is a fast memory error detector.  It finds
# use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

#Package: libn32asan`'ASAN_SO-dbg`'LS
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Section: debug
#Priority: optional
#Depends: BASELDEP, libdep(asan`'ASAN_SO,n32,=), ${misc:Depends}
#BUILT_USING`'dnl
#Description: AddressSanitizer -- a fast memory error detector (n32 debug symbols)
# AddressSanitizer (ASan) is a fast memory error detector.  It finds
# use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

ifenabled(`libx32asan',`
Package: libx32asan`'ASAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (x32)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

Package: libx32asan`'ASAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(asan`'ASAN_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (x32 debug symbols)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.
')`'dnl libx32asan

ifenabled(`libhfasan',`
Package: libhfasan`'ASAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libasan'ASAN_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (hard float ABI)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

Package: libhfasan`'ASAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(asan`'ASAN_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libasan'ASAN_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (hard float ABI debug symbols)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.
')`'dnl libhfasan

ifenabled(`libsfasan',`
Package: libsfasan`'ASAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (soft float ABI)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.

Package: libsfasan`'ASAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(asan`'ASAN_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector (soft float ABI debug symbols)
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.
')`'dnl libsfasan

ifenabled(`libneonasan',`
Package: libasan`'ASAN_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: AddressSanitizer -- a fast memory error detector [neon optimized]
 AddressSanitizer (ASan) is a fast memory error detector.  It finds
 use-after-free and {heap,stack,global}-buffer overflow bugs in C/C++ programs.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneonasan
')`'dnl libasan

ifenabled(`liblsan',`
Package: liblsan`'LSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (runtime)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer.

Package: liblsan`'LSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(lsan`'LSAN_SO,,=), ${misc:Depends}
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (debug symbols)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer.

ifenabled(`lib32lsan',`
Package: lib32lsan`'LSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (32bit)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer (empty package).

Package: lib32lsan`'LSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(lsan`'LSAN_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (32 bit debug symbols)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer (empty package).
')`'dnl lib32lsan

ifenabled(`lib64lsan',`
#Package: lib64lsan`'LSAN_SO`'LS
#Section: ifdef(`TARGET',`devel',`libs')
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
#Priority: optional
#Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
#BUILT_USING`'dnl
#Description: LeakSanitizer -- a memory leak detector (64bit)
# LeakSanitizer (Lsan) is a memory leak detector which is integrated
# into AddressSanitizer.

#Package: lib64lsan`'LSAN_SO-dbg`'LS
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
#Section: debug
#Priority: optional
#Depends: BASELDEP, libdep(lsan`'LSAN_SO,64,=), ${misc:Depends}
#BUILT_USING`'dnl
#Description: LeakSanitizer -- a memory leak detector (64bit debug symbols)
# LeakSanitizer (Lsan) is a memory leak detector which is integrated
# into AddressSanitizer.
')`'dnl lib64lsan

ifenabled(`libn32lsan',`
#Package: libn32lsan`'LSAN_SO`'LS
#Section: ifdef(`TARGET',`devel',`libs')
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Priority: optional
#Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
#BUILT_USING`'dnl
#Description: LeakSanitizer -- a memory leak detector (n32)
# LeakSanitizer (Lsan) is a memory leak detector which is integrated
# into AddressSanitizer.

#Package: libn32lsan`'LSAN_SO-dbg`'LS
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Section: debug
#Priority: optional
#Depends: BASELDEP, libdep(lsan`'LSAN_SO,n32,=), ${misc:Depends}
#BUILT_USING`'dnl
#Description: LeakSanitizer -- a memory leak detector (n32 debug symbols)
# LeakSanitizer (Lsan) is a memory leak detector which is integrated
# into AddressSanitizer.
')`'dnl libn32lsan

ifenabled(`libx32lsan',`
Package: libx32lsan`'LSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (x32)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer (empty package).

Package: libx32lsan`'LSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(lsan`'LSAN_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (x32 debug symbols)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer (empty package).
')`'dnl libx32lsan

ifenabled(`libhflsan',`
Package: libhflsan`'LSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: liblsan'LSAN_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (hard float ABI)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer.

Package: libhflsan`'LSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(lsan`'LSAN_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: liblsan'LSAN_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (hard float ABI debug symbols)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer.
')`'dnl libhflsan

ifenabled(`libsflsan',`
Package: libsflsan`'LSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (soft float ABI)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer.

Package: libsflsan`'LSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(lsan`'LSAN_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector (soft float ABI debug symbols)
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer.
')`'dnl libsflsan

ifenabled(`libneonlsan',`
Package: liblsan`'LSAN_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: LeakSanitizer -- a memory leak detector [neon optimized]
 LeakSanitizer (Lsan) is a memory leak detector which is integrated
 into AddressSanitizer.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneonlsan
')`'dnl liblsan

ifenabled(`libtsan',`
Package: libtsan`'TSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libtsan'TSAN_SO`-armel [armel], libtsan'TSAN_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (runtime)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 

Package: libtsan`'TSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(tsan`'TSAN_SO,,=), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libtsan'TSAN_SO`-dbg-armel [armel], libtsan'TSAN_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (debug symbols)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 

ifenabled(`lib32tsan',`
Package: lib32tsan`'TSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (32bit)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 

Package: lib32tsan`'TSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(tsan`'TSAN_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (32 bit debug symbols)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 
')`'dnl lib32tsan

ifenabled(`lib64tsan',`
Package: lib64tsan`'TSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (64bit)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 

Package: lib64tsan`'TSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(tsan`'TSAN_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (64bit debug symbols)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 
')`'dnl lib64tsan

ifenabled(`libn32tsan',`
Package: libn32tsan`'TSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (n32)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 

Package: libn32tsan`'TSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(tsan`'TSAN_SO,n32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (n32 debug symbols)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 
')`'dnl libn32tsan

ifenabled(`libx32tsan',`
Package: libx32tsan`'TSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (x32)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 

Package: libx32tsan`'TSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(tsan`'TSAN_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (x32 debug symbols)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 
')`'dnl libx32tsan

ifenabled(`libhftsan',`
Package: libhftsan`'TSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libtsan'TSAN_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (hard float ABI)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 

Package: libhftsan`'TSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(tsan`'TSAN_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libtsan'TSAN_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (hard float ABI debug symbols)
')`'dnl libhftsan

ifenabled(`libsftsan',`
Package: libsftsan`'TSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (soft float ABI)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 

Package: libsftsan`'TSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(tsan`'TSAN_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races (soft float ABI debug symbols)
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 
')`'dnl libsftsan

ifenabled(`libneontsan',`
Package: libtsan`'TSAN_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: ThreadSanitizer -- a Valgrind-based detector of data races [neon optimized]
 ThreadSanitizer (Tsan) is a data race detector for C/C++ programs. 
 The Linux and Mac versions are based on Valgrind. 
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneontsan
')`'dnl libtsan

ifenabled(`libubsan',`
Package: libubsan`'UBSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libubsan'UBSAN_SO`-armel [armel], libubsan'UBSAN_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (runtime)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.

Package: libubsan`'UBSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(ubsan`'UBSAN_SO,,=), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libubsan'UBSAN_SO`-dbg-armel [armel], libubsan'UBSAN_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (debug symbols)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.

ifenabled(`lib32ubsan',`
Package: lib32ubsan`'UBSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (32bit)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.

Package: lib32ubsan`'UBSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(ubsan`'UBSAN_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (32 bit debug symbols)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.
')`'dnl lib32ubsan

ifenabled(`lib64ubsan',`
Package: lib64ubsan`'UBSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (64bit)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.

Package: lib64ubsan`'UBSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(ubsan`'UBSAN_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (64bit debug symbols)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.
')`'dnl lib64ubsan

ifenabled(`libn32ubsan',`
#Package: libn32ubsan`'UBSAN_SO`'LS
#Section: ifdef(`TARGET',`devel',`libs')
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Priority: optional
#Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
#BUILT_USING`'dnl
#Description: UBSan -- undefined behaviour sanitizer (n32)
# UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
# Various computations will be instrumented to detect undefined behavior
# at runtime. Available for C and C++.

#Package: libn32ubsan`'UBSAN_SO-dbg`'LS
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Section: debug
#Priority: optional
#Depends: BASELDEP, libdep(ubsan`'UBSAN_SO,n32,=), ${misc:Depends}
#BUILT_USING`'dnl
#Description: UBSan -- undefined behaviour sanitizer (n32 debug symbols)
# UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
# Various computations will be instrumented to detect undefined behavior
# at runtime. Available for C and C++.
')`'dnl libn32ubsan

ifenabled(`libx32ubsan',`
Package: libx32ubsan`'UBSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (x32)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.

Package: libx32ubsan`'UBSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(ubsan`'UBSAN_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (x32 debug symbols)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.
')`'dnl libx32ubsan

ifenabled(`libhfubsan',`
Package: libhfubsan`'UBSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libubsan'UBSAN_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (hard float ABI)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.

Package: libhfubsan`'UBSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(ubsan`'UBSAN_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libubsan'UBSAN_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (hard float ABI debug symbols)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.
')`'dnl libhfubsan

ifenabled(`libsfubsan',`
Package: libsfubsan`'UBSAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (soft float ABI)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.

Package: libsfubsan`'UBSAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(ubsan`'UBSAN_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer (soft float ABI debug symbols)
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.
')`'dnl libsfubsan

ifenabled(`libneonubsan',`
Package: libubsan`'UBSAN_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: UBSan -- undefined behaviour sanitizer [neon optimized]
 UndefinedBehaviorSanitizer can be enabled via -fsanitize=undefined.
 Various computations will be instrumented to detect undefined behavior
 at runtime. Available for C and C++.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneonubsan
')`'dnl libubsan

ifenabled(`libvtv',`
Package: libvtv`'VTV_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (runtime)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.

Package: libvtv`'VTV_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(vtv`'VTV_SO,,=), ${misc:Depends}
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: GNU vtable verification library (debug symbols)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.

ifenabled(`lib32vtv',`
Package: lib32vtv`'VTV_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: GNU vtable verification library (32bit)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.

Package: lib32vtv`'VTV_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(vtv`'VTV_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (32 bit debug symbols)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.
')`'dnl lib32vtv

ifenabled(`lib64vtv',`
Package: lib64vtv`'VTV_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (64bit)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.

Package: lib64vtv`'VTV_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(vtv`'VTV_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (64bit debug symbols)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.
')`'dnl lib64vtv

ifenabled(`libn32vtv',`
Package: libn32vtv`'VTV_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (n32)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.

Package: libn32vtv`'VTV_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(vtv`'VTV_SO,n32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (n32 debug symbols)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.
')`'dnl libn32vtv

ifenabled(`libx32vtv',`
Package: libx32vtv`'VTV_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (x32)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.

Package: libx32vtv`'VTV_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(vtv`'VTV_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (x32 debug symbols)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.
')`'dnl libx32vtv

ifenabled(`libhfvtv',`
Package: libhfvtv`'VTV_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libvtv'VTV_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: GNU vtable verification library (hard float ABI)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.

Package: libhfvtv`'VTV_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(vtv`'VTV_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libvtv'VTV_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: GNU vtable verification library (hard float ABI debug symbols)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.
')`'dnl libhfvtv

ifenabled(`libsfvtv',`
Package: libsfvtv`'VTV_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (soft float ABI)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.

Package: libsfvtv`'VTV_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(vtv`'VTV_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library (soft float ABI debug symbols)
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.
')`'dnl libsfvtv

ifenabled(`libneonvtv',`
Package: libvtv`'VTV_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU vtable verification library [neon optimized]
 Vtable verification is a new security hardening feature for GCC that
 is designed to detect and handle (during program execution) when a
 vtable pointer that is about to be used for a virtual function call is
 not a valid vtable pointer for that call.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneonvtv
')`'dnl libvtv

ifenabled(`libmpx',`
Package: libmpx`'MPX_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libmpx'MPX_SO`-armel [armel], libmpx'MPX_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Replaces: libmpx0 (<< 6-20160120-1)
BUILT_USING`'dnl
Description: Intel memory protection extensions (runtime)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.

Package: libmpx`'MPX_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(mpx`'MPX_SO,,=), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libmpx'MPX_SO`-dbg-armel [armel], libmpx'MPX_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: Intel memory protection extensions (debug symbols)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.

ifenabled(`lib32mpx',`
Package: lib32mpx`'MPX_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
Replaces: lib32mpx0 (<< 6-20160120-1)
BUILT_USING`'dnl
Description: Intel memory protection extensions (32bit)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.

Package: lib32mpx`'MPX_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(mpx`'MPX_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Intel memory protection extensions (32 bit debug symbols)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.
')`'dnl lib32mpx

ifenabled(`lib64mpx',`
Package: lib64mpx`'MPX_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: lib64mpx0 (<< 6-20160120-1)
BUILT_USING`'dnl
Description: Intel memory protection extensions (64bit)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.

Package: lib64mpx`'MPX_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(mpx`'MPX_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Intel memory protection extensions (64bit debug symbols)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.
')`'dnl lib64mpx

ifenabled(`libn32mpx',`
Package: libn32mpx`'MPX_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Intel memory protection extensions (n32)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.

Package: libn32mpx`'MPX_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(mpx`'MPX_SO,n32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Intel memory protection extensions (n32 debug symbols)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.
')`'dnl libn32mpx

ifenabled(`libx32mpx',`
Package: libx32mpx`'MPX_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Intel memory protection extensions (x32)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.

Package: libx32mpx`'MPX_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(mpx`'MPX_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Intel memory protection extensions (x32 debug symbols)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.
')`'dnl libx32mpx

ifenabled(`libhfmpx',`
Package: libhfmpx`'MPX_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libmpx'MPX_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: Intel memory protection extensions (hard float ABI)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.

Package: libhfmpx`'MPX_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(mpx`'MPX_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libmpx'MPX_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: Intel memory protection extensions (hard float ABI debug symbols)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.
')`'dnl libhfmpx

ifenabled(`libsfmpx',`
Package: libsfmpx`'MPX_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Intel memory protection extensions (soft float ABI)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.

Package: libsfmpx`'MPX_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(mpx`'MPX_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Intel memory protection extensions (soft float ABI debug symbols)
 Intel MPX is a set of processor features which, with compiler,
 runtime library and OS support, brings increased robustness to
 software by checking pointer references whose compile time normal
 intentions are usurped at runtime due to buffer overflow.
')`'dnl libsfmpx
')`'dnl libmpx

ifenabled(`libbacktrace',`
Package: libbacktrace`'BTRACE_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libbacktrace'BTRACE_SO`-armel [armel], libbacktrace'BTRACE_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: libbacktrace`'BTRACE_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(backtrace`'BTRACE_SO,,=), ${misc:Depends}
ifdef(`TARGET',`',`Provides: libbacktrace'BTRACE_SO`-dbg-armel [armel], libbacktrace'BTRACE_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: stack backtrace library (debug symbols)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: lib32backtrace`'BTRACE_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: stack backtrace library (32bit)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: lib32backtrace`'BTRACE_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(backtrace`'BTRACE_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (32 bit debug symbols)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: lib64backtrace`'BTRACE_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (64bit)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: lib64backtrace`'BTRACE_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(backtrace`'BTRACE_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (64bit debug symbols)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: libn32backtrace`'BTRACE_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (n32)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: libn32backtrace`'BTRACE_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(backtrace`'BTRACE_SO,n32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (n32 debug symbols)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

ifenabled(`libx32backtrace',`
Package: libx32backtrace`'BTRACE_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (x32)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: libx32backtrace`'BTRACE_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(backtrace`'BTRACE_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (x32 debug symbols)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.
')`'dnl libx32backtrace

ifenabled(`libhfbacktrace',`
Package: libhfbacktrace`'BTRACE_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libbacktrace'BTRACE_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: stack backtrace library (hard float ABI)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: libhfbacktrace`'BTRACE_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(backtrace`'BTRACE_SO,hf,=), ${misc:Depends}
wifdef(`TARGET',`dnl',`Conflicts: libbacktrace'BTRACE_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: stack backtrace library (hard float ABI debug symbols)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.
')`'dnl libhfbacktrace

ifenabled(`libsfbacktrace',`
Package: libsfbacktrace`'BTRACE_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (soft float ABI)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.

Package: libsfbacktrace`'BTRACE_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(backtrace`'BTRACE_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library (soft float ABI debug symbols)
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.
')`'dnl libsfbacktrace

ifenabled(`libneonbacktrace',`
Package: libbacktrace`'BTRACE_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: stack backtrace library [neon optimized]
 libbacktrace uses the GCC unwind interface to collect a stack trace,
 and parses DWARF debug info to get file/line/function information.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneonbacktrace
')`'dnl libbacktrace


ifenabled(`libqmath',`
Package: libquadmath`'QMATH_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype. The library is used to provide on such
 targets the REAL(16) type in the GNU Fortran compiler.

Package: libquadmath`'QMATH_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(quadmath`'QMATH_SO,,=), ${misc:Depends}
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (debug symbols)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype.

Package: lib32quadmath`'QMATH_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (32bit)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype. The library is used to provide on such
 targets the REAL(16) type in the GNU Fortran compiler.

Package: lib32quadmath`'QMATH_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(quadmath`'QMATH_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (32 bit debug symbols)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype.

Package: lib64quadmath`'QMATH_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library  (64bit)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype. The library is used to provide on such
 targets the REAL(16) type in the GNU Fortran compiler.

Package: lib64quadmath`'QMATH_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(quadmath`'QMATH_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library  (64bit debug symbols)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype.

#Package: libn32quadmath`'QMATH_SO`'LS
#Section: ifdef(`TARGET',`devel',`libs')
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Priority: optional
#Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
#BUILT_USING`'dnl
#Description: GCC Quad-Precision Math Library (n32)
# A library, which provides quad-precision mathematical functions on targets
# supporting the __float128 datatype. The library is used to provide on such
# targets the REAL(16) type in the GNU Fortran compiler.

#Package: libn32quadmath`'QMATH_SO-dbg`'LS
#Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
#Section: debug
#Priority: optional
#Depends: BASELDEP, libdep(quadmath`'QMATH_SO,n32,=), ${misc:Depends}
#BUILT_USING`'dnl
#Description: GCC Quad-Precision Math Library (n32 debug symbols)
# A library, which provides quad-precision mathematical functions on targets
# supporting the __float128 datatype.

ifenabled(`libx32qmath',`
Package: libx32quadmath`'QMATH_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (x32)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype. The library is used to provide on such
 targets the REAL(16) type in the GNU Fortran compiler.

Package: libx32quadmath`'QMATH_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(quadmath`'QMATH_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (x32 debug symbols)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype.
')`'dnl libx32qmath

ifenabled(`libhfqmath',`
Package: libhfquadmath`'QMATH_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (hard float ABI)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype. The library is used to provide on such
 targets the REAL(16) type in the GNU Fortran compiler.

Package: libhfquadmath`'QMATH_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(quadmath`'QMATH_SO,hf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (hard float ABI debug symbols)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype.
')`'dnl libhfqmath

ifenabled(`libsfqmath',`
Package: libsfquadmath`'QMATH_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (soft float ABI)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype. The library is used to provide on such
 targets the REAL(16) type in the GNU Fortran compiler.

Package: libsfquadmath`'QMATH_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(quadmath`'QMATH_SO,sf,=), ${misc:Depends}
BUILT_USING`'dnl
Description: GCC Quad-Precision Math Library (hard float ABI debug symbols)
 A library, which provides quad-precision mathematical functions on targets
 supporting the __float128 datatype.
')`'dnl libsfqmath
')`'dnl libqmath

ifenabled(`libcc1',`
Package: libcc1-`'CC1_SO
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASEDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC cc1 plugin for GDB
 libcc1 is a plugin for GDB.
')`'dnl libcc1

ifenabled(`libjit',`
Package: libgccjit`'GCCJIT_SO
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASEDEP, libgcc`'PV-dev, binutils, ${shlibs:Depends}, ${misc:Depends}
Breaks: python-gccjit (<< 0.4-4), python3-gccjit (<< 0.4-4)
BUILT_USING`'dnl
Description: GCC just-in-time compilation (shared library)
 libgccjit provides an embeddable shared library with an API for adding
 compilation to existing programs using GCC.

Package: libgccjit`'GCCJIT_SO-dbg
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASEDEP, libgccjit`'GCCJIT_SO (= ${gcc:Version}),
 ${shlibs:Depends}, ${misc:Depends}
Breaks: libgccjit-5-dbg, libgccjit-6-dbg
Replaces: libgccjit-5-dbg, libgccjit-6-dbg
BUILT_USING`'dnl
Description: GCC just-in-time compilation (debug information)
 libgccjit provides an embeddable shared library with an API for adding
 compilation to existing programs using GCC.
')`'dnl libjit

ifenabled(`jit',`
Package: libgccjit`'PV-doc
Section: doc
Architecture: all
Priority: optional
Depends: BASEDEP, ${misc:Depends}
Conflicts: libgccjit-5-doc, libgccjit-6-doc, libgccjit-7-doc
Description: GCC just-in-time compilation (documentation)
 libgccjit provides an embeddable shared library with an API for adding
 compilation to existing programs using GCC.

Package: libgccjit`'PV-dev
Section: ifdef(`TARGET',`devel',`libdevel')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASEDEP, libgccjit`'GCCJIT_SO (>= ${gcc:Version}),
 ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Suggests: libgccjit`'PV-dbg
Description: GCC just-in-time compilation (development files)
 libgccjit provides an embeddable shared library with an API for adding
 compilation to existing programs using GCC.
')`'dnl jit

ifenabled(`objpp',`
ifenabled(`objppdev',`
Package: gobjc++`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
Depends: BASEDEP, gobjc`'PV`'TS (= ${gcc:Version}), g++`'PV`'TS (= ${gcc:Version}), ${shlibs:Depends}, libidevdep(objc`'PV-dev,,=), ${misc:Depends}
Suggests: ${gobjcxx:multilib}, gcc`'PV-doc (>= ${gcc:SoftVersion})
Provides: objc++-compiler`'TS
BUILT_USING`'dnl
Description: GNU Objective-C++ compiler
 This is the GNU Objective-C++ compiler, which compiles
 Objective-C++ on platforms supported by the gcc compiler. It uses the
 gcc backend to generate optimized code.
')`'dnl obcppdev

ifenabled(`multilib',`
Package: gobjc++`'PV-multilib`'TS
Architecture: ifdef(`TARGET',`any',MULTILIB_ARCHS)
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, gobjc++`'PV`'TS (= ${gcc:Version}), g++`'PV-multilib`'TS (= ${gcc:Version}), gobjc`'PV-multilib`'TS (= ${gcc:Version}), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Objective-C++ compiler (multilib support)
 This is the GNU Objective-C++ compiler, which compiles Objective-C++ on
 platforms supported by the gcc compiler.
 .
 This is a dependency package, depending on development packages
 for the non-default multilib architecture(s).
')`'dnl multilib
')`'dnl obcpp

ifenabled(`objc',`
ifenabled(`objcdev',`
Package: gobjc`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
Depends: BASEDEP, gcc`'PV`'TS (= ${gcc:Version}), ${dep:libcdev}, ${shlibs:Depends}, libidevdep(objc`'PV-dev,,=), ${misc:Depends}
Suggests: ${gobjc:multilib}, gcc`'PV-doc (>= ${gcc:SoftVersion}), libdbgdep(objc`'OBJC_SO-dbg,)
Provides: objc-compiler`'TS
ifdef(`__sparc__',`Conflicts: gcc`'PV-sparc64', `dnl')
BUILT_USING`'dnl
Description: GNU Objective-C compiler
 This is the GNU Objective-C compiler, which compiles
 Objective-C on platforms supported by the gcc compiler. It uses the
 gcc backend to generate optimized code.

ifenabled(`multilib',`
Package: gobjc`'PV-multilib`'TS
Architecture: ifdef(`TARGET',`any',MULTILIB_ARCHS)
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, gobjc`'PV`'TS (= ${gcc:Version}), gcc`'PV-multilib`'TS (= ${gcc:Version}), ${dep:libobjcbiarchdev}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Objective-C compiler (multilib support)`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU Objective-C compiler, which compiles Objective-C on platforms
 supported by the gcc compiler.
 .
 This is a dependency package, depending on development packages
 for the non-default multilib architecture(s).
')`'dnl multilib

Package: libobjc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,), libdep(objc`'OBJC_SO,), ${shlibs:Depends}, ${misc:Depends}
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (development files)
 This package contains the headers and static library files needed to build
 GNU ObjC applications.

Package: lib64objc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,64), libdep(objc`'OBJC_SO,64), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (64bit development files)
 This package contains the headers and static library files needed to build
 GNU ObjC applications.

Package: lib32objc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,32), libdep(objc`'OBJC_SO,32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (32bit development files)
 This package contains the headers and static library files needed to build
 GNU ObjC applications.

Package: libn32objc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,n32), libdep(objc`'OBJC_SO,n32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (n32 development files)
 This package contains the headers and static library files needed to build
 GNU ObjC applications.

ifenabled(`x32dev',`
Package: libx32objc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,x32), libdep(objc`'OBJC_SO,x32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (x32 development files)
 This package contains the headers and static library files needed to build
 GNU ObjC applications.
')`'dnl libx32objc

ifenabled(`armml',`
Package: libhfobjc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,hf), libdep(objc`'OBJC_SO,hf), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (hard float ABI development files)
 This package contains the headers and static library files needed to build
 GNU ObjC applications.
')`'dnl armml

ifenabled(`armml',`
Package: libsfobjc`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,sf), libdep(objc`'OBJC_SO,sf), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (soft float development files)
 This package contains the headers and static library files needed to build
 GNU ObjC applications.
')`'dnl armml
')`'dnl objcdev

ifenabled(`libobjc',`
Package: libobjc`'OBJC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libobjc'OBJC_SO`-armel [armel], libobjc'OBJC_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
ifelse(OBJC_SO,`2',`Breaks: ${multiarch:breaks}
',`')')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications
 Library needed for GNU ObjC applications linked against the shared library.

Package: libobjc`'OBJC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libobjc'OBJC_SO`-dbg-armel [armel], libobjc'OBJC_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Priority: optional
Depends: BASELDEP, libdep(objc`'OBJC_SO,,=), libdbgdep(gcc`'GCC_SO-dbg,,>=,${libgcc:Version}), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (debug symbols)
 Library needed for GNU ObjC applications linked against the shared library.
')`'dnl libobjc

ifenabled(`lib64objc',`
Package: lib64objc`'OBJC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (64bit)
 Library needed for GNU ObjC applications linked against the shared library.

Package: lib64objc`'OBJC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, libdep(objc`'OBJC_SO,64,=), libdbgdep(gcc`'GCC_SO-dbg,64,>=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (64 bit debug symbols)
 Library needed for GNU ObjC applications linked against the shared library.
')`'dnl lib64objc

ifenabled(`lib32objc',`
Package: lib32objc`'OBJC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (32bit)
 Library needed for GNU ObjC applications linked against the shared library.

Package: lib32objc`'OBJC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, libdep(objc`'OBJC_SO,32,=), libdbgdep(gcc`'GCC_SO-dbg,32,>=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (32 bit debug symbols)
 Library needed for GNU ObjC applications linked against the shared library.
')`'dnl lib32objc

ifenabled(`libn32objc',`
Package: libn32objc`'OBJC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (n32)
 Library needed for GNU ObjC applications linked against the shared library.

Package: libn32objc`'OBJC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, libdep(objc`'OBJC_SO,n32,=), libdbgdep(gcc`'GCC_SO-dbg,n32,>=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (n32 debug symbols)
 Library needed for GNU ObjC applications linked against the shared library.
')`'dnl libn32objc

ifenabled(`libx32objc',`
Package: libx32objc`'OBJC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (x32)
 Library needed for GNU ObjC applications linked against the shared library.

Package: libx32objc`'OBJC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, libdep(objc`'OBJC_SO,x32,=), libdbgdep(gcc`'GCC_SO-dbg,x32,>=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (x32 debug symbols)
 Library needed for GNU ObjC applications linked against the shared library.
')`'dnl libx32objc

ifenabled(`libhfobjc',`
Package: libhfobjc`'OBJC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libobjc'OBJC_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (hard float ABI)
 Library needed for GNU ObjC applications linked against the shared library.

Package: libhfobjc`'OBJC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, libdep(objc`'OBJC_SO,hf,=), libdbgdep(gcc`'GCC_SO-dbg,hf,>=,${gcc:EpochVersion}), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libobjc'OBJC_SO`-dbg-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (hard float ABI debug symbols)
 Library needed for GNU ObjC applications linked against the shared library.
')`'dnl libhfobjc

ifenabled(`libsfobjc',`
Package: libsfobjc`'OBJC_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libobjc'OBJC_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (soft float ABI)
 Library needed for GNU ObjC applications linked against the shared library.

Package: libsfobjc`'OBJC_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, libdep(objc`'OBJC_SO,sf,=), libdbgdep(gcc`'GCC_SO-dbg,sf,>=,${gcc:EpochVersion}), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libobjc'OBJC_SO`-dbg-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications (soft float ABI debug symbols)
 Library needed for GNU ObjC applications linked against the shared library.
')`'dnl libsfobjc

ifenabled(`libneonobjc',`
Package: libobjc`'OBJC_SO-neon`'LS
TARGET_PACKAGE`'dnl
Section: libs
Architecture: NEON_ARCHS
Priority: PRI(optional)
Depends: BASELDEP, libc6-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Objective-C applications  [NEON version]
 Library needed for GNU ObjC applications linked against the shared library.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneonobjc
')`'dnl objc

ifenabled(`fortran',`
ifenabled(`fdev',`
Package: gfortran`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
Depends: BASEDEP, gcc`'PV`'TS (= ${gcc:Version}), libidevdep(gfortran`'PV-dev,,=), ${dep:libcdev}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`',`Provides: fortran95-compiler, ${fortran:mod-version}
')dnl
Suggests: ${gfortran:multilib}, gfortran`'PV-doc,
 libdbgdep(gfortran`'FORTRAN_SO-dbg,),
 libcoarrays-dev
BUILT_USING`'dnl
Description: GNU Fortran compiler
 This is the GNU Fortran compiler, which compiles
 Fortran on platforms supported by the gcc compiler. It uses the
 gcc backend to generate optimized code.

ifenabled(`multilib',`
Package: gfortran`'PV-multilib`'TS
Architecture: ifdef(`TARGET',`any',MULTILIB_ARCHS)
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, gfortran`'PV`'TS (= ${gcc:Version}), gcc`'PV-multilib`'TS (= ${gcc:Version}), ${dep:libgfortranbiarchdev}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Fortran compiler (multilib support)`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU Fortran compiler, which compiles Fortran on platforms
 supported by the gcc compiler.
 .
 This is a dependency package, depending on development packages
 for the non-default multilib architecture(s).
')`'dnl multilib

ifenabled(`gfdldoc',`
Package: gfortran`'PV-doc
Architecture: all
Section: doc
Priority: PRI(optional)
Depends: gcc`'PV-base (>= ${gcc:SoftVersion}), dpkg (>= 1.15.4) | install-info, ${misc:Depends}
Description: Documentation for the GNU Fortran compiler (gfortran)
 Documentation for the GNU Fortran compiler in info `format'.
')`'dnl gfdldoc

Package: libgfortran`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev`',), libdep(gfortran`'FORTRAN_SO,), ${shlibs:Depends}, ${misc:Depends}
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (development files)
 This package contains the headers and static library files needed to build
 GNU Fortran applications.

Package: lib64gfortran`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev`',64), libdep(gfortran`'FORTRAN_SO,64), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (64bit development files)
 This package contains the headers and static library files needed to build
 GNU Fortran applications.

Package: lib32gfortran`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev`',32), libdep(gfortran`'FORTRAN_SO,32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (32bit development files)
 This package contains the headers and static library files needed to build
 GNU Fortran applications.

Package: libn32gfortran`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev`',n32), libdep(gfortran`'FORTRAN_SO,n32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (n32 development files)
 This package contains the headers and static library files needed to build
 GNU Fortran applications.

ifenabled(`x32dev',`
Package: libx32gfortran`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev`',x32), libdep(gfortran`'FORTRAN_SO,x32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (x32 development files)
 This package contains the headers and static library files needed to build
 GNU Fortran applications.
')`'dnl libx32gfortran

ifenabled(`armml',`
Package: libhfgfortran`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev`',hf), libdep(gfortran`'FORTRAN_SO,hf), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (hard float ABI development files)
 This package contains the headers and static library files needed to build
 GNU Fortran applications.
')`'dnl armml

ifenabled(`armml',`
Package: libsfgfortran`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev`',sf), libdep(gfortran`'FORTRAN_SO,sf), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (soft float ABI development files)
 This package contains the headers and static library files needed to build
 GNU Fortran applications.
')`'dnl armml
')`'dnl fdev

ifenabled(`libgfortran',`
Package: libgfortran`'FORTRAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libgfortran'FORTRAN_SO`-armel [armel], libgfortran'FORTRAN_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
Breaks: ${multiarch:breaks}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications
 Library needed for GNU Fortran applications linked against the
 shared library.

Package: libgfortran`'FORTRAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libgfortran'FORTRAN_SO`-dbg-armel [armel], libgfortran'FORTRAN_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Priority: optional
Depends: BASELDEP, libdep(gfortran`'FORTRAN_SO,,=), libdbgdep(gcc`'GCC_SO-dbg,,>=,${libgcc:Version}), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (debug symbols)
 Library needed for GNU Fortran applications linked against the
 shared library.
')`'dnl libgfortran

ifenabled(`lib64gfortran',`
Package: lib64gfortran`'FORTRAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (64bit)
 Library needed for GNU Fortran applications linked against the
 shared library.

Package: lib64gfortran`'FORTRAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, libdep(gfortran`'FORTRAN_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (64bit debug symbols)
 Library needed for GNU Fortran applications linked against the
 shared library.
')`'dnl lib64gfortran

ifenabled(`lib32gfortran',`
Package: lib32gfortran`'FORTRAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (32bit)
 Library needed for GNU Fortran applications linked against the
 shared library.

Package: lib32gfortran`'FORTRAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, libdep(gfortran`'FORTRAN_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (32 bit debug symbols)
 Library needed for GNU Fortran applications linked against the
 shared library.
')`'dnl lib32gfortran

ifenabled(`libn32gfortran',`
Package: libn32gfortran`'FORTRAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (n32)
 Library needed for GNU Fortran applications linked against the
 shared library.

Package: libn32gfortran`'FORTRAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, libdep(gfortran`'FORTRAN_SO,n32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (n32 debug symbols)
 Library needed for GNU Fortran applications linked against the
 shared library.
')`'dnl libn32gfortran

ifenabled(`libx32gfortran',`
Package: libx32gfortran`'FORTRAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (x32)
 Library needed for GNU Fortran applications linked against the
 shared library.

Package: libx32gfortran`'FORTRAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, libdep(gfortran`'FORTRAN_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (x32 debug symbols)
 Library needed for GNU Fortran applications linked against the
 shared library.
')`'dnl libx32gfortran

ifenabled(`libhfgfortran',`
Package: libhfgfortran`'FORTRAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgfortran'FORTRAN_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (hard float ABI)
 Library needed for GNU Fortran applications linked against the
 shared library.

Package: libhfgfortran`'FORTRAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, libdep(gfortran`'FORTRAN_SO,hf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgfortran'FORTRAN_SO`-dbg-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (hard float ABI debug symbols)
 Library needed for GNU Fortran applications linked against the
 shared library.
')`'dnl libhfgfortran

ifenabled(`libsfgfortran',`
Package: libsfgfortran`'FORTRAN_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgfortran'FORTRAN_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (soft float ABI)
 Library needed for GNU Fortran applications linked against the
 shared library.

Package: libsfgfortran`'FORTRAN_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, libdep(gfortran`'FORTRAN_SO,sf,=), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libgfortran'FORTRAN_SO`-dbg-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications (hard float ABI debug symbols)
 Library needed for GNU Fortran applications linked against the
 shared library.
')`'dnl libsfgfortran

ifenabled(`libneongfortran',`
Package: libgfortran`'FORTRAN_SO-neon`'LS
TARGET_PACKAGE`'dnl
Section: libs
Architecture: NEON_ARCHS
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
Breaks: ${multiarch:breaks}
')`'dnl
Priority: optional
Depends: BASELDEP, libgcc1-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Fortran applications [NEON version]
 Library needed for GNU Fortran applications linked against the
 shared library.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl libneongfortran
')`'dnl fortran

ifenabled(`ggo',`
ifenabled(`godev',`
Package: gccgo`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
Depends: BASEDEP, ifdef(`STANDALONEGO',`${dep:libcc1}, ',`gcc`'PV`'TS (= ${gcc:Version}), ')libidevdep(go`'GO_SO,,>=), ${dep:libcdev}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`',`Provides: go-compiler
')dnl
Suggests: ${go:multilib}, gccgo`'PV-doc, libdbgdep(go`'GO_SO-dbg,)
Conflicts: ${golang:Conflicts}
Breaks: libgo12`'LS (<< 8-20171209-2)
Replaces: libgo12`'LS (<< 8-20171209-2)
BUILT_USING`'dnl
Description: GNU Go compiler
 This is the GNU Go compiler, which compiles Go on platforms supported
 by the gcc compiler. It uses the gcc backend to generate optimized code.

ifenabled(`multilib',`
Package: gccgo`'PV-multilib`'TS
Architecture: ifdef(`TARGET',`any',MULTILIB_ARCHS)
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, gccgo`'PV`'TS (= ${gcc:Version}), ifdef(`STANDALONEGO',,`gcc`'PV-multilib`'TS (= ${gcc:Version}), ')${dep:libgobiarch}, ${shlibs:Depends}, ${misc:Depends}
Suggests: ${dep:libgobiarchdbg}
Breaks: lib32go12 (<< 8-20171209-2),
  libn32go12`'LS (<< 8-20171209-2),
  libx32go12`'LS (<< 8-20171209-2),
  lib64go12`'LS (<< 8-20171209-2)
Replaces: lib32go12`'LS (<< 8-20171209-2),
  libn32go12`'LS (<< 8-20171209-2),
  libx32go12`'LS (<< 8-20171209-2),
  lib64go12`'LS (<< 8-20171209-2)
BUILT_USING`'dnl
Description: GNU Go compiler (multilib support)`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU Go compiler, which compiles Go on platforms supported
 by the gcc compiler.
 .
 This is a dependency package, depending on development packages
 for the non-default multilib architecture(s).
')`'dnl multilib

ifenabled(`gfdldoc',`
Package: gccgo`'PV-doc
Architecture: all
Section: doc
Priority: PRI(optional)
Depends: gcc`'PV-base (>= ${gcc:SoftVersion}), dpkg (>= 1.15.4) | install-info, ${misc:Depends}
BUILT_USING`'dnl
Description: Documentation for the GNU Go compiler (gccgo)
 Documentation for the GNU Go compiler in info `format'.
')`'dnl gfdldoc
')`'dnl godev

ifenabled(`libggo',`
Package: libgo`'GO_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libgo'GO_SO`-armel [armel], libgo'GO_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Replaces: libgo3`'LS, libgo8`'LS
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications
 Library needed for GNU Go applications linked against the
 shared library.

Package: libgo`'GO_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libgo'GO_SO`-dbg-armel [armel], libgo'GO_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Priority: optional
Depends: BASELDEP, libdep(go`'GO_SO,,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (debug symbols)
 Library needed for GNU Go applications linked against the
 shared library. This currently is an empty package, because the
 library is completely unstripped.
')`'dnl libgo

ifenabled(`lib64ggo',`
Package: lib64go`'GO_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: lib64go3`'LS, lib64go8`'LS
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (64bit)
 Library needed for GNU Go applications linked against the
 shared library.

Package: lib64go`'GO_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, libdep(go`'GO_SO,64,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (64bit debug symbols)
 Library needed for GNU Go applications linked against the
 shared library. This currently is an empty package, because the
 library is completely unstripped.
')`'dnl lib64go

ifenabled(`lib32ggo',`
Package: lib32go`'GO_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
Replaces: lib32go3`'LS, lib32go8`'LS
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (32bit)
 Library needed for GNU Go applications linked against the
 shared library.

Package: lib32go`'GO_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, libdep(go`'GO_SO,32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (32 bit debug symbols)
 Library needed for GNU Go applications linked against the
 shared library. This currently is an empty package, because the
 library is completely unstripped.
')`'dnl lib32go

ifenabled(`libn32ggo',`
Package: libn32go`'GO_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: libn32go3`'LS, libn32go8`'LS
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (n32)
 Library needed for GNU Go applications linked against the
 shared library.

Package: libn32go`'GO_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, libdep(go`'GO_SO,n32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (n32 debug symbols)
 Library needed for GNU Go applications linked against the
 shared library. This currently is an empty package, because the
 library is completely unstripped.
')`'dnl libn32go

ifenabled(`libx32ggo',`
Package: libx32go`'GO_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Replaces: libx32go3`'LS, libx32go8`'LS
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (x32)
 Library needed for GNU Go applications linked against the
 shared library.

Package: libx32go`'GO_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, libdep(go`'GO_SO,x32,=), ${misc:Depends}
BUILT_USING`'dnl
Description: Runtime library for GNU Go applications (x32 debug symbols)
 Library needed for GNU Go applications linked against the
 shared library. This currently is an empty package, because the
 library is completely unstripped.
')`'dnl libx32go
')`'dnl ggo

ifenabled(`c++',`
ifenabled(`libcxx',`
Package: libstdc++CXX_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, ${dep:libc}, ${shlibs:Depends}, ${misc:Depends}
Provides: ifdef(`TARGET',`libstdc++CXX_SO-TARGET-dcv1',`libstdc++'CXX_SO`-armel [armel], libstdc++'CXX_SO`-armhf [armhf]')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
Breaks: ${multiarch:breaks}, PR66145BREAKS
')`'dnl
Conflicts: scim (<< 1.4.2-1)
Replaces: libstdc++CXX_SO`'PV-dbg`'LS (<< 4.9.0-3)
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3`'ifdef(`TARGET',` (TARGET)', `')
 This package contains an additional runtime library for C++ programs
 built with the GNU compiler.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libcxx

ifenabled(`lib32cxx',`
Package: lib32stdc++CXX_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, libdep(gcc1,32), ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
ifdef(`TARGET',`Provides: lib32stdc++CXX_SO-TARGET-dcv1
',`')`'dnl
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (32 bit Version)
 This package contains an additional runtime library for C++ programs
 built with the GNU compiler.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl lib32cxx

ifenabled(`lib64cxx',`
Package: lib64stdc++CXX_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, libdep(gcc1,64), ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: lib64stdc++CXX_SO-TARGET-dcv1
',`')`'dnl
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3`'ifdef(`TARGET',` (TARGET)', `') (64bit)
 This package contains an additional runtime library for C++ programs
 built with the GNU compiler.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl lib64cxx

ifenabled(`libn32cxx',`
Package: libn32stdc++CXX_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, libdep(gcc1,n32), ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libn32stdc++CXX_SO-TARGET-dcv1
',`')`'dnl
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3`'ifdef(`TARGET',` (TARGET)', `') (n32)
 This package contains an additional runtime library for C++ programs
 built with the GNU compiler.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libn32cxx

ifenabled(`libx32cxx',`
Package: libx32stdc++CXX_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, libdep(gcc1,x32), ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libx32stdc++CXX_SO-TARGET-dcv1
',`')`'dnl
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3`'ifdef(`TARGET',` (TARGET)', `') (x32)
 This package contains an additional runtime library for C++ programs
 built with the GNU compiler.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libx32cxx

ifenabled(`libhfcxx',`
Package: libhfstdc++CXX_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, libdep(gcc1,hf), ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libhfstdc++CXX_SO-TARGET-dcv1
',`')`'dnl
ifdef(`TARGET',`dnl',`Conflicts: libstdc++'CXX_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3`'ifdef(`TARGET',` (TARGET)', `') (hard float ABI)
 This package contains an additional runtime library for C++ programs
 built with the GNU compiler.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libhfcxx

ifenabled(`libsfcxx',`
Package: libsfstdc++CXX_SO`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: ifdef(`TARGET',`devel',`libs')
Priority: optional
Depends: BASELDEP, libdep(gcc1,sf), ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libsfstdc++CXX_SO-TARGET-dcv1
',`')`'dnl
ifdef(`TARGET',`dnl',`Conflicts: libstdc++'CXX_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3`'ifdef(`TARGET',` (TARGET)', `') (soft float ABI)
 This package contains an additional runtime library for C++ programs
 built with the GNU compiler.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libsfcxx

ifenabled(`libneoncxx',`
Package: libstdc++CXX_SO-neon`'LS
TARGET_PACKAGE`'dnl
Architecture: NEON_ARCHS
Section: libs
Priority: optional
Depends: BASELDEP, libc6-neon`'LS, libgcc1-neon`'LS, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 [NEON version]
 This package contains an additional runtime library for C++ programs
 built with the GNU compiler.
 .
 This set of libraries is optimized to use a NEON coprocessor, and will
 be selected instead when running under systems which have one.
')`'dnl

ifenabled(`c++dev',`
Package: libstdc++`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,,=),
 libdep(stdc++CXX_SO,,>=), ${dep:libcdev}, ${misc:Depends}
ifdef(`TARGET',`',`dnl native
Conflicts: libg++27-dev, libg++272-dev (<< 2.7.2.8-1), libstdc++2.8-dev,
 libg++2.8-dev, libstdc++2.9-dev, libstdc++2.9-glibc2.1-dev,
 libstdc++2.10-dev (<< 1:2.95.3-2), libstdc++3.0-dev
Suggests: libstdc++`'PV-doc
')`'dnl native
Provides: libstdc++-dev`'LS`'ifdef(`TARGET',`, libstdc++-dev-TARGET-dcv1')
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (development files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the headers and static library files necessary for
 building C++ programs which use libstdc++.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libstdc++`'PV-pic`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdep(stdc++CXX_SO,),
 libdevdep(stdc++`'PV-dev,), ${misc:Depends}
ifdef(`TARGET',`Provides: libstdc++-pic-TARGET-dcv1
',`')`'dnl
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (shared library subset kit)`'ifdef(`TARGET',` (TARGET)', `')
 This is used to develop subsets of the libstdc++ shared libraries for
 use on custom installation floppies and in embedded systems.
 .
 Unless you are making one of those, you will not need this package.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libstdc++CXX_SO`'PV-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(stdc++CXX_SO,),
 libdbgdep(gcc`'GCC_SO-dbg,,>=,${libgcc:Version}), ${shlibs:Depends}, ${misc:Depends}
Provides: ifdef(`TARGET',`libstdc++CXX_SO-dbg-TARGET-dcv1',`libstdc++'CXX_SO`'PV`-dbg-armel [armel], libstdc++'CXX_SO`'PV`-dbg-armhf [armhf]')
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Recommends: libdevdep(stdc++`'PV-dev,)
Conflicts: libstdc++5-dbg`'LS, libstdc++5-3.3-dbg`'LS, libstdc++6-dbg`'LS,
 libstdc++6-4.0-dbg`'LS, libstdc++6-4.1-dbg`'LS, libstdc++6-4.2-dbg`'LS,
 libstdc++6-4.3-dbg`'LS, libstdc++6-4.4-dbg`'LS, libstdc++6-4.5-dbg`'LS,
 libstdc++6-4.6-dbg`'LS, libstdc++6-4.7-dbg`'LS, libstdc++6-4.8-dbg`'LS,
 libstdc++6-4.9-dbg`'LS, libstdc++6-5-dbg`'LS, libstdc++6-6-dbg`'LS,
 libstdc++6-7-dbg`'LS
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (debugging files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the shared library of libstdc++ compiled with
 debugging symbols.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: lib32stdc++`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,32),
 libdep(stdc++CXX_SO,32), libdevdep(stdc++`'PV-dev,), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (development files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the headers and static library files necessary for
 building C++ programs which use libstdc++.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: lib32stdc++CXX_SO`'PV-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(stdc++CXX_SO,32),
 libdevdep(stdc++`'PV-dev,), libdbgdep(gcc`'GCC_SO-dbg,32,>=,${gcc:EpochVersion}),
 ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: lib32stdc++CXX_SO-dbg-TARGET-dcv1
',`')`'dnl
Conflicts: lib32stdc++6-dbg`'LS, lib32stdc++6-4.0-dbg`'LS,
 lib32stdc++6-4.1-dbg`'LS, lib32stdc++6-4.2-dbg`'LS, lib32stdc++6-4.3-dbg`'LS,
 lib32stdc++6-4.4-dbg`'LS, lib32stdc++6-4.5-dbg`'LS, lib32stdc++6-4.6-dbg`'LS,
 lib32stdc++6-4.7-dbg`'LS, lib32stdc++6-4.8-dbg`'LS, lib32stdc++6-4.9-dbg`'LS,
 lib32stdc++6-5-dbg`'LS, lib32stdc++6-6-dbg`'LS, lib32stdc++6-7-dbg`'LS
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (debugging files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the shared library of libstdc++ compiled with
 debugging symbols.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: lib64stdc++`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,64),
 libdep(stdc++CXX_SO,64), libdevdep(stdc++`'PV-dev,), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (development files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the headers and static library files necessary for
 building C++ programs which use libstdc++.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: lib64stdc++CXX_SO`'PV-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(stdc++CXX_SO,64),
 libdevdep(stdc++`'PV-dev,), libdbgdep(gcc`'GCC_SO-dbg,64,>=,${gcc:EpochVersion}),
 ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: lib64stdc++CXX_SO-dbg-TARGET-dcv1
',`')`'dnl
Conflicts: lib64stdc++6-dbg`'LS, lib64stdc++6-4.0-dbg`'LS,
 lib64stdc++6-4.1-dbg`'LS, lib64stdc++6-4.2-dbg`'LS, lib64stdc++6-4.3-dbg`'LS,
 lib64stdc++6-4.4-dbg`'LS, lib64stdc++6-4.5-dbg`'LS, lib64stdc++6-4.6-dbg`'LS,
 lib64stdc++6-4.7-dbg`'LS, lib64stdc++6-4.8-dbg`'LS, lib64stdc++6-4.9-dbg`'LS,
 lib64stdc++6-5-dbg`'LS, lib64stdc++6-6-dbg`'LS, lib64stdc++6-7-dbg`'LS,
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (debugging files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the shared library of libstdc++ compiled with
 debugging symbols.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libn32stdc++`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,n32),
 libdep(stdc++CXX_SO,n32), libdevdep(stdc++`'PV-dev,), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (development files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the headers and static library files necessary for
 building C++ programs which use libstdc++.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libn32stdc++CXX_SO`'PV-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(stdc++CXX_SO,n32),
 libdevdep(stdc++`'PV-dev,), libdbgdep(gcc`'GCC_SO-dbg,n32,>=,${gcc:EpochVersion}),
 ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libn32stdc++CXX_SO-dbg-TARGET-dcv1
',`')`'dnl
Conflicts: libn32stdc++6-dbg`'LS, libn32stdc++6-4.0-dbg`'LS,
 libn32stdc++6-4.1-dbg`'LS, libn32stdc++6-4.2-dbg`'LS, libn32stdc++6-4.3-dbg`'LS,
 libn32stdc++6-4.4-dbg`'LS, libn32stdc++6-4.5-dbg`'LS, libn32stdc++6-4.6-dbg`'LS,
 libn32stdc++6-4.7-dbg`'LS, libn32stdc++6-4.8-dbg`'LS, libn32stdc++6-4.9-dbg`'LS,
 libn32stdc++6-5-dbg`'LS, libn32stdc++6-6-dbg`'LS, libn32stdc++6-7-dbg`'LS,
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (debugging files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the shared library of libstdc++ compiled with
 debugging symbols.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

ifenabled(`x32dev',`
Package: libx32stdc++`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,x32), libdep(stdc++CXX_SO,x32),
 libdevdep(stdc++`'PV-dev,), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (development files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the headers and static library files necessary for
 building C++ programs which use libstdc++.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl x32dev

ifenabled(`libx32dbgcxx',`
Package: libx32stdc++CXX_SO`'PV-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(stdc++CXX_SO,x32),
 libdevdep(stdc++`'PV-dev,), libdbgdep(gcc`'GCC_SO-dbg,x32,>=,${gcc:EpochVersion}),
 ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libx32stdc++CXX_SO-dbg-TARGET-dcv1
',`')`'dnl
Conflicts: libx32stdc++6-dbg`'LS, libx32stdc++6-4.6-dbg`'LS,
 libx32stdc++6-4.7-dbg`'LS, libx32stdc++6-4.8-dbg`'LS, libx32stdc++6-4.9-dbg`'LS,
 libx32stdc++6-5-dbg`'LS, libx32stdc++6-6-dbg`'LS, libx32stdc++6-7-dbg`'LS,
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (debugging files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the shared library of libstdc++ compiled with
 debugging symbols.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libx32dbgcxx

ifenabled(`libhfdbgcxx',`
Package: libhfstdc++`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,hf),
 libdep(stdc++CXX_SO,hf), libdevdep(stdc++`'PV-dev,), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (development files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the headers and static library files necessary for
 building C++ programs which use libstdc++.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libhfstdc++CXX_SO`'PV-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(stdc++CXX_SO,hf),
 libdevdep(stdc++`'PV-dev,), libdbgdep(gcc`'GCC_SO-dbg,hf,>=,${gcc:EpochVersion}),
 ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libhfstdc++CXX_SO-dbg-TARGET-dcv1
',`')`'dnl
ifdef(`TARGET',`dnl',`Conflicts: libhfstdc++6-dbg`'LS, libhfstdc++6-4.3-dbg`'LS, libhfstdc++6-4.4-dbg`'LS, libhfstdc++6-4.5-dbg`'LS, libhfstdc++6-4.6-dbg`'LS, libhfstdc++6-4.7-dbg`'LS, libhfstdc++6-4.8-dbg`'LS, libhfstdc++6-4.9-dbg`'LS, libhfstdc++6-5-dbg`'LS, libhfstdc++6-6-dbg`'LS, libhfstdc++6-7-dbg`'LS, libstdc++'CXX_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (debugging files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the shared library of libstdc++ compiled with
 debugging symbols.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libhfdbgcxx

ifenabled(`libsfdbgcxx',`
Package: libsfstdc++`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: ifdef(`TARGET',`devel',`libdevel')
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,sf),
 libdep(stdc++CXX_SO,sf), libdevdep(stdc++`'PV-dev,), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (development files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the headers and static library files necessary for
 building C++ programs which use libstdc++.
 .
 libstdc++-v3 is a complete rewrite from the previous libstdc++-v2, which
 was included up to g++-2.95. The first version of libstdc++-v3 appeared
 in g++-3.0.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl

Package: libsfstdc++CXX_SO`'PV-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: debug
Priority: optional
Depends: BASELDEP, libdep(stdc++CXX_SO,sf),
 libdevdep(stdc++`'PV-dev,), libdbgdep(gcc`'GCC_SO-dbg,sf,>=,${gcc:EpochVersion}),
 ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`Provides: libsfstdc++CXX_SO-dbg-TARGET-dcv1
',`')`'dnl
ifdef(`TARGET',`dnl',`Conflicts: libsfstdc++6-dbg`'LS, libsfstdc++6-4.3-dbg`'LS, libsfstdc++6-4.4-dbg`'LS, libsfstdc++6-4.5-dbg`'LS, libsfstdc++6-4.6-dbg`'LS, libsfstdc++6-4.7-dbg`'LS, libsfstdc++6-4.8-dbg`'LS, libsfstdc++6-4.9-dbg`'LS, libsfstdc++6-5-dbg`'LS, libhfstdc++6-6-dbg`'LS, libhfstdc++6-7-dbg`'LS, libstdc++'CXX_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: GNU Standard C++ Library v3 (debugging files)`'ifdef(`TARGET',` (TARGET)', `')
 This package contains the shared library of libstdc++ compiled with
 debugging symbols.
ifdef(`TARGET', `dnl
 .
 This package contains files for TARGET architecture, for use in cross-compile
 environment.
')`'dnl
')`'dnl libsfdbgcxx

ifdef(`TARGET', `', `
Package: libstdc++`'PV-doc
Architecture: all
Section: doc
Priority: PRI(optional)
Depends: gcc`'PV-base (>= ${gcc:SoftVersion}), ${misc:Depends}
Conflicts: libstdc++5-doc, libstdc++5-3.3-doc, libstdc++6-doc,
 libstdc++6-4.0-doc, libstdc++6-4.1-doc, libstdc++6-4.2-doc, libstdc++6-4.3-doc,
 libstdc++6-4.4-doc, libstdc++6-4.5-doc, libstdc++6-4.6-doc, libstdc++6-4.7-doc,
 libstdc++-4.8-doc, libstdc++-4.9-doc, libstdc++-5-doc, libstdc++-6-doc,
 libstdc++-7-doc,
Description: GNU Standard C++ Library v3 (documentation files)
 This package contains documentation files for the GNU stdc++ library.
 .
 One set is the distribution documentation, the other set is the
 source documentation including a namespace list, class hierarchy,
 alphabetical list, compound list, file list, namespace members,
 compound members and file members.
')`'dnl native
')`'dnl c++dev
')`'dnl c++

ifenabled(`ada',`
Package: gnat`'-GNAT_V`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
ifdef(`MULTIARCH', `Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Depends: BASEDEP, gcc`'PV`'TS (>= ${gcc:SoftVersion}), ${dep:libgnat}, ${dep:libcdev}, ${shlibs:Depends}, ${misc:Depends}
Suggests: gnat`'PV-doc, ada-reference-manual-2012, gnat`'-GNAT_V-sjlj
Breaks:   gnat (<< 4.6.1), dh-ada-library (<< 6.0), gnat-4.6-base (= 4.6.4-2),
 gnat-4.9-base (= 4.9-20140330-1)
Replaces: gnat (<< 4.6.1), dh-ada-library (<< 6.0), gnat-4.6-base (= 4.6.4-2),
 gnat-4.9-base (= 4.9-20140330-1)
# Takes over symlink from gnat (<< 4.6.1): /usr/bin/gnatgcc.
# Takes over file from dh-ada-library (<< 6.0): debian_packaging.mk.
# g-base 4.6.4-2, 4.9-20140330-1 contain debian_packaging.mk by mistake.
# Newer versions of gnat and dh-ada-library will not provide these files.
Conflicts: gnat (<< 4.1), gnat-3.1, gnat-3.2, gnat-3.3, gnat-3.4, gnat-3.5,
 gnat-4.0, gnat-4.1, gnat-4.2, gnat-4.3, gnat-4.4, gnat-4.6, gnat-4.7, gnat-4.8,
 gnat-4.9, gnat-5`'TS, gnat-6`'TS, gnat-7`'TS,
# These other packages will continue to provide /usr/bin/gnatmake and
# other files.
BUILT_USING`'dnl
Description: GNU Ada compiler
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 This package provides the compiler, tools and runtime library that handles
 exceptions using the default zero-cost mechanism.

ifenabled(`adasjlj',`
Package: gnat`'-GNAT_V-sjlj`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
ifdef(`MULTIARCH', `Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Depends: BASEDEP, gnat`'-GNAT_V`'TS (= ${gnat:Version}), ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Ada compiler (setjump/longjump runtime library)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 This package provides an alternative runtime library that handles
 exceptions using the setjump/longjump mechanism (as a static library
 only).  You can install it to supplement the normal compiler.
')`'dnl adasjlj

ifenabled(`libgnat',`
Package: libgnat`'-GNAT_V`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: runtime for applications compiled with GNAT (shared library)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 The libgnat library provides runtime components needed by most
 applications produced with GNAT.
 .
 This package contains the runtime shared library.

Package: libgnat`'-GNAT_V-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Depends: BASELDEP, libgnat`'-GNAT_V`'LS (= ${gnat:Version}), ${misc:Depends}
BUILT_USING`'dnl
Description: runtime for applications compiled with GNAT (debugging symbols)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 The libgnat library provides runtime components needed by most
 applications produced with GNAT.
 .
 This package contains the debugging symbols.

ifdef(`TARGET',`',`
Package: libgnatvsn`'GNAT_V-dev`'LS
TARGET_PACKAGE`'dnl
Section: libdevel
Architecture: any
Priority: optional
Depends: BASELDEP, gnat`'PV`'TS (= ${gnat:Version}),
 libgnatvsn`'GNAT_V`'LS (= ${gnat:Version}), ${misc:Depends}
Conflicts: libgnatvsn-dev (<< `'GNAT_V),
 libgnatvsn4.1-dev, libgnatvsn4.3-dev, libgnatvsn4.4-dev,
 libgnatvsn4.5-dev, libgnatvsn4.6-dev, libgnatvsn4.9-dev,
 libgnatvsn5-dev`'LS, libgnatvsn6-dev`'LS, libgnatvsn7-dev`'LS,
BUILT_USING`'dnl
Description: GNU Ada compiler selected components (development files)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 The libgnatvsn library exports selected GNAT components for use in other
 packages, most notably ASIS tools. It is licensed under the GNAT-Modified
 GPL, allowing to link proprietary programs with it.
 .
 This package contains the development files and static library.

Package: libgnatvsn`'GNAT_V`'LS
TARGET_PACKAGE`'dnl
Architecture: any
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: PRI(optional)
Section: libs
Depends: BASELDEP, libgnat`'-GNAT_V`'LS (= ${gnat:Version}),
 ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU Ada compiler selected components (shared library)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 The libgnatvsn library exports selected GNAT components for use in other
 packages, most notably ASIS tools. It is licensed under the GNAT-Modified
 GPL, allowing to link proprietary programs with it.
 .
 This package contains the runtime shared library.

Package: libgnatvsn`'GNAT_V-dbg`'LS
TARGET_PACKAGE`'dnl
Architecture: any
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
')`'dnl
Priority: optional
Section: debug
Depends: BASELDEP, libgnatvsn`'GNAT_V`'LS (= ${gnat:Version}), ${misc:Depends}
Suggests: gnat
BUILT_USING`'dnl
Description: GNU Ada compiler selected components (debugging symbols)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 The libgnatvsn library exports selected GNAT components for use in other
 packages, most notably ASIS tools. It is licensed under the GNAT-Modified
 GPL, allowing to link proprietary programs with it.
 .
 This package contains the debugging symbols.
')`'dnl native
')`'dnl libgnat

ifenabled(`lib64gnat',`
Package: lib64gnat`'-GNAT_V
Section: libs
Architecture: biarch64_archs
Priority: PRI(optional)
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: runtime for applications compiled with GNAT (64 bits shared library)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 The libgnat library provides runtime components needed by most
 applications produced with GNAT.
 .
 This package contains the runtime shared library for 64 bits architectures.
')`'dnl libgnat

ifenabled(`gfdldoc',`
Package: gnat`'PV-doc
Architecture: all
Section: doc
Priority: PRI(optional)
Depends: dpkg (>= 1.15.4) | install-info, ${misc:Depends}
Suggests: gnat`'PV
Conflicts: gnat-4.1-doc, gnat-4.2-doc,
  gnat-4.3-doc, gnat-4.4-doc,
  gnat-4.6-doc, gnat-4.9-doc,
  gnat-5-doc, gnat-6-doc, gnat-7-doc,
BUILT_USING`'dnl
Description: GNU Ada compiler (documentation)
 GNAT is a compiler for the Ada programming language. It produces optimized
 code on platforms supported by the GNU Compiler Collection (GCC).
 .
 The libgnat library provides runtime components needed by most
 applications produced with GNAT.
 .
 This package contains the documentation in info `format'.
')`'dnl gfdldoc
')`'dnl ada

ifenabled(`d ',`
Package: gdc`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
Depends: SOFTBASEDEP, g++`'PV`'TS (>= ${gcc:SoftVersion}), ${dep:gdccross}, ${dep:phobosdev}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`',`Provides: gdc, d-compiler, d-v2-compiler
')dnl
Replaces: gdc (<< 4.4.6-5)
BUILT_USING`'dnl
Description: GNU D compiler (version 2)`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU D compiler, which compiles D on platforms supported by gcc.
 It uses the gcc backend to generate optimised code.
 .
 This compiler supports D language version 2.

ifenabled(`multilib',`
Package: gdc`'PV-multilib`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
Depends: SOFTBASEDEP, gdc`'PV`'TS (= ${gcc:Version}), gcc`'PV-multilib`'TS (= ${gcc:Version}), ${dep:libphobosbiarchdev}${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU D compiler (version 2, multilib support)`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU D compiler, which compiles D on platforms supported by gcc.
 It uses the gcc backend to generate optimised code.
 .
 This is a dependency package, depending on development packages
 for the non-default multilib architecture(s).
')`'dnl multilib

ifenabled(`libdevphobos',`
Package: libgphobos`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`libphobos_archs')
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Section: libdevel
Priority: optional
Depends: BASELDEP, libgphobos`'PHOBOS_V`'LS (>= ${gdc:Version}),
  zlib1g-dev, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: lib64gphobos`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, lib64gphobos`'PHOBOS_V`'LS (>= ${gdc:Version}),
  libdevdep(gcc`'PV-dev,64), ifdef(`TARGET',`',`lib64z1-dev,') ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library (64bit development files)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: lib32gphobos`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, lib32gphobos`'PHOBOS_V`'LS (>= ${gdc:Version}),
  libdevdep(gcc`'PV-dev,32), ifdef(`TARGET',`',`lib32z1-dev,') ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library (32bit development files)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

ifenabled(`libdevn32phobos',`
Package: libn32gphobos`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libn32gphobos`'PHOBOS_V`'LS (>= ${gdc:Version}),
  libdevdep(gcc`'PV-dev,n32), ifdef(`TARGET',`',`libn32z1-dev,') ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library (n32 development files)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/
')`'dnl libn32phobos

ifenabled(`libdevx32phobos',`
Package: libx32gphobos`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libx32gphobos`'PHOBOS_V`'LS (>= ${gdc:Version}),
  libdevdep(gcc`'PV-dev,x32), ifdef(`TARGET',`',`${dep:libx32z},') ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library (x32 development files)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/
')`'dnl libx32phobos

ifenabled(`armml',`
Package: libhfgphobos`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libhfgphobos`'PHOBOS_V`'LS (>= ${gdc:Version}),
  libdevdep(gcc`'PV-dev,hf), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library (hard float ABI development files)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: libsfgphobos`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libsfgphobos`'PHOBOS_V`'LS (>= ${gdc:Version}),
  libdevdep(gcc`'PV-dev,sf), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library (soft float ABI development files)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/
')`'dnl armml
')`'dnl libdevphobos

ifenabled(`libphobos',`
Package: libgphobos`'PHOBOS_V`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`libphobos_archs')
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Replaces: libgphobos68`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (runtime library)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: libgphobos`'PHOBOS_V-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`libphobos_archs')
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Priority: optional
Depends: BASELDEP, libgphobos`'PHOBOS_V`'LS (= ${gdc:Version}), ${misc:Depends}
Replaces: libgphobos68-dbg`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (debug symbols)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: lib64gphobos`'PHOBOS_V`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Replaces: lib64gphobos68`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (runtime library)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: lib64gphobos`'PHOBOS_V-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, lib64gphobos`'PHOBOS_V`'LS (= ${gdc:Version}), ${misc:Depends}
Replaces: lib64gphobos68-dbg`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (debug symbols)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: lib32gphobos`'PHOBOS_V`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Replaces: lib32gphobos68`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (runtime library)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: lib32gphobos`'PHOBOS_V-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, lib32gphobos`'PHOBOS_V`'LS (= ${gdc:Version}), ${misc:Depends}
Replaces: lib32gphobos68-dbg`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (debug symbols)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

ifenabled(`libn32phobos',`
Package: libn32gphobos`'PHOBOS_V`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library (runtime library)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: libn32gphobos`'PHOBOS_V-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, libn32gphobos`'PHOBOS_V`'LS (= ${gdc:Version}), ${misc:Depends}
BUILT_USING`'dnl
Description: Phobos D standard library (debug symbols)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/
')`'dnl libn32phobos

ifenabled(`libx32phobos',`
Package: libx32gphobos`'PHOBOS_V`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Replaces: libx32gphobos68`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (runtime library)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: libx32gphobos`'PHOBOS_V-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, libx32gphobos`'PHOBOS_V`'LS (= ${gdc:Version}), ${misc:Depends}
Replaces: libx32gphobos68-dbg`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (debug symbols)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/
')`'dnl libx32phobos

ifenabled(`armml',`
Package: libhfgphobos`'PHOBOS_V`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Replaces: libhfgphobos68`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (runtime library)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: libhfgphobos`'PHOBOS_V-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, libhfgphobos`'PHOBOS_V`'LS (= ${gdc:Version}), ${misc:Depends}
Replaces: libhfgphobos68-dbg`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (debug symbols)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: libsfgphobos`'PHOBOS_V`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
Replaces: libsfgphobos68`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (runtime library)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/

Package: libsfgphobos`'PHOBOS_V-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, libsfgphobos`'PHOBOS_V`'LS (= ${gdc:Version}), ${misc:Depends}
Replaces: libsfgphobos68-dbg`'LS
BUILT_USING`'dnl
Description: Phobos D standard library (debug symbols)
 This is the Phobos standard library that comes with the D2 compiler.
 .
 For more information check http://www.dlang.org/phobos/
')`'dnl armml
')`'dnl libphobos
')`'dnl d

ifenabled(`brig',`
ifenabled(`brigdev',`
Package: gccbrig`'PV`'TS
Architecture: any
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
Depends: BASEDEP, gcc`'PV`'TS (= ${gcc:Version}), ${dep:libcdev},
  hsail-tools,
  ${shlibs:Depends}, libidevdep(hsail-rt`'PV-dev,,=), ${misc:Depends}
Suggests: ${gccbrig:multilib},
  libdbgdep(hsail-rt`'HSAIL_SO-dbg,)
Provides: brig-compiler`'TS
BUILT_USING`'dnl
Description: GNU BRIG (HSA IL) frontend
 This is the GNU BRIG (HSA IL) frontend.
 The consumed format is a binary representation. The textual HSAIL
 can be compiled to it with a separate assembler.

ifenabled(`multiXXXlib',`
Package: gccbrig`'PV-multilib`'TS
Architecture: ifdef(`TARGET',`any',MULTILIB_ARCHS)
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Section: devel
Priority: optional
Depends: BASEDEP, gccbrig`'PV`'TS (= ${gcc:Version}),
  gcc`'PV-multilib`'TS (= ${gcc:Version}), ${dep:libhsailrtbiarchdev},
  ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GNU BRIG (HSA IL) frontend (multilib support)`'ifdef(`TARGET',` (cross compiler for TARGET architecture)', `')
 This is the GNU BRIG (HSA IL) frontend.
 The consumed format is a binary representation. The textual HSAIL
 can be compiled to it with a separate assembler.
 .
 This is a dependency package, depending on development packages
 for the non-default multilib architecture(s).
')`'dnl multilib

Package: libhsail-rt`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,), libdep(hsail-rt`'HSAIL_SO,),
  ${shlibs:Depends}, ${misc:Depends}
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
BUILT_USING`'dnl
Description: HSAIL runtime library (development files)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.

ifenabled(`lib64hsail',`
Package: lib64hsail-rt`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,64), libdep(hsail-rt`'HSAIL_SO,64),
  ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (64bit development files)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl lib64hsail

ifenabled(`lib32hsail',`
Package: lib32hsail-rt`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,32), libdep(hsail-rt`'HSAIL_SO,32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (32bit development files)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl lib32hsail

ifenabled(`libn32hsail',`
Package: libn32hsail-rt`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,n32), libdep(hsail-rt`'HSAIL_SO,n32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (n32 development files)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libn32hsail

ifenabled(`x32dev',`
ifenabled(`libx32hsail',`
Package: libx32hsail-rt`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,x32), libdep(hsail-rt`'HSAIL_SO,x32), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (x32 development files)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libx32hsail
')`'dnl x32dev

ifenabled(`armml',`
ifenabled(`libhfhsail',`
Package: libhfhsail-rt`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,hf), libdep(hsail-rt`'HSAIL_SO,hf), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (hard float ABI development files)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libhfhsail
')`'dnl armml

ifenabled(`armml',`
ifenabled(`libsfhsail',`
Package: libsfhsail-rt`'PV-dev`'LS
TARGET_PACKAGE`'dnl
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Section: libdevel
Priority: optional
Depends: BASELDEP, libdevdep(gcc`'PV-dev,sf), libdep(hsail-rt`'HSAIL_SO,sf), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (soft float development files)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libsfhsail
')`'dnl armml
')`'dnl hsailrtdev

ifenabled(`libhsail',`
Package: libhsail-rt`'HSAIL_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libhsail-rt'HSAIL_SO`-armel [armel], libhsail-rt'HSAIL_SO`-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
Pre-Depends: ${misc:Pre-Depends}
ifelse(HSAIL_SO,`2',`Breaks: ${multiarch:breaks}
',`')')`'dnl
Priority: optional
Depends: BASELDEP, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.

Package: libhsail-rt`'HSAIL_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`any')
ifdef(`TARGET',`',`Provides: libhsail-rt'HSAIL_SO`-dbg-armel [armel], libhsail-rt'HSAIL_SO`-dbg-armhf [armhf]
')`'dnl
ifdef(`MULTIARCH', `Multi-Arch: same
')`'dnl
Priority: optional
Depends: BASELDEP, libdep(hsail-rt`'HSAIL_SO,,=), libdbgdep(gcc`'GCC_SO-dbg,,>=,${libgcc:Version}), ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (debug symbols)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libhsail

ifenabled(`lib64hsail',`
Package: lib64hsail-rt`'HSAIL_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (64bit)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.

Package: lib64hsail-rt`'HSAIL_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch64_archs')
Priority: optional
Depends: BASELDEP, libdep(hsail-rt`'HSAIL_SO,64,=), libdbgdep(gcc`'GCC_SO-dbg,64,>=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (64 bit debug symbols)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl lib64hsail

ifenabled(`lib32hsail',`
Package: lib32hsail-rt`'HSAIL_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
Conflicts: ${confl:lib32}
BUILT_USING`'dnl
Description: HSAIL runtime library (32bit)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.

Package: lib32hsail-rt`'HSAIL_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarch32_archs')
Priority: optional
Depends: BASELDEP, libdep(hsail-rt`'HSAIL_SO,32,=), libdbgdep(gcc`'GCC_SO-dbg,32,>=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (32 bit debug symbols)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl lib32hsail

ifenabled(`libn32hsail',`
Package: libn32hsail-rt`'HSAIL_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (n32)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.

Package: libn32hsail-rt`'HSAIL_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchn32_archs')
Priority: optional
Depends: BASELDEP, libdep(hsail-rt`'HSAIL_SO,n32,=), libdbgdep(gcc`'GCC_SO-dbg,n32,>=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (n32 debug symbols)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libn32hsail

ifenabled(`libx32hsail',`
Package: libx32hsail-rt`'HSAIL_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (x32)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.

Package: libx32hsail-rt`'HSAIL_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchx32_archs')
Priority: optional
Depends: BASELDEP, libdep(hsail-rt`'HSAIL_SO,x32,=), libdbgdep(gcc`'GCC_SO-dbg,x32,>=,${gcc:EpochVersion}), ${misc:Depends}
BUILT_USING`'dnl
Description: HSAIL runtime library (x32 debug symbols)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libx32hsail

ifenabled(`libhfhsail',`
Package: libhfhsail-rt`'HSAIL_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libhsail-rt'HSAIL_SO`-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: HSAIL runtime library (hard float ABI)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.

Package: libhfhsail-rt`'HSAIL_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchhf_archs')
Priority: optional
Depends: BASELDEP, libdep(hsail-rt`'HSAIL_SO,hf,=), libdbgdep(gcc`'GCC_SO-dbg,hf,>=,${gcc:EpochVersion}), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libhsail-rt'HSAIL_SO`-dbg-armhf [biarchhf_archs]')
BUILT_USING`'dnl
Description: HSAIL runtime library (hard float ABI debug symbols)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libhfhsailrt

ifenabled(`libsfhsail',`
Package: libsfhsail-rt`'HSAIL_SO`'LS
TARGET_PACKAGE`'dnl
Section: ifdef(`TARGET',`devel',`libs')
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, ${dep:libcbiarch}, ${shlibs:Depends}, ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libhsail-rt'HSAIL_SO`-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: HSAIL runtime library (soft float ABI)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.

Package: libsfhsail-rt`'HSAIL_SO-dbg`'LS
TARGET_PACKAGE`'dnl
Section: debug
Architecture: ifdef(`TARGET',`CROSS_ARCH',`biarchsf_archs')
Priority: optional
Depends: BASELDEP, libdep(hsail-rt`'HSAIL_SO,sf,=), libdbgdep(gcc`'GCC_SO-dbg,sf,>=,${gcc:EpochVersion}), ${misc:Depends}
ifdef(`TARGET',`dnl',`Conflicts: libhsail-rt'HSAIL_SO`-dbg-armel [biarchsf_archs]')
BUILT_USING`'dnl
Description: HSAIL runtime library (soft float ABI debug symbols)
 This library implements the agent-side runtime functionality required
 to run HSA finalized programs produced by the BRIG frontend.
 .
 The library contains both the code required to run kernels on the agent
 and also functions implementing more complex HSAIL instructions.
')`'dnl libsfhsailrt
')`'dnl brig

ifdef(`TARGET',`',`dnl
ifenabled(`libs',`
#Package: gcc`'PV-soft-float
#Architecture: arm armel armhf
#Priority: PRI(optional)
#Depends: BASEDEP, depifenabled(`cdev',`gcc`'PV (= ${gcc:Version}),') ${shlibs:Depends}, ${misc:Depends}
#Conflicts: gcc-4.4-soft-float, gcc-4.5-soft-float, gcc-4.6-soft-float
#BUILT_USING`'dnl
#Description: GCC soft-floating-point gcc libraries (ARM)
# These are versions of basic static libraries such as libgcc.a compiled
# with the -msoft-float option, for CPUs without a floating-point unit.
')`'dnl commonlibs
')`'dnl

ifenabled(`fixincl',`
Package: fixincludes
Architecture: any
Priority: PRI(optional)
Depends: BASEDEP, gcc`'PV (= ${gcc:Version}), ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: Fix non-ANSI header files
 FixIncludes was created to fix non-ANSI system header files. Many
 system manufacturers supply proprietary headers that are not ANSI compliant.
 The GNU compilers cannot compile non-ANSI headers. Consequently, the
 FixIncludes shell script was written to fix the header files.
 .
 Not all packages with header files are installed on the system, when the
 package is built, so we make fixincludes available at build time of other
 packages, such that checking tools like lintian can make use of it.
')`'dnl fixincl

ifenabled(`cdev',`
ifdef(`TARGET', `', `
ifenabled(`gfdldoc',`
Package: gcc`'PV-doc
Architecture: all
Section: doc
Priority: PRI(optional)
Depends: gcc`'PV-base (>= ${gcc:SoftVersion}), dpkg (>= 1.15.4) | install-info, ${misc:Depends}
Conflicts: gcc-docs (<< 2.95.2)
Replaces: gcc (<=2.7.2.3-4.3), gcc-docs (<< 2.95.2)
Description: Documentation for the GNU compilers (gcc, gobjc, g++)
 Documentation for the GNU compilers in info `format'.
')`'dnl gfdldoc
')`'dnl native
')`'dnl cdev

ifenabled(`olnvptx',`
Package: gcc`'PV-offload-nvptx
Architecture: amd64
ifdef(`TARGET',`Multi-Arch: foreign
')dnl
Priority: optional
Depends: BASEDEP, gcc`'PV (= ${gcc:Version}), ${dep:libcdev},
  nvptx-tools, libgomp-plugin-nvptx`'GOMP_SO (>= ${gcc:Version}),
  ${shlibs:Depends}, ${misc:Depends}
BUILT_USING`'dnl
Description: GCC offloading compiler to NVPTX
 The package provides offloading support for NVidia PTX.  OpenMP and OpenACC
 programs linked with -fopenmp will by default add PTX code into the binaries,
 which can be offloaded to NVidia PTX capable devices if available.

ifenabled(`libgompnvptx',`
Package: libgomp-plugin-nvptx`'GOMP_SO
Architecture: amd64
Multi-Arch: same
Section: libs
Depends: BASEDEP, libgomp`'GOMP_SO`'LS, ${shlibs:Depends}, ${misc:Depends}
Suggests: libcuda1
BUILT_USING`'dnl
Description: GCC OpenMP v4.5 plugin for offloading to NVPTX
 This package contains libgomp plugin for offloading to NVidia
 PTX.  The plugin needs libcuda.so.1 shared library that has to be
 installed separately.
')`'dnl libgompnvptx
')`'dnl olnvptx

ifdef(`TARGET',`',`dnl
ifenabled(`libnof',`
#Package: gcc`'PV-nof
#Architecture: powerpc
#Priority: PRI(optional)
#Depends: BASEDEP, ${shlibs:Depends}ifenabled(`cdev',`, gcc`'PV (= ${gcc:Version})'), ${misc:Depends}
#Conflicts: gcc-3.2-nof
#BUILT_USING`'dnl
#Description: GCC no-floating-point gcc libraries (powerpc)
# These are versions of basic static libraries such as libgcc.a compiled
# with the -msoft-float option, for CPUs without a floating-point unit.
')`'dnl libnof
')`'dnl

ifenabled(`source',`
Package: gcc`'PV-source
Multi-Arch: foreign
Architecture: all
Priority: PRI(optional)
Depends: make, autoconf2.64, quilt, patchutils, sharutils, gawk, lsb-release,
  ${misc:Depends}
Description: Source of the GNU Compiler Collection
 This package contains the sources and patches which are needed to
 build the GNU Compiler Collection (GCC).
')`'dnl source
dnl
')`'dnl gcc-X.Y
dnl last line in file
