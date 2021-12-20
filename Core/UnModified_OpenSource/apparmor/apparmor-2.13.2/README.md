# AppArmor

[![Build status](https://gitlab.com/apparmor/apparmor/badges/master/build.svg)](https://gitlab.com/apparmor/apparmor/commits/master)
[![Overall test coverage](https://gitlab.com/apparmor/apparmor/badges/master/coverage.svg)](https://gitlab.com/apparmor/apparmor/pipelines)
[![Core Infrastructure Initiative Best Practices](https://bestpractices.coreinfrastructure.org/projects/1699/badge)](https://bestpractices.coreinfrastructure.org/projects/1699)

------------
Introduction
------------
AppArmor protects systems from insecure or untrusted processes by
running them in restricted confinement, while still allowing processes
to share files, exercise privilege and communicate with other processes.
AppArmor is a Mandatory Access Control (MAC) mechanism which uses the
Linux Security Module (LSM) framework. The confinement's restrictions
are mandatory and are not bound to identity, group membership, or object
ownership. The protections provided are in addition to the kernel's
regular access control mechanisms (including DAC) and can be used to
restrict the superuser.

The AppArmor kernel module and accompanying user-space tools are
available under the GPL license (the exception is the libapparmor
library, available under the LGPL license, which allows change_hat(2)
and change_profile(2) to be used by non-GPL binaries).

For more information, you can read the techdoc.pdf (available after
building the parser) and by visiting the https://apparmor.net/ web
site.

----------------
Getting in Touch
----------------

Please send all complaints, feature requests, rants about the software,
and questions to the
[AppArmor mailing list](https://lists.ubuntu.com/mailman/listinfo/apparmor).

Bug reports can be filed against the AppArmor project on
[launchpad](https://bugs.launchpad.net/apparmor) or reported to the mailing
list directly for those who wish not to register for an account on
launchpad. See the
[wiki page](https://gitlab.com/apparmor/apparmor/wikis/home#reporting-bugs)
for more information.

Security issues can be filed as security bugs on launchpad
or directed to `security@apparmor.net`. Additional details can be found
in the [wiki](https://gitlab.com/apparmor/apparmor/wikis/home#reporting-security-vulnerabilities).

-------------
Source Layout
-------------

AppArmor consists of several different parts:

```
binutils/	source for basic utilities written in compiled languages
changehat/	source for using changehat with Apache, PAM and Tomcat
common/		common makefile rules
desktop/	empty
kernel-patches/	compatibility patches for various kernel versions
libraries/	libapparmor source and language bindings
parser/		source for parser/loader and corresponding documentation
profiles/	configuration files, reference profiles and abstractions
tests/		regression and stress testsuites
utils/		high-level utilities for working with AppArmor
```

--------------------------------------
Important note on AppArmor kernel code
--------------------------------------

While most of the kernel AppArmor code has been accepted in the
upstream Linux kernel, a few important pieces were not included. These
missing pieces unfortunately are important bits for AppArmor userspace
and kernel interaction; therefore we have included compatibility
patches in the kernel-patches/ subdirectory, versioned by upstream
kernel (2.6.37 patches should apply cleanly to 2.6.38 source).

Without these patches applied to the kernel, the AppArmor userspace
will not function correctly.

------------------------------------------
Building and Installing AppArmor Userspace
------------------------------------------

To build and install AppArmor userspace on your system, build and install in
the following order. Some systems may need to export various python-related
environment variables to complete the build. For example, before building
anything on these systems, use something along the lines of:

```
$ export PYTHONPATH=$(realpath libraries/libapparmor/swig/python)
$ export PYTHON=/usr/bin/python3
$ export PYTHON_VERSION=3
$ export PYTHON_VERSIONS=python3
```

libapparmor:

```
$ cd ./libraries/libapparmor
$ sh ./autogen.sh
$ sh ./configure --prefix=/usr --with-perl --with-python # see below
$ make
$ make check
$ make install
```

[an additional optional argument to libapparmor's configure is --with-ruby, to
generate Ruby bindings to libapparmor.]


Binary Utilities:

```
$ cd binutils
$ make
$ make check
$ make install
```

parser:

```
$ cd parser
$ make		# depends on libapparmor having been built first
$ make check
$ make install
```


Utilities:

```
$ cd utils
$ make
$ make check
$ make install
```

Apache mod_apparmor:

```
$ cd changehat/mod_apparmor
$ make		# depends on libapparmor having been built first
$ make install
```


PAM AppArmor:

```
$ cd changehat/pam_apparmor
$ make		# depends on libapparmor having been built first
$ make install
```


Profiles:

```
$ cd profiles
$ make
$ make check	# depends on the parser having been built first
$ make install
```

[Note that for the parser, binutils, and utils, if you only wish to build/use
 some of the locale languages, you can override the default by passing
 the LANGS arguments to make; e.g. make all install "LANGS=en_US fr".]

-------------------
AppArmor Testsuites
-------------------

A number of testsuites are in the AppArmor sources. Most have documentation on
usage and how to update and add tests. Below is a quick overview of their
location and how to run them.


Regression tests
----------------
For details on structure and adding tests, see
tests/regression/apparmor/README.

To run:

```
$ cd tests/regression/apparmor (requires root)
$ make
$ sudo make tests
$ sudo bash open.sh -r	 # runs and saves the last testcase from open.sh
```

Parser tests
------------
For details on structure and adding tests, see parser/tst/README.

To run:

```
$ cd parser/tst
$ make
$ make tests
```

Libapparmor
-----------
For details on structure and adding tests, see libraries/libapparmor/README.

```
$ cd libraries/libapparmor
$ make check
```

Utils
-----
Tests for the Python utilities exist in the test/ subdirectory.

```
$ cd utils
$ make check
```

The aa-decode utility to be tested can be overridden by
setting up environment variable APPARMOR_DECODE; e.g.:

```
$ APPARMOR_DECODE=/usr/bin/aa-decode make check
```

Profile checks
--------------
A basic consistency check to ensure that the parser and aa-logprof parse
successfully the current set of shipped profiles. The system or other
parser and logprof can be passed in by overriding the PARSER and LOGPROF
variables.

```
$ cd profiles
$ make && make check
```

Stress Tests
------------
To run AppArmor stress tests:

```
$ make all
```

Use these:

```
$ ./change_hat
$ ./child
$ ./kill.sh
$ ./open
$ ./s.sh
```

Or run all at once:

```
$ ./stress.sh
```

Please note that the above will stress the system so much it may end up
invoking the OOM killer.

To run parser stress tests (requires /usr/bin/ruby):

```
$ ./stress.sh
```

(see stress.sh -h for options)

Coverity Support
----------------
Coverity scans are available to AppArmor developers at
https://scan.coverity.com/projects/apparmor.

In order to submit a Coverity build for analysis, the cov-build binary
must be discoverable from your PATH. See the "To Setup" section of
https://scan.coverity.com/download?tab=cxx to obtain a pre-built copy of
cov-build.

To generate a compressed tarball of an intermediate Coverity directory:

```
$ make coverity
```

The compressed tarball is written to
apparmor-<SNAPSHOT_VERSION>-cov-int.tar.gz, where <SNAPSHOT_VERSION>
is something like 2.10.90~3328, and must be uploaded to
https://scan.coverity.com/projects/apparmor/builds/new for analysis. You must
include the snapshot version in Coverity's project build submission form, in
the "Project Version" field, so that it is quickly obvious to all AppArmor
developers what snapshot of the AppArmor repository was used for the analysis.

-----------------------------------------------
Building and Installing AppArmor Kernel Patches
-----------------------------------------------

TODO


-----------------
Required versions
-----------------

The AppArmor userspace utilities are written with some assumptions about
installed and available versions of other tools. This is a (possibly
incomplete) list of known version dependencies:

The Python utilities require a minimum of Python 2.7 (deprecated) or Python 3.3.
Python 3.x is recommended. Python 2.x support is deprecated since AppArmor 2.11.

Some utilities (aa-exec, aa-notify and aa-decode) require Perl 5.10.1 or newer.

Most shell scripts are written for POSIX-compatible sh. aa-decode expects
bash, probably version 3.2 and higher.
