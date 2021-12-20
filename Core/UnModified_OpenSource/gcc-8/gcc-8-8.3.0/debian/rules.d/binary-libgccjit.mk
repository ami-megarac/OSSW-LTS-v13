ifeq ($(with_libgccjit),yes)
  $(lib_binaries)  += libgccjit
endif

$(lib_binaries)  += libgccjitdev

ifneq ($(DEB_CROSS),yes)
  indep_binaries := $(indep_binaries) libgccjitdoc
endif

p_jitlib	= libgccjit$(GCCJIT_SONAME)
p_jitdbg	= libgccjit$(GCCJIT_SONAME)-dbg
p_jitdev	= libgccjit$(pkg_ver)-dev
p_jitdoc	= libgccjit$(pkg_ver)-doc

d_jitlib	= debian/$(p_jitlib)
d_jitdev	= debian/$(p_jitdev)
d_jitdbg	= debian/$(p_jitdbg)
d_jitdoc	= debian/$(p_jitdoc)

$(binary_stamp)-libgccjit: $(install_jit_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_jitlib) $(d_jitdbg)
	dh_installdirs -p$(p_jitlib) \
		$(usr_lib)
	dh_installdirs -p$(p_jitdbg)

	$(dh_compat2) dh_movefiles -p$(p_jitlib) \
		$(usr_lib)/libgccjit.so.*
	rm -f $(d)/$(usr_lib)/libgccjit.so

	debian/dh_doclink -p$(p_jitlib) $(p_base)
	debian/dh_doclink -p$(p_jitdbg) $(p_base)

	dh_strip -p$(p_jitlib) --dbg-package=$(p_jitdbg)
	$(cross_makeshlibs) dh_makeshlibs -p$(p_jitlib)
	$(call cross_mangle_shlibs,$(p_jitlib))
	$(ignshld)$(cross_shlibdeps) dh_shlibdeps -p$(p_jitlib) \
		$(if $(filter yes, $(with_common_libs)),,-- -Ldebian/shlibs.common$(2))
	$(call cross_mangle_substvars,$(p_jitlib))
	echo $(p_jitlib) $(p_jitdbg) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
	touch $@

$(binary_stamp)-libgccjitdev: $(install_jit_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_jitdev)
	dh_installdirs -p$(p_jitdev) \
		$(usr_lib) \
		$(gcc_lib_dir)/include

	rm -f $(d)/$(usr_lib)/libgccjit.so

	$(dh_compat2) dh_movefiles -p$(p_jitdev) \
		$(gcc_lib_dir)/include/libgccjit*.h
	dh_link -p$(p_jitdev) \
		$(usr_lib)/libgccjit.so.$(GCCJIT_SONAME) $(gcc_lib_dir)/libgccjit.so

	debian/dh_doclink -p$(p_jitdev) $(p_base)

	echo $(p_jitdev) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
	touch $@

$(binary_stamp)-libgccjitdoc: $(install_jit_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_jitdoc)
	dh_installdirs -p$(p_jitdoc) \
		$(PF)/share/info

	$(dh_compat2) dh_movefiles -p$(p_jitdoc) \
		$(PF)/share/info/libgccjit*
	cp -p $(srcdir)/gcc/jit/docs/_build/texinfo/*.png \
		$(d_jitdoc)/$(PF)/share/info/.

	debian/dh_doclink -p$(p_jitdoc) $(p_base)
	echo $(p_jitdoc) >> debian/indep_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
	touch $@
