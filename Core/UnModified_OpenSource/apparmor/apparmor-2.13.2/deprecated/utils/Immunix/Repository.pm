# ----------------------------------------------------------------------
#    Copyright (c) 2008 Dominic Reynolds
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

package Immunix::Repository;

use strict;
use warnings;

use Carp;
use Cwd qw(cwd realpath);
use Data::Dumper;
use File::Basename;
use File::Temp qw/ tempfile tempdir /;
use Immunix::Config;
use Locale::gettext;
use POSIX;
use RPC::XML;
use RPC::XML::Client;


require Exporter;
our @ISA    = qw(Exporter);
our @EXPORT = qw(
    get_repo_client
    did_result_succeed
    get_result_error
    user_login
    user_register
    upload_profile
    fetch_profile_by_id
    fetch_profiles_by_user
    fetch_profiles_by_name
    fetch_profiles_by_name_and_user
    fetch_newer_profile
    get_repo_config
    set_repo_config
);

our %clients;
our %uid2login;
our $DEBUGGING = 0;
our $repo_cfg;
our $aa_cfg;

sub get_repo_client ($) {
    my $repo_url = shift;
    unless ( $clients{$repo_url} ) {
        $clients{$repo_url} = new RPC::XML::Client $repo_url;
    }
    return $clients{$repo_url};
}

sub did_result_succeed {
    my $result = shift;

    my $ref = ref $result;
    return ($ref && $ref ne "RPC::XML::fault") ? 1 : 0;
}

sub get_result_error {
    my $result = shift;

    if (ref $result) {
        if (ref $result eq "RPC::XML::fault") {
            $result = $result->string;
        } else {
            $result = $$result;
        }
    }
    return $result;
}

sub user_login ($$$) {
    my ($repo_url,$user,$pass) = @_;
    my ($status,$detail);
    my $repo_client = get_repo_client( $repo_url );
    if ( $repo_client ) {
        my $res = $repo_client->send_request('LoginConfirm', $user, $pass);
        if (did_result_succeed($res)) {
            $status = 1;
            $detail = "";
        } else {
            $status = 0;
            $detail = get_result_error($res);
        }
    }
    return $status,$detail;
}


sub user_register ($$$$) {
    my ($repo_url,$user,$pass,$email) = @_;
    my $repo_client = get_repo_client( $repo_url );
    my ($status,$detail);
    if ( $repo_client ) {
        my $res = $repo_client->send_request('Signup', $user, $pass, $email);
        if (did_result_succeed($res)) {
            $status = 1;
            $detail = "";
        } else {
            $status  = 0;
            $detail = get_result_error($res);
        }
    }
    return $status,$detail;
}

sub upload_profile ($$$$$$$) {
    my ($repo_url,$user,$pass,$distro,$pname,$profile,$changelog) = @_;
    my ($status,$detail);
    my $repo_client = get_repo_client( $repo_url );
    my $res = $repo_client->send_request( 'Create', $user, $pass, $distro,
                                          $pname, $profile, $changelog);
    if (did_result_succeed($res)) {
       $detail = $res->value;
       $status  = 1;
    } else {
       $detail = get_result_error($res);
       $status  = 0;
    }
    return $status,$detail;
}

sub fetch_profile_by_id ($$) {
    my ($repo_url,$id) = @_;
    my $repo_client = get_repo_client( $repo_url );
    my $repo_profile;
    my ($status,$detail);
    my $res = $repo_client->send_request('Show', $id);
    if (did_result_succeed($res)) {
        $status = 1;
        $detail = $res->value();
    } else {
        $status  = 0;
        $detail = get_result_error($res);
    }

    return $status, $detail;
}


sub fetch_profiles ($$$$) {
    my ($repo_url,$distro,$username,$fqdn) = @_;
    my $p_hash = {};
    my ($status,$detail);
    my $repo_client = get_repo_client( $repo_url );
    my $res =
      $repo_client->send_request('FindProfiles', $distro, $fqdn, $username);
    if (did_result_succeed($res)) {
        $status = 1;
        for my $p ( @$res ) {
            my $p_repo = $p->{profile}->value();
            $p_repo =~ s/flags=\(complain\)// if ( $p_repo );  #strip complain flag
            $p->{profile}             = $p_repo;
            $p->{user_id}             = $p->{user_id}->value();
            $p->{id}                  = $p->{id}->value();
            $p->{name}                = $p->{name}->value();
            $p->{created_at}          = $p->{created_at}->value();
            $p->{downloaded_count}    = $p->{downloaded_count}->value();
        }
        $detail = $res;
    } else {
        $status = 0;
        $detail = get_result_error($res);
    }
    return $status,$detail;
}

sub fetch_profiles_by_user ($$$) {
    my ($repo_url,$distro,$username) = @_;
    my $p_hash = {};
    my ($status,$detail) = fetch_profiles( $repo_url, $distro, $username, "" );
    if ( $status ) {
        for my $p ( @$detail ) {
            my $p_repo = $p->{profile};
            if ($p_repo ne "") {
                $p->{username} = $username;
                $p_hash->{$p->{name}} = $p;
            }
        }
    } else {
        return ($status,$detail);
    }
    return($status,$p_hash);
}


