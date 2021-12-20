#! /bin/bash
#	Copyright (C) 2015 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME aa_policy_cache
#=DESCRIPTION
# This test verifies that the aa_policy_cache API works as expected.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

cachedir=$tmpdir/cache
policies=$(echo aa_policy_cache_test_{0001..1024})

create_cachedir()
{
	mkdir -p "$cachedir"
}

remove_cachedir()
{
	if [ -n "$cachedir" ]
	then
		rm -rf "$cachedir"
	fi
}

create_empty_cache()
{
	$test new --max-caches 1 "$cachedir" > /dev/null
}

create_cache_files()
{
	local cachefile

	create_cachedir
	for policy in $policies
	do
		cachefile="${cachedir}/${policy}"

		echo "profile $policy { /f r, }" | ${subdomain} -qS > "$cachefile"
	done
}

install_bad_features_file()
{
	echo "file {\n}\n" > "${cachedir}/.features"
}

remove_features_file()
{
	if [ -n "$cachedir" ]
	then
		rm -f "${cachedir}/.features"
	fi
}

verify_policies_are_not_loaded()
{
	for policy in $policies
	do
		if grep -q "^policy " /sys/kernel/security/apparmor/profiles
		then
			fatalerror "Policy \"${policy}\" must not be loaded"
			return
		fi
	done
}

runchecktest_policies_are_loaded()
{
	for policy in $policies
	do
		if ! grep -q "^$policy (enforce)" /sys/kernel/security/apparmor/profiles
		then
			echo "Error: Policy \"${policy}\" was not loaded"
			testfailed
			return
		fi
	done
}

runchecktest_remove_policies()
{
	for policy in $policies
	do
		runchecktest "AA_POLICY_CACHE remove-policy ($policy)" pass remove-policy "$policy"
	done
}

# IMPORTANT: These tests build on themselves so the first failing test can
# cause many failures

runchecktest "AA_POLICY_CACHE new (no cachedir)" fail new "$cachedir"
create_cachedir
runchecktest "AA_POLICY_CACHE new (no .features)" fail new "$cachedir"
remove_cachedir
runchecktest "AA_POLICY_CACHE new create (no cachedir)" pass new --max-caches 1 "$cachedir"
runchecktest "AA_POLICY_CACHE new create (existing cache)" pass new --max-caches 1 "$cachedir"
runchecktest "AA_POLICY_CACHE new (existing cache)" pass new "$cachedir"

install_bad_features_file
runchecktest "AA_POLICY_CACHE new (bad .features)" fail new "$cachedir"
runchecktest "AA_POLICY_CACHE new create (bad .features)" pass new --max-caches 1 "$cachedir"

# Make sure that no test policies are already loaded
verify_policies_are_not_loaded

remove_cachedir
runchecktest "AA_POLICY_CACHE replace-all (no cachedir)" fail replace-all "$cachedir"
create_cachedir
runchecktest "AA_POLICY_CACHE replace-all (no .features)" fail replace-all "$cachedir"
create_empty_cache
runchecktest "AA_POLICY_CACHE replace-all (empty cache)" pass replace-all "$cachedir"
create_cache_files
runchecktest "AA_POLICY_CACHE replace-all (full cache)" pass replace-all "$cachedir"

# Test that the previous policy load was successful 
runchecktest_policies_are_loaded

runchecktest "AA_POLICY_CACHE remove-policy (DNE)" fail remove-policy "aa_policy_cache_test_DNE"
runchecktest_remove_policies

runchecktest "AA_POLICY_CACHE remove (full cache)" pass remove "$cachedir"
runchecktest "AA_POLICY_CACHE remove (no .features)" pass remove "$cachedir"
install_bad_features_file
runchecktest "AA_POLICY_CACHE remove (empty cache)" pass remove "$cachedir"
remove_cachedir
runchecktest "AA_POLICY_CACHE remove (DNE)" fail remove "$cachedir"
