# ----------------------------------------------------------------------
#    Copyright (c) 2016 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# ----------------------------------------------------------------------
NAME 		= documentation
all:
COMMONDIR=../common/

include $(COMMONDIR)/Make.rules

all: docs

SOURCES:= $(wildcard *.odt)
DOCS:=$(SOURCES:.odt=.pdf)

.PHONY: docs
docs: $(DOCS)

%.pdf: %.odt
	unoconv -v -f pdf --output "$@" "$<"

.PHONY: clean
ifndef VERBOSE
.SILENT: clean
endif
clean:
	rm -f *.pdf

