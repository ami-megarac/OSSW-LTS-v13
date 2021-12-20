#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME named_pipe
#=DESCRIPTION 
# This test verifies that subdomain file access checks function correctly 
# for named piped (nodes in the filesystem created with mknod).  The test 
# creates a parent/child process relationship which attempt to rendevous via 
# the named pipe.   The tests are attempted for unconfined and confined 
# processes and also for subhats.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

fifo=${tmpdir}/pipe

subtest=sub
okperm=rw

subparent=parent
okparent=r
badparent=w

subchild=child
okchild=w
badchild=r

# Add genprofile params that are common to all hats here
common=""

if [ "$(kernel_features signal)" == "true" -a "$(parser_supports 'signal,')" == "true" ] ; then
	# Allow send/receive of all signals
	common="${common} signal:ALL"
fi

mknod ${fifo} p

# NAMED PIPE - no confinement 

runchecktest "NAMED PIPE (no confinement)" pass nochange nochange ${fifo}

# PIPE - confined.

#rm -f ${fifo} && mknod ${fifo} p
genprofile $common $fifo:${okperm}
runchecktest "NAMED PIPE RW (confinement)" pass nochange nochange ${fifo}

# PIPE - confined - no access.

#rm -f ${fifo} && mknod ${fifo} p
genprofile $common
runchecktest "NAMED PIPE (confinement)" fail nochange nochange ${fifo}

# PIPE - child in subprofile.

genprofile $common ${fifo}:${okparent} hat:$subchild $common ${fifo}:${okchild}

runchecktest "NAMED PIPE (child subprofile)" pass nochange ${subchild} ${fifo}

# PIPE - child in subprofile - no child access.

genprofile $common ${fifo}:${okparent} hat:$subchild $common

runchecktest "NAMED PIPE no child access (child subprofile)" fail nochange ${subchild} ${fifo}

# PIPE - child in subprofile - bad child access.

genprofile $common ${fifo}:${okparent} hat:$subchild $common ${fifo}:${badchild}

runchecktest "NAMED PIPE bad child access (child subprofile)" fail nochange ${subchild} ${fifo}

# PIPE - child in subprofile - no parent access.

genprofile $common hat:$subchild $common

runchecktest "NAMED PIPE no parent access (child subprofile)" fail nochange ${subchild} ${fifo}

# PIPE - child in subprofile - bad parent access.

genprofile $common ${fifo}:${badparent} hat:$subchild $common ${fifo}:${badchild}

runchecktest "NAMED PIPE bad parent access (child subprofile)" fail nochange ${subchild} ${fifo}

# PIPE - in the same subprofile.

#rm -f ${fifo} && mknod ${fifo} p
genprofile $common ${fifo}:${okperm} hat:$subtest $common ${fifo}:${okperm}

runchecktest "NAMED PIPE RW (subprofile)" pass ${subtest} ${subtest} ${fifo}

# PIPE - in the same subprofile - no access

#rm -f ${fifo} && mknod ${fifo} p
genprofile $common ${fifo}:${okperm} hat:$subtest $common

runchecktest "NAMED PIPE (subprofile)" fail ${subtest} ${subtest} ${fifo}

# PIPE - in separate subprofiles

genprofile hat:$subparent $common ${fifo}:${okparent} \
	   hat:$subchild $common ${fifo}:${okchild}

runchecktest "NAMED PIPE RW (parent & child subprofiles)" pass ${subparent} ${subchild} ${fifo}

# PIPE - in separate subprofiles - no access for child

genprofile hat:$subparent $common ${fifo}:${okparent} \
	   hat:$subchild $common

runchecktest "NAMED PIPE R (parent & child subprofiles)" fail ${subparent} ${subchild} ${fifo}

# PIPE - in separate subprofiles - no access for parent

genprofile hat:$subparent $common \
	   hat:$subchild $common ${fifo}:${okchild}

runchecktest "NAMED PIPE W (parent & child subprofiles)" fail ${subparent} ${subchild} ${fifo}

# PIPE - in separate subprofiles - bad access for child

genprofile hat:$subparent $common ${fifo}:${okparent} \
	   hat:$subchild $common ${fifo}:${badchild}

runchecktest "NAMED PIPE bad child (parent & child subprofiles)" fail ${subparent} ${subchild} ${fifo}

# PIPE - in separate subprofiles - bad access for parent

genprofile hat:$subparent $common ${fifo}:${badparent} \
	   hat:$subchild $common ${fifo}:${okchild}

runchecktest "NAMED PIPE bad parent (parent & child subprofiles)" fail ${subparent} ${subchild} ${fifo}
