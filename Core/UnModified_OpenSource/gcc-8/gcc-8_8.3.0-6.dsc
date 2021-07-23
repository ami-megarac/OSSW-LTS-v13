-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 1.0
Source: gcc-8
Binary: gcc-8-base, libgcc1, libgcc1-dbg, libgcc2, libgcc2-dbg, libgcc-8-dev, libgcc4, libgcc4-dbg, lib64gcc1, lib64gcc1-dbg, lib64gcc-8-dev, lib32gcc1, lib32gcc1-dbg, lib32gcc-8-dev, libn32gcc1, libn32gcc1-dbg, libn32gcc-8-dev, libx32gcc1, libx32gcc1-dbg, libx32gcc-8-dev, gcc-8, gcc-8-multilib, gcc-8-test-results, gcc-8-plugin-dev, gcc-8-hppa64-linux-gnu, cpp-8, gcc-8-locales, g++-8, g++-8-multilib, libgomp1, libgomp1-dbg, lib32gomp1, lib32gomp1-dbg, lib64gomp1, lib64gomp1-dbg, libn32gomp1, libn32gomp1-dbg, libx32gomp1, libx32gomp1-dbg, libitm1, libitm1-dbg, lib32itm1, lib32itm1-dbg, lib64itm1, lib64itm1-dbg, libx32itm1, libx32itm1-dbg, libatomic1, libatomic1-dbg, lib32atomic1, lib32atomic1-dbg, lib64atomic1, lib64atomic1-dbg, libn32atomic1, libn32atomic1-dbg, libx32atomic1, libx32atomic1-dbg, libasan5, libasan5-dbg, lib32asan5, lib32asan5-dbg, lib64asan5, lib64asan5-dbg, libx32asan5, libx32asan5-dbg, liblsan0, liblsan0-dbg, lib32lsan0, lib32lsan0-dbg, libx32lsan0,
 libx32lsan0-dbg, libtsan0, libtsan0-dbg, libubsan1, libubsan1-dbg, lib32ubsan1, lib32ubsan1-dbg, lib64ubsan1, lib64ubsan1-dbg, libx32ubsan1, libx32ubsan1-dbg, libmpx2, libmpx2-dbg, lib32mpx2, lib32mpx2-dbg, lib64mpx2, lib64mpx2-dbg, libquadmath0, libquadmath0-dbg, lib32quadmath0, lib32quadmath0-dbg, lib64quadmath0, lib64quadmath0-dbg, libx32quadmath0, libx32quadmath0-dbg, libcc1-0, libgccjit0, libgccjit0-dbg, libgccjit-8-doc, libgccjit-8-dev, gobjc++-8, gobjc++-8-multilib, gobjc-8, gobjc-8-multilib, libobjc-8-dev, lib64objc-8-dev, lib32objc-8-dev, libn32objc-8-dev, libx32objc-8-dev, libobjc4, libobjc4-dbg, lib64objc4, lib64objc4-dbg, lib32objc4, lib32objc4-dbg, libn32objc4, libn32objc4-dbg, libx32objc4, libx32objc4-dbg, gfortran-8, gfortran-8-multilib, libgfortran-8-dev, lib64gfortran-8-dev, lib32gfortran-8-dev, libn32gfortran-8-dev, libx32gfortran-8-dev, libgfortran5, libgfortran5-dbg, lib64gfortran5, lib64gfortran5-dbg, lib32gfortran5, lib32gfortran5-dbg,
 libn32gfortran5, libn32gfortran5-dbg, libx32gfortran5, libx32gfortran5-dbg, gccgo-8, gccgo-8-multilib, libgo13, libgo13-dbg, lib64go13, lib64go13-dbg, lib32go13, lib32go13-dbg, libn32go13, libn32go13-dbg, libx32go13, libx32go13-dbg, libstdc++6, lib32stdc++6, lib64stdc++6, libn32stdc++6, libx32stdc++6, libstdc++-8-dev, libstdc++-8-pic, libstdc++6-8-dbg, lib32stdc++-8-dev, lib32stdc++6-8-dbg, lib64stdc++-8-dev, lib64stdc++6-8-dbg, libn32stdc++-8-dev, libn32stdc++6-8-dbg, libx32stdc++-8-dev, libx32stdc++6-8-dbg, libstdc++-8-doc, gnat-8, gnat-8-sjlj, libgnat-8, libgnat-8-dbg, libgnatvsn8-dev, libgnatvsn8, libgnatvsn8-dbg, gdc-8, gdc-8-multilib, libgphobos-8-dev, lib64gphobos-8-dev, lib32gphobos-8-dev, libx32gphobos-8-dev, libgphobos76, libgphobos76-dbg, lib64gphobos76, lib64gphobos76-dbg, lib32gphobos76, lib32gphobos76-dbg, libx32gphobos76, libx32gphobos76-dbg, gccbrig-8, libhsail-rt-8-dev, libhsail-rt0, libhsail-rt0-dbg, fixincludes, gcc-8-offload-nvptx,
 libgomp-plugin-nvptx1,
 gcc-8-source
