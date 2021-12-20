#!/usr/bin/perl -w
use strict;
use Test::More tests => 6;

# Test that the current upstream version is listed in debian/released-versions
# for uploads targetting unstable.
#
# Forgetting this will only cause problems if/when the next minor release is
# uploaded, at which point it will be missing perlapi-5.xx.x for this version.
# Running this check from autopkgtest should therefore pinpoint such issues
# well in advance.

my $upstream_version;
my $distribution;
ok(open(P, "dpkg-parsechangelog |"), "successfully piping from dpkg-parsechangelog");
while (<P>) {
    chomp;
    /^Version: (.+)-[^-]+$/ and $upstream_version = $1;
    /^Distribution: (.+)+$/ and $distribution = $1;
}
ok(close P, "dpkg-parsechangelog exited normally");
ok(defined $upstream_version, "found upstream version from dpkg-parsechangelog output");
ok(defined $distribution, "found distribution information from dpkg-parsechangelog output");

SKIP: {
    skip("Only checking debian/released-versions for unstable (not $distribution)", 2)
        if $distribution ne 'unstable';
    my $found=0;
    ok(open(C, "<debian/released-versions"), "successfully opened debian/released-versions");
    while (<C>) {
        chomp;
        $found++,last if $_ eq $upstream_version;
    }
    close C;
    ok($found, "found version $upstream_version in debian/released-versions");
}
