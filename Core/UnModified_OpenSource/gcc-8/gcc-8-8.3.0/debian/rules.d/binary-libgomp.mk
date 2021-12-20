$(lib_binaries)  += libgomp
ifeq ($(with_lib64gomp),yes)
  $(lib_binaries)  += lib64gomp
endif
ifeq ($(with_lib32gomp),yes)
  $(lib_binaries)	+= lib32gomp
endif
ifeq ($(with_libn32gomp),yes)
  $(lib_binaries)	+= libn32gomp
endif
ifeq ($(with_libx32gomp),yes)
  $(lib_binaries)	+= libx32gomp
endif
ifeq ($(with_libhfgomp),yes)
  $(lib_binaries)	+= libhfgomp
endif
ifeq ($(with_libsfgomp),yes)
  $(lib_binaries)	+= libsfgomp
endif

define __do_gomp
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l) $(d_d)
	dh_installdirs -p$(p_l) $(usr_lib$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) $(usr_lib$(2))/libgomp.so.*

	debian/dh_doclink -p$(p_l) $(p_lbase)
	debian/dh_doclink -p$(p_d) $(p_lbase)

	dh_strip -p$(p_l) --dbg-package=$(p_d)
	ln -sf libgomp.symbols debian/$(p_l).symbols
	$(cross_makeshlibs) dh_makeshlibs $(ldconfig_arg) -p$(p_l)
	$(call cross_mangle_shlibs,$(p_l))
	$(ignshld)DIRNAME=$(subst n,,$(2)) $(cross_shlibdeps) dh_shlibdeps -p$(p_l) \
		$(call shlibdirs_to_search,$(subst gomp$(GOMP_SONAME),gcc$(GCC_SONAME),$(p_l)),$(2)) \
		$(if $(filter yes, $(with_common_libs)),,-- -Ldebian/shlibs.common$(2))
	$(call cross_mangle_substvars,$(p_l))
	echo $(p_l) $(p_d) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef

# ----------------------------------------------------------------------

do_gomp = $(call __do_gomp,lib$(1)gomp$(GOMP_SONAME),$(1))

$(binary_stamp)-libgomp: $(install_stamp)
	$(call do_gomp,)

$(binary_stamp)-lib64gomp: $(install_stamp)
	$(call do_gomp,64)

$(binary_stamp)-lib32gomp: $(install_stamp)
	$(call do_gomp,32)

$(binary_stamp)-libn32gomp: $(install_stamp)
	$(call do_gomp,n32)

$(binary_stamp)-libx32gomp: $(install_stamp)
	$(call do_gomp,x32)

$(binary_stamp)-libhfgomp: $(install_dependencies)
	$(call do_gomp,hf)

$(binary_stamp)-libsfgomp: $(install_dependencies)
	$(call do_gomp,sf)
