ifeq ($(with_libgo),yes)
  $(lib_binaries) += libgo
endif
ifeq ($(with_lib64go),yes)
  $(lib_binaries) += lib64go
endif
ifeq ($(with_lib32go),yes)
  $(lib_binaries) += lib32go
endif
ifeq ($(with_libn32go),yes)
  $(lib_binaries) += libn32go
endif
ifeq ($(with_libx32go),yes)
  $(lib_binaries) += libx32go
endif

ifneq ($(DEB_STAGE),rtlibs)
  arch_binaries  := $(arch_binaries) gccgo
  ifneq (,$(filter yes, $(biarch64) $(biarch32) $(biarchn32) $(biarchx32)))
    arch_binaries  := $(arch_binaries) gccgo-multi
  endif
  ifneq ($(DEB_CROSS),yes)
    ifneq ($(GFDL_INVARIANT_FREE),yes)
      indep_binaries := $(indep_binaries) go-doc
    endif
  endif
endif

p_go	= gccgo$(pkg_ver)$(cross_bin_arch)
p_go_m	= gccgo$(pkg_ver)-multilib$(cross_bin_arch)
p_god	= gccgo$(pkg_ver)-doc
p_golib	= libgo$(GO_SONAME)$(cross_lib_arch)

d_go	= debian/$(p_go)
d_go_m	= debian/$(p_go_m)
d_god	= debian/$(p_god)
d_golib	= debian/$(p_golib)

dirs_go = \
	$(docdir)/$(p_xbase)/go \
	$(PF)/bin \
	$(gcc_lexec_dir) \
	$(gcc_lib_dir) \
	$(PF)/include \
	$(PF)/share/man/man1
files_go = \
	$(PF)/bin/$(cmd_prefix)gccgo$(pkg_ver) \
	$(gcc_lexec_dir)/go1

ifneq (,$(filter $(build_type), build-native cross-build-native))
  files_go += \
	$(PF)/bin/$(cmd_prefix){go,gofmt}$(pkg_ver) \
	$(gcc_lexec_dir)/cgo \
	$(gcc_lexec_dir)/{buildid,test2json,vet} \
	$(PF)/share/man/man1/$(cmd_prefix){go,gofmt}$(pkg_ver).1
endif

ifneq ($(GFDL_INVARIANT_FREE),yes)
  files_go += \
	$(PF)/share/man/man1/$(cmd_prefix)gccgo$(pkg_ver).1
endif

ifeq ($(with_standalone_go),yes)

  dirs_go += \
	$(gcc_lib_dir)/include \
	$(PF)/share/man/man1

# XXX: what about triarch mapping?
  files_go += \
	$(PF)/bin/$(cmd_prefix){cpp,gcc,gcov,gcov-tool}$(pkg_ver) \
	$(PF)/bin/$(cmd_prefix)gcc-{ar,ranlib,nm}$(pkg_ver) \
	$(PF)/share/man/man1/$(cmd_prefix)gcc-{ar,nm,ranlib}$(pkg_ver).1 \
	$(gcc_lexec_dir)/{cc1,collect2,lto1,lto-wrapper} \
	$(gcc_lexec_dir)/liblto_plugin.so{,.0,.0.0.0} \
	$(gcc_lib_dir)/{libgcc*,libgcov.a,*.o} \
	$(header_files) \
	$(shell test -e $(d)/$(gcc_lib_dir)/SYSCALLS.c.X \
		&& echo $(gcc_lib_dir)/SYSCALLS.c.X)

  ifeq ($(with_cc1),yes)
    files_go += \
	$(gcc_lib_dir)/plugin/libcc1plugin.so{,.0,.0.0.0}
  endif

  ifneq ($(GFDL_INVARIANT_FREE),yes)
    files_go += \
	$(PF)/share/man/man1/$(cmd_prefix){cpp,gcc,gcov,gcov-tool}$(pkg_ver).1
  endif

  ifeq ($(biarch64),yes)
    files_go += $(gcc_lib_dir)/$(biarch64subdir)/{libgcc*,libgcov.a,*.o}
  endif
  ifeq ($(biarch32),yes)
    files_go += $(gcc_lib_dir)/$(biarch32subdir)/{libgcc*,*.o}
  endif
  ifeq ($(biarchn32),yes)
    files_go += $(gcc_lib_dir)/$(biarchn32subdir)/{libgcc*,libgcov.a,*.o}
  endif
  ifeq ($(biarchx32),yes)
    files_go += $(gcc_lib_dir)/$(biarchx32subdir)/{libgcc*,libgcov.a,*.o}
  endif
