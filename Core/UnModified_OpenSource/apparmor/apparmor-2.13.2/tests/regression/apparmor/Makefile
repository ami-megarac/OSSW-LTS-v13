#	Copyright (C) 2002-2005 Novell/SUSE
#	Copyright (C) 2013 Canonical, Ltd
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

all:
COMMONDIR=../../../common/
include $(COMMONDIR)/Make.rules

ifdef USE_SYSTEM
  # use the system libapparmor headers and library
  LIBAPPARMOR = $(shell if pkg-config --exists libapparmor ; then \
				pkg-config --silence-errors --libs libapparmor ; \
			elif ldconfig -p | grep -q libapparmor\.so$$ ; then \
				echo -lapparmor ; \
			fi )
  ifeq ($(strip $(LIBAPPARMOR)),)
    LIBAPPARMOR_ERROR_MESSAGE = $(error ${nl}\
************************************************************************${nl}\
Unable to find libapparmor installed on this system; either${nl}\
install libapparmor devel packages, set the LIBAPPARMOR variable${nl}\
manually, or build against in-tree libapparmor.${nl}\
************************************************************************${nl})
  endif # LIBAPPARMOR not set
  LDLIBS += $(LIBAPPARMOR)

  AA_EXEC = $(shell which aa-exec)
  ifeq ($(AA_EXEC),)
    AA_EXEC_ERROR_MESSAGE = $(error ${nl}\
************************************************************************${nl}\
Unable to find aa-exec installed on this system; either install the${nl}\
apparmor package, set the AA_EXEC variable manually, or use the in-tree${nl}\
aa-exec.${nl}\
************************************************************************${nl})
  endif # AA_EXEC not set

else # !USE_SYSTEM
  # use in-tree versions
  LIBAPPARMOR_SRC := ../../../libraries/libapparmor/
  LIBAPPARMOR_INCLUDE = $(LIBAPPARMOR_SRC)/include
  LIBAPPARMOR_PATH := $(LIBAPPARMOR_SRC)/src/.libs/
  ifeq ($(realpath $(LIBAPPARMOR_PATH)/libapparmor.a),)
        LIBAPPARMOR_ERROR_MESSAGE = $(error ${nl}\
************************************************************************${nl}\
$(LIBAPPARMOR_PATH)/libapparmor.a is missing; either build against${nl}\
the in-tree libapparmor by building it first and then trying again${nl}\
(see the top-level README for help) or build against the system${nl}\
libapparmor by adding USE_SYSTEM=1 to your make command.${nl}\
************************************************************************${nl})
  endif

  BINUTILS_SRC := ../../../binutils
  AA_EXEC = $(BINUTILS_SRC)/aa-exec
  ifeq ($(realpath $(AA_EXEC)),)
        AA_EXEC_ERROR_MESSAGE = $(error ${nl}\
************************************************************************${nl}\
$(AA_EXEC) is missing; either build the $(BINUTILS_SRC) directory${nl}\
and then try again (see the top-level README for help) or use the${nl}\
system aa-exec by adding USE_SYSTEM=1 to your make command.${nl}\
************************************************************************${nl})
  endif

  CFLAGS += -L$(LIBAPPARMOR_PATH) -I$(LIBAPPARMOR_INCLUDE)
  LDLIBS += -Wl,-Bstatic -lapparmor -Wl,-Bdynamic -lpthread
endif # USE_SYSTEM

CFLAGS += -g -O0 -Wall -Wstrict-prototypes

