#!/usr/bin/perl
# ----------------------------------------------------------------------
#    Copyright (c) 2008 Dominic Reynolds. All Rights Reserved.
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
#
# ----------------------------------------------------------------------

# ----------------------------------------------------------------------
#
my $usage =
"aa-repo.pl --command args\n";

my $usage_search =
"    --search [author=XXX] [prog=XXX] [id=XXX]
             Search the repository for profiles matching the search criteria
             and return the results.
             NOTE: One --search switch per option

    --verbose|v
             Verbosity level. Supply either one or two switches. Two switches
             adds full profile text in returned search results.\n";

my $usage_push =

"    --push   [--profile=XXX|all] [--changelog=XXX]
             Push local profiles to repository, uses configured user and upon
             overwrite of an existing profile in the repository then prompt
             user with a diff for confirmation XXX the name of the application
             whose profile should be uploaded or \"all\"  to upload all
             profiles.  Multiple --profile switches may be passed to supply
             multiple profile names

             e.g.  --push --profile /usr/sbin/mdnsd --profile /usr/sbin/ftp
             e.g.  --push --profile all\n";

my $usage_pull =
"    --pull   [--author=XXX] [--profile=XXX] or [--id=XXX] [--mode=complain]
             pull remote profiles and install on local system
             If operation will change local profiles then prompt user with
             diff for confirmation
             NOTE: One --pull switch per option and there are three acceptable
                   combinations

             --pull --author=XXX
               * pull all profiles in the repo for the author

             --pull --author=XXX  --profile=XXXX
               * pull the profile for prog owned by author

             --pull --id=XXXX
               * pull the profile with id

             --pull --mode=complain
               * set the profile(s) to complain mode when installed

             Profiles are checked for conflicts with currently installed
             profiles and presented as a list to the user to confirm and view.\n";


my $usage_sync =
"    --sync   [--up] [--down] [--noconfirm]
             Synchronize local profile set with the repository - showing
             changes and allowing prompting the user with the diffs and
             suggest the newest version to be activated. If the --all option
             is passed then treat profiles not marked as remote as new
             profiles that will be uploaded to the repository.\n";

my $usage_stat =
"    --status
             Show the current status of the local profile set. This operation
             is similar to sync but does not prompt the user to up|down load
             changes\n";

my $usage_getconfig =
"    --getconfig|c
             Print the current configuration for the repsository\n";


my $usage_setconfig =
"    --setconfig [url=xxx] [username=xxxx] [password=xxxx] [enabled=(yes|no)]
                [upload=(yes|no)]
              Set the configuration options for the repository.
              NOTE: One --setconfig switch per option\n";

my $usage_bottom =
"    --quiet|q Don't prompt user - assume that all changes should be made.

    ISSUES:
      o Should changes made to the system be recorded somehow? An audit event?
      o Should the tool allow a repo/distro to be passed for each operation?

";

use strict;
use Getopt::Long;

use Immunix::AppArmor;
use Immunix::Repository;
use Data::Dumper;

use Locale::gettext;
use POSIX;

# force $PATH to be sane
$ENV{PATH} = "/bin:/sbin:/usr/bin:/usr/sbin";

# initialize the local poo
setlocale(LC_MESSAGES, "");
textdomain("apparmor-utils");

# options variables
my $help = '';
my $verbose = '';

my ( $id, $author, $mode, %search, $sync, $getconfig, $push,
     $pull, %setconfig, @profiles, $all, $changelog, $stat );

GetOptions(
    'search=s%'    => \%search,
    'sync=s'       => \$sync,
    'status'       => \$stat,
    'getconfig|c'  => \$getconfig,
    'setconfig=s%' => \%setconfig,
    'push'         => \$push,
    'id=s'         => \$id,
    'author=s'     => \$author,
    'profile=s'    => \@profiles,
    'changelog=s'  => \$changelog,
    'pull'         => \$pull,
    'all|a'        => \$all,
    'help|h'       => \$help,
    'verbose|v+'   => \$verbose
);

#
# Root privs required to run the repo tool
#
if ( geteuid() != 0 ) {
    print STDERR gettext(
"You must be logged in with root user privileges to use this program.\n"
                        );
    exit;
}

# --help
# tell 'em how to use it...
&usage && exit if $help;

my $config = get_repo_config();

#
# --getconfig operation
#
&config && exit if $getconfig;

