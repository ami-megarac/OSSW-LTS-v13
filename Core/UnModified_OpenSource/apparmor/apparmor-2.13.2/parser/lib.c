/*
 *   Copyright (c) 2012
 *   Canonical Ltd. (All rights reserved)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, contact Novell, Inc. or Canonical,
 *   Ltd.
 */

#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <cstdint>

#include <sys/apparmor_private.h>

#include "lib.h"
#include "parser.h"

int dirat_for_each(int dirfd, const char *name, void *data,
		   int (* cb)(int, const char *, struct stat *, void *))
{
	int retval = _aa_dirat_for_each(dirfd, name, data, cb);

	if (retval)
		PDEBUG("dirat_for_each failed: %m\n");

	return retval;
}

/**
 * isodigit - test if a character is an octal digit
 * @c: character to test
 *
 * Returns: true if an octal digit, else false
 */
int isodigit(char c)
{
	return (c >= '0' && c <= '7') ? true : false;
}

/* convert char character 0..9a..z into a number 0-35
 *
 * Returns: digit value of character or -1 if character is invalid
 */
static int chrtoi(char c, int base)
{
	int val = -1;

	if (base < 2 || base > 36)
		return -1;

	if (isdigit(c))
		val = c - '0';
	else if (isalpha(c) && isascii(c))
		val = tolower(c) - 'a' + 10;

	if (val >= base)
		return -1;

	return val;
}

/**
 * strntol - convert a sequence of characters as a hex number
 * @str: pointer to a string of character to convert
 * @endptr: RETURNS: if not NULL, the first char after converted chars.
 * @base: base of convertion
 * @maxval: maximum value. don't consume next char if value will exceed @maxval
 * @n: maximum number of characters to consume doing the conversion
 *
 * Returns: converted number. If there is no conversion 0 is returned and
 *          *@endptr = @str
 *
 * Not a complete replacement for strtol yet, Does not process base prefixes,
 * nor +/- sign yet.
 *
 * - take the largest sequence of character that is in range of 0-@maxval
 * - will consume the minimum of @maxlen or @base digits in @maxval
 * - if there is not n valid characters for the base only the n-1 will be taken
 *   eg. for the sequence string 4z with base 16 only 4 will be taken as the
 *   hex number
 */
long strntol(const char *str, const char **endptr, int base, long maxval,
	     size_t n)
{
	long c, val = 0;

	if (base > 1 && base < 37) {
		for (; n && (c = chrtoi(*str, base)) != -1; str++, n--) {
			long tmp = (val * base) + c;
			if (tmp > maxval)
				break;
			val = tmp;
		}
	}

	if (endptr)
		*endptr = str;

	return val;
}

/**
 * strn_escseq -
 * @pos: position of first character in esc sequence
 * @chrs: list of exact return chars to support eg. \+ returns + instead of -1
 * @n: maximum length of string to processes
 *
 * Returns: character for escape sequence or -1 if an error
 *
 * pos will point to first character after esc sequence
 * OR
 * pos will point to first character where an error was discovered
 * errors can be unrecognized esc character, octal, decimal, or hex
 * character encoding with no valid number. eg. \xT
 */
int strn_escseq(const char **pos, const char *chrs, size_t n)
{
	const char *end;
	long tmp;

	if (n < 1)
		return -1;

	if (isodigit(**pos)) {
		tmp = strntol(*pos, &end, 8, 255, min((size_t) 3, n));
		if (tmp == 0 && end == *pos) {
			/* this should never happen because of isodigit test */
			return -1;
		}
		*pos = end;
		return tmp;
	}

	char c = *(*pos)++;
	switch(c) {
	case '\\':
		return '\\';
	case '"':
		return '"';
	case 'd':
		tmp = strntol(*pos, &end, 10, 255, min((size_t) 3, n));
		if (tmp == 0 && end == *pos) {
			/* \d no valid encoding */
			return -1;
		}
		*pos = end;
		return tmp;
	case 'x':
		tmp = strntol(*pos, &end, 16, 255, min((size_t) 2, n));
		if (tmp == 0 && end == *pos) {
			/* \x no valid encoding */
			return -1;
		}
		*pos = end;
		return tmp;
	case 'a':
		return '\a';
	case 'e':
		return 033  /* ESC */;
	case 'f':
		return '\f';
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case 't':
		return '\t';
	}

	if (strchr(chrs, c))
		return c;

	/* unsupported escap sequence, backup to return that char */
	pos--;
	return -1;
}

int str_escseq(const char **pos, const char *chrs)
{
	/* no len limit just use end of string, yes could use strlen(pos) */
	return strn_escseq(pos, chrs, SIZE_MAX);
}

#ifdef UNIT_TEST

#include "lib.h"
#include "parser.h"
#include "unit_test.h"

static int test_oct(const char *str)
{
	const char *end;
	long retval = strntol(str, &end, 8, 255, 3);
	if (retval == 0 && str == end)
		return -1;
	return retval;
}

static int test_dec(const char *str)
{
	const char *end;
	long retval = strntol(str, &end, 10, 255, 3);
	if (retval == 0 && str == end)
		return -1;
	return retval;
}

