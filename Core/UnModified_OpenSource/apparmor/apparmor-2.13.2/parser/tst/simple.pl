#!/usr/bin/perl -w
#
# A simple driver for testing the subdomain parser.
# All files in $CWD named *.sd will be tested against the parser.
#
use strict;
use Getopt::Long;
use Test::More;

my %config;
$config{'parser'} = "/sbin/apparmor_parser";
$config{'profiledir'} = "./simple_tests/";
$config{'timeout'} = 480; # in seconds

my $help;
my $pwd = `pwd`;
chomp($pwd);

GetOptions(
  "help|h" => \$help,
);

sub usage {
  print STDERR "Usage $0 profile_directory\n";
  print STDERR "\tTests the subdomain parser on the given profile directory\n";
  print STDOUT "Bail out! Got the usage statement\n";
  exit 0;
}

&usage if ($help);
read_config();

# let environment variable override config file, for use in automated
# test suites
if ($ENV{APPARMOR_PARSER}) {
  $config{'parser'} = $ENV{APPARMOR_PARSER};
}

# Override config file profile location when passed on command line
if (@ARGV >= 1) {
  $config{'profiledir'} = shift;
}

if ($config{'profiledir'} =~ /^\//) {
  $config{'includedir'} = $config{'profiledir'};
} else {
  $config{'includedir'} = "$pwd/$config{'profiledir'}";
}

sub read_config {
  my $which;

  if(open(CONF, "uservars.conf")) {
    while(<CONF>) {
      chomp;

      next if /^\s*#/;

      if (m/^\s*(\S+)\s*=\s*(.+)\s*$/) {
        my ($key, $value) = ($1, $2);
	$config{$key} = $value;
      }
    }
    close(CONF);
  }
}

sub test_profile {
  my $profile = shift;
  my $description = "no description for testcase";
  my $expass = 1;
  my $istodo = 0;
  my $isdisabled = 0;
  my $result = 1;
  my $signal = 0;
  my $coredump = 0;
  my $child;

  $child = open(PARSER, "|-");
  if ($child == 0) {
    # child
    open(STDOUT, ">/dev/null") or die "Failed to redirect STDOUT";
    open(STDERR, ">/dev/null") or die "Failed to redirect STDERR";
    exec("$config{'parser'}", "--config-file=./parser.conf", "-M", "features_files/features.all", "-S", "-I", "$config{'includedir'}") or die "Bail out! couldn't open parser";
    # noreturn
  }

  # parent
  eval {
    local $SIG{ALRM} = sub {
      kill PIPE => $child;
      $description = "$description - TESTCASE TIMED OUT";
    };

    alarm $config{'timeout'};

    open(PROFILE, $profile) or die "Bail out! couldn't open profile $profile";
    while (<PROFILE>) {
      if (/^#=DESCRIPTION\s*(.*)/i) {
        $description = $1;
      } elsif (/^#=EXRESULT\s*(\w+)/) {
        if ($1 eq "PASS") {
          $expass = 1;
        } elsif ($1 eq "FAIL") {
          $expass = 0;
        } else {
          die "Bail out! unknown expected result '$1' in $profile";
        }
      } elsif (/^#=TODO\s*/) {
          $istodo = 1;
      } elsif (/^#=DISABLED\s*/) {
          $isdisabled = 1;
      } else {
        print PARSER if not $isdisabled;
      }
    }

    close(PARSER) or ($! and die "Bail out! couldn't close parser pipe");
    $result = $? >> 8;
    $signal = $? & 127;
    $coredump = $signal ? $? & 128 : 0;
    alarm 0;
  };

  alarm 0;
  if ($isdisabled) {
    TODO: {
      local $TODO = "Disabled testcase.";
      ok(0, "TODO: $profile: $description");
    }
  } elsif ($coredump) {
    ok(0, "$profile: Produced core dump (signal $signal): $description");
  } elsif ($istodo) {
    if ($expass != $result) {
        fail("TODO passed unexpectedly: $profile: $description");
    } else {
      TODO: {
        local $TODO = "Unfixed testcase.";
        ok($expass ? !$result : $result, "TODO: $profile: $description");
      }
    }
  } else {
    ok($expass ? !$result : $result, "$profile: $description");
  }
}


sub find_all_tests {
    my $testdir = shift;

    opendir(DIR, $testdir) or die "Bail out! can't opendir $testdir: $!";
    my @files = sort grep { /\.sd$/ && -f "$testdir/$_" } readdir(DIR);
    closedir(DIR);

    my @profiles;
    foreach my $profile (@files) {
	push (@profiles, "$testdir/$profile");
    }

    opendir(DIR, $testdir) or die "Bail out! can't opendir $testdir: $!";
    my @dirs = sort grep { /^[^\.]/ && -d "$testdir/$_" } readdir(DIR);
    closedir(DIR);

    foreach my $dir (@dirs) {
	push(@profiles, find_all_tests("$testdir/$dir"));
    }
    return @profiles;
}

my @profiles = find_all_tests($config{'profiledir'});

plan tests => scalar(@profiles);

foreach my $profile (@profiles) {
    test_profile ("$profile");
}
