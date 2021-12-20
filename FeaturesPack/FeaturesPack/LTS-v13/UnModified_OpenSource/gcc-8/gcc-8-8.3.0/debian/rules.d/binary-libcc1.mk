ifeq ($(with_libcc1),yes)
  ifneq ($(DEB_CROSS),yes)
    arch_binaries  := $(arch_binaries) libcc1
  endif
endif

p_cc1	= libcc1-$(CC1_SONAME)
d_cc1	= debian/$(p_cc1)

# ----------------------------------------------------------------------
$(binary_stamp)-libcc1: $(install_dependencies)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_cc1)
	dh_installdirs -p$(p_cc1) \
		$(docdir) \
		$(usr_lib)
	$(dh_compat2) dh_movefiles -p$(p_cc1) \
		$(usr_lib)/libcc1.so.*

	debian/dh_doclink -p$(p_cc1) $(p_xbase)
	debian/dh_rmemptydirs -p$(p_cc1)

	dh_strip -p$(p_cc1)
	dh_makeshlibs -p$(p_cc1)
	dh_shlibdeps -p$(p_cc1)
	echo $(p_cc1) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