SRC=access.c \
    at_secure.c \
    introspect.c \
    changeprofile.c \
    changehat.c \
    changehat_fork.c \
    changehat_misc.c \
    changehat_misc2.c \
    changehat_twice.c \
    changehat_fail.c \
    changehat_wrapper.c \
    changehat_pthread.c \
    chdir.c \
    chgrp.c \
    chmod.c \
    chown.c \
    clone.c \
    coredump.c \
    deleted.c \
    environ.c \
    env_check.c \
    exec.c \
    exec_qual.c \
    exec_qual2.c \
    fchdir.c \
    fchgrp.c \
    fchmod.c \
    fchown.c \
    fd_inheritance.c \
    fd_inheritor.c \
    fork.c \
    link.c \
    link_subset.c \
    mmap.c \
    mkdir.c \
    mount.c \
    named_pipe.c \
    net_raw.c \
    open.c \
    openat.c \
    pipe.c \
    pivot_root.c \
    ptrace.c \
    ptrace_helper.c \
    pwrite.c \
    query_label.c \
    rename.c \
    readdir.c \
    rw.c \
    socketpair.c \
    symlink.c \
    syscall_mknod.c \
    swap.c \
    syscall_chroot.c \
    syscall_mlockall.c \
    syscall_ptrace.c \
    syscall_reboot.c \
    syscall_setpriority.c \
    syscall_sethostname.c \
    syscall_setdomainname.c \
    syscall_setscheduler.c \
    syscall_sysctl.c \
    sysctl_proc.c \
    tcp.c \
    transition.c \
    unix_fd_client.c \
    unix_fd_server.c \
    unix_socket.c \
    unix_socket_client.c \
    unlink.c \
    xattrs.c

#only do the ioperm/iopl tests for x86 derived architectures
ifneq (,$(findstring $(shell uname -i),i386 i486 i586 i686 x86 x86_64))
SRC+=syscall_ioperm.c syscall_iopl.c
endif

#only do dbus if proper libs are installl
ifneq (,$(shell pkg-config --exists dbus-1 && echo TRUE))
SRC+=dbus_eavesdrop.c dbus_message.c dbus_service.c dbus_unrequested_reply.c
else
$(warning ${nl}\
************************************************************************${nl}\
No dbus pkg-config skipping dbus_eavesdrop dbus_message dbus_services tests ...${nl}\
Install libdbus-1-dev or equivalent package to build and run these tests${nl}\
************************************************************************${nl})
endif

TRANSITION_CFLAGS=
ifdef USE_SYSTEM
  ifneq (,$(shell pkg-config --atleast-version 2.10 libapparmor && echo TRUE))
    SRC+=aa_policy_cache.c
    CONDITIONAL_TESTS+=aa_policy_cache
  else
    $(warning ${nl}\
    ************************************************************************${nl}\
    Skipping aa_policy_cache tests: requires libapparmor 2.10 or newer ...${nl}\
    ************************************************************************${nl})
  endif

  ifneq (,$(shell pkg-config --atleast-version 2.10.95 libapparmor && echo TRUE))
    CONDITIONAL_TESTS+=exec_stack stackonexec stackprofile
  else
    $(warning ${nl}\
    ************************************************************************${nl}\
    Skipping stacking tests: requires libapparmor 2.11 Beta 1 or newer ...${nl}\
    ************************************************************************${nl})
    TRANSITION_CFLAGS=-DWITHOUT_STACKING
  endif
else
  SRC+=aa_policy_cache.c
  CONDITIONAL_TESTS+=exec_stack aa_policy_cache stackonexec stackprofile
endif

EXEC=$(SRC:%.c=%)

TESTS=aa_exec \
      access \
      at_secure \
      introspect \
      capabilities \
      changeprofile \
      onexec \
      changehat \
      changehat_fork \
      changehat_misc \
      chdir \
      clone \
      coredump \
      deleted \
      environ \
      exec \
      exec_qual \
      fchdir \
      fd_inheritance \
      fork \
      i18n \
      link \
      link_subset \
      mkdir \
      mmap \
      mount \
      mult_mount \
      named_pipe \
      namespaces \
      net_raw \
      open \
      openat \
      pipe \
      pivot_root \
      ptrace \
      pwrite \
      query_label \
      regex \
      rename \
      readdir \
      rw \
      socketpair \
      swap \
      sd_flags \
      setattr \
      symlink \
      syscall \
      tcp \
      unix_fd_server \
      unix_socket_pathname \
      unix_socket_abstract \
      unix_socket_unnamed \
      unlink\
      xattrs\
      longpath

#only do dbus if proper libs are installl
ifneq (,$(shell pkg-config --exists dbus-1 && echo TRUE))
TESTS+=dbus_eavesdrop dbus_message dbus_service dbus_unrequested_reply
endif

