arch_binaries  := $(arch_binaries) hppa64

# ----------------------------------------------------------------------
$(binary_stamp)-hppa64: $(install_hppa64_stamp)
	dh_testdir
	dh_testroot

#	dh_installdirs -p$(p_hppa64)

	rm -f $(d_hppa64)/usr/lib/libiberty.a
	-find $(d_hppa64) ! -type d

	: # provide as and ld links
	dh_link -p $(p_hppa64) \
		/usr/bin/hppa64-linux-gnu-as \
		/$(hppa64libexecdir)/gcc/hppa64-linux-gnu/$(versiondir)/as \
		/usr/bin/hppa64-linux-gnu-ld \
		/$(hppa64libexecdir)/gcc/hppa64-linux-gnu/$(versiondir)/ld

	debian/dh_doclink -p$(p_hppa64) $(p_xbase)
	debian/dh_rmemptydirs -p$(p_hppa64)

ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTONS)))
	$(DWZ) \
	  $(d_hppa64)/$(hppa64libexecdir)/gcc/hppa64-linux-gnu/$(versiondir)/cc1 \
	  $(d_hppa64)/$(hppa64libexecdir)/gcc/hppa64-linux-gnu/$(versiondir)/collect2 \
	  $(d_hppa64)/$(hppa64libexecdir)/gcc/hppa64-linux-gnu/$(versiondir)/lto-wrapper \
	  $(d_hppa64)/$(hppa64libexecdir)/gcc/hppa64-linux-gnu/$(versiondir)/lto1
endif
	dh_strip -p$(p_hppa64) -X.o -Xlibgcc.a -Xlibgcov.a
	dh_shlibdeps -p$(p_hppa64)

	mkdir -p $(d_hppa64)/usr/share/lintian/overrides
	cp -p debian/$(p_hppa64).overrides \
		$(d_hppa64)/usr/share/lintian/overrides/$(p_hppa64)

	echo $(p_hppa64) >> debian/arch_binaries

	touch $@