sub fetch_profiles_by_name_and_user ($$$$) {
    my ($repo_url,$distro,$fqdbin, $username) = @_;
    my $p_hash = {};
    my ($status,$detail) = fetch_profiles( $repo_url, $distro, $username, $fqdbin );
    if ( $status ) {
        for my $p ( @$detail ) {
            my $p_repo = $p->{profile}?$p->{profile}:"";
            $p_hash->{$p->{name}} = $p if ($p_repo ne "");
        }
    } else {
        return ($status,$detail);
    }
    return($status,$p_hash);
}


sub fetch_profiles_by_name ($$$) {
    my ($repo_url,$distro,$fqdbin) = @_;
    my ($status,$detail,$data);
    $detail = {};
    ($status,$data) = fetch_profiles( $repo_url, $distro, "", $fqdbin);
    if ($status) {
        my @uids;
        for my $p (@$data) {
            push @uids, $p->{user_id};
        }
        my ($status_unames,$unames) = fetch_usernames_from_uids($repo_url, @uids);
        if ( $status_unames ) {
            for my $p (@$data) {
                if ( $unames->{$p->{user_id}} ) {
                    $p->{username} = $unames->{$p->{user_id}};
                } else {
                    $p->{username} = "unkown-" . $p->{user_id};
                }
            }

        } else {
            print STDOUT "ERROR UID\n";
        }
        for my $p (@$data) {
            $p->{profile_type} = "REPOSITORY";
            $detail->{$p->{username}} = $p;
        }
    } else {
        $detail = $data;
    }
    return $status,$detail;
}


sub fetch_newer_profile ($$$$$) {
    my ($repo_url,$distro,$user,$id,$profile)  = @_;
    my $repo_client = get_repo_client( $repo_url );
    my $p;
    my ($status,$detail);

    if ($repo_client) {
        my $res =
          $repo_client->send_request('FindProfiles', $distro, $profile, $user);
        if (did_result_succeed($res)) {
            my @profiles;
            my @profile_list = @{$res->value};
            $status = 1;

            if (@profile_list) {
                if ($profile_list[0]->{id} > $id) {
                    $p = $profile_list[0];
                }
            }
            $detail = $p;
        } else {
            $status = 0;
            $detail = get_result_error($res);
        }
    }
    return $status,$detail;
}

sub fetch_usernames_from_uids ($) {
    my ($repo_url,@searchuids) = @_;
    my ($status,$result) = (1,{});
    my @uids;

    for my $uid ( @searchuids ) {
        if ( $uid2login{$uid} ) {
            $result->{$uid} = $uid2login{$uid};
        } else {
            push @uids, $uid;
        }
    }
    if (@uids) {
        my $repo_client = get_repo_client( $repo_url );
	#RPC::XML will serialize the array into XML with the is_utf8 flag set
	#which causes, HTTP:Message to fail.  Looping on the array elements
	#stops this from happening, and since these are all numbers it
	#will not cause problems.
	for my $foo (@uids) {
	    Encode::_utf8_off($foo);
	}
        my $res = $repo_client->send_request('LoginNamesFromUserIds', [@uids]);
        if (did_result_succeed($res)) {
            my @usernames = @{ $res->value };
            for my $uid (@uids) {
                my $username = shift @usernames;
                $uid2login{$uid} = $username;
                $result->{$uid} = $uid2login{$uid};
            }
        } else {
            $status = 0;
            $result = get_result_error($res);
        }
    }
    return $status,$result;
}

sub get_repo_config {
    unless ( $repo_cfg ) {
       $repo_cfg = Immunix::Config::read_config("repository.conf");
    }
    unless ( $aa_cfg ) {
       $aa_cfg = Immunix::Config::read_config("logprof.conf");
    }
    return {
              "url"      => $aa_cfg->{repository}{url},
              "distro"   => $aa_cfg->{repository}{distro},
              "enabled"  => $repo_cfg->{repository}{enabled},
              "upload"   => $repo_cfg->{repository}{upload},
              "user"     => $repo_cfg->{repository}{user},
              "password" => $repo_cfg->{repository}{pass},
              "email"    => $repo_cfg->{repository}{email}
            };
}

sub set_repo_config ($) {
    my $cfg = shift;
    my ($url,$distro,$enabled,$upload,$user,$pass);
    unless ( $repo_cfg ) {
       $repo_cfg = Immunix::Config::read_config("repository.conf");
    }
    unless ( $aa_cfg ) {
       $aa_cfg = Immunix::Config::read_config("logprof.conf");
    }
    $repo_cfg->{repository}{enabled} = $cfg->{enabled} if ( $cfg->{enabled} );
    $repo_cfg->{repository}{upload}  = $cfg->{upload}  if ( $cfg->{upload} );
    $repo_cfg->{repository}{user}    = $cfg->{user}    if ( $cfg->{user} );
    $repo_cfg->{repository}{pass}    = $cfg->{password}if ( $cfg->{password} );
    $repo_cfg->{repository}{email}   = $cfg->{email}   if ( $cfg->{email} );
    $aa_cfg->{repository}{distro}    = $cfg->{distro}  if ( $cfg->{distro} );
    $aa_cfg->{repository}{url}       = $cfg->{url}     if ( $cfg->{url} );
    write_config("repository.conf", $repo_cfg);
    write_config("logprof.conf", $aa_cfg);
}


1;
