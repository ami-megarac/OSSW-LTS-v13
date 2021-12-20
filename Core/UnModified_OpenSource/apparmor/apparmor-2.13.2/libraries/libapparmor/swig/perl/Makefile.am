EXTRA_DIST =Makefile.PL libapparmor_wrap.c LibAppArmor.pm examples/*.pl

if HAVE_PERL
noinst_DATA =LibAppArmor.so

libapparmor_wrap.c: $(srcdir)/../SWIG/libapparmor.i
	$(SWIG) -perl -I$(srcdir)/../../include -module LibAppArmor -o $@ $(srcdir)/../SWIG/libapparmor.i

MOSTLYCLEANFILES=libapparmor_wrap.c LibAppArmor.pm

LibAppArmor.pm: libapparmor_wrap.c

Makefile.perl: Makefile.PL LibAppArmor.pm
	$(PERL) $< PREFIX=$(prefix) MAKEFILE=$@
	sed -ie 's/LD_RUN_PATH="\x24(LD_RUN_PATH)"//g' Makefile.perl
	sed -ie 's/^LD_RUN_PATH.*//g' Makefile.perl

LibAppArmor.so: libapparmor_wrap.c Makefile.perl
	if test ! -f libapparmor_wrap.c; then cp $(srcdir)/libapparmor_wrap.c . ; fi
	$(MAKE) -fMakefile.perl
	if test $(top_srcdir) != $(top_builddir) ; then rm -f libapparmor_wrap.c ; fi

install-exec-local: Makefile.perl
	$(MAKE) -fMakefile.perl install_vendor

# sadly there is no make uninstall for perl
#uninstall-local: Makefile.perl
#$(MAKE) -fMakefile.perl uninstall

clean-local:
	if test -f Makefile.perl; then $(MAKE) -fMakefile.perl realclean; fi
#rm -f Makefile.perl Makefile.perl.old
	rm -f *.so # *.o

endif
