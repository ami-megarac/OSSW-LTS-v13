arch_binaries  := $(arch_binaries) snapshot

ifneq (,$(findstring gcc-snapshot, $(PKGSOURCE)))
  p_snap = gcc-snapshot
else ifneq (,$(findstring gcc-linaro, $(PKGSOURCE)))
  p_snap = gcc-linaro
else
  $(error unknown build for single gcc package)
endif

ifeq ($(DEB_CROSS),yes)
  p_snap := $(p_snap)$(cross_bin_arch)
endif
d_snap	= debian/$(p_snap)

dirs_snap = \
	$(docdir)/$(p_snap) \
	usr/lib

ifeq ($(with_hppa64),yes)
  snapshot_depends = binutils-hppa64
endif

# ----------------------------------------------------------------------
$(binary_stamp)-snapshot: $(install_snap_stamp)
	dh_testdir
	dh_testroot
	mv $(install_snap_stamp) $(install_snap_stamp)-tmp

	rm -rf $(d_snap)
	dh_installdirs -p$(p_snap) $(dirs_snap)

	mv $(d)/$(PF) $(d_snap)/usr/lib/

	find $(d_snap) -name '*.gch' -type d | xargs -r rm -rf
	find $(d_snap) -name '*.la' -o -name '*.lai' | xargs -r rm -f

	: # FIXME: libbacktrace is not installed by default
	for d in . 32 n32 64 sf hf; do \
	  if [ -f $(buildlibdir)/$$d/libbacktrace/.libs/libbacktrace.a ]; then \
	    install -m644 $(buildlibdir)/$$d/libbacktrace/.libs/libbacktrace.a \
	      $(d_snap)/$(gcc_lib_dir)/$$d; \
	  fi; \
	done
	if [ -f $(buildlibdir)/libbacktrace/backtrace-supported.h ]; then \
	  install -m644 $(buildlibdir)/libbacktrace/backtrace-supported.h \
	    $(d_snap)/$(gcc_lib_dir)/include/; \
	  install -m644 $(srcdir)/libbacktrace/backtrace.h \
	    $(d_snap)/$(gcc_lib_dir)/include/; \
	fi

	rm -rf $(d_snap)/$(PF)/lib/nof

ifeq ($(with_ada),yes FIXME: apply our ada patches)
	dh_link -p$(p_snap) \
	   $(gcc_lib_dir)/rts-sjlj/adalib/libgnat.a \
	   $(gcc_lib_dir)/rts-sjlj/adalib/libgnat-$(GNAT_VERSION).a
	dh_link -p$(p_snap) \
	   $(gcc_lib_dir)/rts-sjlj/adalib/libgnarl.a \
	   $(gcc_lib_dir)/rts-sjlj/adalib/libgnarl-$(GNAT_VERSION).a

	set -e; \
	for lib in lib{gnat,gnarl}; do \
	  vlib=$$lib-$(GNAT_SONAME); \
	  mv $(d_snap)/$(gcc_lib_dir)/adalib/$$vlib.so.1 $(d_snap)/$(PF)/$(libdir)/. ; \
	  rm -f $(d_snap)/$(gcc_lib_dir)/adalib/$$lib.so.1; \
	  dh_link -p$(p_snap) \
	    /$(PF)/$(libdir)/$$vlib.so.1 /$(PF)/$(libdir)/$$vlib.so \
	    /$(PF)/$(libdir)/$$vlib.so.1 /$(PF)/$(libdir)/$$lib.so \
	    /$(PF)/$(libdir)/$$vlib.so.1 /$(gcc_lib_dir)/rts-native/adalib/$$lib.so; \
	done
endif
ifeq ($(with_ada),yes)
	ln -sf gcc $(d_snap)/$(PF)/bin/gnatgcc
endif

ifeq ($(with_hppa64),yes)
	: # provide as and ld links
	dh_link -p $(p_snap) \
		/usr/bin/hppa64-linux-gnu-as \
		/$(PF)/libexec/gcc/hppa64-linux-gnu/$(GCC_VERSION)/as \
		/usr/bin/hppa64-linux-gnu-ld \
		/$(PF)/libexec/gcc/hppa64-linux-gnu/$(GCC_VERSION)/ld
endif

ifeq ($(with_check),yes)
	dh_installdocs -p$(p_snap) test-summary
