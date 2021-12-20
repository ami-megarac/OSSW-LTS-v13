#! /bin/bash
#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME tcp
#=DESCRIPTION a series of tests for tcp/netdomain.

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc
requires_kernel_features network

port=34567
ip="127.0.0.1"
#badperm1=r
#badperm2=w

# PASS TEST - no apparmor rules
runchecktest "TCP (no apparmor)" pass $port

# FAIL TEST - no network rules
genprofile 
runchecktest "TCP (accept, connect) no network rules" fail $port

# PASS TEST - allow tcp
genprofile network:tcp
runchecktest "TCP (accept, connect) allow tcp" pass $port

# PASS TEST - allow inet
genprofile network:inet
runchecktest "TCP (accept, connect) allow inet" pass $port

# PASS TEST - allow inet stream
genprofile "network:inet stream"
runchecktest "TCP (accept, connect) allow inet stream" pass $port

# PASS TEST - simple / low-numbered port
# you damn well better not be running telnet
genprofile network:inet cap:net_bind_service
runchecktest "TCP (accept, connect) low numbered port/bind cap" pass 23

# FAIL TEST - simple / low-numbered port
# will always fail unless process has net_bind_service capability.
# you damn well better not be running telnetd.
genprofile network:inet 
runchecktest "TCP (accept, connect) low numbered port/no bind cap" fail 23

# FAIL TEST - make sure that unspec doesn't match
genprofile network:unspec
runchecktest "TCP (accept, connect) wrong socket family" fail 23

exit 0

# PASS TEST - accept via interface
genprofile tcp_accept:via:lo tcp_connect:
runchecktest "TCP (accept, connect)" pass $port

# PASS TEST - accept to ip addr
genprofile tcp_accept:to:${ip} tcp_connect:
runchecktest "TCP (accept, connect)" pass $port

# PASS TEST - accept to ip addr + cidr
genprofile tcp_accept:to:127.0.0.0/24 tcp_connect:
runchecktest "TCP (accept, connect)" pass $port

# PASS TEST - accept to ip addr + netmask
genprofile tcp_accept:to:127.0.0.0/255.255.255.0 tcp_connect:
runchecktest "TCP (accept, connect)" pass $port

# PASS TEST - accept to ip addr:port
genprofile tcp_accept:to:${ip}::${port} tcp_connect:
runchecktest "TCP (accept, connect)" pass $port

# PASS TEST - accept to ip addr/cidr:port
genprofile tcp_accept:to:127.0.0.0/24::${port} tcp_connect:
runchecktest "TCP (accept, connect)" pass $port

# PASS TEST - accept to ip addr/mask:port
genprofile tcp_accept:to:127.0.0.0/255.255.192.0::${port} tcp_connect:
runchecktest "TCP (accept, connect)" pass $port

# PASS TEST - simple / low-numbered port
# will always fail unless process has net_bind_service capability.
# you damn well better not be running telnetd.
genprofile tcp_accept: tcp_connect: cap:net_bind_service
runchecktest "TCP (accept, connect, port 23)" pass 23

# The following tests will FAIL only if netdomain is enabled. If
# netdomain is disabled, they are expected to pass. netdomain is
# disabled for the SHASS 1.1 release. FIXME - sure would be nice to
# detect this programmatically.
#EXPECTED=fail
EXPECTED=pass

# FAIL TEST - needs tcp_connect
genprofile tcp_accept: 
runchecktest "TCP (accept)" ${EXPECTED} $port

# FAIL TEST - needs tcp_accept
genprofile tcp_connect: 
runchecktest "TCP (connect)" ${EXPECTED} $port

