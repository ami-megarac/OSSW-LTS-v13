# ----------------------------------------------------------------------
#    Copyright (c) 2004, 2005 NOVELL (All rights reserved)
#    Copyright (c) 2016 Canonical, Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, contact Novell, Inc.
# ----------------------------------------------------------------------
NAME:=apache2-mod_apparmor
all:
COMMONDIR=../../common/

include $(COMMONDIR)/Make.rules

TARGET:=mod_apparmor.so
MANPAGES=mod_apparmor.8

APXS:=$(shell if [ -x "/usr/sbin/apxs2" ] ; then \
		echo "/usr/sbin/apxs2" ; \
              elif [ -x "/usr/sbin/apxs" ] ; then \
	        echo "/usr/sbin/apxs" ; \
              elif [ -x "/usr/bin/apxs2" ] ; then \
	        echo "/usr/bin/apxs2" ; \
              elif [ -x "/usr/bin/apxs" ] ; then \
	        echo "/usr/bin/apxs" ; \
              else \
	        echo "apxs" ; \
	      fi ) 
APXS_INSTALL_DIR=$(shell ${APXS} -q LIBEXECDIR)
DESTDIR=
ifdef USE_SYSTEM
  LIBAPPARMOR = $(shell if pkg-config --exists libapparmor ; then \
                                pkg-config --silence-errors --libs libapparmor ; \
                        elif ldconfig -p | grep -q libapparmor\.so$$ ; then \
                                echo -lapparmor ; \
                        fi )
  ifeq ($(strip $(LIBAPPARMOR)),)
    ERROR_MESSAGE = $(error ${nl}\
************************************************************************${nl}\
Unable to find libapparmor installed on this system; either${nl}\
install libapparmor devel packages, set the LIBAPPARMOR variable${nl}\
manually, or build against in-tree libapparmor.${nl}\
************************************************************************${nl})
  endif # LIBAPPARMOR not set
  LDLIBS += $(LIBAPPARMOR)
else
  LIBAPPARMOR_SRC := ../../libraries/libapparmor/
  LIBAPPARMOR_INCLUDE = $(LIBAPPARMOR_SRC)/include
  LIBAPPARMOR_PATH := $(LIBAPPARMOR_SRC)/src/.libs/
    ifeq ($(realpath $(LIBAPPARMOR_PATH)/libapparmor.a),)
        ERROR_MESSAGE = $(error ${nl}\
************************************************************************${nl}\
$(LIBAPPARMOR_PATH)/libapparmor.a is missing; either build against${nl}\
the in-tree libapparmor by building it first and then trying again${nl}\
(see the top-level README for help) or build against the system${nl}\
libapparmor by adding USE_SYSTEM=1 to your make command.${nl}\
************************************************************************${nl})
    endif
  # Need to pass -Wl twice here to get past both apxs2 and libtool, as
  # libtool will add the path to the RPATH of the library if passed -L/some/path
  LIBAPPARMOR_FLAGS = -I$(LIBAPPARMOR_INCLUDE) -Wl,-Wl,-L$(LIBAPPARMOR_PATH)
  LDLIBS = -lapparmor
endif

.PHONY: libapparmor_check
.SILENT: libapparmor_check
libapparmor_check: ; $(ERROR_MESSAGE)

all: libapparmor_check $(TARGET) docs

.PHONY: docs
docs: ${MANPAGES} ${HTMLMANPAGES}

%.so: %.c
	${APXS} ${LIBAPPARMOR_FLAGS} -c $< ${LDLIBS}
	mv .libs/$@ .

.PHONY: install
install: ${TARGET} ${MANPAGES}
	mkdir -p ${DESTDIR}/${APXS_INSTALL_DIR}
	install -m 755 $< ${DESTDIR}/${APXS_INSTALL_DIR}
	$(MAKE) install_manpages DESTDIR=${DESTDIR}

.PHONY: clean
clean: pod_clean
	rm -rf .libs
	rm -f *.la *.lo *.so *.o *.slo

.PHONY: check
check: check_pod_files
