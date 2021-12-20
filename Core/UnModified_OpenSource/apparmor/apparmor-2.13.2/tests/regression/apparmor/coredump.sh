#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#	Copyright (C) 2010 Canonical, Ltd
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME coredump
#=DESCRIPTION coredump test

cleancorefile()
{
	rm -f "$tmpdir/core.$_pid"
}

checkcorefile()
{
	# global _testdesc _pfmode _known outfile
	if [ ${1:0:1} == "x" ] ; then
		requirement=${1#x}
		_known=" (known problem)"
        else
		requirement=$1
		_known=""
        fi

	#check pid of last test run by the test suite
	if [ -f "$tmpdir/core.$_pid" ]
	then
		_corefile=yes
	else
		_corefile=no
	fi

	if [ "$requirement" = "yes" -a "$_corefile" = "no" ] ; then
		if [ -n "$_known" ] ; then
			echo -n "XFAIL: "
		fi
		echo "Error: corefile expected but not present - $2"
		if [ -z "$_known" ] ; then
			cat $profile
			testfailed
		fi
	elif [ "$requirement" = "no" -a "$_corefile"  = "yes" ] ; then
		if [ -n "$_known" ] ; then
			echo -n "XFAIL: "
		fi
		echo "Error: corefile present when not expected -- $2"
		if [ -z "$_known" ] ; then
			cat $profile
			testfailed
		fi
	fi

	unset _corefile
	cleancorefile
}

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

coreperm=r
nocoreperm=ix

# enable coredumps
ulimit -c 1000000

# set the core_pattern so we can reliably check for the expected cores
#echo -n "core dump pattern: " ; cat /proc/sys/kernel/core_pattern
dumppattern=`cat /proc/sys/kernel/core_pattern`
echo "$tmpdir/core.%p" > /proc/sys/kernel/core_pattern
#echo -n "set core patter to: " ; cat /proc/sys/kernel/core_pattern

cleancorefile
checkcorefile no "COREDUMP (starting with clean slate)"

# PASS TEST, no confinement
cleancorefile
echo "*** A 'Segmentation Fault' message from bash is expected for the following test"
runchecktest "COREDUMP (no confinement)" signal11
checkcorefile yes "COREDUMP (no confinement)"

# FAIL TEST, with r confinement, no permission to write core file
cleancorefile
genprofile -I $test:$coreperm

echo
echo "*** A 'Segmentation Fault' message from bash is expected for the following test"
runchecktest "COREDUMP ($coreperm confinement)" signal11
checkcorefile no "COREDUMP ($coreperm confinement)"

# PASS TEST, with r confinement, permission to write core file
cleancorefile
genprofile -I $test:$coreperm $tmpdir/core.*:w

echo
echo "*** A 'Segmentation Fault' message from bash is expected for the following test"
runchecktest "COREDUMP ($coreperm confinement)" signal11
checkcorefile yes "COREDUMP ($coreperm confinement)"

# FAIL TEST, with x confinement, no permission to write core file
cleancorefile
genprofile -I $test:$nocoreperm 

echo
echo "*** A 'Segmentation Fault' message from bash is expected for the following test"
runchecktest "COREDUMP ($nocoreperm confinement)" signal11
checkcorefile no "COREDUMP ($nocoreperm confinement)"

# FAIL TEST, with x confinement, permission to write core file
# should fail because of no read permission on executable (see man 5 core)
cleancorefile
genprofile -I $test:$nocoreperm $tmpdir/core.*:w

echo
echo "*** A 'Segmentation Fault' message from bash is expected for the following test"
runchecktest "COREDUMP ($nocoreperm confinement)" signal11
checkcorefile xno "COREDUMP ($nocoreperm confinement)"




#restore core dump pattern
echo "$dumppattern" > /proc/sys/kernel/core_pattern