my $sd_mountpoint = check_for_subdomain();
unless ($sd_mountpoint) {
    fatal_error(gettext(
"AppArmor does not appear to be started.  Please enable AppArmor and try again."
                       )
               );
}

#
# --setconfig operation
#
if ( keys %setconfig ) {
   $config->{url}      = $setconfig{url}      if ( $setconfig{url} );
   $config->{distro}   = $setconfig{distro}   if ( $setconfig{distro} );
   $config->{enabled}  = $setconfig{enabled}  if ( $setconfig{enabled} );
   $config->{email}    = $setconfig{email}    if ( $setconfig{email} );
   $config->{user}     = $setconfig{username} if ( $setconfig{username} );
   $config->{password} = $setconfig{password} if ( $setconfig{password} );
   $config->{upload}   = $setconfig{upload}   if ( $setconfig{upload} );
   set_repo_config( $config );
}

#
# --push operation
#
if ( $push ) {
    my ($conflicts, $repo_profiles, $local_profiles, @overrides);
    if ( ! @profiles ) {
       print STDERR gettext(
"Must supply at least one profile using \"--profile XXX\" to --push\n"
                           );
       exit 1;
    } else {
       print STDERR Data::Dumper->Dump([@profiles], [qw(*profiles)]);
    }
    my $changelog = $changelog?$changelog:"none";
    push_profiles( \@profiles, $changelog, 1 );
}


#
#  --pull operation
#
if ( $pull ) {
    my $type = "";
    if ( $id ) {
        if ( $author || @profiles ) {
            print STDERR gettext(
"Option --id=XX is only allowed by itself and not in combination to
other options for the --pull command.\n"
                                );
            exit 1;
       }
       $type = "id";
    }
    if ( @profiles && ! $author ) {
        print STDERR gettext(
"Option --profile=XX requires that the --author=XX option be supplied
to distinguish a specific profile.\n"
                            );
        exit 1;
    } else {
        $type = "profile";
    }

    my $mode = $mode eq "complain"?1:0;
    pull_profiles( \@profiles, $type, $mode, 1 );
}

#
#  --search operation
#
if ( keys %search ) {
    if ( $search{id} ) {
        my($status,$result) = fetch_profile_by_id( $config->{url},
                                                   $search{id} );
        if ($status) {
            my $title = sprintf(gettext( "Profile ID %s\n"), $search{id});
            console_print_search_results( $title,
                                          "profile",
                                          { $result->{name} => $result }
                                        );

        } else {
            print STDERR "ERROR $result\n";
        }
    } elsif ( $search{author} && $search{prog} ) {
        my($status,$result) =
            fetch_profiles_by_name_and_user( $config->{url},
                                             $config->{distro},
                                             $search{prog},
                                             $search{author}
                                           );
        if ( $status ) {
            my $title =
                sprintf(gettext("Profiles matching user: %s and program: %s\n"),
                        $search{author},
                        $search{prog}
                       );
            console_print_search_results( $title, "profile", $result );
        } else {
            print STDERR "ERROR $result\n";
        }
    } elsif ( $search{author} ) {
        my($status,$result) = fetch_profiles_by_user( $config->{url},
                                                      $config->{distro},
                                                      $search{author}
                                                    );
        if ( $status ) {
            my $title = sprintf(gettext( "Profiles for %s\n"), $search{author});
            console_print_search_results( $title, "profile", $result );
        } else {
            print STDERR "ERROR $result\n";
        }
    } elsif ( $search{prog} ) {
        my($status,$result) = fetch_profiles_by_name( $config->{url},
                                                      $config->{distro},
                                                      $search{prog},
                                                    );
        if ( $status ) {
            my $title = sprintf(gettext("Profiles matching program: %s\n"),
                                $search{prog});
            console_print_search_results( $title, "user", $result );
        } else {
            print STDERR "ERROR $result\n";
        }
    } else  {
        print STDERR
"Unsupported search criteria. Please specify at least one of
author=XXX prog=XXX id=XXX\n";
    }
}

