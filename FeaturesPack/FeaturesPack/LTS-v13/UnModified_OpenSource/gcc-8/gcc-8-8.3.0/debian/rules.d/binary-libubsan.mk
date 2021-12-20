$(lib_binaries)  += libubsan
ifeq ($(with_lib64ubsan),yes)
  $(lib_binaries)  += lib64ubsan
endif
ifeq ($(with_lib32ubsan),yes)
  $(lib_binaries)	+= lib32ubsan
endif
ifeq ($(with_libn32ubsan),yes)
  $(lib_binaries)	+= libn32ubsan
endif
ifeq ($(with_libx32ubsan),yes)
  $(lib_binaries)	+= libx32ubsan
endif
ifeq ($(with_libhfubsan),yes)
  $(lib_binaries)	+= libhfubsan
endif
ifeq ($(with_libsfubsan),yes)
  $(lib_binaries)	+= libsfubsan
endif

define __do_ubsan
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l) $(d_d)
	dh_installdirs -p$(p_l) $(usr_lib$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) $(usr_lib$(2))/libubsan.so.*

	debian/dh_doclink -p$(p_l) $(p_lbase)
	debian/dh_doclink -p$(p_d) $(p_lbase)

	if [ -f debian/$(p_l).overrides ]; then \
		mkdir -p debian/$(p_l)/usr/share/lintian/overrides; \
		cp debian/$(p_l).overrides debian/$(p_l)/usr/share/lintian/overrides/$(p_l); \
	fi

	dh_strip -p$(p_l) --dbg-package=$(p_d)
	$(cross_makeshlibs) dh_makeshlibs $(ldconfig_arg) -p$(p_l)
	$(call cross_mangle_shlibs,$(p_l))
	$(ignshld)DIRNAME=$(subst n,,$(2)) $(cross_shlibdeps) dh_shlibdeps -p$(p_l) \
		$(call shlibdirs_to_search, \
			$(subst ubsan$(UBSAN_SONAME),gcc$(GCC_SONAME),$(p_l)) \
			$(subst ubsan$(UBSAN_SONAME),stdc++$(CXX_SONAME),$(p_l)) \
		,$(2)) \
		$(if $(filter yes, $(with_common_libs)),,-- -Ldebian/shlibs.common$(2))
	$(call cross_mangle_substvars,$(p_l))
	echo $(p_l) $(p_d) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef

# ----------------------------------------------------------------------

do_ubsan = $(call __do_ubsan,lib$(1)ubsan$(UBSAN_SONAME),$(1))

$(binary_stamp)-libubsan: $(install_stamp)
	$(call do_ubsan,)

$(binary_stamp)-lib64ubsan: $(install_stamp)
	$(call do_ubsan,64)

$(binary_stamp)-lib32ubsan: $(install_stamp)
	$(call do_ubsan,32)

$(binary_stamp)-libn32ubsan: $(install_stamp)
	$(call do_ubsan,n32)

$(binary_stamp)-libx32ubsan: $(install_stamp)
	$(call do_ubsan,x32)

$(binary_stamp)-libhfubsan: $(install_dependencies)
	$(call do_ubsan,hf)

$(binary_stamp)-libsfubsan: $(install_dependencies)
	$(call do_ubsan,sf)