static int test_hex(const char *str)
{
	const char *end;
	long retval = strntol(str, &end, 16, 255, 2);
	if (retval == 0 && str == end)
		return -1;
	return retval;
}

int main(void)
{
	int rc = 0;
	int retval;

	struct test_struct {
		const char *test;	/* test string */
		int expected;		/* expected result */
		const char *msg;	/* failure message */
	};

	struct test_struct oct_tests[] = {
		{ "0a", 0, "oct conversion of \\0a failed" },
		{ "00000003a", 0, "oct conversion of \\00000003a failed" },
		{ "62", 50, "oct conversion of \\62 failed" },
		{ "623", 50, "oct conversion of \\623 failed" },
		{ "123", 83, "oct conversion of \\123 failed" },
		{ "123;", 83, "oct conversion of \\123; failed" },
		{ "2234", 147, "oct conversion of \\2234 failed" },
		{ "xx", -1, "oct conversion of \\xx failed" },
		{ NULL, 0, NULL }
	};

	struct test_struct dec_tests[] = {
		{ "0a", 0, "dec conversion of \\d0a failed" },
		{ "00000003a", 0, "dec conversion of \\d00000003a failed" },
		{ "62", 62, "dec conversion of \\d62 failed" },
		{ "623", 62, "dec conversion of \\d623 failed" },
		{ "132", 132, "dec conversion of \\d132 failed" },
		{ "132UL", 132, "dec conversion of \\d132UL failed" },
		{ "255", 255, "dec conversion of \\d255 failed" },
		{ "256", 25, "dec conversion of \\d256 failed" },
		{ "2234", 223, "dec conversion of \\d2234 failed" },
		{ "xx", -1, "dec conversion of \\dxx failed" },
		{ NULL, 0, NULL }
	};

	struct test_struct hex_tests[] = {
		{ "0", 0x0, "hex conversion of 0x0 failed" },
		{ "0x1", 0x0, "hex conversion of 0x0x1 failed" },
		{ "1x", 0x1, "hex conversion of 0x1x failed" },
		{ "00", 0x0, "hex conversion of 0x00 failed" },
		{ "00x", 0x0, "hex conversion of 0x00x failed" },
		{ "01", 0x1, "hex conversion of 0x01 failed" },
		{ "01x", 0x1, "hex conversion of 0x01x failed" },
		{ "ab", 0xab, "hex conversion of 0xAb failed" },
		{ "AB", 0xab, "hex conversion of 0xAB failed" },
		{ "Ab", 0xab, "hex conversion of 0xAb failed" },
		{ "aB", 0xab, "hex conversion of 0xaB failed" },
		{ "4z", 0x4, "hex conversion of 0x4z failed" },
		{ "123", 0x12, "hex conversion of 0x123 failed" },
		{ "12M", 0x12, "hex conversion of 0x12M failed" },
		{ "ff", 0xff, "hex conversion of 0x255 failed" },
		{ "FF", 0xff, "hex conversion of 0x255 failed" },
		{ "XX", -1, "hex conversion of 0xXX failed" },
		{ NULL, 0, NULL }
	};

	struct test_struct escseq_tests[] = {
		{ "", -1, "escseq conversion of \"\" failed" },
		{ "0a", 0, "escseq oct conversion of \\0a failed" },
		{ "00000003a", 0, "escseq oct conversion of \\00000003a failed" },
		{ "62", 50, "escseq oct conversion of \\62 failed" },
		{ "623", 50, "escseq oct conversion of \\623 failed" },
		{ "123", 83, "escseq oct conversion of \\123 failed" },
		{ "123;", 83, "escseq oct conversion of \\123; failed" },
		{ "2234", 147, "escseq oct conversion of \\2234 failed" },
		{ "xx", -1, "escseq oct conversion of \\xx failed" },

		{ "d0a", 0, "escseq dec conversion of \\d0a failed" },
		{ "d00000003a", 0, "escseq dec conversion of \\d00000003a failed" },
		{ "d62", 62, "escseq dec conversion of \\d62 failed" },
		{ "d623", 62, "escseq dec conversion of \\d623 failed" },
		{ "d132", 132, "escseq dec conversion of \\d132 failed" },
		{ "d132UL", 132, "escseq dec conversion of \\d132UL failed" },
		{ "d255", 255, "escseq dec conversion of \\d255 failed" },
		{ "d256", 25, "escseq dec conversion of \\d256 failed" },
		{ "d2234", 223, "escseq dec conversion of \\d2234 failed" },
		{ "dxx", -1, "escseq dec conversion of \\dxx failed" },

		{ "x0", 0x0, "escseq hex conversion of 0x0 failed" },
		{ "x0x1", 0x0, "escseq hex conversion of 0x0x1 failed" },
		{ "x1x", 0x1, "escseq hex conversion of 0x1x failed" },
		{ "x00", 0x0, "escseq hex conversion of 0x00 failed" },
		{ "x00x", 0x0, "escseq hex conversion of 0x00x failed" },
		{ "x01", 0x1, "escseq hex conversion of 0x01 failed" },
		{ "x01x", 0x1, "escseq hex conversion of 0x01x failed" },
		{ "xab", 0xab, "escseq hex conversion of 0xAb failed" },
		{ "xAB", 0xab, "escseq hex conversion of 0xAB failed" },
		{ "xAb", 0xab, "escseq hex conversion of 0xAb failed" },
		{ "xaB", 0xab, "escseq hex conversion of 0xaB failed" },
		{ "x4z", 0x4, "escseq hex conversion of 0x4z failed" },
		{ "x123", 0x12, "escseq hex conversion of 0x123 failed" },
		{ "x12M", 0x12, "escseq hex conversion of 0x12M failed" },
		{ "xff", 0xff, "escseq hex conversion of 0x255 failed" },
		{ "xFF", 0xff, "escseq hex conversion of 0x255 failed" },
		{ "xXX", -1, "escseq hex conversion of 0xXX failed" },

		{ "\\", '\\', "escseq '\\\\' failed" },
		{ "\"", '"', "escseq  '\\\"' failed" },
		{ "a", '\a', "escseq '\\a' failed" },
		{ "e", '\033', "escseq '\\e' failed" },
		{ "f", '\f', "escseq '\\f' failed" },
		{ "n", '\n', "escseq '\\n' failed" },
		{ "r", '\r', "escseq '\\r' failed" },
		{ "t", '\t', "escseq '\\t' failed" },
		{ NULL, 0, NULL }
	};

	struct test_struct escseqextra_tests[] = {
		{ "+", '+', "escseq extra conversion of \\+ failed" },
		{ "-", '-', "escseq conversion of \\- failed" },
		{ "*", '*', "escseq conversion of \\* failed" },
		{ "(", '(', "escseq conversion of \\( failed" },
		{ ")", ')', "escseq conversion of \\) failed" },
		{ "|", '|', "escseq conversion of \\| failed" },
		{ ".", '.', "escseq conversion of \\. failed" },
		{ "[", '[', "escseq conversion of \\[ failed" },
		{ "]", ']', "escseq conversion of \\] failed" },
		{ "^", '^', "escseq conversion of \\^ failed" },
		{ NULL, 0, NULL }
	};

	/* test chrtoi */
	for (int base = -1; base < 38; base++) {
		for (int c = 0; c < 256; c++) {
			int expected;
			int i = chrtoi(c, base);
			if (base < 2 || base > 36 || !isascii(c) || !(isdigit(c) || isalpha(c)))
				expected = -1;
			else if (isdigit(c) && (c - '0') < base)
				expected = c - '0';
			else if (isalpha(c) && (toupper(c) - 'A') + 10 < base)
				expected = (toupper(c) - 'A') + 10;
			else
				expected = -1;
			if (i != expected)
//				printf("   chrtoi test: convert base %d '%c'(%d)\texpected %d\tresult: %d\n", base, c, c, expected, i);
			MY_TEST(i == expected, "failed");
		}
	}

	/* test strntol */
	for (struct test_struct *t = oct_tests; t->test; t++) {
		retval = test_oct(t->test);
//		printf("  oct test: %s\texpected %d\tresult: %d\n", t->test, t->expected, retval);
		MY_TEST(retval == t->expected, t->msg);
	}

	for (struct test_struct *t = dec_tests; t->test; t++) {
		retval = test_dec(t->test);
//		printf("  dec test: %s\texpected %d\tresult: %d\n", t->test, t->expected, retval);
		MY_TEST(retval == t->expected, t->msg);
	}

	for (struct test_struct *t = hex_tests; t->test; t++) {
		retval = test_hex(t->test);
//		printf("  hex test: %s\texpected %d\tresult: %d\n", t->test, t->expected, retval);
		MY_TEST(retval == t->expected, t->msg);
	}

	/* test strn_escseq */
	for (struct test_struct *t = escseq_tests; t->test; t++) {
		const char *pos = t->test;
		retval = strn_escseq(&pos, "", strlen(t->test));
//		printf("  strn_escseq test: %s\texpected %d\tresult: %d\n", t->test, t->expected, retval);
		MY_TEST(retval == t->expected, t->msg);
	}

	for (struct test_struct *t = escseqextra_tests; t->test; t++) {
		const char *pos = t->test;
		retval = strn_escseq(&pos, "", strlen(t->test));
//		printf("  strn_escseq test: %s\texpected %d\tresult: %d\n", t->test, t->expected, retval);
		MY_TEST(retval == -1, t->msg);
		pos = t->test;
		retval = strn_escseq(&pos, "*+.|^-[]()", strlen(t->test));
		MY_TEST(retval == t->expected, t->msg);
	}

	for (int c = 1; c < 256; c++) {
		const char *pos;
		char str[2] = " ";
		if (strchr("01234567\\\"dxaefnrt", c))
			/* skip chars already tested above */
			continue;
		str[0] = c;
		pos = str;
		retval = strn_escseq(&pos, "", 2);
		MY_TEST(retval == -1, "  strn_escseq: of unsupported char failed");
	}

	return rc;
}

#endif /* UNIT_TEST */