if ( $stat ) {
    my ( $local_profiles, $remote_profiles );
    my $msg =
" The following profiles are stored in the repository but
 are not synchronized with the copy in the repository\n";

    my ($status, $result) = fetch_profiles_by_user( $config->{url},
                                                    $config->{distro},
                                                    $config->{user}
                                                  );
    if ( $status ) {
        $remote_profiles = $result;
    } else {
        print STDERR sprintf(gettext("ERROR connecting to repository: %s\n"),
                             $result);
        exit;
    }

    readprofiles();
    $local_profiles = serialize_local_profiles( \%sd );
    my ($local_only,$unsynched,$synched,$conflicts) = ({}, {}, {});
    $unsynched = find_profile_conflicts($remote_profiles, $local_profiles);
    for my $p ( keys %$local_profiles ) {
        if ( ! $remote_profiles->{$p} ) {
            $local_only->{$p} = $local_profiles->{$p};
        }
    }

    for my $p ( keys %$remote_profiles ) {
        $synched->{$p} =
            $remote_profiles->{$p}->{profile} if ( ! %$unsynched->{$p} );
    }
    UI_status($synched, $unsynched, $local_only);
}

######################
# Helper functions
######################

#
# Compare the local profile set with the remote profile set.
# Return a list of the conflicting profiles as a list
#  { PROFILE_NAME =>  [LOCAL_PROFILE, REMOTE_PROFILE] ]
#
#
# remote_profiles = repository profiles as returned by one of the
#                   Immunix::Repository::fetch... functions
# local_profiles  = hash ref containing
#                    {  name => serialized local profile }
#
#

sub find_profile_conflicts ($$) {
    my ($remote_profiles,$local_profiles) = @_;
    my $conflicts = {};
    for my $p ( keys(%$local_profiles) ) {
        if ( $local_profiles->{$p} and $remote_profiles->{$p} ) {
            my $p_local  = $local_profiles->{$p};
            my $p_remote = $remote_profiles->{$p}->{profile};
            chomp($p_local);
            chomp($p_remote);
            if ( $p_remote ne $p_local ) {
                $conflicts->{$p} = [ $p_local, $p_remote ];
            }
        }
    }
   return( $conflicts );
}

sub serialize_local_profiles ($) {
   my $profiles = shift;
   my $local_profiles = {};
   for my $p ( keys %$profiles ) {
       my $serialize_opts = {};
       $serialize_opts->{NO_FLAGS} = 1;
       my $p_local = serialize_profile( $profiles->{$p},
                                        $p,
                                        $serialize_opts );
       $local_profiles->{$p} = $p_local;
   }
   return $local_profiles;
}


sub console_print_search_results ($$$) {
    my ($title, $type,$result) = @_;
    open(PAGER, "| less") or die "Can't open pager";
    print PAGER $title;
    print PAGER "Found " . values(%$result) . " profiles \n";
    for my $p ( values(%$result) ) {
        if ( $verbose ) {
            if ( $type eq "user" ) {
                print PAGER " Author    [ " . $p->{username} . " ]\n";
            } elsif ( $type eq "profile" ) {
                print PAGER " Name      [ " . $p->{name} . " ]\n";
            }
            print PAGER " Created   [ " . $p->{created_at} . " ]\n";
            print PAGER " Downloads [ " . $p->{downloaded_count} . " ]\n";
            print PAGER " ID        [ " . $p->{id} . " ]\n";
            if ( $verbose > 1 ) {
                print PAGER " Profile   [ \n" . $p->{profile} . " ]\n\n";
            } else {
                print PAGER "\n";
            }
        } else {
            my $data =  $type eq "user"?$p->{username}:$p->{name};
            print PAGER " " . $data . "\n";
        }
    }
    close PAGER;
}