Architecture: any all
Version: 8.3.0-6
Maintainer: Debian GCC Maintainers <debian-gcc@lists.debian.org>
Uploaders: Matthias Klose <doko@debian.org>
Homepage: http://gcc.gnu.org/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/toolchain-team/gcc/tree/gcc-8-debian
Vcs-Git: https://salsa.debian.org/toolchain-team/gcc.git -b gcc-8-debian
Testsuite: autopkgtest
Testsuite-Triggers: apt, libc-dev, libc6-dev, python3-minimal
Build-Depends: debhelper (>= 9.20141010), dpkg-dev (>= 1.17.14), g++-multilib [amd64 i386 kfreebsd-amd64 mips mips64 mips64el mips64r6 mips64r6el mipsel mipsn32 mipsn32el mipsn32r6 mipsn32r6el mipsr6 mipsr6el powerpc ppc64 s390 s390x sparc sparc64 x32] <!cross>, libc6.1-dev (>= 2.13-5) [alpha ia64] | libc0.3-dev (>= 2.13-5) [hurd-i386] | libc0.1-dev (>= 2.13-5) [kfreebsd-i386 kfreebsd-amd64] | libc6-dev (>= 2.13-5), libc6-dev (>= 2.13-31) [armel armhf], libc6-dev-amd64 [i386 x32], libc6-dev-sparc64 [sparc], libc6-dev-sparc [sparc64], libc6-dev-s390 [s390x], libc6-dev-s390x [s390], libc6-dev-i386 [amd64 x32], libc6-dev-powerpc [ppc64], libc6-dev-ppc64 [powerpc], libc0.1-dev-i386 [kfreebsd-amd64], lib32gcc1 [amd64 ppc64 kfreebsd-amd64 mipsn32 mipsn32el mips64 mips64el mipsn32r6 mipsn32r6el mips64r6 mips64r6el s390x sparc64 x32], libn32gcc1 [mips mipsel mips64 mips64el mipsr6 mipsr6el mips64r6 mips64r6el], lib64gcc1 [i386 mips mipsel mipsn32 mipsn32el mipsr6 mipsr6el mipsn32r6 mipsn32r6el powerpc sparc s390 x32], libc6-dev-mips64 [mips mipsel mipsn32 mipsn32el mipsr6 mipsr6el mipsn32r6 mipsn32r6el], libc6-dev-mipsn32 [mips mipsel mips64 mips64el mipsr6 mipsr6el mips64r6 mips64r6el], libc6-dev-mips32 [mipsn32 mipsn32el mips64 mips64el mipsn32r6 mipsn32r6el mips64r6 mips64r6el], libc6-dev-x32 [amd64 i386], libx32gcc1 [amd64 i386], libc6.1-dbg [alpha ia64] | libc0.3-dbg [hurd-i386] | libc0.1-dbg [kfreebsd-i386 kfreebsd-amd64] | libc6-dbg, kfreebsd-kernel-headers (>= 0.84) [kfreebsd-any], linux-libc-dev [m68k], m4, libtool, autoconf2.64, dwz, libunwind8-dev [ia64], libatomic-ops-dev [ia64], gawk, lzma, xz-utils, patchutils, zlib1g-dev, systemtap-sdt-dev [linux-any kfreebsd-any hurd-any], binutils:native (>= 2.30) | binutils-multiarch:native (>= 2.30), binutils-hppa64-linux-gnu:native (>= 2.30) [hppa amd64 i386 x32], gperf (>= 3.0.1), bison (>= 1:2.3), flex, gettext, gdb:native [!riscv64], nvptx-tools [amd64], texinfo (>= 4.3), locales-all, sharutils, procps, gnat-8:native [!m32r !riscv64 !sh3 !sh3eb !sh4eb !m68k !powerpcspe], g++-8:native, netbase, libisl-dev (>= 0.20), libmpc-dev (>= 1.0), libmpfr-dev (>= 3.0.0-9~), libgmp-dev (>= 2:5.0.1~), lib32z1-dev [amd64 kfreebsd-amd64], lib64z1-dev [i386], dejagnu [!m68k !hurd-amd64 !hurd-i386 !hurd-alpha !kfreebsd-amd64 !kfreebsd-i386 !kfreebsd-alpha] <!nocheck>, coreutils (>= 2.26) | realpath (>= 1.9.12), chrpath, lsb-release, quilt, pkg-config, libgc-dev, g++-8-alpha-linux-gnu [alpha] <cross>, gobjc-8-alpha-linux-gnu [alpha] <cross>, gfortran-8-alpha-linux-gnu [alpha] <cross>, gdc-8-alpha-linux-gnu [alpha] <cross>, gccgo-8-alpha-linux-gnu [alpha] <cross>, gnat-8-alpha-linux-gnu [alpha] <cross>, g++-8-x86-64-linux-gnu [amd64] <cross>, gobjc-8-x86-64-linux-gnu [amd64] <cross>, gfortran-8-x86-64-linux-gnu [amd64] <cross>, gdc-8-x86-64-linux-gnu [amd64] <cross>, gccgo-8-x86-64-linux-gnu [amd64] <cross>, gnat-8-x86-64-linux-gnu [amd64] <cross>, g++-8-arm-linux-gnueabi [armel] <cross>, gobjc-8-arm-linux-gnueabi [armel] <cross>, gfortran-8-arm-linux-gnueabi [armel] <cross>, gdc-8-arm-linux-gnueabi [armel] <cross>, gccgo-8-arm-linux-gnueabi [armel] <cross>, gnat-8-arm-linux-gnueabi [armel] <cross>, g++-8-arm-linux-gnueabihf [armhf] <cross>, gobjc-8-arm-linux-gnueabihf [armhf] <cross>, gfortran-8-arm-linux-gnueabihf [armhf] <cross>, gdc-8-arm-linux-gnueabihf [armhf] <cross>, gccgo-8-arm-linux-gnueabihf [armhf] <cross>, gnat-8-arm-linux-gnueabihf [armhf] <cross>, g++-8-aarch64-linux-gnu [arm64] <cross>, gobjc-8-aarch64-linux-gnu [arm64] <cross>, gfortran-8-aarch64-linux-gnu [arm64] <cross>, gdc-8-aarch64-linux-gnu [arm64] <cross>, gccgo-8-aarch64-linux-gnu [arm64] <cross>, gnat-8-aarch64-linux-gnu [arm64] <cross>, g++-8-i686-linux-gnu [i386] <cross>, gobjc-8-i686-linux-gnu [i386] <cross>, gfortran-8-i686-linux-gnu [i386] <cross>, gdc-8-i686-linux-gnu [i386] <cross>, gccgo-8-i686-linux-gnu [i386] <cross>, gnat-8-i686-linux-gnu [i386] <cross>, g++-8-mips-linux-gnu [mips] <cross>, gobjc-8-mips-linux-gnu [mips] <cross>, gfortran-8-mips-linux-gnu [mips] <cross>, gdc-8-mips-linux-gnu [mips] <cross>, gccgo-8-mips-linux-gnu [mips] <cross>, gnat-8-mips-linux-gnu [mips] <cross>, g++-8-mipsel-linux-gnu [mipsel] <cross>, gobjc-8-mipsel-linux-gnu [mipsel] <cross>, gfortran-8-mipsel-linux-gnu [mipsel] <cross>, gdc-8-mipsel-linux-gnu [mipsel] <cross>, gccgo-8-mipsel-linux-gnu [mipsel] <cross>, gnat-8-mipsel-linux-gnu [mipsel] <cross>, g++-8-mips64-linux-gnuabi64 [mips64] <cross>, gobjc-8-mips64-linux-gnuabi64 [mips64] <cross>, gfortran-8-mips64-linux-gnuabi64 [mips64] <cross>, gdc-8-mips64-linux-gnuabi64 [mips64] <cross>, gccgo-8-mips64-linux-gnuabi64 [mips64] <cross>, gnat-8-mips64-linux-gnuabi64 [mips64] <cross>, g++-8-mips64el-linux-gnuabi64 [mips64el] <cross>, gobjc-8-mips64el-linux-gnuabi64 [mips64el] <cross>, gfortran-8-mips64el-linux-gnuabi64 [mips64el] <cross>, gdc-8-mips64el-linux-gnuabi64 [mips64el] <cross>, gccgo-8-mips64el-linux-gnuabi64 [mips64el] <cross>, gnat-8-mips64el-linux-gnuabi64 [mips64el] <cross>, g++-8-mips64-linux-gnuabin32 [mipsn32] <cross>, gobjc-8-mips64-linux-gnuabin32 [mipsn32] <cross>, gfortran-8-mips64-linux-gnuabin32 [mipsn32] <cross>, gdc-8-mips64-linux-gnuabin32 [mipsn32] <cross>, gccgo-8-mips64-linux-gnuabin32 [mipsn32] <cross>, gnat-8-mips64-linux-gnuabin32 [mipsn32] <cross>, g++-8-powerpc-linux-gnu [powerpc] <cross>, gobjc-8-powerpc-linux-gnu [powerpc] <cross>, gfortran-8-powerpc-linux-gnu [powerpc] <cross>, gdc-8-powerpc-linux-gnu [powerpc] <cross>, gccgo-8-powerpc-linux-gnu [powerpc] <cross>, gnat-8-powerpc-linux-gnu [powerpc] <cross>, g++-8-powerpc-linux-gnuspe [powerpcspe] <cross>, gobjc-8-powerpc-linux-gnuspe [powerpcspe] <cross>, gfortran-8-powerpc-linux-gnuspe [powerpcspe] <cross>, gdc-8-powerpc-linux-gnuspe [powerpcspe] <cross>, gccgo-8-powerpc-linux-gnuspe [powerpcspe] <cross>, gnat-8-powerpc-linux-gnuspe [powerpcspe] <cross>, g++-8-powerpc64-linux-gnu [ppc64] <cross>, gobjc-8-powerpc64-linux-gnu [ppc64] <cross>, gfortran-8-powerpc64-linux-gnu [ppc64] <cross>, gdc-8-powerpc64-linux-gnu [ppc64] <cross>, gccgo-8-powerpc64-linux-gnu [ppc64] <cross>, gnat-8-powerpc64-linux-gnu [ppc64] <cross>, g++-8-powerpc64le-linux-gnu [ppc64el] <cross>, gobjc-8-powerpc64le-linux-gnu [ppc64el] <cross>, gfortran-8-powerpc64le-linux-gnu [ppc64el] <cross>, gdc-8-powerpc64le-linux-gnu [ppc64el] <cross>, gccgo-8-powerpc64le-linux-gnu [ppc64el] <cross>, gnat-8-powerpc64le-linux-gnu [ppc64el] <cross>, g++-8-m68k-linux-gnu [m68k] <cross>, gobjc-8-m68k-linux-gnu [m68k] <cross>, gfortran-8-m68k-linux-gnu [m68k] <cross>, gdc-8-m68k-linux-gnu [m68k] <cross>, g++-8-sh4-linux-gnu [sh4] <cross>, gobjc-8-sh4-linux-gnu [sh4] <cross>, gfortran-8-sh4-linux-gnu [sh4] <cross>, gnat-8-sh4-linux-gnu [sh4] <cross>, g++-8-sparc64-linux-gnu [sparc64] <cross>, gobjc-8-sparc64-linux-gnu [sparc64] <cross>, gfortran-8-sparc64-linux-gnu [sparc64] <cross>, gdc-8-sparc64-linux-gnu [sparc64] <cross>, gccgo-8-sparc64-linux-gnu [sparc64] <cross>, gnat-8-sparc64-linux-gnu [sparc64] <cross>, g++-8-s390x-linux-gnu [s390x] <cross>, gobjc-8-s390x-linux-gnu [s390x] <cross>, gfortran-8-s390x-linux-gnu [s390x] <cross>, gdc-8-s390x-linux-gnu [s390x] <cross>, gccgo-8-s390x-linux-gnu [s390x] <cross>, gnat-8-s390x-linux-gnu [s390x] <cross>, g++-8-x86-64-linux-gnux32 [x32] <cross>, gobjc-8-x86-64-linux-gnux32 [x32] <cross>, gfortran-8-x86-64-linux-gnux32 [x32] <cross>, gdc-8-x86-64-linux-gnux32 [x32] <cross>, gccgo-8-x86-64-linux-gnux32 [x32] <cross>, gnat-8-x86-64-linux-gnux32 [x32] <cross>, g++-8-mips64el-linux-gnuabin32 [mipsn32el] <cross>, gobjc-8-mips64el-linux-gnuabin32 [mipsn32el] <cross>, gfortran-8-mips64el-linux-gnuabin32 [mipsn32el] <cross>, gdc-8-mips64el-linux-gnuabin32 [mipsn32el] <cross>, gccgo-8-mips64el-linux-gnuabin32 [mipsn32el] <cross>, gnat-8-mips64el-linux-gnuabin32 [mipsn32el] <cross>, g++-8-mipsisa32r6-linux-gnu [mipsr6] <cross>, gobjc-8-mipsisa32r6-linux-gnu [mipsr6] <cross>, gfortran-8-mipsisa32r6-linux-gnu [mipsr6] <cross>, gdc-8-mipsisa32r6-linux-gnu [mipsr6] <cross>, gccgo-8-mipsisa32r6-linux-gnu [mipsr6] <cross>, gnat-8-mipsisa32r6-linux-gnu [mipsr6] <cross>, g++-8-mipsisa32r6el-linux-gnu [mipsr6el] <cross>, gobjc-8-mipsisa32r6el-linux-gnu [mipsr6el] <cross>, gfortran-8-mipsisa32r6el-linux-gnu [mipsr6el] <cross>, gdc-8-mipsisa32r6el-linux-gnu [mipsr6el] <cross>, gccgo-8-mipsisa32r6el-linux-gnu [mipsr6el] <cross>, gnat-8-mipsisa32r6el-linux-gnu [mipsr6el] <cross>, g++-8-mipsisa64r6-linux-gnuabi64 [mips64r6] <cross>, gobjc-8-mipsisa64r6-linux-gnuabi64 [mips64r6] <cross>, gfortran-8-mipsisa64r6-linux-gnuabi64 [mips64r6] <cross>, gdc-8-mipsisa64r6-linux-gnuabi64 [mips64r6] <cross>, gccgo-8-mipsisa64r6-linux-gnuabi64 [mips64r6] <cross>, gnat-8-mipsisa64r6-linux-gnuabi64 [mips64r6] <cross>, g++-8-mipsisa64r6el-linux-gnuabi64 [mips64r6el] <cross>, gobjc-8-mipsisa64r6el-linux-gnuabi64 [mips64r6el] <cross>, gfortran-8-mipsisa64r6el-linux-gnuabi64 [mips64r6el] <cross>, gdc-8-mipsisa64r6el-linux-gnuabi64 [mips64r6el] <cross>, gccgo-8-mipsisa64r6el-linux-gnuabi64 [mips64r6el] <cross>, gnat-8-mipsisa64r6el-linux-gnuabi64 [mips64r6el] <cross>, g++-8-mipsisa64r6-linux-gnuabin32 [mipsn32r6] <cross>, gobjc-8-mipsisa64r6-linux-gnuabin32 [mipsn32r6] <cross>, gfortran-8-mipsisa64r6-linux-gnuabin32 [mipsn32r6] <cross>, gdc-8-mipsisa64r6-linux-gnuabin32 [mipsn32r6] <cross>, gccgo-8-mipsisa64r6-linux-gnuabin32 [mipsn32r6] <cross>, gnat-8-mipsisa64r6-linux-gnuabin32 [mipsn32r6] <cross>, g++-8-mipsisa64r6el-linux-gnuabin32 [mipsn32r6el] <cross>, gobjc-8-mipsisa64r6el-linux-gnuabin32 [mipsn32r6el] <cross>, gfortran-8-mipsisa64r6el-linux-gnuabin32 [mipsn32r6el] <cross>, gdc-8-mipsisa64r6el-linux-gnuabin32 [mipsn32r6el] <cross>, gccgo-8-mipsisa64r6el-linux-gnuabin32 [mipsn32r6el] <cross>, gnat-8-mipsisa64r6el-linux-gnuabin32 [mipsn32r6el] <cross>
Build-Depends-Indep: doxygen (>= 1.7.2), graphviz (>= 2.2), ghostscript, texlive-latex-base, xsltproc, libxml2-utils, docbook-xsl-ns
Package-List:
 cpp-8 deb interpreters optional arch=any
 fixincludes deb devel optional arch=any
 g++-8 deb devel optional arch=any
 g++-8-multilib deb devel optional arch=amd64,i386,kfreebsd-amd64,mips,mips64,mips64el,mips64r6,mips64r6el,mipsel,mipsn32,mipsn32el,mipsn32r6,mipsn32r6el,mipsr6,mipsr6el,powerpc,ppc64,s390,s390x,sparc,sparc64,x32
 gcc-8 deb devel optional arch=any
 gcc-8-base deb libs required arch=any
 gcc-8-hppa64-linux-gnu deb devel optional arch=hppa,amd64,i386,x32
 gcc-8-locales deb devel optional arch=all
 gcc-8-multilib deb devel optional arch=amd64,i386,kfreebsd-amd64,mips,mips64,mips64el,mips64r6,mips64r6el,mipsel,mipsn32,mipsn32el,mipsn32r6,mipsn32r6el,mipsr6,mipsr6el,powerpc,ppc64,s390,s390x,sparc,sparc64,x32
 gcc-8-offload-nvptx deb devel optional arch=amd64
 gcc-8-plugin-dev deb devel optional arch=any
 gcc-8-source deb devel optional arch=all
 gcc-8-test-results deb devel optional arch=any
 gccbrig-8 deb devel optional arch=any
 gccgo-8 deb devel optional arch=any
 gccgo-8-multilib deb devel optional arch=amd64,i386,kfreebsd-amd64,mips,mips64,mips64el,mips64r6,mips64r6el,mipsel,mipsn32,mipsn32el,mipsn32r6,mipsn32r6el,mipsr6,mipsr6el,powerpc,ppc64,s390,s390x,sparc,sparc64,x32
 gdc-8 deb devel optional arch=any
 gdc-8-multilib deb devel optional arch=any
 gfortran-8 deb devel optional arch=any
 gfortran-8-multilib deb devel optional arch=amd64,i386,kfreebsd-amd64,mips,mips64,mips64el,mips64r6,mips64r6el,mipsel,mipsn32,mipsn32el,mipsn32r6,mipsn32r6el,mipsr6,mipsr6el,powerpc,ppc64,s390,s390x,sparc,sparc64,x32
 gnat-8 deb devel optional arch=any
 gnat-8-sjlj deb devel optional arch=any
 gobjc++-8 deb devel optional arch=any
 gobjc++-8-multilib deb devel optional arch=amd64,i386,kfreebsd-amd64,mips,mips64,mips64el,mips64r6,mips64r6el,mipsel,mipsn32,mipsn32el,mipsn32r6,mipsn32r6el,mipsr6,mipsr6el,powerpc,ppc64,s390,s390x,sparc,sparc64,x32
 gobjc-8 deb devel optional arch=any
 gobjc-8-multilib deb devel optional arch=amd64,i386,kfreebsd-amd64,mips,mips64,mips64el,mips64r6,mips64r6el,mipsel,mipsn32,mipsn32el,mipsn32r6,mipsn32r6el,mipsr6,mipsr6el,powerpc,ppc64,s390,s390x,sparc,sparc64,x32
 lib32asan5 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32asan5-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32atomic1 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32atomic1-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gcc-8-dev deb libdevel optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gcc1 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gcc1-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gfortran-8-dev deb libdevel optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gfortran5 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gfortran5-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32go13 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32go13-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gomp1 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gomp1-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gphobos-8-dev deb libdevel optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gphobos76 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32gphobos76-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32itm1 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32itm1-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32lsan0 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32lsan0-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32mpx2 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32mpx2-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32objc-8-dev deb libdevel optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32objc4 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32objc4-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32quadmath0 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32quadmath0-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32stdc++-8-dev deb libdevel optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32stdc++6 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32stdc++6-8-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32ubsan1 deb libs optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib32ubsan1-dbg deb debug optional arch=amd64,ppc64,kfreebsd-amd64,s390x,sparc64,x32,mipsn32,mipsn32el,mips64,mips64el,mipsn32r6,mipsn32r6el,mips64r6,mips64r6el
 lib64asan5 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64asan5-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64atomic1 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64atomic1-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gcc-8-dev deb libdevel optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gcc1 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gcc1-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gfortran-8-dev deb libdevel optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gfortran5 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gfortran5-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64go13 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64go13-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gomp1 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gomp1-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gphobos-8-dev deb libdevel optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gphobos76 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64gphobos76-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64itm1 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64itm1-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64mpx2 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64mpx2-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64objc-8-dev deb libdevel optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64objc4 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64objc4-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64quadmath0 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64quadmath0-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64stdc++-8-dev deb libdevel optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64stdc++6 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64stdc++6-8-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64ubsan1 deb libs optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 lib64ubsan1-dbg deb debug optional arch=i386,powerpc,sparc,s390,mips,mipsel,mipsn32,mipsn32el,mipsr6,mipsr6el,mipsn32r6,mipsn32r6el,x32
 libasan5 deb libs optional arch=any
 libasan5-dbg deb debug optional arch=any
 libatomic1 deb libs optional arch=any
 libatomic1-dbg deb debug optional arch=any
 libcc1-0 deb libs optional arch=any
 libgcc-8-dev deb libdevel optional arch=any
 libgcc1 deb libs optional arch=any
 libgcc1-dbg deb debug optional arch=any
 libgcc2 deb libs optional arch=m68k
 libgcc2-dbg deb debug optional arch=m68k
 libgcc4 deb libs optional arch=hppa
 libgcc4-dbg deb debug optional arch=hppa
 libgccjit-8-dev deb libdevel optional arch=any
 libgccjit-8-doc deb doc optional arch=all
 libgccjit0 deb libs optional arch=any
 libgccjit0-dbg deb debug optional arch=any
 libgfortran-8-dev deb libdevel optional arch=any
 libgfortran5 deb libs optional arch=any
 libgfortran5-dbg deb debug optional arch=any
 libgnat-8 deb libs optional arch=any
 libgnat-8-dbg deb debug optional arch=any
 libgnatvsn8 deb libs optional arch=any
 libgnatvsn8-dbg deb debug optional arch=any
 libgnatvsn8-dev deb libdevel optional arch=any
 libgo13 deb libs optional arch=any
 libgo13-dbg deb debug optional arch=any
 libgomp-plugin-nvptx1 deb libs optional arch=amd64
 libgomp1 deb libs optional arch=any
 libgomp1-dbg deb debug optional arch=any
 libgphobos-8-dev deb libdevel optional arch=amd64,arm64,armel,armhf,i386,x32,kfreebsd-amd64,kfreebsd-i386
 libgphobos76 deb libs optional arch=amd64,arm64,armel,armhf,i386,x32,kfreebsd-amd64,kfreebsd-i386
 libgphobos76-dbg deb debug optional arch=amd64,arm64,armel,armhf,i386,x32,kfreebsd-amd64,kfreebsd-i386
 libhsail-rt-8-dev deb libdevel optional arch=any
 libhsail-rt0 deb libs optional arch=any
 libhsail-rt0-dbg deb debug optional arch=any
 libitm1 deb libs optional arch=any
 libitm1-dbg deb debug optional arch=any
 liblsan0 deb libs optional arch=any
 liblsan0-dbg deb debug optional arch=any
 libmpx2 deb libs optional arch=any
 libmpx2-dbg deb debug optional arch=any
 libn32atomic1 deb libs optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32atomic1-dbg deb debug optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32gcc-8-dev deb libdevel optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32gcc1 deb libs optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32gcc1-dbg deb debug optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32gfortran-8-dev deb libdevel optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32gfortran5 deb libs optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32gfortran5-dbg deb debug optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32go13 deb libs optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32go13-dbg deb debug optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32gomp1 deb libs optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32gomp1-dbg deb debug optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32objc-8-dev deb libdevel optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32objc4 deb libs optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32objc4-dbg deb debug optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32stdc++-8-dev deb libdevel optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32stdc++6 deb libs optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libn32stdc++6-8-dbg deb debug optional arch=mips,mipsel,mips64,mips64el,mipsr6,mipsr6el,mips64r6,mips64r6el
 libobjc-8-dev deb libdevel optional arch=any
 libobjc4 deb libs optional arch=any
 libobjc4-dbg deb debug optional arch=any
 libquadmath0 deb libs optional arch=any
 libquadmath0-dbg deb debug optional arch=any
 libstdc++-8-dev deb libdevel optional arch=any
 libstdc++-8-doc deb doc optional arch=all
 libstdc++-8-pic deb libdevel optional arch=any
 libstdc++6 deb libs optional arch=any
 libstdc++6-8-dbg deb debug optional arch=any
 libtsan0 deb libs optional arch=any
 libtsan0-dbg deb debug optional arch=any
 libubsan1 deb libs optional arch=any
 libubsan1-dbg deb debug optional arch=any
 libx32asan5 deb libs optional arch=amd64,i386
 libx32asan5-dbg deb debug optional arch=amd64,i386
 libx32atomic1 deb libs optional arch=amd64,i386
 libx32atomic1-dbg deb debug optional arch=amd64,i386
 libx32gcc-8-dev deb libdevel optional arch=amd64,i386
 libx32gcc1 deb libs optional arch=amd64,i386
 libx32gcc1-dbg deb debug optional arch=amd64,i386
 libx32gfortran-8-dev deb libdevel optional arch=amd64,i386
 libx32gfortran5 deb libs optional arch=amd64,i386
 libx32gfortran5-dbg deb debug optional arch=amd64,i386
 libx32go13 deb libs optional arch=amd64,i386
 libx32go13-dbg deb debug optional arch=amd64,i386
 libx32gomp1 deb libs optional arch=amd64,i386
 libx32gomp1-dbg deb debug optional arch=amd64,i386
 libx32gphobos-8-dev deb libdevel optional arch=amd64,i386
 libx32gphobos76 deb libs optional arch=amd64,i386
 libx32gphobos76-dbg deb debug optional arch=amd64,i386
 libx32itm1 deb libs optional arch=amd64,i386
 libx32itm1-dbg deb debug optional arch=amd64,i386
 libx32lsan0 deb libs optional arch=amd64,i386
 libx32lsan0-dbg deb debug optional arch=amd64,i386
 libx32objc-8-dev deb libdevel optional arch=amd64,i386
 libx32objc4 deb libs optional arch=amd64,i386
 libx32objc4-dbg deb debug optional arch=amd64,i386
 libx32quadmath0 deb libs optional arch=amd64,i386
 libx32quadmath0-dbg deb debug optional arch=amd64,i386
 libx32stdc++-8-dev deb libdevel optional arch=amd64,i386
 libx32stdc++6 deb libs optional arch=amd64,i386
 libx32stdc++6-8-dbg deb debug optional arch=amd64,i386
 libx32ubsan1 deb libs optional arch=amd64,i386
 libx32ubsan1-dbg deb debug optional arch=amd64,i386
