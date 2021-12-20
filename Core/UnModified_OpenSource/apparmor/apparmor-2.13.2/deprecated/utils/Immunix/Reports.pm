# ------------------------------------------------------------------
#
#    Copyright (C) 2005-2006 Novell/SUSE
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

package Immunix::Reports;

################################################################################
# /usr/lib/perl5/site_perl/Reports.pm
#
#   - Parses /var/log/messages for AppArmor messages
#   - Writes results to .html or comma-delimited (.csv) files (Optional)
#
#  Requires:
#   Immunix::Events;
#   Time::Local (temporary)
#
#  Input (Optional):
#       -Start Date|End Date (Month, Day, Year, Time)
#       -Program Name
#       -Profile Name
#       -PID
#       -Denied Resources
#
################################################################################

use strict;

use DBI;
use DBD::SQLite;
use Locale::gettext;
use POSIX;
use ycp;

setlocale(LC_MESSAGES, "");
textdomain("Reports");

my $eventDb   = '/var/log/apparmor/events.db';
my $numEvents = 1000;

sub YcpDebug ($$) {

    my $argList = "";
	#my ($script, $args) = @_;
	my $script = shift;
	my $args = shift;

    if ($args && ref($args) eq "HASH") {

        for (sort keys(%$args) ) {
            $argList .= "$_ is ..$args->{$_}.., " if $args->{$_};
        }

    } elsif ($args && ref($args) eq "ARRAY") {

        for my $row (@$args) {
            for (sort keys(%$row) ) {
                $argList .= "$_ is ..$row->{$_}.., " if $row->{$_};
            }
        }
	} elsif ( $args ) {
		$argList = $args;
    } else {
        my $prob = ref($args);
		$argList = "Type not supported for printing debug: $prob";
    }

    ycp::y2milestone("[apparmor $script] vars:  $argList");

}

sub month2Num {
    my $lexMon = shift;
    my $months = {
        "Jan" => '01',
        "Feb" => '02',
        "Mar" => '03',
        "Apr" => '04',
        "May" => '05',
        "Jun" => '06',
        "Jul" => '07',
        "Aug" => '08',
        "Sep" => '09',
        "Oct" => '10',
        "Nov" => '11',
        "Dec" => '12'
    };

    my $numMonth = $months->{$lexMon};

    return $numMonth;
}

sub num2Month {
    my $monthNum = shift;

    my @months = ("Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec");
    my $lexMonth = $months[ ($monthNum - 1) ];

    return $lexMonth;
}

# Converts Epoch Time to Formatted Date String
sub getDate {
    my $epTime = shift;

    my $date = localtime($epTime);

    my ($day, $mon, $mondate, $time, $year) = split(/\s+/, $date);
    my ($hour, $min, $sec) = split(/:/, $time);

    $mon = month2Num($mon);

    # we want 2 digits for easier reading
    $mon     = sprintf("%02d", $mon);
    $mondate = sprintf("%02d", $mondate);

    my $newDate = "$year-$mon-$mondate $time";
    return $newDate;
}

sub round {
    my $num = shift;
    $num = sprintf("%.2f", $num);
    return ("$num");
}

# round up
sub pageRound {
    my $num  = shift;
    my $pnum = int($num);

    if ($pnum < $num) {
        $pnum++;
    }

    return $pnum;
}

sub checkFileExists {
    my $file = shift;

    if ($file && -e $file) {
        return 1;
    } else {
        return 0;
    }
}

sub getNetSockList {

    my @netsockList = (
        "All", "UNIX Domain Sockets", "IP v4", "Radio AX.25",
        "Novell IPX", "Appletalk", "Radio NET/ROM",
        "Multiprotocol Bridge", "ATM PVC", "X.25", "IP v6",
        "Radio X.25 PLP", "DECnet", "NetBEUI", "Security Callback",
        "PF Key Management", "Netlink", "Packet", "Ash",
        "Econet", "ATM SVC", "Filler (Ignore)", "Linux SNA",
        "IRDA", "PPPoX", "Wanpipe", "Linux LLC",
        "Filler (Ignore)", "Filler (Ignore)", "Filler (Ignore)", "TIPC",
        "Bluetooth", "IUCV", "RxRPC"
    );


	return \@netsockList;
}

sub netsock_name2num {

    my $sockName = shift;
    my $sockNum = '';
	my $netsockList = getNetSockList();

    my $i;

    for ($i = 0; $i < @$netsockList; $i++) {
        last if $_ eq @$netsockList[$i];
    }

	if ($i < @$netsockList) {
		$sockNum = $i;
	}

	return $sockNum;
}

sub netsock_num2name {

	my $sockNum = shift;
	my $sockName = undef;
	my $netsockList = getNetSockList();

	if ( $sockNum < @$netsockList) {
    	$sockName = @$netsockList[$sockNum];
	}

	if ( $sockName eq "Filler (Ignore)" ) {
		$sockName = undef;
	}

	return $sockName;
}

sub matchFailed ($$) {

	my $args = shift;
	my $rec = shift;

        # Check filters
        if ($args->{'pid'} && $args->{'pid'} ne '-') {
            return(1) unless ($args->{'pid'} eq $rec->{'pid'});
        }
        if (   $args->{'severity'}
            && $args->{'severity'} ne "00"
            && $args->{'severity'} ne '-')
        {
            if ($args->{'severity'} eq "U") { $args->{'severity'} = '-1'; }
            return(1) unless ($args->{'severity'} eq $rec->{'severity'});
        }
        if ($args->{'mode_deny'} && $args->{'mode_deny'} ne '-') {
            return(1) unless ($args->{'mode_deny'} eq $rec->{'mode_deny'});
        }
        if ($args->{'mode_req'} && $args->{'mode_req'} ne '-') {
            return(1) unless ($args->{'mode_req'} eq $rec->{'mode_req'});
        }

        if ($args->{'resource'} && $args->{'resource'} ne '-') {
            return(1) unless ($args->{'resource'} eq $rec->{'resource'});
        }
        if ($args->{'sdmode'} && $args->{'sdmode'} ne '-') {
            # Needs reversal of comparison for sdmode
            return(1) unless ($rec->{'sdmode'} =~ /$args->{'sdmode'}/);
        }
        if ($args->{'op'} && $args->{'op'}  ne '-') {
            return(1) unless ($args->{'op'} eq $rec->{'op'});
        }
        if ($args->{'attr'} && $args->{'attr'}  ne '-') {
            return(1) unless ($args->{'attr'} eq $rec->{'attr'});
        }
        if ($args->{'name_alt'} && $args->{'name_alt'}  ne '-') {
            return(1) unless ($args->{'name_alt'} eq $rec->{'name_alt'});
        }
        if ($args->{'net_family'} && $args->{'net_family'}  ne '-') {
            return(1) unless ($args->{'net_family'} eq $rec->{'net_family'});
        }
        if ($args->{'net_proto'} && $args->{'net_proto'}  ne '-') {
            return(1) unless ($args->{'net_proto'} eq $rec->{'net_proto'});
        }
        if ($args->{'net_socktype'} && $args->{'net_socktype'}  ne '-') {
            return(1) unless ($args->{'net_socktype'} eq $rec->{'net_socktype'});
        }
 
	return 0;
}

