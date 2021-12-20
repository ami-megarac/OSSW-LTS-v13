$(lib_binaries)  += libmpx
ifeq ($(with_lib64mpx),yes)
  $(lib_binaries)  += lib64mpx
endif
ifeq ($(with_lib32mpx),yes)
  $(lib_binaries)	+= lib32mpx
endif
ifeq ($(with_libn32mpx),yes)
  $(lib_binaries)	+= libn32mpx
endif
ifeq ($(with_libx32mpx),yes)
  $(lib_binaries)	+= libx32mpx
endif
ifeq ($(with_libhfmpx),yes)
  $(lib_binaries)	+= libhfmpx
endif
ifeq ($(with_libsfmpx),yes)
  $(lib_binaries)	+= libsfmpx
endif

define __do_mpx
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l) $(d_d)
	dh_installdirs -p$(p_l) $(usr_lib$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) \
		$(usr_lib$(2))/libmpx.so.* \
		$(usr_lib$(2))/libmpxwrappers.so.*

	debian/dh_doclink -p$(p_l) $(p_lbase)
	debian/dh_doclink -p$(p_d) $(p_lbase)

	if [ -f debian/$(p_l).overrides ]; then \
		mkdir -p debian/$(p_l)/usr/share/lintian/overrides; \
		cp debian/$(p_l).overrides debian/$(p_l)/usr/share/lintian/overrides/$(p_l); \
	fi

	dh_strip -p$(p_l) --dbg-package=$(p_d)
	ln -sf libmpx.symbols debian/$(p_l).symbols
	$(cross_makeshlibs) dh_makeshlibs $(ldconfig_arg) -p$(p_l)
	$(call cross_mangle_shlibs,$(p_l))
	$(ignshld)DIRNAME=$(subst n,,$(2)) $(cross_shlibdeps) dh_shlibdeps -p$(p_l) \
		$(call shlibdirs_to_search, \
			$(subst mpx$(MPX_SONAME),gcc$(GCC_SONAME),$(p_l)) \
		,$(2)) \
		$(if $(filter yes, $(with_common_libs)),,-- -Ldebian/shlibs.common$(2))
	$(call cross_mangle_substvars,$(p_l))
	echo $(p_l) $(p_d) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef

# ----------------------------------------------------------------------

do_mpx = $(call __do_mpx,lib$(1)mpx$(MPX_SONAME),$(1))

$(binary_stamp)-libmpx: $(install_stamp)
	$(call do_mpx,)

$(binary_stamp)-lib64mpx: $(install_stamp)
	$(call do_mpx,64)

$(binary_stamp)-lib32mpx: $(install_stamp)
	$(call do_mpx,32)

$(binary_stamp)-libn32mpx: $(install_stamp)
	$(call do_mpx,n32)

$(binary_stamp)-libx32mpx: $(install_stamp)
	$(call do_mpx,x32)

$(binary_stamp)-libhfmpx: $(install_dependencies)
	$(call do_mpx,hf)

$(binary_stamp)-libsfmpx: $(install_dependencies)
	$(call do_mpx,sf)
