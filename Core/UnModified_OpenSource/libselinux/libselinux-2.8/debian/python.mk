#! /usr/bin/make --no-print-directory -f

## Default target
PYTHON_VERSIONS := $(shell pyversions -r)
PYTHON3_VERSIONS := $(shell py3versions -r)
all: $(PYTHON_VERSIONS) $(PYTHON3_VERSIONS)

## Targets share the same output files, so must be run serially
.NOTPARALLEL:
.PHONY: all $(PYTHON_VERSIONS) $(PYTHON3_VERSIONS)

## SELinux does not have a very nice build process
extra_python_args  = PYTHON=$@
extra_python_args += PYLIBS=

## How to build and install each individually-versioned copy
$(PYTHON_VERSIONS) $(PYTHON3_VERSIONS): python%:
	+$(MAKE) $(extra_python_args) clean-pywrap
	+$(MAKE) $(extra_python_args) install-pywrap
