ifeq ($(with_libobjc),yes)
  $(lib_binaries) += libobjc
endif
ifeq ($(with_objcdev),yes)
  $(lib_binaries) += libobjc-dev
endif
ifeq ($(with_lib64objc),yes)
  $(lib_binaries)	+= lib64objc
endif
ifeq ($(with_lib64objcdev),yes)
  $(lib_binaries)	+= lib64objc-dev
endif
ifeq ($(with_lib32objc),yes)
  $(lib_binaries)	+= lib32objc
endif
ifeq ($(with_lib32objcdev),yes)
  $(lib_binaries)	+= lib32objc-dev
endif
ifeq ($(with_libn32objc),yes)
  $(lib_binaries)	+= libn32objc
endif
ifeq ($(with_libn32objcdev),yes)
  $(lib_binaries)	+= libn32objc-dev
endif
ifeq ($(with_libx32objc),yes)
  $(lib_binaries)	+= libx32objc
endif
ifeq ($(with_libx32objcdev),yes)
  $(lib_binaries)	+= libx32objc-dev
endif
ifeq ($(with_libhfobjc),yes)
  $(lib_binaries)	+= libhfobjc
endif
ifeq ($(with_libhfobjcdev),yes)
  $(lib_binaries)	+= libhfobjc-dev
endif
ifeq ($(with_libsfobjc),yes)
  $(lib_binaries)	+= libsfobjc
endif
ifeq ($(with_libsfobjcdev),yes)
  $(lib_binaries)	+= libsfobjc-dev
endif

files_lobjcdev= \
	$(gcc_lib_dir)/include/objc

files_lobjc = \
	$(usr_lib$(2))/libobjc.so.*
ifeq ($(with_objc_gc),yes)
  files_lobjc += \
	$(usr_lib$(2))/libobjc_gc.so.*
endif

define __do_libobjc
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l) $(d_d)
	dh_installdirs -p$(p_l) \
		$(usr_lib$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) \
		$(files_lobjc)

	debian/dh_doclink -p$(p_l) $(p_lbase)
	debian/dh_doclink -p$(p_d) $(p_lbase)

	dh_strip -p$(p_l) --dbg-package=$(p_d)
	rm -f debian/$(p_l).symbols
	$(if $(2),
	  ln -sf libobjc.symbols debian/$(p_l).symbols ,
	  fgrep -v libobjc.symbols.gc debian/libobjc.symbols > debian/$(p_l).symbols
	)
	$(cross_makeshlibs) dh_makeshlibs $(ldconfig_arg) -p$(p_l) \
		-- -a$(call mlib_to_arch,$(2)) || echo XXXXXXXXXXXXXX ERROR $(p_l)
	rm -f debian/$(p_l).symbols
	$(call cross_mangle_shlibs,$(p_l))
	$(ignshld)DIRNAME=$(subst n,,$(2)) $(cross_shlibdeps) dh_shlibdeps -p$(p_l) \
		$(call shlibdirs_to_search,$(subst objc$(OBJC_SONAME),gcc$(GCC_SONAME),$(p_l)),$(2)) \
		$(if $(filter yes, $(with_common_libs)),,-- -Ldebian/shlibs.common$(2))
	$(call cross_mangle_substvars,$(p_l))
	echo $(p_l) $(p_d) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef


define __do_libobjc_dev
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l)
	dh_installdirs -p$(p_l) \
		$(gcc_lib_dir$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) \
		$(files_lobjcdev)

	$(call install_gcc_lib,libobjc,$(OBJC_SONAME),$(2),$(p_l))
	$(if $(filter yes,$(with_objc_gc)),
	  $(if $(2),,
		dh_link -p$(p_l) \
		  /$(usr_lib$(2))/libobjc_gc.so.$(OBJC_SONAME) \
		  /$(gcc_lib_dir$(2))/libobjc_gc.so
	))

	debian/dh_doclink -p$(p_l) $(p_lbase)
	echo $(p_l) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef



# ----------------------------------------------------------------------

do_libobjc = $(call __do_libobjc,lib$(1)objc$(OBJC_SONAME),$(1))
do_libobjc_dev = $(call __do_libobjc_dev,lib$(1)objc-$(BASE_VERSION)-dev,$(1))

$(binary_stamp)-libobjc: $(install_stamp)
	$(call do_libobjc,)

$(binary_stamp)-lib64objc: $(install_stamp)
	$(call do_libobjc,64)

$(binary_stamp)-lib32objc: $(install_stamp)
	$(call do_libobjc,32)

$(binary_stamp)-libn32objc: $(install_stamp)
	$(call do_libobjc,n32)

$(binary_stamp)-libx32objc: $(install_stamp)
	$(call do_libobjc,x32)

$(binary_stamp)-libhfobjc: $(install_stamp)
	$(call do_libobjc,hf)

$(binary_stamp)-libsfobjc: $(install_stamp)
	$(call do_libobjc,sf)


$(binary_stamp)-libobjc-dev: $(install_stamp)
	$(call do_libobjc_dev,)

$(binary_stamp)-lib64objc-dev: $(install_stamp)
	$(call do_libobjc_dev,64)

$(binary_stamp)-lib32objc-dev: $(install_stamp)
	$(call do_libobjc_dev,32)

$(binary_stamp)-libx32objc-dev: $(install_stamp)
	$(call do_libobjc_dev,x32)

$(binary_stamp)-libn32objc-dev: $(install_stamp)
	$(call do_libobjc_dev,n32)

$(binary_stamp)-libhfobjc-dev: $(install_stamp)
	$(call do_libobjc_dev,hf)

$(binary_stamp)-libsfobjc-dev: $(install_stamp)
	$(call do_libobjc_dev,sf)

