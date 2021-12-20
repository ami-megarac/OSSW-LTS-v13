#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME syscall_sysctl
#=DESCRIPTION
# Confined processes are prohibited from executing certain system calls.
# This test checks sysctl which is mediated like filesystem accesses
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

sysctlgood=/proc/sys/kernel/threads-max
sysctlbad=/proc/sys/kernel/sysrq

bin=$pwd

. $bin/prologue.inc

##
## C. SYSCTL
##

test_syscall_sysctl()
{
    settest syscall_sysctl

    runchecktest "SYSCTL (no confinement read only)" pass ro

    runchecktest "SYSCTL (no confinement rw)" pass

    genprofile $sysctlgood:r
    runchecktest "SYSCTL (confinement/good r w/ r perm)" pass ro

    genprofile $sysctlgood:r
    runchecktest "SYSCTL (confinement/good rw w/ r perm)" fail

    genprofile $sysctlgood:w
    runchecktest "SYSCTL (confinement/good r w/ w perm)" fail ro

    genprofile $sysctlgood:w
    runchecktest "SYSCTL (confinement/good rw w/ w perm)" fail

    genprofile $sysctlgood:rw
    runchecktest "SYSCTL (confinement/good r w/ rw perm)" pass ro

    genprofile $sysctlgood:rw
    runchecktest "SYSCTL (confinement/good rw w/ rw perm)" pass

    genprofile $sysctlbad:r
    runchecktest "SYSCTL (confinement/bad r w/ r perm)" fail ro

    genprofile $sysctlbad:r
    runchecktest "SYSCTL (confinement/bad rw w/ r perm)" fail ro

    genprofile $sysctlbad:w
    runchecktest "SYSCTL (confinement/bad r w/ w perm)" fail ro

    genprofile $sysctlbad:w
    runchecktest "SYSCTL (confinement/bad rw w/ w perm)" fail

    genprofile $sysctlbad:rw
    runchecktest "SYSCTL (confinement/bad r w/ rw perm)" fail ro

    genprofile $sysctlbad:rw
    runchecktest "SYSCTL (confinement/bad rw w/ rw perm)" fail
}

test_sysctl_proc()
{
    settest sysctl_proc

    #unconfined
    runchecktest "SYSCTL /proc (read no confinement)" pass $sysctlgood r
    value=`cat $sysctlgood`
    runchecktest "SYSCTL /proc (write no confinement)" pass $sysctlgood w $value
    runchecktest "SYSCTL /proc (rw no confinement)" pass $sysctlgood rw

    #test with profile giving access to sysctlgood
    genprofile $sysctlgood:r
    runchecktest "SYSCTL /proc (confinement/good r w/ r perm)" pass $sysctlgood r

    genprofile $sysctlgood:w
    runchecktest "SYSCTL /proc (confinement/good r w/ w perm)" fail $sysctlgood r

    genprofile $sysctlgood:rw
    runchecktest "SYSCTL /proc (confinement/good r w/ rw perm)" pass $sysctlgood r

    genprofile $sysctlgood:r
    value=`cat $sysctlgood`
    runchecktest "SYSCTL /proc (confinement/good w w/ r perm)" fail $sysctlgood w $value

    genprofile $sysctlgood:w
    value=`cat $sysctlgood`
    runchecktest "SYSCTL /proc (confinement/good w w/ w perm)" pass $sysctlgood w $value

    genprofile $sysctlgood:rw
    value=`cat $sysctlgood`
    runchecktest "SYSCTL /proc (confinement/good w w/ rw perm)" pass $sysctlgood w $value

    genprofile $sysctlgood:r
    runchecktest "SYSCTL /proc (confinement/good rw w/ r perm)" fail $sysctlgood rw

    genprofile $sysctlgood:w
    runchecktest "SYSCTL /proc (confinement/good rw w/ w perm)" fail $sysctlgood rw

    genprofile $sysctlgood:rw
    runchecktest "SYSCTL /proc (confinement/good rw w/ rw perm)" pass $sysctlgood rw

    #test with profile giving access to sysctlbad but access to sysctlgood
    genprofile $sysctlbad:r
    runchecktest "SYSCTL /proc (confinement/bad r w/ r perm)" fail $sysctlgood r

    genprofile $sysctlbad:w
    runchecktest "SYSCTL /proc (confinement/bad r w/ w perm)" fail $sysctlgood r

    genprofile $sysctlbad:rw
    runchecktest "SYSCTL /proc (confinement/bad r w/ rw perm)" fail $sysctlgood r

    genprofile $sysctlbad:r
    value=`cat $sysctlgood`
    runchecktest "SYSCTL /proc (confinement/bad w w/ r perm)" fail $sysctlgood w $value

    genprofile $sysctlbad:w
    value=`cat $sysctlgood`
    runchecktest "SYSCTL /proc (confinement/bad w w/ w perm)" fail $sysctlgood w $value

    genprofile $sysctlbad:rw
    value=`cat $sysctlgood`
    runchecktest "SYSCTL /proc (confinement/bad w w/ rw perm)" fail $sysctlgood w $value

    genprofile $sysctlbad:r
    runchecktest "SYSCTL /proc (confinement/bad rw w/ r perm)" fail $sysctlgood rw

    genprofile $sysctlbad:w
    runchecktest "SYSCTL /proc (confinement/bad rw w/ w perm)" fail $sysctlgood rw

    genprofile $sysctlbad:rw
    runchecktest "SYSCTL /proc (confinement/bad rw w/ rw perm)" fail $sysctlgood rw
}


# check if the kernel supports CONFIG_SYSCTL_SYSCALL
# generally we want to encourage kernels to disable it, but if it's
# enabled we want to test against it
settest syscall_sysctl
if ! res="$(${test} ro 2>&1)" && [ "$res" = "FAIL: sysctl read failed - Function not implemented" ] ; then
    echo "	WARNING: syscall sysctl not implemented, skipping tests ..."
else
    test_syscall_sysctl
fi

# now test /proc/sys/ paths
if [ ! -f "${sysctlgood}" ] ; then
    echo "	WARNING: proc sysctl path not found, /proc not mounted? Skipping tests ..."
else
    test_sysctl_proc
fi
