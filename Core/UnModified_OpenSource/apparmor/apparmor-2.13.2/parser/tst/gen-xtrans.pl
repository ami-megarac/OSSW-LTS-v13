#!/usr/bin/perl

use strict;
use Locale::gettext;
use POSIX;

setlocale(LC_MESSAGES, "");

my $prefix="simple_tests/generated_x";
my $prefix_leading="simple_tests/generated_perms_leading";
my $prefix_safe="simple_tests/generated_perms_safe";

my @trans_types = ("p", "P", "c", "C", "u", "i");
my @modifiers = ("i", "u");
my %trans_modifiers = (
    "p" => \@modifiers,
    "P" => \@modifiers,
    "c" => \@modifiers,
    "C" => \@modifiers,
    );

my @targets = ("", "target", "target2");
my @null_target = ("");

my %named_trans = (
    "p" => \@targets,
    "P" => \@targets,
    "c" => \@targets,
    "C" => \@targets,
    "u" => \@null_target,
    "i" => \@null_target,
    );

my %safe_map = (
    "p" => "unsafe",
    "P" => "safe",
    "c" => "unsafe",
    "C" => "safe",
    "u" => "",
    "i" => "",
    );

my %invert_safe = (
    "safe" => "unsafe",
    "unsafe" => "safe",
    );

# audit qualifier disabled for now it really shouldn't affect the conflict
# test but it may be worth checking every once in awhile
#my @qualifiers = ("", "owner", "audit", "audit owner");
my @qualifiers = ("", "owner");

my $count = 0;

gen_conflicting_x();
gen_overlap_re_exact();
gen_dominate_re_re();
gen_ambiguous_re_re();
gen_leading_perms("exact", "/bin/cat", "/bin/cat");
gen_leading_perms("exact-re", "/bin/*", "/bin/*");
gen_leading_perms("overlap", "/*", "/bin/cat");
gen_leading_perms("dominate", "/**", "/*");
gen_leading_perms("ambiguous", "/a*", "/*b");
gen_safe_perms("exact", "PASS", "", "/bin/cat", "/bin/cat");
gen_safe_perms("exact-re", "PASS", "", "/bin/*", "/bin/*");
gen_safe_perms("overlap", "PASS", "", "/*", "/bin/cat");
gen_safe_perms("dominate", "PASS", "", "/**", "/*");
gen_safe_perms("ambiguous", "PASS", "", "/a*", "/*b");
gen_safe_perms("exact", "FAIL", "inv", "/bin/cat", "/bin/cat");
gen_safe_perms("exact-re", "FAIL", "inv", "/bin/*", "/bin/*");
gen_safe_perms("overlap", "PASS", "inv", "/*", "/bin/cat");
gen_safe_perms("dominate", "FAIL", "inv", "/**", "/*");
gen_safe_perms("ambiguous", "FAIL", "inv", "/a*", "/*b");

print "Generated $count xtransition interaction tests\n";

sub gen_list {
    my @output;
    foreach my $trans (@trans_types) {
	if ($trans_modifiers{$trans}) {
	    foreach my $mod (@{$trans_modifiers{$trans}}) {
		push @output, "${trans}${mod}x";
	    }
	}
	push @output, "${trans}x";
    }
    return @output;
}

sub print_rule($$$$$$) {
    my ($file, $leading, $qual, $name, $perm, $target) = @_;
    if ($leading) {
	print $file "\t${qual} ${perm} ${name}";
    } else {
	print $file "\t${qual} ${name} ${perm}";
    }
    if ($target ne "") {
	print $file " -> $target";
    }
    print $file ",\n";
}

sub gen_file($$$$$$$$$$$$) {
    my ($name, $xres, $leading1, $qual1, $rule1, $perm1, $target1, $leading2, $qual2, $rule2, $perm2, $target2) = @_;

#    print "$xres $rule1 $perm1 $target1 $rule2 $perm2 $target2\n";

    my $file;
    unless (open $file, ">$name") {
	print("couldn't open $name\n");
	exit 1;
    }

    print $file "#\n";
    print $file "#=DESCRIPTION ${name}\n";
    print $file "#=EXRESULT ${xres}\n";
    print $file "#\n";
    print $file "/usr/bin/foo {\n";
    print_rule($file, $leading1, $qual1, $rule1, $perm1, $target1);
    print_rule($file, $leading2, $qual2, $rule2, $perm2, $target2);
    print $file "}\n";
    close($file);

    $count++;
}

