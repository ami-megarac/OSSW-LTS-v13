WRAPPERFILES = apparmorlogparse_wrap.c

BUILT_SOURCES = apparmorlogparse_wrap.c

all-local: apparmorlogparse_wrap.o
	$(CC) -module apparmorlogparse_wrap.o -o libaalogparse.so
        
apparmorlogparse_wrap.o: apparmorlogparse_wrap.c
	$(CC) -c apparmorlogparse_wrap.c $(CFLAGS) -I../../src -I/usr/include/classpath  -fno-strict-aliasing -o apparmorlogparse_wrap.o
        
clean-local:
	rm -rf org

apparmorlogparse_wrap.c: org/aalogparse ../SWIG/*.i
	$(SWIG) -java -I../SWIG -I../../src -outdir org/aalogparse \
		-package org.aalogparse -o apparmorlogparse_wrap.c libaalogparse.i

org/aalogparse:
	mkdir -p org/aalogparse

EXTRA_DIST = $(BUILT_SOURCES) 
