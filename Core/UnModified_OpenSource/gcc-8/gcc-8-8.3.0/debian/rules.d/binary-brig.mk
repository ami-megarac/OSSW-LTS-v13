ifneq ($(DEB_STAGE),rtlibs)
#  ifneq (,$(filter yes, $(biarch64) $(biarch32) $(biarchn32) $(biarchx32) $(biarchhf) $(biarchsf)))
#    arch_binaries  := $(arch_binaries) brig-multi
#  endif
  arch_binaries := $(arch_binaries) brig
endif

p_brig	= gccbrig$(pkg_ver)$(cross_bin_arch)
d_brig	= debian/$(p_brig)

p_brig_m= gccbrig$(pkg_ver)-multilib$(cross_bin_arch)
d_brig_m= debian/$(p_brig_m)

dirs_brig = \
	$(docdir)/$(p_xbase)/BRIG \
	$(gcc_lexec_dir)

files_brig = \
	$(PF)/bin/$(cmd_prefix)gccbrig$(pkg_ver) \
	$(gcc_lexec_dir)/brig1

$(binary_stamp)-brig: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_brig)
	dh_installdirs -p$(p_brig) $(dirs_brig)
	$(dh_compat2) dh_movefiles -p$(p_brig) $(files_brig)

ifeq ($(unprefixed_names),yes)
	ln -sf $(cmd_prefix)gccbrig$(pkg_ver) \
	    $(d_brig)/$(PF)/bin/gccbrig$(pkg_ver)
#  ifneq ($(GFDL_INVARIANT_FREE),yes-now-pure-gfdl)
#	ln -sf $(cmd_prefix)gccbrig$(pkg_ver).1 \
#	    $(d_brig)/$(PF)/share/man/man1/gccbrig$(pkg_ver).1
#  endif
endif
	cp -p $(srcdir)/gcc/brig/ChangeLog \
		$(d_brig)/$(docdir)/$(p_xbase)/BRIG/changelog.BRIG

	mkdir -p $(d_brig)/usr/share/lintian/overrides
	echo '$(p_brig) binary: hardening-no-pie' \
	  > $(d_brig)/usr/share/lintian/overrides/$(p_brig)
ifeq ($(GFDL_INVARIANT_FREE),yes)
	echo '$(p_brig) binary: binary-without-manpage' \
	  >> $(d_brig)/usr/share/lintian/overrides/$(p_brig)
endif

	debian/dh_doclink -p$(p_brig) $(p_xbase)

	debian/dh_rmemptydirs -p$(p_brig)

ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTONS)))
	$(DWZ) \
	  $(d_brig)/$(gcc_lexec_dir)/brig1
endif
	dh_strip -p$(p_brig) \
	  $(if $(unstripped_exe),-X/brig1)
	dh_shlibdeps -p$(p_brig)
	echo $(p_brig) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)

$(binary_stamp)-brig-multi: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_brig_m)
	dh_installdirs -p$(p_brig_m) $(docdir)

	debian/dh_doclink -p$(p_brig_m) $(p_xbase)

	dh_strip -p$(p_brig_m)
	dh_shlibdeps -p$(p_brig_m)
	echo $(p_brig_m) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