sub UI_resolve_profile_conflicts {

    my ($explanation, $conflict_hash) = @_;
    my $url = $config->{url};
    my @conflicts = map { [ $_,
                            $conflict_hash->{$_}->[0],
                            $conflict_hash->{$_}->[1]
                          ] }
                          keys %$conflict_hash;
    my @commits = [];
    my $title = "Profile conflicts";
    my %resolution = ();
    my $q = {};
    $q->{title} = $title;
    $q->{headers} = [ "Repository", $url, ];

    $q->{explanation} = $explanation;

    $q->{functions} = [ "CMD_OVERWRITE",
                        "CMD_KEEP",
                        "CMD_VIEW_CHANGES",
                        "CMD_ABORT",
                        "CMD_CONTINUE", ];

    $q->{default} = "CMD_OVERWRITE";
    $q->{options} = [ map { $_->[0] } @conflicts ];
    $q->{selected} = 0;

    my ($ans, $arg);
    do {
        ($ans, $arg) = UI_PromptUser($q);

        if ($ans eq "CMD_VIEW_CHANGES") {
            display_changes($conflicts[$arg]->[2], $conflicts[$arg]->[1]);
        }
        if ( $ans eq "CMD_OVERWRITE") {
           $q->{options} =
             [ map { $_ =~ /$conflicts[$arg]->[0](  K|  O)?$/?
                           $conflicts[$arg]->[0] . "  O":
                           $_ }
                   @{$q->{options}}
             ];
             $resolution{$conflicts[$arg]->[0]} = "O";
        }
        if ( $ans eq "CMD_KEEP") {
           $q->{options} =
             [ map { $_ =~ /$conflicts[$arg]->[0](  K|  O)?$/?
                           $conflicts[$arg]->[0] . "  K":
                           $_ }
                   @{$q->{options}}
             ];
             $resolution{$conflicts[$arg]->[0]} = "K";
        }
        $q->{selected} = ($arg+1) % @conflicts;
    } until $ans =~ /^CMD_CONTINUE/;
    if ($ans eq "CMD_CONTINUE") {
        my @results = ();
        for my $p ( keys %resolution ) {
            if ( $resolution{$p} eq "O" ) {
                 push @results, $p;
            }
        }
        return @results;
    }
}

sub UI_display_profiles {
    my ($explanation, $profile_hash) = @_;
    my $url = $config->{url};
    my @profiles = map { [ $_, $profile_hash->{$_} ] } keys %$profile_hash;
    my $title = gettext("Profiles");
    my $q = {};
    $q->{title} = $title;
    $q->{headers} = [ "Repository", $url, ];

    $q->{explanation} = $explanation;

    $q->{functions} = [ "CMD_VIEW",
                        "CMD_CONTINUE", ];

    $q->{default} = "CMD_CONTINUE";
    $q->{options} = [ map { $_->[0] } @profiles ];
    $q->{selected} = 0;

    my ($ans, $arg);
    do {
        ($ans, $arg) = UI_PromptUser($q);

        if ($ans eq "CMD_VIEW") {
            my $pager = get_pager();
            open ( PAGER, "| $pager" ) or die "Can't open $pager";
            print PAGER gettext("Profile: ") . $profiles[$arg]->[0] . "\n";
            print PAGER $profiles[$arg]->[1];
            close PAGER;
        }
        $q->{selected} = ($arg+1) % @profiles;
    } until $ans =~ /^CMD_CONTINUE/;
    return;
}

sub UI_display_profile_conflicts {
    my ($explanation, $conflict_hash) = @_;
    my $url = $config->{url};
    my @conflicts = map { [ $_,
                            $conflict_hash->{$_}->[0],
                            $conflict_hash->{$_}->[1]
                          ] }
                          keys %$conflict_hash;
    my @commits = [];
    my $title = gettext("Profile conflicts");
    my $q = {};
    $q->{title} = $title;
    $q->{headers} = [ "Repository", $url, ];

    $q->{explanation} = $explanation;

    $q->{functions} = [ "CMD_VIEW_CHANGES",
                        "CMD_CONTINUE", ];

    $q->{default} = "CMD_CONTINUE";
    $q->{options} = [ map { $_->[0] } @conflicts ];
    $q->{selected} = 0;

    my ($ans, $arg);
    do {
        ($ans, $arg) = UI_PromptUser($q);

        if ($ans eq "CMD_VIEW_CHANGES") {
            display_changes($conflicts[$arg]->[2], $conflicts[$arg]->[1]);
        }
        $q->{selected} = ($arg+1) % @conflicts;
    } until $ans =~ /^CMD_CONTINUE/;
    return;
}

sub usage {
    if ( $help eq "push" ) {
        print STDERR $usage . $usage_push ."\n";
    } elsif ( $help eq "pull" ) {
        print STDERR $usage . $usage_pull ."\n";
    } elsif ( $help eq "sync" ) {
        print STDERR $usage . $usage_sync ."\n";
    } elsif ( $help eq "getconfig" ) {
        print STDERR $usage . $usage_getconfig ."\n";
    } elsif ( $help eq "setconfig" ) {
        print STDERR $usage . $usage_setconfig ."\n";
    } elsif ( $help eq "status" ) {
        print STDERR $usage . $usage_stat ."\n";
    } elsif ( $help eq "search" ) {
        print STDERR $usage . $usage_search ."\n";
    } else {
     open(PAGER, "| less") or die "Can't open pager";
     print PAGER $usage .
                 $usage_search .
                 $usage_push .
                 $usage_pull .
                 $usage_sync .
                 $usage_stat .
                 $usage_setconfig .
                 $usage_getconfig .
                 $usage_bottom . "\n";
     close PAGER;
   }
}

