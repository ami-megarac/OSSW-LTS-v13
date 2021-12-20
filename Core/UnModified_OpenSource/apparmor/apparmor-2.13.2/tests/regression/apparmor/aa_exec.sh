#! /bin/bash
#	Copyright (C) 2015 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME aa_exec
#=DESCRIPTION
# This test verifies that the aa_exec command is indeed transitioning
# profiles as intended.
#=END

#set -x

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

ns=aa_exec_ns

genprofile_aa_exec()
{
	mode=""
	if [ $# -eq 2 ]; then
		if [ $2 -ne 0 ]; then
			mode="(complain) "
		fi
	fi
	genprofile --stdin <<EOF
$1 ${mode}{
  file,
}

:${ns}:${1} ${mode}{
  file,
}
EOF
}

settest aa_exec_profile ${bin}/aa_exec_wrapper.sh

genprofile_aa_exec "$test" 0
runchecktest "unconfined" pass "$aa_exec" "unconfined"

genprofile_aa_exec "$test" 0
runchecktest "enforce" pass "$aa_exec -p $test" "$test (enforce)"

genprofile_aa_exec "$test" 1
runchecktest "complain" pass "$aa_exec -p $test" "$test (complain)"

genprofile_aa_exec "$test" 0
runchecktest "negative test: not unconfined" fail "$aa_exec -p $test" "unconfined"

genprofile_aa_exec "$test" 0
runchecktest "negative test: bad mode: (complain)" fail "$aa_exec -p $test" "$test (complain)"

genprofile_aa_exec "$test" 0
runchecktest "negative test: bad mode: (enforceXXX)" fail "$aa_exec -p $test" "$test (enforceXXX)"

genprofile_aa_exec "$test" 0
runchecktest "enforce (--immediate)" pass "$aa_exec -i -p $test" "$test (enforce)"

genprofile_aa_exec "$test" 1
runchecktest "complain (--immediate)" pass "$aa_exec -p $test" "$test (complain)"

genprofile_aa_exec "$test" 0
runchecktest "negative test: bad profile (--immediate)" fail "$aa_exec -ip $test" "${test}XXX (enforce)"

genprofile_aa_exec "$test" 0
runchecktest "enforce (--namespace=${ns})" pass "$aa_exec -n $ns -p $test" "$test (enforce)"

genprofile_aa_exec "$test" 1
runchecktest "complain (--namespace=${ns})" pass "$aa_exec -n $ns -p $test" "$test (complain)"

genprofile_aa_exec "$test" 0
runchecktest "negative test: bad ns (--namespace=${ns}XXX)" fail "$aa_exec -n ${ns}XXX -p $test" "$test (enforce)"
