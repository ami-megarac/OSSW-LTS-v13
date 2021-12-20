INCLUDE=/usr/lib/jvm/java/include
TOP=../..
CLASSPATH=${TOP}/build
CFLAGS=-g -O2 -Wall -Wstrict-prototypes -Wl,-soname,$@.${SO_VERS} -pipe  -fpic -D_REENTRANT
INCLUDES=-I$(INCLUDE)  -I$(INCLUDE)/linux
CLASSFILE=${CLASSPATH}/com/novell/apparmor/${JAVA_CLASSNAME}.class
DESTDIR=${TOP}/dist
SO_VERS         = 1
LIB             = lib/
LIBDIR          = /usr/${LIB}
JAVA_CLASSNAME=JNIChangeHat
TARGET=lib${JAVA_CLASSNAME}

all: ${TARGET}.so

clean:
	rm -f *.so  *.so.${SO_VERS} ${JAVA_CLASSNAME}.java com_novell_apparmor_${JAVA_CLASSNAME}.h

${JAVA_CLASSNAME}.java com_novell_apparmor_${JAVA_CLASSNAME}.h:  ${CLASSFILE}
	javah -jni -classpath ${CLASSPATH} com.novell.apparmor.${JAVA_CLASSNAME} 

${TARGET}.so:    ${JAVA_CLASSNAME}.c  ${JAVA_CLASSNAME}.java com_novell_apparmor_${JAVA_CLASSNAME}.h
	gcc ${INCLUDES} ${CFLAGS} -shared -o ${TARGET}.so  ${JAVA_CLASSNAME}.c -lapparmor

install: ${TARGET}.so
	install -d $(DESTDIR)/${LIB} $(DESTDIR)${LIBDIR}
	mv -f $(TARGET).so $(TARGET)-$(VERSION)-$(RELEASE).so.$(SO_VERS)
	install -m 755 $(TARGET)-$(VERSION)-$(RELEASE).so.$(SO_VERS) ${DESTDIR}/${LIB}
	ln -sf /${LIB}/$(TARGET)-$(VERSION)-$(RELEASE).so.$(SO_VERS) ${DESTDIR}/${LIB}/$(TARGET).so.$(SO_VERS)
	ln -sf /${LIB}/$(TARGET).so.$(SO_VERS) ${DESTDIR}${LIBDIR}/$(TARGET).so
