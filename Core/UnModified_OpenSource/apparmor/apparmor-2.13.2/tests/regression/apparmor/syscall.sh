#! /bin/bash
#
#	Copyright (C) 2002-2005 Novell/SUSE
#	Copyright (C) 2010 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME syscall
#=DESCRIPTION 
# Confined processes are prohibited from executing certain system calls 
# entirely.  This test checks a variety of such syscalls including ptrace, 
# mknod, sysctl (write), sethostname, setdomainname, ioperm, iopl and reboot.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

##
## A. PTRACE
##
settest syscall_ptrace

# TEST A1
runchecktest "PTRACE with no profile" pass sub

# TEST A2.  ptrace will fail
genprofile

runchecktest "PTRACE with confinement" fail sub

##
## B. MKNOD
##

mknod_file=$tmpdir/mknod_file
mknod_okperm=rw
mknod_badperm=r

settest syscall_mknod

# TEST B1
rm -f $mknod_file
runchecktest "MKNOD block (no confinement)" pass b $mknod_file

# TEST B2
rm -f $mknod_file
runchecktest "MKNOD char (no confinement)" pass c $mknod_file

# TEST B3
rm -f $mknod_file
runchecktest "MKNOD fifo (no confinement)" pass f $mknod_file

# TEST B4
rm -f $mknod_file
runchecktest "MKNOD sock (no confinement)" pass s $mknod_file

# TEST B5
rm -f $mknod_file
runchecktest "MKNOD regular file (no confinement)" pass r $mknod_file

# PASS with acceptable permissions
genprofile $mknod_file:$mknod_okperm cap:mknod

# TEST B6 
rm -f $mknod_file
runchecktest "MKNOD block (confined)" pass b $mknod_file

# TEST B7
rm -f $mknod_file
runchecktest "MKNOD char (confined)" pass c $mknod_file

genprofile $mknod_file:$mknod_okperm

# TEST B8
rm -f $mknod_file
runchecktest "MKNOD fifo (confined)" pass f $mknod_file

# TEST B9
rm -f $mknod_file
runchecktest "MKNOD sock (confined)" pass s $mknod_file

# TEST B10
rm -f $mknod_file
runchecktest "MKNOD regular file (confined)" pass r $mknod_file

# FAIL due to permissions
genprofile $mknod_file:$mknod_badperm cap:mknod

# TEST B11
rm -f $mknod_file
runchecktest "MKNOD block (permissions)" fail b $mknod_file

# TEST B12
rm -f $mknod_file
runchecktest "MKNOD char (permissions)" fail c $mknod_file

# TEST B13
rm -f $mknod_file
runchecktest "MKNOD regular file (permissions)" fail r $mknod_file

# TEST B14
rm -f $mknod_file
runchecktest "MKNOD fifo (permissions)" fail f $mknod_file

# TEST B15
rm -f $mknod_file
runchecktest "MKNOD sock (permissions)" fail s $mknod_file

##
## C. SYSCTL
##
bash syscall_sysctl.sh

##
## D. SETHOSTNAME 
##
settest syscall_sethostname

# TEST D1
runchecktest "SETHOSTNAME (no confinement)" pass dumb.example.com

# TEST D2.  sethostname will fail
genprofile

runchecktest "SETHOSTNAME (confinement)" fail another.dumb.example.com

##
## E. SETDOMAINNAME 
##
settest syscall_setdomainname

# TEST D1
runchecktest "SETDOMAINNAME (no confinement)" pass example.com

# TEST D2.  sethostname will fail
genprofile

runchecktest "SETDOMAINNAME (confinement)" fail dumb.com

#only do the ioperm/iopl tests for x86 derived architectures
case `uname -i` in
i386 | i486 | i586 | i686 | x86 | x86_64) 
# But don't run them on xen kernels
if [ ! -d /proc/xen ] ; then

##
## F. IOPERM
##
settest syscall_ioperm

# TEST F1
runchecktest "IOPERM (no confinement)" pass 0 0x3ff

# TEST F2. ioperm will fail
genprofile

runchecktest "IOPERM (confinement)" fail 0 0x3ff

##
## G. IOPL
##
settest syscall_iopl

# TEST G1
runchecktest "IOPL (no confinement)" pass 3

# TEST G2. iopl will fail
genprofile

runchecktest "IOPL (confinement)" fail 3
fi
esac

##
## I. reboot - you can enable/disable the Ctrl-Alt-Del behavior
##             (supposedly) through the reboot(2) syscall, so these
##	       tests should be safe
##
settest syscall_reboot

# TEST I1
runchecktest "REBOOT - disable CAD (no confinement)" pass off

# TEST I2. reboot will fail
genprofile

runchecktest "REBOOT - disable CAD (confinement)" fail off
