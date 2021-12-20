#! /usr/bin/perl -w

# Very simple script to try converting AppArmor profiles to the new
# profile syntax as of April 2007.
#
# Copyright (C) 2007 Andreas Gruenbacher <agruen@suse.de>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License published by the Free Software Foundation.
#  
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

use FileHandle;
use File::Temp;
use Getopt::Std;
use strict;

sub match($) {
    my ($str) = @_;
    my @fields;

    @fields = ($str =~ /^(\s*)([@\/]\S*)(\s.*,)$/);
    if (!@fields) {
	@fields = ($str =~ /^(\s*")((?:[^"]|\\")*)("\s.*,)$/);
    }

    return @fields;
}

sub alterations($) {
    my ($str) = @_;

    if ($str =~ /^([^{]*){([^}]*,[^}]*)}(.*)$/) {
	my @strs = map { "$1$_$3" } split(/,/, $2);
	return map { alterations($_) } @strs;
    } else {
	return ($str);
    }
}

my %known_dirs;

sub remember_pathname($) {
    my ($str) = @_;
    my $pathname;

    for (split /(\/)/, $str) {
	if ($_ eq '/' && $pathname ne '') {
	    #print "<<>> $pathname\n";
	    $known_dirs{$pathname} = 1;
	}
	$pathname .= $_;
    }
}

sub add_slash($$) {
    my ($str, $perms) = @_;

    return exists $known_dirs{$str} || -d $str;
}

sub never_add_slash($$) {
    my ($str, $perms) = @_;

    return $perms =~ /[lmx]/ || $str =~ /\.(so|cf|db|conf|config|log|pid|so\*)$/ ||
	   $str =~ /(\*\*|\/)$/ || (-e $str && ! -d $str);

}

our($opt_i);
getopts('i');

foreach my $filename (@ARGV) {
    my $fh_in;
    
    $fh_in = new FileHandle("< $filename")
	or die "$filename: $!\n";

    while (<$fh_in>) {
	    if (my @fields = match($_)) {
		for my $x (alterations($fields[1])) {
		    remember_pathname($x);
		}
	    }
    }
}

if (@ARGV == 0) {
    print "Usage: $0 profile ...\n";
    print "Tries to convert the profile to the new profile syntax, and\n" .
	  "prints the result to standard output. The result may need" .
	  "further review.\n";
    exit 0;
}

foreach my $filename (@ARGV) {
    my ($fh_in, $fh_out, $tmpname);

    $fh_in = new FileHandle("< $filename")
	or die "$filename: $!\n";

    if ($opt_i) {
	($fh_out, $tmpname) = mkstemp("$filename.XXXXXX")
	    or die "$!\n";
	*STDOUT = $fh_out;
    }

    while (<$fh_in>) {
	    if (my @fields = match($_)) {
		for my $x (alterations($fields[1])) {
		    if (never_add_slash($x, $fields[2])) {
			print $_;
		    } elsif (add_slash($x, $fields[2])) {
			print "$fields[0]$x/$fields[2]	# (dir)\n";
		    } else {
			print "$fields[0]$x/$fields[2]	# (maybe-dir)\n";
			print $_;
		    }
		}
	    } else {
		print $_;
	    }
    }

    if ($opt_i) {
	rename $tmpname, $filename
	    or die "$filename: $!\n";
    }
}

# vim: smartindent softtabstop=4 shiftwidth=4
