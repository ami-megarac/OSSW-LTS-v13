#!./perl

# These tests are in a separate file, because they use fresh_perl_is()
# from test.pl.

# The mb* functions use the "underlying locale" that is not affected by
# the Perl one.  So we run the tests in a separate "fresh_perl" process
# with the correct LC_CTYPE set in the environment.

BEGIN {
    require Config; import Config;
    if ($^O ne 'VMS' and $Config{'extensions'} !~ /\bPOSIX\b/) {
	print "1..0\n";
	exit 0;
    }
    unshift @INC, "../../t";
    require 'loc_tools.pl';
    require 'test.pl';
}

plan tests => 3;

use POSIX qw();

SKIP: {
    skip("mblen() not present", 3) unless $Config{d_mblen};

    is(&POSIX::mblen("a", &POSIX::MB_CUR_MAX), 1, 'mblen() basically works');

    skip("LC_CTYPE locale support not available", 2)
      unless locales_enabled('LC_CTYPE');

    my $utf8_locale = find_utf8_ctype_locale();
    skip("no utf8 locale available", 2) unless $utf8_locale;

    local $ENV{LC_CTYPE} = $utf8_locale;
    local $ENV{LC_ALL};
    delete $ENV{LC_ALL};

    fresh_perl_is(
      'use POSIX; print &POSIX::mblen("\x{c3}\x{28}", &POSIX::MB_CUR_MAX)',
      -1, {}, 'mblen() recognizes invalid multibyte characters');

    fresh_perl_is(
     'use POSIX; print &POSIX::mblen("\N{GREEK SMALL LETTER SIGMA}", &POSIX::MB_CUR_MAX)',
     2, {}, 'mblen() works on UTF-8 characters');
}
