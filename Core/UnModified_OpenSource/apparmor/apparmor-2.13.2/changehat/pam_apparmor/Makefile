# ----------------------------------------------------------------------
#    Copyright (c) 1999, 2004, 2005 NOVELL (All rights reserved)
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
NAME=pam_apparmor
all:
COMMONDIR=../../common/

include $(COMMONDIR)/Make.rules

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
  endif
  LIBAPPARMOR_INCLUDE =
  AA_LDLIBS = $(LIBAPPARMOR)
  AA_LINK_FLAGS =
else
  LIBAPPARMOR_SRC := ../../libraries/libapparmor/
  LIBAPPARMOR_INCLUDE_PATH = $(LIBAPPARMOR_SRC)/include
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
  LIBAPPARMOR_INCLUDE = -I$(LIBAPPARMOR_INCLUDE_PATH)
  AA_LINK_FLAGS = -L$(LIBAPPARMOR_PATH)
  AA_LDLIBS = -lapparmor
endif
EXTRA_CFLAGS=$(CFLAGS) $(CPPFLAGS) -fPIC -shared -Wall $(LIBAPPARMOR_INCLUDE)
LINK_FLAGS=-Xlinker -x $(AA_LINK_FLAGS) $(LDFLAGS)
LIBS=-lpam $(AA_LDLIBS)
OBJECTS=${NAME}.o get_options.o

.PHONY: libapparmor_check
.SILENT: libapparmor_check
libapparmor_check: ; $(ERROR_MESSAGE)

all: libapparmor_check $(NAME).so docs

.PHONY: docs
# docs: we should have some
docs:

$(NAME).so: ${OBJECTS}
	$(CC) $(EXTRA_CFLAGS) $(LINK_FLAGS) -o $@ ${OBJECTS} $(LIBS)

%.o: %.c
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

# need some better way of determining this
DESTDIR=/
SECDIR ?= ${DESTDIR}/lib/security

.PHONY: install
install: $(NAME).so
	install -m 755 -d $(SECDIR)
	install -m 755 $(NAME).so $(SECDIR)/
	
.PHONY: clean
clean:
	rm -f core core.* *.so *.o *.s *.a *~
