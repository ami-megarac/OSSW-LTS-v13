# ------------------------------------------------------------------
#
#    Copyright (C) 2005-2006 Novell/SUSE
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

package Immunix::Severity;
use strict;
use Data::Dumper;

my ($debug) = 0;

sub debug {
    print @_ if $debug;
}

sub new {
    my $self = {};
    $self->{DATABASENAME} = undef;
    $self->{CAPABILITIES} = {};
    $self->{FILES}        = {};
    $self->{REGEXPS}      = {};
    $self->{DEFAULT_RANK} = 10;
    bless($self);
    shift;
    $self->init(@_) if @_;
    return $self;
}

sub init ($;$) {
    my ($self, $resource, $read, $write, $execute, $severity);
    $self = shift;
    $self->{DATABASENAME} = shift;
    $self->{DEFAULT_RANK} = shift if defined $_[0];
    open(DATABASE, $self->{DATABASENAME})
      or die "Could not open severity db $self->{DATABASENAME}: $!\n";
    while (<DATABASE>) {
        chomp();
        next if m/^\s*#/;
        next if m/^\s*$/;

        # leading whitespace is fine; maybe it shouldn't be?
        if (/^\s*\/(\S+)\s+(\d+)\s+(\d+)\s+(\d+)\s*$/) {
            my ($path, $read, $write, $execute) = ($1, $2, $3, $4);

            if (index($path, "*") == -1) {

                $self->{FILES}{$path} = {
                    r => $read,
                    w => $write,
                    x => $execute
                };

            } else {

                my $ptr = $self->{REGEXPS};
                my @pieces = split(/\//, $path);

                while (my $piece = shift @pieces) {
                    if (index($piece, "*") != -1) {
                        my $path = join("/", $piece, @pieces);
                        my $regexp = convert_regexp($path);
                        $ptr->{$regexp}{SD_RANK} = {
                            r => $read,
                            w => $write,
                            x => $execute
                        };
                        last;
                    } else {
                        $ptr->{$piece} = {} unless exists $ptr->{$piece};
                        $ptr = $ptr->{$piece};
                    }
                }
            }
        } elsif (m|^\s*CAP|) {
            ($resource, $severity) = split;
            $self->{CAPABILITIES}{$resource} = $severity;
        } else {
            print "unexpected database line: $_\n";
        }
    }
    close(DATABASE);
    debug Dumper($self);
    return $self;
}

#rank:
# handle capability
# handle file
#
# handle capability
#   if the name is in the database, return it
#   otherwise, send a diagnostic message to stderr and return the default
#
# handle file
#   initialize the current return value to 0
#   loop over each entry in the database;
#     find the max() value for each mode that matches and set a 'found' flag
#   if the found flag has not been set, return the default;
#   otherwise, return the maximum from the database

sub handle_capability ($) {
    my ($self, $resource) = @_;

    my $ret = $self->{CAPABILITIES}{$resource};
    if (!defined($ret)) {
        return "unexpected capability rank input: $resource\n";
    }
    return $ret;
}

sub check_subtree {
    my ($tree, $mode, $sev, $first, @rest) = @_;

    # reassemble the remaining path from this directory level
    my $path = join("/", $first, @rest);

    # first check if we have a literal directory match to descend into
    if ($tree->{$first}) {
        $sev = check_subtree($tree->{$first}, $mode, $sev, @rest);
    }

    # if we didn't get a severity already, check for matching globs
    unless ($sev) {

        # check each glob at this directory level
        for my $chunk (grep { index($_, "*") != -1 } keys %{$tree}) {

            # does it match the rest of our path?
            if ($path =~ /^$chunk$/) {

                # if we've got a ranking, check if it's higher than
                # current one, if any
                if ($tree->{$chunk}->{SD_RANK}) {
                    for my $m (split(//, $mode)) {
                        if ((!defined $sev)
                            || $tree->{$chunk}->{SD_RANK}->{$m} > $sev)
                        {
                            $sev = $tree->{$chunk}->{SD_RANK}->{$m};
                        }
                    }
                }
            }
        }
    }

    return $sev;
}

sub handle_file ($$) {
    my ($self, $resource, $mode) = @_;

    # strip off the initial / from the path we're checking
    $resource = substr($resource, 1);

    # break the path into directory-level chunks
    my @pieces = split(/\//, $resource);

    my $sev;

    # if there's a exact match for this path in the db, use that instead of
    # checking the globs
    if ($self->{FILES}{$resource}) {

        # check each piece of the passed mode against the db entry
        for my $m (split(//, $mode)) {
            if ((!defined $sev) || $self->{FILES}{$resource}{$m} > $sev) {
                $sev = $self->{FILES}{$resource}{$m};
            }
        }

    } else {

        # descend into the regexp tree looking for matches
        $sev = check_subtree($self->{REGEXPS}, $mode, $sev, @pieces);

    }

    return (defined $sev) ? $sev : $self->{DEFAULT_RANK};
}

sub rank ($;$) {
    my ($self, $resource, $mode) = @_;

    if (substr($resource, 0, 1) eq "/") {
        return $self->handle_file($resource, $mode);
    } elsif (substr($resource, 0, 3) eq "CAP") {
        return $self->handle_capability($resource);
    } else {
        return "unexpected rank input: $resource\n";
    }
}

sub convert_regexp ($) {
    my ($input) = shift;

    # we need to convert subdomain regexps to perl regexps
    my $regexp = $input;

    # escape + . [ and ] characters
    $regexp =~ s/(\+|\.|\[|\])/\\$1/g;

    # convert ** globs to match anything
    $regexp =~ s/\*\*/.SDPROF_INTERNAL_GLOB/g;

    # convert * globs to match anything at current path level
    $regexp =~ s/\*/[^\/]SDPROF_INTERNAL_GLOB/g;

    # convert {foo,baz} to (foo|baz)
    $regexp =~ y/\{\}\,/\(\)\|/ if $regexp =~ /\{.*\,.*\}/;

    # twiddle the escaped * chars back
    $regexp =~ s/SDPROF_INTERNAL_GLOB/\*/g;
    return $regexp;
}

1;    # so the require or use succeeds
