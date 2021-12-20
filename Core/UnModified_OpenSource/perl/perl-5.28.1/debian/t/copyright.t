#!/usr/bin/perl -w
use strict;
use Test::More tests => 8;

my $upstream_version;
ok(open(P, "dpkg-parsechangelog |"), "successfully piping from dpkg-parsechangelog");
while (<P>) {
    /^Version: (.+)-[^-]+$/ or next;
    $upstream_version = $1;
    last;
}
isnt($upstream_version, "", "found upstream version from dpkg-parsechangelog output");
ok(close P, "dpkg-parsechangelog exited normally");

my $checked_version;
ok(open(C, "<debian/copyright"), "successfully opened debian/copyright");
while (<C>) {
    next if !/^ Last checked against: Perl (.+)/;
    $checked_version = $1;
    last;
}
isnt($checked_version, "", "found checked version from debian/copyright");
close C;

is($checked_version, $upstream_version,
    "debian/copyright last checked for the current upstream version");

SKIP: {
    system('which cme >/dev/null 2>&1');
    my $cmd;
    if ($?) {
        system('which config-edit >/dev/null 2>&1');
        skip('no cme or config-edit or available', 2) if $?;
        $cmd = 'config-edit -application dpkg-copyright -ui none';
    } else {
        skip('no cme dpkg-copyright application available (try installing libconfig-model-dpkg-perl)', 2)
            if qx/cme list/ !~ /dpkg-copyright/;
        $cmd = 'cme check dpkg-copyright';
    }
    note("checking debian/copyright with copyright checker '$cmd'");
    unlike( qx/$cmd 2>&1/, qr/error/,
        'no error messages from copyright checker when parsing debian/copyright');
    is($?, 0, 'copyright checker exited successfully');
}