#
# --getconfig helper function
#
sub config {
    my $configstr = gettext("Current config\n");
    my $config = get_repo_config();
    $configstr .= "\turl:\t\t$config->{url}\n";
    $configstr .= "\tdistro:\t\t$config->{distro}\n";
    $configstr .= "\tenabled:\t$config->{enabled}\n";
    $configstr .= "\temail:\t\t$config->{email}\n";
    $configstr .= "\tusername:\t$config->{user}\n";
    $configstr .= "\tpassword:\t$config->{password}\n";
    $configstr .= "\tupload:\t\t$config->{upload}\n";
    print STDERR $configstr . "\n";
}

#
#  helper function to push profiles to the repository
#  used by --push and --sync options
#
sub push_profiles($$$) {
    my ( $p_ref, $changelog, $confirm ) = @_;
    my ( $conflicts, $remote_profiles, $local_profiles, @overrides );
    my @profiles = @$p_ref;

    my $conflict_msg =
" The following profile(s) selected for upload conflicts with a profile already
 stored in the repository for your account. Please choose whether to keep the
 current version or overwrite it.\n";
    $all = 0;

    readprofiles();
    my ($status, $result) = fetch_profiles_by_user( $config->{url},
                                                    $config->{distro},
                                                    $config->{user}
                                                  );
    if ( $status ) {
        $remote_profiles = $result;
    } else {
        print STDERR sprintf(gettext("ERROR connecting to repository: %s\n"),
                             $result);
        exit;
    }

    $all = 1 if ( grep(/^all$/, @profiles) );

    if ( $all ) {
        $local_profiles = serialize_local_profiles( \%sd );
    } else {
        my $local_sd = {};
        for my $p ( @profiles ) {
            if ( !$sd{$p} ) {
                print STDERR
                    sprintf(gettext("Profile for [%s] does not exist\n"), $p);
                exit;
            }
            $local_sd->{$p} = $sd{$p};
        }
        $local_profiles = serialize_local_profiles( $local_sd );
    }

    $conflicts = find_profile_conflicts($remote_profiles, $local_profiles);

    if ( keys %$conflicts ) {
        @overrides = UI_resolve_profile_conflicts( $conflict_msg, $conflicts );
    }

    if ( $local_profiles ) {
        my @uploads;
        for my $p ( keys %$local_profiles ) {
            unless ( $conflicts->{$p} and !grep(/^$p$/, @overrides) ) {
                print STDERR gettext("Uploading ") .  $p . "... ";
                my ($status,$result) = upload_profile( $config->{url},
                                                       $config->{user},
                                                       $config->{password},
                                                       $config->{distro},
                                                       $p,
                                                       $local_profiles->{$p},
                                                       $changelog
                                                     );
                print STDERR gettext("done") . "\n";
            }
            if ( $status ) {
               push @uploads, $p;
            } else {
              print STDERR gettext("Error uploading") . "$p: $result\n";
            }
        }
        if ( @uploads ) {
            #
            # Currently the upload API with the repository returns the
            # the current users profile set before the update so we have
            # to refetch to obtain the metadata to update the local profiles
            #
            my $repo_p = [];
            print STDERR gettext("Updating local profile metedata....\n");
            my ($status,$result) = fetch_profiles_by_user( $config->{url},
                                                           $config->{distro},
                                                           $config->{user} );
            if ( $status ) {
                for my $p ( @uploads ) {
                   push( @$repo_p, [$p, $result->{$p}] ) if ( $result->{$p} );
                }
                activate_repo_profiles( $config->{url}, $repo_p, 0 );
                print STDERR gettext(" done\n");
            } else {
                print STDERR gettext(
"Failed to retrieve updated profiles from the repository. Error: "
                                    ) . $result . "\n";
            }
        }
    }
}