Checksums-Sha1:
 6630480075f371093003ecd57b020db45bb343e6 87764363 gcc-8_8.3.0.orig.tar.gz
 daf823f22e1f1df5b362decd6a11cf6ca8dff500 704334 gcc-8_8.3.0-6.diff.gz
Checksums-Sha256:
 ee3fd608f66e5737f20cf71b176cfbf58f7c1d190ad6def33d57610cdae8eac2 87764363 gcc-8_8.3.0.orig.tar.gz
 211e5e1022e115abbcb9eeb39cf4bf84958c4e8469c0cbe430569947a04c5415 704334 gcc-8_8.3.0-6.diff.gz
Files:
 fff19b9ab5803a8c7afc6baf022c2b6a 87764363 gcc-8_8.3.0.orig.tar.gz
 4bffc7f12a9198d10b5d38847f6e1ce3 704334 gcc-8_8.3.0-6.diff.gz

-----BEGIN PGP SIGNATURE-----

iQJEBAEBCAAuFiEE1WVxuIqLuvFAv2PWvX6qYHePpvUFAlyov5cQHGRva29AZGVi
aWFuLm9yZwAKCRC9fqpgd4+m9Sk1EADBnKzJnDr+wtQms6Agqi8MWElOb1DbWV9l
QUiaCPgZ4KvhLXRUEJmWqCrUYVj/ZIxFtyAGPrQeym0JGguq02iqQnulEIxnAzDV
+28Yl5o9yLrixoCrJzjAfbWAl/8gjkLbwYvSpgdasMakUd+t9Md6ZuZ2YtXV1rS4
xZCPOVu7daeB4V4T01cg5r/KRIh/Bjhnhm7NlMPkH+8m/gY/GwBavR6ZB439hirX
GZaao40gV0nJLghH7cVhhyC+LC/pzVV8vI6hBJNt7XYmAG64lHZ2K9lKbDP96Lc9
pp1B6y8/Blh329Nsqc3fcDLoVZZ/r42VZR5ZYfoIH3D2ATviTUw1Dj20q/Q/Xuby
Ko+Xypivk799gJXiq0m2NCdGrkkKbzLsgk+sWIetFtgbzbYPSz/yhbupMyBS3BZl
kfQLZbZEdRtS1pjmYuy8VasWGu3eCl3GMPr+fOUA9MRwPGM+PENctn9lJ8Ju/q6x
coZyfC1RmBaXdyimOJGYjgqQSDLbuSeBEZdJJFYWHNVCFqchQPiL6VwtK73HpiTl
HgD1WVJgUb3MnlWk/p0V0Fndv03MR2Facq6mAANH5kpD86w9m0Vi1jJd0S9NaFUV
eZE5sNDWD5kdrHlZjvRale5Ia/9xOZE4ymI9mdJZ9LwMBiahHEswZb0Xjded1d1F
OPggM8UXvA==
=1psl
-----END PGP SIGNATURE-----
