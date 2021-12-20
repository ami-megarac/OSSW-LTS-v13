arch_binaries += base
ifeq ($(with_gcclbase),yes)
  ifneq ($(with_base_only),yes)
    indep_binaries += lbase
  endif
endif

# ---------------------------------------------------------------------------
# gcc-base

ifneq (,$(filter $(distrelease),oneiric precise wheezy sid))
  additional_links =
else ifneq (,$(filter $(distrelease),))
  additional_links =
else
  additional_links =
endif

$(binary_stamp)-base: $(install_dependencies)
	dh_testdir
	dh_testroot
	rm -rf $(d_base)
	dh_installdirs -p$(p_base) \
		$(gcc_lexec_dir)

ifeq ($(with_base_only),yes)
	dh_installdocs -p$(p_base) debian/README.Debian
else
	dh_installdocs -p$(p_base) debian/README.Debian.$(DEB_TARGET_ARCH)
endif
	rm -f $(d_base)/usr/share/doc/$(p_base)/README.Debian
	dh_installchangelogs -p$(p_base)
	dh_compress -p$(p_base)
	dh_fixperms -p$(p_base)
ifeq ($(DEB_STAGE)-$(DEB_CROSS),rtlibs-yes)
	$(cross_gencontrol) dh_gencontrol -p$(p_base) -- -v$(DEB_VERSION) $(common_substvars)
else
	dh_gencontrol -p$(p_base) -- -v$(DEB_VERSION) $(common_substvars)
endif
	dh_installdeb -p$(p_base)
	dh_md5sums -p$(p_base)
	dh_builddeb -p$(p_base)
	touch $@

$(binary_stamp)-lbase: $(install_dependencies)
	dh_testdir
	dh_testroot
	rm -rf $(d_lbase)
	dh_installdocs -p$(p_lbase) debian/README.Debian
	rm -f debian/$(p_lbase)/usr/share/doc/$(p_lbase)/README.Debian
	dh_installchangelogs -p$(p_lbase)
	dh_compress -p$(p_lbase)
	dh_fixperms -p$(p_lbase)
	dh_gencontrol -p$(p_lbase) -- -v$(DEB_VERSION) $(common_substvars)
	dh_installdeb -p$(p_lbase)
	dh_md5sums -p$(p_lbase)
	dh_builddeb -p$(p_lbase)
	touch $@