TESTS+=$(CONDITIONAL_TESTS)

# Tests that can crash the kernel should be placed here
RISKY_TESTS=

.PHONY: libapparmor_check
.SILENT: libapparmor_check
libapparmor_check: ; $(LIBAPPARMOR_ERROR_MESSAGE)

aa_exec_check: ; $(AA_EXEC_ERROR_MESSAGE)

all: libapparmor_check aa_exec_check $(EXEC) changehat.h uservars.inc

uservars.inc: uservars.inc.source uservars.inc.system
ifdef USE_SYSTEM
	cp uservars.inc.system uservars.inc
else # !USE_SYSTEM
	cp uservars.inc.source uservars.inc
endif # USE_SYSTEM

at_secure: at_secure.c transition
	${CC} ${CFLAGS} ${LDFLAGS} $< -o $@ ${LDLIBS}

changehat_pthread: changehat_pthread.c changehat.h
	${CC} ${CFLAGS} ${LDFLAGS} $< -o $@ ${LDLIBS} -pthread

dbus_common.o: dbus_common.c dbus_common.h
	${CC} ${CFLAGS} ${LDFLAGS} $< -c ${LDLIBS} $(shell pkg-config --cflags --libs dbus-1)

dbus_eavesdrop: dbus_eavesdrop.c dbus_common.o
	${CC} ${CFLAGS} ${LDFLAGS} $^ -o $@ ${LDLIBS} $(shell pkg-config --cflags --libs dbus-1)

dbus_message: dbus_message.c dbus_common.o
	${CC} ${CFLAGS} ${LDFLAGS} $^ -o $@ ${LDLIBS} $(shell pkg-config --cflags --libs dbus-1)

dbus_service: dbus_message dbus_service.c dbus_common.o
	${CC} ${CFLAGS} ${LDFLAGS} $(filter-out dbus_message, $^) -o $@ ${LDLIBS} $(shell pkg-config --cflags --libs dbus-1)

dbus_unrequested_reply: dbus_service dbus_unrequested_reply.c dbus_common.o
	${CC} ${CFLAGS} ${LDFLAGS} $(filter-out dbus_service, $^) -o $@ ${LDLIBS} $(shell pkg-config --cflags --libs dbus-1)

transition: transition.c
	${CC} ${CFLAGS} ${TRANSITION_CFLAGS} ${LDFLAGS} $< -o $@ ${LDLIBS}

unix_socket_common.o: unix_socket_common.c unix_socket_common.h
	${CC} ${CFLAGS} ${LDFLAGS} $< -c ${LDLIBS}

unix_socket_client: unix_socket_client.c unix_socket_common.o
	${CC} ${CFLAGS} ${LDFLAGS} $^ -o $@ ${LDLIBS}

unix_socket: unix_socket.c unix_socket_common.o unix_socket_client
	${CC} ${CFLAGS} ${LDFLAGS} $(filter-out unix_socket_client, $^) -o $@ ${LDLIBS}

tests: all
	@if [ `whoami` = "root" ] ;\
	then \
		rc=0; \
		for i in $(TESTS) ;\
		do \
			echo ;\
			echo "running $$i" ;\
			bash $$i.sh ;\
			if [ $$? -ne 0 ] ; then \
				rc=1;\
			fi;\
		done ;\
		exit $$rc;\
	else \
		echo "must be root to run tests" ;\
		exit 1;\
	fi

alltests: all
	@if [ `whoami` = "root" ] ;\
	then \
		rc=0; \
		for i in $(TESTS) $(RISKY_TESTS) ;\
		do \
			echo ;\
			echo "running $$i" ;\
			bash $$i.sh ;\
			if [ $$? -ne 0 ] ; then \
				rc=1;\
			fi;\
		done ;\
		exit $$rc;\
	else \
		echo "must be root to run tests" ;\
		exit 1;\
	fi

clean:
	rm -f $(EXEC) dbus_common.o unix_socket_common.o uservars.inc

regex.sh: open exec
