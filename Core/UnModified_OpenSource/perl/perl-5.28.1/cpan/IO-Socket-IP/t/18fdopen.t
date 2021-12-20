#!/usr/bin/perl

use strict;
use warnings;

use Test::More;

use IO::Socket::IP;
use Socket qw( SOCK_STREAM AI_NUMERICHOST );

my $s1 = IO::Socket::IP->new(
   LocalHost => "127.0.0.1",
   Type      => SOCK_STREAM,
   Listen    => 1,
   GetAddrInfoFlags => AI_NUMERICHOST,
) or die "Cannot listen on AF_INET - $@";

my $s2 = IO::Socket::IP->new;
$s2->fdopen( $s1->fileno, 'r' ) or die "Cannot fdopen - $!";

ok( defined $s2->socktype, '$s2->socktype defined' );
is( $s2->sockport, $s1->sockport, '$s2->sockport' );

done_testing;
