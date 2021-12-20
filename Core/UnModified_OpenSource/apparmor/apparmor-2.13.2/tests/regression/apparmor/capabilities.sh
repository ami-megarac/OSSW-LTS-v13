#!/bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME capabilities
#=DESCRIPTION 
# The capabilities test is an attempt to determine that for a variety of 
# syscalls, the expected capability (especially since Immunix intercepts 
# capability processing for confined processes) and no others allows successful
# access.  For every syscall in the test, we iterate over each capability 
# individually (plus no capabilities) in order to verify that only the expected
# capability grants access to the priviledged operation. The same is repeated 
# for capabilities within hats.
#=END

# An attempt to verify what subdomain/posix capabilities actually grant
# access to. This overlaps _a lot_ with the syscall test.
# this now verifies that a capability functions within a changehat().
# FIXME: should test for a cap in the parent, but the need for the cap
# within the subprofile.  Wow. oogly.

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`
bin=$pwd

. $bin/prologue.inc

TESTS="syscall_ptrace syscall_sethostname \
       syscall_setdomainname syscall_setpriority syscall_setscheduler \
       syscall_reboot syscall_chroot \
       syscall_mlockall net_raw"
# FIXME/XXX - need a test case for syscall_mknod

#only do the ioperm/iopl tests for x86 derived architectures
case `uname -i` in
i386 | i486 | i586 | i686 | x86 | x86_64) 
	TESTS="$TESTS syscall_ioperm syscall_iopl"
	;;
esac

CAPABILITIES="chown dac_override dac_read_search fowner fsetid kill \
	      setgid setuid setpcap linux_immutable net_bind_service \
	      net_broadcast net_admin net_raw ipc_lock ipc_owner \
	      sys_module sys_rawio sys_chroot sys_ptrace sys_pacct \
	      sys_admin sys_boot sys_nice sys_resource sys_time \
	      sys_tty_config mknod lease audit_write audit_control"

# defines which test+capability pairs should succeed.
syscall_reboot_sys_boot=TRUE
syscall_sethostname_sys_admin=TRUE
syscall_setdomainname_sys_admin=TRUE
syscall_setpriority_sys_nice=TRUE
syscall_setscheduler_sys_nice=TRUE
syscall_ioperm_sys_rawio=TRUE
syscall_iopl_sys_rawio=TRUE
syscall_chroot_sys_chroot=TRUE
syscall_mlockall_ipc_lock=TRUE
syscall_sysctl_sys_admin=TRUE
net_raw_net_raw=TRUE

# we completely disable ptrace(), but it's not clear if we should allow it
# when the sys_ptrace cap is specified.
# NOTE: we handle special casing of v6 ptrace not needing ptrace cap inline
syscall_ptrace_sys_ptrace=TRUE

# if a test case requires arguments, add them here.
syscall_reboot_args=off
syscall_sethostname_args=a.dumb.example.com
syscall_setdomainname_args=dumb.example.com
syscall_ioperm_args="0 0x3ff"
syscall_iopl_args=3
syscall_chroot_args=${tmpdir}/
syscall_ptrace_args=sub

# if a testcase requires extra subdomain rules, add them here
syscall_chroot_extra_entries="/:r ${tmpdir}/:r"
syscall_ptrace_extra_entries="ptrace:ALL hat:sub ptrace:ALL"
net_raw_extra_entries="network:"

testwrapper=changehat_wrapper

# needed for modern linux kernels
ulimit -l 0

for TEST in ${TESTS} ; do 
	echo "        (${TEST#syscall_})"
	my_arg=$(eval echo \${${TEST}_args})
	my_entries=$(eval echo \${${TEST}_extra_entries})

	settest ${TEST}
	# base case, unconfined
	runchecktest "${TEST} -- unconfined" pass ${my_arg}

	# no capabilities allowed
	genprofile ${my_entries}
	if [ "${TEST}" == "syscall_ptrace" -a "$(kernel_features ptrace)" == "true" ] ; then
	    # ptrace between profiles confining tasks of same pid is controlled by the ptrace rule
	    # capability + ptrace rule needed between pids
	    runchecktest "${TEST} -- no caps" pass ${my_arg}
	else
	    runchecktest "${TEST} -- no caps" fail ${my_arg}
	fi

	# all capabilities allowed
	genprofile cap:ALL ${my_entries}
	runchecktest "${TEST} -- all caps" pass ${my_arg}

	# iterate through each of the capabilities
	for cap in ${CAPABILITIES} ; do
		if [ "X$(eval echo \${${TEST}_${cap}})" == "XTRUE" ] ; then
			expected_result=pass
		elif [ "${TEST}" == "syscall_ptrace" -a "$(kernel_features ptrace)" == "true" ]; then
			expected_result=pass
		else
			expected_result=fail
		fi
		genprofile cap:${cap} ${my_entries}
		runchecktest "${TEST} -- capability ${cap}" ${expected_result} ${my_arg}
	done

	# okay, now check to see if the capability functions from within
	# a subprofile.
	settest ${testwrapper}
	genprofile hat:$bin/${TEST} addimage:${bin}/${TEST} ${my_entries}
	if [ "${TEST}" == "syscall_ptrace" -a "$(kernel_features ptrace)" == "true" ] ; then
	    # ptrace between profiles confining tasks of same pid is controlled by the ptrace rule
	    # capability + ptrace rule needed between pids
	    runchecktest "${TEST} changehat -- no caps" pass $bin/${TEST} ${my_arg}
	else
	    runchecktest "${TEST} changehat -- no caps" fail $bin/${TEST} ${my_arg}
	fi

	# all capabilities allowed
	genprofile hat:$bin/${TEST} addimage:${bin}/${TEST} cap:ALL ${my_entries}
	runchecktest "${TEST} changehat -- all caps" pass $bin/${TEST} ${my_arg}

	for cap in ${CAPABILITIES} ; do
		if [ "X$(eval echo \${${TEST}_${cap}})" == "XTRUE" ] ; then
			expected_result=pass
		elif [ "${TEST}" == "syscall_ptrace" -a "$(kernel_features ptrace)" == "true" ]; then
			expected_result=pass
		else
			expected_result=fail
		fi
		genprofile hat:$bin/${TEST} addimage:${bin}/${TEST} cap:${cap} ${my_entries}
		runchecktest "${TEST} changehat -- capability ${cap}" ${expected_result} $bin/${TEST} ${my_arg}
	done

done

cap=sys_chroot
settest syscall_chroot

# test deny keyword works
genprofile cap:${cap}:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, deny keyword" fail ${syscall_chroot_args}

# test allow keyword works
genprofile cap:${cap}:allow ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow keyword" pass ${syscall_chroot_args}

### allow/deny overlap tests ###

# test allow & deny keyword behavior, allow first
genprofile cap:${cap}:allow cap:${cap}:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow & deny keyword, allow first" fail ${syscall_chroot_args}

# test implicit allow & deny keyword behavior, allow first
genprofile cap:${cap} cap:${cap}:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, implicit allow & deny keyword, allow first" fail ${syscall_chroot_args}

# test allow & deny keyword behavior, deny first
genprofile cap:${cap}:deny cap:${cap}:allow ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow & deny keyword, deny first" fail ${syscall_chroot_args}

# test implicit allow & deny keyword behavior, deny first
genprofile cap:${cap}:deny cap:${cap} ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, implicit allow & deny keyword, deny first" fail ${syscall_chroot_args}

# test allow all & deny all capability keyword behavior, allow first
genprofile cap:ALL:allow cap:ALL:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow & deny all caps keyword, allow first" fail ${syscall_chroot_args}

# test implicit allow all & deny all capability keyword behavior, allow first
genprofile cap:ALL cap:ALL:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, implicit allow all & deny all caps keyword, allow first" fail ${syscall_chroot_args}

# test allow all & deny all capability keyword behavior, deny first
genprofile cap:ALL:deny cap:ALL:allow ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow & deny all caps keyword, deny first" fail ${syscall_chroot_args}

# test implicit allow all & deny all capability keyword behavior, deny first
genprofile cap:ALL:deny cap:ALL ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, implicit allow & deny all caps keyword, deny first" fail ${syscall_chroot_args}

# test allow all & deny keywords behavior, allow first
genprofile cap:ALL:allow cap:${cap}:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow all & deny keyword, allow first" fail ${syscall_chroot_args}

# test implicit allow all & deny keywords behavior, allow first
genprofile cap:ALL cap:${cap}:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, implicit allow all & deny keyword, allow first" fail ${syscall_chroot_args}

# test allow all & deny keywords behavior, deny first
genprofile cap:${cap}:deny cap:ALL:allow ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow all & deny keyword, deny first" fail ${syscall_chroot_args}

# test implicit allow all & deny keywords behavior, deny first
genprofile cap:${cap}:deny cap:ALL ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, implicit allow all & deny keyword, deny first" fail ${syscall_chroot_args}

# test allow & deny all keywords behavior, allow first
genprofile cap:${cap}:allow cap:ALL:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow & deny all keyword, allow first" fail ${syscall_chroot_args}

# test implicit allow & deny all keywords behavior, allow first
genprofile cap:${cap} cap:ALL:deny ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, implicit allow & deny all keyword, allow first" fail ${syscall_chroot_args}

# test allow & deny all keywords behavior, deny first
genprofile cap:ALL:deny cap:${cap}:allow ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, allow & deny all keyword, deny first" fail ${syscall_chroot_args}

# test implicit allow & deny all keywords behavior, deny first
genprofile cap:ALL:deny cap:${cap} ${syscall_chroot_extra_entries}
runchecktest "syscall_chroot -- capability ${cap}, implicit allow & deny all keyword, deny first" fail ${syscall_chroot_args}
