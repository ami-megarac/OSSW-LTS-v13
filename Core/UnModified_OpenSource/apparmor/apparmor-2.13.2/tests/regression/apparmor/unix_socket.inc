# Copyright (C) 2014 Canonical, Ltd.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, contact Canonical Ltd.

client=$bin/unix_socket_client
message=4a0c83d87aaa7afa2baab5df3ee4df630f0046d5bfb7a3080c550b721f401b3b\
8a738e1435a3b77aa6482a70fb51c44f20007221b85541b0184de66344d46a4c

do_test()
{
	local addr_type="$1" # abstract or unnamed
	local test_prog="$2" # server or client
	local l_u_access="$3" # optional local unbound perms
	local l_b_access="$4" # local bound perms
	local type="$5" # stream, dgram, or seqpacket
	local addr="$6" # optional socket address
	local p_access="$7" # peer perms
	local p_label="$8" # peer socket label
	local p_addr="$9" # optional peer socket address
	local bad_type="${10}"
	local bad_addr="${11}" # optional
	local bad_p_label="${12}"
	local bad_p_addr="${13}" # optional

	local desc="AF_UNIX $addr_type socket ($type);"
	local l_access # combind local perms: local bound and local unbound
	local c_access # combined perms: local bound, local unbound, and peer
	local access # used as an iterator
	local u_rule # rule for pre-bind accesses
	local u_type_rule # rule for pre-bind accesses with a type conditional
	local genprof="genprofile"
	local args

	# this is a workaround for
	# https://bugs.launchpad.net/ubuntu/+source/linux/+bug/1373172
	local p_ex_result="pass"
	if [ "$addr_type" == "unnamed" ] && [ "$test_prog" == "server" ] &&
	   [ "$type" == "dgram" ] ; then
        	p_ex_result="xpass"
	fi

	if [ "$test_prog" == "server" ]; then
		genprof+=" $client:Ux"
		args="$addr $type $message $client"
	elif [ "$test_prog" == "client" ]; then
		genprof+=" unix:ALL $client:px -- image=$client"
		args="$p_addr $type $message $client"
	fi

	l_access="$l_b_access"
	if [ -n "$l_u_access" ]; then
		l_access="${l_u_access},${l_access}"
		c_access="${l_u_access},${c_access}"

		u_rule="unix:($l_u_access)"
		u_type_rule="${u_rule}:type=$type"
	fi
	c_access="${l_access},${p_access}"

	runchecktest "$desc unconfined $test_prog" pass $args

	desc+=" confined $test_prog"

	$genprof "unix:ALL"
	runchecktest "$desc (implicit perms)" pass $args

	$genprof "unix:($c_access)"
	runchecktest "$desc (explicit perms)" pass $args

	$genprof "unix:($c_access):type=$type"
	runchecktest "$desc (type)" pass $args

	if [ -n "$addr" ]; then
		$genprof $u_rule "unix:(${l_b_access},${p_access}):addr=$addr"
		runchecktest "$desc (addr)" pass $args
	fi

	$genprof "unix:($l_access)" "unix::peer=(label=$p_label)"
	runchecktest "$desc (peer label w/ implicit perms)" ${p_ex_result} $args

	$genprof "unix:($l_access)" "unix:($p_access):peer=(label=$p_label)"
	runchecktest "$desc (peer label w/ explicit perms)" ${p_ex_result} $args

	if [ -n "$p_addr" ]; then
		$genprof "unix:($l_access)" "unix:($p_access):peer=(addr=$p_addr)"
		runchecktest "$desc (peer addr)" pass $args

		$genprof "unix:($l_access)" "unix:($p_access):peer=(label=$p_label addr=$p_addr)"
		runchecktest "$desc (peer label, peer addr)" ${p_ex_result} $args

		$genprof "unix:($l_access):type=$type" "unix:($p_access):type=$type:peer=(label=$p_label addr=$p_addr)"
		runchecktest "$desc (type, peer label, peer addr)" ${p_ex_result} $args
	fi

	if [ -n "$addr" ]; then
		$genprof $u_type_rule "unix:($l_b_access):type=$type:addr=$addr" "unix:($p_access):type=$type:addr=$addr"
		runchecktest "$desc (type, addr)" pass $args

		$genprof $u_type_rule "unix:($l_b_access):type=$type:addr=$addr" "unix:($p_access):type=$type:addr=$addr:peer=(label=$p_label)"
		runchecktest "$desc (type, addr, peer label)" ${p_ex_result} $args
	fi

	if [ -n "$addr" -a -n "$p_addr" ]; then
		$genprof $u_type_rule "unix:($l_b_access):type=$type:addr=$addr" "unix:($p_access):type=$type:addr=$addr:peer=(label=$p_label addr=$p_addr)"
		runchecktest "$desc (type, addr, peer label, peer addr)" ${p_ex_result} $args
	fi

	$genprof
	runchecktest "$desc (no unix rule)" fail $args

	for access in ${c_access//,/ }; do
		$genprof "unix:(${c_access//$access/})"
		runchecktest "$desc (missing perm: $access)" fail $args
	done

	$genprof "unix:($c_access):type=$bad_type"
	runchecktest "$desc (bad type)" fail $args

	if [ -n "$bad_addr" ]; then
		$genprof $u_rule "unix:(${l_b_access},${p_access}):addr=$bad_addr"
		runchecktest "$desc (bad addr)" fail $args
	fi

	$genprof "unix:($l_access)" "unix:($p_access):peer=(label=$bad_p_label)"
	runchecktest "$desc (bad peer label)" fail $args

	if [ -n "$bad_p_addr" ]; then
		$genprof "unix:($l_access)" "unix:($p_access):peer=(addr=$bad_p_addr)"
		runchecktest "$desc (bad peer addr)" fail $args
	fi

	removeprofile
}
