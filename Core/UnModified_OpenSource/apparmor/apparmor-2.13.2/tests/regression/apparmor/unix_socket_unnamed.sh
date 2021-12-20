#! /bin/bash
#
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

#=NAME unix_socket_unnamed
#=DESCRIPTION
# This tests access to unnamed unix domain sockets. The server opens a socket,
# forks a client with it's own profile, passes an fd across exec, sends a
# message to the client over the socket pair, and sees what happens.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc
. $bin/unix_socket.inc
requires_kernel_features policy/versions/v7
requires_kernel_features network/af_unix
requires_parser_support "unix,"

settest unix_socket

addr=none
client_addr=none

# Test unnamed stream server and client
do_test "unnamed" \
	"server" \
	"" \
	"create,getopt,setopt,shutdown" \
	stream \
	"$addr" \
	"read,write" \
	"$test" \
	"" \
	dgram \
	"@none" \
	"${test}XXX" \
	""
do_test "unnamed" \
	"client" \
	"" \
	"getopt,setopt,getattr" \
	stream \
	"" \
	"write,read" \
	"$test" \
	"$addr" \
	seqpacket \
	"" \
	"${test}XXX" \
	"@none"

# Test unnamed dgram server and client
do_test "unnamed" \
	"server" \
	"" \
	"create,getopt,setopt,shutdown" \
	dgram \
	"$addr" \
	"read,write" \
	"$test" \
	"$client_addr" \
	seqpacket \
	"@none" \
	"${test}XXX" \
	"@none"
do_test "unnamed" \
	"client" \
	"" \
	"getopt,setopt,getattr" \
	dgram \
	"$client_addr" \
	"write,read" \
	"$test" \
	"$addr" \
	stream \
	"@none" \
	"${test}XXX" \
	"@none"

# Test unnamed seqpacket server and client
do_test "unnamed" \
	"server" \
	"" \
	"create,getopt,setopt,shutdown" \
	seqpacket \
	"$addr" \
	"read,write" \
	"$test" \
	"" \
	stream \
	"@none" \
	"${test}XXX" \
	""
do_test "unnamed" \
	"client" \
	"" \
	"getopt,setopt,getattr" \
	seqpacket \
	"" \
	"write,read" \
	"$test" \
	"$addr" \
	dgram \
	"" \
	"${test}XXX" \
	"@none"
