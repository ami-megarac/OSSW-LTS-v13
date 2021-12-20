# ----------------------------------------------------------------------
#    Copyright (c) 2006 Novell, Inc. All Rights Reserved.
#    Copyright (c) 2010 Canonical, Ltd.
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

package Immunix::AppArmor;

use strict;
use warnings;

use Carp;
use Cwd qw(cwd realpath);
use File::Basename;
use File::Temp qw/ tempfile tempdir /;
use Data::Dumper;

use Locale::gettext;
use POSIX;
use Storable qw(dclone);

use Term::ReadKey;

use Immunix::Severity;
use Immunix::Repository;
use Immunix::Config;
use LibAppArmor;

require Exporter;
our @ISA    = qw(Exporter);
our @EXPORT = qw(
    %sd
    %qualifiers
    %include
    %helpers

    $filename
    $profiledir
    $parser
    $logger
    $UI_Mode
    $running_under_genprof

    which
    getprofilefilename
    get_full_path
    fatal_error
    get_pager

    getprofileflags
    setprofileflags
    complain
    enforce

    autodep
    reload

    UI_GetString
    UI_GetFile
    UI_YesNo
    UI_ShortMessage
    UI_LongMessage

    UI_Important
    UI_Info
    UI_PromptUser
    display_changes
    getkey

    do_logprof_pass

    loadincludes
    readprofile
    readprofiles
    writeprofile
    serialize_profile
    attach_profile_data
    parse_repo_profile
    activate_repo_profiles

    check_for_subdomain

    setup_yast
    shutdown_yast
    GetDataFromYast
    SendDataToYast

    checkProfileSyntax
    checkIncludeSyntax
    check_qualifiers

    isSkippableFile
    isSkippableDir
);

our $confdir = "/etc/apparmor";

our $running_under_genprof = 0;

our $DEBUGGING;

our $unimplemented_warning = 0;

# keep track of if we're running under yast or not - default to text mode
our $UI_Mode = "text";

our $sevdb;

# initialize Term::ReadLine if it's available
our $term;
eval {
    require Term::ReadLine;
    import Term::ReadLine;
    $term = new Term::ReadLine 'AppArmor';
};

# initialize the local poo
setlocale(LC_MESSAGES, "")
    unless defined(LC_MESSAGES);
textdomain("apparmor-utils");

# where do we get our log messages from?
our $filename;

our $cfg;
our $repo_cfg;

our $parser;
our $ldd;
our $logger;
our $profiledir;
our $extraprofiledir;

# we keep track of the included profile fragments with %include
my %include;

my %existing_profiles;

our $seenevents = 0;


# these are globs that the user specifically entered.  we'll keep track of
# them so that if one later matches, we'll suggest it again.
our @userglobs;

### THESE VARIABLES ARE USED WITHIN LOGPROF
our %t;
our %transitions;
our %sd;    # we keep track of the original profiles in %sd
our %original_sd;
our %extras;  # inactive profiles from extras

my @log;
my %pid;

my %seen;
my %profilechanges;
my %prelog;
my %log;
my %changed;
my @created;
my %skip;
our %helpers;    # we want to preserve this one between passes

### THESE VARIABLES ARE USED WITHIN LOGPROF

my %filelist;   # file level stuff including variables in config files

my $AA_MAY_EXEC = 1;
my $AA_MAY_WRITE = 2;
my $AA_MAY_READ = 4;
my $AA_MAY_APPEND = 8;
my $AA_MAY_LINK = 16;
my $AA_MAY_LOCK = 32;
my $AA_EXEC_MMAP = 64;
my $AA_EXEC_UNSAFE = 128;
my $AA_EXEC_INHERIT = 256;
my $AA_EXEC_UNCONFINED = 512;
my $AA_EXEC_PROFILE = 1024;
my $AA_EXEC_CHILD = 2048;
my $AA_EXEC_NT = 4096;
my $AA_LINK_SUBSET = 8192;

my $AA_OTHER_SHIFT = 14;
my $AA_USER_MASK = 16384 -1;

my $AA_EXEC_TYPE = $AA_MAY_EXEC | $AA_EXEC_UNSAFE | $AA_EXEC_INHERIT |
		    $AA_EXEC_UNCONFINED | $AA_EXEC_PROFILE | $AA_EXEC_CHILD | $AA_EXEC_NT;

my $ALL_AA_EXEC_TYPE = $AA_EXEC_TYPE;

my %MODE_HASH = (
    x => $AA_MAY_EXEC,
    X => $AA_MAY_EXEC,
    w => $AA_MAY_WRITE,
    W => $AA_MAY_WRITE,
    r => $AA_MAY_READ,
    R => $AA_MAY_READ,
    a => $AA_MAY_APPEND,
    A => $AA_MAY_APPEND,
    l => $AA_MAY_LINK,
    L => $AA_MAY_LINK,
    k => $AA_MAY_LOCK,
    K => $AA_MAY_LOCK,
    m => $AA_EXEC_MMAP,
    M => $AA_EXEC_MMAP,
#   Unsafe => 128,
    i => $AA_EXEC_INHERIT,
    I => $AA_EXEC_INHERIT,
    u => $AA_EXEC_UNCONFINED + $AA_EXEC_UNSAFE,		# U + Unsafe
    U => $AA_EXEC_UNCONFINED,
    p => $AA_EXEC_PROFILE + $AA_EXEC_UNSAFE,		# P + Unsafe
    P => $AA_EXEC_PROFILE,
    c => $AA_EXEC_CHILD + $AA_EXEC_UNSAFE,
    C => $AA_EXEC_CHILD,
    n => $AA_EXEC_NT + $AA_EXEC_UNSAFE,
    N => $AA_EXEC_NT,
    );


# Currently only used by netdomain but there's no reason it couldn't
# be extended to support other types.
my %operation_types = (

	# Old socket names
	"socket_create",	=> "net",
	"socket_post_create"	=> "net",
	"socket_bind"		=> "net",
	"socket_connect"	=> "net",
	"socket_listen"		=> "net",
	"socket_accept"		=> "net",
	"socket_sendmsg"	=> "net",
	"socket_recvmsg"	=> "net",
	"socket_getsockname"	=> "net",
	"socket_getpeername"	=> "net",
	"socket_getsockopt"	=> "net",
	"socket_setsockopt"	=> "net",
	"socket_shutdown"	=> "net",

	# New socket names
	"create"		=> "net",
	"post_create"		=> "net",
	"bind"			=> "net",
	"connect"		=> "net",
	"listen"		=> "net",
	"accept"		=> "net",
	"sendmsg"		=> "net",
	"recvmsg"		=> "net",
	"getsockname"		=> "net",
	"getpeername"		=> "net",
	"getsockopt"		=> "net",
	"setsockopt"		=> "net",
	"sock_shutdown"		=> "net",
);

sub optype($) {
	my $op = shift;
	my $type = $operation_types{$op};

	return "unknown" if !defined($type);
	return $type;
}

sub debug ($) {
    my $message = shift;
    chomp($message);

    print DEBUG "$message\n" if $DEBUGGING;
}

my %arrows = ( A => "UP", B => "DOWN", C => "RIGHT", D => "LEFT" );

sub getkey() {
    # change to raw mode
    ReadMode(4);

    my $key = ReadKey(0);

    # decode arrow key control sequences
    if ($key eq "\x1B") {
        $key = ReadKey(0);
        if ($key eq "[") {
            $key = ReadKey(0);
            if ($arrows{$key}) {
                $key = $arrows{$key};
            }
        }
    }

    # return to cooked mode
    ReadMode(0);
    return $key;
}

BEGIN {
    # set things up to log extra info if they want...
    if ($ENV{LOGPROF_DEBUG}) {
        $DEBUGGING = 1;
        open(DEBUG, ">>$ENV{LOGPROF_DEBUG}");
        my $oldfd = select(DEBUG);
        $| = 1;
        select($oldfd);
    } else {
        $DEBUGGING = 0;
    }
}

END {
    $DEBUGGING && debug "Exiting...";

    # close the debug log if necessary
    close(DEBUG) if $DEBUGGING;
}

# returns true if the specified program contains references to LD_PRELOAD or
# LD_LIBRARY_PATH to give the PX/UX code better suggestions
sub check_for_LD_XXX ($) {
    my $file = shift;

    return undef unless -f $file;

    # limit our checking to programs/scripts under 10k to speed things up a bit
    my $size = -s $file;
    return undef unless ($size && $size < 10000);

    my $found = undef;
    if (open(F, $file)) {
        while (<F>) {
            $found = 1 if /LD_(PRELOAD|LIBRARY_PATH)/;
        }
        close(F);
    }

    return $found;
}

sub fatal_error ($) {
    my $message = shift;

    my $details = "$message\n";

    if ($DEBUGGING) {

        # we'll include the stack backtrace if we're debugging...
        $details = Carp::longmess($message);

        # write the error to the log
        print DEBUG $details;
    }

    # we'll just shoot ourselves in the head if it was one of the yast
    # interface functions that ran into an error.  it gets really ugly if
    # the yast frontend goes away and we try to notify the user of that
    # problem by trying to send the yast frontend a pretty dialog box
    my $caller = (caller(1))[3];

    exit 1 if defined($caller) && $caller =~ /::(Send|Get)Data(To|From)Yast$/;

    # tell the user what the hell happened
    UI_Important($details);

    # make sure the frontend exits cleanly...
    shutdown_yast();

    # die a horrible flaming death
    exit 1;
}

sub setup_yast() {

    # set up the yast connection if we're running under yast...
    if ($ENV{YAST_IS_RUNNING}) {

        # load the yast module if available.
        eval { require ycp; };
        unless ($@) {
            import ycp;

            $UI_Mode = "yast";

            # let the frontend know that we're starting
            SendDataToYast({
                type   => "initial_handshake",
                status => "backend_starting"
            });

            # see if the frontend is just starting up also...
            my ($ypath, $yarg) = GetDataFromYast();
            unless ($yarg
                && (ref($yarg)      eq "HASH")
                && ($yarg->{type}   eq "initial_handshake")
                && ($yarg->{status} eq "frontend_starting"))
            {

                # something's broken, die a horrible, painful death
                fatal_error "Yast frontend is out of sync from backend agent.";
            }
            $DEBUGGING && debug "Initial handshake ok";

            # the yast connection seems to be working okay
            return 1;
        }

    }

    # couldn't init yast
    return 0;
}

sub shutdown_yast() {
    if ($UI_Mode eq "yast") {
        SendDataToYast({ type => "final_shutdown" });
        my ($ypath, $yarg) = GetDataFromYast();
    }
}

sub check_for_subdomain () {

    my ($support_subdomainfs, $support_securityfs);
    if (open(MOUNTS, "/proc/filesystems")) {
        while (<MOUNTS>) {
            $support_subdomainfs = 1 if m/subdomainfs/;
            $support_securityfs  = 1 if m/securityfs/;
        }
        close(MOUNTS);
    }

    my $sd_mountpoint = "";
    if (open(MOUNTS, "/proc/mounts")) {
        while (<MOUNTS>) {
            if ($support_subdomainfs) {
                $sd_mountpoint = $1 if m/^\S+\s+(\S+)\s+subdomainfs\s/;
            } elsif ($support_securityfs) {
                if (m/^\S+\s+(\S+)\s+securityfs\s/) {
                    if (-e "$1/apparmor") {
                        $sd_mountpoint = "$1/apparmor";
                    } elsif (-e "$1/subdomain") {
                        $sd_mountpoint = "$1/subdomain";
                    }
                }
            }
        }
        close(MOUNTS);
    }

    # make sure that subdomain is actually mounted there
    $sd_mountpoint = undef unless -f "$sd_mountpoint/profiles";

    return $sd_mountpoint;
}

sub check_for_apparmor () {
	return check_for_subdomain();
}

sub which ($) {
    my $file = shift;

    foreach my $dir (split(/:/, $ENV{PATH})) {
        return "$dir/$file" if -x "$dir/$file";
    }

    return undef;
}

