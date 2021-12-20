#!/usr/bin/make -f

export DEB_BUILD_MAINT_OPTIONS = hardening=+all
DPKG_EXPORT_BUILDFLAGS = 1
include /usr/share/dpkg/default.mk

INCDIR1=$(T)/../include
INCDIR2=$(T)/include
LIBDIR=$(T)/lib/.libs

all: sample-server sample-client

sample-server: sample-server.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DPLUGINDIR=\"/usr/lib/$(DEB_HOST_MULTIARCH)/sasl2\" $(LDFLAGS) -g -o sample-server sample-server.c -I. -I$(T) -I$(INCDIR1) -I$(INCDIR2) -L$(LIBDIR) -lsasl2

sample-client: sample-client.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DPLUGINDIR=\"/usr/lib/$(DEB_HOST_MULTIARCH)/sasl2\" $(LDFLAGS) -g -o sample-client sample-client.c -I. -I$(T) -I$(INCDIR1) -I$(INCDIR2) -L$(LIBDIR) -lsasl2