endif

# ----------------------------------------------------------------------
define __do_gccgo
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_l) $(d_d)
	dh_installdirs -p$(p_l) $(usr_lib$(2))
	$(dh_compat2) dh_movefiles -p$(p_l) \
		$(usr_lib$(2))/libgo.so.*

	debian/dh_doclink -p$(p_l) $(p_lbase)
	debian/dh_doclink -p$(p_d) $(p_lbase)

	mkdir -p debian/$(p_l)/usr/share/lintian/overrides
	echo '$(p_l) binary: unstripped-binary-or-object' \
	  >> debian/$(p_l)/usr/share/lintian/overrides/$(p_l)

	: # don't strip: https://gcc.gnu.org/ml/gcc-patches/2015-02/msg01722.html
	: # dh_strip -p$(p_l) --dbg-package=$(p_d)
	$(cross_makeshlibs) dh_makeshlibs $(ldconfig_arg) -p$(p_l)
	$(call cross_mangle_shlibs,$(p_l))
	$(ignshld)DIRNAME=$(subst n,,$(2)) $(cross_shlibdeps) dh_shlibdeps -p$(p_l) \
		$(call shlibdirs_to_search, \
			$(subst go$(GO_SONAME),gcc$(GCC_SONAME),$(p_l)) \
			$(subst go$(GO_SONAME),atomic$(ATOMIC_SONAME),$(p_l)) \
		,$(2)) \
		$(if $(filter yes, $(with_common_libs)),,-- -Ldebian/shlibs.common$(2))
	$(call cross_mangle_substvars,$(p_l))
	echo $(p_l) $(p_d) >> debian/$(lib_binaries)

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
endef

do_gccgo = $(call __do_gccgo,lib$(1)go$(GO_SONAME),$(1))

define install_gccgo_lib
	mv $(d)/$(usr_lib$(3))/$(1).a debian/$(4)/$(gcc_lib_dir$(3))/
	if [ -f $(d)/$(usr_lib$(3))/$(1)libbegin.a ]; then \
	  mv $(d)/$(usr_lib$(3))/$(1)libbegin.a debian/$(4)/$(gcc_lib_dir$(3))/; \
	fi
	rm -f $(d)/$(usr_lib$(3))/$(1)*.{la,so}
	dh_link -p$(4) \
	  /$(usr_lib$(3))/$(1).so.$(2) /$(gcc_lib_dir$(3))/$(1).so
endef

# __do_gccgo_libgcc(flavour,package,todir,fromdir)
define __do_gccgo_libgcc
	$(if $(findstring gccgo,$(PKGSOURCE)),
		: # libgcc_s.so may be a linker script on some architectures
		set -e; \
		if [ -h $(4)/libgcc_s.so ]; then \
		  rm -f $(4)/libgcc_s.so; \
		  dh_link -p$(2) /$(libgcc_dir$(1))/libgcc_s.so.$(GCC_SONAME) \
		    $(3)/libgcc_s.so; \
		else \
		  mv $(4)/libgcc_s.so $(d)/$(3)/libgcc_s.so; \
		  dh_link -p$(2) /$(libgcc_dir$(1))/libgcc_s.so.$(GCC_SONAME) \
		    $(3)/libgcc_s.so.$(GCC_SONAME); \
		fi; \
		$(if $(1), dh_link -p$(2) /$(3)/libgcc_s.so \
		    $(gcc_lib_dir)/libgcc_s_$(1).so;)
	)
