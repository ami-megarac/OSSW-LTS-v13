#! /usr/bin/make --no-print-directory -f

## Default target
RUBY_VERSIONS := $(shell dh_ruby --print-supported)
all: $(RUBY_VERSIONS)

## Targets share the same output files, so must be run serially
.NOTPARALLEL:
.PHONY: all $(RUBY_VERSIONS)

## SELinux does not have a very nice build process
extra_ruby_args  = RUBY=$@
extra_ruby_args += RUBYLIBS="$(shell $@ -e 'puts "-L" + RbConfig::CONFIG["archlibdir"] + " " + RbConfig::CONFIG["LIBRUBYARG_SHARED"]')"

## How to build and install each individually-versioned copy
$(RUBY_VERSIONS): ruby%:
	+$(MAKE) $(extra_ruby_args) clean-rubywrap
	+$(MAKE) $(extra_ruby_args) install-rubywrap
