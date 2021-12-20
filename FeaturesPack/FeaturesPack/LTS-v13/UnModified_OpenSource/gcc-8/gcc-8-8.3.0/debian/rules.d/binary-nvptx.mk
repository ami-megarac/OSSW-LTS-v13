ifeq ($(with_offload_nvptx),yes)
  arch_binaries := $(arch_binaries) nvptx
  ifeq ($(with_common_libs),yes)
    arch_binaries := $(arch_binaries) nvptx-plugin
  endif
endif

p_nvptx	= gcc$(pkg_ver)-offload-nvptx
d_nvptx	= debian/$(p_nvptx)

p_pl_nvptx = libgomp-plugin-nvptx1
d_pl_nvptx = debian/$(p_pl_nvptx)

dirs_nvptx = \
	$(docdir)/$(p_xbase)/ \
	$(PF)/bin \
	$(gcc_lexec_dir)/accel

files_nvptx = \
	$(PF)/bin/$(DEB_TARGET_GNU_TYPE)-accel-nvptx-none-gcc$(pkg_ver) \
	$(gcc_lexec_dir)/accel/nvptx-none

# not needed: libs moved, headers not needed for lto1
#	$(PF)/nvptx-none

# are these needed?
#	$(PF)/lib/gcc/nvptx-none/$(versiondir)/{include,finclude,mgomp}

ifneq ($(GFDL_INVARIANT_FREE),yes)
  files_nvptx += \
	$(PF)/share/man/man1/$(DEB_HOST_GNU_TYPE)-accel-nvptx-none-gcc$(pkg_ver).1
endif

$(binary_stamp)-nvptx: $(install_stamp)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_nvptx)
	dh_installdirs -p$(p_nvptx) $(dirs_nvptx)
	$(dh_compat2) dh_movefiles --sourcedir=$(d)-nvptx -p$(p_nvptx) \
		$(files_nvptx)

	mkdir -p $(d_nvptx)/usr/share/lintian/overrides
	echo '$(p_nvptx) binary: hardening-no-pie' \
	  > $(d_nvptx)/usr/share/lintian/overrides/$(p_nvptx)
ifeq ($(GFDL_INVARIANT_FREE),yes)
	echo '$(p_nvptx) binary: binary-without-manpage' \
	  >> $(d_nvptx)/usr/share/lintian/overrides/$(p_nvptx)
endif

	debian/dh_doclink -p$(p_nvptx) $(p_xbase)

	debian/dh_rmemptydirs -p$(p_nvptx)

ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTONS)))
	$(DWZ) \
	  $(d_nvptx)/$(gcc_lexec_dir)/accel/nvptx-none/{collect2,lto1,lto-wrapper,mkoffload}
endif
	dh_strip -p$(p_nvptx) \
	  $(if $(unstripped_exe),-X/lto1)
	dh_shlibdeps -p$(p_nvptx)
	echo $(p_nvptx) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)

# ----------------------------------------------------------------------
$(binary_stamp)-nvptx-plugin: $(install_dependencies)
	dh_testdir
	dh_testroot
	mv $(install_stamp) $(install_stamp)-tmp

	rm -rf $(d_pl_nvptx)
	dh_installdirs -p$(p_pl_nvptx) \
		$(docdir) \
		$(usr_lib)
	$(dh_compat2) dh_movefiles -p$(p_pl_nvptx) \
		$(usr_lib)/libgomp-plugin-nvptx.so.*

	debian/dh_doclink -p$(p_pl_nvptx) $(p_xbase)
	debian/dh_rmemptydirs -p$(p_pl_nvptx)

	dh_strip -p$(p_pl_nvptx)
	dh_makeshlibs -p$(p_pl_nvptx)
	dh_shlibdeps -p$(p_pl_nvptx)
	echo $(p_pl_nvptx) >> debian/arch_binaries

	trap '' 1 2 3 15; touch $@; mv $(install_stamp)-tmp $(install_stamp)