endef

define do_go_dev
	dh_installdirs -p$(2) $(gcc_lib_dir$(1)) $(usr_lib$(1))
	$(dh_compat2) dh_movefiles -p$(2) \
		$(gcc_lib_dir$(1))/{libgobegin,libgolibbegin}.a \
		$(usr_lib$(1))/go
	$(if $(filter yes, $(with_standalone_go)), \
	  $(call install_gccgo_lib,libgomp,$(GOMP_SONAME),$(1),$(2)))
	$(call install_gccgo_lib,libgo,$(GO_SONAME),$(1),$(2))
	$(call __do_gccgo_libgcc,$(1),$(2),$(gcc_lib_dir$(1)),$(d)/$(usr_lib$(1)))
endef
# ----------------------------------------------------------------------
$(binary_stamp)-libgo: $(install_stamp)
	$(call do_gccgo,)

$(binary_stamp)-lib64go: $(install_stamp)
	$(call do_gccgo,64)

$(binary_stamp)-lib32go: $(install_stamp)
	$(call do_gccgo,32)

$(binary_stamp)-libn32go: $(install_stamp)
	$(call do_gccgo,n32)

$(binary_stamp)-libx32go: $(install_stamp)
	$(call do_gccgo,x32)

# ----------------------------------------------------------------------
$(binary_stamp)-gccgo: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_go)
	dh_installdirs -p$(p_go) $(dirs_go)

	mv $(d)/$(usr_lib)/{libgobegin,libgolibbegin}.a \
		$(d)/$(gcc_lib_dir)/
	if [ -f $(d)/$(usr_lib64)/libgobegin.a ]; then \
	    mv $(d)/$(usr_lib64)/{libgobegin,libgolibbegin}.a \
		$(d)/$(gcc_lib_dir)/64/; \
	fi
	if [ -f $(d)/$(usr_lib32)/libgobegin.a ]; then \
	    mv $(d)/$(usr_lib32)/{libgobegin,libgolibbegin}.a \
		$(d)/$(gcc_lib_dir)/32/; \
	fi
	if [ -f $(d)/$(usr_libn32)/libgobegin.a ]; then \
	    mv $(d)/$(usr_libn32)/{libgobegin,libgolibbegin}.a \
		$(d)/$(gcc_lib_dir)/n32/; \
	fi
	if [ -f $(d)/$(usr_libx32)/libgobegin.a ]; then \
	    mv $(d)/$(usr_libx32)/{libgobegin,libgolibbegin}.a \
		$(d)/$(gcc_lib_dir)/x32/; \
	fi

	$(call do_go_dev,,$(p_go))

	$(dh_compat2) dh_movefiles -p$(p_go) $(files_go)

ifneq (,$(findstring gccgo,$(PKGSOURCE)))
	rm -rf $(d_go)/$(gcc_lib_dir)/include/cilk
	rm -rf $(d_go)/$(gcc_lib_dir)/include/openacc.h
endif

ifeq ($(unprefixed_names),yes)
	ln -sf $(cmd_prefix)gccgo$(pkg_ver) \
	    $(d_go)/$(PF)/bin/gccgo$(pkg_ver)
	ln -sf $(cmd_prefix)go$(pkg_ver) \
	    $(d_go)/$(PF)/bin/go$(pkg_ver)
	ln -sf $(cmd_prefix)gofmt$(pkg_ver) \
	    $(d_go)/$(PF)/bin/gofmt$(pkg_ver)
  ifneq ($(GFDL_INVARIANT_FREE),yes)
	ln -sf $(cmd_prefix)gccgo$(pkg_ver).1 \
	    $(d_go)/$(PF)/share/man/man1/gccgo$(pkg_ver).1
  endif
	ln -sf $(cmd_prefix)go$(pkg_ver).1 \
	    $(d_go)/$(PF)/share/man/man1/go$(pkg_ver).1
	ln -sf $(cmd_prefix)gofmt$(pkg_ver).1 \
	    $(d_go)/$(PF)/share/man/man1/gofmt$(pkg_ver).1
