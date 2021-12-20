# Common settings for Ada Debian packaging.
#
#  Copyright (C) 2012-2018 Nicolas Boulenguez <nicolas@debian.org>
#
#  This program is free software: you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License for more details.
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <http://www.gnu.org/licenses/>.


# Typical use:
#
# gnat_version := $(shell gnatgcc -dumpversion)
# DEB_BUILD_MAINT_OPTIONS := hardening=+all
# DEB_LDFLAGS_MAINT_APPEND := -Wl,--as-needed -Wl,--no-undefined -Wl,--no-copy-dt-needed-entries -Wl,--no-allow-shlib-undefined
# DEB_ADAFLAGS_MAINT_APPEND := -gnatwa -Wall
# include /usr/share/dpkg/buildflags.mk
# include /usr/share/ada/debian_packaging-$(gnat_version).mk


# dpkg-dev provides /usr/share/dpkg/default.mk (or the
# more specific buildflags.mk) to set standard variables like
# DEB_HOST_MULTIARCH, CFLAGS, LDFLAGS...  according to the build
# environment (DEB_BUILD_OPTIONS...) and the policy (hardening
# flags...).
# You must include it before this file.
ifeq (,$(findstring /usr/share/dpkg/buildflags.mk,$(MAKEFILE_LIST)))
  $(error Please include /usr/share/dpkg/default.mk (or the more specific \
          buildflags.mk) before $(lastword $(MAKEFILE_LIST)))
endif

# Ada is not in dpkg-dev flag list. We add a sensible default here.

# Format checking is meaningless for Ada sources.
ADAFLAGS := $(filter-out -Wformat -Werror=format-security, $(CFLAGS))

ifdef DEB_ADAFLAGS_SET
  ADAFLAGS := $(DEB_ADAFLAGS_SET)
endif
ADAFLAGS := $(DEB_ADAFLAGS_PREPEND) \
            $(filter-out $(DEB_ADAFLAGS_STRIP),$(ADAFLAGS)) \
            $(DEB_ADAFLAGS_APPEND)

ifdef DEB_ADAFLAGS_MAINT_SET
  ADAFLAGS := $(DEB_ADAFLAGS_MAINT_SET)
endif
ADAFLAGS := $(DEB_ADAFLAGS_MAINT_PREPEND) \
            $(filter-out $(DEB_ADAFLAGS_MAINT_STRIP),$(ADAFLAGS)) \
            $(DEB_ADAFLAGS_MAINT_APPEND)

ifdef DPKG_EXPORT_BUILDFLAGS
  export ADAFLAGS
endif


# Modifying LDFLAGS directly is confusing and deprecated,
# but we keep the old behaviour during the transition period
# because some bug work-arounds rely on --as-needed.

# Avoid dpkg-shlibdeps warning about depending on a library from which
# no symbol is used, see http://wiki.debian.org/ToolChain/DSOLinking.
# Gnatmake users must upgrade to >= 4.6.4-1 to circumvent #680292.
comma := ,
ifeq (,$(filter -Wl$(comma)--as-needed,$(LDFLAGS)))
  $(warning adding -Wl,--as-needed to LDFLAGS for compatibility, \
      but please use DEB_LDFLAGS_MAINT_APPEND instead.)
  LDFLAGS += -Wl,--as-needed
  ifdef DPKG_EXPORT_BUILDFLAGS
    export LDFLAGS
  endif
endif

# Warn during build time if undefined symbols.
ifeq (,$(filter -Wl$(comma)-z$(comma)defs -Wl$(comma)--no-undefined,$(LDFLAGS)))
  $(warning adding -Wl,--no-undefined to LDFLAGS for compatibility, \
     but please use DEB_LDFLAGS_MAINT_APPEND instead.)
  LDFLAGS += -Wl,--no-undefined
  ifdef DPKG_EXPORT_BUILDFLAGS
    export LDFLAGS
  endif
endif

######################################################################
# C compiler version

# GCC binaries must be compatible with GNAT at the binary level, use
# the same version. This setting is mandatory for every upstream C
# compilation ("export CC" is enough for dh_auto_configure with a
# normal ./configure).

CC := gnatgcc

######################################################################
# Options for gprbuild/gnatmake.

# Let Make delegate parallelism to gnatmake/gprbuild.
.NOTPARALLEL:

# Use all processors unless parallel=n is set in DEB_BUILD_OPTIONS.
# http://www.debian.org/doc/debian-policy/ch-source.html#s-debianrules-options
# The value may be useful elsewhere. Example: SPHINXOPTS=-j$(BUILDER_JOBS)
BUILDER_JOBS := $(filter parallel=%,$(DEB_BUILD_OPTIONS))
ifneq (,$(BUILDER_JOBS))
  BUILDER_JOBS := $(subst parallel=,,$(BUILDER_JOBS))
else
  BUILDER_JOBS := $(shell getconf _NPROCESSORS_ONLN)
endif
BUILDER_OPTIONS += -j$(BUILDER_JOBS)

BUILDER_OPTIONS += -R
# Avoid lintian warning about setting an explicit library runpath.
# http://wiki.debian.org/RpathIssue

ifeq (,$(filter terse,$(DEB_BUILD_OPTIONS)))
BUILDER_OPTIONS += -v
endif
# Make exact command lines available for automatic log checkers.

BUILDER_OPTIONS += -eS
# Tell gnatmake to echo commands to stdout instead of stderr, avoiding
# buildds thinking it is inactive and killing it.
# -eS is the default on gprbuild.

# You may be interested in
# -s  recompile if compilation switches have changed
#     (bad default because of interactions between -amxs and standard library)
# -we handle warnings as errors
# -vP2 verbose when parsing projects.
