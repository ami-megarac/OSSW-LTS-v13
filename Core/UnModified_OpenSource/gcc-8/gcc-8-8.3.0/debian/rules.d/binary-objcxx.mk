ifneq ($(DEB_STAGE),rtlibs)
  ifneq (,$(filter yes, $(biarch64) $(biarch32) $(biarchn32) $(biarchx32) $(biarchhf) $(biarchsf)))
    arch_binaries  := $(arch_binaries) objcxx-multi
  endif
  arch_binaries := $(arch_binaries) objcxx
endif

p_objcx		= gobjc++$(pkg_ver)$(cross_bin_arch)
d_objcx		= debian/$(p_objcx)

p_objcx_m	= gobjc++$(pkg_ver)-multilib$(cross_bin_arch)
d_objcx_m	= debian/$(p_objcx_m)

dirs_objcx = \
	$(docdir)/$(p_xbase)/Obj-C++ \
	$(gcc_lexec_dir)

files_objcx = \
	$(gcc_lexec_dir)/cc1objplus

$(binary_stamp)-objcxx: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_objcx)
	dh_installdirs -p$(p_objcx) $(dirs_objcx)
	$(dh_compat2) dh_movefiles -p$(p_objcx) $(files_objcx)

	debian/dh_doclink -p$(p_objcx) $(p_xbase)
	cp -p $(srcdir)/gcc/objcp/ChangeLog \
		$(d_objcx)/$(docdir)/$(p_xbase)/Obj-C++/changelog

	mkdir -p $(d_objcx)/usr/share/lintian/overrides
	echo '$(p_objcx) binary: hardening-no-pie' \
	  > $(d_objcx)/usr/share/lintian/overrides/$(p_objcx)
ifeq ($(GFDL_INVARIANT_FREE),yes)
	echo '$(p_objcx) binary: binary-without-manpage' \
	  >> $(d_objcx)/usr/share/lintian/overrides/$(p_objcx)
endif

	debian/dh_rmemptydirs -p$(p_objcx)

ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTONS)))
	$(DWZ) \
	  $(d_objcx)/$(gcc_lexec_dir)/cc1objplus
endif
	dh_strip -p$(p_objcx) \
	  $(if $(unstripped_exe),-X/cc1objplus)
	dh_shlibdeps -p$(p_objcx)
	echo $(p_objcx) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)

$(binary_stamp)-objcxx-multi: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp
	rm -rf $(d_objcx_m)
	debian/dh_doclink -p$(p_objcx_m) $(p_xbase)
	debian/dh_rmemptydirs -p$(p_objcx_m)
	dh_strip -p$(p_objcx_m)
	dh_shlibdeps -p$(p_objcx_m)
	echo $(p_objcx_m) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
