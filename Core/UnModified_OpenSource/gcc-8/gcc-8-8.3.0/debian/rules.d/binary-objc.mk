ifneq ($(DEB_STAGE),rtlibs)
  ifneq (,$(filter yes, $(biarch64) $(biarch32) $(biarchn32) $(biarchx32) $(biarchhf) $(biarchsf)))
    arch_binaries  := $(arch_binaries) objc-multi
  endif
  arch_binaries := $(arch_binaries) objc
endif

p_objc	= gobjc$(pkg_ver)$(cross_bin_arch)
d_objc	= debian/$(p_objc)

p_objc_m= gobjc$(pkg_ver)-multilib$(cross_bin_arch)
d_objc_m= debian/$(p_objc_m)

dirs_objc = \
	$(docdir)/$(p_xbase)/ObjC \
	$(gcc_lexec_dir)

files_objc = \
	$(gcc_lexec_dir)/cc1obj

$(binary_stamp)-objc: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_objc)
	dh_installdirs -p$(p_objc) $(dirs_objc)
	$(dh_compat2) dh_movefiles -p$(p_objc) $(files_objc)

	cp -p $(srcdir)/libobjc/{README*,THREADS*} \
		$(d_objc)/$(docdir)/$(p_xbase)/ObjC/.

	cp -p $(srcdir)/libobjc/ChangeLog \
		$(d_objc)/$(docdir)/$(p_xbase)/ObjC/changelog.libobjc

	mkdir -p $(d_objc)/usr/share/lintian/overrides
	echo '$(p_objc) binary: hardening-no-pie' \
	  > $(d_objc)/usr/share/lintian/overrides/$(p_objc)
ifeq ($(GFDL_INVARIANT_FREE),yes)
	echo '$(p_objc) binary: binary-without-manpage' \
	  >> $(d_objc)/usr/share/lintian/overrides/$(p_objc)
endif

	debian/dh_doclink -p$(p_objc) $(p_xbase)

	debian/dh_rmemptydirs -p$(p_objc)

ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTONS)))
	$(DWZ) \
	  $(d_objc)/$(gcc_lexec_dir)/cc1obj
endif
	dh_strip -p$(p_objc) \
	  $(if $(unstripped_exe),-X/cc1obj)
	dh_shlibdeps -p$(p_objc)
	echo $(p_objc) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)

$(binary_stamp)-objc-multi: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_objc_m)
	dh_installdirs -p$(p_objc_m) $(docdir)

	debian/dh_doclink -p$(p_objc_m) $(p_xbase)

	dh_strip -p$(p_objc_m)
	dh_shlibdeps -p$(p_objc_m)
	echo $(p_objc_m) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