endif

ifeq ($(with_standalone_go),yes)
  ifeq ($(unprefixed_names),yes)
	for i in gcc gcov gcov-tool gcc-ar gcc-nm gcc-ranlib; do \
	  ln -sf $(cmd_prefix)$$i$(pkg_ver) \
	    $(d_go)/$(PF)/bin/$$i$(pkg_ver); \
	done
    ifneq ($(GFDL_INVARIANT_FREE),yes)
	for i in gcc gcov gcov-tool gcc-ar gcc-nm gcc-ranlib; do \
	  ln -sf $(cmd_prefix)gcc$(pkg_ver).1 \
	    $(d_go)/$(PF)/share/man/man1/$$i$(pkg_ver).1; \
	done
    endif
  endif
  ifeq ($(with_gomp),yes)
	mv $(d)/$(usr_lib)/libgomp*.spec $(d_go)/$(gcc_lib_dir)/
  endif
  ifeq ($(with_cc1),yes)
	rm -f $(d)/$(usr_lib)/libcc1.so
	dh_link -p$(p_go) \
		/$(usr_lib)/libcc1.so.$(CC1_SONAME) /$(gcc_lib_dir)/libcc1.so
  endif
endif

	mkdir -p $(d_go)/usr/share/lintian/overrides
	echo '$(p_go) binary: unstripped-binary-or-object' \
	  > $(d_go)/usr/share/lintian/overrides/$(p_go)
	echo '$(p_go) binary: hardening-no-pie' \
	  > $(d_go)/usr/share/lintian/overrides/$(p_go)
ifeq ($(GFDL_INVARIANT_FREE),yes)
	echo '$(p_go) binary: binary-without-manpage' \
	  >> $(d_go)/usr/share/lintian/overrides/$(p_go)
endif

	debian/dh_doclink -p$(p_go) $(p_xbase)

#	cp -p $(srcdir)/gcc/go/ChangeLog \
#		$(d_go)/$(docdir)/$(p_base)/go/changelog
	debian/dh_rmemptydirs -p$(p_go)

ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTONS)))
	$(DWZ) \
	  $(d_go)/$(gcc_lexec_dir)/go1
endif
	dh_strip -v -p$(p_go) -X/cgo -X/go$(pkg_ver) -X/gofmt$(pkg_ver) \
	  -X/buildid -X/test2json -X/vet $(if $(unstripped_exe),-X/go1)
	dh_shlibdeps -p$(p_go)
	echo $(p_go) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)

# ----------------------------------------------------------------------
$(binary_stamp)-gccgo-multi: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_go_m)
	dh_installdirs -p$(p_go_m) $(docdir)

	$(foreach flavour,$(flavours), \
		$(call do_go_dev,$(flavour),$(p_go_m)))

	debian/dh_doclink -p$(p_go_m) $(p_xbase)
	debian/dh_rmemptydirs -p$(p_go_m)
	dh_strip -p$(p_go_m)
	dh_shlibdeps -p$(p_go_m)
	echo $(p_go_m) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)

# ----------------------------------------------------------------------
$(binary_stamp)-go-doc: $(build_html_stamp) $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_god)
	dh_installdirs -p$(p_god) \
		$(docdir)/$(p_xbase)/go \
		$(PF)/share/info
	$(dh_compat2) dh_movefiles -p$(p_god) \
		$(PF)/share/info/gccgo*

	debian/dh_doclink -p$(p_god) $(p_xbase)
	dh_installdocs -p$(p_god)
	rm -f $(d_god)/$(docdir)/$(p_xbase)/copyright
	cp -p html/gccgo.html $(d_god)/$(docdir)/$(p_xbase)/go/
	echo $(p_god) >> debian/indep_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