# we need to convert subdomain regexps to perl regexps
sub convert_regexp ($) {
    my $regexp = shift;

    # escape regexp-special characters we don't support
    $regexp =~ s/(?<!\\)(\.|\+|\$)/\\$1/g;

    # * and ** globs can't collapse to match an empty string when they're
    # the only part of the glob at a specific directory level, which
    # complicates things a little.

    # ** globs match multiple directory levels
    $regexp =~ s{(?<!\\)\*\*+}{
      my ($pre, $post) = ($`, $');
      if (($pre =~ /\/$/) && (!$post || $post =~ /^\//)) {
        'SD_INTERNAL_MULTI_REQUIRED';
      } else {
        'SD_INTERNAL_MULTI_OPTIONAL';
      }
    }gex;

    # convert * globs to match anything at the current path level
    $regexp =~ s{(?<!\\)\*}{
      my ($pre, $post) = ($`, $');
      if (($pre =~ /\/$/) && (!$post || $post =~ /^\//)) {
        'SD_INTERNAL_SINGLE_REQUIRED';
      } else {
        'SD_INTERNAL_SINGLE_OPTIONAL';
      }
    }gex;

    # convert ? globs to match a single character at current path level
    $regexp =~ s/(?<!\\)\?/[^\/]/g;

    # convert {foo,baz} to (foo|baz)
    $regexp =~ y/\{\}\,/\(\)\|/ if $regexp =~ /\{.*\,.*\}/;

    # convert internal markers to their appropriate regexp equivalents
    $regexp =~ s/SD_INTERNAL_SINGLE_OPTIONAL/[^\/]*/g;
    $regexp =~ s/SD_INTERNAL_SINGLE_REQUIRED/[^\/]+/g;
    $regexp =~ s/SD_INTERNAL_MULTI_OPTIONAL/.*/g;
    $regexp =~ s/SD_INTERNAL_MULTI_REQUIRED/[^\/].*/g;

    return $regexp;
}

sub get_full_path ($) {
    my $originalpath = shift;

    my $path = $originalpath;

    # keep track so we can break out of loops
    my $linkcount = 0;

    # if we don't have any directory foo, look in the current dir
    $path = cwd() . "/$path" if $path !~ m/\//;

    # beat symlinks into submission
    while (-l $path) {

        if ($linkcount++ > 64) {
            fatal_error "Followed too many symlinks resolving $originalpath";
        }

        # split out the directory/file components
        if ($path =~ m/^(.*)\/(.+)$/) {
            my ($dir, $file) = ($1, $2);

            # figure out where the link is pointing...
            my $link = readlink($path);
            if ($link =~ /^\//) {
                # if it's an absolute link, just replace it
                $path = $link;
            } else {
                # if it's relative, let abs_path handle it
                $path = $dir . "/$link";
            }
        }
    }

    return realpath($path);
}

sub findexecutable ($) {
    my $bin = shift;

    my $fqdbin;
    if (-e $bin) {
        $fqdbin = get_full_path($bin);
        chomp($fqdbin);
    } else {
        if ($bin !~ /\//) {
            my $which = which($bin);
            if ($which) {
                $fqdbin = get_full_path($which);
            }
        }
    }

    unless ($fqdbin && -e $fqdbin) {
        return undef;
    }

    return $fqdbin;
}

sub name_to_prof_filename($) {
    my $bin    = shift;
    my $filename;

    unless ($bin =~ /^($profiledir)/) {
	my $fqdbin = findexecutable($bin);
	if ($fqdbin) {
	    $filename = getprofilefilename($fqdbin);
	    return ($filename, $fqdbin) if -f $filename;
	}
    }

    if ($bin =~ /^$profiledir(.*)/) {
	my $profile = $1;
	return ($bin, $profile);
    } elsif ($bin =~ /^\//) {
	$filename = getprofilefilename($bin);
	return ($filename, $bin);
    } else {
	# not an absolute path try it as a profile_
	$bin = $1 if ($bin !~ /^profile_(.*)/);
	$filename = getprofilefilename($bin);
	return ($filename, "profile_${bin}");
    }
    return undef;
}

sub complain ($) {
    my $bin = shift;

    return if (!$bin);

    my ($filename, $name) = name_to_prof_filename($bin)
	or fatal_error(sprintf(gettext('Can\'t find %s.'), $bin));

    UI_Info(sprintf(gettext('Setting %s to complain mode.'), $name));

    setprofileflags($filename, "complain");
}

sub enforce ($) {
    my $bin = shift;

    return if (!$bin);

    my ($filename, $name) = name_to_prof_filename($bin)
	or fatal_error(sprintf(gettext('Can\'t find %s.'), $bin));

    UI_Info(sprintf(gettext('Setting %s to enforce mode.'), $name));

    setprofileflags($filename, "");
}

sub head ($) {
    my $file = shift;

    my $first = "";
    if (open(FILE, $file)) {
        $first = <FILE>;
        close(FILE);
    }

    return $first;
}

sub get_output ($@) {
    my ($program, @args) = @_;

    my $ret = -1;

    my $pid;
    my @output;

    if (-x $program) {
        $pid = open(KID_TO_READ, "-|");
        unless (defined $pid) {
            fatal_error "can't fork: $!";
        }

        if ($pid) {
            while (<KID_TO_READ>) {
                chomp;
                push @output, $_;
            }
            close(KID_TO_READ);
            $ret = $?;
        } else {
            ($>, $)) = ($<, $();
            open(STDERR, ">&STDOUT")
              || fatal_error "can't dup stdout to stderr";
            exec($program, @args) || fatal_error "can't exec program: $!";

            # NOTREACHED
        }
    }

    return ($ret, @output);
}

sub get_reqs ($) {
    my $file = shift;

    my @reqs;
    my ($ret, @ldd) = get_output($ldd, $file);

    if ($ret == 0) {
        for my $line (@ldd) {
            last if $line =~ /not a dynamic executable/;
            last if $line =~ /cannot read header/;
            last if $line =~ /statically linked/;

            # avoid new kernel 2.6 poo
            next if $line =~ /linux-(gate|vdso(32|64)).so/;

            if ($line =~ /^\s*\S+ => (\/\S+)/) {
                push @reqs, $1;
            } elsif ($line =~ /^\s*(\/\S+)/) {
                push @reqs, $1;
            }
        }
    }

    return @reqs;
}

sub handle_binfmt ($$) {
    my ($profile, $fqdbin) = @_;

    my %reqs;
    my @reqs = get_reqs($fqdbin);

    while (my $library = shift @reqs) {

        $library = get_full_path($library);

        push @reqs, get_reqs($library) unless $reqs{$library}++;

        # does path match anything pulled in by includes in original profile?
        my $combinedmode = match_prof_incs_to_path($profile, 'allow', $library);

        # if we found any matching entries, do the modes match?
        next if $combinedmode;

        $library = globcommon($library);
        chomp $library;
        next unless $library;

        $profile->{allow}{path}->{$library}{mode} |= str_to_mode("mr");
        $profile->{allow}{path}->{$library}{audit} |= 0;
    }
}

sub get_inactive_profile($) {
    my $fqdbin = shift;
    if ( $extras{$fqdbin} ) {
        return {$fqdbin => $extras{$fqdbin}};
    }
}



sub create_new_profile($) {
    my $fqdbin = shift;

    my $profile;
    $profile = {
	$fqdbin => {
	    flags   => "complain",
	    include => { "abstractions/base" => 1    },
	}
    };

    # if the executable exists on this system, pull in extra dependencies
    if (-f $fqdbin) {
        my $hashbang = head($fqdbin);
        if ($hashbang && $hashbang =~ /^#!\s*(\S+)/) {
            my $interpreter = get_full_path($1);
            $profile->{$fqdbin}{allow}{path}->{$fqdbin}{mode} |= str_to_mode("r");
            $profile->{$fqdbin}{allow}{path}->{$fqdbin}{audit} |= 0;
            $profile->{$fqdbin}{allow}{path}->{$interpreter}{mode} |= str_to_mode("ix");
            $profile->{$fqdbin}{allow}{path}->{$interpreter}{audit} |= 0;
            if ($interpreter =~ /perl/) {
                $profile->{$fqdbin}{include}->{"abstractions/perl"} = 1;
            } elsif ($interpreter =~ m/\/bin\/(bash|dash|sh)/) {
                $profile->{$fqdbin}{include}->{"abstractions/bash"} = 1;
            } elsif ($interpreter =~ m/python/) {
                $profile->{$fqdbin}{include}->{"abstractions/python"} = 1;
            } elsif ($interpreter =~ m/ruby/) {
                $profile->{$fqdbin}{include}->{"abstractions/ruby"} = 1;
            }
            handle_binfmt($profile->{$fqdbin}, $interpreter);
        } else {
          $profile->{$fqdbin}{allow}{path}->{$fqdbin}{mode} |= str_to_mode("mr");
          $profile->{$fqdbin}{allow}{path}->{$fqdbin}{audit} |= 0;
          handle_binfmt($profile->{$fqdbin}, $fqdbin);
        }
    }

    # create required infrastructure hats if it's a known change_hat app
    for my $hatglob (keys %{$cfg->{required_hats}}) {
        if ($fqdbin =~ /$hatglob/) {
            for my $hat (sort split(/\s+/, $cfg->{required_hats}{$hatglob})) {
                $profile->{$hat} = { flags => "complain" };
            }
        }
    }
    push @created, $fqdbin;
    $DEBUGGING && debug( Data::Dumper->Dump([$profile], [qw(*profile)]));
    return { $fqdbin => $profile };
}

sub delete_profile ($) {
    my $profile = shift;
    my $profilefile = getprofilefilename( $profile );
    if ( -e $profilefile ) {
      unlink( $profilefile );
    }
    if ( defined $sd{$profile} ) {
        delete $sd{$profile};
    }
}

sub get_profile($) {
    my $fqdbin = shift;
    my $profile_data;

    my $distro     = $cfg->{repository}{distro};
    my $repo_url   = $cfg->{repository}{url};
    my @profiles;
    my %profile_hash;

    if (repo_is_enabled()) {
       my $results;
       UI_BusyStart( gettext("Connecting to repository.....") );

       my ($status_ok,$ret) =
           fetch_profiles_by_name($repo_url, $distro, $fqdbin );
       UI_BusyStop();
       if ( $status_ok ) {
           %profile_hash = %$ret;
       } else {
           my $errmsg =
             sprintf(gettext("WARNING: Error fetching profiles from the repository:\n%s\n"),
                     $ret?$ret:gettext("UNKNOWN ERROR"));
           UI_Important( $errmsg );
       }
    }

    my $inactive_profile = get_inactive_profile($fqdbin);
    if ( defined $inactive_profile && $inactive_profile ne "" ) {
        # set the profile to complain mode
        my $uname = gettext( "Inactive local profile for ") . $fqdbin;
        $inactive_profile->{$fqdbin}{$fqdbin}{flags} = "complain";
	# inactive profiles store where they came from
	delete $inactive_profile->{$fqdbin}{$fqdbin}{filename};
        $profile_hash{$uname} =
            {
              "username"     => $uname,
              "profile_type" => "INACTIVE_LOCAL",
              "profile"      => serialize_profile($inactive_profile->{$fqdbin},
                                  $fqdbin
                                ),
              "profile_data" => $inactive_profile,
            };
    }

    return undef if ( keys %profile_hash == 0 ); # No repo profiles, no inactive
                                            # profile
    my @options;
    my @tmp_list;
    my $preferred_present = 0;
    my $preferred_user  = $cfg->{repository}{preferred_user} || "NOVELL";

    foreach my $p ( keys %profile_hash ) {
        if ( $profile_hash{$p}->{username} eq $preferred_user ) {
             $preferred_present = 1;
        } else {
            push @tmp_list, $profile_hash{$p}->{username};
        }
    }

    if ( $preferred_present ) {
        push  @options, $preferred_user;
    }
    push  @options, @tmp_list;

    my $q = {};
    $q->{headers} = [];
    push @{ $q->{headers} }, gettext("Profile"), $fqdbin;

    $q->{functions} = [ "CMD_VIEW_PROFILE", "CMD_USE_PROFILE",
                        "CMD_CREATE_PROFILE", "CMD_ABORT", "CMD_FINISHED" ];

    $q->{default} = "CMD_VIEW_PROFILE";

    $q->{options}  = [@options];
    $q->{selected} = 0;

    my ($p, $ans, $arg);
    do {
        ($ans, $arg) = UI_PromptUser($q);
        $p = $profile_hash{$options[$arg]};
        for (my $i = 0; $i < scalar(@options); $i++) {
            if ($options[$i] eq $options[$arg]) {
                $q->{selected} = $i;
            }
        }

        if ($ans eq "CMD_VIEW_PROFILE") {
            if ($UI_Mode eq "yast") {
                SendDataToYast(
                    {
                        type         => "dialog-view-profile",
                        user         => $options[$arg],
                        profile      => $p->{profile},
                        profile_type => $p->{profile_type}
                    }
                );
                my ($ypath, $yarg) = GetDataFromYast();
            } else {
                my $pager = get_pager();
                open(PAGER, "| $pager");
                print PAGER gettext("Profile submitted by") .
                                    " $options[$arg]:\n\n" . $p->{profile} . "\n\n";
                close(PAGER);
            }
        } elsif ($ans eq "CMD_USE_PROFILE") {
            if ( $p->{profile_type} eq "INACTIVE_LOCAL" ) {
                $profile_data = $p->{profile_data};
                push @created, $fqdbin; # This really is ugly here
                                        # need to find a better place to mark
                                        # this as newly created
            } else {
                $profile_data =
                    parse_repo_profile($fqdbin, $repo_url, $p);
            }
        }
    } until ($ans =~ /^CMD_(USE_PROFILE|CREATE_PROFILE)$/);

    return $profile_data;
}

sub activate_repo_profiles ($$$) {
    my ($url,$profiles,$complain) = @_;

    readprofiles();
    eval {
        for my $p ( @$profiles ) {
            my $pname = $p->[0];
            my $profile_data = parse_repo_profile( $pname, $url, $p->[1] );
            attach_profile_data(\%sd, $profile_data);
            writeprofile($pname);
            if ( $complain ) {
                my $filename = getprofilefilename($pname);
                setprofileflags($filename, "complain");
                UI_Info(sprintf(gettext('Setting %s to complain mode.'),
                                        $pname));
            }
        }
    };
    # if there were errors....
    if ($@) {
        $@ =~ s/\n$//;
        print STDERR sprintf(gettext("Error activating profiles: %s\n"), $@);
    }
}

sub autodep_base($$) {
    my ($bin, $pname) = @_;
    %extras = ();

    $bin = $pname if (! $bin) && ($pname =~ /^\//);

    unless ($repo_cfg || not defined $cfg->{repository}{url}) {
        $repo_cfg = read_config("repository.conf");
        if ( (not defined $repo_cfg->{repository}) ||
             ($repo_cfg->{repository}{enabled} eq "later") ) {
                UI_ask_to_enable_repo();
        }
    }

    my $fqdbin;
    if ($bin) {
	# findexecutable() might fail if we're running on a different system
	# than the logs were collected on.  ugly.  we'll just hope for the best.
	$fqdbin = findexecutable($bin) || $bin;

	# try to make sure we have a full path in case findexecutable failed
	return unless $fqdbin =~ /^\//;

	# ignore directories
	return if -d $fqdbin;
    }

    $pname = $fqdbin if $fqdbin;

    my $profile_data;

    readinactiveprofiles(); # need to read the profiles to see if an
                            # inactive local profile is present
    $profile_data = eval { get_profile($pname) };
    # propagate any errors we hit inside the get_profile call
    if ($@) { die $@; }

    unless ($profile_data) {
        $profile_data = create_new_profile($pname);
    }

    my $file = getprofilefilename($pname);

    # stick the profile into our data structure.
    attach_profile_data(\%sd, $profile_data);
    # and store a "clean" version also so we can display the changes we've
    # made during this run
    attach_profile_data(\%original_sd, $profile_data);

    if (-f "$profiledir/tunables/global") {
        unless (exists $filelist{$file}) {
            $filelist{$file} = { };
        }
        $filelist{$file}{include}{'tunables/global'} = 1; # sorry
    }

    # write out the profile...
    writeprofile_ui_feedback($pname);
}

sub autodep ($) {
    my $bin = shift;
    return autodep_base($bin, "");
}

sub getprofilefilename ($) {
    my $profile = shift;

    my $filename = $profile;
    if ($filename =~ /^\//) {
	$filename =~ s/^\///;                              # strip leading /
    } else {
	$filename = "profile_$filename";
    }
    $filename =~ s/\//./g;                            # convert /'s to .'s

    return "$profiledir/$filename";
}

sub setprofileflags ($$) {
    my $filename = shift;
    my $newflags = shift;

    if (open(PROFILE, "$filename")) {
        if (open(NEWPROFILE, ">$filename.new")) {
            while (<PROFILE>) {
                if (m/^(\s*)(("??\/.+?"??)|(profile\s+("??.+?"??)))\s+(flags=\(.+\)\s+)*\{\s*$/) {
                    my ($space, $binary, $flags) = ($1, $2, $6);

                    if ($newflags) {
                        $_ = "$space$binary flags=($newflags) {\n";
                    } else {
                        $_ = "$space$binary {\n";
                    }
                } elsif (m/^(\s*\^\S+)\s+(flags=\(.+\)\s+)*\{\s*$/) {
                    my ($hat, $flags) = ($1, $2);

                    if ($newflags) {
                        $_ = "$hat flags=($newflags) {\n";
                    } else {
                        $_ = "$hat {\n";
                    }
                }
                print NEWPROFILE;
            }
            close(NEWPROFILE);
            rename("$filename.new", "$filename");
        }
        close(PROFILE);
    }
}

sub profile_exists($) {
    my $program = shift || return 0;

    # if it's already in the cache, return true
    return 1 if $existing_profiles{$program};

    # if the profile exists, mark it in the cache and return true
    my $profile = getprofilefilename($program);
    if (-e $profile) {
        $existing_profiles{$program} = 1;
        return 1;
    }

    # couldn't find a profile, so we'll return false
    return 0;
}

sub sync_profiles() {

    my ($user, $pass) = get_repo_user_pass();
    return unless ( $user && $pass );

    my @repo_profiles;
    my @changed_profiles;
    my @new_profiles;
    my $serialize_opts = { };
    my ($status_ok,$ret) =
        fetch_profiles_by_user($cfg->{repository}{url},
                               $cfg->{repository}{distro},
                               $user
                              );
    if ( !$status_ok ) {
        my $errmsg =
          sprintf(gettext("WARNING: Error syncronizing profiles with the repository:\n%s\n"),
                  $ret?$ret:gettext("UNKNOWN ERROR"));
        UI_Important($errmsg);
        return;
    } else {
        my $users_repo_profiles = $ret;
        $serialize_opts->{NO_FLAGS} = 1;
        #
        # Find changes made to non-repo profiles
        #
        for my $profile (sort keys %sd) {
            if (is_repo_profile($sd{$profile}{$profile})) {
                push @repo_profiles, $profile;
            }
            if ( grep(/^$profile$/, @created) )  {
                my $p_local = serialize_profile($sd{$profile},
                                                $profile,
                                                $serialize_opts);
                if ( not defined $users_repo_profiles->{$profile} ) {
                    push @new_profiles,  [ $profile, $p_local, "" ];
                } else {
                    my $p_repo = $users_repo_profiles->{$profile}->{profile};
                    if ( $p_local ne $p_repo ) {
                        push @changed_profiles, [ $profile, $p_local, $p_repo ];
                    }
                }
            }
        }

        #
        # Find changes made to local profiles with repo metadata
        #
        if (@repo_profiles) {
            for my $profile (@repo_profiles) {
                my $p_local = serialize_profile($sd{$profile},
                                                $profile,
                                                $serialize_opts);
                if ( not exists $users_repo_profiles->{$profile} ) {
                    push @new_profiles,  [ $profile, $p_local, "" ];
                } else {
                    my $p_repo = "";
                    if ( $sd{$profile}{$profile}{repo}{user} eq $user ) {
                       $p_repo = $users_repo_profiles->{$profile}->{profile};
                    }  else {
                        my ($status_ok,$ret) =
                            fetch_profile_by_id($cfg->{repository}{url},
                                                $sd{$profile}{$profile}{repo}{id}
                                               );
                        if ( $status_ok ) {
                           $p_repo = $ret->{profile};
                        } else {
                            my $errmsg =
                              sprintf(
                                gettext("WARNING: Error syncronizing profiles with the repository:\n%s\n"),
                                $ret?$ret:gettext("UNKNOWN ERROR"));
                            UI_Important($errmsg);
                            next;
                        }
                    }
                    if ( $p_repo ne $p_local ) {
                        push @changed_profiles, [ $profile, $p_local, $p_repo ];
                    }
                }
            }
        }

        if ( @changed_profiles ) {
           submit_changed_profiles( \@changed_profiles );
        }
        if ( @new_profiles ) {
           submit_created_profiles( \@new_profiles );
        }
    }
}

sub submit_created_profiles($) {
    my $new_profiles = shift;
    my $url = $cfg->{repository}{url};

    if ($UI_Mode eq "yast") {
        my $title       = gettext("New profiles");
        my $explanation =
          gettext("Please choose the newly created profiles that you would".
          " like\nto store in the repository");
        yast_select_and_upload_profiles($title,
                                        $explanation,
                                        $new_profiles);
    } else {
        my $title       =
          gettext("Submit newly created profiles to the repository");
        my $explanation =
          gettext("Would you like to upload the newly created profiles?");
        console_select_and_upload_profiles($title,
                                           $explanation,
                                           $new_profiles);
    }
}

sub submit_changed_profiles($) {
    my $changed_profiles = shift;
    my $url = $cfg->{repository}{url};
    if (@$changed_profiles) {
        if ($UI_Mode eq "yast") {
            my $explanation =
              gettext("Select which of the changed profiles you would".
              " like to upload\nto the repository");
            my $title       = gettext("Changed profiles");
            yast_select_and_upload_profiles($title,
                                            $explanation,
                                            $changed_profiles);
        } else {
            my $title       =
              gettext("Submit changed profiles to the repository");
            my $explanation =
              gettext("The following profiles from the repository were".
              " changed.\nWould you like to upload your changes?");
            console_select_and_upload_profiles($title,
                                               $explanation,
                                               $changed_profiles);
        }
    }
}

sub yast_select_and_upload_profiles($$$) {

    my ($title, $explanation, $profiles_ref) = @_;
    my $url = $cfg->{repository}{url};
    my %profile_changes;
    my @profiles = @$profiles_ref;

    foreach my $prof (@profiles) {
        $profile_changes{ $prof->[0] } =
          get_profile_diff($prof->[2], $prof->[1]);
    }

    my (@selected_profiles, $changelog, $changelogs, $single_changelog);
    SendDataToYast(
        {
            type               => "dialog-select-profiles",
            title              => $title,
            explanation        => $explanation,
            default_select     => "false",
            disable_ask_upload => "true",
            profiles           => \%profile_changes
        }
    );
    my ($ypath, $yarg) = GetDataFromYast();
    if ($yarg->{STATUS} eq "cancel") {
        return;
    } else {
        my $selected_profiles_ref = $yarg->{PROFILES};
        @selected_profiles = @$selected_profiles_ref;
        $changelogs        = $yarg->{CHANGELOG};
        if (defined $changelogs->{SINGLE_CHANGELOG}) {
            $changelog        = $changelogs->{SINGLE_CHANGELOG};
            $single_changelog = 1;
        }
    }

    for my $profile (@selected_profiles) {
        my ($user, $pass) = get_repo_user_pass();
        my $profile_string = serialize_profile($sd{$profile}, $profile);
        if (!$single_changelog) {
            $changelog = $changelogs->{$profile};
        }
        my ($status_ok, $ret) = upload_profile( $url,
                                                $user,
                                                $pass,
                                                $cfg->{repository}{distro},
                                                $profile,
                                                $profile_string,
                                                $changelog
                                              );
        if ($status_ok) {
            my $newprofile = $ret;
            my $newid      = $newprofile->{id};
            set_repo_info($sd{$profile}{$profile}, $url, $user, $newid);
            writeprofile_ui_feedback($profile);
        } else {
            my $errmsg =
              sprintf(
                gettext("WARNING: An error occured while uploading the profile %s\n%s\n"),
                $profile, $ret?$ret:gettext("UNKNOWN ERROR"));
            UI_Important( $errmsg );
        }
    }
    UI_Info(gettext("Uploaded changes to repository."));

    # Check to see if unselected profiles should be marked as local only
    # this is outside of the main repo code as we want users to be able to mark
    # profiles as local only even if they aren't able to connect to the repo.
    if (defined $yarg->{NEVER_ASK_AGAIN}) {
        my @unselected_profiles;
        foreach my $prof (@profiles) {
            if ( grep(/^$prof->[0]$/, @selected_profiles) == 0 ) {
                push @unselected_profiles, $prof->[0];
            }
        }
        set_profiles_local_only( @unselected_profiles );
    }
}

sub console_select_and_upload_profiles($$$) {
    my ($title, $explanation, $profiles_ref) = @_;
    my $url = $cfg->{repository}{url};
    my @profiles = @$profiles_ref;
    my $q = {};
    $q->{title} = $title;
    $q->{headers} = [ gettext("Repository"), $url, ];

    $q->{explanation} = $explanation;

    $q->{functions} = [ "CMD_UPLOAD_CHANGES",
                        "CMD_VIEW_CHANGES",
                        "CMD_ASK_LATER",
                        "CMD_ASK_NEVER",
                        "CMD_ABORT", ];

    $q->{default} = "CMD_VIEW_CHANGES";

    $q->{options} = [ map { $_->[0] } @profiles ];
    $q->{selected} = 0;

    my ($ans, $arg);
    do {
        ($ans, $arg) = UI_PromptUser($q);

        if ($ans eq "CMD_VIEW_CHANGES") {
            display_changes($profiles[$arg]->[2], $profiles[$arg]->[1]);
        }
    } until $ans =~ /^CMD_(UPLOAD_CHANGES|ASK_NEVER|ASK_LATER)/;

    if ($ans eq "CMD_ASK_NEVER") {
        set_profiles_local_only(  map { $_->[0] } @profiles  );
    } elsif ($ans eq "CMD_UPLOAD_CHANGES") {
        my $changelog = UI_GetString(gettext("Changelog Entry: "), "");
        my ($user, $pass) = get_repo_user_pass();
        if ($user && $pass) {
            for my $p_data (@profiles) {
                my $profile          = $p_data->[0];
                my $profile_string   = $p_data->[1];
                my ($status_ok,$ret) =
                    upload_profile( $url,
                                    $user,
                                    $pass,
                                    $cfg->{repository}{distro},
                                    $profile,
                                    $profile_string,
                                    $changelog
                                  );
                if ($status_ok) {
                    my $newprofile = $ret;
                    my $newid      = $newprofile->{id};
                    set_repo_info($sd{$profile}{$profile}, $url, $user, $newid);
                    writeprofile_ui_feedback($profile);
                    UI_Info(
                      sprintf(gettext("Uploaded %s to repository."), $profile)
                    );
                } else {
                    my $errmsg =
                      sprintf(
                        gettext("WARNING: An error occured while uploading the profile %s\n%s\n"),
                        $profile, $ret?$ret:gettext("UNKNOWN ERROR"));
                    UI_Important( $errmsg );
                }
            }
        } else {
            UI_Important(gettext("Repository Error\n" .
                      "Registration or Signin was unsuccessful. User login\n" .
                      "information is required to upload profiles to the\n" .
                      "repository. These changes have not been sent.\n"));
        }
    }
}

#
# Mark the profiles passed in @profiles as local only
# and don't prompt to upload changes to the repository
#
sub set_profiles_local_only(@) {
    my @profiles = @_;
    for my $profile (@profiles) {
         $sd{$profile}{$profile}{repo}{neversubmit} = 1;
         writeprofile_ui_feedback($profile);
    }
}

##########################################################################
# Here are the console/yast interface functions

sub UI_Info ($) {
    my $text = shift;

    $DEBUGGING && debug "UI_Info: $UI_Mode: $text";

    if ($UI_Mode eq "text") {
        print "$text\n";
    } else {
        ycp::y2milestone($text);
    }
}

sub UI_Important ($) {
    my $text = shift;

    $DEBUGGING && debug "UI_Important: $UI_Mode: $text";

    if ($UI_Mode eq "text") {
        print "\n$text\n";
    } else {
        SendDataToYast({ type => "dialog-error", message => $text });
        my ($path, $yarg) = GetDataFromYast();
    }
}

sub UI_YesNo ($$) {
    my $text    = shift;
    my $default = shift;

    $DEBUGGING && debug "UI_YesNo: $UI_Mode: $text $default";

    my $ans;
    if ($UI_Mode eq "text") {

        my $yes = gettext("(Y)es");
        my $no  = gettext("(N)o");

        # figure out our localized hotkeys
        my $usrmsg = "PromptUser: " . gettext("Invalid hotkey for");
        $yes =~ /\((\S)\)/ or fatal_error "$usrmsg '$yes'";
        my $yeskey = lc($1);
        $no =~ /\((\S)\)/ or fatal_error "$usrmsg '$no'";
        my $nokey = lc($1);

        print "\n$text\n";
        if ($default eq "y") {
            print "\n[$yes] / $no\n";
        } else {
            print "\n$yes / [$no]\n";
        }
        $ans = getkey() || (($default eq "y") ? $yeskey : $nokey);

        # convert back from a localized answer to english y or n
        $ans = (lc($ans) eq $yeskey) ? "y" : "n";
    } else {

        SendDataToYast({ type => "dialog-yesno", question => $text });
        my ($ypath, $yarg) = GetDataFromYast();
        $ans = $yarg->{answer} || $default;

    }

    return $ans;
}

sub UI_YesNoCancel ($$) {
    my $text    = shift;
    my $default = shift;

    $DEBUGGING && debug "UI_YesNoCancel: $UI_Mode: $text $default";

    my $ans;
    if ($UI_Mode eq "text") {

        my $yes    = gettext("(Y)es");
        my $no     = gettext("(N)o");
        my $cancel = gettext("(C)ancel");

        # figure out our localized hotkeys
        my $usrmsg = "PromptUser: " . gettext("Invalid hotkey for");
        $yes =~ /\((\S)\)/ or fatal_error "$usrmsg '$yes'";
        my $yeskey = lc($1);
        $no =~ /\((\S)\)/ or fatal_error "$usrmsg '$no'";
        my $nokey = lc($1);
        $cancel =~ /\((\S)\)/ or fatal_error "$usrmsg '$cancel'";
        my $cancelkey = lc($1);

        $ans = "XXXINVALIDXXX";
        while ($ans !~ /^(y|n|c)$/) {
            print "\n$text\n";
            if ($default eq "y") {
                print "\n[$yes] / $no / $cancel\n";
            } elsif ($default eq "n") {
                print "\n$yes / [$no] / $cancel\n";
            } else {
                print "\n$yes / $no / [$cancel]\n";
            }

            $ans = getkey();

            if ($ans) {
                # convert back from a localized answer to english y or n
                $ans = lc($ans);
                if ($ans eq $yeskey) {
                    $ans = "y";
                } elsif ($ans eq $nokey) {
                    $ans = "n";
                } elsif ($ans eq $cancelkey) {
                    $ans = "c";
                }
            } else {
                $ans = $default;
            }
        }
    } else {

        SendDataToYast({ type => "dialog-yesnocancel", question => $text });
        my ($ypath, $yarg) = GetDataFromYast();
        $ans = $yarg->{answer} || $default;

    }

    return $ans;
}

sub UI_GetString ($$) {
    my $text    = shift;
    my $default = shift;

    $DEBUGGING && debug "UI_GetString: $UI_Mode: $text $default";

    my $string;
    if ($UI_Mode eq "text") {

        if ($term) {
            $string = $term->readline($text, $default);
        } else {
            local $| = 1;
            print "$text";
            $string = <STDIN>;
            chomp($string);
        }

    } else {

        SendDataToYast({
            type    => "dialog-getstring",
            label   => $text,
            default => $default
        });
        my ($ypath, $yarg) = GetDataFromYast();
        $string = $yarg->{string};

    }
    return $string;
}

sub UI_GetFile ($) {
    my $f = shift;

    $DEBUGGING && debug "UI_GetFile: $UI_Mode";

    my $filename;
    if ($UI_Mode eq "text") {

        local $| = 1;
        print "$f->{description}\n";
        $filename = <STDIN>;
        chomp($filename);

    } else {

        $f->{type} = "dialog-getfile";

        SendDataToYast($f);
        my ($ypath, $yarg) = GetDataFromYast();
        if ($yarg->{answer} eq "okay") {
            $filename = $yarg->{filename};
        }
    }

    return $filename;
}

sub UI_BusyStart ($) {
    my $message = shift;
    $DEBUGGING && debug "UI_BusyStart: $UI_Mode";

    if ($UI_Mode eq "text") {
      UI_Info( $message );
    } else {
        SendDataToYast({
                        type    => "dialog-busy-start",
                        message => $message,
                       });
        my ($ypath, $yarg) = GetDataFromYast();
    }
}

sub UI_BusyStop()  {
    $DEBUGGING && debug "UI_BusyStop: $UI_Mode";

    if ($UI_Mode ne "text") {
        SendDataToYast({ type    => "dialog-busy-stop" });
        my ($ypath, $yarg) = GetDataFromYast();
    }
}


my %CMDS = (
    CMD_ALLOW            => "(A)llow",
    CMD_OTHER		 => "(M)ore",
    CMD_AUDIT_NEW	 => "Audi(t)",
    CMD_AUDIT_OFF	 => "Audi(t) off",
    CMD_AUDIT_FULL	 => "Audit (A)ll",
    CMD_OTHER		 => "(O)pts",
    CMD_USER_ON		 => "(O)wner permissions on",
    CMD_USER_OFF	 => "(O)wner permissions off",
    CMD_DENY             => "(D)eny",
    CMD_ABORT            => "Abo(r)t",
    CMD_FINISHED         => "(F)inish",
    CMD_ix               => "(I)nherit",
    CMD_px               => "(P)rofile",
    CMD_px_safe		 => "(P)rofile Clean Exec",
    CMD_cx		 => "(C)hild",
    CMD_cx_safe		 => "(C)hild Clean Exec",
    CMD_nx		 => "(N)ame",
    CMD_nx_safe		 => "(N)amed Clean Exec",
    CMD_ux               => "(U)nconfined",
    CMD_ux_safe		 => "(U)nconfined Clean Exec",
    CMD_pix		 => "(P)rofile ix",
    CMD_pix_safe	 => "(P)rofile ix Clean Exec",
    CMD_cix		 => "(C)hild ix",
    CMD_cix_safe	 => "(C)hild ix Cx Clean Exec",
    CMD_nix		 => "(N)ame ix",
    CMD_nix_safe	 => "(N)ame ix",
    CMD_EXEC_IX_ON	 => "(X)ix",
    CMD_EXEC_IX_OFF	 => "(X)ix",
    CMD_SAVE             => "(S)ave Changes",
    CMD_CONTINUE         => "(C)ontinue Profiling",
    CMD_NEW              => "(N)ew",
    CMD_GLOB             => "(G)lob",
    CMD_GLOBEXT          => "Glob w/(E)xt",
    CMD_ADDHAT           => "(A)dd Requested Hat",
    CMD_USEDEFAULT       => "(U)se Default Hat",
    CMD_SCAN             => "(S)can system log for AppArmor events",
    CMD_HELP             => "(H)elp",
    CMD_VIEW_PROFILE     => "(V)iew Profile",
    CMD_USE_PROFILE      => "(U)se Profile",
    CMD_CREATE_PROFILE   => "(C)reate New Profile",
    CMD_UPDATE_PROFILE   => "(U)pdate Profile",
    CMD_IGNORE_UPDATE    => "(I)gnore Update",
    CMD_SAVE_CHANGES     => "(S)ave Changes",
    CMD_UPLOAD_CHANGES   => "(U)pload Changes",
    CMD_VIEW_CHANGES     => "(V)iew Changes",
    CMD_VIEW             => "(V)iew",
    CMD_ENABLE_REPO      => "(E)nable Repository",
    CMD_DISABLE_REPO     => "(D)isable Repository",
    CMD_ASK_NEVER        => "(N)ever Ask Again",
    CMD_ASK_LATER        => "Ask Me (L)ater",
    CMD_YES              => "(Y)es",
    CMD_NO               => "(N)o",
    CMD_ALL_NET          => "Allow All (N)etwork",
    CMD_NET_FAMILY       => "Allow Network Fa(m)ily",
    CMD_OVERWRITE        => "(O)verwrite Profile",
    CMD_KEEP             => "(K)eep Profile",
    CMD_CONTINUE         => "(C)ontinue",
);

sub UI_PromptUser ($) {
    my $q = shift;

    my ($cmd, $arg);
    if ($UI_Mode eq "text") {

        ($cmd, $arg) = Text_PromptUser($q);

    } else {

        $q->{type} = "wizard";

        SendDataToYast($q);
        my ($ypath, $yarg) = GetDataFromYast();

        $cmd = $yarg->{selection} || "CMD_ABORT";
        $arg = $yarg->{selected};
    }

    if ($cmd eq "CMD_ABORT") {
        confirm_and_abort();
        $cmd = "XXXINVALIDXXX";
    } elsif ($cmd eq "CMD_FINISHED") {
        confirm_and_finish();
        $cmd = "XXXINVALIDXXX";
    }

    if (wantarray) {
        return ($cmd, $arg);
    } else {
        return $cmd;
    }
}


sub UI_ShortMessage($$) {
    my ($headline, $message) = @_;

    SendDataToYast(
        {
            type     => "short-dialog-message",
            headline => $headline,
            message  => $message
        }
    );
    my ($ypath, $yarg) = GetDataFromYast();
}

sub UI_LongMessage($$) {
    my ($headline, $message) = @_;

    $headline = "MISSING" if not defined $headline;
    $message  = "MISSING" if not defined $message;

    SendDataToYast(
        {
            type     => "long-dialog-message",
            headline => $headline,
            message  => $message
        }
    );
    my ($ypath, $yarg) = GetDataFromYast();
}

##########################################################################
# here are the interface functions to send data back and forth between
# the yast frontend and the perl backend

# this is super ugly, but waits for the next ycp Read command and sends data
# back to the ycp front end.

sub SendDataToYast($) {
    my $data = shift;

    $DEBUGGING && debug "SendDataToYast: Waiting for YCP command";

    while (<STDIN>) {
        $DEBUGGING && debug "SendDataToYast: YCP: $_";
        my ($ycommand, $ypath, $yargument) = ycp::ParseCommand($_);

        if ($ycommand && $ycommand eq "Read") {

            if ($DEBUGGING) {
                my $debugmsg = Data::Dumper->Dump([$data], [qw(*data)]);
                debug "SendDataToYast: Sending--\n$debugmsg";
            }

            ycp::Return($data);
            return 1;

        } else {

            $DEBUGGING && debug "SendDataToYast: Expected 'Read' but got-- $_";

        }
    }

    # if we ever break out here, something's horribly wrong.
    fatal_error "SendDataToYast: didn't receive YCP command before connection died";
}

# this is super ugly, but waits for the next ycp Write command and grabs
# whatever the ycp front end gives us

sub GetDataFromYast() {

    $DEBUGGING && debug "GetDataFromYast: Waiting for YCP command";

    while (<STDIN>) {
        $DEBUGGING && debug "GetDataFromYast: YCP: $_";
        my ($ycmd, $ypath, $yarg) = ycp::ParseCommand($_);

        if ($DEBUGGING) {
            my $debugmsg = Data::Dumper->Dump([$yarg], [qw(*data)]);
            debug "GetDataFromYast: Received--\n$debugmsg";
        }

        if ($ycmd && $ycmd eq "Write") {

            ycp::Return("true");
            return ($ypath, $yarg);

        } else {
            $DEBUGGING && debug "GetDataFromYast: Expected 'Write' but got-- $_";
        }
    }

    # if we ever break out here, something's horribly wrong.
    fatal_error "GetDataFromYast: didn't receive YCP command before connection died";
}

sub confirm_and_abort() {
    my $ans = UI_YesNo(gettext("Are you sure you want to abandon this set of profile changes and exit?"), "n");
    if ($ans eq "y") {
        UI_Info(gettext("Abandoning all changes."));
        shutdown_yast();
        foreach my $prof (@created) {
            delete_profile($prof);
        }
        exit 0;
    }
}

sub confirm_and_finish() {
    die "FINISHING\n";
}

sub build_x_functions($$$) {
    my ($default, $options, $exec_toggle) = @_;
    my @{list};
    if ($exec_toggle) {
	push @list, "CMD_ix" if $options =~ /i/;
	push @list, "CMD_pix" if $options =~ /p/ and $options =~ /i/;
	push @list, "CMD_cix" if $options =~ /c/ and $options =~ /i/;
	push @list, "CMD_nix" if $options =~ /n/ and $options =~ /i/;
	push @list, "CMD_ux" if $options =~ /u/;
    } else {
	push @list, "CMD_ix" if $options =~ /i/;
	push @list, "CMD_px" if $options =~ /p/;
	push @list, "CMD_cx" if $options =~ /c/;
	push @list, "CMD_nx" if $options =~ /n/;
	push @list, "CMD_ux" if $options =~ /u/;
    }
    if ($exec_toggle) {
	push @list, "CMD_EXEC_IX_OFF" if $options =~/p|c|n/;
    } else {
	push @list, "CMD_EXEC_IX_ON" if $options =~/p|c|n/;
    }
    push @list, "CMD_DENY", "CMD_ABORT", "CMD_FINISHED";
    return @list;
}

##########################################################################
# this is the hideously ugly function that descends down the flow/event
# trees that we've generated by parsing the logfile

sub handlechildren($$$);

sub handlechildren($$$) {
    my $profile = shift;
    my $hat     = shift;
    my $root    = shift;

    my @entries = @$root;
    for my $entry (@entries) {
        fatal_error "$entry is not a ref" if not ref($entry);

        if (ref($entry->[0])) {
            handlechildren($profile, $hat, $entry);
        } else {

            my @entry = @$entry;
            my $type  = shift @entry;

            if ($type eq "fork") {
                my ($pid, $p, $h) = @entry;

                if (   ($p !~ /null(-complain)*-profile/)
                    && ($h !~ /null(-complain)*-profile/))
                {
                    $profile = $p;
                    $hat     = $h;
                }

		if ($hat) {
		    $profilechanges{$pid} = $profile . "//" . $hat;
		} else {
		    $profilechanges{$pid} = $profile;
		}
            } elsif ($type eq "unknown_hat") {
                my ($pid, $p, $h, $sdmode, $uhat) = @entry;

                if ($p !~ /null(-complain)*-profile/) {
                    $profile = $p;
                }

                if ($sd{$profile}{$uhat}) {
                    $hat = $uhat;
                    next;
                }

                my $new_p = update_repo_profile($sd{$profile}{$profile});
                if ( $new_p and
                     UI_SelectUpdatedRepoProfile($profile, $new_p) and
                     $sd{$profile}{$uhat} ) {
                    $hat = $uhat;
                    next;
                }

                # figure out what our default hat for this application is.
                my $defaulthat;
                for my $hatglob (keys %{$cfg->{defaulthat}}) {
                    $defaulthat = $cfg->{defaulthat}{$hatglob}
                      if $profile =~ /$hatglob/;
                }
                # keep track of previous answers for this run...
                my $context = $profile;
                $context .= " -> ^$uhat";
                my $ans = $transitions{$context} || "XXXINVALIDXXX";

                while ($ans !~ /^CMD_(ADDHAT|USEDEFAULT|DENY)$/) {
                    my $q = {};
                    $q->{headers} = [];
                    push @{ $q->{headers} }, gettext("Profile"), $profile;
                    if ($defaulthat) {
                        push @{ $q->{headers} }, gettext("Default Hat"), $defaulthat;
                    }
                    push @{ $q->{headers} }, gettext("Requested Hat"), $uhat;

                    $q->{functions} = [];
                    push @{ $q->{functions} }, "CMD_ADDHAT";
                    push @{ $q->{functions} }, "CMD_USEDEFAULT" if $defaulthat;
                    push @{$q->{functions}}, "CMD_DENY", "CMD_ABORT",
                      "CMD_FINISHED";

                    $q->{default} = ($sdmode eq "PERMITTING") ? "CMD_ADDHAT" : "CMD_DENY";

                    $seenevents++;

                    $ans = UI_PromptUser($q);

                }
                $transitions{$context} = $ans;

                if ($ans eq "CMD_ADDHAT") {
                    $hat = $uhat;
                    $sd{$profile}{$hat}{flags} = $sd{$profile}{$profile}{flags};
                } elsif ($ans eq "CMD_USEDEFAULT") {
                    $hat = $defaulthat;
                } elsif ($ans eq "CMD_DENY") {
                    return;
                }

            } elsif ($type eq "capability") {
               my ($pid, $p, $h, $prog, $sdmode, $capability) = @entry;

                if (   ($p !~ /null(-complain)*-profile/)
                    && ($h !~ /null(-complain)*-profile/))
                {
                    $profile = $p;
                    $hat     = $h;
                }

                # print "$pid $profile $hat $prog $sdmode capability $capability\n";

                next unless $profile && $hat;

                $prelog{$sdmode}{$profile}{$hat}{capability}{$capability} = 1;
            } elsif (($type eq "path") || ($type eq "exec")) {
                my ($pid, $p, $h, $prog, $sdmode, $mode, $detail, $to_name) = @entry;

		$mode = 0 unless ($mode);

                if (   ($p !~ /null(-complain)*-profile/)
                    && ($h !~ /null(-complain)*-profile/))
                {
                    $profile = $p;
                    $hat     = $h;
                }

                next unless $profile && $hat && $detail;
                my $domainchange = ($type eq "exec") ? "change" : "nochange";

                # escape special characters that show up in literal paths
                $detail =~ s/(\[|\]|\+|\*|\{|\})/\\$1/g;

                # we need to give the Execute dialog if they're requesting x
                # access for something that's not a directory - we'll force
                # a "ix" Path dialog for directories
                my $do_execute  = 0;
                my $exec_target = $detail;

                if ($mode & str_to_mode("x")) {
                    if (-d $exec_target) {
			$mode &= (~$ALL_AA_EXEC_TYPE);
                        $mode |= str_to_mode("ix");
                    } else {
                        $do_execute = 1;
                    }
                }

		if ($mode & $AA_MAY_LINK) {
                    if ($detail =~ m/^from (.+) to (.+)$/) {
                        my ($path, $target) = ($1, $2);

                        my $frommode = str_to_mode("lr");
                        if (defined $prelog{$sdmode}{$profile}{$hat}{path}{$path}) {
                            $frommode |= $prelog{$sdmode}{$profile}{$hat}{path}{$path};
                        }
                        $prelog{$sdmode}{$profile}{$hat}{path}{$path} = $frommode;

                        my $tomode = str_to_mode("lr");
                        if (defined $prelog{$sdmode}{$profile}{$hat}{path}{$target}) {
                            $tomode |= $prelog{$sdmode}{$profile}{$hat}{path}{$target};
                        }
                        $prelog{$sdmode}{$profile}{$hat}{path}{$target} = $tomode;

                        # print "$pid $profile $hat $prog $sdmode $path:$frommode -> $target:$tomode\n";
                    } else {
                        next;
                    }
                } elsif ($mode) {
                    my $path = $detail;

                    if (defined $prelog{$sdmode}{$profile}{$hat}{path}{$path}) {
                        $mode |= $prelog{$sdmode}{$profile}{$hat}{path}{$path};
                    }
                    $prelog{$sdmode}{$profile}{$hat}{path}{$path} = $mode;

                    # print "$pid $profile $hat $prog $sdmode $mode $path\n";
                }

                if ($do_execute) {
                    next if ( profile_known_exec($sd{$profile}{$hat},
						 "exec", $exec_target ) );

                    my $p = update_repo_profile($sd{$profile}{$profile});

		    if ($to_name) {
			next if ( $to_name and
				  UI_SelectUpdatedRepoProfile($profile, $p) and
				  profile_known_exec($sd{$profile}{$hat},
						     "exec", $to_name ) );
		    } else {
			next if ( UI_SelectUpdatedRepoProfile($profile, $p) and
				  profile_known_exec($sd{$profile}{$hat},
						     "exec", $exec_target ) );
		    }

                    my $context = $profile;
                    $context .= "^$hat" if $profile ne $hat;
                    $context .= " -> $exec_target";
                    my $ans = $transitions{$context} || "";

                    my ($combinedmode, $combinedaudit, $cm, $am, @m);
		    $combinedmode = 0;
		    $combinedaudit = 0;

                    # does path match any regexps in original profile?
                    ($cm, $am, @m) = rematchfrag($sd{$profile}{$hat}, 'allow', $exec_target);
                    $combinedmode |= $cm if $cm;
		    $combinedaudit |= $am if $am;

		    # find the named transition if is present
		    if ($combinedmode & str_to_mode("x")) {
			my $nt_name;
			foreach my $entry (@m) {
			    if ($sd{$profile}{$hat}{allow}{path}{$entry}{to}) {
				$nt_name = $sd{$profile}{$hat}{allow}{path}{$entry}{to};
				last;
			    }
			}
			if ($to_name and $nt_name and ($to_name ne $nt_name)) {
			    #fatal_error "transition name from "
			} elsif ($nt_name) {
			    $to_name = $nt_name;
			}
		    }

                    # does path match anything pulled in by includes in
                    # original profile?
                    ($cm, $am, @m) = match_prof_incs_to_path($sd{$profile}{$hat}, 'allow', $exec_target);
                    $combinedmode |= $cm if $cm;
                    $combinedaudit |= $am if $am;
		    if ($combinedmode & str_to_mode("x")) {
			my $nt_name;
			foreach my $entry (@m) {
			    if ($sd{$profile}{$hat}{allow}{path}{$entry}{to}) {
				$nt_name = $sd{$profile}{$hat}{allow}{path}{$entry}{to};
				last;
			    }
			}
			if ($to_name and $nt_name and ($to_name ne $nt_name)) {
			    #fatal_error "transition name from "
			} elsif ($nt_name) {
			    $to_name = $nt_name;
			}
		    }


		    #nx does not exist in profiles.  It does in log
		    #files however.  The log parsing routines will convert
		    #it to its profile form.
		    #nx is internally represented by cx/px/cix/pix + to_name
                    my $exec_mode = 0;
		    if (contains($combinedmode, "pix")) {
			if ($to_name) {
			    $ans = "CMD_nix";
			} else {
			    $ans = "CMD_pix";
			}
			$exec_mode = str_to_mode("pixr");
		    } elsif (contains($combinedmode, "cix")) {
			if ($to_name) {
			    $ans = "CMD_nix";
			} else {
			    $ans = "CMD_cix";
			}
			$exec_mode = str_to_mode("cixr");
		    } elsif (contains($combinedmode, "Pix")) {
			if ($to_name) {
			    $ans = "CMD_nix_safe";
			} else {
			    $ans = "CMD_pix_safe";
			}
			$exec_mode = str_to_mode("Pixr");
		    } elsif (contains($combinedmode, "Cix")) {
			if ($to_name) {
			    $ans = "CMD_nix_safe";
			} else {
			    $ans = "CMD_cix_safe";
			}
			$exec_mode = str_to_mode("Cixr");
		    } elsif (contains($combinedmode, "ix")) {
                        $ans       = "CMD_ix";
                        $exec_mode = str_to_mode("ixr");
                    } elsif (contains($combinedmode, "px")) {
			if ($to_name) {
			    $ans = "CMD_nx";
			} else {
			    $ans = "CMD_px";
			}
                        $exec_mode = str_to_mode("px");
		    } elsif (contains($combinedmode, "cx")) {
			if ($to_name) {
			    $ans = "CMD_nx";
			} else {
			    $ans = "CMD_cx";
			}
			$exec_mode = str_to_mode("cx");
                    } elsif (contains($combinedmode, "ux")) {
                        $ans       = "CMD_ux";
                        $exec_mode = str_to_mode("ux");
                    } elsif (contains($combinedmode, "Px")) {
			if ($to_name) {
			    $ans = "CMD_nx_safe";
			} else {
			    $ans       = "CMD_px_safe";
			}
                        $exec_mode = str_to_mode("Px");
		    } elsif (contains($combinedmode, "Cx")) {
			if ($to_name) {
			    $ans = "CMD_nx_safe";
			} else {
			    $ans = "CMD_cx_safe";
			}
			$exec_mode = str_to_mode("Cx");
                    } elsif (contains($combinedmode, "Ux")) {
                        $ans       = "CMD_ux_safe";
                        $exec_mode = str_to_mode("Ux");
                    } else {
                        my $options = $cfg->{qualifiers}{$exec_target} || "ipcnu";
			fatal_error "$entry has transition name but not transition mode" if $to_name;

                        # force "ix" as the only option when the profiled
                        # program executes itself
                        $options = "i" if $exec_target eq $profile;

			# for now don't allow hats to cx
			$options =~ s/c// if $hat and $hat ne $profile;

                        # we always need deny...
                        $options .= "d";

                        # figure out what our default option should be...
                        my $default;
                        if ($options =~ /p/
                            && -e getprofilefilename($exec_target))
                        {
                            $default = "CMD_px";
                        } elsif ($options =~ /i/) {
                            $default = "CMD_ix";
                        } elsif ($options =~ /c/) {
                            $default = "CMD_cx";
                        } elsif ($options =~ /n/) {
                            $default = "CMD_nx";
                        } else {
                            $default = "CMD_DENY";
                        }

                        # ugh, this doesn't work if someone does an ix before
                        # calling this particular child process.  at least
                        # it's only a hint instead of mandatory to get this
                        # right.
                        my $parent_uses_ld_xxx = check_for_LD_XXX($profile);

                        my $severity = $sevdb->rank($exec_target, "x");

                        # build up the prompt...
                        my $q = {};
                        $q->{headers} = [];
                        push @{ $q->{headers} }, gettext("Profile"), combine_name($profile, $hat);
                        if ($prog && $prog ne "HINT") {
                            push @{ $q->{headers} }, gettext("Program"), $prog;
                        }
			# $to_name should NOT exist here other wise we know what
			# mode we are supposed to be transitioning to
			# which is handled above.
                        push @{ $q->{headers} }, gettext("Execute"),  $exec_target;
                        push @{ $q->{headers} }, gettext("Severity"), $severity;

                        $q->{functions} = [];

                        my $prompt = "\n$context\n";
			my $exec_toggle = 0;

			push @{ $q->{functions} }, build_x_functions($default, $options, $exec_toggle);

                        $options = join("|", split(//, $options));

                        $seenevents++;

			while ($ans !~ m/^CMD_(ix|px|cx|nx|pix|cix|nix|px_safe|cx_safe|nx_safe|pix_safe|cix_safe|nix_safe|ux|ux_safe|EXEC_TOGGLE|DENY)$/) {
			    $ans = UI_PromptUser($q);

			    if ($ans =~ /CMD_EXEC_IX_/) {
				$exec_toggle = !$exec_toggle;

				$q->{functions} = [ ];
				push @{ $q->{functions} }, build_x_functions($default, $options, $exec_toggle);
				$ans = "";
				next;
			    }
			    if ($ans =~ /CMD_(nx|nix)/) {
                                my $arg = $exec_target;

				my $ynans = "n";
				if ($profile eq $hat) {
				    $ynans = UI_YesNo("Are you specifying a transition to a local profile?", "n");
				}

				if ($ynans eq "y") {
				    if ($ans eq "CMD_nx") {
					$ans = "CMD_cx";
				    } else {
					$ans = "CMD_cix";
				    }
				} else {
				    if ($ans eq "CMD_nx") {
					$ans = "CMD_px";
				    } else {
					$ans = "CMD_pix";
				    }
				}
				$to_name = UI_GetString(gettext("Enter profile name to transition to: "), $arg);
			    }
			    if ($ans =~ /CMD_ix/) {
				$exec_mode = str_to_mode("ix");
                            } elsif ($ans =~ /CMD_(px|cx|nx|pix|cix|nix)/) {
				my $match = $1;
				$exec_mode = str_to_mode($match);
                                my $px_default = "n";
                                my $px_mesg    = gettext("Should AppArmor sanitize the environment when\nswitching profiles?\n\nSanitizing the environment is more secure,\nbut some applications depend on the presence\nof LD_PRELOAD or LD_LIBRARY_PATH.");
                                if ($parent_uses_ld_xxx) {
                                    $px_mesg = gettext("Should AppArmor sanitize the environment when\nswitching profiles?\n\nSanitizing the environment is more secure,\nbut this application appears to use LD_PRELOAD\nor LD_LIBRARY_PATH and clearing these could\ncause functionality problems.");
                                }
                                my $ynans = UI_YesNo($px_mesg, $px_default);
				$ans = "CMD_$match";
                                if ($ynans eq "y") {
                                    $exec_mode &= ~($AA_EXEC_UNSAFE | ($AA_EXEC_UNSAFE << $AA_OTHER_SHIFT));
                                }
                            } elsif ($ans eq "CMD_ux") {
				$exec_mode = str_to_mode("ux");
                                my $ynans = UI_YesNo(sprintf(gettext("Launching processes in an unconfined state is a very\ndangerous operation and can cause serious security holes.\n\nAre you absolutely certain you wish to remove all\nAppArmor protection when executing \%s?"), $exec_target), "n");
                                if ($ynans eq "y") {
                                    my $ynans = UI_YesNo(gettext("Should AppArmor sanitize the environment when\nrunning this program unconfined?\n\nNot sanitizing the environment when unconfining\na program opens up significant security holes\nand should be avoided if at all possible."), "y");
                                    if ($ynans eq "y") {
					$exec_mode &= ~($AA_EXEC_UNSAFE | ($AA_EXEC_UNSAFE << $AA_OTHER_SHIFT));
                                    }
                                } else {
                                    $ans = "INVALID";
                                }
                            }
                        }
                        $transitions{$context} = $ans;

			if ($ans =~ /CMD_(ix|px|cx|nx|pix|cix|nix)/) {
			    # if we're inheriting, things'll bitch unless we have r
			    if ($exec_mode & str_to_mode("i")) {
				$exec_mode |= str_to_mode("r");
			    }

			} else {
			    if ($ans eq "CMD_DENY") {
				$sd{$profile}{$hat}{deny}{path}{$exec_target}{mode} |= str_to_mode("x");

				$sd{$profile}{$hat}{deny}{path}{$exec_target}{audit} |= 0;
				$changed{$profile} = 1;
                                # skip all remaining events if they say to deny
                                # the exec
                                return if $domainchange eq "change";
			    }

                        }

			unless ($ans eq "CMD_DENY") {
# ???? if its defined in the prelog we shouldn't have asked
                            if (defined $prelog{PERMITTING}{$profile}{$hat}{path}{$exec_target}) {
#                                $exec_mode = $prelog{PERMITTING}{$profile}{$hat}{path}{$exec_target};
                            }

                            $prelog{PERMITTING}{$profile}{$hat}{path}{$exec_target} |= $exec_mode;
                            $log{PERMITTING}{$profile}              = {};
                            $sd{$profile}{$hat}{allow}{path}{$exec_target}{mode} |= $exec_mode;
                            $sd{$profile}{$hat}{allow}{path}{$exec_target}{audit} |= 0;
                            $sd{$profile}{$hat}{allow}{path}{$exec_target}{to} = $to_name if ($to_name);

                            # mark this profile as changed
                            $changed{$profile} = 1;

                            if ($exec_mode & str_to_mode("i")) {
                                if ($exec_target =~ /perl/) {
                                    $sd{$profile}{$hat}{include}{"abstractions/perl"} = 1;
                                } elsif ($detail =~ m/\/bin\/(bash|sh)/) {
                                    $sd{$profile}{$hat}{include}{"abstractions/bash"} = 1;
                                }
                                my $hashbang = head($exec_target);
                                if ($hashbang =~ /^#!\s*(\S+)/) {
                                    my $interpreter = get_full_path($1);
                                    $sd{$profile}{$hat}{path}->{$interpreter}{mode} |= str_to_mode("ix");
                                    $sd{$profile}{$hat}{path}->{$interpreter}{audit} |= 0;
                                    if ($interpreter =~ /perl/) {
                                        $sd{$profile}{$hat}{include}{"abstractions/perl"} = 1;
                                    } elsif ($interpreter =~ m/\/bin\/(bash|sh)/) {
                                        $sd{$profile}{$hat}{include}{"abstractions/bash"} = 1;
                                    }
                                }
                            }
                        }
		    }

                    # print "$pid $profile $hat EXEC $exec_target $ans $exec_mode\n";

                    # update our tracking info based on what kind of change
                    # this is...
                    if ($ans eq "CMD_ix") {
			if ($hat) {
			    $profilechanges{$pid} = $profile . "//" . $hat;
			} else {
			    $profilechanges{$pid} = $profile;
			}
                    } elsif ($ans =~ /^CMD_(px|nx|pix|nix)/) {
			$exec_target = $to_name if ($to_name);
                        if ($sdmode eq "PERMITTING") {
                            if ($domainchange eq "change") {
                                $profile              = $exec_target;
                                $hat                  = $exec_target;
                                $profilechanges{$pid} = $profile;
                            }
                        }
                        # if they want to use px, make sure a profile
                        # exists for the target.
                        unless (-e getprofilefilename($exec_target)) {
			    my $ynans = "y";
			    if ($exec_mode & str_to_mode("i")) {
				$ynans = UI_YesNo(sprintf(gettext("A profile for %s does not exist. Create one?"), $exec_target), "n");
			    }
			    if ($ynans eq "y") {
				$helpers{$exec_target} = "enforce";
				if ($to_name) {
				    autodep_base("", $exec_target);
				} else {
				    autodep_base($exec_target, "");
				}
				reload_base($exec_target);
			    }
                        }
                    } elsif ($ans =~ /^CMD_(cx|cix)/) {
			$exec_target = $to_name if ($to_name);
                        if ($sdmode eq "PERMITTING") {
                            if ($domainchange eq "change") {
                                $profilechanges{$pid} = "${profile}//${exec_target}";
#                                $profile              = $exec_target;
#                                $hat                  = $exec_target;
                            }
                        }

                        # if they want to use cx, make sure a profile
                        # exists for the target.
			unless ($sd{$profile}{$exec_target}) {
			    my $ynans = "y";
			    if ($exec_mode & str_to_mode("i")) {
				$ynans = UI_YesNo(sprintf(gettext("A local profile for %s does not exist. Create one?"), $exec_target), "n");
			    }
			    if ($ynans eq "y") {
				$hat = $exec_target;
				# keep track of profile flags
				#$profile_data->{$profile}{$hat}{flags} = ;

				# we have seen more than a declaration so clear it
				$sd{$profile}{$hat}{'declared'} = 0;
				$sd{$profile}{$hat}{profile} = 1;

				# Otherwise sub-profiles end up getting
				# put in enforce mode with genprof
				$sd{$profile}{$hat}{flags} = $sd{$profile}{$profile}{flags} if $profile ne $hat;

				# autodep our new child
				my $stub_profile = create_new_profile($hat);

				$sd{$profile}{$hat}{flags} = 'complain';
				$sd{$profile}{$hat}{allow}{path} = { };
				if (defined $stub_profile->{$hat}{$hat}{allow}{path}) {
				  $sd{$profile}{$hat}{allow}{path} = $stub_profile->{$hat}{$hat}{allow}{path};
				}
				$sd{$profile}{$hat}{include} = { };
				if (defined $stub_profile->{$hat}{$hat}{include}) {
				  $sd{$profile}{$hat}{include} = $stub_profile->{$hat}{$hat}{include};
				}
				$sd{$profile}{$hat}{allow}{netdomain} = { };
				my $file = $sd{$profile}{$profile}{filename};
				$filelist{$file}{profiles}{$profile}{$hat} = 1;

			    }
                        }
                    } elsif ($ans =~ /^CMD_ux/) {
                        $profilechanges{$pid} = "unconfined";
                        return if $domainchange eq "change";
                    }
                }
            } elsif ( $type eq "netdomain" ) {
               my ($pid, $p, $h, $prog, $sdmode, $family, $sock_type, $protocol) =
                  @entry;

                if (   ($p !~ /null(-complain)*-profile/)
                    && ($h !~ /null(-complain)*-profile/))
                {
                    $profile = $p;
                    $hat     = $h;
                }

                next unless $profile && $hat;
                $prelog{$sdmode}
                       {$profile}
                       {$hat}
                       {netdomain}
                       {$family}
                       {$sock_type} = 1 unless ( !$family || !$sock_type );

            }
        }
    }
}

sub add_to_tree ($$$@) {
    my ($pid, $parent, $type, @event) = @_;
    if ( $DEBUGGING ) {
        my $eventmsg = Data::Dumper->Dump([@event], [qw(*event)]);
        $eventmsg =~ s/\n/ /g;
        debug " add_to_tree: pid [$pid] type [$type] event [ $eventmsg ]";
    }

    unless (exists $pid{$pid}) {
	my $profile = $event[0];
	my $hat = $event[1];
	if ($parent && exists $pid{$parent}) {
	    # fork entry is missing fake one so that fork tracking will work
	    $hat     ||= "null-complain-profile";
	    my $arrayref = [];
            push @{ $pid{$parent} }, $arrayref;
	    $pid{$pid} = $arrayref;
	    push @{$arrayref}, [ "fork", $pid, $profile, $hat ];
	} else {
	    my $arrayref = [];
	    push @log, $arrayref;
	    $pid{$pid} = $arrayref;
	}
    }

    push @{ $pid{$pid} }, [ $type, $pid, @event ];
}

#
# variables used in the logparsing routines
#
our $LOG;
our $next_log_entry;
our $logmark;
our $seenmark;
my $RE_LOG_v2_0_syslog = qr/SubDomain/;
my $RE_LOG_v2_1_syslog = qr/kernel:\s+(\[[\d\.\s]+\]\s+)?(audit\([\d\.\:]+\):\s+)?type=150[1-6]/;
my $RE_LOG_v2_6_syslog = qr/kernel:\s+(\[[\d\.\s]+\]\s+)?type=\d+\s+audit\([\d\.\:]+\):\s+apparmor=/;
my $RE_LOG_v2_0_audit  =
    qr/type=(APPARMOR|UNKNOWN\[1500\]) msg=audit\([\d\.\:]+\):/;
my $RE_LOG_v2_1_audit  =
    qr/type=(UNKNOWN\[150[1-6]\]|APPARMOR_(AUDIT|ALLOWED|DENIED|HINT|STATUS|ERROR))/;
my $RE_LOG_v2_6_audit =
    qr/type=AVC\s+(msg=)?audit\([\d\.\:]+\):\s+apparmor=/;

sub prefetch_next_log_entry() {
    # if we already have an existing cache entry, something's broken
    if ($next_log_entry) {
        print STDERR "Already had next log entry: $next_log_entry";
    }

    # read log entries until we either hit the end or run into an
    # AA event message format we recognize
    do {
        $next_log_entry = <$LOG>;
        $DEBUGGING && debug "prefetch_next_log_entry: next_log_entry = " . ($next_log_entry ? $next_log_entry : "empty");
    } until (!$next_log_entry || $next_log_entry =~ m{
        $RE_LOG_v2_0_syslog |
        $RE_LOG_v2_0_audit  |
        $RE_LOG_v2_1_audit  |
        $RE_LOG_v2_1_syslog |
        $RE_LOG_v2_6_syslog |
        $RE_LOG_v2_6_audit  |
        $logmark
    }x);
}

sub get_next_log_entry() {
    # make sure we've got a next log entry if there is one
    prefetch_next_log_entry() unless $next_log_entry;

    # save a copy of the next log entry...
    my $log_entry = $next_log_entry;

    # zero out our cache of the next log entry
    $next_log_entry = undef;

    return $log_entry;
}

sub peek_at_next_log_entry() {
    # make sure we've got a next log entry if there is one
    prefetch_next_log_entry() unless $next_log_entry;

    # return a copy of the next log entry without pulling it out of the cache
    return $next_log_entry;
}

sub throw_away_next_log_entry() {
    $next_log_entry = undef;
}

sub parse_log_record_v_2_0 ($$) {
    my ($record, $last) = @_;
    $DEBUGGING && debug "parse_log_record_v_2_0: $record";

    # What's this early out for?  As far as I can tell, parse_log_record_v_2_0
    # won't ever be called without something in $record
    return $last if ( ! $record );

    $_ = $record;

    if (s/(PERMITTING|REJECTING)-SYSLOGFIX/$1/) {
        s/%%/%/g;
    }

    if (m/LOGPROF-HINT unknown_hat (\S+) pid=(\d+) profile=(.+) active=(.+)/) {
        my ($uhat, $pid, $profile, $hat) = ($1, $2, $3, $4);

        $last = $&;

        # we want to ignore entries for profiles that don't exist
        # they're most likely broken entries or old entries for
        # deleted profiles
        return $&
          if ( ($profile ne 'null-complain-profile')
            && (!profile_exists($profile)));

        add_to_tree($pid, 0, "unknown_hat", $profile, $hat,
                    "PERMITTING", $uhat);
    } elsif (m/LOGPROF-HINT (unknown_profile|missing_mandatory_profile) image=(.+) pid=(\d+) profile=(.+) active=(.+)/) {
        my ($image, $pid, $profile, $hat) = ($2, $3, $4, $5);

        return $& if $last =~ /PERMITTING x access to $image/;
        $last = $&;

        # we want to ignore entries for profiles that don't exist
        # they're most likely broken entries or old entries for
        # deleted profiles
        return $&
          if ( ($profile ne 'null-complain-profile')
            && (!profile_exists($profile)));

        add_to_tree($pid, 0, "exec", $profile, $hat, "HINT", "PERMITTING", "x", $image);

    } elsif (m/(PERMITTING|REJECTING) (\S+) access (.+) \((.+)\((\d+)\) profile (.+) active (.+)\)/) {
        my ($sdmode, $mode, $detail, $prog, $pid, $profile, $hat) =
           ($1, $2, $3, $4, $5, $6, $7);

	if ($mode eq "link") {
	    $mode = "l";
	}
        if (!validate_log_mode($mode)) {
            fatal_error(sprintf(gettext('Log contains unknown mode %s.'), $mode));
        }

        my $domainchange = "nochange";
        if ($mode =~ /x/) {

            # we need to try to check if we're doing a domain transition
            if ($sdmode eq "PERMITTING") {
                my $following = peek_at_next_log_entry();

                if ($following && ($following =~ m/changing_profile/)) {
                    $domainchange = "change";
                    throw_away_next_log_entry();
                }
            }
        } else {

            # we want to ignore duplicates for things other than executes...
            return $& if $seen{$&};
            $seen{$&} = 1;
        }

        $last = $&;

        # we want to ignore entries for profiles that don't exist
        # they're most likely broken entries or old entries for
        # deleted profiles
        if (($profile ne 'null-complain-profile')
            && (!profile_exists($profile)))
        {
            return $&;
        }

        # currently no way to stick pipe mediation in a profile, ignore
        # any messages like this
        return $& if $detail =~ /to pipe:/;

        # strip out extra extended attribute info since we don't
        # currently have a way to specify it in the profile and
        # instead just need to provide the access to the base filename
        $detail =~ s/\s+extended attribute \S+//;

        # kerberos code checks to see if the krb5.conf file is world
        # writable in a stupid way so we'll ignore any w accesses to
        # krb5.conf
        return $& if (($detail eq "to /etc/krb5.conf") && contains($mode, "w"));

        # strip off the (deleted) tag that gets added if it's a
        # deleted file
        $detail =~ s/\s+\(deleted\)$//;

    #            next if (($detail =~ /to \/lib\/ld-/) && ($mode =~ /x/));

        $detail =~ s/^to\s+//;

        if ($domainchange eq "change") {
            add_to_tree($pid, 0, "exec", $profile, $hat, $prog,
                        $sdmode, str_to_mode($mode), $detail);
        } else {
            add_to_tree($pid, 0, "path", $profile, $hat, $prog,
                        $sdmode, str_to_mode($mode), $detail);
        }

    } elsif (m/(PERMITTING|REJECTING) (?:mk|rm)dir on (.+) \((.+)\((\d+)\) profile (.+) active (.+)\)/) {
        my ($sdmode, $path, $prog, $pid, $profile, $hat) =
           ($1, $2, $3, $4, $5, $6);

        # we want to ignore duplicates for things other than executes...
        return $& if $seen{$&}++;

        $last = $&;

        # we want to ignore entries for profiles that don't exist
        # they're most likely broken entries or old entries for
        # deleted profiles
        return $&
          if ( ($profile ne 'null-complain-profile')
            && (!profile_exists($profile)));

        add_to_tree($pid, 0, "path", $profile, $hat, $prog, $sdmode,
                    "w", $path);

    } elsif (m/(PERMITTING|REJECTING) xattr (\S+) on (.+) \((.+)\((\d+)\) profile (.+) active (.+)\)/) {
        my ($sdmode, $xattr_op, $path, $prog, $pid, $profile, $hat) =
           ($1, $2, $3, $4, $5, $6, $7);

        # we want to ignore duplicates for things other than executes...
        return $& if $seen{$&}++;

        $last = $&;

        # we want to ignore entries for profiles that don't exist
        # they're most likely broken entries or old entries for
        # deleted profiles
        return $&
          if ( ($profile ne 'null-complain-profile')
            && (!profile_exists($profile)));

        my $xattrmode;
        if ($xattr_op eq "get" || $xattr_op eq "list") {
            $xattrmode = "r";
        } elsif ($xattr_op eq "set" || $xattr_op eq "remove") {
            $xattrmode = "w";
        }

        if ($xattrmode) {
            add_to_tree($pid, 0, "path", $profile, $hat, $prog, $sdmode,
                        str_to_mode($xattrmode), $path);
        }

    } elsif (m/(PERMITTING|REJECTING) attribute \((.*?)\) change to (.+) \((.+)\((\d+)\) profile (.+) active (.+)\)/) {
        my ($sdmode, $change, $path, $prog, $pid, $profile, $hat) =
           ($1, $2, $3, $4, $5, $6, $7);

        # we want to ignore duplicates for things other than executes...
        return $& if $seen{$&};
        $seen{$&} = 1;

        $last = $&;

        # we want to ignore entries for profiles that don't exist
        # they're most likely broken entries or old entries for
        # deleted profiles
        return $&
          if ( ($profile ne 'null-complain-profile')
            && (!profile_exists($profile)));

        # kerberos code checks to see if the krb5.conf file is world
        # writable in a stupid way so we'll ignore any w accesses to
        # krb5.conf
        return $& if $path eq "/etc/krb5.conf";

        add_to_tree($pid, 0, "path", $profile, $hat, $prog, $sdmode,
                    str_to_mode("w"), $path);

    } elsif (m/(PERMITTING|REJECTING) access to capability '(\S+)' \((.+)\((\d+)\) profile (.+) active (.+)\)/) {
        my ($sdmode, $capability, $prog, $pid, $profile, $hat) =
           ($1, $2, $3, $4, $5, $6);

        return $& if $seen{$&};

        $seen{$&} = 1;
        $last = $&;

        # we want to ignore entries for profiles that don't exist - they're
        # most likely broken entries or old entries for deleted profiles
        return $&
          if ( ($profile ne 'null-complain-profile')
            && (!profile_exists($profile)));

        add_to_tree($pid, 0, "capability", $profile, $hat, $prog,
                    $sdmode, $capability);

    } elsif (m/Fork parent (\d+) child (\d+) profile (.+) active (.+)/
        || m/LOGPROF-HINT fork pid=(\d+) child=(\d+) profile=(.+) active=(.+)/
        || m/LOGPROF-HINT fork pid=(\d+) child=(\d+)/)
    {
        my ($parent, $child, $profile, $hat) = ($1, $2, $3, $4);

        $profile ||= "null-complain-profile";
        $hat     ||= "null-complain-profile";

        $last = $&;

        # we want to ignore entries for profiles that don't exist
        # they're  most likely broken entries or old entries for
        # deleted profiles
        return $&
          if ( ($profile ne 'null-complain-profile')
            && (!profile_exists($profile)));

        my $arrayref = [];
        if (exists $pid{$parent}) {
            push @{ $pid{$parent} }, $arrayref;
        } else {
            push @log, $arrayref;
        }
        $pid{$child} = $arrayref;
        push @{$arrayref}, [ "fork", $child, $profile, $hat ];
    } else {
        $DEBUGGING && debug "UNHANDLED: $_";
    }
    return $last;
}

sub parse_log_record ($) {
    my $record = shift;
    $DEBUGGING && debug "parse_log_record: $record";
    my $e = parse_event($record);

    return $e;
}


sub add_event_to_tree ($) {
    my $e = shift;

    my $sdmode = $e->{sdmode}?$e->{sdmode}:"UNKNOWN";
    if ( $e->{type} ) {
        if ( $e->{type} =~ /(UNKNOWN\[1501\]|APPARMOR_AUDIT|1501)/ ) {
            $sdmode = "AUDIT";
        } elsif ( $e->{type} =~ /(UNKNOWN\[1502\]|APPARMOR_ALLOWED|1502)/ ) {
            $sdmode = "PERMITTING";
        } elsif ( $e->{type} =~ /(UNKNOWN\[1503\]|APPARMOR_DENIED|1503)/ ) {
            $sdmode = "REJECTING";
        } elsif ( $e->{type} =~ /(UNKNOWN\[1504\]|APPARMOR_HINT|1504)/ ) {
            $sdmode = "HINT";
        } elsif ( $e->{type} =~ /(UNKNOWN\[1505\]|APPARMOR_STATUS|1505)/ ) {
            $sdmode = "STATUS";
        } elsif ( $e->{type} =~ /(UNKNOWN\[1506\]|APPARMOR_ERROR|1506)/ ) {
            $sdmode = "ERROR";
        } else {
            $sdmode = "UNKNOWN";
        }
    }
    return if ( $sdmode =~ /UNKNOWN|AUDIT|STATUS|ERROR/ );
    return if ($e->{operation} =~ /profile_set/);

    my ($profile, $hat);

    # The version of AppArmor that was accepted into the mainline kernel
    # issues audit events for things like change_hat while unconfined.
    # Previous versions just returned -EPERM without the audit so the
    # events wouldn't have been picked up here.
    return if (!$e->{profile});

    # just convert new null profile style names to old before we begin processing
    # profile and name can contain multiple layers of null- but all we care about
    # currently is single level.
    if ($e->{profile} =~ m/\/\/null-/) {
        $e->{profile} = "null-complain-profile";
    }
    ($profile, $hat) = split /\/\//, $e->{profile};
    if ( $e->{operation} eq "change_hat" ) {
	#screen out change_hat events that aren't part of learning, as before
	#AppArmor 2.4 these events only happend as hints during learning
	return if ($sdmode ne "HINT" &&  $sdmode ne "PERMITTING");
        ($profile, $hat) = split /\/\//, $e->{name};
    }
    $hat = $profile if ( !$hat );
    # TODO - refactor add_to_tree as prog is no longer supplied
    #        HINT is from previous format where prog was not
    #        consistently passed
    my $prog = "HINT";

    return if ($profile ne 'null-complain-profile' && !profile_exists($profile));

    if ($e->{operation} eq "exec") {
        if ( defined $e->{info} && $e->{info} eq "mandatory profile missing" ) {
            add_to_tree( $e->{pid},
			 $e->{parent},
                         "exec",
                         $profile,
                         $hat,
                         $sdmode,
                         "PERMITTING",
                         $e->{denied_mask},
                         $e->{name},
                         $e->{name2}
                       );
        } elsif ( defined $e->{name2} && $e->{name2} =~ m/\/\/null-/) {
            add_to_tree( $e->{pid},
			 $e->{parent},
                          "exec",
                          $profile,
                          $hat,
                          $prog,
                          $sdmode,
                          $e->{denied_mask},
                          $e->{name},
			  ""
                        );
        } elsif (defined $e->{name}) {
            add_to_tree( $e->{pid},
			 $e->{parent},
                          "exec",
                          $profile,
                          $hat,
                          $prog,
                          $sdmode,
                          $e->{denied_mask},
                          $e->{name},
			  ""
                        );
        } else {
            $DEBUGGING && debug "add_event_to_tree: dropped exec event in $e->{profile}";
	}
    } elsif ($e->{operation} =~ m/file_/) {
        add_to_tree( $e->{pid},
		     $e->{parent},
                     "path",
                     $profile,
                     $hat,
                     $prog,
                     $sdmode,
                     $e->{denied_mask},
                     $e->{name},
		     "",
                   );
    } elsif ($e->{operation} eq "open" ||
             $e->{operation} eq "truncate" ||
             $e->{operation} eq "mkdir" ||
             $e->{operation} eq "mknod" ||
             $e->{operation} eq "rename_src" ||
             $e->{operation} eq "rename_dest" ||
             $e->{operation} =~ m/^(unlink|rmdir|symlink_create|link)$/) {
        add_to_tree( $e->{pid},
		     $e->{parent},
                     "path",
                     $profile,
                     $hat,
                     $prog,
                     $sdmode,
                     $e->{denied_mask},
                     $e->{name},
		     "",
                   );
    } elsif ($e->{operation} eq "capable") {
        add_to_tree( $e->{pid},
		     $e->{parent},
                     "capability",
                     $profile,
                     $hat,
                     $prog,
                     $sdmode,
                     $e->{name}
                   );
    } elsif ($e->{operation} =~  m/xattr/ ||
             $e->{operation} eq "setattr") {
        add_to_tree( $e->{pid},
		     $e->{parent},
                     "path",
                     $profile,
                     $hat,
                     $prog,
                     $sdmode,
                     $e->{denied_mask},
                     $e->{name},
		     ""
                    );
    } elsif ($e->{operation} =~ m/inode_/) {
        my $is_domain_change = 0;

        if ($e->{operation}   eq "inode_permission" &&
            $e->{denied_mask} & $AA_MAY_EXEC                &&
            $sdmode           eq "PERMITTING") {

            my $following = peek_at_next_log_entry();
            if ($following) {
                my $entry = parse_log_record($following);
                if ($entry &&
                    $entry->{info} &&
                    $entry->{info} eq "set profile" ) {

                    $is_domain_change = 1;
                    throw_away_next_log_entry();
                }
            }
        }

        if ($is_domain_change) {
            add_to_tree( $e->{pid},
			 $e->{parent},
                          "exec",
                          $profile,
                          $hat,
                          $prog,
                          $sdmode,
                          $e->{denied_mask},
                          $e->{name},
			  $e->{name2}
                        );
        } else {
             add_to_tree( $e->{pid},
			  $e->{parent},
                          "path",
                          $profile,
                          $hat,
                          $prog,
                          $sdmode,
                          $e->{denied_mask},
                          $e->{name},
			  ""
                        );
        }
    } elsif ($e->{operation} eq "sysctl") {
        add_to_tree( $e->{pid},
		     $e->{parent},
                     "path",
                     $profile,
                     $hat,
                     $prog,
                     $sdmode,
                     $e->{denied_mask},
                     $e->{name},
		     ""
                   );
    } elsif ($e->{operation} eq "clone") {
        my ($parent, $child)  = ($e->{pid}, $e->{task});
        $profile ||= "null-complain-profile";
        $hat     ||= "null-complain-profile";
        my $arrayref = [];
        if (exists $pid{$parent}) {
            push @{ $pid{$parent} }, $arrayref;
        } else {
            push @log, $arrayref;
        }
        $pid{$child} = $arrayref;
        push @{$arrayref}, [ "fork", $child, $profile, $hat ];
    } elsif (optype($e->{operation}) eq "net") {
        add_to_tree( $e->{pid},
		     $e->{parent},
                     "netdomain",
                     $profile,
                     $hat,
                     $prog,
                     $sdmode,
                     $e->{family},
                     $e->{sock_type},
                     $e->{protocol},
                   );
    } elsif ($e->{operation} eq "change_hat") {
        add_to_tree($e->{pid}, $e->{parent}, "unknown_hat", $profile, $hat, $sdmode, $hat);
    } else {
        if ( $DEBUGGING ) {
            my $msg = Data::Dumper->Dump([$e], [qw(*event)]);
            debug "UNHANDLED: $msg";
        }
    }
}

sub read_log($) {
    $logmark = shift;
    $seenmark = $logmark ? 0 : 1;
    my $last;
    my $event_type;

    # okay, done loading the previous profiles, get on to the good stuff...
    open($LOG, $filename)
      or fatal_error "Can't read AppArmor logfile $filename: $!";
    while ($_ = get_next_log_entry()) {
        chomp;

	$DEBUGGING && debug "read_log: $_";

        $seenmark = 1 if /$logmark/;

	$DEBUGGING && debug "read_log: seenmark = $seenmark";
        next unless $seenmark;

        my $last_match = ""; # v_2_0 syslog record parsing requires
                             # the previous aa record in the mandatory profile
                             # case
        # all we care about is apparmor messages
        if (/$RE_LOG_v2_0_syslog/ || /$RE_LOG_v2_0_audit/) {
           $last_match = parse_log_record_v_2_0( $_, $last_match );
        } else {
            my $event = parse_log_record($_);
            add_event_to_tree($event) if ( $event );
        }
    }
    close($LOG);
    $logmark = "";
}


sub UI_SelectUpdatedRepoProfile ($$) {

    my ($profile, $p) = @_;
    my $distro        = $cfg->{repository}{distro};
    my $url           = $sd{$profile}{$profile}{repo}{url};
    my $user          = $sd{$profile}{$profile}{repo}{user};
    my $id            = $sd{$profile}{$profile}{repo}{id};
    my $updated       = 0;

    if ($p) {
        my $q = { };
        $q->{headers} = [
          "Profile", $profile,
          "User", $user,
          "Old Revision", $id,
          "New Revision", $p->{id},
        ];
        $q->{explanation} =
          gettext( "An updated version of this profile has been found in the profile repository.  Would you like to use it?");
        $q->{functions} = [
          "CMD_VIEW_CHANGES", "CMD_UPDATE_PROFILE", "CMD_IGNORE_UPDATE",
          "CMD_ABORT", "CMD_FINISHED"
        ];

        my $ans;
        do {
            $ans = UI_PromptUser($q);

            if ($ans eq "CMD_VIEW_CHANGES") {
                my $oldprofile = serialize_profile($sd{$profile}, $profile);
                my $newprofile = $p->{profile};
                display_changes($oldprofile, $newprofile);
            }
        } until $ans =~ /^CMD_(UPDATE_PROFILE|IGNORE_UPDATE)/;

        if ($ans eq "CMD_UPDATE_PROFILE") {
            eval {
                my $profile_data =
                  parse_profile_data($p->{profile}, getprofilefilename($profile), 0);
                if ($profile_data) {
                    attach_profile_data(\%sd, $profile_data);
                    $changed{$profile} = 1;
                }

                set_repo_info($sd{$profile}{$profile}, $url, $user, $p->{id});

                UI_Info(
                    sprintf(
                        gettext("Updated profile %s to revision %s."),
                        $profile, $p->{id}
                    )
                );
            };

            if ($@) {
                UI_Info(gettext("Error parsing repository profile."));
            } else {
                $updated = 1;
            }
        }
    }
    return $updated;
}

sub UI_repo_signup() {

    my ($url, $res, $save_config, $newuser, $user, $pass, $email, $signup_okay);
    $url = $cfg->{repository}{url};
    do {
        if ($UI_Mode eq "yast") {
            SendDataToYast(
                {
                    type     => "dialog-repo-sign-in",
                    repo_url => $url
                }
            );
            my ($ypath, $yarg) = GetDataFromYast();
            $email       = $yarg->{email};
            $user        = $yarg->{user};
            $pass        = $yarg->{pass};
            $newuser     = $yarg->{newuser};
            $save_config = $yarg->{save_config};
            if ($yarg->{cancelled} && $yarg->{cancelled} eq "y") {
                return;
            }
            $DEBUGGING && debug("AppArmor Repository: \n\t " .
                                ($newuser eq "1") ?
                                "New User\n\temail: [" . $email . "]" :
                                "Signin" . "\n\t user[" . $user . "]" .
                                "password [" . $pass . "]\n");
        } else {
            $newuser = UI_YesNo(gettext("Create New User?"), "n");
            $user    = UI_GetString(gettext("Username: "), $user);
            $pass    = UI_GetString(gettext("Password: "), $pass);
            $email   = UI_GetString(gettext("Email Addr: "), $email)
                         if ($newuser eq "y");
            $save_config = UI_YesNo(gettext("Save Configuration? "), "y");
        }

        if ($newuser eq "y") {
            my ($status_ok,$res) = user_register($url, $user, $pass, $email);
            if ($status_ok) {
                $signup_okay = 1;
            } else {
                my $errmsg =
                   gettext("The Profile Repository server returned the following error:") .
                   "\n" .  $res?$res:gettext("UNKOWN ERROR") .  "\n" .
                   gettext("Please re-enter registration information or contact the administrator.");
                UI_Important(gettext("Login Error\n") . $errmsg);
            }
        } else {
            my ($status_ok,$res) = user_login($url, $user, $pass);
            if ($status_ok) {
                $signup_okay = 1;
            } else {
                my $errmsg = gettext("Login failure\n Please check username and password and try again.") . "\n" . $res;
                UI_Important($errmsg);
            }
        }
    } until $signup_okay;

    $repo_cfg->{repository}{user} = $user;
    $repo_cfg->{repository}{pass} = $pass;
    $repo_cfg->{repository}{email} = $email;

    write_config("repository.conf", $repo_cfg) if ( $save_config eq "y" );

    return ($user, $pass);
}

sub UI_ask_to_enable_repo() {

    my $q = { };
    return if ( not defined $cfg->{repository}{url} );
    $q->{headers} = [
      gettext("Repository"), $cfg->{repository}{url},
    ];
    $q->{explanation} = gettext( "Would you like to enable access to the
profile repository?" ); $q->{functions} = [ "CMD_ENABLE_REPO",
"CMD_DISABLE_REPO", "CMD_ASK_LATER", ];

    my $cmd;
    do {
        $cmd = UI_PromptUser($q);
    } until $cmd =~ /^CMD_(ENABLE_REPO|DISABLE_REPO|ASK_LATER)/;

    if ($cmd eq "CMD_ENABLE_REPO") {
        $repo_cfg->{repository}{enabled} = "yes";
    } elsif ($cmd eq "CMD_DISABLE_REPO") {
        $repo_cfg->{repository}{enabled} = "no";
    } elsif ($cmd eq "CMD_ASK_LATER") {
        $repo_cfg->{repository}{enabled} = "later";
    }

    eval { write_config("repository.conf", $repo_cfg) };
    if ($@) {
        fatal_error($@);
    }
}


sub UI_ask_to_upload_profiles() {

    my $q = { };
    $q->{headers} = [
      gettext("Repository"), $cfg->{repository}{url},
    ];
    $q->{explanation} =
      gettext( "Would you like to upload newly created and changed profiles to
      the profile repository?" );
    $q->{functions} = [
      "CMD_YES", "CMD_NO", "CMD_ASK_LATER",
    ];

    my $cmd;
    do {
        $cmd = UI_PromptUser($q);
    } until $cmd =~ /^CMD_(YES|NO|ASK_LATER)/;

    if ($cmd eq "CMD_NO") {
        $repo_cfg->{repository}{upload} = "no";
    } elsif ($cmd eq "CMD_YES") {
        $repo_cfg->{repository}{upload} = "yes";
    } elsif ($cmd eq "CMD_ASK_LATER") {
        $repo_cfg->{repository}{upload} = "later";
    }

    eval { write_config("repository.conf", $repo_cfg) };
    if ($@) {
        fatal_error($@);
    }
}


sub parse_repo_profile($$$) {
    my ($fqdbin, $repo_url, $profile) = @_;

    my $profile_data = eval {
        parse_profile_data($profile->{profile}, getprofilefilename($fqdbin), 0);
    };
    if ($@) {
        print STDERR "PARSING ERROR: $@\n";
        $profile_data = undef;
    }

    if ($profile_data) {
        set_repo_info($profile_data->{$fqdbin}{$fqdbin}, $repo_url,
                      $profile->{username}, $profile->{id});
    }

    return $profile_data;
}


sub set_repo_info($$$$) {
    my ($profile_data, $repo_url, $username, $id) = @_;

    # save repository metadata
    $profile_data->{repo}{url}  = $repo_url;
    $profile_data->{repo}{user} = $username;
    $profile_data->{repo}{id}   = $id;
}


sub is_repo_profile($) {
    my $profile_data = shift;

    return $profile_data->{repo}{url}  &&
           $profile_data->{repo}{user} &&
           $profile_data->{repo}{id};
}


sub get_repo_user_pass() {
    my ($user, $pass);

    if ($repo_cfg) {
        $user = $repo_cfg->{repository}{user};
        $pass = $repo_cfg->{repository}{pass};
    }

    unless ($user && $pass) {
        ($user, $pass) = UI_repo_signup();
    }

    return ($user, $pass);
}


sub get_preferred_user ($) {
    my $repo_url = shift;
    return $cfg->{repository}{preferred_user} || "NOVELL";
}


sub repo_is_enabled () {
    my $enabled;
    if ($cfg->{repository}{url} &&
        $repo_cfg &&
        $repo_cfg->{repository}{enabled} &&
        $repo_cfg->{repository}{enabled} eq "yes") {
        $enabled = 1;
    }
    return $enabled;
}


sub update_repo_profile($) {
    my $profile = shift;

    return undef if ( not is_repo_profile($profile) );
    my $distro = $cfg->{repository}{distro};
    my $url    = $profile->{repo}{url};
    my $user   = $profile->{repo}{user};
    my $id     = $profile->{repo}{id};

    UI_BusyStart( gettext("Connecting to repository.....") );
    my ($status_ok,$res) = fetch_newer_profile( $url,
                                                $distro,
                                                $user,
                                                $id,
                                                $profile->{name}
                                              );
    UI_BusyStop();
    if ( ! $status_ok ) {
        my $errmsg =
          sprintf(
            gettext("WARNING: Profile update check failed\nError Detail:\n%s"),
            defined $res?$res:gettext("UNKNOWN ERROR"));
        UI_Important($errmsg);
        $res = undef;
    }
    return( $res );
}

sub UI_ask_mode_toggles ($$$) {
    my ($audit_toggle, $owner_toggle, $oldmode) = @_;
    my $q = { };
    $q->{headers} = [ ];
#      "Repository", $cfg->{repository}{url},
#    ];
    $q->{explanation} = gettext( "Change mode modifiers");

    if ($audit_toggle) {
	$q->{functions} = [ "CMD_AUDIT_OFF" ];
    } else {
	$q->{functions} = [ "CMD_AUDIT_NEW" ];
	push @{$q->{functions}}, "CMD_AUDIT_FULL" if ($oldmode);
    }

    if ($owner_toggle) {
	push @{$q->{functions}}, "CMD_USER_OFF";
    } else {
	push @{$q->{functions}}, "CMD_USER_ON";
    }
    push @{$q->{functions}}, "CMD_CONTINUE";

    my $cmd;
    do {
        $cmd = UI_PromptUser($q);
    } until $cmd =~ /^CMD_(AUDIT_OFF|AUDIT_NEW|AUDIT_FULL|USER_ON|USER_OFF|CONTINUE)/;

    if ($cmd eq "CMD_AUDIT_OFF") {
	$audit_toggle = 0;
    } elsif ($cmd eq "CMD_AUDIT_NEW") {
	$audit_toggle = 1;
    } elsif ($cmd eq "CMD_AUDIT_FULL") {
	$audit_toggle = 2;
    } elsif ($cmd eq "CMD_USER_ON") {
	$owner_toggle = 1;
    } elsif ($cmd eq "CMD_USER_OFF") {
	$owner_toggle = 0;
#	$owner_toggle++;
#	$owner_toggle++ if (!$oldmode && $owner_toggle == 2);
#	$owner_toggle = 0 if ($owner_toggle > 3);
    }
    return ($audit_toggle, $owner_toggle);
}

sub ask_the_questions() {
    my $found; # do the magic foo-foo
    for my $sdmode (sort keys %log) {

        # let them know what sort of changes we're about to list...
        if ($sdmode eq "PERMITTING") {
            UI_Info(gettext("Complain-mode changes:"));
        } elsif ($sdmode eq "REJECTING") {
            UI_Info(gettext("Enforce-mode changes:"));
        } else {

            # if we're not permitting and not rejecting, something's broken.
            # most likely  the code we're using to build the hash tree of log
            # entries - this should never ever happen
            fatal_error(sprintf(gettext('Invalid mode found: %s'), $sdmode));
        }

        for my $profile (sort keys %{ $log{$sdmode} }) {
            my $p = update_repo_profile($sd{$profile}{$profile});
            UI_SelectUpdatedRepoProfile($profile, $p) if ( $p );

            $found++;

            # this sorts the list of hats, but makes sure that the containing
            # profile shows up in the list first to keep the question order
            # rational
            my @hats =
              grep { $_ ne $profile } keys %{ $log{$sdmode}{$profile} };
            unshift @hats, $profile
              if defined $log{$sdmode}{$profile}{$profile};

            for my $hat (@hats) {

                # step through all the capabilities first...
                for my $capability (sort keys %{ $log{$sdmode}{$profile}{$hat}{capability} }) {

                    # we don't care about it if we've already added it to the
                    # profile
                    next if profile_known_capability($sd{$profile}{$hat},
						     $capability);

                    my $severity = $sevdb->rank(uc("cap_$capability"));

                    my $defaultoption = 1;
                    my @options       = ();
                    my @newincludes;
                    @newincludes = matchcapincludes($sd{$profile}{$hat},
                                                    $capability);


                    my $q = {};

                    if (@newincludes) {
                        push @options,
                          map { "#include <$_>" } sort(uniq(@newincludes));
                    }

                    if ( @options ) {
                        push @options, "capability $capability";
                        $q->{options}  = [@options];
                        $q->{selected} = $defaultoption - 1;
                    }

                    $q->{headers} = [];
                    push @{ $q->{headers} }, gettext("Profile"), combine_name($profile, $hat);
                    push @{ $q->{headers} }, gettext("Capability"), $capability;
                    push @{ $q->{headers} }, gettext("Severity"),   $severity;

		    my $audit_toggle = 0;
		    $q->{functions} = [
			"CMD_ALLOW", "CMD_DENY", "CMD_AUDIT_NEW", "CMD_ABORT", "CMD_FINISHED"
			];

                    # complain-mode events default to allow - enforce defaults
                    # to deny
                    $q->{default} = ($sdmode eq "PERMITTING") ? "CMD_ALLOW" : "CMD_DENY";

                    $seenevents++;
                    my $done = 0;
                    while ( not $done ) {
                        # what did the grand exalted master tell us to do?
                        my ($ans, $selected) = UI_PromptUser($q);

			if ($ans =~ /^CMD_AUDIT/) {
			    $audit_toggle = !$audit_toggle;
			    my $audit = "";
			    if ($audit_toggle) {
				$q->{functions} = [
				    "CMD_ALLOW", "CMD_DENY", "CMD_AUDIT_OFF", "CMD_ABORT", "CMD_FINISHED"
				    ];
				$audit = "audit ";
			    } else {
				$q->{functions} = [
				    "CMD_ALLOW", "CMD_DENY", "CMD_AUDIT_NEW", "CMD_ABORT", "CMD_FINISHED"
				    ];
			    }
			    $q->{headers} = [];
			    push @{ $q->{headers} }, gettext("Profile"), combine_name($profile, $hat);
			    push @{ $q->{headers} }, gettext("Capability"), $audit . $capability;
			    push @{ $q->{headers} }, gettext("Severity"),   $severity;

                        } if ($ans eq "CMD_ALLOW") {

                            # they picked (a)llow, so...

                            my $selection = $options[$selected];
                            $done = 1;
                            if ($selection &&
                                $selection =~ m/^#include <(.+)>$/) {
                                my $deleted = 0;
                                my $inc = $1;
                                $deleted = delete_duplicates($sd{$profile}{$hat},
                                                               $inc
                                                             );
                                $sd{$profile}{$hat}{include}{$inc} = 1;

                                $changed{$profile} = 1;
                                UI_Info(sprintf(
                                  gettext('Adding #include <%s> to profile.'),
                                          $inc));
                                UI_Info(sprintf(
                                  gettext('Deleted %s previous matching profile entries.'),
                                           $deleted)) if $deleted;
                            }
                            # stick the capability into the profile
                            $sd{$profile}{$hat}{allow}{capability}{$capability}{set} = 1;
                            $sd{$profile}{$hat}{allow}{capability}{$capability}{audit} = $audit_toggle;

                            # mark this profile as changed
                            $changed{$profile} = 1;
                            $done = 1;
                            # give a little feedback to the user
                            UI_Info(sprintf(gettext('Adding capability %s to profile.'), $capability));
                        } elsif ($ans eq "CMD_DENY") {
                            $sd{$profile}{$hat}{deny}{capability}{$capability}{set} = 1;
                            # mark this profile as changed
                            $changed{$profile} = 1;
                            UI_Info(sprintf(gettext('Denying capability %s to profile.'), $capability));
                            $done = 1;
                        } else {
                            redo;
                        }
                    }
                }

                # and then step through all of the path entries...
                for my $path (sort keys %{ $log{$sdmode}{$profile}{$hat}{path} }) {

                    my $mode = $log{$sdmode}{$profile}{$hat}{path}{$path};

		    # do original profile lookup once.

		    my $allow_mode = 0;
		    my $allow_audit = 0;
		    my $deny_mode = 0;
		    my $deny_audit = 0;

		    my ($fmode, $famode, $imode, $iamode, @fm, @im, $cm, $am, $cam, @m);
		    ($fmode, $famode, @fm) = rematchfrag($sd{$profile}{$hat}, 'allow', $path);
		    $allow_mode |= $fmode if $fmode;
		    $allow_audit |= $famode if $famode;
		    ($imode, $iamode, @im) = match_prof_incs_to_path($sd{$profile}{$hat}, 'allow', $path);
		    $allow_mode |= $imode if $imode;
		    $allow_audit |= $iamode if $iamode;

		    ($cm, $cam, @m) = rematchfrag($sd{$profile}{$hat}, 'deny', $path);
		    $deny_mode |= $cm if $cm;
		    $deny_audit |= $cam if $cam;
		    ($cm, $cam, @m) = match_prof_incs_to_path($sd{$profile}{$hat}, 'deny', $path);
		    $deny_mode |= $cm if $cm;
		    $deny_audit |= $cam if $cam;

		    if ($deny_mode & $AA_MAY_EXEC) {
			$deny_mode |= $ALL_AA_EXEC_TYPE;
		    }

		    # mask off the modes that have been denied
		    $mode &= ~$deny_mode;
		    $allow_mode &= ~$deny_mode;

                    # if we had an access(X_OK) request or some other kind of
                    # event that generates a "PERMITTING x" syslog entry,
                    # first check if it was already dealt with by a i/p/x
                    # question due to a exec().  if not, ask about adding ix
                    # permission.
                    if ($mode & $AA_MAY_EXEC) {

                        # get rid of the access() markers.
                        $mode &= (~$ALL_AA_EXEC_TYPE);

                        unless ($allow_mode & $allow_mode & $AA_MAY_EXEC) {
                            $mode |= str_to_mode("ix");
                        }
                    }

                    # if we had an mmap(PROT_EXEC) request, first check if we
                    # already have added an ix rule to the profile
                    if ($mode & $AA_EXEC_MMAP) {
                        # ix implies m.  don't ask if they want to add an "m"
                        # rule when we already have a matching ix rule.
                        if ($allow_mode && contains($allow_mode, "ix")) {
                            $mode &= (~$AA_EXEC_MMAP);
                        }
                    }

                    next unless $mode;


                    my @matches;

                    if ($fmode) {
                        push @matches, @fm;
                    }
                    if ($imode) {
                        push @matches, @im;
                    }

                    unless ($allow_mode && mode_contains($allow_mode, $mode)) {

                        my $defaultoption = 1;
                        my @options       = ();

                        # check the path against the available set of include
                        # files
                        my @newincludes;
                        my $includevalid;
                        for my $incname (keys %include) {
                            $includevalid = 0;

                            # don't suggest it if we're already including it,
                            # that's dumb
                            next if $sd{$profile}{$hat}{$incname};

                            # only match includes that can be suggested to
                            # the user
			    if ($cfg->{settings}{custom_includes}) {
                            for my $incm (split(/\s+/,
                                                $cfg->{settings}{custom_includes})
                                         ) {
                                $includevalid = 1 if $incname =~ /$incm/;
                            }
			    }
                            $includevalid = 1 if $incname =~ /abstractions/;
                            next if ($includevalid == 0);

                            ($cm, $am, @m) = match_include_to_path($incname, 'allow', $path);
                            if ($cm && mode_contains($cm, $mode)) {
				#make sure it doesn't deny $mode
				my $dm = match_include_to_path($incname, 'deny', $path);
				unless (($mode & $dm) || (grep { $_ eq "/**" } @m)) {
                                    push @newincludes, $incname;
                                }
                            }
                        }


                        # did any match?  add them to the option list...
                        if (@newincludes) {
                            push @options,
                              map { "#include <$_>" }
                              sort(uniq(@newincludes));
                        }

                        # include the literal path in the option list...
                        push @options, $path;

                        # match the current path against the globbing list in
                        # logprof.conf
                        my @globs = globcommon($path);
                        if (@globs) {
                            push @matches, @globs;
                        }

                        # suggest any matching globs the user manually entered
                        for my $userglob (@userglobs) {
                            push @matches, $userglob
                              if matchliteral($userglob, $path);
                        }

                        # we'll take the cheesy way and order the suggested
                        # globbing list by length, which is usually right,
                        # but not always always
                        push @options,
                          sort { length($b) <=> length($a) }
                          grep { $_ ne $path }
                          uniq(@matches);
                        $defaultoption = $#options + 1;

                        my $severity = $sevdb->rank($path, mode_to_str($mode));

			my $audit_toggle = 0;
			my $owner_toggle = $cfg->{settings}{default_owner_prompt};
                        my $done = 0;
                        while (not $done) {

                            my $q = {};
                            $q->{headers} = [];
                            push @{ $q->{headers} }, gettext("Profile"), combine_name($profile, $hat);
                            push @{ $q->{headers} }, gettext("Path"), $path;

                            # merge in any previous modes from this run
                            if ($allow_mode) {
				my $str;
#print "mode: " . print_mode($mode) . " allow: " . print_mode($allow_mode) . "\n";
                                $mode |= $allow_mode;
				my $tail;
				my $prompt_mode;
				if ($owner_toggle == 0) {
				    $prompt_mode = flatten_mode($mode);
				    $tail = "     " . gettext("(owner permissions off)");
				} elsif ($owner_toggle == 1) {
				    $prompt_mode = $mode;
				    $tail = "";
				} elsif ($owner_toggle == 2) {
				    $prompt_mode = $allow_mode | owner_flatten_mode($mode & ~$allow_mode);
				    $tail = "     " . gettext("(force new perms to owner)");
				} else {
				    $prompt_mode = owner_flatten_mode($mode);
				    $tail = "     " . gettext("(force all rule perms to owner)");
				}

				if ($audit_toggle == 1) {
				    $str = mode_to_str_user($allow_mode);
				    $str .= ", " if ($allow_mode);
				    $str .= "audit " . mode_to_str_user($prompt_mode & ~$allow_mode) . $tail;
				} elsif ($audit_toggle == 2) {
				    $str = "audit " . mode_to_str_user($prompt_mode) . $tail;
				} else {
				    $str = mode_to_str_user($prompt_mode) . $tail;
				}
                                push @{ $q->{headers} }, gettext("Old Mode"), mode_to_str_user($allow_mode);
                                push @{ $q->{headers} }, gettext("New Mode"), $str;
                            } else {
				my $str = "";
				if ($audit_toggle) {
				    $str = "audit ";
				}
				my $tail;
				my $prompt_mode;
				if ($owner_toggle == 0) {
				    $prompt_mode = flatten_mode($mode);
				    $tail = "     " . gettext("(owner permissions off)");
				} elsif ($owner_toggle == 1) {
				    $prompt_mode = $mode;
				    $tail = "";
				} else {
				    $prompt_mode = owner_flatten_mode($mode);
				    $tail = "     " . gettext("(force perms to owner)");
				}
				$str .= mode_to_str_user($prompt_mode) . $tail;
                                push @{ $q->{headers} }, gettext("Mode"), $str; 
                            }
                            push @{ $q->{headers} }, gettext("Severity"), $severity;

                            $q->{options}  = [@options];
                            $q->{selected} = $defaultoption - 1;

                            $q->{functions} = [
                              "CMD_ALLOW", "CMD_DENY", "CMD_GLOB", "CMD_GLOBEXT", "CMD_NEW",
                              "CMD_ABORT", "CMD_FINISHED", "CMD_OTHER"
                            ];

                            $q->{default} =
                              ($sdmode eq "PERMITTING")
                              ? "CMD_ALLOW"
                              : "CMD_DENY";

                            $seenevents++;
                            # if they just hit return, use the default answer
                            my ($ans, $selected) = UI_PromptUser($q);

			    if ($ans eq "CMD_OTHER") {

				($audit_toggle, $owner_toggle) = UI_ask_mode_toggles($audit_toggle, $owner_toggle, $allow_mode);
			    } elsif ($ans eq "CMD_USER_TOGGLE") {
				$owner_toggle++;
				$owner_toggle++ if (!$allow_mode && $owner_toggle == 2);
				$owner_toggle = 0 if ($owner_toggle > 3);
			    } elsif ($ans eq "CMD_ALLOW") {
                                $path = $options[$selected];
                                $done = 1;
                                if ($path =~ m/^#include <(.+)>$/) {
                                    my $inc = $1;
                                    my $deleted = 0;

                                    $deleted = delete_duplicates($sd{$profile}{$hat},
                                                                  $inc );

                                    # record the new entry
                                    $sd{$profile}{$hat}{include}{$inc} = 1;

                                    $changed{$profile} = 1;
                                    UI_Info(sprintf(gettext('Adding #include <%s> to profile.'), $inc));
                                    UI_Info(sprintf(gettext('Deleted %s previous matching profile entries.'), $deleted)) if $deleted;
                                } else {
                                    if ($sd{$profile}{$hat}{allow}{path}{$path}{mode}) {
                                        $mode |= $sd{$profile}{$hat}{allow}{path}{$path}{mode};
                                    }

                                    my $deleted = 0;
                                    for my $entry (keys %{ $sd{$profile}{$hat}{allow}{path} }) {

                                        next if $path eq $entry;

                                        if (matchregexp($path, $entry)) {

                                            # regexp matches, add it's mode to
                                            # the list to check against
                                            if (mode_contains($mode,
                                                $sd{$profile}{$hat}{allow}{path}{$entry}{mode})) {
                                                delete $sd{$profile}{$hat}{allow}{path}{$entry};
                                                $deleted++;
                                            }
                                        }
                                    }

                                    # record the new entry
				    if ($owner_toggle == 0) {
					$mode = flatten_mode($mode);
				    } elsif ($owner_toggle == 1) {
					$mode = $mode;
				    } elsif ($owner_toggle == 2) {
					$mode = $allow_mode | owner_flatten_mode($mode & ~$allow_mode);
				    } elsif  ($owner_toggle == 3) {
					$mode = owner_flatten_mode($mode);
				    }
                                    $sd{$profile}{$hat}{allow}{path}{$path}{mode} |= $mode;
				    my $tmpmode = ($audit_toggle == 1) ? $mode & ~$allow_mode : 0;
				    $tmpmode = ($audit_toggle == 2) ? $mode : $tmpmode;
                                    $sd{$profile}{$hat}{allow}{path}{$path}{audit} |= $tmpmode;

                                    $changed{$profile} = 1;
                                    UI_Info(sprintf(gettext('Adding %s %s to profile.'), $path, mode_to_str_user($mode)));
                                    UI_Info(sprintf(gettext('Deleted %s previous matching profile entries.'), $deleted)) if $deleted;
                                }
                            } elsif ($ans eq "CMD_DENY") {
				# record the new entry
				$sd{$profile}{$hat}{deny}{path}{$path}{mode} |= $mode & ~$allow_mode;
				$sd{$profile}{$hat}{deny}{path}{$path}{audit} |= 0;

				$changed{$profile} = 1;

                                # go on to the next entry without saving this
                                # one
                                $done = 1;
                            } elsif ($ans eq "CMD_NEW") {
                                my $arg = $options[$selected];
                                if ($arg !~ /^#include/) {
                                    $ans = UI_GetString(gettext("Enter new path: "), $arg);
                                    if ($ans) {
                                        unless (matchliteral($ans, $path)) {
                                            my $ynprompt = gettext("The specified path does not match this log entry:") . "\n\n";
                                            $ynprompt .= "  " . gettext("Log Entry") . ":    $path\n";
                                            $ynprompt .= "  " . gettext("Entered Path") . ": $ans\n\n";
                                            $ynprompt .= gettext("Do you really want to use this path?") . "\n";

                                            # we default to no if they just hit return...
                                            my $key = UI_YesNo($ynprompt, "n");

                                            next if $key eq "n";
                                        }

                                        # save this one for later
                                        push @userglobs, $ans;

                                        push @options, $ans;
                                        $defaultoption = $#options + 1;
                                    }
                                }
                            } elsif ($ans eq "CMD_GLOB") {

                                # do globbing if they don't have an include
                                # selected
                                my $newpath = $options[$selected];
                                chomp $newpath ;
                                unless ($newpath =~ /^#include/) {
                                    # is this entry directory specific
                                    if ( $newpath =~ m/\/$/ ) {
                                        # do we collapse to /* or /**?
                                        if ($newpath =~ m/\/\*{1,2}\/$/) {
                                            $newpath =~
                                            s/\/[^\/]+\/\*{1,2}\/$/\/\*\*\//;
                                        } else {
                                            $newpath =~ s/\/[^\/]+\/$/\/\*\//;
                                        }
                                    } else {
                                        # do we collapse to /* or /**?
                                        if ($newpath =~ m/\/\*{1,2}$/) {
                                            $newpath =~ s/\/[^\/]+\/\*{1,2}$/\/\*\*/;
                                        } else {
                                            $newpath =~ s/\/[^\/]+$/\/\*/;
                                        }
                                    }
                                    if (not grep { $newpath eq $_ } @options) {
                                        push @options, $newpath;	
                                        $defaultoption = $#options + 1;
                                    }
                                }
                            } elsif ($ans eq "CMD_GLOBEXT") {

                                # do globbing if they don't have an include
                                # selected
                                my $newpath = $options[$selected];
                                unless ($newpath =~ /^#include/) {
                                    # do we collapse to /*.ext or /**.ext?
                                    if ($newpath =~ m/\/\*{1,2}\.[^\/]+$/) {
                                        $newpath =~ s/\/[^\/]+\/\*{1,2}(\.[^\/]+)$/\/\*\*$1/;
                                    } else {
                                        $newpath =~ s/\/[^\/]+(\.[^\/]+)$/\/\*$1/;
                                    }
                                    if (not grep { $newpath eq $_ } @options) {
                                        push @options, $newpath;
                                        $defaultoption = $#options + 1;
                                    }
                                }
                            } elsif ($ans =~ /\d/) {
                                $defaultoption = $ans;
                            }
                        }
                    }
                }

                # and then step through all of the netdomain entries...
                for my $family (sort keys %{$log{$sdmode}
                                                {$profile}
                                                {$hat}
                                                {netdomain}}) {

                    # TODO - severity handling for net toggles
                    #my $severity = $sevdb->rank();
                    for my $sock_type (sort keys %{$log{$sdmode}
                                                       {$profile}
                                                       {$hat}
                                                       {netdomain}
                                                       {$family}}) {

                        # we don't care about it if we've already added it to the
                        # profile
                        next if ( profile_known_network($sd{$profile}{$hat},
							$family,
							$sock_type));
                        my $defaultoption = 1;
                        my @options       = ();
                        my @newincludes;
                        @newincludes = matchnetincludes($sd{$profile}{$hat},
                                                        $family,
                                                        $sock_type);

                        my $q = {};

                        if (@newincludes) {
                            push @options,
                              map { "#include <$_>" } sort(uniq(@newincludes));
                        }

                        if ( @options ) {
                            push @options, "network $family $sock_type";
                            $q->{options}  = [@options];
                            $q->{selected} = $defaultoption - 1;
                        }

                        $q->{headers} = [];
                        push @{ $q->{headers} },
                             gettext("Profile"),
                             combine_name($profile, $hat);
                        push @{ $q->{headers} },
                             gettext("Network Family"),
                             $family;
                        push @{ $q->{headers} },
                             gettext("Socket Type"),
                             $sock_type;

			my $audit_toggle = 0;

                        $q->{functions} = [
                                            "CMD_ALLOW",
                                            "CMD_DENY",
					    "CMD_AUDIT_NEW",
                                            "CMD_ABORT",
                                            "CMD_FINISHED"
                                          ];

                        # complain-mode events default to allow - enforce defaults
                        # to deny
                        $q->{default} = ($sdmode eq "PERMITTING") ? "CMD_ALLOW" :
                                                                    "CMD_DENY";

                        $seenevents++;

                        # what did the grand exalted master tell us to do?
                        my $done = 0;
                        while ( not $done ) {
                            my ($ans, $selected) = UI_PromptUser($q);
			    if ($ans =~ /^CMD_AUDIT/) {
				$audit_toggle = !$audit_toggle;
				my $audit = $audit_toggle ? "audit " : "";
				if ($audit_toggle) {
				    $q->{functions} = [
					"CMD_ALLOW",
					"CMD_DENY",
					"CMD_AUDIT_OFF",
					"CMD_ABORT",
					"CMD_FINISHED"
					];
				} else {
				    $q->{functions} = [
					"CMD_ALLOW",
					"CMD_DENY",
					"CMD_AUDIT_NEW",
					"CMD_ABORT",
					"CMD_FINISHED"
					];
				}
				$q->{headers} = [];
				push @{ $q->{headers} },
				gettext("Profile"),
				combine_name($profile, $hat);
				push @{ $q->{headers} },
				gettext("Network Family"),
				$audit . $family;
				push @{ $q->{headers} },
				gettext("Socket Type"),
				$sock_type;
                            } elsif ($ans eq "CMD_ALLOW") {
                                my $selection = $options[$selected];
                                $done = 1;
                                if ($selection &&
                                    $selection =~ m/^#include <(.+)>$/) {
                                    my $inc = $1;
                                    my $deleted = 0;
                                    $deleted = delete_duplicates($sd{$profile}{$hat},
                                                                   $inc
                                                                 );
                                    # record the new entry
                                    $sd{$profile}{$hat}{include}{$inc} = 1;

                                    $changed{$profile} = 1;
                                    UI_Info(
                                      sprintf(
                                        gettext('Adding #include <%s> to profile.'),
                                                $inc));
                                    UI_Info(
                                      sprintf(
                                        gettext('Deleted %s previous matching profile entries.'),
                                                 $deleted)) if $deleted;
                                } else {

                                    # stick the whole rule into the profile
                                    $sd{$profile}
                                       {$hat}
				       {allow}
                                       {netdomain}
				       {audit}
                                       {$family}
                                       {$sock_type} = $audit_toggle;

                                    $sd{$profile}
                                       {$hat}
				       {allow}
                                       {netdomain}
				       {rule}
                                       {$family}
                                       {$sock_type} = 1;

                                    # mark this profile as changed
                                    $changed{$profile} = 1;

                                    # give a little feedback to the user
                                    UI_Info(sprintf(
                                           gettext('Adding network access %s %s to profile.'),
                                                    $family,
                                                    $sock_type
                                                   )
                                           );
                                }
                            } elsif ($ans eq "CMD_DENY") {
                                $done = 1;
				# record the new entry
                                    $sd{$profile}
                                       {$hat}
				       {deny}
                                       {netdomain}
				       {rule}
                                       {$family}
                                       {$sock_type} = 1;

				$changed{$profile} = 1;
                                UI_Info(sprintf(
                                        gettext('Denying network access %s %s to profile.'),
                                                $family,
                                                $sock_type
                                               )
                                       );
                            } else {
                                redo;
                            }
                        }
                    }
                }
            }
        }
    }
}

sub delete_net_duplicates($$) {
    my ($netrules, $incnetrules) = @_;
    my $deleted = 0;
    if ( $incnetrules && $netrules ) {
        my $incnetglob = defined $incnetrules->{all};

        # See which if any profile rules are matched by the include and can be
        # deleted
        for my $fam ( keys %$netrules ) {
            if ( $incnetglob || (ref($incnetrules->{rule}{$fam}) ne "HASH" &&
                                 $incnetrules->{rule}{$fam} == 1)) { # include allows
                                                               # all net or
                                                               # all fam
                if ( ref($netrules->{rule}{$fam}) eq "HASH" ) {
                    $deleted += ( keys %{$netrules->{rule}{$fam}} );
                } else {
                    $deleted++;
                }
                delete $netrules->{rule}{$fam};
            } elsif ( ref($netrules->{rule}{$fam}) ne "HASH" &&
                      $netrules->{rule}{$fam} == 1 ){
                next; # profile has all family
            } else {
                for my $socket_type ( keys %{$netrules->{rule}{$fam}} )  {
                    if ( defined $incnetrules->{$fam}{$socket_type} ) {
                        delete $netrules->{$fam}{$socket_type};
                        $deleted++;
                    }
                }
            }
        }
    }
    return $deleted;
}

sub delete_cap_duplicates ($$) {
    my ($profilecaps, $inccaps) = @_;
    my $deleted = 0;
    if ( $profilecaps && $inccaps ) {
        for my $capname ( keys %$profilecaps ) {
            if ( defined $inccaps->{$capname}{set} && $inccaps->{$capname}{set} == 1 ) {
               delete $profilecaps->{$capname};
               $deleted++;
            }
        }
    }
    return $deleted;
}

sub delete_path_duplicates ($$$) {
    my ($profile, $incname, $allow) = @_;
    my $deleted = 0;

    for my $entry (keys %{ $profile->{$allow}{path} }) {
        next if $entry eq "#include <$incname>";
	my ($cm, $am, @m) = match_include_to_path($incname, $allow, $entry);
        if ($cm
            && mode_contains($cm, $profile->{$allow}{path}{$entry}{mode})
	    && mode_contains($am, $profile->{$allow}{path}{$entry}{audit}))
        {
            delete $profile->{$allow}{path}{$entry};
            $deleted++;
        }
    }
    return $deleted;
}

sub delete_duplicates (\%$) {
    my ( $profile, $incname ) = @_;
    my $deleted = 0;

    # don't cross delete allow rules covered by denied rules as the coverage
    # may not be complete.  ie. want to deny a subset of allow, allow a subset
    # of deny with different perms.

    ## network rules
    $deleted += delete_net_duplicates($profile->{allow}{netdomain}, $include{$incname}{$incname}{allow}{netdomain});
    $deleted += delete_net_duplicates($profile->{deny}{netdomain}, $include{$incname}{$incname}{deny}{netdomain});

    ## capabilities
    $deleted += delete_cap_duplicates($profile->{allow}{capability},
				     $include{$incname}{$incname}{allow}{capability});
    $deleted += delete_cap_duplicates($profile->{deny}{capability},
				     $include{$incname}{$incname}{deny}{capability});

    ## paths
    $deleted += delete_path_duplicates($profile, $incname, 'allow');
    $deleted += delete_path_duplicates($profile, $incname, 'deny');

    return $deleted;
}

sub matchnetinclude ($$$) {
    my ($incname, $family, $type) = @_;

    my @matches;

    # scan the include fragments for this profile looking for matches
    my @includelist = ($incname);
    my @checked;
    while (my $name = shift @includelist) {
        push @checked, $name;
        return 1
          if netrules_access_check($include{$name}{$name}{allow}{netdomain}, $family, $type);
        # if this fragment includes others, check them too
        if (keys %{ $include{$name}{$name}{include} } &&
            (grep($name, @checked) == 0) ) {
            push @includelist, keys %{ $include{$name}{$name}{include} };
        }
    }
    return 0;
}

sub matchcapincludes (\%$) {
    my ($profile, $cap) = @_;

    # check the path against the available set of include
    # files
    my @newincludes;
    my $includevalid;
    for my $incname (keys %include) {
	$includevalid = 0;

	# don't suggest it if we're already including it,
	# that's dumb
	next if $profile->{include}{$incname};

	# only match includes that can be suggested to
	# the user
	if ($cfg->{settings}{custom_includes}) {
	    for my $incm (split(/\s+/,
				$cfg->{settings}{custom_includes})) {
		$includevalid = 1 if $incname =~ /$incm/;
	    }
	}
	$includevalid = 1 if $incname =~ /abstractions/;
	next if ($includevalid == 0);

	push @newincludes, $incname
	    if ( defined $include{$incname}{$incname}{allow}{capability}{$cap}{set} &&
		 $include{$incname}{$incname}{allow}{capability}{$cap}{set} == 1 );
    }
    return @newincludes;
}

sub matchnetincludes (\%$$) {
    my ($profile, $family, $type) = @_;

    # check the path against the available set of include
    # files
    my @newincludes;
    my $includevalid;
    for my $incname (keys %include) {
	$includevalid = 0;

	# don't suggest it if we're already including it,
	# that's dumb
	next if $profile->{include}{$incname};

	# only match includes that can be suggested to
	# the user
	if ($cfg->{settings}{custom_includes}) {
	    for my $incm (split(/\s+/, $cfg->{settings}{custom_includes})) {
		$includevalid = 1 if $incname =~ /$incm/;
	    }
	}
	$includevalid = 1 if $incname =~ /abstractions/;
	next if ($includevalid == 0);

	push @newincludes, $incname
	    if matchnetinclude($incname, $family, $type);
    }
    return @newincludes;
}


sub do_logprof_pass($) {
    my $logmark = shift || "";

    # zero out the state variables for this pass...
    %t              = ( );
    %transitions    = ( );
    %seen           = ( );
    %sd             = ( );
    %profilechanges = ( );
    %prelog         = ( );
    @log            = ( );
    %log            = ( );
    %changed        = ( );
    %skip           = ( );
    %filelist       = ( );

    UI_Info(sprintf(gettext('Reading log entries from %s.'), $filename));
    UI_Info(sprintf(gettext('Updating AppArmor profiles in %s.'), $profiledir));

    readprofiles();
    unless ($sevdb) {
        $sevdb = new Immunix::Severity("$confdir/severity.db", gettext("unknown
"));
    }

    # we need to be able to break all the way out of deep into subroutine calls
    # if they select "Finish" so we can take them back out to the genprof prompt
    eval {
        unless ($repo_cfg || not defined $cfg->{repository}{url}) {
            $repo_cfg = read_config("repository.conf");
            unless ($repo_cfg->{repository}{enabled} &&
                    ($repo_cfg->{repository}{enabled} eq "yes" ||
                     $repo_cfg->{repository}{enabled} eq "no")) {
                UI_ask_to_enable_repo();
            }
        }

        read_log($logmark);

        for my $root (@log) {
            handlechildren(undef, undef, $root);
        }

        for my $pid (sort { $a <=> $b } keys %profilechanges) {
            setprocess($pid, $profilechanges{$pid});
        }

        collapselog();

        ask_the_questions();

        if ($UI_Mode eq "yast") {
            if (not $running_under_genprof) {
                if ($seenevents) {
                    my $w = { type => "wizard" };
                    $w->{explanation} = gettext("The profile analyzer has completed processing the log files.\n\nAll updated profiles will be reloaded");
                    $w->{functions} = [ "CMD_ABORT", "CMD_FINISHED" ];
                    SendDataToYast($w);
                    my $foo = GetDataFromYast();
                } else {
                    my $w = { type => "wizard" };
                    $w->{explanation} = gettext("No unhandled AppArmor events were found in the system log.");
                    $w->{functions} = [ "CMD_ABORT", "CMD_FINISHED" ];
                    SendDataToYast($w);
                    my $foo = GetDataFromYast();
                }
            }
        }
    };

    my $finishing = 0;
    if ($@) {
        if ($@ =~ /FINISHING/) {
            $finishing = 1;
        } else {
            die $@;
        }
    }

    save_profiles();

    if (repo_is_enabled()) {
        if ( (not defined $repo_cfg->{repository}{upload}) ||
             ($repo_cfg->{repository}{upload} eq "later") ) {
	    UI_ask_to_upload_profiles();
        }
        if ($repo_cfg->{repository}{upload} eq "yes") {
            sync_profiles();
        }
        @created = ();
    }

    # if they hit "Finish" we need to tell the caller that so we can exit
    # all the way instead of just going back to the genprof prompt
    return $finishing ? "FINISHED" : "NORMAL";
}

sub save_profiles() {
    # make sure the profile changes we've made are saved to disk...
    my @changed = sort keys %changed;
    #
    # first make sure that profiles in %changed are active (or actual profiles
    # in %sd) - this is to handle the sloppiness of setting profiles as changed
    # when they are parsed in the case of legacy hat code that we want to write
    # out in an updated format
    foreach  my $profile_name ( keys %changed ) {
        if ( ! is_active_profile( $profile_name ) ) {
            delete $changed{ $profile_name };
        }
    }
    @changed = sort keys %changed;

    if (@changed) {
        if ($UI_Mode eq "yast") {
            my (@selected_profiles, $title, $explanation, %profile_changes);
            foreach my $prof (@changed) {
                my $oldprofile = serialize_profile($original_sd{$prof}, $prof);
                my $newprofile = serialize_profile($sd{$prof}, $prof);

                $profile_changes{$prof} = get_profile_diff($oldprofile,
                                                           $newprofile);
            }
            $explanation = gettext("Select which profile changes you would like to save to the\nlocal profile set");
            $title       = gettext("Local profile changes");
            SendDataToYast(
                {
                    type           => "dialog-select-profiles",
                    title          => $title,
                    explanation    => $explanation,
                    default_select => "true",
                    get_changelog  => "false",
                    profiles       => \%profile_changes
                }
            );
            my ($ypath, $yarg) = GetDataFromYast();
            if ($yarg->{STATUS} eq "cancel") {
                return;
            } else {
                my $selected_profiles_ref = $yarg->{PROFILES};
                for my $profile (@$selected_profiles_ref) {
                    writeprofile_ui_feedback($profile);
                    reload_base($profile);
                }
            }
        } else {
            my $q = {};
            $q->{title}   = "Changed Local Profiles";
            $q->{headers} = [];

            $q->{explanation} =
              gettext( "The following local profiles were changed.  Would you like to save them?");

            $q->{functions} = [ "CMD_SAVE_CHANGES",
                                "CMD_VIEW_CHANGES",
                                "CMD_ABORT", ];

            $q->{default} = "CMD_VIEW_CHANGES";

            $q->{options}  = [@changed];
            $q->{selected} = 0;

            my ($p, $ans, $arg);
            do {
                ($ans, $arg) = UI_PromptUser($q);

                if ($ans eq "CMD_VIEW_CHANGES") {
                    my $which      = $changed[$arg];
                    my $oldprofile =
                      serialize_profile($original_sd{$which}, $which);
                    my $newprofile = serialize_profile($sd{$which}, $which);
                    display_changes($oldprofile, $newprofile);
                }

            } until $ans =~ /^CMD_SAVE_CHANGES/;

            for my $profile (sort keys %changed) {
                writeprofile_ui_feedback($profile);
                reload_base($profile);
            }
        }
    }
}


sub get_pager() {

    if ( $ENV{PAGER} and (-x "/usr/bin/$ENV{PAGER}" ||
                          -x "/usr/sbin/$ENV{PAGER}" )
       ) {
        return $ENV{PAGER};
    } else {
        return "less"
    }
}


sub display_text($$) {
    my ($header, $body) = @_;
    my $pager = get_pager();
    if (open(PAGER, "| $pager")) {
        print PAGER "$header\n\n$body";
        close(PAGER);
    }
}

sub get_profile_diff($$) {
    my ($oldprofile, $newprofile) = @_;
    my $oldtmp = new File::Temp(UNLINK => 0);
    print $oldtmp $oldprofile;
    close($oldtmp);

    my $newtmp = new File::Temp(UNLINK => 0);
    print $newtmp $newprofile;
    close($newtmp);

    my $difftmp = new File::Temp(UNLINK => 0);
    my @diff;
    system("diff -u $oldtmp $newtmp > $difftmp");
    while (<$difftmp>) {
        push(@diff, $_) unless (($_ =~ /^(---|\+\+\+)/) ||
                                ($_ =~ /^\@\@.*\@\@$/));
    }
    unlink($difftmp);
    unlink($oldtmp);
    unlink($newtmp);
    return join("", @diff);
}

sub display_changes($$) {
    my ($oldprofile, $newprofile) = @_;

    my $oldtmp = new File::Temp( UNLINK => 0 );
    print $oldtmp $oldprofile;
    close($oldtmp);

    my $newtmp = new File::Temp( UNLINK => 0 );
    print $newtmp $newprofile;
    close($newtmp);

    my $difftmp = new File::Temp(UNLINK => 0);
    my @diff;
    system("diff -u $oldtmp $newtmp > $difftmp");
    if ($UI_Mode eq "yast") {
        while (<$difftmp>) {
            push(@diff, $_) unless (($_ =~ /^(---|\+\+\+)/) ||
                                    ($_ =~ /^\@\@.*\@\@$/));
        }
        UI_LongMessage(gettext("Profile Changes"), join("", @diff));
    } else {
        system("less $difftmp");
    }

    unlink($difftmp);
    unlink($oldtmp);
    unlink($newtmp);
}

sub setprocess ($$) {
    my ($pid, $profile) = @_;

    # don't do anything if the process exited already...
    return unless -e "/proc/$pid/attr/current";

    return unless open(CURR, "/proc/$pid/attr/current");
    my $current = <CURR>;
    return unless $current;
    chomp $current;
    close(CURR);

    # only change null profiles
    return unless $current =~ /null(-complain)*-profile/;

    return unless open(STAT, "/proc/$pid/stat");
    my $stat = <STAT>;
    chomp $stat;
    close(STAT);

    return unless $stat =~ /^\d+ \((\S+)\) /;
    my $currprog = $1;

    open(CURR, ">/proc/$pid/attr/current") or return;
    print CURR "setprofile $profile";
    close(CURR);
}

sub collapselog () {
    for my $sdmode (keys %prelog) {
        for my $profile (keys %{ $prelog{$sdmode} }) {
            for my $hat (keys %{ $prelog{$sdmode}{$profile} }) {
                for my $path (keys %{ $prelog{$sdmode}{$profile}{$hat}{path} }) {

                    my $mode = $prelog{$sdmode}{$profile}{$hat}{path}{$path};

                    # we want to ignore anything from the log that's already
                    # in the profile
                    my $combinedmode = 0;

                    # is it in the original profile?
                    if ($sd{$profile}{$hat}{allow}{path}{$path}) {
                        $combinedmode |= $sd{$profile}{$hat}{allow}{path}{$path}{mode};
                    }

                    # does path match any regexps in original profile?
                    $combinedmode |= rematchfrag($sd{$profile}{$hat}, 'allow', $path);

                    # does path match anything pulled in by includes in
                    # original profile?
                    $combinedmode |= match_prof_incs_to_path($sd{$profile}{$hat}, 'allow', $path);

                    # if we found any matching entries, do the modes match?
                    unless ($combinedmode && mode_contains($combinedmode, $mode)) {

                        # merge in any previous modes from this run
                        if ($log{$sdmode}{$profile}{$hat}{$path}) {
                            $mode |= $log{$sdmode}{$profile}{$hat}{path}{$path};
                        }

                        # record the new entry
                        $log{$sdmode}{$profile}{$hat}{path}{$path} = $mode;
                    }
                }

                for my $capability (keys %{ $prelog{$sdmode}{$profile}{$hat}{capability} }) {

                    # if we don't already have this capability in the profile,
                    # add it
                    unless ($sd{$profile}{$hat}{allow}{capability}{$capability}{set}) {
                        $log{$sdmode}{$profile}{$hat}{capability}{$capability} = 1;
                    }
                }

                # Network toggle handling
                my $ndref = $prelog{$sdmode}{$profile}{$hat}{netdomain};
                for my $family ( keys %{$ndref} ) {
                    for my $sock_type ( keys %{$ndref->{$family}} ) {
                        unless ( profile_known_network($sd{$profile}{$hat},
						       $family, $sock_type)) {
                            $log{$sdmode}
                                {$profile}
                                {$hat}
                                {netdomain}
                                {$family}
                                {$sock_type}=1;
                        }
                    }
                }
            }
        }
    }
}

sub profilemode ($) {
    my $mode = shift;

    my $modifier = ($mode =~ m/[iupUP]/)[0];
    if ($modifier) {
        $mode =~ s/[iupUPx]//g;
        $mode .= $modifier . "x";
    }

    return $mode;
}

# kinky.
sub commonprefix (@) { (join("\0", @_) =~ m/^([^\0]*)[^\0]*(\0\1[^\0]*)*$/)[0] }
sub commonsuffix (@) { reverse(((reverse join("\0", @_)) =~ m/^([^\0]*)[^\0]*(\0\1[^\0]*)*$/)[0]); }

sub uniq (@) {
    my %seen;
    my @result = sort grep { !$seen{$_}++ } @_;
    return @result;
}

our $MODE_MAP_RE = "r|w|l|m|k|a|x|i|u|p|c|n|I|U|P|C|N";
our $LOG_MODE_RE = "r|w|l|m|k|a|x|ix|ux|px|cx|nx|pix|cix|Ix|Ux|Px|PUx|Cx|Nx|Pix|Cix";
our $PROFILE_MODE_RE = "r|w|l|m|k|a|ix|ux|px|cx|pix|cix|Ux|Px|PUx|Cx|Pix|Cix";
our $PROFILE_MODE_NT_RE = "r|w|l|m|k|a|x|ix|ux|px|cx|pix|cix|Ux|Px|PUx|Cx|Pix|Cix";
our $PROFILE_MODE_DENY_RE = "r|w|l|m|k|a|x";

sub split_log_mode($) {
    my $mode = shift;
    my $user = "";
    my $other = "";

    if ($mode =~ /(.*?)::(.*)/) {
	$user = $1 if ($1);
	$other = $2 if ($2);
    } else {
	$user = $mode;
	$other = $mode;
    }
    return ($user, $other);
}

sub map_log_mode ($) {
    my $mode = shift;
    return $mode;
#    $mode =~ s/(.*l.*)::.*/$1/ge;
#    $mode =~ s/.*::(.*l.*)/$1/ge;
#    $mode =~ s/:://;
#     return $mode;
#    return $1;
}

sub hide_log_mode($) {
    my $mode = shift;

    $mode =~ s/:://;
    return $mode;
}

sub validate_log_mode ($) {
    my $mode = shift;

    return ($mode =~ /^($LOG_MODE_RE)+$/) ? 1 : 0;
}

sub validate_profile_mode ($$$) {
    my ($mode, $allow, $nt_name) = @_;

    if ($allow eq 'deny') {
	return ($mode =~ /^($PROFILE_MODE_DENY_RE)+$/) ? 1 : 0;
    } elsif ($nt_name) {
	return ($mode =~ /^($PROFILE_MODE_NT_RE)+$/) ? 1 : 0;
    }

    return ($mode =~ /^($PROFILE_MODE_RE)+$/) ? 1 : 0;
}

# modes internally are stored as a bit Mask
sub sub_str_to_mode($) {
    my $str = shift;
    my $mode = 0;

    return 0 if (not $str);

    while ($str =~ s/(${MODE_MAP_RE})//) {
	my $tmp = $1;
#print "found mode $1\n";

	if ($tmp && $MODE_HASH{$tmp}) {
	    $mode |= $MODE_HASH{$tmp};
	} else {
#print "found mode $tmp\n";
	}
    }

#my $tmp = mode_to_str($mode);
#print "parsed_mode $mode\n";
    return $mode;
}

sub print_mode ($) {
    my $mode = shift;

    my ($user, $other) = split_mode($mode);
    my $str = sub_mode_to_str($user) . "::" . sub_mode_to_str($other);

    return $str;
}

sub str_to_mode ($) {
    my $str = shift;

    return 0 if (not $str);

    my ($user, $other) = split_log_mode($str);

#print "str: $str  user: $user, other $other\n";
    # we only allow user or all
    $user = $other if (!$user);

    my $mode = sub_str_to_mode($user);
    $mode |= (sub_str_to_mode($other) << $AA_OTHER_SHIFT);

#print "user: $user " .sub_str_to_mode($user) . " other: $other " . (sub_str_to_mode($other) << $AA_OTHER_SHIFT) . " mode = $mode\n";

    return $mode;
}

sub log_str_to_mode($$$) {
    my ($profile, $str, $nt_name) = @_;

    my $mode = str_to_mode($str);

    # this will cover both nx and nix
    if (contains($mode, "Nx")) {
	# need to transform to px, cx

	if ($nt_name =~ /(.+?)\/\/(.+?)/) {
	    my ($lprofile, $lhat) = @_;
	    my $tmode = 0;
	    if ($profile eq $profile) {
		if ($mode & ($AA_MAY_EXEC)) {
		    $tmode = str_to_mode("Cx::");
		}
		if ($mode & ($AA_MAY_EXEC << $AA_OTHER_SHIFT)) {
		    $tmode |= str_to_mode("Cx");
		}
		$nt_name = $lhat;
	    } else {
		if ($mode & ($AA_MAY_EXEC)) {
		    $tmode = str_to_mode("Px::");
		}
		if ($mode & ($AA_MAY_EXEC << $AA_OTHER_SHIFT)) {
		    $tmode |= str_to_mode("Px");
		}
		$nt_name = $lhat;
	    }
	    $mode = ($mode & ~(str_to_mode("Nx")));
	    $mode |= $tmode;
	}
    }
    return ($mode, $nt_name);
}

sub split_mode ($) {
    my $mode = shift;

    my $user = $mode & $AA_USER_MASK;
    my $other = ($mode >> $AA_OTHER_SHIFT) & $AA_USER_MASK;

    return ($user, $other);
}

sub is_user_mode ($) {
    my $mode = shift;

    my ($user, $other) = split_mode($mode);

    if ($user && !$other) {
	return 1;
    }
    return 0;
}

sub sub_mode_to_str($) {
    my $mode = shift;
    my $str = "";

    # "w" implies "a"
    $mode &= (~$AA_MAY_APPEND) if ($mode & $AA_MAY_WRITE);
    $str .= "m" if ($mode & $AA_EXEC_MMAP);
    $str .= "r" if ($mode & $AA_MAY_READ);
    $str .= "w" if ($mode & $AA_MAY_WRITE);
    $str .= "a" if ($mode & $AA_MAY_APPEND);
    $str .= "l" if ($mode & $AA_MAY_LINK);
    $str .= "k" if ($mode & $AA_MAY_LOCK);

    # modes P and C *must* come before I and U; otherwise syntactically
    # invalid profiles result
    if ($mode & ($AA_EXEC_PROFILE | $AA_EXEC_NT)) {
	if ($mode & $AA_EXEC_UNSAFE) {
	    $str .= "p";
	} else {
	    $str .= "P";
	}
    }
    if ($mode & $AA_EXEC_CHILD) {
	if ($mode & $AA_EXEC_UNSAFE) {
	    $str .= "c";
	} else {
	    $str .= "C";
	}
    }

    # modes P and C *must* come before I and U; otherwise syntactically
    # invalid profiles result
    if ($mode & $AA_EXEC_UNCONFINED) {
	if ($mode & $AA_EXEC_UNSAFE) {
	    $str .= "u";
	} else {
	    $str .= "U";
	}
    }
    $str .= "i" if ($mode & $AA_EXEC_INHERIT);

    $str .= "x" if ($mode & $AA_MAY_EXEC);

    return $str;
}

sub flatten_mode ($) {
    my $mode = shift;

    return 0 if (!$mode);

    $mode = ($mode & $AA_USER_MASK) | (($mode >> $AA_OTHER_SHIFT) & $AA_USER_MASK);
    $mode |= ($mode << $AA_OTHER_SHIFT);
}

sub mode_to_str ($) {
    my $mode = shift;
    $mode = flatten_mode($mode);
    return sub_mode_to_str($mode);
}

sub owner_flatten_mode($) {
    my $mode = shift;
    $mode = flatten_mode($mode) & $AA_USER_MASK;
    return $mode;
}

sub mode_to_str_user ($) {
    my $mode = shift;

    my ($user, $other) = split_mode($mode);

    my $str = "";
    $user = 0 if (!$user);
    $other = 0 if (!$other);

    if ($user & ~$other) {
	# more user perms than other
	$str = sub_mode_to_str($other). " + " if ($other);
	$str .= "owner " . sub_mode_to_str($user & ~$other);
    } elsif (is_user_mode($mode)) {
	$str = "owner " . sub_mode_to_str($user);
    } else {
	$str = sub_mode_to_str(flatten_mode($mode));
    }
    return $str;
}

sub mode_contains ($$) {
    my ($mode, $subset) = @_;

    # "w" implies "a"
    if ($mode & $AA_MAY_WRITE) {
	$mode |= $AA_MAY_APPEND;
    }
    if ($mode & ($AA_MAY_WRITE << $AA_OTHER_SHIFT)) {
	$mode |= ($AA_MAY_APPEND << $AA_OTHER_SHIFT);
    }

    # "?ix" implies "m"
    if ($mode & $AA_EXEC_INHERIT) {
	$mode |= $AA_EXEC_MMAP;
    }
    if ($mode & ($AA_EXEC_INHERIT << $AA_OTHER_SHIFT)) {
	$mode |= ($AA_EXEC_MMAP << $AA_OTHER_SHIFT);
    }

    return (($mode & $subset) == $subset);
}

sub contains ($$) {
    my ($mode, $str) = @_;

    return mode_contains($mode, str_to_mode($str));
}

# isSkippableFile - return true if filename matches something that
# should be skipped (rpm backup files, dotfiles, emacs backup files
# Annoyingly, this needs to be kept in sync with the skipped files
# in the apparmor initscript.
sub isSkippableFile($) {
    my $path = shift;

    return ($path =~ /(^|\/)\.[^\/]*$/
            || $path =~ /\.rpm(save|new)$/
            || $path =~ /\.dpkg-(old|new)$/
	    || $path =~ /\.swp$/
            || $path =~ /\~$/);
}

# isSkippableDir - return true if directory matches something that
# should be skipped (cache directory, symlink directories, etc.)
sub isSkippableDir($) {
    my $path = shift;

    return ($path eq "disable"
            || $path eq "cache"
            || $path eq "force-complain");
}

sub checkIncludeSyntax($) {
    my $errors = shift;

    if (opendir(SDDIR, $profiledir)) {
        my @incdirs = grep { (!/^\./) && (-d "$profiledir/$_") } readdir(SDDIR);
        close(SDDIR);
        while (my $id = shift @incdirs) {
            next if isSkippableDir($id);
            if (opendir(SDDIR, "$profiledir/$id")) {
                for my $path (grep { !/^\./ } readdir(SDDIR)) {
                    chomp($path);
                    next if isSkippableFile($path);
                    if (-f "$profiledir/$id/$path") {
                        my $file = "$id/$path";
                        $file =~ s/$profiledir\///;
                        eval { loadinclude($file); };
                        if ( defined $@ && $@ ne "" ) {
                            push @$errors, $@;
                        }
                    } elsif (-d "$id/$path") {
                        push @incdirs, "$id/$path";
                    }
                }
                closedir(SDDIR);
            }
        }
    }
    return $errors;
}

sub checkProfileSyntax ($) {
    my $errors = shift;

    # Check the syntax of profiles

    opendir(SDDIR, $profiledir)
      or fatal_error "Can't read AppArmor profiles in $profiledir.";
    for my $file (grep { -f "$profiledir/$_" } readdir(SDDIR)) {
        next if isSkippableFile($file);
        my $err = readprofile("$profiledir/$file", \&printMessageErrorHandler, 1);
        if (defined $err and $err ne "") {
            push @$errors, $err;
        }
    }
    closedir(SDDIR);
    return $errors;
}

sub printMessageErrorHandler ($) {
    my $message = shift;
    return $message;
}

sub readprofiles () {
    opendir(SDDIR, $profiledir)
      or fatal_error "Can't read AppArmor profiles in $profiledir.";
    for my $file (grep { -f "$profiledir/$_" } readdir(SDDIR)) {
        next if isSkippableFile($file);
        readprofile("$profiledir/$file", \&fatal_error, 1);
    }
    closedir(SDDIR);
}

sub readinactiveprofiles () {
    return if ( ! -e $extraprofiledir );
    opendir(ESDDIR, $extraprofiledir) or
      fatal_error "Can't read AppArmor profiles in $extraprofiledir.";
    for my $file (grep { -f "$extraprofiledir/$_" } readdir(ESDDIR)) {
        next if $file =~ /\.rpm(save|new)|README$/;
        readprofile("$extraprofiledir/$file", \&fatal_error, 0);
    }
    closedir(ESDDIR);
}

sub readprofile ($$$) {
    my $file          = shift;
    my $error_handler = shift;
    my $active_profile = shift;
    if (open(SDPROF, "$file")) {
        local $/;
        my $data = <SDPROF>;
        close(SDPROF);

        eval {
            my $profile_data = parse_profile_data($data, $file, 0);
            if ($profile_data && $active_profile) {
                attach_profile_data(\%sd, $profile_data);
                attach_profile_data(\%original_sd, $profile_data);
            } elsif ( $profile_data ) {
                attach_profile_data(\%extras,      $profile_data);
            }
        };

        # if there were errors loading the profile, call the error handler
        if ($@) {
            $@ =~ s/\n$//;
            return &$error_handler($@);
        }
    } else {
        $DEBUGGING && debug "readprofile: can't read $file - skipping";
    }
}

sub attach_profile_data($$) {
    my ($profiles, $profile_data) = @_;

    # make deep copies of the profile data so that if we change one set of
    # profile data, we're not changing others because of sharing references
    for my $p ( keys %$profile_data) {
          $profiles->{$p} = dclone($profile_data->{$p});
    }
}

sub parse_profile_data($$$) {
    my ($data, $file, $do_include) = @_;


    my ($profile_data, $profile, $hat, $in_contained_hat, $repo_data,
        @parsed_profiles);
    my $initial_comment = "";

    if ($do_include) {
	$profile = $file;
	$hat = $file;
    }

    for (split(/\n/, $data)) {
        chomp;

        # we don't care about blank lines
        next if /^\s*$/;

        # start of a profile...
        if (m/^\s*(("??\/.+?"??)|(profile\s+("??.+?"??)))\s+((flags=)?\((.+)\)\s+)*\{\s*(#.*)?$/) {
            # if we run into the start of a profile while we're already in a
            # profile, something's wrong...
            if ($profile) {
		unless (($profile eq $hat) and $4) {
		    die "$profile profile in $file contains syntax errors.\n";
		}
	    }

            # we hit the start of a profile, keep track of it...
	    if ($profile && ($profile eq $hat) && $4) {
		# local profile
		$hat = $4;
		$in_contained_hat = 1;
		$profile_data->{$profile}{$hat}{profile} = 1;
	    } else {
		$profile  = $2 || $4;
		# hat is same as profile name if we're not in a hat
		($profile, $hat) = split /\/\//, $profile;
		$in_contained_hat = 0;
		if ($hat) {
		    $profile_data->{$profile}{$hat}{external} = 1;
		}

		$hat ||= $profile;
	    }

            my $flags = $7;

            # deal with whitespace in profile and hat names.
            $profile = strip_quotes($profile);
            $hat     = strip_quotes($hat) if $hat;

	    # save off the name and filename
	    $profile_data->{$profile}{$hat}{name} = $profile;
	    $profile_data->{$profile}{$hat}{filename} = $file;
	    $filelist{$file}{profiles}{$profile}{$hat} = 1;

            # keep track of profile flags
	    $profile_data->{$profile}{$hat}{flags} = $flags;

            $profile_data->{$profile}{$hat}{allow}{netdomain} = { };
            $profile_data->{$profile}{$hat}{allow}{path} = { };

            # store off initial comment if they have one
            $profile_data->{$profile}{$hat}{initial_comment} = $initial_comment
              if $initial_comment;
            $initial_comment = "";

            if ($repo_data) {
                $profile_data->{$profile}{$profile}{repo}{url}  = $repo_data->{url};
                $profile_data->{$profile}{$profile}{repo}{user} = $repo_data->{user};
                $profile_data->{$profile}{$profile}{repo}{id}   = $repo_data->{id};
                $repo_data = undef;
            }

        } elsif (m/^\s*\}\s*(#.*)?$/) { # end of a profile...

            # if we hit the end of a profile when we're not in one, something's
            # wrong...
            if (not $profile) {
                die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
            }

            if ($in_contained_hat) {
                $hat = $profile;
                $in_contained_hat = 0;
            } else {
                push @parsed_profiles, $profile;
                # mark that we're outside of a profile now...
                $profile = undef;
            }

            $initial_comment = "";

        } elsif (m/^\s*(audit\s+)?(deny\s+)?capability(\s+(\S+))?\s*,\s*(#.*)?$/) {  # capability entry
            if (not $profile) {
                die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
            }

	    my $audit = $1 ? 1 : 0;
	    my $allow = $2 ? 'deny' : 'allow';
	    $allow = 'deny' if ($2);
            my $capability = $3 ? $3 : 'all';
            $profile_data->{$profile}{$hat}{$allow}{capability}{$capability}{set} = 1;
            $profile_data->{$profile}{$hat}{$allow}{capability}{$capability}{audit} = $audit;
        } elsif (m/^\s*set capability\s+(\S+)\s*,\s*(#.*)?$/) {  # capability entry
            if (not $profile) {
                die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
            }

            my $capability = $1;
            $profile_data->{$profile}{$hat}{set_capability}{$capability} = 1;

	} elsif (m/^\s*(audit\s+)?(deny\s+)?link\s+(((subset)|(<=))\s+)?([\"\@\/].*?"??)\s+->\s*([\"\@\/].*?"??)\s*,\s*(#.*)?$/) { # for now just keep link
            if (not $profile) {
                die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
            }
	    my $audit = $1 ? 1 : 0;
	    my $allow = $2 ? 'deny' : 'allow';

	    my $subset = $4;
            my $link = strip_quotes($7);
	    my $value = strip_quotes($8);
	    $profile_data->{$profile}{$hat}{$allow}{link}{$link}{to} = $value;
	    $profile_data->{$profile}{$hat}{$allow}{link}{$link}{mode} |= $AA_MAY_LINK;
	    if ($subset) {
		$profile_data->{$profile}{$hat}{$allow}{link}{$link}{mode} |= $AA_LINK_SUBSET;
	    }
	    if ($audit) {
		$profile_data->{$profile}{$hat}{$allow}{link}{$link}{audit} |= $AA_LINK_SUBSET;
	    } else {
		$profile_data->{$profile}{$hat}{$allow}{link}{$link}{audit} |= 0;
	    }

	} elsif (m/^\s*change_profile\s+->\s*("??.+?"??),(#.*)?$/) { # for now just keep change_profile
            if (not $profile) {
                die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
            }
            my $cp = strip_quotes($1);

            $profile_data->{$profile}{$hat}{change_profile}{$cp} = 1;
	} elsif (m/^\s*alias\s+("??.+?"??)\s+->\s*("??.+?"??)\s*,(#.*)?$/) { # never do anything with aliases just keep them
            my $from = strip_quotes($1);
	    my $to = strip_quotes($2);

            if ($profile) {
		$profile_data->{$profile}{$hat}{alias}{$from} = $to;
	    } else {
		unless (exists $filelist{$file}) {
		    $filelist{$file} = { };
		}
		$filelist{$file}{alias}{$from} = $to;
	    }

       } elsif (m/^\s*set\s+rlimit\s+(.+)\s+<=\s*(.+)\s*,(#.*)?$/) { # never do anything with rlimits just keep them
	   if (not $profile) {
	       die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
	   }
	   my $from = $1;
           my $to = $2;

	   $profile_data->{$profile}{$hat}{rlimit}{$from} = $to;

        } elsif (/^\s*(\$\{?[[:alpha:]][[:alnum:]_]*\}?)\s*=\s*(true|false)\s*,?\s*(#.*)?$/i) { # boolean definition
	   if (not $profile) {
	       die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
	   }
	   my $bool_var = $1;
           my $value = $2;

	   $profile_data->{$profile}{$hat}{lvar}{$bool_var} = $value;
        } elsif (/^\s*(@\{?[[:alpha:]][[:alnum:]_]+\}?)\s*\+?=\s*(.+?)\s*,?\s*(#.*)?$/) { # variable additions both += and = doesn't mater
	   my $list_var = strip_quotes($1);
           my $value = strip_quotes($2);

	   if ($profile) {
	       unless (exists $profile_data->{$profile}{$hat}{lvar}) {
		   # create lval hash by sticking an empty list into list_var
		   my @empty = ();
		   $profile_data->{$profile}{$hat}{lvar}{$list_var} = \@empty;
	       }

	       store_list_var($profile_data->{$profile}{$hat}{lvar}, $list_var, $value);
	   } else  {
	       unless (exists $filelist{$file}{lvar}) {
		   # create lval hash by sticking an empty list into list_var
		   my @empty = ();
		   $filelist{$file}{lvar}{$list_var} = \@empty;
	       }

	       store_list_var($filelist{$file}{lvar}, $list_var, $value);
	   }
        } elsif (m/^\s*if\s+(not\s+)?(\$\{?[[:alpha:]][[:alnum:]_]*\}?)\s*\{\s*(#.*)?$/) { # conditional -- boolean
        } elsif (m/^\s*if\s+(not\s+)?defined\s+(@\{?[[:alpha:]][[:alnum:]_]+\}?)\s*\{\s*(#.*)?$/) { # conditional -- variable defined
        } elsif (m/^\s*if\s+(not\s+)?defined\s+(\$\{?[[:alpha:]][[:alnum:]_]+\}?)\s*\{\s*(#.*)?$/) { # conditional -- boolean defined
        } elsif (m/^\s*(audit\s+)?(deny\s+)?(owner\s+)?(file|([\"\@\/].*?)\s+(\S+))(\s+->\s*(.*?))?\s*,\s*(#.*)?$/) {     # path entry
            if (not $profile) {
                die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
            }

	    my $audit = $1 ? 1 : 0;
	    my $allow = $2 ? 'deny' : 'allow';
	    my $user = $3 ? 1 : 0;
            my ($path, $mode, $nt_name) = ($5, $6, $8);
            my $file_keyword = 0;
            my $use_mode = 1;

            if ($4 eq "file") {
                $path = "/{**,}";
                $file_keyword = 1;
                if (!$mode) {
                    # what the parser uses, but we don't care
                    $mode = "rwixlka";
                    $use_mode = 0;
                }
            }

            # strip off any trailing spaces.
            $path =~ s/\s+$//;
            $nt_name =~ s/\s+$// if $nt_name;

            $path = strip_quotes($path);
            $nt_name = strip_quotes($nt_name) if $nt_name;

            # make sure they don't have broken regexps in the profile
            my $p_re = convert_regexp($path);
            eval { "foo" =~ m/^$p_re$/; };
            if ($@) {
                die sprintf(gettext('Profile %s contains invalid regexp %s.'),
                                     $file, $path) . "\n";
            }

            if (!validate_profile_mode($mode, $allow, $nt_name)) {
                fatal_error(sprintf(gettext('Profile %s contains invalid mode %s.'), $file, $mode));
            }

	    $profile_data->{$profile}{$hat}{$allow}{path}{$path}{use_mode} = $use_mode;
	    $profile_data->{$profile}{$hat}{$allow}{path}{$path}{file_keyword} = 1 if $file_keyword;

	    my $tmpmode;
	    if ($user) {
		$tmpmode = str_to_mode("${mode}::");
	    } else {
		$tmpmode = str_to_mode($mode);
	    }

            $profile_data->{$profile}{$hat}{$allow}{path}{$path}{mode} |= $tmpmode;
            $profile_data->{$profile}{$hat}{$allow}{path}{$path}{to} = $nt_name if $nt_name;
	    if ($audit) {
		$profile_data->{$profile}{$hat}{$allow}{path}{$path}{audit} |= $tmpmode;
	    } else {
		$profile_data->{$profile}{$hat}{$allow}{path}{$path}{audit} |= 0;
	    }
        } elsif (m/^\s*#include <(.+)>\s*$/) {     # include stuff
            my $include = $1;

            if ($profile) {
                $profile_data->{$profile}{$hat}{include}{$include} = 1;
            } else {
                unless (exists $filelist{$file}) {
                   $filelist{$file} = { };
                }
                $filelist{$file}{include}{$include} = 1;
            }

            # include is a dir
            if (-d "$profiledir/$include") {
                if (opendir(SDINCDIR, "$profiledir/$include")) {
                    for my $path (readdir(SDINCDIR)) {
                        chomp($path);
                        next if isSkippableFile($path);
                        if (-f "$profiledir/$include/$path") {
                            my $file = "$include/$path";
                            $file =~ s/$profiledir\///;
                            my $ret = eval { loadinclude($file); };
                            if ($@) { die $@; }
                            return $ret if ( $ret != 0 );
                        }
                    }
                }
                closedir(SDINCDIR);
            } else {
                # try to load the include...
                my $ret = eval { loadinclude($include); };
                # propagate errors up the chain
                if ($@) { die $@; }
                return $ret if ( $ret != 0 );
            }
        } elsif (/^\s*(audit\s+)?(deny\s+)?network(.*)/) {
            if (not $profile) {
                die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
            }
	    my $audit = $1 ? 1 : 0;
	    my $allow = $2 ? 'deny' : 'allow';
	    my $network = $3;

            unless ($profile_data->{$profile}{$hat}{$allow}{netdomain}{rule}) {
                $profile_data->{$profile}{$hat}{$allow}{netdomain}{rule} = { };
            }

            if ($network =~ /\s+(\S+)\s+(\S+)\s*,\s*(#.*)?$/ ) {
		my $fam = $1;
		my $type = $2;
                $profile_data->{$profile}{$hat}{$allow}{netdomain}{rule}{$fam}{$type} = 1;
                $profile_data->{$profile}{$hat}{$allow}{netdomain}{audit}{$fam}{$type} = $audit;
            } elsif ( $network =~ /\s+(\S+)\s*,\s*(#.*)?$/ ) {
		my $fam = $1;
                $profile_data->{$profile}{$hat}{$allow}{netdomain}{rule}{$fam} = 1;
		$profile_data->{$profile}{$hat}{$allow}{netdomain}{audit}{$fam} = $audit;
            } else {
                $profile_data->{$profile}{$hat}{$allow}{netdomain}{rule}{all} = 1;
                $profile_data->{$profile}{$hat}{$allow}{netdomain}{audit}{all} = $audit;
            }
        } elsif (/^\s*(tcp_connect|tcp_accept|udp_send|udp_receive)/) {
# just ignore and drop old style network
#	    die sprintf(gettext('%s contains old style network rules.'), $file) . "\n";

        } elsif (m/^\s*\^(\"??.+?\"??)\s*,\s*(#.*)?$/) {
	    if (not $profile) {
		die "$file contains syntax errors.";
	    }
	    # change_hat declaration - needed to change_hat to an external
	    # hat
            $hat = $1;
            $hat = $1 if $hat =~ /^"(.+)"$/;

	    #store we have a declaration if the hat hasn't been seen
	    $profile_data->{$profile}{$hat}{'declared'} = 1
		unless exists($profile_data->{$profile}{$hat}{declared});

        } elsif (m/^\s*\^(\"??.+?\"??)\s+((flags=)?\((.+)\)\s+)*\{\s*(#.*)?$/) {
            # start of embedded hat syntax hat definition
            # read in and mark as changed so that will be written out in the new
            # format

            # if we hit the start of a contained hat when we're not in a profile
            # something is wrong...
            if (not $profile) {
                die sprintf(gettext('%s contains syntax errors.'), $file) . "\n";
            }

            $in_contained_hat = 1;

            # we hit the start of a hat inside the current profile
            $hat = $1;
            my $flags = $4;

            # strip quotes.
            $hat = $1 if $hat =~ /^"(.+)"$/;

            # keep track of profile flags
	    $profile_data->{$profile}{$hat}{flags} = $flags;
	    # we have seen more than a declaration so clear it
	    $profile_data->{$profile}{$hat}{'declared'} = 0;
            $profile_data->{$profile}{$hat}{allow}{path} = { };
            $profile_data->{$profile}{$hat}{allow}{netdomain} = { };

            # store off initial comment if they have one
            $profile_data->{$profile}{$hat}{initial_comment} = $initial_comment
              if $initial_comment;
            $initial_comment = "";
            #don't mark profile as changed just because it has an embedded
	    #hat.
            #$changed{$profile} = 1;

	    $filelist{$file}{profiles}{$profile}{$hat} = 1;

        } elsif (/^\s*\#/) {
            # we only currently handle initial comments
            if (not $profile) {
                # ignore vim syntax highlighting lines
                next if /^\s*\# vim:syntax/;
                # ignore Last Modified: lines
                next if /^\s*\# Last Modified:/;
                if (/^\s*\# REPOSITORY: (\S+) (\S+) (\S+)$/) {
                    $repo_data = { url => $1, user => $2, id => $3 };
                } elsif (/^\s*\# REPOSITORY: NEVERSUBMIT$/) {
                    $repo_data = { neversubmit => 1 };
                } else {
                  $initial_comment .= "$_\n";
                }
            }
        } elsif (/^\s*(audit\s+)?(deny\s+)?(owner\s+)?(capability|dbus|file|mount|pivot_root|remount|umount|signal|unix|ptrace)/) {
	    # ignore valid rules that are currently unsupported by AppArmor.pm
            if (! defined $profile_data->{$profile}{$hat}{unsupported_rules}) {
                $profile_data->{$profile}{$hat}{unsupported_rules} = [];
            }
            $_ =~ s/^\s+|\s+$//g;
            push @{$profile_data->{$profile}{$hat}{unsupported_rules}}, $_ ;

        } else {
	    # we hit something we don't understand in a profile...
	    die sprintf(gettext('%s contains syntax errors. Line [%s]'), $file, $_) . "\n";
        }
    }

    #
    # Cleanup : add required hats if not present in the
    #           parsed profiles
    #
if (not $do_include) {
    for my $hatglob (keys %{$cfg->{required_hats}}) {
        for my $parsed_profile  ( sort @parsed_profiles )  {
            if ($parsed_profile =~ /$hatglob/) {
                for my $hat (split(/\s+/, $cfg->{required_hats}{$hatglob})) {
                    unless ($profile_data->{$parsed_profile}{$hat}) {
                        $profile_data->{$parsed_profile}{$hat} = { };
                    }
                }
            }
        }
    }

}    # if we're still in a profile when we hit the end of the file, it's bad
    if ($profile and not $do_include) {
        die "Reached the end of $file while we were still inside the $profile profile.\n";
    }

    return $profile_data;
}

sub eliminate_duplicates(@) {
    my @data =@_;

    my %set = map { $_ => 1 } @_;
    @data = keys %set;

    return @data;
}

sub separate_vars($) {
    my $vs = shift;
    my @data;

#    while ($vs =~ /\s*(((\"([^\"]|\\\"))+?\")|\S*)\s*(.*)$/) {
    while ($vs =~ /\s*((\".+?\")|([^\"]\S+))\s*(.*)$/) {
	my $tmp = $1;
	push @data, strip_quotes($tmp);
	$vs = $4;
    }

    return @data;
}

sub is_active_profile ($) {
    my $pname = shift;
    if ( $sd{$pname} ) {
        return 1;
    }  else {
        return 0;
    }
}

sub store_list_var (\%$$) {
    my ($vars, $list_var, $value) = @_;

    my @vlist = (separate_vars($value));

#	   if (exists $profile_data->{$profile}{$hat}{lvar}{$list_var}) {
#	       @vlist = (@vlist, @{$profile_data->{$profile}{$hat}{lvar}{$list_var}});
#	   }
#
#	   @vlist = eliminate_duplicates(@vlist);
#	   $profile_data->{$profile}{$hat}{lvar}{$list_var} = \@vlist;

    if (exists $vars->{$list_var}) {
	@vlist = (@vlist, @{$vars->{$list_var}});
    }

    @vlist = eliminate_duplicates(@vlist);
    $vars->{$list_var} = \@vlist;


}

sub strip_quotes ($) {
    my $data = shift;
    $data = $1 if $data =~ /^\"(.*)\"$/;
    return $data;
}

sub quote_if_needed ($) {
    my $data = shift;
    $data = "\"$data\"" if $data =~ /\s/;

    return $data;
}

sub escape ($) {
    my $dangerous = shift;

    $dangerous = strip_quotes($dangerous);

    $dangerous =~ s/((?<!\\))"/$1\\"/g;
    if ($dangerous =~ m/(\s|^$|")/) {
        $dangerous = "\"$dangerous\"";
    }

    return $dangerous;
}

sub writeheader ($$$$$) {
    my ($profile_data, $depth, $name, $embedded_hat, $write_flags) = @_;

    my $pre = '  ' x $depth;
    my @data;
    # deal with whitespace in profile names...
    $name = quote_if_needed($name);

    $name = "profile $name" if ((!$embedded_hat && $name =~ /^[^\/]|^"[^\/]/)
				|| ($embedded_hat && $name =~/^[^^]/));

    #push @data, "#include <tunables/global>" unless ( $is_hat );
    if ($write_flags and  $profile_data->{flags}) {
        push @data, "${pre}$name flags=($profile_data->{flags}) {";
    } else {
        push @data, "${pre}$name {";
    }

    return @data;
}

sub qin_trans ($) {
    my $value = shift;
    return quote_if_needed($value);
}

sub write_single ($$$$$$) {
    my ($profile_data, $depth, $allow, $name, $prefix, $tail) = @_;
    my $ref;
    my @data;

    if ($allow) {
	$ref = $profile_data->{$allow};
	if ($allow eq 'deny') {
	    $allow .= " ";
	} else {
	    $allow = "";
	}
    } else {
	$ref = $profile_data;
	$allow = "";
    }

    my $pre = "  " x $depth;


    # dump out the data
    if (exists $ref->{$name}) {
        for my $key (sort keys %{$ref->{$name}}) {
	    my $qkey = quote_if_needed($key);
	    push @data, "${pre}${allow}${prefix}${qkey}${tail}";
        }
        push @data, "" if keys %{$ref->{$name}};
    }

    return @data;
}

sub write_pair ($$$$$$$$) {
    my ($profile_data, $depth, $allow, $name, $prefix, $sep, $tail, $fn) = @_;
    my $ref;
    my @data;

    if ($allow) {
	$ref = $profile_data->{$allow};
	if ($allow eq 'deny') {
	    $allow .= " ";
	} else {
	    $allow = "";
	}
    } else {
	$ref = $profile_data;
	$allow = "";
    }

    my $pre = "  " x $depth;

    # dump out the data
    if (exists $ref->{$name}) {
        for my $key (sort keys %{$ref->{$name}}) {
	    my $value = &{$fn}($ref->{$name}{$key});
            push @data, "${pre}${allow}${prefix}${key}${sep}${value}${tail}";
        }
        push @data, "" if keys %{$ref->{$name}};
    }

    return @data;
}

sub writeincludes ($$) {
    my ($prof_data, $depth) = @_;

    return write_single($prof_data, $depth,'', 'include', "#include <", ">");
}

sub writechange_profile ($$) {
    my ($prof_data, $depth) = @_;

    return write_single($prof_data, $depth, '', 'change_profile', "change_profile -> ", ",");
}

sub writealiases ($$) {
    my ($prof_data, $depth) = @_;

    return write_pair($prof_data, $depth, '', 'alias', "alias ", " -> ", ",", \&qin_trans);
}

sub writerlimits ($$) {
    my ($prof_data, $depth) = @_;

    return write_pair($prof_data, $depth, '', 'rlimit', "set rlimit ", " <= ", ",", \&qin_trans);
}

# take a list references and process it
sub var_transform($) {
    my $ref = shift;
    my @in = @{$ref};
    my @data;

    foreach my $value (@in) {
	push @data, quote_if_needed($value);
    }

    return join " ", @data;
}

sub writelistvars ($$) {
    my ($prof_data, $depth) = @_;

    return write_pair($prof_data, $depth, '', 'lvar', "", " = ", "", \&var_transform);
}

sub writecap_rules ($$$) {
    my ($profile_data, $depth, $allow) = @_;

    my $allowstr = $allow eq 'deny' ? 'deny ' : '';
    my $pre = "  " x $depth;

    my @data;
    if (exists $profile_data->{$allow}{capability}) {
	my $audit;
	if (exists $profile_data->{$allow}{capability}{all}) {
	    $audit = ($profile_data->{$allow}{capability}{all}{audit}) ? 'audit ' : '';
	    push @data, "${pre}${audit}${allowstr}capability,";
	}
	for my $cap (sort keys %{$profile_data->{$allow}{capability}}) {
	    next if ($cap eq "all");
	    my $audit = ($profile_data->{$allow}{capability}{$cap}{audit}) ? 'audit ' : '';
	    if ($profile_data->{$allow}{capability}{$cap}{set}) {
		push @data, "${pre}${audit}${allowstr}capability ${cap},";
	    }
        }
	push @data, "";
    }

    return @data;
}

sub writecapabilities ($$) {
    my ($prof_data, $depth) = @_;
    my @data;
    push @data, write_single($prof_data, $depth, '', 'set_capability', "set capability ", ",");
    push @data, writecap_rules($prof_data, $depth, 'deny');
    push @data, writecap_rules($prof_data, $depth, 'allow');
    return @data;
}

sub writenet_rules ($$$) {
    my ($profile_data, $depth, $allow) = @_;

    my $allowstr = $allow eq 'deny' ? 'deny ' : '';

    my $pre = "  " x $depth;
    my $audit = "";

    my @data;
    # dump out the netdomain entries...
    if (exists $profile_data->{$allow}{netdomain}) {
        if ( $profile_data->{$allow}{netdomain}{rule} &&
             $profile_data->{$allow}{netdomain}{rule}{all}) {
	    $audit = "audit " if $profile_data->{$allow}{netdomain}{audit}{all};
            push @data, "${pre}${audit}network,";
        } else {
            for my $fam (sort keys %{$profile_data->{$allow}{netdomain}{rule}}) {
                if ( $profile_data->{$allow}{netdomain}{rule}{$fam} == 1 ) {
		    $audit = "audit " if $profile_data->{$allow}{netdomain}{audit}{$fam};
                    push @data, "${pre}${audit}${allowstr}network $fam,";
                } else {
                    for my $type 
                        (sort keys %{$profile_data->{$allow}{netdomain}{rule}{$fam}}) {
			    $audit = "audit " if $profile_data->{$allow}{netdomain}{audit}{$fam}{$type};
			    push @data, "${pre}${audit}${allowstr}network $fam $type,";
                    }
                }
            }
        }
        push @data, "" if %{$profile_data->{$allow}{netdomain}};
    }
    return @data;

}

sub writenetdomain ($$) {
    my ($prof_data, $depth) = @_;
    my @data;

    push @data, writenet_rules($prof_data, $depth, 'deny');
    push @data, writenet_rules($prof_data, $depth, 'allow');

    return @data;
}

sub writelink_rules ($$$) {
    my ($profile_data, $depth, $allow) = @_;

    my $allowstr = $allow eq 'deny' ? 'deny ' : '';
    my $pre = "  " x $depth;

    my @data;
    if (exists $profile_data->{$allow}{link}) {
        for my $path (sort keys %{$profile_data->{$allow}{link}}) {
            my $to = $profile_data->{$allow}{link}{$path}{to};
	    my $subset = ($profile_data->{$allow}{link}{$path}{mode} & $AA_LINK_SUBSET) ? 'subset ' : '';
	    my $audit = ($profile_data->{$allow}{link}{$path}{audit}) ? 'audit ' : '';
            # deal with whitespace in path names
            $path = quote_if_needed($path);
	    $to = quote_if_needed($to);
	    push @data, "${pre}${audit}${allowstr}link ${subset}${path} -> ${to},";
        }
	push @data, "";
    }

    return @data;
}

sub writelinks ($$) {
    my ($profile_data, $depth) = @_;
    my @data;

    push @data, writelink_rules($profile_data, $depth, 'deny');
    push @data, writelink_rules($profile_data, $depth, 'allow');

    return @data;
}

sub writepath_rules ($$$) {
    my ($profile_data, $depth, $allow) = @_;

    my $allowstr = $allow eq 'deny' ? 'deny ' : '';
    my $pre = "  " x $depth;

    my @data;
    if (exists $profile_data->{$allow}{path}) {
        for my $path (sort keys %{$profile_data->{$allow}{path}}) {
            my $mode = $profile_data->{$allow}{path}{$path}{mode};
            my $audit = $profile_data->{$allow}{path}{$path}{audit};
	    my $tail = "";
	    $tail = " -> " . $profile_data->{$allow}{path}{$path}{to} if ($profile_data->{$allow}{path}{$path}{to});
	    my ($user, $other) = split_mode($mode);
	    my ($user_audit, $other_audit) = split_mode($audit);
	    # determine whether the rule contains any owner only components

	    while ($user || $other) {
		my $ownerstr = "";
		my ($tmpmode, $tmpaudit) = 0;
		if ($user & ~$other) {
		    # user contains bits not set in other
		    $ownerstr = "owner ";
		    $tmpmode = $user & ~$other;
		    $tmpaudit = $user_audit;
		    $user &= ~$tmpmode;
#		} elsif ($other & ~$user) {
#		    $ownerstr = "other ";
#		    $tmpmode = $other & ~$user;
#		    $tmpaudit = $other_audit;
#		    $other &= ~$tmpmode;
		} else {
		    if ($user_audit & ~$other_audit & $user) {
			$ownerstr = "owner ";
			$tmpaudit = $user_audit & ~$other_audit & $user;
			$tmpmode = $user & $tmpaudit;
			$user &= ~$tmpmode;
#		    } elsif ($other_audit & ~$user_audit & $other) {
#			$ownerstr = "other ";
#			$tmpaudit = $other_audit & ~$user_audit & $other;
#			$tmpmode = $other & $tmpaudit;
#			$other &= ~$tmpmode;
		    } else {
			# user == other && user_audit == other_audit
			$ownerstr = "";
#include exclusive other for now
#			$tmpmode = $user;
#			$tmpaudit = $user_audit;
			$tmpmode = $user | $other;
			$tmpaudit = $user_audit | $other_audit;
			$user &= ~$tmpmode;
			$other &= ~$tmpmode;
		    }
		}

		if ($tmpmode & $tmpaudit) {
		    my $modestr = mode_to_str($tmpmode & $tmpaudit);
		    if ($path =~ /\s/) {
			push @data, "${pre}audit ${allowstr}${ownerstr}\"$path\" ${modestr}${tail},";
		    } else {
			push @data, "${pre}audit ${allowstr}${ownerstr}$path ${modestr}${tail},";
		    }
		    $tmpmode &= ~$tmpaudit;
		}
		my $kw = $profile_data->{$allow}{path}{$path}{file_keyword};
		my $use_mode = $profile_data->{$allow}{path}{$path}{use_mode};
		if ($kw) {
		    my $modestr = "";
		    $modestr = " " . mode_to_str($tmpmode) if $use_mode;
		    push @data, "${pre}${allowstr}${ownerstr}file${modestr}${tail},";
		} elsif ($tmpmode) {
		    my $modestr = mode_to_str($tmpmode);
		    if ($path =~ /\s/) {
			push @data, "${pre}${allowstr}${ownerstr}\"$path\" ${modestr}${tail},";
		    } else {
			push @data, "${pre}${allowstr}${ownerstr}$path ${modestr}${tail},";
		    }
		}
	    }

        }
	push @data, "";
    }

    return @data;
}

sub writepaths ($$) {
    my ($prof_data, $depth) = @_;

    my @data;
    push @data, writepath_rules($prof_data, $depth, 'deny');
    push @data, writepath_rules($prof_data, $depth, 'allow');

    return @data;
}

sub writeunsupportedrules ($$) {
    my ($prof_data, $depth) = @_;

    my @data;
    my $pre = "  " x $depth;

    if (defined $prof_data->{unsupported_rules}) {

        for my $rule (@{$prof_data->{unsupported_rules}}){
            push @data, "${pre}${rule}";
        }

        push @data, "";
    }

    return @data;

}

sub write_rules ($$) {
    my ($prof_data, $depth) = @_;

    my @data;
    push @data, writealiases($prof_data, $depth);
    push @data, writelistvars($prof_data, $depth);
    push @data, writeincludes($prof_data, $depth);
    push @data, writerlimits($prof_data, $depth);
    push @data, writecapabilities($prof_data, $depth);
    push @data, writenetdomain($prof_data, $depth);
    push @data, writeunsupportedrules($prof_data, $depth); ## Legacy support for unknown/new rules
    push @data, writelinks($prof_data, $depth);
    push @data, writepaths($prof_data, $depth);
    push @data, writechange_profile($prof_data, $depth);

    return @data;
}

sub writepiece ($$$$$);
sub writepiece ($$$$$) {
    my ($profile_data, $depth, $name, $nhat, $write_flags) = @_;

    my $pre = '  ' x $depth;
    my @data;
    my $wname;
    my $inhat = 0;
    if ($name eq $nhat) {
	$wname = $name;
    } else {
	$wname = "$name//$nhat";
	$name = $nhat;
	$inhat = 1;
    }
    push @data, writeheader($profile_data->{$name}, $depth, $wname, 0, $write_flags);
    push @data, write_rules($profile_data->{$name}, $depth + 1);

    my $pre2 = '  ' x ($depth + 1);
    # write external hat declarations
    for my $hat (grep { $_ ne $name } sort keys %{$profile_data}) {
	if ($profile_data->{$hat}{declared}) {
	    push @data, "${pre2}^$hat,";
	}
    }

    if (!$inhat) {
	# write embedded hats
	for my $hat (grep { $_ ne $name } sort keys %{$profile_data}) {
	    if ((not $profile_data->{$hat}{external}) and
		(not $profile_data->{$hat}{declared})) {
		push @data, "";
		if ($profile_data->{$hat}{profile}) {
		    push @data, map { "$_" } writeheader($profile_data->{$hat},
							 $depth + 1, $hat,
							 1, $write_flags);
		} else {
		    push @data, map { "$_" } writeheader($profile_data->{$hat},
							 $depth + 1, "^$hat",
							 1, $write_flags);
		}
		push @data, map { "$_" } write_rules($profile_data->{$hat},
						     $depth + 2);
		push @data, "${pre2}}";
	    }
	}
	push @data, "${pre}}";

	#write external hats
	for my $hat (grep { $_ ne $name } sort keys %{$profile_data}) {
	    if (($name eq $nhat) and $profile_data->{$hat}{external}) {
		push @data, "";
		push @data, map { "  $_" } writepiece($profile_data, $depth - 1,
						      $name, $hat, $write_flags);
		push @data, "  }";
	    }
	}
    }
    return @data;
}

sub serialize_profile($$$) {
    my ($profile_data, $name, $options) = @_;

    my $string = "";
    my $include_metadata = 0;  # By default don't write out metadata
    my $include_flags = 1;
    if ( $options and ref($options) eq "HASH" ) {
       $include_metadata = 1 if ( defined $options->{METADATA} );
       $include_flags    = 0 if ( defined $options->{NO_FLAGS} );
    }

    if ($include_metadata) {
        # keep track of when the file was last updated
        $string .= "# Last Modified: " . localtime(time) . "\n";

        # print out repository metadata
        if ($profile_data->{$name}{repo}       &&
            $profile_data->{$name}{repo}{url}  &&
            $profile_data->{$name}{repo}{user} &&
            $profile_data->{$name}{repo}{id}) {
            my $repo = $profile_data->{$name}{repo};
            $string .= "# REPOSITORY: $repo->{url} $repo->{user} $repo->{id}\n";
        } elsif ($profile_data->{$name}{repo}{neversubmit}) {
            $string .= "# REPOSITORY: NEVERSUBMIT\n";
        }
    }

    # print out initial comment
    if ($profile_data->{$name}{initial_comment}) {
        my $comment = $profile_data->{$name}{initial_comment};
        $comment =~ s/\\n/\n/g;
        $string .= "$comment\n";
    }

    #bleah this is stupid the data structure needs to be reworked
    my $filename = getprofilefilename($name);
    my @data;
    if ($filelist{$filename}) {
	push @data, writealiases($filelist{$filename}, 0);
	push @data, writelistvars($filelist{$filename}, 0);
	push @data, writeincludes($filelist{$filename}, 0);
    }


# XXX - FIXME
#
#  # dump variables defined in this file
#  if ($variables{$filename}) {
#    for my $var (sort keys %{$variables{$filename}}) {
#      if ($var =~ m/^@/) {
#        my @values = sort @{$variables{$filename}{$var}};
#        @values = map { escape($_) } @values;
#        my $values = join (" ", @values);
#        print SDPROF "$var = ";
#        print SDPROF $values;
#      } elsif ($var =~ m/^\$/) {
#        print SDPROF "$var = ";
#        print SDPROF ${$variables{$filename}{$var}};
#      } elsif ($var =~ m/^\#/) {
#        my $inc = $var;
#        $inc =~ s/^\#//;
#        print SDPROF "#include <$inc>";
#      }
#      print SDPROF "\n";
#    }
#  }

    push @data, writepiece($profile_data, 0, $name, $name, $include_flags);
    $string .= join("\n", @data);

    return "$string\n";
}

sub writeprofile_ui_feedback ($) {
    my $profile = shift;
    UI_Info(sprintf(gettext('Writing updated profile for %s.'), $profile));
    writeprofile($profile);
}

sub writeprofile ($) {
    my ($profile) = shift;

    my $filename = $sd{$profile}{$profile}{filename} || getprofilefilename($profile);

    open(SDPROF, ">$filename") or
      fatal_error "Can't write new AppArmor profile $filename: $!";
    my $serialize_opts = { };
    $serialize_opts->{METADATA} = 1;

    #make sure to write out all the profiles in the file
    my $profile_string = serialize_profile($sd{$profile}, $profile, $serialize_opts);
    print SDPROF $profile_string;
    close(SDPROF);

    # mark the profile as up-to-date
    delete $changed{$profile};
    $original_sd{$profile} = dclone($sd{$profile});
}

sub getprofileflags($) {
    my $filename = shift;

    my $flags = "enforce";

    if (open(PROFILE, "$filename")) {
        while (<PROFILE>) {
            if (m/^\s*\/\S+\s+flags=\((.+)\)\s+{\s*$/) {
                $flags = $1;
                close(PROFILE);
                return $flags;
            }
        }
        close(PROFILE);
    }

    return $flags;
}


sub matchliteral($$) {
    my ($sd_regexp, $literal) = @_;

    my $p_regexp = convert_regexp($sd_regexp);

    # check the log entry against our converted regexp...
    my $matches = eval { $literal =~ /^$p_regexp$/; };

    # doesn't match if we've got a broken regexp
    return undef if $@;

    return $matches;
}

# test if profile has exec rule for $exec_target
sub profile_known_exec (\%$$) {
    my ($profile, $type, $exec_target) = @_;
    if ( $type eq "exec" ) {
        my ($cm, $am, @m);

        # test denies first
        ($cm, $am, @m) = rematchfrag($profile, 'deny', $exec_target);
	if ($cm & $AA_MAY_EXEC) {
	    return -1;
	}
        ($cm, $am, @m) = match_prof_incs_to_path($profile, 'deny', $exec_target);
	if ($cm & $AA_MAY_EXEC) {
	    return -1;
	}

	# now test the generally longer allow lists
        ($cm, $am, @m) = rematchfrag($profile, 'allow', $exec_target);
	if ($cm & $AA_MAY_EXEC) {
	    return 1;
	}

        ($cm, $am, @m) = match_prof_incs_to_path($profile, 'allow', $exec_target);
	if ($cm & $AA_MAY_EXEC) {
	    return 1;
	}
    }
    return 0;
}

sub profile_known_capability (\%$) {
    my ($profile, $capname) = @_;

    return -1 if $profile->{deny}{capability}{$capname}{set};
    return 1 if $profile->{allow}{capability}{$capname}{set};
    for my $incname ( keys %{$profile->{include}} ) {
	return -1 if $include{$incname}{$incname}{deny}{capability}{$capname}{set};
	return 1 if $include{$incname}{$incname}{allow}{capability}{$capname}{set};
    }
    return 0;
}

sub profile_known_network (\%$$) {
    my ($profile, $family, $sock_type) = @_;

    return -1 if netrules_access_check( $profile->{deny}{netdomain},
                                       $family, $sock_type);
    return 1 if netrules_access_check( $profile->{allow}{netdomain},
                                       $family, $sock_type);

    for my $incname ( keys %{$profile->{include}} ) {
        return -1 if netrules_access_check($include{$incname}{$incname}{deny}{netdomain},
                                        $family, $sock_type);
        return 1 if netrules_access_check($include{$incname}{$incname}{allow}{netdomain},
					  $family, $sock_type);
    }

    return 0;
}

sub netrules_access_check ($$$) {
    my ($netrules, $family, $sock_type) = @_;
    return 0 if ( not defined $netrules );
    my %netrules        = %$netrules;
    my $all_net         = defined $netrules{rule}{all};
    my $all_net_family  = defined $netrules{rule}{$family} && $netrules{rule}{$family} == 1;
    my $net_family_sock = defined $netrules{rule}{$family} &&
                          ref($netrules{rule}{$family}) eq "HASH" &&
                          defined $netrules{rule}{$family}{$sock_type};

    if ( $all_net || $all_net_family || $net_family_sock ) {
        return 1;
    } else {
      return 0;
    }
}

sub reload_base($) {
    my $bin = shift;

    # don't try to reload profile if AppArmor is not running
    return unless check_for_subdomain();

    my $filename = getprofilefilename($bin);

    system("/bin/cat '$filename' | $parser -I$profiledir -r >/dev/null 2>&1");
}

sub reload ($) {
    my $bin = shift;

    # don't reload the profile if the corresponding executable doesn't exist
    my $fqdbin = findexecutable($bin) or return;

    return reload_base($fqdbin);
}

sub read_include_from_file($) {
    my $which = shift;

    my $data;
    if (open(INCLUDE, "$profiledir/$which")) {
        local $/;
        $data = <INCLUDE>;
        close(INCLUDE);
    }

    return $data;
}

sub get_include_data($) {
    my $which = shift;

    my $data = read_include_from_file($which);
    unless($data) {
        fatal_error "Can't find include file $which: $!";
    }
    return $data;
}

sub loadinclude($) {
    my $which = shift;

    # don't bother loading it again if we already have
    return 0 if $include{$which}{$which};

    my @loadincludes = ($which);
    while (my $incfile = shift @loadincludes) {

        my $data = get_include_data($incfile);
	my $incdata = parse_profile_data($data, $incfile, 1);
	if ($incdata) {
                    attach_profile_data(\%include, $incdata);
	}
    }
    return 0;
}

sub rematchfrag ($$$) {
    my ($frag, $allow, $path) = @_;

    my $combinedmode = 0;
    my $combinedaudit = 0;
    my @matches;

    for my $entry (keys %{ $frag->{$allow}{path} }) {

        my $regexp = convert_regexp($entry);

        # check the log entry against our converted regexp...
        if ($path =~ /^$regexp$/) {

            # regexp matches, add it's mode to the list to check against
            $combinedmode |= $frag->{$allow}{path}{$entry}{mode};
            $combinedaudit |= $frag->{$allow}{path}{$entry}{audit};
            push @matches, $entry;
        }
    }

    return wantarray ? ($combinedmode, $combinedaudit, @matches) : $combinedmode;
}

sub match_include_to_path ($$$) {
    my ($incname, $allow, $path) = @_;

    my $combinedmode = 0;
    my $combinedaudit = 0;
    my @matches;

    my @includelist = ( $incname );
    while (my $incfile = shift @includelist) {
        my $ret = eval { loadinclude($incfile); };
        if ($@) { fatal_error $@; }
        my ($cm, $am, @m) = rematchfrag($include{$incfile}{$incfile}, $allow, $path);
        if ($cm) {
            $combinedmode |= $cm;
	    $combinedaudit |= $am;
            push @matches, @m;
        }

        # check if a literal version is in the current include fragment
        if ($include{$incfile}{$incfile}{$allow}{path}{$path}) {
            $combinedmode |= $include{$incfile}{$incfile}{$allow}{path}{$path}{mode};
            $combinedaudit |= $include{$incfile}{$incfile}{$allow}{path}{$path}{audit};
        }

        # if this fragment includes others, check them too
        if (keys %{ $include{$incfile}{$incfile}{include} }) {
            push @includelist, keys %{ $include{$incfile}{$incfile}{include} };
        }
    }

    return wantarray ? ($combinedmode, $combinedaudit, @matches) : $combinedmode;
}

sub match_prof_incs_to_path ($$$) {
    my ($frag, $allow, $path) = @_;

    my $combinedmode = 0;
    my $combinedaudit = 0;
    my @matches;

    # scan the include fragments for this profile looking for matches
    my @includelist = keys %{ $frag->{include} };
    while (my $include = shift @includelist) {
	my ($cm, $am, @m) = match_include_to_path($include, $allow, $path);
        if ($cm) {
            $combinedmode |= $cm;
            $combinedaudit |= $am;
            push @matches, @m;
        }
    }

    return wantarray ? ($combinedmode, $combinedaudit, @matches) : $combinedmode;
}

#find includes that match the path to suggest
sub suggest_incs_for_path($$$) {
    my ($incname, $path, $allow) = @_;


    my $combinedmode = 0;
    my $combinedaudit = 0;
    my @matches;

    # scan the include fragments looking for matches
    my @includelist = ($incname);
    while (my $include = shift @includelist) {
        my ($cm, $am, @m) = rematchfrag($include{$include}{$include}, 'allow', $path);
        if ($cm) {
            $combinedmode |= $cm;
            $combinedaudit |= $am;
            push @matches, @m;
        }

        # check if a literal version is in the current include fragment
        if ($include{$include}{$include}{allow}{path}{$path}) {
            $combinedmode |= $include{$include}{$include}{allow}{path}{$path}{mode};
            $combinedaudit |= $include{$include}{$include}{allow}{path}{$path}{audit};
        }

        # if this fragment includes others, check them too
        if (keys %{ $include{$include}{$include}{include} }) {
            push @includelist, keys %{ $include{$include}{$include}{include} };
        }
    }

    if ($combinedmode) {
        return wantarray ? ($combinedmode, $combinedaudit, @matches) : $combinedmode;
    } else {
        return;
    }
}

sub check_qualifiers($) {
    my $program = shift;

    if ($cfg->{qualifiers}{$program}) {
        unless($cfg->{qualifiers}{$program} =~ /p/) {
            fatal_error(sprintf(gettext("\%s is currently marked as a program that should not have it's own profile.  Usually, programs are marked this way if creating a profile for them is likely to break the rest of the system.  If you know what you're doing and are certain you want to create a profile for this program, edit the corresponding entry in the [qualifiers] section in /etc/apparmor/logprof.conf."), $program));
        }
    }
}

sub loadincludes() {
    if (opendir(SDDIR, $profiledir)) {
        my @incdirs = grep { (!/^\./) && (-d "$profiledir/$_") } readdir(SDDIR);
        close(SDDIR);

        while (my $id = shift @incdirs) {
            next if isSkippableDir($id);
            if (opendir(SDDIR, "$profiledir/$id")) {
                for my $path (readdir(SDDIR)) {
                    chomp($path);
                    next if isSkippableFile($path);
                    if (-f "$profiledir/$id/$path") {
                        my $file = "$id/$path";
                        $file =~ s/$profiledir\///;
                        my $ret = eval { loadinclude($file); };
                        if ($@) { fatal_error $@; }
                    } elsif (-d "$id/$path") {
                        push @incdirs, "$id/$path";
                    }
                }
                closedir(SDDIR);
            }
        }
    }
}

sub globcommon ($) {
    my $path = shift;

    my @globs;

    # glob library versions in both foo-5.6.so and baz.so.9.2 form
    if ($path =~ m/[\d\.]+\.so$/ || $path =~ m/\.so\.[\d\.]+$/) {
        my $libpath = $path;
        $libpath =~ s/[\d\.]+\.so$/*.so/;
        $libpath =~ s/\.so\.[\d\.]+$/.so.*/;
        push @globs, $libpath if $libpath ne $path;
    }

    for my $glob (keys %{$cfg->{globs}}) {
        if ($path =~ /$glob/) {
            my $globbedpath = $path;
            $globbedpath =~ s/$glob/$cfg->{globs}{$glob}/g;
            push @globs, $globbedpath if $globbedpath ne $path;
        }
    }

    if (wantarray) {
        return sort { length($b) <=> length($a) } uniq(@globs);
    } else {
        my @list = sort { length($b) <=> length($a) } uniq(@globs);
        return $list[$#list];
    }
}

# this is an ugly, nasty function that attempts to see if one regexp
# is a subset of another regexp
sub matchregexp ($$) {
    my ($new, $old) = @_;

    # bail out if old pattern has {foo,bar,baz} stuff in it
    return undef if $old =~ /\{.*(\,.*)*\}/;

    # are there any regexps at all in the old pattern?
    if ($old =~ /\[.+\]/ or $old =~ /\*/ or $old =~ /\?/) {

        # convert {foo,baz} to (foo|baz)
        $new =~ y/\{\}\,/\(\)\|/ if $new =~ /\{.*\,.*\}/;

        # \001 == SD_GLOB_RECURSIVE
        # \002 == SD_GLOB_SIBLING

        $new =~ s/\*\*/\001/g;
        $new =~ s/\*/\002/g;

        $old =~ s/\*\*/\001/g;
        $old =~ s/\*/\002/g;

        # strip common prefix
        my $prefix = commonprefix($new, $old);
        if ($prefix) {

            # make sure we don't accidentally gobble up a trailing * or **
            $prefix =~ s/(\001|\002)$//;
            $new    =~ s/^$prefix//;
            $old    =~ s/^$prefix//;
        }

        # strip common suffix
        my $suffix = commonsuffix($new, $old);
        if ($suffix) {

            # make sure we don't accidentally gobble up a leading * or **
            $suffix =~ s/^(\001|\002)//;
            $new    =~ s/$suffix$//;
            $old    =~ s/$suffix$//;
        }

        # if we boiled the differences down to a ** in the new entry, it matches
        # whatever's in the old entry
        return 1 if $new eq "\001";

        # if we've paired things down to a * in new, old matches if there are no
        # slashes left in the path
        return 1 if ($new eq "\002" && $old =~ /^[^\/]+$/);

        # we'll bail out if we have more globs in the old version
        return undef if $old =~ /\001|\002/;

        # see if we can match * globs in new against literal elements in old
        $new =~ s/\002/[^\/]*/g;

        return 1 if $old =~ /^$new$/;

    } else {

        my $new_regexp = convert_regexp($new);

        # check the log entry against our converted regexp...
        return 1 if $old =~ /^$new_regexp$/;

    }

    return undef;
}

sub combine_name($$) { return ($_[0] eq $_[1]) ? $_[0] : "$_[0]^$_[1]"; }
sub split_name ($) { my ($p, $h) = split(/\^/, $_[0]); $h ||= $p; ($p, $h); }

##########################
#
# prompt_user($headers, $functions, $default, $options, $selected);
#
# $headers:
#   a required arrayref made up of "key, value" pairs in the order you'd
#   like them displayed to user
#
# $functions:
#   a required arrayref of the different options to display at the bottom
#   of the prompt like "(A)llow", "(D)eny", and "Ba(c)on".  the character
#   contained by ( and ) will be used as the key to select the specified
#   option.
#
# $default:
#   a required character which is the default "key" to enter when they
#   just hit enter
#
# $options:
#   an optional arrayref of the choices like the glob suggestions to be
#   presented to the user
#
# $selected:
#   specifies which option is currently selected
#
# when prompt_user() is called without an $options list, it returns a
# single value which is the key for the specified "function".
#
# when prompt_user() is called with an $options list, it returns an array
# of two elements, the key for the specified function as well as which
# option was currently selected
#######################################################################

sub Text_PromptUser ($) {
    my $question = shift;

    my $title     = $question->{title};
    my $explanation = $question->{explanation};

    my @headers   = (@{ $question->{headers} });
    my @functions = (@{ $question->{functions} });

    my $default  = $question->{default};
    my $options  = $question->{options};
    my $selected = $question->{selected} || 0;

    my $helptext = $question->{helptext};

    push @functions, "CMD_HELP" if $helptext;

    my %keys;
    my @menu_items;
    for my $cmd (@functions) {

        # make sure we know about this particular command
        my $cmdmsg = "PromptUser: " . gettext("Unknown command") . " $cmd";
        fatal_error $cmdmsg unless $CMDS{$cmd};

        # grab the localized text to use for the menu for this command
        my $menutext = gettext($CMDS{$cmd});

        # figure out what the hotkey for this menu item is
        my $menumsg = "PromptUser: " .
                      gettext("Invalid hotkey in") .
                      " '$menutext'";
        $menutext =~ /\((\S)\)/ or fatal_error $menumsg;

        # we want case insensitive comparisons so we'll force things to
        # lowercase
        my $key = lc($1);

        # check if we're already using this hotkey for this prompt
        my $hotkeymsg = "PromptUser: " .
                        gettext("Duplicate hotkey for") .
                        " $cmd: $menutext";
        fatal_error $hotkeymsg if $keys{$key};

        # keep track of which command they're picking if they hit this hotkey
        $keys{$key} = $cmd;

        if ($default && $default eq $cmd) {
            $menutext = "[$menutext]";
        }

        push @menu_items, $menutext;
    }

    # figure out the key for the default option
    my $default_key;
    if ($default && $CMDS{$default}) {
        my $defaulttext = gettext($CMDS{$default});

        # figure out what the hotkey for this menu item is
        my $defmsg = "PromptUser: " .
                      gettext("Invalid hotkey in default item") .
                      " '$defaulttext'";
        $defaulttext =~ /\((\S)\)/ or fatal_error $defmsg;

        # we want case insensitive comparisons so we'll force things to
        # lowercase
        $default_key = lc($1);

        my $defkeymsg = "PromptUser: " .
                        gettext("Invalid default") .
                        " $default";
        fatal_error $defkeymsg unless $keys{$default_key};
    }

    my $widest = 0;
    my @poo    = @headers;
    while (my $header = shift @poo) {
        my $value = shift @poo;
        $widest = length($header) if length($header) > $widest;
    }
    $widest++;

    my $format = '%-' . $widest . "s \%s\n";

    my $function_regexp = '^(';
    $function_regexp .= join("|", keys %keys);
    $function_regexp .= '|\d' if $options;
    $function_regexp .= ')$';

    my $ans = "XXXINVALIDXXX";
    while ($ans !~ /$function_regexp/i) {
        # build up the prompt...
        my $prompt = "\n";

        $prompt .= "= $title =\n\n" if $title;

        if (@headers) {
            my @poo = @headers;
            while (my $header = shift @poo) {
                my $value = shift @poo;
                $prompt .= sprintf($format, "$header:", $value);
            }
            $prompt .= "\n";
        }

        if ($explanation) {
            $prompt .= "$explanation\n\n";
        }

        if ($options) {
            for (my $i = 0; $options->[$i]; $i++) {
                my $f = ($selected == $i) ? ' [%d - %s]' : '  %d - %s ';
                $prompt .= sprintf("$f\n", $i + 1, $options->[$i]);
            }
            $prompt .= "\n";
        }
        $prompt .= join(" / ", @menu_items);
        print "$prompt\n";

        # get their input...
        $ans = lc(getkey());

        if ($ans) {
            # handle escape sequences so you can up/down in the list
            if ($ans eq "up") {

                if ($options && ($selected > 0)) {
                    $selected--;
                }
                $ans = "XXXINVALIDXXX";

            } elsif ($ans eq "down") {

                if ($options && ($selected < (scalar(@$options) - 1))) {
                    $selected++;
                }
                $ans = "XXXINVALIDXXX";

            } elsif ($keys{$ans} && $keys{$ans} eq "CMD_HELP") {

                print "\n$helptext\n";
                $ans = "XXXINVALIDXXX";

            } elsif (ord($ans) == 10) {

                # pick the default if they hit return...
                $ans = $default_key;

            } elsif ($options && ($ans =~ /^\d$/)) {

                # handle option poo
                if ($ans > 0 && $ans <= scalar(@$options)) {
                    $selected = $ans - 1;
                }
                $ans = "XXXINVALIDXXX";
            }
        }

        if ($keys{$ans} && $keys{$ans} eq "CMD_HELP") {
            print "\n$helptext\n";
            $ans = "again";
        }
    }

    # pull our command back from our hotkey map
    $ans = $keys{$ans} if $keys{$ans};
    return ($ans, $selected);

}

# Parse event record into key-value pairs
sub parse_event($) {
    my %ev = ();
    my $msg = shift;
    chomp($msg);
    my $event = LibAppArmor::parse_record($msg);
    my ($rmask, $dmask);

    $DEBUGGING && debug("parse_event: $msg");

    $ev{'resource'}   = LibAppArmor::aa_log_record::swig_info_get($event);
    $ev{'active_hat'} = LibAppArmor::aa_log_record::swig_active_hat_get($event);
    $ev{'sdmode'}     = LibAppArmor::aa_log_record::swig_event_get($event);
    $ev{'time'}       = LibAppArmor::aa_log_record::swig_epoch_get($event);
    $ev{'operation'}  = LibAppArmor::aa_log_record::swig_operation_get($event);
    $ev{'profile'}    = LibAppArmor::aa_log_record::swig_profile_get($event);
    $ev{'name'}       = LibAppArmor::aa_log_record::swig_name_get($event);
    $ev{'name2'}      = LibAppArmor::aa_log_record::swig_name2_get($event);
    $ev{'attr'}       = LibAppArmor::aa_log_record::swig_attribute_get($event);
    $ev{'parent'}     = LibAppArmor::aa_log_record::swig_parent_get($event);
    $ev{'pid'}        = LibAppArmor::aa_log_record::swig_pid_get($event);
    $ev{'task'}        = LibAppArmor::aa_log_record::swig_task_get($event);
    $ev{'info'}        = LibAppArmor::aa_log_record::swig_info_get($event);
    $dmask = LibAppArmor::aa_log_record::swig_denied_mask_get($event);
    $rmask = LibAppArmor::aa_log_record::swig_requested_mask_get($event);
    $ev{'magic_token'}  =
       LibAppArmor::aa_log_record::swig_magic_token_get($event);

    # NetDomain
    if ( $ev{'operation'} && optype($ev{'operation'}) eq "net" ) {
        $ev{'family'}    =
            LibAppArmor::aa_log_record::swig_net_family_get($event);
        $ev{'protocol'}  =
            LibAppArmor::aa_log_record::swig_net_protocol_get($event);
        $ev{'sock_type'} =
            LibAppArmor::aa_log_record::swig_net_sock_type_get($event);
    }

    LibAppArmor::free_record($event);

    if ($ev{'operation'} && $ev{'operation'} =~ /^(capable|dbus|mount|pivotroot|umount)/) {
       $DEBUGGING && debug("parser_event: previous event IGNORED");
       return( undef );
    }

    #map new c and d to w as logprof doesn't support them yet
    if ($rmask) {
        $rmask =~ s/c/w/g;
        $rmask =~ s/d/w/g;
    }
    if ($dmask) {
        $dmask =~ s/c/w/g;
        $dmask =~ s/d/w/g;
    }

    if ($rmask && !validate_log_mode(hide_log_mode($rmask))) {
        fatal_error(sprintf(gettext('Log contains unknown mode %s.'),
                            $rmask));
    }

    if ($dmask && !validate_log_mode(hide_log_mode($dmask))) {
        fatal_error(sprintf(gettext('Log contains unknown mode %s.'),
                    $dmask));
    }
#print "str_to_mode deny $dmask = " . str_to_mode($dmask) . "\n" if ($dmask);
#print "str_to_mode req $rmask = "  . str_to_mode($rmask) . "\n" if ($rmask);

    my ($mask, $name);
    ($mask, $name) = log_str_to_mode($ev{profile}, $dmask, $ev{name2});
    $ev{'denied_mask'} = $mask;
    $ev{name2} = $name;

    ($mask, $name) = log_str_to_mode($ev{profile}, $rmask, $ev{name2});
    $ev{'request_mask'} = $mask;
    $ev{name2} = $name;

    if ( ! $ev{'time'} ) { $ev{'time'} = time; }

    # remove null responses
    for (keys(%ev)) {
        if ( ! $ev{$_} || $ev{$_} !~ /[\/\w]+/)  { delete($ev{$_}); }
    }

    if ( $ev{'sdmode'} ) {
        #0 = invalid, 1 = error, 2 = AUDIT, 3 = ALLOW/PERMIT,
        #4 = DENIED/REJECTED, 5 = HINT, 6 = STATUS/config change
        if    ( $ev{'sdmode'} == 0 ) { $ev{'sdmode'} = "UNKNOWN"; }
        elsif ( $ev{'sdmode'} == 1 ) { $ev{'sdmode'} = "ERROR"; }
        elsif ( $ev{'sdmode'} == 2 ) { $ev{'sdmode'} = "AUDITING"; }
        elsif ( $ev{'sdmode'} == 3 ) { $ev{'sdmode'} = "PERMITTING"; }
        elsif ( $ev{'sdmode'} == 4 ) { $ev{'sdmode'} = "REJECTING"; }
        elsif ( $ev{'sdmode'} == 5 ) { $ev{'sdmode'} = "HINT"; }
        elsif ( $ev{'sdmode'} == 6 ) { $ev{'sdmode'} = "STATUS"; }
        else  { delete($ev{'sdmode'}); }
    }
    if ( $ev{sdmode} ) {
       $DEBUGGING && debug( Data::Dumper->Dump([%ev], [qw(*event)]));
       return \%ev;
    } else {
       return( undef );
    }
}

###############################################################################
# required initialization

$cfg = read_config("logprof.conf");
if ((not defined $cfg->{settings}{default_owner_prompt})) {
    $cfg->{settings}{default_owner_prompt} = 0;
}

$profiledir = find_first_dir($cfg->{settings}{profiledir}) || "/etc/apparmor.d";
unless (-d $profiledir) { fatal_error "Can't find AppArmor profiles."; }

$extraprofiledir = find_first_dir($cfg->{settings}{inactive_profiledir}) ||
"/usr/share/apparmor/extra-profiles/";

$parser = find_first_file($cfg->{settings}{parser}) || "/sbin/apparmor_parser";
unless (-x $parser) { fatal_error "Can't find apparmor_parser."; }

$filename = find_first_file($cfg->{settings}{logfiles}) || "/var/log/syslog";
unless (-f $filename) { fatal_error "Can't find system log."; }

$ldd = find_first_file($cfg->{settings}{ldd}) || "/usr/bin/ldd";
unless (-x $ldd) { fatal_error "Can't find ldd."; }

$logger = find_first_file($cfg->{settings}{logger}) || "/bin/logger";
unless (-x $logger) { fatal_error "Can't find logger."; }

1;

