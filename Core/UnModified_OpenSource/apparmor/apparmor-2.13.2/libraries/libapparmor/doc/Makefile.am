## Process this file with automake to produce Makefile.in

POD2MAN = pod2man
PODCHECKER = podchecker

if ENABLE_MAN_PAGES

man_MANS = aa_change_hat.2 aa_change_profile.2 aa_stack_profile.2 aa_getcon.2 aa_find_mountpoint.2 aa_splitcon.3 aa_query_label.2 aa_features.3 aa_kernel_interface.3 aa_policy_cache.3

PODS = $(subst .2,.pod,$(man_MANS)) $(subst .3,.pod,$(man_MANS))

EXTRA_DIST = $(man_MANS) $(PODS)

## delete man pages at make clean
CLEANFILES = $(man_MANS)

%.2: %.pod
	$(PODCHECKER) -warnings -warnings $<
	$(POD2MAN) \
		--section=2 \
		--release="AppArmor $(VERSION)" \
		--center="AppArmor" \
		--stderr \
		$< > $@

%.3: %.pod
	$(PODCHECKER) -warnings -warnings $<
	$(POD2MAN) \
		--section=3 \
		--release="AppArmor $(VERSION)" \
		--center="AppArmor" \
		--stderr \
		$< > $@

endif
