#!/usr/bin/perl
# Test for ssh

use strict;

use Carp;
use Test;
use Term::ReadKey; # For password input
use Expect; # Testing of various things

# Global Vars
my $ssh = '/usr/bin/ssh'; # SSH binary
my $host = '127.0.0.1'; # Host to test
my $timeout = 10; # Timeout for expect calls

# Prompt for a username
BEGIN { 
my $user = getlogin() || (getpwuid($<))[0];
ReadMode('normal');
print "Username to login as [$user]: ";
my $result = ReadLine(0);
chomp($result);
$user = ($result ne '') ? $result : $user;

# Prompt for a password
print "password for $user: ";
ReadMode('noecho');
my $password = ReadLine(0);
ReadMode('normal');
chomp($password);
croak "\nERROR: no password\n" if ($password eq '');

print "\n\n";

plan tests => 2; }

ok(sshd_alive(),0,"sshd is not running or not reachable");
ok(sshd_pass(),0,"sshd did not take your password");

sub sshd_alive {
  my $exp;
  my $result = 1;
 
  my @options = ('-o RhostsAuthentication=no',
		 '-o RhostsRSAAuthentication=no',
		 '-o RSAAuthentication=no',
		 '-o HostbasedAuthentication=no',
		 '-o BatchMode=yes',
		 '-o CheckHostIP=no',
		 '-o StrictHostKeyChecking=yes',
                 "-o User=$user",
		 $host);

  $exp = new Expect;
  $exp->log_stdout(0);
  $exp->spawn($ssh,@options) || return 1 && $exp->soft_close();
  $exp->expect($timeout,
	       ['Connection refused',sub { $result = 1}],
	       ['No RSA',sub { $result = 0 }],
	       ['Permission denied',sub { $result = 0}])
    || return 1 && $exp->soft_close();
  $exp->soft_close();
  return $result;
}

sub sshd_pass {
  my $exp;
  my $result = 1;

  my @options = ('-o PasswordAuthentication=yes',
		 '-o RhostsAuthentication=no',
		 '-o RhostsRSAAuthentication=no',
		 '-o RSAAuthentication=no',
		 '-o HostbasedAuthentication=no',
		 '-o CheckHostIP=no',
		 '-o StrictHostKeyChecking=yes',
		 "-o User=$user",
		 $host);

  $exp = new Expect;
  $exp->log_stdout(0);
  $exp->spawn($ssh,@options) || return 1 && $exp->soft_close();
  $exp->expect($timeout,
	       ['password:',sub { $exp->send("$password\r") }])
    || return 1 && $exp->soft_close();
  $exp->clear_accum();
   $exp->expect($timeout,
	       ['Permission denied', sub { $result = 1; }],
	       ['Last login:',sub { $result = 0; }])
    || return 1 && $exp->soft_close();
  $exp->soft_close();
  return $result;
}

sub sshd_rhosts {
  my $exp;
  my $result = 1;

  my @options = ('-o PasswordAuthentication=no',
		 '-o RhostsAuthentication=yes',
		 '-o RhostsRSAAuthentication=no',
		 '-o RSAAuthentication=no',
		 '-o HostbasedAuthentication=no',
		 '-o CheckHostIP=no',
		 '-o StrictHostKeyChecking=yes',
		 "-o User=$user",
		 $host);
  
  `cat $host $user > ~/.rhosts`;
  $exp = new Expect;
  $exp->log_stdout(0);
  $exp->spawn($ssh,@options) || return 1 && $exp->soft_close();
  $exp->expect($timeout,
	       ['password:',sub { $exp->send("$password\r") }])
    || return 1 && $exp->soft_close();
  $exp->clear_accum();
   $exp->expect($timeout,
	       ['Permission denied', sub { $result = 1; }],
	       ['Last login:',sub { $result = 0; }])
    || return 1 && $exp->soft_close();
  $exp->soft_close();
  return $result;
}
