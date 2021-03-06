#!/usr/bin/perl

use strict;
use warnings;

use Test::More;

use IO::Socket::IP;
use Socket qw( AI_NUMERICHOST );

my $server = IO::Socket::IP->new(
   Listen    => 1,
   LocalHost => "127.0.0.1",
   LocalPort => 0,
   GetAddrInfoFlags => AI_NUMERICHOST,
) or die "Cannot listen on PF_INET - $!";

my $client = IO::Socket::IP->new(
   PeerHost => $server->sockhost,
   PeerPort => $server->sockport,
   GetAddrInfoFlags => AI_NUMERICHOST,
) or die "Cannot connect on PF_INET - $!";

my $accepted = $server->accept
   or die "Cannot accept - $!";

my $inet_client = $client->as_inet;

isa_ok( $inet_client, "IO::Socket::INET", '->as_inet returns IO::Socket::INET' );

is( $inet_client->fileno, $client->fileno, '->as_inet socket wraps the same fileno' );

undef $client;

$accepted->syswrite( "Message\n" );

$inet_client->sysread( my $buffer, 8192 );
is( $buffer, "Message\n", '->as_inet still passes data' );

done_testing;
