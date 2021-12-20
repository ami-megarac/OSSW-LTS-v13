if HAVE_RUBY

EXTRA_DIST =            extconf.rb LibAppArmor_wrap.c examples/*.rb
noinst_DATA =           LibAppArmor.so

LibAppArmor_wrap.c : $(srcdir)/../SWIG/libapparmor.i
	$(SWIG) -ruby -module LibAppArmor -I$(top_srcdir)/include -o $@ $(srcdir)/../SWIG/libapparmor.i

MOSTLYCLEANFILES=LibAppArmor_wrap.c

Makefile.ruby: extconf.rb
	PREFIX=$(prefix) $(RUBY) $< --with-LibAppArmor-include=$(top_srcdir)/include

LibAppArmor.so: LibAppArmor_wrap.c Makefile.ruby
	$(MAKE) -fMakefile.ruby

install-exec-local: Makefile.ruby
	$(MAKE) -fMakefile.ruby install

#uninstall
#./lib/ruby/site_ruby/1.8/i686-linux/LibAppArmor.so

clean-local:
	if test -f Makefile.ruby; then $(MAKE) -fMakefile.ruby clean; fi
	rm -f Makefile.ruby Makefile.new
	rm -f *.o *.so *.log

endif
