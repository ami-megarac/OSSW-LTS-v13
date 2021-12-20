#! /bin/bash
#	Copyright (C) 2013 Canonical, Ltd.
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME query_label
#=DESCRIPTION
# This test verifies the results returned from aa_query_label()
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc
requires_query_interface

settest query_label

expect=""
perms=""
qprof="/profile/to/query"
label="--label=$qprof"

dbus_send="--dbus=send"
dbus_receive="--dbus=receive"
dbus_bind="--dbus=bind"
dbus_send_receive="--dbus=send,receive"
dbus_all="--dbus=send,receive,bind"
dbus_none="--dbus="

dbus_msg_query="session com.foo.bar /usr/bin/bar /com/foo/bar com.foo.bar Method"
dbus_svc_query="session com.foo.baz"

# Generate a profile for $test, granting all file accesses, and $qprof,
# granting anything specified in $@.
genqueryprofile()
{
	genprofile --stdin <<EOF
$test {
  file,
}

$qprof {
  $@
}
EOF
}

# Generate the --expect parameter for query_label. Example usage:
# `expect audit` would generate --expect=audit
# `expect allow audit` would generate --expect=allow,audit
# `expect` would generate --expect=
expect()
{
	local old=$IFS

	IFS=","
	expect=$(printf "%s=%s" "--expect" "$*")
	IFS=$old
}

# Generate the --CLASS=PERMS parameter query_label. Example usage:
# `perms dbus send` would generate --dbus=send
# `perms dbus send bind` would generate --dbus=send,bind
# `perms dbus` would generate --dbus=
perms()
{
	local old=$IFS
	local class=$1

	shift
	IFS=","
	perms=$(printf "%s%s=%s" "--" "$class" "$*")
	IFS=$old
}

# Gather up the globals ($expect, $label, $perms) and call runchecktest
# @1: the test description
# @2: pass or fail
# @3: the query string
querytest()
{
	local desc=$1
	local pf=$2

	shift
	shift
	runchecktest "$desc" "$pf" "$expect" "$label" "$perms" $*
}

