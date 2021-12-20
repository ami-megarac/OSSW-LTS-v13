# ----------------------------------------------------------------------
#    Copyright (c) 2006 Novell, Inc. All Rights Reserved.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License as published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, contact Novell, Inc.
#
#    To contact Novell about this file by physical or electronic mail,
#    you may find current contact information at www.novell.com.
# ----------------------------------------------------------------------

package Immunix::Config;

use strict;
use warnings;

use Carp;
use Cwd qw(cwd realpath);
use File::Basename;
use File::Temp qw/ tempfile tempdir /;
use Data::Dumper;
use Locale::gettext;
use POSIX;

require Exporter;
our @ISA    = qw(Exporter);
our @EXPORT = qw(
    read_config
    write_config
    find_first_file
    find_first_dir
);

our $confdir = "/etc/apparmor";

# config vars
our $cfg;
our $repo_cfg;

sub read_config {
    my $filename = shift;
    my $config;

    if (open(CONF, "$confdir/$filename")) {
        my $which;
        while (<CONF>) {
            chomp;
            # ignore comments
            next if /^\s*#/;
            if (m/^\[(\S+)\]/) {
                $which = $1;
            } elsif (m/^\s*(\S+)\s*=\s*(.*)\s*$/) {
                my ($key, $value) = ($1, $2);
                $config->{$which}{$key} = $value;
            }
        }
        close(CONF);
    }

    # LP: #692406
    # Explicitly disable the repository until there is an alternative, since
    # the OpenSUSE site went away
    if ($filename eq "repository.conf") {
        $config->{repository}{enabled} = "no";
    }

    return $config;
}

sub write_config {
    my ($filename, $config) = @_;
    if (open(my $CONF, ">$confdir/$filename")) {
        for my $section (sort keys %$config) {
            print $CONF "[$section]\n";

            for my $key (sort keys %{$config->{$section}}) {
                print $CONF "  $key = $config->{$section}{$key}\n"
                    if ($config->{$section}{$key});
            }
        }
        chmod(0600, $CONF);
        close($CONF);
    } else {
        die "Can't write config file $filename: $!";
    }
}

sub find_first_file {
    my $list = shift;
    return if ( not defined $list );
    my $filename;
    for my $f (split(/\s+/, $list)) {
        if (-f $f) {
            $filename = $f;
            last;
        }
    }

    return $filename;
}

sub find_first_dir {
    my $list = shift;
    return if ( not defined $list );
    my $dirname;
    for my $f (split(/\s+/, $list)) {
        if (-d $f) {
            $dirname = $f;
            last;
        }
    }

    return $dirname;
}

1;
