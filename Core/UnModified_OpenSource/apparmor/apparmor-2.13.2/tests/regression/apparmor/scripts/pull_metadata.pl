#!/usr/bin/perl
#
# Pulls metadata out of the .sh files and writes it to a file
#
$i = 1;

open (WRITEFILE, ">metadata.txt");
opendir (TESTDIR, ".") or die "Could not open directory";
while (defined ($file = readdir(TESTDIR))) {
	if ($file =~ /\.sh$/) {	
		push @file_array, $file;
	}
}
closedir(TESTDIR);
@file_array = sort(@file_array);

$in_description = 0;
foreach $file (@file_array) {
		open (SCRIPT, $file);
		print "Handling: $file\n";
		while (<SCRIPT>) {
			if (/^#=NAME\s(.*)/) {
				print "Found #=NAME in $file\n";
				print WRITEFILE "\n$i\. $1\n\n";
				$i++;
				$in_description = 0;
			} elsif (/^#=DESCRIPTION\s+(\S.*)\s*$/) {
				print WRITEFILE "$1\n";
			} elsif (/^#=DESCRIPTION\s*$/) {
				$in_description = 1;
			} elsif ($in_description && /^#=END\s*$/) {
				$in_description = 0;
			} elsif ($in_description && /^#\s(.+)$/) {
				print WRITEFILE "$1\n";
			}
		}
		close(SCRIPT);
}
close(WRITEFILE);
	