if [ "$(kernel_features dbus)" == "true" ]; then
    # Check querying of a label that the kernel doesn't know about
    # aa_query_label() should return an error
    expect anything
    perms dbus send
    querytest "QUERY no profile loaded" fail $dbus_msg_query

    # Check querying with an empty mask - aa_query_label() should error out
    genqueryprofile "dbus,"
    expect anything
    perms dbus # no perms
    querytest "QUERY empty mask" fail $dbus_msg_query

    # Check dbus - allowed without auditing
    genqueryprofile "dbus,"
    expect allow
    perms dbus send
    querytest "QUERY dbus (msg send)" pass $dbus_msg_query
    perms dbus receive
    querytest "QUERY dbus (msg receive)" pass $dbus_msg_query
    perms dbus send receive
    querytest "QUERY dbus (msg send & receive)" pass $dbus_msg_query
    perms dbus bind
    querytest "QUERY dbus (svc)" pass $dbus_svc_query

    # Check deny dbus - denied without auditing
    genqueryprofile "deny dbus,"
    expect # neither allow, nor audit
    perms dbus send
    querytest "QUERY deny dbus (msg send)" pass $dbus_msg_query
    perms dbus receive
    querytest "QUERY deny dbus (msg receive)" pass $dbus_msg_query
    perms dbus send receive
    querytest "QUERY deny dbus (msg send & receive)" pass $dbus_msg_query
    perms dbus bind
    querytest "QUERY deny dbus (svc)" pass $dbus_svc_query

    # Check audit dbus - allowed, but audited
    genqueryprofile "audit dbus,"
    expect allow audit
    perms dbus send
    querytest "QUERY audit dbus (msg send)" pass $dbus_msg_query
    perms dbus receive
    querytest "QUERY audit dbus (msg receive)" pass $dbus_msg_query
    perms dbus send receive
    querytest "QUERY audit dbus (msg send & receive)" pass $dbus_msg_query
    perms dbus bind
    querytest "QUERY audit dbus (svc)" pass $dbus_svc_query

    # Check audit deny dbus - explicit deny without auditing
    genqueryprofile "audit deny dbus,"
    expect audit
    perms dbus send
    querytest "QUERY audit deny dbus (msg send)" pass $dbus_msg_query
    perms dbus receive
    querytest "QUERY audit deny dbus (msg receive)" pass $dbus_msg_query
    perms dbus send receive
    querytest "QUERY audit deny dbus (msg send & receive)" pass $dbus_msg_query
    perms dbus bind
    querytest "QUERY audit deny dbus (svc)" pass $dbus_svc_query

    # Check dbus send - ensure that receive and bind bits aren't set
    genqueryprofile "dbus send,"
    expect allow
    perms dbus send
    querytest "QUERY dbus send (msg send)" pass $dbus_msg_query
    perms dbus receive
    querytest "QUERY dbus send (msg receive)" fail $dbus_msg_query
    perms dbus send receive
    querytest "QUERY dbus send (msg send & receive)" fail $dbus_msg_query
    perms dbus bind
    querytest "QUERY dbus send (msg bind)" fail $dbus_msg_query
    perms dbus send bind
    querytest "QUERY dbus send (msg send & bind)" fail $dbus_msg_query

    # Check dbus receive - ensure that send and bind bits aren't set
    genqueryprofile "dbus receive,"
    expect allow
    perms dbus receive
    querytest "QUERY dbus receive (msg receive)" pass $dbus_msg_query
    perms dbus send
    querytest "QUERY dbus receive (msg send)" fail $dbus_msg_query
    perms dbus send receive
    querytest "QUERY dbus receive (msg send & receive)" fail $dbus_msg_query
    perms dbus bind
    querytest "QUERY dbus receive (msg bind)" fail $dbus_msg_query
    perms dbus receive bind
    querytest "QUERY dbus receive (msg receive & bind)" fail $dbus_msg_query

    # Check dbus bind - ensure that send and receive bits aren't set
    genqueryprofile "dbus bind,"
    expect allow
    perms dbus bind
    querytest "QUERY dbus bind (svc bind)" pass $dbus_svc_query
    perms dbus send
    querytest "QUERY dbus bind (svc send)" fail $dbus_svc_query
    perms dbus send bind
    querytest "QUERY dbus bind (svc send & bind)" fail $dbus_svc_query
    perms dbus receive
    querytest "QUERY dbus bind (svc receive)" fail $dbus_svc_query
    perms dbus receive bind
    querytest "QUERY dbus bind (svc receive & bind)" fail $dbus_svc_query

    # Check dbus - ensure that send and receive bits aren't set in service queries
    # and the bind bit isn't set in message queries
    genqueryprofile "dbus,"
    expect allow
    perms dbus send receive
    querytest "QUERY dbus (msg send & receive)" pass $dbus_msg_query
    perms dbus bind
    querytest "QUERY dbus (msg bind)" fail $dbus_msg_query
    perms dbus bind
    querytest "QUERY dbus (svc bind)" pass $dbus_svc_query
    perms dbus send
    querytest "QUERY dbus (svc send)" fail $dbus_svc_query
    perms dbus receive
    querytest "QUERY dbus (svc receive)" fail $dbus_svc_query
else
    echo "	required feature dbus missing, skipping dbus queries ..."
fi

genqueryprofile "file,"
expect allow
perms file exec,write,read,append,create,delete,setattr,getattr,chmod,chown,link,linksubset,lock,exec_mmap
if [ "$(kernel_features query/label/multi_transaction)" == "true" ] ; then
    querytest "QUERY file (all base perms #1)" pass /anything
    querytest "QUERY file (all base perms #2)" pass /everything
else
    querytest "QUERY file (all base perms #1)" xpass /anything
    querytest "QUERY file (all base perms #2)" xpass /everything
fi

genqueryprofile "/etc/passwd r,"
expect allow
perms file read
querytest "QUERY file (passwd)" pass /etc/passwd
querytest "QUERY file (passwd bad path #1)" fail /etc/pass
querytest "QUERY file (passwd bad path #2)" fail /etc/passwdXXX
querytest "QUERY file (passwd bad path #3)" fail /etc/passwd/XXX
perms file write
querytest "QUERY file (passwd bad perms #1)" fail /etc/passwd
perms file read,write
querytest "QUERY file (passwd bad perms #2)" fail /etc/passwd

genqueryprofile "/tmp/ rw,"
expect allow
perms file read,write
querytest "QUERY file (/tmp/)" pass /tmp/
querytest "QUERY file (/tmp/ bad path)" fail /tmp
querytest "QUERY file (/tmp/ bad path)" fail /tmp/tmp/
perms file read
querytest "QUERY file (/tmp/ read only)" pass /tmp/
perms file write
querytest "QUERY file (/tmp/ write only)" pass /tmp/
expect audit
perms file read,write
querytest "QUERY file (/tmp/ wrong dir)" pass /etc/
