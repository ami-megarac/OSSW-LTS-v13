indep_binaries := $(indep_binaries) gcc-source

ifeq ($(BACKPORT),true)
  p_source = gcc$(pkg_ver)-$(GCC_VERSION)-source
else
  p_source = gcc$(pkg_ver)-source
endif
d_source= debian/$(p_source)

$(binary_stamp)-gcc-source: $(install_stamp)
	dh_testdir
	dh_testroot

	dh_installdocs -p$(p_source)
	dh_installchangelogs -p$(p_source)

	dh_install -p$(p_source) $(gcc_tarball) usr/src/gcc$(pkg_ver)
ifneq (,$(gdc_tarball))
	dh_install -p$(p_source) $(gdc_tarball) usr/src/gcc$(pkg_ver)
endif
	tar cf - $$(find './debian' -mindepth 1 \( \
		-name .svn -prune -o \
		-path './debian/.debhelper' -prune -o \
		-path './debian/gcc-*' -type d -prune -o \
		-path './debian/cpp-*' -type d -prune -o \
		-path './debian/*fortran*' -type d -prune -o \
		-path './debian/lib*' -type d -prune -o \
		-path './debian/patches/*' -prune -o \
		-path './debian/tmp*' -prune -o \
		-path './debian/files' -prune -o \
		-path './debian/rules.d/*' -prune -o \
		-path './debian/rules.parameters' -prune -o \
		-path './debian/soname-cache' -prune -o \
		-path './debian/*substvars*' -prune -o \
		-path './debian/gcc-snapshot*' -prune -o \
		-path './debian/*[0-9]*.p*' -prune -o \
		-path './debian/*$(pkg_ver)[.-]*' -prune -o \
		-print \) ) \
	  | tar -x -C $(d_source)/usr/src/gcc$(pkg_ver)  -f -
	# FIXME: Remove generated files
	find $(d_source)/usr/src/gcc$(pkg_ver) -name '*.debhelper.log' -o -name .svn | xargs rm -rf

	touch $(d_source)/usr/src/gcc$(pkg_ver)/debian/rules.parameters

	dh_link -p$(p_source) \
		/usr/src/gcc$(pkg_ver)/debian/patches /usr/src/gcc$(pkg_ver)/patches

	mkdir -p $(d_source)/usr/share/lintian/overrides
	cp -p debian/$(p_source).overrides \
		$(d_source)/usr/share/lintian/overrides/$(p_source)
	echo $(p_source) >> debian/indep_binaries

	touch $@