# Translate mode & sdmode for parsing
sub rewriteModes {
    my $filts = shift;

    # Mode wrangling - Rewrite for better matches
    for ('mode_req','mode_deny') {

        if ($filts->{$_} && $filts->{$_} ne "All") {

            my @mode    = ();
            my $tmpMode = undef;

            @mode = split(//, $filts->{$_});

            if (@mode > 0) {
                #$tmpMode = join("|", @mode);
                $tmpMode = join("", @mode);
				if ($tmpMode =~ /m/) {
					$tmpMode =~ s/m//g;
					$tmpMode = "m" . $tmpMode;
				}
            } else {
                delete($filts->{$_});
            }

            if ($tmpMode) {
                $filts->{$_} = $tmpMode;
            }
        }
    }

    # Rewrite sdmode for more flexible matches
    if ($filts->{'sdmode'} && $filts->{'sdmode'} ne "All") {
        my @tmpMode = ();
        if ($filts->{'sdmode'} =~ /[pP]/) { push(@tmpMode, 'PERMIT'); }
        if ($filts->{'sdmode'} =~ /[rR]/) { push(@tmpMode, 'REJECT'); }
        if ($filts->{'sdmode'} =~ /[aA]/) { push(@tmpMode, 'AUDIT'); }
        if (@tmpMode > 0) {
            $filts->{'sdmode'} = join('|', @tmpMode);
        } else {
            delete($filts->{'sdmode'});
        }
    }

    return $filts;
}

sub getFilterList ($) {

    my $args = shift;
	my $filts = undef;

    if ($args->{'prog'})     { $filts->{'prog'}     = $args->{'prog'}; }
    if ($args->{'profile'})  { $filts->{'profile'}  = $args->{'profile'}; }
    if ($args->{'pid'})      { $filts->{'pid'}      = $args->{'pid'}; }
    if ($args->{'resource'}) { $filts->{'resource'} = $args->{'resource'}; }
    if ($args->{'severity'}) { $filts->{'severity'} = $args->{'severity'}; }
    if ($args->{'sdmode'})   { $filts->{'sdmode'}   = $args->{'sdmode'}; }
    if ($args->{'mode_req'}) { $filts->{'mode_req'} = $args->{'mode_req'}; }
    if ($args->{'mode_deny'}) { $filts->{'mode_deny'} = $args->{'mode_deny'}; }
    if ($args->{'op'})       { $filts->{'op'}       = $args->{'op'}; }
    if ($args->{'attr'})     { $filts->{'attr'}     = $args->{'attr'}; }
    if ($args->{'name_alt'}) { $filts->{'name_alt'} = $args->{'name_alt'}; }
    if ($args->{'net_family'}) { $filts->{'net_family'} = $args->{'net_family'}; }
    if ($args->{'net_proto'})  { $filts->{'net_proto'}  = $args->{'net_proto'}; }
    if ($args->{'net_socktype'}) { $filts->{'net_socktype'} = $args->{'net_socktype'}; }

    for (sort(keys(%$filts))) {
        if ($filts->{$_} eq '-' || $filts->{$_} eq 'All') {
            delete($filts->{$_});
        }
    }
    return $filts;
}

sub enableEventD {

    # make sure the eventd is enabled before we do any reports
    my $need_enable = 0;
    if (open(SDCONF, "/etc/apparmor/subdomain.conf")) {
        while (<SDCONF>) {
            if (/^\s*APPARMOR_ENABLE_AAEVENTD\s*=\s*(\S+)\s*$/) {
                my $flag = lc($1);

                # strip quotes from the value if present
                $flag = $1 if $flag =~ /^"(\S+)"$/;
                $need_enable = 1 if $flag ne "yes";
            }
        }
        close(SDCONF);
    }

    # if the eventd isn't enabled, we'll turn it on the first time they
    # run a report and start it up - if something fails for some reason,
    # we should just fall through and the db check should correctly tell
    # the caller that the db isn't initialized correctly
    if ($need_enable) {
        my $old = "/etc/apparmor/subdomain.conf";
        my $new = "/etc/apparmor/subdomain.conf.$$";
        if (open(SDCONF, $old)) {
            if (open(SDCONFNEW, ">$new")) {
                my $foundit = 0;

                while (<SDCONF>) {
                    if (/^\s*APPARMOR_ENABLE_AAEVENTD\s*=/) {
                        print SDCONFNEW "APPARMOR_ENABLE_AAEVENTD=\"yes\"\n";

                        $foundit = 1;
                    } else {
                        print SDCONFNEW;
                    }
                }

                unless ($foundit) {
                    print SDCONFNEW "APPARMOR_ENABLE_AAEVENTD=\"yes\"\n";
                }

                close(SDCONFNEW);

                # if we were able to overwrite the old config
                # config file with the new stuff, we'll kick
                # the init script to start up aa-eventd
                if (rename($new, $old)) {
                    if (-e "/sbin/rcaaeventd") {
                        system("/sbin/rcaaeventd restart >/dev/null 2>&1");
                    } else {
                        system("/sbin/rcapparmor restart >/dev/null 2>&1");
                    }
                }
            }
            close(SDCONF);
        }

    }

    return $need_enable;
}

# Check that events db exists and is populated
#	- Returns 1 for good db, 0 for bad db
sub checkEventDb {
    my $count   = undef;
    my $eventDb = '/var/log/apparmor/events.db';

    # make sure the event daemon is enabled
    if (enableEventD()) {

        my $now = time;

        # wait until the event db appears or we hit 1 min
        while (!-e $eventDb) {
            sleep 2;
            return 0 if ((time - $now) >= 60);
        }

        # wait until it stops changing or we hit 1 min - the event
        # daemon flushes events to the db every five seconds.
        my $last_modified = 0;
        my $modified      = (stat($eventDb))[9];
        while ($last_modified != $modified) {
            sleep 10;
            last if ((time - $now) >= 60);
            $last_modified = $modified;
            $modified      = (stat($eventDb))[9];
        }
    }

    my $query = "SELECT count(*) FROM events ";

    # Pull stuff from db
    my $dbh = DBI->connect("dbi:SQLite:dbname=$eventDb", "", "", { RaiseError => 1, AutoCommit => 1 });

    eval {
        my $sth = $dbh->prepare($query);
        $sth->execute;
        $count = $sth->fetchrow_array();

        $sth->finish;
    };

    if ($@) {
        ycp::y2error(sprintf(gettext("DBI Execution failed: %s."), $DBI::errstr));
        return;
    }

    $dbh->disconnect();

    if ($count && $count > 0) {
        return 1;
    } else {
        return 0;
    }
}

# Called from ag_reports_parse
sub getNumPages ($) {
    my $args     = shift;
    my $db       = ();
    my $numPages = 0;
    my $count    = 0;
    my $type     = undef;
    my $eventRep = "/var/log/apparmor/reports/events.rpt";

    # Figure out whether we want db count or file parse
    if ($args->{'type'}) {
        if ($args->{'type'} eq 'sir' || $args->{'type'} eq 'ess-multi') {
            $type = 'db';
        } elsif ($args->{'type'} eq 'ess') {
            return 1;    # ess reports have one page by definition
        } else {
            $type = 'arch';    # archived or file
        }
    }

    # Parse sdmode & mode labels
    if ($args->{'sdmode'}) {
        $args->{'sdmode'} =~ s/\&//g;
        $args->{'sdmode'} =~ s/\://g;
        $args->{'sdmode'} =~ s/\s//g;
        $args->{'sdmode'} =~ s/AccessType//g;

        if ($args->{'sdmode'} eq "All") {
            delete($args->{'sdmode'});
        }
    }

    if ($args->{'mode_req'}) {
        $args->{'mode_req'} =~ s/\&//g;
        $args->{'mode_req'} =~ s/Mode\://g;
        $args->{'mode_req'} =~ s/\s//g;

        if ($args->{'mode_req'} eq "All") {
            delete($args->{'mode_req'});
        }
    }
    ########################################

    $args = rewriteModes($args);

    if ($type && $type eq 'db') {

        my $start = undef;
        my $end   = undef;

        if ($args->{'startTime'} && $args->{'startTime'} > 0) {
            $start = $args->{'startTime'};
        }

        if ($args->{'endTime'} && $args->{'endTime'} > 0) {
            $end = $args->{'endTime'};
        }

        my $query = "SELECT count(*) FROM events ";

        # We need filter information for getting a correct count
        my $filts = getFilterList($args);
        my $midQuery = getQueryFilters($filts, $start, $end);
        if ($midQuery) { $query .= "$midQuery"; }
        # Pull stuff from db
        my $dbh = DBI->connect("dbi:SQLite:dbname=$eventDb", "", "", { RaiseError => 1, AutoCommit => 1 });

        eval {
            my $sth = $dbh->prepare($query);
            $sth->execute;
            $count = $sth->fetchrow_array();

            $sth->finish;
        };

        if ($@) {
            ycp::y2error(sprintf(gettext("DBI Execution failed: %s."), $DBI::errstr));
            return;
        }

        $dbh->disconnect();

        $numPages = pageRound($count / $numEvents);
        if ($numPages < 1) { $numPages = 1; }

    } elsif ($type && $type eq 'arch') {

        if (open(REP, "<$eventRep")) {

            while (<REP>) {
                if (/^Page/) {
                    $numPages++;
                } else {
                    $count++;
                }
            }

            close REP;

        } else {
            ycp::y2error(sprintf(gettext("Couldn't open file: %s."), $eventRep));
        }

    } else {
        ycp::y2error(gettext("No type value passed.  Unable to determine page count."));
        return ("1");
    }

    if ($numPages < 1) { $numPages = 1; }

    my $numCheck = int($count / $numEvents);

    if ($numPages < $numCheck) {
        $numPages = $numCheck;
    }

    return ($numPages);
}

sub getEpochFromNum {
    my $date = shift;
    my $place = shift || undef;    # Used to set default $sec if undef

    my ($numMonth, $numDay, $time, $year) = split(/\s+/, $date);
    my ($hour, $min, $sec) = '0';
    my $junk = undef;

    if ($time =~ /:/) {
        ($hour, $min, $sec, $junk) = split(/\:/, $time);
        if (!$hour || $hour eq "") { $hour = '0'; }
        if (!$min  || $min  eq "") { $min  = '0'; }
        if (!$sec || $sec eq "") {
            if ($place eq 'end') {
                $sec = '59';
            } else {
                $sec = '0';
            }
        }
    }

    $numMonth--;    # Months start from 0 for epoch translation

    if (!$year) { $year = (split(/\s+/, localtime))[4]; }
    my $epochDate = timelocal($sec, $min, $hour, $numDay, $numMonth, $year);

    return $epochDate;
}

sub getEpochFromStr {
    my $lexDate = shift;

    my ($lexMonth, $dateDay, $fullTime, $year) = split(/\s+/, $lexDate);
    my ($hour, $min, $sec) = split(/\:/, $fullTime);

    if (!$year) { $year = (split(/\s+/, localtime))[4]; }

    my $numMonth = month2Num($lexMonth);

    my $epochDate = timelocal($sec, $min, $hour, $dateDay, $numMonth, $year);

    return $epochDate;
}

# Replaces old files with new files
sub updateFiles {
    my ($oldFile, $newFile) = @_;

    if (unlink("$oldFile")) {
        if (!rename("$newFile", "$oldFile")) {
            if (!system('/bin/mv', "$newFile", "$oldFile")) {
                ycp::y2error(sprintf(gettext("Failed copying %s."), $oldFile));
                return 1;
            }
        }
    } else {
        system('/bin/rm', "$oldFile");
        system('/bin/mv', "$newFile", "$oldFile");
    }

    return 0;
}

# This is a holder, that was originally part of exportLog()
# Used by /usr/bin/reportgen.pl
sub exportFormattedText {
    my ($repName, $logFile, $db) = @_;

    my $date = localtime;
    open(LOG, ">$logFile") || die "Couldn't open $logFile";

    print LOG "$repName: Log generated by Novell AppArmor, $date\n\n";
    printf LOG "%-21s%-32s%-8s%-51s", "Host", "Date", "Program", "Profile",
		"PID", "Severity", "Mode Deny", "Mode Request","Detail", "Access Type",
		"Operation", "Attribute", "Additional Name", "Parent", "Active Hat",
		"Net Family", "Net Protocol", "Net Socket Type";

    print LOG "\n";

    for (sort (@$db)) {
        print LOG "$_->{'host'},$_->{'time'},$_->{'prog'},$_->{'profile'},";
        print LOG "$_->{'pid'},$_->{'severity'},$_->{'mode_deny'},$_->{'mode_req'},";
		print LOG "$_->{'resource'},$_->{'sdmode'},$_->{'op'},$_->{'attr'},";
		print LOG "$_->{'name_alt'},$_->{'parent'},$_->{'active_hat'},";
		print LOG "$_->{'net_family'},$_->{'net_proto'},$_->{'net_socktype'}\n";
    }

    close LOG;
}

sub exportLog {

    my ($exportLog, $db, $header) = @_;

	return unless $db;

    if (open(LOG, ">$exportLog")) {

        my $date = localtime();

        if ($exportLog =~ /csv/) {

            # $header comes from reportgen.pl (scheduled reports)
            if ($header) { print LOG "$header\n\n"; }

            for (@$db) {
		        print LOG "$_->{'host'},$_->{'time'},$_->{'prog'},$_->{'profile'},";
		        print LOG "$_->{'pid'},$_->{'severity'},$_->{'mode_deny'},$_->{'mode_req'},";
				print LOG "$_->{'resource'},$_->{'sdmode'},$_->{'op'},$_->{'attr'},";
				print LOG "$_->{'name_alt'},$_->{'parent'},$_->{'active_hat'},";
				print LOG "$_->{'net_family'},$_->{'net_proto'},$_->{'net_socktype'}\n";
            }

        } elsif ($exportLog =~ /html/) {

            print LOG "<html><body bgcolor='fffeec'>\n\n";
            print LOG "<font face='Helvetica,Arial,Sans-Serif'>\n";

            # $header comes from reportgen.pl (scheduled reports)
            if ($header) {
                print LOG "$header\n\n";
            } else {
                print LOG "<br><h3>$exportLog</h3><br>\n<h4>Log generated by Novell AppArmor, $date</h4>\n\n";
            }

            print LOG "<hr><br><table border='1' cellpadding='2'>\n";

            print LOG "<tr bgcolor='edefff'><th>Host</th><th>Date</th><th>Program</th>" .
				"<th>Profile</th><th>PID</th><th>Severity</th><th>Mode Deny</th>" .
				"<th>Mode Request</th><th>Detail</th><th>Access Type</th><th>Operation</th>" .
				"<th>Attribute</th><th>Additional Name</th><th>Parent</th><th>Active Hat</th>" .
				"<th>Net Family</th><th>Net Protocol</th><th>Net Socket Type</th></tr>\n";

            my $idx = 1;

            for (@$db) {
                $idx++;

				my $logLine = 
					"<td>&nbsp;$_->{'date'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'prog'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'profile'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'pid'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'severity'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'mode_deny'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'mode_req'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'resource'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'sdmode'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'op'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'attr'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'name_alt'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'parent'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'active_hat'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'net_family'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'net_proto'}&nbsp;</td>"
                      . "<td>&nbsp;$_->{'net_socktype'}&nbsp;</td></tr>";

	            my $plainCell = "<tr><td>&nbsp;$_->{'host'}&nbsp;</td>";
				my $shadedCell = "<tr='edefef'><td>&nbsp;$_->{'host'}&nbsp;</td>";
				my $logLinePlain = $plainCell . $logLine;
				my $logLineShaded = $shadedCell . $logLine; 

                if ($idx % 2 == 0) {
                    print LOG "$logLinePlain\n"; 
                } else {
                    # Shade every other row
                    print LOG "$logLineShaded\n"; 
                }
            }

            print LOG "<br></table></font></body></html>\n\n";
        }

        close LOG;
    } else {
        ycp::y2error(sprintf(gettext("Export Log Error: Couldn't open %s"), $exportLog));
    }

}

# Pulls info on single report from apparmor xml file
sub getXmlReport {
    my ($repName, $repConf) = @_;

    my $repFlag = 0;
    my %rep     = ();

    if (defined($repName) && ref($repName)) {

        if ($repName->{'base'}) {
            $repName = $repName->{'base'};
        } elsif ($repName->{'name'}) {
            $repName = $repName->{'name'};
        }
    }

    if (!$repName) {
        ycp::y2error(gettext("Fatal error.  No report name given. Exiting."));
    }

    if (!$repConf || !-e $repConf) {
        $repConf = '/etc/apparmor/reports.conf';
        if (!-e $repConf) {
            ycp::y2error(
                sprintf(
                    gettext(
                        "Unable to get configuration info for %s.
                Unable to find %s."
                    ),
                    $repName,
                    $repConf
                )
            );
            exit 1;
        }
    }

    if (open(XML, "<$repConf")) {

        while (<XML>) {

            chomp;

            if (/\<name\>/) {

                /\<name\>(.+)\<\/name\>/;
                my $name = $1;
                if ($name eq $repName) {
                    $rep{'name'} = $name;
                    $repFlag = 1;
                }

            } elsif (/\<\/report\>/) {

                $repFlag = 0;

            } elsif ($repFlag == 1) {
                if (/\s*\<\w+\s+(.*)\/\>.*$/) {
                    my $attrs = $1;
                    chomp($attrs);
                    my @attrlist = split(/\s+/, $attrs);
                    for (@attrlist) {

                        #Match attributes
                        if (/\s*(\S+)=\"(\S+)\"/) {
                            $rep{$1} = $2 unless $2 eq '-';
                        }
                    }
                } elsif (/\<(\w+)\>([\w+|\/].*)\<\//) {

                    if ($1) {
                        $rep{"$1"} = $2 unless $2 eq '-';
                    } else {
                        ycp::y2error(sprintf(gettext("Failed to parse: %s."), $_));
                    }
                }
            }
        }

        close XML;

    } else {
        ycp::y2error(sprintf(gettext("Fatal Error.  Couldn't open %s."), $repConf));
        exit 1;
    }

    return \%rep;
}

# Returns info on currently confined processes
sub getCfInfo {

    my $ref  = ();
    my @cfDb = ();

    my $cfApp = '/usr/sbin/unconfined';

    if (open(CF, "$cfApp |")) {

        my $host = `hostname`;
        chomp($host);

        my $date = localtime;

        while (<CF>) {

            my $ref = ();
            my $all = undef;
            $ref->{'host'} = $host;
            $ref->{'date'} = $date;
            chomp;

            ($ref->{'pid'}, $ref->{'prog'}, $all) = split(/\s+/, $_, 3);
            $all = /\s*((not)*\s*confined\s*(by)*)/;
            $ref->{'state'} = $1;
            $ref->{'state'} =~ s/\s*by//g;
            $ref->{'state'} =~ s/not\s+/not-/g;
            ($ref->{'prof'}, $ref->{'type'}) = split(/\s+/, $_);

            if ($ref->{'prog'}  eq "") { $ref->{'prog'}  = "-"; }
            if ($ref->{'prof'}  eq "") { $ref->{'prof'}  = "-"; }
            if ($ref->{'pid'}   eq "") { $ref->{'pid'}   = "-"; }
            if ($ref->{'state'} eq "") { $ref->{'state'} = "-"; }
            if ($ref->{'type'}  eq "") { $ref->{'type'}  = "-"; }

            push(@cfDb, $ref);
        }
        close CF;

    } else {
        my $error = sprintf(gettext("Fatal Error.  Can't run %s.  Exiting."), $cfApp);
        ycp::y2error($error);
        return $error;
    }

    return (\@cfDb);
}

# generate stats for ESS reports
sub getEssStats {
    my $args = shift;

    #my ($host, $targetDir, $startdate, $enddate) = @_;

    my @hostDb    = ();
    my @hostList  = ();
    my $targetDir = undef;
    my $host      = undef;
    my $startdate = undef;
    my $enddate   = undef;

    if (!$args->{'targetDir'}) {
        $targetDir = '/var/log/apparmor/';
    }

    if ($args->{'host'}) { $host = $args->{'host'}; }

    if ($args->{'startdate'}) {
        $startdate = $args->{'startdate'};
    } else {
        $startdate = '1104566401';    # Jan 1, 2005
    }

    if ($args->{'enddate'}) {
        $enddate = $args->{'enddate'};
    } else {
        $enddate = time;
    }

    if (!-e $targetDir) {
        ycp::y2error(sprintf(gettext("Fatal Error.  No directory, %s, found.  Exiting."), $targetDir));
        return;
    }

    # Max Sev, Ave. Sev, Num. Rejects, Start Time, End Time
    my $ctQuery = "SELECT count(*) FROM events WHERE time >= $startdate AND time <= $enddate";

    my $query = "SELECT MAX(severity), AVG(severity), COUNT(id), MIN(time), "
      . "MAX(time) FROM events WHERE sdmode='REJECTING' AND "
      . "time >= $startdate AND time <= $enddate";

    # Get list of hosts to scan
    if (opendir(TDIR, $targetDir)) {

        @hostList = grep(/\.db/, readdir(TDIR));
        close TDIR;

    } else {
        ycp::y2error(sprintf(gettext("Fatal Error.  Couldn't open %s.  Exiting"), $targetDir));
        return;
    }

    # Cycle through for each host
    for my $eventDb (@hostList) {

        $eventDb = "$targetDir/$eventDb";

        my $ess   = undef;
        my $ret   = undef;
        my $count = undef;

        my $dbh = DBI->connect("dbi:SQLite:dbname=$eventDb", "", "", { RaiseError => 1, AutoCommit => 1 });

        # get hostname
        my $host      = undef;
        my $hostQuery = "SELECT * FROM info WHERE name='host'";

        eval {
            my $sth = $dbh->prepare($hostQuery);
            $sth->execute;
            $host = $sth->fetchrow_array();
            $sth->finish;
        };

        if ($@) {
            ycp::y2error(sprintf(gettext("DBI Execution failed: %s."), $DBI::errstr));
            return;
        }

        # Get number of events
        eval {
            my $sth = $dbh->prepare($ctQuery);
            $sth->execute;
            $count = $sth->fetchrow_array();
            $sth->finish;
        };

        if ($@) {
            ycp::y2error(sprintf(gettext("DBI Execution failed: %s."), $DBI::errstr));
            return;
        }

        # Get rest of stats
        eval { $ret = $dbh->selectall_arrayref("$query"); };

        if ($@) {
            ycp::y2error(sprintf(gettext("DBI Execution failed: %s."), $DBI::errstr));
            return;
        }

        $dbh->disconnect();

        # hostIp, startDate, endDate, sevHi,  sevMean, numRejects
        if ($host) {
            $ess->{'host'} = $host;
        } else {
            $ess->{'host'} = '';
        }

        $ess->{'sevHi'} = $$ret[0]->[0];

        if (!$ess->{'sevHi'}) {
            $ess->{'sevHi'} = 0;
        }

        $ess->{'sevMean'} = $$ret[0]->[1];

        if (!$ess->{'sevMean'} || $ess->{'sevHi'} == 0) {
            $ess->{'sevMean'} = 0;
        } else {
            $ess->{'sevMean'} = round("$ess->{'sevMean'}");
        }

        $ess->{'numRejects'} = $$ret[0]->[2];
        $ess->{'startdate'}  = $$ret[0]->[3];
        $ess->{'enddate'}    = $$ret[0]->[4];
        $ess->{'numEvents'}  = $count;

        # Convert dates
        if ($ess->{'startdate'} && $ess->{'startdate'} !~ /:/) {
            $ess->{'startdate'} = Immunix::Reports::getDate($ess->{'startdate'});
        }
        if ($ess->{'enddate'} && $ess->{'enddate'} !~ /:/) {
            $ess->{'enddate'} = Immunix::Reports::getDate($ess->{'enddate'});
        }

        push(@hostDb, $ess);
    }

    return \@hostDb;
}

# get ESS stats for archived reports (warning -- this can be slow for large files
# debug -- not fully functional yet
sub getArchEssStats {
    my $args = shift;

    my $prevTime  = '0';
    my $prevDate  = '0';
    my $startDate = '1104566401';    # Jan 1, 2005
    my $endDate   = time;

    if ($args->{'startdate'}) { $startDate = $args->{'startdate'}; }
    if ($args->{'enddate'})   { $endDate   = $args->{'enddate'}; }

    # hostIp, startDate, endDate, sevHi,  sevMean, numRejects
    my @eventDb = getEvents("$startDate", "$endDate");

    my @hostIdx = ();    # Simple index to all hosts for quick host matching
    my @hostDb  = ();    # Host-keyed Data for doing REJECT stats

    # Outer Loop for Raw Event db
    for (@eventDb) {

        if ($_->{'host'}) {

            my $ev = $_;             # current event record

            # Create new host entry, or add to existing
            if (grep(/$ev->{'host'}/, @hostIdx) == 1) {

                # Inner loop, but the number of hosts should be small
                for (@hostDb) {

                    if ($_->{'host'} eq $ev->{'host'}) {

                        # Find earliest start date
                        if ($_->{'startdate'} > $ev->{'date'}) {
                            $_->{'startdate'} = $ev->{'date'};
                        }

                        # tally all events reported for host
                        $_->{'numEvents'}++;

                        if ($ev->{'sdmode'}) {
                            if ($ev->{'sdmode'} =~ /PERMIT/) {
                                $_->{'numPermits'}++;
                            }
                            if ($ev->{'sdmode'} =~ /REJECT/) {
                                $_->{'numRejects'}++;
                            }
                            if ($ev->{'sdmode'} =~ /AUDIT/) {
                                $_->{'numAudits'}++;
                            }
                        }

                        # Add stats to host entry
                        #if ( $ev->{'severity'} && $ev->{'severity'} =~ /\b\d+\b/ ) {}
                        if ($ev->{'severity'} && $ev->{'severity'} != -1) {

                            $_->{'sevNum'}++;
                            $_->{'sevTotal'} = $_->{'sevTotal'} + $ev->{'severity'};

                            if ($ev->{'severity'} > $_->{'sevHi'}) {
                                $_->{'sevHi'} = $ev->{'severity'};
                            }
                        } else {
                            $_->{'unknown'}++;
                        }
                    }
                }

            } else {

                # New host
                my $rec = undef;
                push(@hostIdx, $ev->{'host'});    # Add host entry to index

                $rec->{'host'}      = $ev->{'host'};
                $rec->{'startdate'} = $startDate;

                #$rec->{'startdate'} = $ev->{'date'};

                if ($endDate) {
                    $rec->{'enddate'} = $endDate;
                } else {
                    $rec->{'enddate'} = time;
                }

                # Add stats to host entry
                if ($ev->{'sev'} && $ev->{'sev'} ne "U") {

                    $rec->{'sevHi'}    = $ev->{'sev'};
                    $rec->{'sevTotal'} = $ev->{'sev'};
                    $rec->{'sevNum'}   = 1;
                    $rec->{'unknown'}  = 0;

                } else {

                    $rec->{'sevHi'}    = 0;
                    $rec->{'sevTotal'} = 0;
                    $rec->{'sevNum'}   = 0;
                    $rec->{'unknown'}  = 1;

                }

                # Start sdmode stats
                $rec->{'numPermits'} = 0;
                $rec->{'numRejects'} = 0;
                $rec->{'numAudits'}  = 0;
                $rec->{'numEvents'}  = 1;    # tally all events reported for host

                if ($ev->{'sdmode'}) {
                    if ($ev->{'sdmode'} =~ /PERMIT/) { $rec->{'numPermits'}++; }
                    if ($ev->{'sdmode'} =~ /REJECT/) { $rec->{'numRejects'}++; }
                    if ($ev->{'sdmode'} =~ /AUDIT/)  { $rec->{'numAudits'}++; }
                }

                push(@hostDb, $rec);         # Add new records to host data list
            }

        } else {
            next;                            # Missing host info -- big problem
        }
    }    # END @eventDb loop

    # Process simple REJECT-related stats (for Executive Security Summaries)
    for (@hostDb) {

# In the end, we want this info:
#	- Hostname, Startdate, Enddate, # Events, # Rejects, Ave. Severity, High Severity

        if ($_->{'sevTotal'} > 0 && $_->{'sevNum'} > 0) {
            $_->{'sevMean'} = round($_->{'sevTotal'} / $_->{'sevNum'});
        } else {
            $_->{'sevMean'} = 0;
        }

        # Convert dates
        if ($_->{'startdate'} !~ /:/) {
            $_->{'startdate'} = getDate($startDate);
        }
        if ($_->{'enddate'} !~ /:/) {
            $_->{'enddate'} = getDate($_->{'enddate'});
        }

        # Delete stuff that we may use in later versions (YaST is a silly,
        # silly data handler)
        delete($_->{'sevTotal'});
        delete($_->{'sevNum'});
        delete($_->{'numPermits'});
        delete($_->{'numAudits'});
        delete($_->{'unknown'});

    }

    return (\@hostDb);
}

# special version of getEvents() for /usr/bin/reportgen.pl
sub grabEvents {
    my ($rep, $start, $end) = @_;

    my $db       = undef;
    my $prevDate = "0";
    my $prevTime = "0";

    my $query = "SELECT * FROM events ";

    # Clear unnecessary filters
	for my $filt (%$rep) {
		next unless $filt && $rep->{$filt};
		$rep->{$filt} =~ s/\s+//g; 	# repname won't be in here, so no spaces
		if ( $rep->{$filt} eq "-" || $rep->{$filt} eq 'All' || 
			$rep->{$filt} eq '*' )
		{
			delete($rep->{$filt});
		}	
	}

    $rep = rewriteModes($rep);

    # Set Dates far enough apart to get all entries (ie. no date filter)
    my $startDate = '1104566401';    # Jan 1, 2005
    my $endDate   = time;

    if ($start && $start > 0) { $startDate = $start; }

    if (ref($rep)) {
        my $midQuery = getQueryFilters($rep, $startDate, $endDate);
        $query .= "$midQuery";
    }

    $db = getEvents($query, "$startDate", "$endDate");

    return ($db);
}

sub getQueryFilters {
    my ($filts, $start, $end) = @_;

    my $query = undef;
    my $wFlag = 0;

    if ($filts) {

        # Match any requested filters or drop record
        ############################################################
		for my $key(keys(%$filts)) {

			# Special case for severity
	        if ( $key eq 'severity' ) {

	            if ($filts->{$key} eq "-" || $filts->{$key} eq "All") {
	                delete($filts->{$key});
	            } elsif ($filts->{$key} eq "-1"
	                || $filts->{$key} eq "U")
	            {
	                if ($wFlag == 1) {
	                    $query .= "AND events.severity = '-1' ";
	                } else {
	                    $query .= "WHERE events.severity = '-1' ";
	                }
	                $wFlag = 1;
	            } else {
	                if ($wFlag == 1) {
	                    $query .= "AND events.severity >= \'$filts->{$key}\' ";
	                } else {
	                    $query .= "WHERE events.severity >= \'$filts->{$key}\' ";
	                }
	                $wFlag = 1;
	            }

			# Special case for sdmode
	        } elsif ($filts->{'sdmode'}) {

	            if ($filts->{'sdmode'} =~ /\|/) {

	                my @sdmunge = split(/\|/, $filts->{'sdmode'});
	                for (@sdmunge) { $_ = "\'\%" . "$_" . "\%\'"; }

	                $filts->{'sdmode'} = join(" OR events.sdmode LIKE ", @sdmunge);

	            } else {
	                $filts->{'sdmode'} = "\'\%" . "$filts->{'sdmode'}" . "\%\'";
	            }

	            if ($wFlag == 1) {
	                $query .= "AND events.sdmode LIKE $filts->{'sdmode'} ";
	            } else {
	                $query .= "WHERE events.sdmode LIKE $filts->{'sdmode'} ";
	            }
	            $wFlag = 1;

			# All other filters
			} elsif ($wFlag == 0) {	
	            $query .= "WHERE events.$key LIKE \'\%$filts->{$key}\%\' ";
	            $wFlag = 1;
			} else {
                $query .= "AND events.$key LIKE \'\%$filts->{$key}\%\' ";
			}
		}
    }

    if ($start && $start =~ /\d+/ && $start > 0) {
        if ($wFlag == 1) {
            $query .= "AND events.time >= $start ";
        } else {
            $query .= "WHERE events.time >= $start ";
        }
        $wFlag = 1;
    }

    if ($end && $end =~ /\d+/ && $end > $start) {
        if ($wFlag == 1) {
            $query .= "AND events.time <= $end ";
        } else {
            $query .= "WHERE events.time <= $end ";
        }
    }

    return $query;
}

sub getQuery {
    my ($filts, $page, $sortKey, $numEvents) = @_;

    if (!$page || $page < 1 || $page !~ /\d+/) { $page = 1; }
    if (!$sortKey)   { $sortKey   = 'time'; }
    if (!$numEvents) { $numEvents = '1000'; }

    my $limit = (($page * $numEvents) - $numEvents);

    my $query = "SELECT * FROM events ";

    if ($filts) {
        my $midQuery = getQueryFilters($filts);
        if ($midQuery) { $query .= "$midQuery"; }
    }

    # Finish query
    $query .= "Order by $sortKey LIMIT $limit,$numEvents";

    return $query;
}

# Creates single hashref for the various filters
sub setFormFilters {
    my $args  = shift;

    my $filts = undef;

    if ($args) {

        if ($args->{'prog'})     { $filts->{'prog'}     = $args->{'prog'}; }
        if ($args->{'profile'})  { $filts->{'profile'}  = $args->{'profile'}; }
        if ($args->{'pid'})      { $filts->{'pid'}      = $args->{'pid'}; }
        if ($args->{'resource'}) { $filts->{'resource'} = $args->{'resource'}; }
        if ($args->{'severity'}) { $filts->{'severity'} = $args->{'severity'}; }
        if ($args->{'sdmode'})   { $filts->{'sdmode'}   = $args->{'sdmode'}; }
        if ($args->{'mode'})     { $filts->{'mode_req'} = $args->{'mode'}; }
        if ($args->{'mode_req'}) { $filts->{'mode_req'} = $args->{'mode_req'}; }
        if ($args->{'mode_deny'}) { $filts->{'mode_deny'} = $args->{'mode_deny'}; }

    }

    return $filts;
}

# helper for getSirFilters()
# Makes gui-centric filters querying-friendly
sub rewriteFilters {
    my $filts = shift;

    # Clear unnecessary filters
    for (keys(%$filts)) {
        if ($filts->{$_} eq "All") { delete($filts->{$_}); }
    }

    if ($filts->{'prog'}
        && ($filts->{'prog'} eq "-" || $filts->{'prog'} eq "All"))
    {
        delete($filts->{'prog'});
    }
    if ($filts->{'profile'} && ($filts->{'profile'} eq "-")) {
        delete($filts->{'profile'});
    }
    if ($filts->{'pid'} && ($filts->{'pid'} eq "-")) {
        delete($filts->{'pid'});
    }
    if ($filts->{'severity'} && ($filts->{'severity'} eq "-")) {
        delete($filts->{'severity'});
    }
    if ($filts->{'resource'} && ($filts->{'resource'} eq "-")) {
        delete($filts->{'resource'});
    }

    if ($filts->{'mode_req'}
        && ($filts->{'mode_req'} eq "-" || $filts->{'mode_req'} eq "All"))
    {
        delete($filts->{'mode_req'});
    }

    if ($filts->{'sdmode'}
        && ($filts->{'sdmode'} eq "-" || $filts->{'sdmode'} eq "All"))
    {
        delete($filts->{'sdmode'});
    }

    $filts = rewriteModes($filts);
    return $filts;
}

# returns ref to active filters for the specific SIR report
sub getSirFilters {
    my $args    = shift;

    my $repName = undef;

    if ($args && $args->{'name'}) {
        $repName = $args->{'name'};
    } else {
        $repName = "Security.Incident.Report";
    }

    my $repConf = '/etc/apparmor/reports.conf';
    my $rec     = undef;

    my $filts = getXmlReport($repName);

    # Clean hash of useless refs
    for (sort keys(%$filts)) {
        if ($filts->{$_} eq "-") {
            delete($filts->{$_});
        }
    }

    # remove non-filter info
    if ($filts->{'name'})       { delete($filts->{'name'}); }
    if ($filts->{'exportpath'}) { delete($filts->{'exportpath'}); }
    if ($filts->{'exporttype'}) { delete($filts->{'exporttype'}); }
    if ($filts->{'addr1'})      { delete($filts->{'addr1'}); }
    if ($filts->{'addr2'})      { delete($filts->{'addr2'}); }
    if ($filts->{'addr3'})      { delete($filts->{'addr3'}); }
    if ($filts->{'time'})       { delete($filts->{'time'}); }

    if (!$args->{'gui'} || $args->{'gui'} ne "1") {
        $filts = rewriteModes($filts);
        $filts = rewriteFilters($filts);
    }

    return $filts;
}

# Main SIR report generator
sub getEvents {
    my ($query, $start, $end, $dbFile) = @_;

    my @events   = ();
    my $prevTime = 0;
    my $prevDate = '0';

    if (!$query || $query !~ /^SELECT/) { $query = "SELECT * FROM events"; }
    if ($dbFile && -f $dbFile) { $eventDb = $dbFile; }

    my $hostName = `/bin/hostname` || 'unknown';
    chomp $hostName unless $hostName eq 'unknown';

    if (!$start) { $start = '1104566401'; }    # Give default start of 1/1/2005
    if (!$end)   { $end   = time; }

    # make sure they don't give us a bad range
    ($start, $end) = ($end, $start) if $start > $end;

	# Events Schema
	#   id, time, counter, op, pid, sdmode, type, mode_deny, mode_req,
	#   resource, target, profile, prog, name_alt, attr, parent, active_hat,
	#   net_family, net_proto, net_socktype, severity

    # Pull stuff from db
    my $dbh = DBI->connect("dbi:SQLite:dbname=$eventDb", "", "", { RaiseError => 1, AutoCommit => 1 });
    my $all = undef;
    eval { $all = $dbh->selectall_arrayref("$query"); };

    if ($@) {
        ycp::y2error(sprintf(gettext("DBI Execution failed: %s."), $DBI::errstr));
        return;
    }

    $dbh->disconnect();

    for my $row (@$all) {
        my $rec = undef;

        ($rec->{'id'},$rec->{'time'},$rec->{'counter'},$rec->{'op'},$rec->{'pid'},
		$rec->{'sdmode'},$rec->{'type'},$rec->{'mode_deny'},$rec->{'mode_req'},
		$rec->{'resource'},$rec->{'target'},$rec->{'profile'}, $rec->{'prog'},
		$rec->{'name_alt'},$rec->{'attr'},$rec->{'parent'},$rec->{'active_hat'},
		$rec->{'net_family'},$rec->{'net_proto'},$rec->{'net_socktype'},
		$rec->{'severity'}) = @$row;

        # Give empty record values a default value
        if (!$rec->{'host'}) { $rec->{'host'} = $hostName; }
        for (keys(%$rec)) {
            if (!$rec->{$_}) { $rec->{$_} = '-'; }
        }

        # Change 'time' to date
        if ($rec->{'time'} && $rec->{'time'} == $prevTime) {
            $rec->{'date'} = $prevDate;
        } elsif ($rec->{'time'}) {
            my $newDate = getDate("$rec->{'time'}");
            $rec->{'date'} = $newDate;
            $prevDate      = $newDate;
            $prevTime      = $rec->{'time'};
        } else {
            $rec->{'date'} = "0000-00-00-00:00:00";
        }

        if ($rec->{'severity'} && $rec->{'severity'} eq '-1') {
            $rec->{'severity'} = 'U';
        }

        delete($rec->{'time'});
        delete($rec->{'counter'});

        push(@events, $rec);
    }

    return \@events;
}

# Archived Reports Stuff -- Some of this would go away in an ideal world
################################################################################
sub getArchReport {
    my $args     = shift;
    my @rec      = ();
    my $eventRep = "/var/log/apparmor/reports/events.rpt";

    if ($args->{'logFile'}) {
        $eventRep = $args->{'logFile'};
    }

    if (open(REP, "<$eventRep")) {

        my $page = 1;

        if ($args->{'page'}) { $page = $args->{'page'}; }

        my $id    = 1;
        my $slurp = 0;

        my $prevTime = undef;
        my $prevDate = undef;

        while (<REP>) {

            my $db = ();

            # Why not get rid of page and just do divide by $i later?
            if (/Page/) {
                chomp;
                if ($_ eq "Page $page") {
                    $slurp = 1;
                } else {
                    $slurp = 0;
                }
            } elsif ($slurp == 1) {

                chomp;

				($db->{'host'},$db->{'time'},$db->{'prog'},$db->{'profile'},
				$db->{'pid'},$db->{'severity'},$db->{'mode_deny'},$db->{'mode_req'},
				$db->{'resource'},$db->{'sdmode'},$db->{'op'},$db->{'attr'},
				$db->{'name_alt'},$db->{'parent'},$db->{'active_hat'},
				$db->{'net_family'},$db->{'net_proto'},$db->{'net_socktype'}) 
				= split(/\,/, $_);

                # Convert epoch time to date
                if ($db->{'time'} == $prevTime) {
                    $db->{'date'} = $prevDate;
                } else {
                    $prevTime     = $db->{'time'};
                    $prevDate     = getDate("$db->{'time'}");
                    $db->{'date'} = $prevDate;
                }

                $id++;
                $db->{'date'} = $db->{'time'};
                delete $db->{'time'};
                push(@rec, $db);
            }
        }

        close REP;

    } else {
        ycp::y2error(sprintf(gettext("Fatal Error.  getArchReport() couldn't open %s"), $eventRep));
        return ("Couldn't open $eventRep");
    }

    return (\@rec);
}

sub writeEventReport {

    my ($db, $args) = @_;    # Filters for date, && regexp
    my $eventRep = "/var/log/apparmor/reports/events.rpt";

    if (open(REP, ">$eventRep")) {

        my $i    = 1;
        my $page = 1;
        my $skip = 0;

        # Title for scheduled reports
        if ($args->{'title'}) { print REP "$args->{'title'}"; }

        print REP "Page $page\n";
        $page++;

        for (@$db) {

			print REP "$_->{'host'},$_->{'time'},$_->{'prog'},$_->{'profile'},";
        	print REP "$_->{'pid'},$_->{'severity'},$_->{'mode_deny'},$_->{'mode_req'},";
		    print REP "$_->{'resource'},$_->{'sdmode'},$_->{'op'},$_->{'attr'},";
		    print REP "$_->{'name_alt'},$_->{'parent'},$_->{'active_hat'},";
		    print REP "$_->{'net_family'},$_->{'net_proto'},$_->{'net_socktype'}\n";

            if (($i % $numEvents) == 0 && $skip == 0) {
                print REP "Page $page\n";
                $page++;
                $skip = 1;
            } else {
                $i++;
                $skip = 0;
            }

        }

        close REP;

    } else {
        return ("Couldn't open $eventRep");
    }

    return 0;
}

sub prepSingleLog {
    my $args = shift;

    my $dir      = '/var/log/apparmor/reports-archived';
    my $error    = "0";
    my @errors   = ();                                             # For non-fatal errors
    my @repList  = ();
    my $readFile = "";
    my $eventRep = "/var/log/apparmor/reports/all-reports.rpt";    # write summary here 

    if ($args->{'logFile'}) { $readFile = $args->{'logFile'}; }
    if ($args->{'repPath'}) { $dir      = $args->{'repPath'}; }

    my @rawDb      = ();
    my $numPages   = 1;
    my $numRecords = 1;
    my $skip       = 0;

    # Open record compilation file
    if (open(RREP, "<$dir/$readFile")) {

        if (open(WREP, ">$eventRep")) {

            $numPages++;

            while (<RREP>) {

                next if (/Page/);
                next if /^#/;

                print WREP "$_";

                if (($numRecords % $numEvents) == 0 && $skip == 0) {
                    print WREP "Page $numPages\n";
                    $numPages++;
                    $skip = 1;
                } else {
                    $numRecords++;
                    $skip = 0;
                }

            }
            close WREP;
        } else {
            $error = "Problem in prepSingleLog() - couldn't open $eventRep.";
            return $error;
        }

        close RREP;

    } else {
        $error = "Problem in prepSingleLog() - couldn't open -$dir/$readFile-.";
        return $error;
    }

    return $error;
}

# Cats files in specified directory for easier parsing
sub prepArchivedLogs {
    my $args = shift;

    my $dir      = '/var/log/apparmor/reports-archived';
    my $error    = "0";
    my @errors   = ();                                            # For non-fatal errors
    my @repList  = ();
    my @db       = ();
    my $eventRep = "/var/log/apparmor/reports/all-reports.rpt";

    my $useFilters = 0;

    if ($args->{'logFile'}) {
        $eventRep = $args->{'logFile'};
    }

    if ($args->{'repPath'}) {
        $dir = $args->{'repPath'};
    }

    # Check to see if we need to use filters
    if ($args->{'mode_req'}
        && ($args->{'mode_req'} =~ /All/ || $args->{'mode_req'} =~ /^\s*-\s*$/))
    {
        delete($args->{'mode_req'});
    }
    if ($args->{'mode_deny'}
        && ($args->{'mode_deny'} =~ /All/ || $args->{'mode_deny'} =~ /^\s*-\s*$/))
    {
        delete($args->{'mode_deny'});
    }

    if ($args->{'sdmode'}
        && ($args->{'sdmode'} =~ /All/ || $args->{'sdmode'} =~ /^\s*-\s*$/))
    {
        delete($args->{'sdmode'});
    }
    if ($args->{'resource'}
        && ($args->{'resource'} =~ /All/ || $args->{'resource'} =~ /^\s*-\s*$/))
    {
        delete($args->{'resource'});
    }
    if ($args->{'severity'}
        && ($args->{'severity'} =~ /All/ || $args->{'severity'} =~ /^\s*-\s*$/))
    {
        delete($args->{'severity'});
    }

    my $regExp = 'prog|profile|pid|resource|mode|severity|date|op|target|attr|net_|name_alt';

    # get list of keys
    my @keyList = keys(%$args);

    # find filters in @keyList
    if ( grep(/$regExp/, @keyList) == 1 ) {
        $useFilters = 1;
    }

    ############################################################

    # Get list of files in archived report directory
    if (opendir(RDIR, $dir)) {

        my @firstPass = grep(/csv/, readdir(RDIR));
        @repList =
          grep(!/Applications.Audit|Executive.Security.Summary/, @firstPass);
        close RDIR;

    } else {
        $error = "Failure in prepArchivedLogs() - couldn't open $dir.";
        return ($error);    # debug - exit instead?
    }

    my @rawDb      = ();
    my $numPages   = 1;
    my $numRecords = 1;

    # Open record compilation file
    if (open(AREP, ">$eventRep")) {

        for (@repList) {

            my $file = $_;

            # Cycle through each $file in $dir
            if (open(RPT, "<$dir/$file")) {
                push(@rawDb, <RPT>);
                close RPT;
            } else {
                $error = "Problem in prepArchivedLogs() - couldn't open $dir/$file.";
                push(@errors, $error);
            }
        }

        # sort & store cat'd files
        if (@rawDb > 0) {

            # Run Filters
            if ($useFilters == 1) {

                my @tmpDb = parseMultiDb($args, @rawDb);
                @db = sort(@tmpDb);

            } else {
                @db = sort(@rawDb);
            }

            my $skip = 0;
            print AREP "Page $numPages\n";
            $numPages++;

            for (@db) {

                next if /^Page/;
                next if /^#/;

                print AREP "$_";

                if (($numRecords % $numEvents) == 0 && $skip == 0) {
                    print AREP "Page $numPages\n";
                    $numPages++;
                    $skip = 1;
                } else {
                    $numRecords++;
                    $skip = 0;
                }
            }

        } else {
            $error = "DB created from $dir is empty.";
        }

        close AREP;

    } else {
        $error = "Problem in prepArchivedLogs() - couldn't open $eventRep.";
        push(@errors, $error);
    }

    return $error;
}

# Similar to parseLog(), but expects @db to be passed
sub parseMultiDb {
    my ($args, @db) = @_;

    my @newDb = ();

    my $error     = undef;
    my $startDate = undef;
    my $endDate   = undef;

    # deref dates for speed
    if ($args->{'startdate'} && $args->{'enddate'}) {
        $startDate = getEpochFromNum("$args->{'startdate'}", 'start');
        $endDate   = getEpochFromNum("$args->{'enddate'}",   'end');
    }

    $args = rewriteModes($args);

    for (@db) {

        my $rec  = undef;
        my $line = $_;

        next if /true|false/;    # avoid horrible yast bug
        next if /^Page/;
        next if /^#/;
        chomp;
        next if (!$_ || $_ eq "");

        # Lazy filters -- maybe these should be with the rest below
        if ($args->{'prog'})    { next unless /$args->{'prog'}/; }
        if ($args->{'profile'}) { next unless /$args->{'profile'}/; }

        # Need (epoch) 'time' element here, do we want to store 'date' instead?
        ($rec->{'host'},$rec->{'time'},$rec->{'prog'},$rec->{'profile'},
        $rec->{'pid'},$rec->{'severity'},$rec->{'mode_deny'},$rec->{'mode_req'},
        $rec->{'resource'},$rec->{'sdmode'},$rec->{'op'},$rec->{'attr'},
        $rec->{'name_alt'},$rec->{'parent'},$rec->{'active_hat'},
        $rec->{'net_family'},$rec->{'net_proto'},$rec->{'net_socktype'})
        = split(/\,/, $_);


		# Get the time/date ref. name right.  If it's $args->"time",
		# the arg will be converted to a human-friendly "date" ref in writeEventReport().
        if ($rec->{'time'} =~ /\:|\-/) {
            $rec->{'date'} = $rec->{'time'};
            delete $rec->{'time'};
        }

        # Check filters
		next if matchFailed($args,$rec);

        push(@newDb, $line);

    }

    return @newDb;
}

# Grab & filter events from archived reports (.csv files)
sub parseLog {
    my $args = shift;

    my @db       = ();
    my $eventRep = "/var/log/apparmor/reports/events.rpt";

    if ($args->{'logFile'}) {
        $eventRep = $args->{'logFile'};
    }

    my $error     = undef;
    my $startDate = undef;
    my $endDate   = undef;

    # deref dates for speed
    if ($args->{'startdate'} && $args->{'enddate'}) {
        $startDate = getEpochFromNum("$args->{'startdate'}", 'start');
        $endDate   = getEpochFromNum("$args->{'enddate'}",   'end');
    }

    if ($args->{'mode_req'}
        && ($args->{'mode_req'} =~ /All/ || $args->{'mode_req'} =~ /^\s*-\s*$/))
    {
        delete($args->{'mode_req'});
    }

    if ($args->{'mode_deny'}
        && ($args->{'mode_deny'} =~ /All/ || $args->{'mode_deny'} =~ /^\s*-\s*$/))
    {
        delete($args->{'mode_deny'});
    }

    if ($args->{'sdmode'}
        && ($args->{'sdmode'} =~ /All/ || $args->{'sdmode'} =~ /^\s*-\s*$/))
    {
        delete($args->{'sdmode'});
    }
    if ($args->{'resource'}
        && ($args->{'resource'} =~ /All/ || $args->{'resource'} =~ /^\s*-\s*$/))
    {
        delete($args->{'resource'});
    }
    if ($args->{'severity'}
        && ($args->{'severity'} =~ /All/ || $args->{'severity'} =~ /^\s*-\s*$/))
    {
        delete($args->{'severity'});
    }

    $args = rewriteModes($args);

    if (open(LOG, "<$eventRep")) {

        # Log Parsing
        while (<LOG>) {

            my $rec = undef;

            next if /true|false/;    # avoid horrible yast bug
            next if /Page/;
            next if /^#/;
            chomp;
            next if (!$_ || $_ eq "");

            # Lazy filters -- maybe these should be with the rest below
            if ($args->{'prog'})    { next unless /$args->{'prog'}/; }
            if ($args->{'profile'}) { next unless /$args->{'profile'}/; }

            ($rec->{'host'}, $rec->{'time'}, $rec->{'prog'}, $rec->{'profile'},
			$rec->{'pid'}, $rec->{'severity'}, $rec->{'mode_req'}, $rec->{'resource'},
			$rec->{'sdmode'}) = split(/\,/, $_);

			# Get the time/date ref. name right.  If it's $args->{'time'}, the arg
			# will be converted to a human-friendly date ref in writeEventReport().
            if ($rec->{'time'} =~ /\:|\-/) {
                $rec->{'date'} = $rec->{'time'};
                delete $rec->{'time'};
            }

            # Check filters
			next if matchFailed($args,$rec);

            push(@db, $rec);

        }

        close LOG;

        # Export results to file if requested
        if ($args->{'exporttext'} || $args->{'exporthtml'}) {

            my $rawLog = undef;
            my $expLog = undef;

            if ($args->{'exportPath'}) {
                $rawLog = $args->{'exportPath'} . '/export-log';
            } else {
                $rawLog = '/var/log/apparmor/reports-exported/export-log';
            }

            if ($args->{'exporttext'} && $args->{'exporttext'} eq 'true') {
                $expLog = "$rawLog.csv";
                exportLog($expLog, \@db);    # redo w/ @$db instead of %db?
            }

            if ($args->{'exporthtml'} && $args->{'exporthtml'} eq 'true') {
                $expLog = "$rawLog.html";
                exportLog($expLog, \@db);    # redo w/ @$db instead of %db?
            }
        }

        $error = writeEventReport(\@db, $args);

    } else {
        $error = "Couldn't open $eventRep.";
    }

    return $error;
}

1;

