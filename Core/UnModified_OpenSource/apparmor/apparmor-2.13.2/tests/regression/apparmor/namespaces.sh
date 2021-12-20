#! /bin/bash
#	Copyright (C) 2016 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME namespaces
#=DESCRIPTION
# Verifies basic namespace functionality
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc
requires_namespace_interface

# unique_ns - Print a randomly generated, unused namespace identifier to stdout
unique_ns() {
	# racy way of generating a namespace name that is likely to be unique
	local ns=$(mktemp --dry-run -dp /sys/kernel/security/apparmor/policy/namespaces -t test_namespaces_XXXXXX)
	basename "$ns"
}

# genprofile_ns - Generate and load a profile using a randomly generated namespace
# $1: The profile name to use (without a namespace)
# $2: Non-zero if the 'profile' keyword should be prefixed to the declaration
#
# Returns the randomly generated namespace that the profile was loaded into
genprofile_ns() {
	local prefix=""
	local ns=$(unique_ns)
	local prof=$1

	if [ $2 -ne 0 ]; then
		prefix="profile "
	fi

	# override the sys_profiles variable with a bad path so that genprofile
	# doesn't perform profile load checking in the wrong policy namespace
	echo "${prefix}:${ns}:${prof} {}" | sys_profiles="${sys_profiles}XXX" genprofile --stdin
	echo "$ns"
}

# genprofile_ns_and_verify - Generate and load a profile into a namespace and
#                            verify the creation of the profile and namespace
# $1: A description of this test
# $2: Non-zero if the 'profile' keyword should be prefixed to the declaration
genprofile_ns_and_verify() {
	local desc=$1
	local prof="p"
	local ns=$(genprofile_ns "$prof" $2)


	[ -d /sys/kernel/security/apparmor/policy/namespaces/${ns} ]
	local dir_created=$?
	[ -d /sys/kernel/security/apparmor/policy/namespaces/${ns}/profiles/${prof}* ]
	local prof_created=$?
	removeprofile
	if [ $dir_created -ne 0 ]; then
		echo "Error: ${testname} failed. Test '${desc}' did not create the expected namespace directory in apparmorfs: policy/namespaces/${ns}"
		testfailed
	elif [ $prof_created -ne 0 ]; then
		echo "Error: ${testname} failed. Test '${desc}' did not create the expected namespaced profile directory in apparmorfs: policy/namespaces/${ns}/profiles/${prof}"
		testfailed
	elif [ -n "$VERBOSE" ]; then
		echo "ok: ${desc}"
	fi
}

genprofile_ns_and_verify "NAMESPACES create unique ns (w/o profile keyword prefix)" 0
genprofile_ns_and_verify "NAMESPACES create unique ns (w/ profile keyword prefix)" 1
