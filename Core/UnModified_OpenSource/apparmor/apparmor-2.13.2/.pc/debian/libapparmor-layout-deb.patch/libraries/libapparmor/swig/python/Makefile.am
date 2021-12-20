if HAVE_PYTHON

EXTRA_DIST      = libapparmor_wrap.c

SUBDIRS = test

libapparmor_wrap.c: $(srcdir)/../SWIG/libapparmor.i
	$(SWIG) -python -I$(srcdir)/../../include -module LibAppArmor -o $@ $(srcdir)/../SWIG/libapparmor.i

MOSTLYCLEANFILES=libapparmor_wrap.c LibAppArmor.py

all-local: libapparmor_wrap.c setup.py
	if test ! -f libapparmor_wrap.c; then cp $(srcdir)/libapparmor_wrap.c . ; fi
	$(PYTHON) setup.py build

install-exec-local:
	$(PYTHON) setup.py install --root="/$(DESTDIR)" --prefix="$(prefix)"

clean-local:
	if test -x "$(PYTHON)"; then $(PYTHON) setup.py clean; fi
	rm -rf build
	if test $(top_srcdir) != $(top_builddir) ; then rm -f libapparmor_wrap.c ; fi

endif