#NOTE: currently we don't do px to cx, or cx to px conversion
#      so
# /foo {
#    /* px -> /foo//bar,
#    /* cx -> bar,
#
# will conflict
#
#NOTE: conflict tests don't tests leading permissions or using unsafe keywords
#      It is assumed that there are extra tests to verify 1 to 1 coorispondance
sub gen_files($$$$) {
    my ($name, $rule1, $rule2, $default) = @_;

    my @perms = gen_list();

#    print "@perms\n";

    foreach my $i (@perms) {
	foreach my $t (@{$named_trans{substr($i, 0, 1)}}) {
	    foreach my $q (@qualifiers) {
		foreach my $j (@perms) {
		    foreach my $u (@{$named_trans{substr($j, 0, 1)}}) {
			foreach my $r (@qualifiers) {
			    my $file="${prefix}/${name}-$q$i$t-$r$j$u.sd";
#		    print "$file\n";

		    #override failures when transitions are the same
			    my $xres = ${default};
			    if ($i eq $j && $t eq $u) {
				$xres = "PASS";
			    }


#		    print "foo $xres $rule1 $i $t $rule2 $j $u\n";
			    gen_file($file, $xres, 0, $q, $rule1, $i, $t, 0, $r, $rule2, $j, $u);
			}
		    }
		}
	    }
	}
    }

}

sub gen_conflicting_x {
    gen_files("conflict", "/bin/cat", "/bin/cat", "FAIL");
}

sub gen_overlap_re_exact {

    gen_files("exact", "/bin/cat", "/bin/*", "PASS");
}

# we currently don't support this, once supported change to "PASS"
sub gen_dominate_re_re {
    gen_files("dominate", "/bin/*", "/bin/**", "FAIL");
}

sub gen_ambiguous_re_re {
    gen_files("ambiguous", "/bin/a*", "/bin/*b", "FAIL");
}


# test that rules that lead with permissions don't conflict with
# the same rule using trailing permissions.
sub gen_leading_perms($$$) {
    my ($name, $rule1, $rule2) = @_;

    my @perms = gen_list();

    foreach my $i (@perms) {
	foreach my $t (@{$named_trans{substr($i, 0, 1)}}) {
	    foreach my $q (@qualifiers) {
		my $file="${prefix_leading}/${name}-$q$i$t.sd";
#		    print "$file\n";

		gen_file($file, "PASS", 0, $q, $rule1, $i, $t, 1, $q, $rule2, $i, $t);
	    }
	}
    }
}

# test for rules with leading safe or unsafe keywords.
# check they are equivalent to their counter part,
# or if $invert that they properly conflict with their counterpart
sub gen_safe_perms($$$$$) {
    my ($name, $xres, $invert, $rule1, $rule2) = @_;

    my @perms = gen_list();

    foreach my $i (@perms) {
	foreach my $t (@{$named_trans{substr($i, 0, 1)}}) {
	    foreach my $q (@qualifiers) {
		my $qual = $safe_map{substr($i, 0, 1)};
		if ($invert) {
		    $qual = $invert_safe{$qual};
		}
		if (! $invert || $qual) {
		    my $file="${prefix_safe}/${name}-$invert-$q${qual}-rule-$i$t.sd";
#		    print "$file\n";
		    gen_file($file, $xres, 0, "$q $qual", $rule1, $i, $t, 1, $q, $rule2, $i, $t);
		    $file="${prefix_safe}/${name}-$invert-$q$qual${i}-rule-$t.sd";
		    gen_file($file, $xres, 0, $q, $rule1, $i, $t, 1, "$q $qual", $rule2, $i, $t);
		}

	    }
	}
    }
}
