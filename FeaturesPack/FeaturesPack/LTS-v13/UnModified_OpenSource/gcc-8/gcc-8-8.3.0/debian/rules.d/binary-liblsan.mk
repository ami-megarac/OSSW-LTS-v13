$(lib_binaries)  += liblsan
ifeq ($(with_lib64lsan),yes)
  $(lib_binaries)  += lib64lsan
endif
ifeq ($(with_lib32lsan),yes)
  $(lib_binaries)	+= lib32lsan
endif
ifeq ($(with_libn32lsan),yes)
  $(lib_binaries)	+= libn32lsan
endif
ifeq ($(with_libx32lsan),yes)
  $(lib_binaries)	+= libx32lsan
endif
ifeq ($(with_libhflsan),yes)
  $(lib_binaries)	+= libhflsan
endif
ifeq ($(with_libsflsan),yes)
  $(lib_binaries)	+= libsflsan
endif

define __do_lsan
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l) $(d_d)
	dh_installdirs -p$(p_l) $(usr_lib$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) $(usr_lib$(2))/liblsan.so.*

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
			$(subst lsan$(LSAN_SONAME),gcc$(GCC_SONAME),$(p_l)) \
			$(subst lsan$(LSAN_SONAME),stdc++$(CXX_SONAME),$(p_l)) \
		,$(2)) \
		$(if $(filter yes, $(with_common_libs)),,-- -Ldebian/shlibs.common$(2))
	$(call cross_mangle_substvars,$(p_l))
	echo $(p_l) $(p_d) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef

# ----------------------------------------------------------------------

do_lsan = $(call __do_lsan,lib$(1)lsan$(LSAN_SONAME),$(1))

$(binary_stamp)-liblsan: $(install_stamp)
	$(call do_lsan,)

$(binary_stamp)-lib64lsan: $(install_stamp)
	$(call do_lsan,64)

$(binary_stamp)-lib32lsan: $(install_stamp)
	$(call do_lsan,32)

$(binary_stamp)-libn32lsan: $(install_stamp)
	$(call do_lsan,n32)

$(binary_stamp)-libx32lsan: $(install_stamp)
	$(call do_lsan,x32)

$(binary_stamp)-libhflsan: $(install_dependencies)
	$(call do_lsan,hf)

$(binary_stamp)-libsflsan: $(install_dependencies)
	$(call do_lsan,sf)