#
# Helper function for pulling profiles from the repository
# used by --pull and --sync options
#
sub pull_profiles($$$$) {
    my ( $p_ref, $mode, $confirm, $opts ) = @_;
    my @profiles = @$p_ref;
    my ( $conflicts, $commit_list, $remote_profiles,
         $local_profiles, @overrides );

    my $conflict_msg =
"  The following profiles selected for download conflict with profiles
  already deployed on the system. Please choose whether to keep the local
  version or overwrite with the version from the repository\n";

    readprofiles();

    if ( $opts->{id} ) {
        my ($status,$newp) = fetch_profile_by_id( $config->{url}, $opts->{id} );
        if ( ! $status ) {
            print STDERR gettext(
                             sprintf("Error occured during operation\n\t[%s]\n",
                                     $newp
                                    )
                                );
            exit 1;
        } else {
            $remote_profiles = { $newp->{name} => $newp->{profile} };
        }
    } elsif ( @profiles && $opts->{author} ) {
        $remote_profiles = {};
        for my $p ( @profiles ) {
            my ($status,$profiles) =
                fetch_profiles_by_name_and_user( $config->{url},
                                                 $config->{distro},
                                                 $p,
                                                 $opts->{author} );
            if ( ! $status ) {
                print STDERR gettext(sprintf(
                                     "Error occured during operation\n\t[%s]\n",
                                             $profiles
                                            )
                                    );
                exit 1;
            } else {
                $remote_profiles->{$p} = $profiles->{$p};
            }
        }
    } elsif ( $opts->{author} ) {
        my ($status,$profiles) = fetch_profiles_by_user( $config->{url},
                                                         $config->{distro},
                                                         $opts->{author} );
        if ( ! $status ) {
            print STDERR gettext(sprintf(
                                     "Error occured during operation\n\t[%s]\n",
                                         $profiles
                                        )
                                );
            exit 1;
        } else {
            $remote_profiles = $profiles;
        }
    }
    $local_profiles = serialize_local_profiles( \%sd );
    $conflicts = find_profile_conflicts( $remote_profiles, $local_profiles );
    if ( keys %$conflicts ) {
        @overrides = UI_resolve_profile_conflicts( $conflict_msg, $conflicts );
    }
    for my $p ( keys %$remote_profiles ) {
        unless ( $conflicts->{$p} and !grep(/^$p$/, @overrides) ) {
            $remote_profiles->{$p}->{username} = $opts->{author};
            push @$commit_list, [$p, $remote_profiles->{$p}];
        }
    }

    if ( $commit_list and @$commit_list ) {
        activate_repo_profiles( $config->{url}, $commit_list, $mode );
        system("rcapparmor reload");
    } else {
        UI_Info(gettext("No changes to make"));
    }
}

sub UI_status {

    my ($synched, $unsynched, $local) = @_;
    my $url = $config->{url};
    my $synched_text   = gettext("Synchronized repository profiles:\t\t") .
                                 keys %$synched;
    my $unsynched_text = gettext("Unsynchronized repository profiles:\t") .
                                 keys %$unsynched;
    my $local_text     = gettext("Local only profiles :\t\t\t") . keys %$local;
    my $options        = [ $synched_text, $unsynched_text, $local_text ];
    my $title          = gettext("Profile Status");
    my $explanation    = gettext(
" This is the current status of active profiles on the system.
 To view the profiles or unsyncronized changes select VIEW\n"
    );
    my $q = {};
    $q->{title} = $title;
    $q->{headers} = [ "Repository", $url, ];
    $q->{explanation} = $explanation;
    $q->{functions} = [ "CMD_VIEW", "CMD_FINISHED", ];
    $q->{default} = "CMD_FINISHED";
    $q->{options} = $options;
    $q->{selected} = 0;

    my ($ans, $arg);
    do {
        ($ans, $arg) = UI_PromptUser($q);

        if ($ans eq "CMD_VIEW") {
            if ( $arg == 0 ) {
                UI_display_profiles(
                    gettext("Profiles stored in the repository"),
                    $synched
                );
            } elsif ( $arg == 1 ) {
                UI_display_profile_conflicts(
                                      gettext("Unsyncronised profile changes"),
                                      $unsynched
                                            );
            } elsif ( $arg == 2 ) {
                UI_display_profiles(
                    gettext("Profiles stored in the repository"),
                    $local
                );
            }
        }
    } until $ans =~ /^CMD_FINSHED/;
}


