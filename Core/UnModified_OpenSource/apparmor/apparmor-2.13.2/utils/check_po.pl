#!/usr/bin/perl

# Look in the po directory and check the files for missing shortcuts - where the
# msgid has a string "(B)lah" and the msgstr contains no character surrounded by
# parens

use Data::Dumper;

my $dir = "./po";

opendir(PODIR, $dir) || die "Can't open  directory $dir.";
my $errors = {};
for my $file (grep { /.*\.po$/ && -f "$dir/$_" } readdir(PODIR)) {
    #print STDOUT "Processing $file\n";
    check_po_for_shortcuts( "$dir/$file", $errors );
    
}
my $msg = Data::Dumper->Dump([$errors], [qw(*ERRORS)]);
my $count = 0;
for my $f ( keys %$errors ) {
   for my $err ( keys %{$errors->{$f}} ) {
       $count++;
   }
} 
print STDOUT "$count missing shortcuts in the po files\n.$msg\n";
closedir(PODIR);


sub check_po_for_shortcuts {
    my ($filename, $errors)  = @_;
    my $line = 0;
    if (open(PO, "$filename")) {
        while (<PO>) {
            $line++;
            chomp;
            if ( /^.*msgid.*\(\w{1}?\)*/ ) {
              $looking_for_msgstr = 1;
              $msgid = $_;
            } 
            if ( /^.*msgstr*/ && $looking_for_msgstr ) {
                unless (/^.*msgstr.*\(\w{1}?\)*/ or /^msgstr ""$/) {
                    $errors->{$filename}{$line} =  {
                                             "msgid" => $msgid,
                                             "msgstr" => $_,
                                            };
                }
                $msgid = "";
                $looking_for_msgstr = 0;
             } 
        }
        close(PO);
    }
    return $config;
}