# more than one libgo.sum, avoid it 
	mkdir -p $(d_snap)/$(docdir)/$(p_snap)/test-summaries
	cp -p $$(find $(builddir)/gcc/testsuite -maxdepth 2 \( -name '*.sum' -o -name '*.log' \)) \
	      $$(find $(buildlibdir)/*/testsuite -maxdepth 1 \( -name '*.sum'  -o -name '*.log' \) ! -name 'libgo.*') \
		$(d_snap)/$(docdir)/$(p_snap)/test-summaries/
  ifeq ($(with_go),yes)
	cp -p $(buildlibdir)/libgo/libgo.sum \
		$(d_snap)/$(docdir)/$(p_snap)/test-summaries/
  endif
	if which xz 2>&1 >/dev/null; then \
	  echo -n $(d_snap)/$(docdir)/$(p_snap)/test-summaries/* \
	    | xargs -d ' ' -L 1 -P $(USE_CPUS)	xz -7v; \
	fi
else
	dh_installdocs -p$(p_snap)
endif
	if [ -f $(buildlibdir)/libstdc++-v3/testsuite/current_symbols.txt ]; \
	then \
	  cp -p $(buildlibdir)/libstdc++-v3/testsuite/current_symbols.txt \
	    $(d_snap)/$(docdir)/$(p_snap)/libstdc++6_symbols.txt; \
	fi
	cp -p debian/README.snapshot \
		$(d_snap)/$(docdir)/$(p_snap)/README.Debian
	cp -p debian/README.Bugs \
		$(d_snap)/$(docdir)/$(p_snap)/
	dh_installchangelogs -p$(p_snap)
ifeq ($(DEB_TARGET_ARCH),hppa)
	dh_dwz -p$(p_snap) -Xdebug -X/cgo -Xbin/go -Xbin/gofmt \
	  $(if $(unstripped_exe),$(foreach i,cc1 cc1obj cc1objplus cc1plus cc1d f951 go1 jc1 lto1, -X/$(i)))
	dh_strip -p$(p_snap) -Xdebug -X.o -X.a -X/cgo -Xbin/go -Xbin/gofmt \
	  $(if $(unstripped_exe),$(foreach i,cc1 cc1obj cc1objplus cc1plus cc1d f951 go1 jc1 lto1, -X/$(i)))
else
	dh_dwz -p$(p_snap) -Xdebug -X/cgo -Xbin/go -Xbin/gofmt \
	  $(if $(unstripped_exe),$(foreach i,cc1 cc1obj cc1objplus cc1plus cc1d f951 go1 jc1 lto1, -X/$(i)))
	dh_strip -p$(p_snap) -Xdebug -X/cgo -Xbin/go -Xbin/gofmt \
	  $(if $(unstripped_exe),$(foreach i,cc1 cc1obj cc1objplus cc1plus cc1d f951 go1 jc1 lto1, -X/$(i)))
endif
	dh_compress -p$(p_snap) -X README.Bugs -X.log.xz -X.sum.xz
	-find $(d_snap) -type d ! -perm 755 -exec chmod 755 {} \;
	dh_fixperms -p$(p_snap)
ifeq ($(with_ada),yes)
	find $(d_snap)/$(gcc_lib_dir) -name '*.ali' | xargs -r chmod 444
endif

	mkdir -p $(d_snap)/usr/share/lintian/overrides
	cp -p debian/gcc-snapshot.overrides \
		$(d_snap)/usr/share/lintian/overrides/$(p_snap)

	( \
	  echo 'libgcc_s $(GCC_SONAME) ${p_snap} (>= $(DEB_VERSION))'; \
	  echo 'libobjc $(OBJC_SONAME) ${p_snap} (>= $(DEB_VERSION))'; \
	  echo 'libgfortran $(FORTRAN_SONAME) ${p_snap} (>= $(DEB_VERSION))'; \
	  echo 'libffi $(FFI_SONAME) ${p_snap} (>= $(DEB_VERSION))'; \
	  echo 'libgomp $(GOMP_SONAME) ${p_snap} (>= $(DEB_VERSION))'; \
	  echo 'libgnat-$(GNAT_SONAME) 1 ${p_snap} (>= $(DEB_VERSION))'; \
	  echo 'libgnarl-$(GNAT_SONAME) 1 ${p_snap} (>= $(DEB_VERSION))'; \
	) > debian/shlibs.local

	$(ignshld)DIRNAME=$(subst n,,$(2)) $(cross_shlibdeps)  \
	  dh_shlibdeps -p$(p_snap) -l$(CURDIR)/$(d_snap)/$(PF)/lib:$(CURDIR)/$(d_snap)/$(PF)/$(if $(filter $(DEB_TARGET_ARCH),amd64 ppc64),lib32,lib64):/usr/$(DEB_TARGET_GNU_TYPE)/lib
	-sed -i -e 's/$(p_snap)[^,]*, //g' debian/$(p_snap).substvars

ifeq ($(with_multiarch_lib),yes)
	: # paths needed for relative lookups from startfile_prefixes
	for ma in $(xarch_multiarch_names); do \
	  mkdir -p $(d_snap)/lib/$$ma; \
	  mkdir -p $(d_snap)/usr/lib/$$ma; \
	done
endif

	dh_gencontrol -p$(p_snap) -- $(common_substvars) \
		'-Vsnap:depends=$(snapshot_depends)' '-Vsnap:recommends=$(snapshot_recommends)'
	dh_installdeb -p$(p_snap)
	dh_md5sums -p$(p_snap)
	dh_builddeb -p$(p_snap)

	trap '' 1 2 3 15; touch $@; mv $(install_snap_stamp)-tmp $(install_snap_stamp)
