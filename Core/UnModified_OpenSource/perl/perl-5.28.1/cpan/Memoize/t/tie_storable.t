#!/usr/bin/perl
# -*- mode: perl; perl-indent-level: 2 -*-

use lib qw(. ..);
use Memoize 0.45 qw(memoize unmemoize);
# $Memoize::Storable::Verbose = 0;

eval {require Memoize::Storable};
if ($@) {
  print "1..0\n";
  exit 0;
}

sub i {
  $_[0];
}

sub c119 { 119 }
sub c7 { 7 }
sub c43 { 43 }
sub c23 { 23 }
sub c5 { 5 }

sub n {
  $_[0]+1;
}

eval {require Storable};
if ($@) {
  print "1..0\n";
  exit 0;
}

print "1..9\n";

$file = "storable$$";
1 while unlink $file;
tryout('Memoize::Storable', $file, 1);  # Test 1..4
1 while unlink $file;
tryout('Memoize::Storable', $file, 5, 'nstore');  # Test 5..8
assert_netorder($file, 9);  # Test 9
1 while unlink $file;


sub assert_netorder {
  my ($file, $testno) = @_;

  my $netorder = Storable::file_magic($file)->{'netorder'};
  print ($netorder ? "ok $testno\n" : "not ok $testno\n");
}

sub tryout {
  my ($tiepack, $file, $testno, $option) = @_;

  if (defined $option) {
    tie my %cache => $tiepack, $file, $option
      or die $!;
  } else {
    tie my %cache => $tiepack, $file
      or die $!;
  }

  memoize 'c5', 
  SCALAR_CACHE => [HASH => \%cache],
  LIST_CACHE => 'FAULT'
    ;

  my $t1 = c5();	
  my $t2 = c5();	
  print (($t1 == 5) ? "ok $testno\n" : "not ok $testno\n");
  $testno++;
  print (($t2 == 5) ? "ok $testno\n" : "not ok $testno\n");
  unmemoize 'c5';
  1;
  1;

  # Now something tricky---we'll memoize c23 with the wrong table that
  # has the 5 already cached.
  memoize 'c23', 
  SCALAR_CACHE => [HASH => \%cache],
  LIST_CACHE => 'FAULT'
    ;
  
  my $t3 = c23();
  my $t4 = c23();
  $testno++;
  print (($t3 == 5) ? "ok $testno\n" : "not ok $testno\n");
  $testno++;
  print (($t4 == 5) ? "ok $testno\n" : "not ok $testno\n");
  unmemoize 'c23';
}
