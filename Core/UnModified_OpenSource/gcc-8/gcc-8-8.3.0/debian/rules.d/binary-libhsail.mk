ifeq ($(with_libhsailrt),yes)
  $(lib_binaries)  += libhsail
endif
ifeq ($(with_brigdev),yes)
  $(lib_binaries) += libhsail-dev
endif
#ifeq ($(with_lib64hsailrt),yes)
#  $(lib_binaries)  += lib64hsail
#endif
#ifeq ($(with_lib64hsailrtdev),yes)
#  $(lib_binaries)  += lib64hsail-dev
#endif
#ifeq ($(with_lib32hsailrt),yes)
#  $(lib_binaries)	+= lib32hsail
#endif
#ifeq ($(with_lib32hsailrtdev),yes)
#  $(lib_binaries)	+= lib32hsail-dev
#endif
#ifeq ($(with_libn32hsailrt),yes)
#  $(lib_binaries)	+= libn32hsail
#endif
#ifeq ($(with_libn32hsailrtdev),yes)
#  $(lib_binaries)	+= libn32hsail-dev
#endif
#ifeq ($(with_libx32hsailrt),yes)
#  $(lib_binaries)	+= libx32hsail
#endif
#ifeq ($(with_libx32hsailrtdev),yes)
#  $(lib_binaries)	+= libx32hsail-dev
#endif
#ifeq ($(with_libhfhsailrt),yes)
#  $(lib_binaries)	+= libhfhsail
#endif
#ifeq ($(with_libhfhsailrtdev),yes)
#  $(lib_binaries)	+= libhfhsail-dev
#endif
#ifeq ($(with_libsfhsailrt),yes)
#  $(lib_binaries)	+= libsfhsail
#endif
#ifeq ($(with_libsfhsailrt-dev),yes)
#  $(lib_binaries)	+= libsfhsail-dev
#endif

define __do_hsail
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l) $(d_d)
	dh_installdirs -p$(p_l) $(usr_lib$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) $(usr_lib$(2))/libhsail-rt.so.*

	debian/dh_doclink -p$(p_l) $(p_lbase)
	debian/dh_doclink -p$(p_d) $(p_lbase)

	dh_strip -p$(p_l) --dbg-package=$(p_d)
	ln -sf libhsail-rt.symbols debian/$(p_l).symbols
	$(cross_makeshlibs) dh_makeshlibs $(ldconfig_arg) -p$(p_l)
	$(call cross_mangle_shlibs,$(p_l))
	$(ignshld)DIRNAME=$(subst n,,$(2)) $(cross_shlibdeps) dh_shlibdeps -p$(p_l) \
		$(call shlibdirs_to_search,$(subst hsail-rt$(HSAIL_SONAME),gcc$(GCC_SONAME),$(p_l)),$(2)) \
		$(if $(filter yes, $(with_common_libs)),,-- -Ldebian/shlibs.common$(2))
	$(call cross_mangle_substvars,$(p_l))
	echo $(p_l) $(p_d) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef

define __do_hsail_dev
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l)
	dh_installdirs -p$(p_l) \
		$(gcc_lib_dir$(2))
#	$(dh_compat2) dh_movefiles -p$(p_l)

	$(call install_gcc_lib,libhsail-rt,$(HSAIL_SONAME),$(2),$(p_l))

	debian/dh_doclink -p$(p_l) $(p_lbase)
	echo $(p_l) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef

# ----------------------------------------------------------------------

do_hsail = $(call __do_hsail,lib$(1)hsail-rt$(HSAIL_SONAME),$(1))
do_hsail_dev = $(call __do_hsail_dev,lib$(1)hsail-rt-$(BASE_VERSION)-dev,$(1))

$(binary_stamp)-libhsail: $(install_stamp)
	@echo XXXXXXXXXXXX XX $(HSAIL_SONAME)
	$(call do_hsail,)

$(binary_stamp)-lib64hsail: $(install_stamp)
	$(call do_hsail,64)

$(binary_stamp)-lib32hsail: $(install_stamp)
	$(call do_hsail,32)

$(binary_stamp)-libn32hsail: $(install_stamp)
	$(call do_hsail,n32)

$(binary_stamp)-libx32hsail: $(install_stamp)
	$(call do_hsail,x32)

$(binary_stamp)-libhfhsail: $(install_dependencies)
	$(call do_hsail,hf)

$(binary_stamp)-libsfhsail: $(install_dependencies)
	$(call do_hsail,sf)


$(binary_stamp)-libhsail-dev: $(install_stamp)
	$(call do_hsail_dev,)

$(binary_stamp)-lib64hsail-dev: $(install_stamp)
	$(call do_hsail_dev,64)

$(binary_stamp)-lib32hsail-dev: $(install_stamp)
	$(call do_hsail_dev,32)

$(binary_stamp)-libx32hsail-dev: $(install_stamp)
	$(call do_hsail_dev,x32)

$(binary_stamp)-libn32hsail-dev: $(install_stamp)
	$(call do_hsail_dev,n32)

$(binary_stamp)-libhfhsail-dev: $(install_stamp)
	$(call do_hsail_dev,hf)

$(binary_stamp)-libsfhsail-dev: $(install_stamp)
	$(call do_hsail_dev,sf)
