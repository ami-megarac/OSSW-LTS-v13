#!/usr/bin/perl
#
#   Copyright (c) 2013
#   Canonical, Ltd. (All rights reserved)
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of version 2 of the GNU General Public
#   License published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, contact Canonical Ltd.
#

use strict;
use Locale::gettext;
use POSIX;

setlocale(LC_MESSAGES, "");

my $count=0;

my $prefix="simple_tests/generated_dbus";

my @quantifier = ("", "deny", "audit");
my @session = ("", "bus=session", "bus=system", "bus=accessibility");
my @path = ("", "path=/foo/bar", "path=\"/foo/bar\"");
my @interface = ("", "interface=com.baz", "interface=\"com.baz\"");
my @member = ("", "member=bar", "member=\"bar\"");

my @name = ("", "name=com.foo", "name=\"com.foo\"");
my @peer = map { "peer=($_)" } (@name, "label=/usr/bin/app",
				"label=\"/usr/bin/app\"",
				"name=com.foo label=/usr/bin/app",
				"name=\"com.foo\" label=\"/usr/bin/app\"");

# @msg_perms are the permissions that are related to sending and receiving
# messages. @svc_perms are the permissions related to services.
my @base_msg_perms = ("r", "w", "rw", "read", "receive", "write", "send");
my @msg_perms = ("", @base_msg_perms, (map { "($_)" } @base_msg_perms),
		 "(write, read)", "(send receive)", "(send read)",
		 "(receive write)");

gen_files("message-rules", "PASS", \@quantifier, \@msg_perms, \@session,
	  [""], \@path, \@interface, \@member, \@peer);
gen_files("service-rules", "PASS", \@quantifier, ["bind"], \@session,
	  \@name, [""], [""], [""], [""]);
gen_files("eavesdrop-rules", "PASS", \@quantifier, ["eavesdrop"], \@session,
	  [""], [""], [""], [""], [""]);
gen_file("sloppy-formatting", "PASS", "", "(send , receive )", "bus=session",
	 "", "path =\"/foo/bar\"", "interface = com.foo", "  member=bar",
	 "peer =(   label= /usr/bin/app name  =\"com.foo\")");
gen_file("sloppy-formatting", "PASS", "", "bind", "bus =session",
	 "name= com.foo", "", "", "", "");
gen_file("sloppy-formatting", "PASS", "", "eavesdrop", "bus = system",
	 "", "", "", "", "");

# Don't use the first element, which is empty, from each array since all empty
# conditionals would PASS but we want all FAILs
shift @msg_perms;
shift @name;
shift @path;
shift @interface;
shift @member;
shift @peer;
gen_files("message-incompat", "FAIL", \@quantifier, \@msg_perms, \@session,
	  \@name, [""], [""], [""], [""]);
gen_files("service-incompat", "FAIL", \@quantifier, ["bind"], \@session,
	  \@name, \@path, [""], [""], [""]);
gen_files("service-incompat", "FAIL", \@quantifier, ["bind"], \@session,
	  \@name, [""], \@interface, [""], [""]);
gen_files("service-incompat", "FAIL", \@quantifier, ["bind"], \@session,
	  \@name, [""], [""], \@member, [""]);
gen_files("service-incompat", "FAIL", \@quantifier, ["bind"], \@session,
	  \@name, [""], [""], [""], \@peer);
gen_files("eavesdrop-incompat", "FAIL", \@quantifier, ["eavesdrop"], \@session,
	  \@name, \@path, \@interface, \@member, \@peer);

gen_files("pairing-unsupported", "FAIL", \@quantifier, ["send", "bind"],
	  \@session, ["name=sn", "label=sl"], [""], [""], [""],
	  ["peer=(name=pn)", "peer=(label=pl)"]);

# missing bus= prefix
gen_file("bad-formatting", "FAIL", "", "send", "session", "", "", "", "", "");
# incorrectly formatted permissions
gen_files("bad-perms", "FAIL", [""], ["send receive", "(send", "send)"],
	  ["bus=session"], [""], [""], [""], [""], [""]);
# invalid permissions
gen_files("bad-perms", "FAIL", [""],
	  ["a", "x", "Ux", "ix", "m", "k", "l", "(a)", "(x)"], [""], [""],
	  [""], [""], [""], [""]);

gen_file("duplicated-conditionals", "FAIL", "", "bus=1 bus=2");
gen_file("duplicated-conditionals", "FAIL", "", "name=1 name=2");
gen_file("duplicated-conditionals", "FAIL", "", "path=1 path=2");
gen_file("duplicated-conditionals", "FAIL", "", "interface=1 interface=2");
gen_file("duplicated-conditionals", "FAIL", "", "member=1 member=2");
gen_file("duplicated-conditionals", "FAIL", "", "peer=(name=1) peer=(name=2)");
gen_file("duplicated-conditionals", "FAIL", "", "peer=(label=1) peer=(label=2)");
gen_file("duplicated-conditionals", "FAIL", "", "peer=(name=1) peer=(label=2)");

print "Generated $count dbus tests\n";

sub print_rule($$$$$$$$$) {
    my ($file, $quantifier, $perms, $session, $name, $path, $interface, $member, $peer) = @_;

    print $file " ";
    print $file " ${quantifier}" if ${quantifier};
    print $file " dbus";
    print $file " ${perms}" if ${perms};
    print $file " ${session}" if ${session};
    print $file " ${name}" if ${name};
    print $file " ${path}" if ${path};
    print $file " ${interface}" if ${interface};
    print $file " ${member}" if ${member};
    print $file " ${peer}" if ${peer};
    print $file ",\n";
}

sub gen_file($$$$$$$$$$) {
    my ($test, $xres, $quantifier, $perms, $session, $name, $path, $interface, $member, $peer) = @_;

    my $file;
    unless (open $file, ">${prefix}/$test-$count.sd") {
	print("couldn't open $test\n");
	exit 1;
    }

    print $file "#\n";
    print $file "#=DESCRIPTION ${test}\n";
    print $file "#=EXRESULT ${xres}\n";
    print $file "#\n";
    print $file "/usr/bin/foo {\n";
    print_rule($file, $quantifier, $perms, $session, $name, $path, $interface,
	       $member, $peer);
    print $file "}\n";
    close($file);

    $count++;
}

sub gen_files($$$$$$$$$$) {
    my ($test, $xres, $quantifiers, $perms, $sessions, $names, $paths, $interfaces, $members, $peers) = @_;

    foreach my $quantifier (@{$quantifiers}) {
      foreach my $perm (@{$perms}) {
	foreach my $session (@{$sessions}) {
	  foreach my $name (@{$names}) {
	    foreach my $path (@{$paths}) {
	      foreach my $interface (@{$interfaces}) {
		foreach my $member (@{$members}) {
		  foreach my $peer (@{$peers}) {
		    gen_file($test, $xres, $quantifier, $perm, $session, $name,
			     $path, $interface, $member, $peer);
		  }
		}
	      }
	    }
	  }
	}
      }
    }
}
