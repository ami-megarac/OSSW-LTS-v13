$(lib_binaries)  += libatomic
ifeq ($(with_lib64atomic),yes)
  $(lib_binaries)  += lib64atomic
endif
ifeq ($(with_lib32atomic),yes)
  $(lib_binaries)	+= lib32atomic
endif
ifeq ($(with_libn32atomic),yes)
  $(lib_binaries)	+= libn32atomic
endif
ifeq ($(with_libx32atomic),yes)
  $(lib_binaries)	+= libx32atomic
endif
ifeq ($(with_libhfatomic),yes)
  $(lib_binaries)	+= libhfatomic
endif
ifeq ($(with_libsfatomic),yes)
  $(lib_binaries)	+= libsfatomic
endif

define __do_atomic
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l) $(d_d)
	dh_installdirs -p$(p_l) $(usr_lib$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) $(usr_lib$(2))/libatomic.so.*

	debian/dh_doclink -p$(p_l) $(p_lbase)
	debian/dh_doclink -p$(p_d) $(p_lbase)

	dh_strip -p$(p_l) --dbg-package=$(p_d)
	ln -sf libatomic.symbols debian/$(p_l).symbols
	$(cross_makeshlibs) dh_makeshlibs $(ldconfig_arg) -p$(p_l)
	$(call cross_mangle_shlibs,$(p_l))
	$(ignshld)DIRNAME=$(subst n,,$(2)) $(cross_shlibdeps) dh_shlibdeps -p$(p_l) \
		$(call shlibdirs_to_search,,$(2))
	$(call cross_mangle_substvars,$(p_l))
	echo $(p_l) $(p_d) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef

# ----------------------------------------------------------------------

do_atomic = $(call __do_atomic,lib$(1)atomic$(ATOMIC_SONAME),$(1))

$(binary_stamp)-libatomic: $(install_stamp)
	$(call do_atomic,)

$(binary_stamp)-lib64atomic: $(install_stamp)
	$(call do_atomic,64)

$(binary_stamp)-lib32atomic: $(install_stamp)
	$(call do_atomic,32)

$(binary_stamp)-libn32atomic: $(install_stamp)
	$(call do_atomic,n32)

$(binary_stamp)-libx32atomic: $(install_stamp)
	$(call do_atomic,x32)

$(binary_stamp)-libhfatomic: $(install_dependencies)
	$(call do_atomic,hf)

$(binary_stamp)-libsfatomic: $(install_dependencies)
	$(call do_atomic,sf)
