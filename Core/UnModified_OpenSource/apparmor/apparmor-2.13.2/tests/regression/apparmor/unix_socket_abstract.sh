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

#=NAME unix_socket_abstract
#=DESCRIPTION
# This tests access to abstract unix domain sockets. The server opens a socket,
# forks a client with it's own profile, sends a message to the client over the
# socket, and sees what happens.
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

addr=@aa_sock
client_addr=${addr}.client

# Test abstract stream server and client
do_test "abstract" \
	"server" \
	"create,setopt" \
	"bind,listen,getopt,shutdown" \
	stream \
	"$addr" \
	"accept,read,write" \
	"unconfined" \
	"" \
	dgram \
	"${addr}XXX" \
	"XXX" \
	""
do_test "abstract" \
	"client" \
	"" \
	"create,getopt,setopt,getattr" \
	stream \
	"" \
	"connect,write,read" \
	"$test" \
	"$addr" \
	seqpacket \
	"" \
	"${test}XXX" \
	"${addr}XXX"

# Test abstract dgram server and client
do_test "abstract" \
	"server" \
	"create,setopt" \
	"bind,getopt,shutdown" \
	dgram \
	"$addr" \
	"read,write" \
	"unconfined" \
	"$client_addr" \
	seqpacket \
	"${addr}XXX" \
	"XXX" \
	"${client_addr}XXX"
do_test "abstract" \
	"client" \
	"create,setopt,getattr" \
	"bind,getopt" \
	dgram \
	"$client_addr" \
	"write,read" \
	"$test" \
	"$addr" \
	stream \
	"${client_addr}XXX" \
	"${test}XXX" \
	"${addr}XXX"

# Test abstract seqpacket server and client
do_test "abstract" \
	"server" \
	"create,setopt" \
	"bind,listen,getopt,shutdown" \
	seqpacket \
	"$addr" \
	"accept,read,write" \
	"unconfined" \
	"" \
	stream \
	"${addr}XXX" \
	"XXX" \
	""
do_test "abstract" \
	"client" \
	"" \
	"create,getopt,setopt,getattr" \
	seqpacket \
	"" \
	"connect,write,read" \
	"$test" \
	"$addr" \
	dgram \
	"" \
	"${test}XXX" \
	"${addr}XXX"
