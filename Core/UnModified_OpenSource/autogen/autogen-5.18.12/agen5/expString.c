
/**
 * @file expString.c
 *
 *  This module implements expression functions that
 *  manipulate string values.
 *
 * @addtogroup autogen
 * @{
 */
/*
 *  This file is part of AutoGen.
 *  AutoGen Copyright (C) 1992-2016 by Bruce Korb - all rights reserved
 *
 * AutoGen is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AutoGen is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* = = = START-STATIC-FORWARD = = = */
static size_t
stringify_for_sh(char * pzNew, uint_t qt, char const * pzDta);

static SCM
shell_stringify(SCM obj, uint_t qt);

static int
sub_count(char const * haystack, char const * needle);

static void
do_substitution(
    char const * src_str,
    ssize_t      str_len,
    SCM          match,
    SCM          repl,
    char **      ppz_res,
    ssize_t *    res_len);

static inline void
tr_char_range(unsigned char * ch_map, unsigned char * from, unsigned char * to);
/* = = = END-STATIC-FORWARD = = = */

static size_t
stringify_for_sh(char * pzNew, uint_t qt, char const * pzDta)
{
    char * pz = pzNew;
    *(pz++) = (char)qt;

    for (;;) {
        char c = *(pz++) = *(pzDta++);
        switch (c) {
        case NUL:
            pz[-1]  = (char)qt;
            *pz     = NUL;

            return (size_t)(pz - pzNew);

        case '\\':
            /*
             *  If someone went to the trouble to escape a backquote or a
             *  dollar sign, then we should not neutralize it.  Note that
             *  we handle a following backslash as a normal character.
             *
             *  i.e.  \\ --> \\\\ *BUT* \\$ --> \\\$
             */
            c = *pzDta;
            switch (*pzDta) {
            case '$':
                break;

            case '"':
            case '`':
                /*
                 *  IF the ensuing quote character does *NOT* match the
                 *  quote character for the string, then we will preserve
                 *  the single copy of the backslash.  If it does match,
                 *  then we will double the backslash and a third backslash
                 *  will be inserted when we emit the quote character.
                 */
                if ((unsigned)c != qt)
                    break;
                /* FALLTHROUGH */

            default:
                *(pz++) = '\\';   /* \   -->  \\    */
            }
            break;

        case '"': case '`':
            if ((unsigned)c == qt) {
                /*
                 *  This routine does both `xx` and "xx" strings, we have
                 *  to worry about this stuff differently.  I.e., in ""
                 *  strings, add a single \ in front of ", and in ``
                 *  preserve a add \ in front of `.
                 */
                pz[-1]  = '\\';       /* "   -->   \"   */
                *(pz++) = c;
            }
        }
    }
}

static SCM
shell_stringify(SCM obj, uint_t qt)
{
    char * pzNew;
    size_t dtaSize = 3;
    char * pzDta   = ag_scm2zchars(obj, "AG Object");
    char * pz      = pzDta;

    for (;;) {
        char c = *(pz++);

        switch (c) {
        case NUL:
            goto loopDone1;

        case '"': case '`': case '\\':
            dtaSize += 2;
            break;

        default:
            dtaSize++;
        }
    } loopDone1:;

    pzNew = AGALOC(dtaSize, "shell string");
    dtaSize = stringify_for_sh(pzNew, qt, pzDta);

    {
        SCM res = scm_from_latin1_stringn(pzNew, dtaSize);
        AGFREE(pzNew);
        return res;
    }
}

static int
sub_count(char const * haystack, char const * needle)
{
    int repCt = 0;
    size_t needle_len = strlen(needle);

    for (;;) {
        haystack = strstr(haystack, needle);
        if (haystack == NULL) break;
        repCt++;
        haystack += needle_len;
    }
    return repCt;
}

/**
 *  Replace marker text.
 *
 *  Replace all occurrances of the marker text with the substitution text.
 *  The result is stored in an automatically freed temporary buffer.
 *
 *  @param src_str  The source string
 *  @param str_len  The length of the string
 *  @param match    the SCM-ized marker string
 *  @param repl     the SCM-ized replacement string
 *  @param ppz_res  pointer to the result pointer
 *  @param res_len  pointer to result length
 */
static void
do_substitution(
    char const * src_str,
    ssize_t      str_len,
    SCM          match,
    SCM          repl,
    char **      ppz_res,
    ssize_t *    res_len)
{
    char * pzMatch  = ag_scm2zchars(match, "match text");
    char * rep_str  = ag_scm2zchars(repl,  "repl text");
    int    mark_len = (int)scm_c_string_length(match);
    int    repl_len = (int)scm_c_string_length(repl);

    {
        int ct = sub_count(src_str, pzMatch);
        if (ct == 0)
            return; /* No substitutions -- no work. */

        str_len += (repl_len - mark_len) * ct;
    }

    {
        char * dest = scribble_get(str_len + 1);
        *ppz_res = dest;
        *res_len = str_len;

        for (;;) {
            char const * next = strstr(src_str, pzMatch);
            size_t len;

            if (next == NULL)
                break;
            len = (size_t)(next - src_str);
            if (len != 0) {
                memcpy(dest, src_str, len);
                dest += len;
            }
            memcpy(dest, rep_str, (size_t)repl_len);
            dest   += repl_len;
            src_str = next + mark_len;
        }

        strcpy(dest, src_str);
    }
}


/*
 *  Recursive routine.  It calls itself for list values and calls
 *  "do_substitution" for string values.  Each substitution will
 *  be done in the order found in the tree walk of list values.
 *  The "match" and "repl" trees *must* be identical in structure.
 */
LOCAL void
do_multi_subs(char ** ppzStr, ssize_t * pStrLen, SCM match, SCM repl)
{
    char * pzStr = *ppzStr;
    char * pzNxt = pzStr;

    /*
     *  Loop for as long as our list has more entries
     */
    while (! scm_is_null(match)) {
        /*
         *  "CAR" is the current value, "CDR" is rest of list
         */
        SCM  matchCar  = SCM_CAR(match);
        SCM  replCar   = SCM_CAR(repl);

        match = SCM_CDR(match);
        repl  = SCM_CDR(repl);

        if (scm_is_string(matchCar)) {
            do_substitution(pzStr, *pStrLen, matchCar, replCar,
                            &pzNxt, pStrLen);

            // coverity[use_after_free] -- invalid alias analysis
            pzStr = pzNxt;
        }

        else if (AG_SCM_LIST_P(matchCar))
            do_multi_subs(&pzStr, pStrLen, matchCar, replCar);

        else
            /*
             *  Whatever it is it is not part of what we would expect.  Bail.
             */
            break;
    }

    *ppzStr = pzStr;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  EXPRESSION EVALUATION ROUTINES
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc mk_gettextable
 *
 * what:   print a string in a gettext-able format
 * exparg: string, a multi-paragraph string
 *
 * doc: Returns SCM_UNDEFINED.  The input text string is printed
 *      to the current output as one puts() call per paragraph.
=*/
SCM
ag_scm_mk_gettextable(SCM txt)
{
    if (scm_is_string(txt)) {
        char const * pz = ag_scm2zchars(txt, "txt");
        optionPrintParagraphs(pz, false, cur_fpstack->stk_fp);
    }
    return SCM_UNDEFINED;
}

/*=gfunc in_p
 *
 * what:   test for string in list
 * general_use:
 * exparg: test-string, string to look for
 * exparg: string-list, list of strings to check,, list
 *
 * doc:  Return SCM_BOOL_T if the first argument string is found
 *      in one of the entries in the second (list-of-strings) argument.
=*/
SCM
ag_scm_in_p(SCM obj, SCM list)
{
    int     len;
    size_t  lenz;
    SCM     car;
    char const * pz1;

    if (! scm_is_string(obj))
        return SCM_UNDEFINED;

    pz1  = scm_i_string_chars(obj);
    lenz = scm_c_string_length(obj);

    /*
     *  If the second argument is a string somehow, then treat
     *  this as a straight out string comparison
     */
    if (scm_is_string(list)) {
        if (  (scm_c_string_length(list) == lenz)
           && (strncmp(pz1, scm_i_string_chars(list), lenz) == 0))
            return SCM_BOOL_T;
        return SCM_BOOL_F;
    }

    len = (int)scm_ilength(list);
    if (len == 0)
        return SCM_BOOL_F;

    /*
     *  Search all the lists and sub-lists passed in
     */
    while (len-- > 0) {
        car  = SCM_CAR(list);
        list = SCM_CDR(list);

        /*
         *  This routine is listed as getting a list as the second
         *  argument.  That means that if someone builds a list and
         *  hands it to us, it magically becomes a nested list.
         *  This unravels that.
         */
        if (! scm_is_string(car)) {
            if (ag_scm_in_p(obj, car) == SCM_BOOL_T)
                return SCM_BOOL_T;
            continue;
        }

        if (  (scm_c_string_length(car) == lenz)
           && (strncmp(pz1, scm_i_string_chars(car), lenz) == 0) )
            return SCM_BOOL_T;
    }

    return SCM_BOOL_F;
}


/*=gfunc join
 *
 * what:   join string list with separator
 * general_use:
 * exparg: separator, string to insert between entries
 * exparg: list, list of strings to join,, list
 *
 * doc:  With the first argument as the separator string,
 *       joins together an a-list of strings into one long string.
 *       The list may contain nested lists, partly because you
 *       cannot always control that.
=*/
SCM
ag_scm_join(SCM sep, SCM list)
{
    int      l_len, sv_l_len;
    SCM      car;
    SCM      alist = list;
    size_t   sep_len;
    size_t   str_len;
    char *   pzRes;
    char const * pzSep;
    char *   pzScan;

    if (! scm_is_string(sep))
        return SCM_UNDEFINED;

    sv_l_len = l_len = (int)scm_ilength(list);
    if (l_len == 0)
        return scm_from_latin1_string(zNil);

    pzSep   = scm_i_string_chars(sep);
    sep_len = scm_c_string_length(sep);
    str_len = 0;

    /*
     *  Count up the lengths of all the strings to be joined.
     */
    for (;;) {
        car  = SCM_CAR(list);
        list = SCM_CDR(list);

        /*
         *  This routine is listed as getting a list as the second
         *  argument.  That means that if someone builds a list and
         *  hands it to us, it magically becomes a nested list.
         *  This unravels that.
         */
        if (! scm_is_string(car)) {
            if (car != SCM_UNDEFINED)
                car = ag_scm_join(sep, car);
            if (! scm_is_string(car))
                return SCM_UNDEFINED;
        }

        str_len += scm_c_string_length(car);

        if (--l_len <= 0)
            break;

        str_len += sep_len;
    }

    l_len = sv_l_len;
    pzRes = pzScan = scribble_get((ssize_t)str_len);

    /*
     *  Now, copy each one into the output
     */
    for (;;) {
        size_t cpy_len;

        car   = SCM_CAR(alist);
        alist = SCM_CDR(alist);

        /*
         *  This unravels nested lists.
         */
        if (! scm_is_string(car))
            car = ag_scm_join(sep, car);

        cpy_len = scm_c_string_length(car);
        memcpy(VOIDP(pzScan), scm_i_string_chars(car), cpy_len);
        pzScan += cpy_len;

        /*
         *  IF we reach zero, then do not insert a separation and bail out
         */
        if (--l_len <= 0)
            break;
        memcpy(VOIDP(pzScan), VOIDP(pzSep), sep_len);
        pzScan += sep_len;
    }

    return scm_from_latin1_stringn(pzRes, str_len);
}


/*=gfunc prefix
 *
 * what:  prefix lines with a string
 * general_use:
 *
 * exparg: prefix, string to insert at start of each line
 * exparg: text, multi-line block of text
 *
 * doc:
 *  Prefix every line in the second string with the first string.
 *  This includes empty lines, though trailing white space will
 *  be removed if the line consists only of the "prefix".
 *  Also, if the last character is a newline, then *two* prefixes will
 *  be inserted into the result text.
 *
 *  For example, if the first string is "# " and the second contains:
 *  @example
 *  "two\nlines\n"
 *  @end example
 *  @noindent
 *  The result string will contain:
 *  @example
 *  # two
 *  # lines
 *  #
 *  @end example
 *
 *  The last line will be incomplete:  no newline and no space after the
 *  hash character, either.
=*/
SCM
ag_scm_prefix(SCM prefx, SCM txt)
{
    char *   prefix   = ag_scm2zchars(prefx, "pfx");
    char *   text     = ag_scm2zchars(txt,   "txt");
    char *   scan     = text;
    size_t   pfx_size = strlen(prefix);
    char *   r_str;   /* result string */

    {
        size_t out_size = pfx_size + 1; // NUL or NL byte adjustment
        for (;;) {
            switch (*(scan++)) {
            case NUL:
                out_size += scan - text;
                goto exit_count;
            case NL:
                out_size += pfx_size;
            }
        } exit_count:;

        r_str = scan = scribble_get((ssize_t)out_size);
    }

    memcpy(scan, prefix, pfx_size);
    scan += pfx_size;
    pfx_size++;

    for (;;) {
        char ch = *(text++);
        switch (ch) {
        case NUL:
            /*
             * Trim trailing white space on the final line.
             */
            scan = SPN_HORIZ_WHITE_BACK(r_str, scan);
            return scm_from_latin1_stringn(r_str, scan - r_str);

        case NL:
            /*
             * Trim trailing white space on previous line first.
             */
            scan  = SPN_HORIZ_WHITE_BACK(r_str, scan);
            *scan = NL;
            memcpy(scan+1, prefix, pfx_size - 1);
            scan += pfx_size;  // prefix length plus 1 for new line
            break;

        default:
            *(scan++) = ch;
            break;
        }
    }
}

/*=gfunc raw_shell_str
 *
 * what:  single quote shell string
 * general_use:
 *
 * exparg: string, string to transform
 *
 * doc:
 *  Convert the text of the string into a singly quoted string
 *  that a normal shell will process into the original string.
 *  (It will not do macro expansion later, either.)
 *  Contained single quotes become tripled, with the middle quote
 *  escaped with a backslash.  Normal shells will reconstitute the
 *  original string.
 *
 *  @strong{Notice}:  some shells will not correctly handle unusual
 *  non-printing characters.  This routine works for most reasonably
 *  conventional ASCII strings.
=*/
SCM
ag_scm_raw_shell_str(SCM obj)
{
    char * data;
    char * pz;
    char * pzFree;

    data = ag_scm2zchars(obj, "AG Object");

    {
        size_t dtaSize = scm_c_string_length(obj) + 3; /* NUL + 2 quotes */
        pz = data-1;
        for (;;) {
            pz = strchr(pz+1, '\'');
            if (pz == NULL)
                break;
            dtaSize += 3; /* '\'' -> 3 additional chars */
        }

        pzFree = pz = AGALOC(dtaSize + 2, "raw string");
    }

    /*
     *  Handle leading single quotes before starting the first quote.
     */
    while (*data == '\'') {
        *(pz++) = '\\';
        *(pz++) = '\'';

        /*
         *  IF pure single quotes, then we're done.
         */
        if (*++data == NUL) {
            *pz = NUL;
            goto returnString;
        }
    }

    /*
     *  Start quoting.  If the string is empty, we wind up with two quotes.
     */
    *(pz++) = '\'';

    for (;;) {
        switch (*(pz++) = *(data++)) {
        case NUL:
            goto loopDone;

        case '\'':
            /*
             *  We've inserted a single quote, which ends the quoting session.
             *  Now, insert escaped quotes for every quote char we find, then
             *  restart the quoting.
             */
            data--;
            do {
                *(pz++) = '\\';
                *(pz++) = '\'';
            } while (*++data == '\'');
            if (*data == NUL) {
                *pz = NUL;
                goto returnString;
            }
            *(pz++) = '\'';
        }
    } loopDone:;
    pz[-1] = '\'';
    *pz    = NUL;

 returnString:
    {
        SCM res = scm_from_latin1_string(pzFree);
        AGFREE(pzFree);
        return res;
    }
}


/*=gfunc shell_str
 *
 * what:  double quote shell string
 * general_use:
 *
 * exparg: string, string to transform
 *
 * doc:
 *
 *  Convert the text of the string into a double quoted string that a normal
 *  shell will process into the original string, almost.  It will add the
 *  escape character @code{\\} before two special characters to
 *  accomplish this: the backslash @code{\\} and double quote @code{"}.
 *
 *  @strong{Notice}: some shells will not correctly handle unusual
 *  non-printing characters.  This routine works for most reasonably
 *  conventional ASCII strings.
 *
 *  @strong{WARNING}:
 *@*
 *  This function omits the extra backslash in front of a backslash, however,
 *  if it is followed by either a backquote or a dollar sign.  It must do this
 *  because otherwise it would be impossible to protect the dollar sign or
 *  backquote from shell evaluation.  Consequently, it is not possible to
 *  render the strings "\\$" or "\\`".  The lesser of two evils.
 *
 *  All others characters are copied directly into the output.
 *
 *  The @code{sub-shell-str} variation of this routine behaves identically,
 *  except that the extra backslash is omitted in front of @code{"} instead
 *  of @code{`}.  You have to think about it.  I'm open to suggestions.
 *
 *  Meanwhile, the best way to document is with a detailed output example.
 *  If the backslashes make it through the text processing correctly,
 *  below you will see what happens with three example strings.  The first
 *  example string contains a list of quoted @code{foo}s, the second is
 *  the same with a single backslash before the quote characters and the
 *  last is with two backslash escapes.  Below each is the result of the
 *  @code{raw-shell-str}, @code{shell-str} and @code{sub-shell-str} functions.
 *
 *  @example
 *  foo[0]           ''foo'' 'foo' "foo" `foo` $foo
 *  raw-shell-str -> \'\''foo'\'\'' '\''foo'\'' "foo" `foo` $foo'
 *  shell-str     -> "''foo'' 'foo' \"foo\" `foo` $foo"
 *  sub-shell-str -> `''foo'' 'foo' "foo" \`foo\` $foo`
 *
 *  foo[1]           \'bar\' \"bar\" \`bar\` \$bar
 *  raw-shell-str -> '\'\''bar\'\'' \"bar\" \`bar\` \$bar'
 *  shell-str     -> "\\'bar\\' \\\"bar\\\" \`bar\` \$bar"
 *  sub-shell-str -> `\\'bar\\' \"bar\" \\\`bar\\\` \$bar`
 *
 *  foo[2]           \\'BAZ\\' \\"BAZ\\" \\`BAZ\\` \\$BAZ
 *  raw-shell-str -> '\\'\''BAZ\\'\'' \\"BAZ\\" \\`BAZ\\` \\$BAZ'
 *  shell-str     -> "\\\\'BAZ\\\\' \\\\\"BAZ\\\\\" \\\`BAZ\\\` \\\$BAZ"
 *  sub-shell-str -> `\\\\'BAZ\\\\' \\\"BAZ\\\" \\\\\`BAZ\\\\\` \\\$BAZ`
 *  @end example
 *
 *  There should be four, three, five and three backslashes for the four
 *  examples on the last line, respectively.  The next to last line should
 *  have four, five, three and three backslashes.  If this was not accurately
 *  reproduced, take a look at the agen5/test/shell.test test.  Notice the
 *  backslashes in front of the dollar signs.  It goes from zero to one to
 *  three for the "cooked" string examples.
=*/
SCM
ag_scm_shell_str(SCM obj)
{
    return shell_stringify(obj, (unsigned char)'"');
}

/*=gfunc sub_shell_str
 *
 * what:  back quoted (sub-)shell string
 * general_use:
 *
 * exparg: string, string to transform
 *
 * doc:
 *   This function is substantially identical to @code{shell-str}, except
 *   that the quoting character is @code{`} and the "leave the escape alone"
 *   character is @code{"}.
=*/
SCM
ag_scm_sub_shell_str(SCM obj)
{
    return shell_stringify(obj, (unsigned char)'`');
}


/*=gfunc stack
 *
 * what:  make list of AutoGen values
 *
 * exparg: ag-name, AutoGen value name
 *
 * doc:  Create a scheme list of all the strings that are associated
 *       with a name.  They must all be text values or we choke.
=*/
SCM
ag_scm_stack(SCM obj)
{
    SCM          res;
    SCM *        pos = &res;
    def_ent_t ** ppDE;
    def_ent_t *  pDE;
    SCM          str;

    res = SCM_EOL;

    ppDE = find_def_ent_list(ag_scm2zchars(obj, "AG Object"));
    if (ppDE == NULL)
        return SCM_EOL;

    for (;;) {
        pDE = *(ppDE++);

        if (pDE == NULL)
            break;

        if (pDE->de_type != VALTYP_TEXT)
            return SCM_UNDEFINED;

        str  = scm_from_latin1_string(pDE->de_val.dvu_text);
        *pos = scm_cons(str, SCM_EOL);
        pos  = SCM_CDRLOC(*pos);
    }

    return res;
}


/*=gfunc kr_string
 *
 * what:  emit string for K&R C
 * general_use:
 *
 * exparg: string, string to reformat
 *
 *  doc:
 *  Reform a string so that, when printed, a K&R C compiler will be able
 *  to compile the data and construct a string that contains exactly
 *  what the current string contains.  Many non-printing characters are
 *  replaced with escape sequences.  New-lines are replaced with a
 *  backslash-n-backslash and newline sequence,
=*/
SCM
ag_scm_kr_string(SCM str)
{
    char const * pz = ag_scm2zchars(str, "krstr");
    SCM res;
    pz  = optionQuoteString(pz, KR_STRING_NEWLINE);
    res = scm_from_latin1_string(pz);
    AGFREE(pz);
    return res;
}


/*=gfunc c_string
 *
 * what:  emit string for ANSI C
 * general_use:
 *
 * exparg: string, string to reformat
 *
 * doc:
 *  Reform a string so that, when printed, the C compiler will be able to
 *  compile the data and construct a string that contains exactly what the
 *  current string contains.  Many non-printing characters are replaced with
 *  escape sequences.  Newlines are replaced with a backslash, an @code{n}, a
 *  closing quote, a newline, seven spaces and another re-opening quote.  The
 *  compiler will implicitly concatenate them.  The reader will see line
 *  breaks.
 *
 *  A K&R compiler will choke.  Use @code{kr-string} for that compiler.
 *
=*/
SCM
ag_scm_c_string(SCM str)
{
    char const * pz = ag_scm2zchars(str, "cstr");
    SCM res;
    pz  = optionQuoteString(pz, C_STRING_NEWLINE);
    res = scm_from_latin1_string(pz);
    AGFREE(pz);
    return res;
}

/**
 * Map a character range for ag_scm_string_tr_x()
 */
static inline void
tr_char_range(unsigned char * ch_map, unsigned char * from, unsigned char * to)
{
    unsigned char fs = (unsigned char)from[-2]; // "from" start char
    unsigned char fe = (unsigned char)from[0];  // "from" end char
    unsigned char ts = (unsigned char)to[-2];   // "to" start char
    unsigned char te = (unsigned char)to[0];    // "to" end char

    while (fs < fe) {
        ch_map[ fs++ ] = ts;
        if (ts < te)
            ts++;
    }
}

/*=gfunc string_tr_x
 *
 * what:  convert characters
 * general_use:
 *
 *  exparg:  source, string to transform
 *  exparg:  match,  characters to be converted
 *  exparg:  translation, conversion list
 *
 * doc: This is the same as the @code{tr(1)} program, except the
 *      string to transform is the first argument.  The second and
 *      third arguments are used to construct mapping arrays for the
 *      transformation of the first argument.
 *
 *      It is too bad this little program has so many different
 *      and incompatible implementations!
=*/
SCM
ag_scm_string_tr_x(SCM str, SCM from_xform, SCM to_xform)
{
    unsigned char ch_map[ 1 << 8 /* bits-per-byte */ ];
    int    i    = sizeof(ch_map) - 1;
    char * from = ag_scm2zchars(from_xform, "str");
    char * to   = ag_scm2zchars(to_xform, "str");

    do  {
        ch_map[i] = (unsigned char)i;
    } while (--i > 0);

    for (; i <= (int)sizeof(ch_map) - 1; i++) {
        unsigned char fch = (unsigned char)*(from++);
        unsigned char tch = (unsigned char)*(to++);

        if (tch == NUL) {
            to--;
            tch = (unsigned char)to[-1];
        }

        switch (fch) {
        case NUL:
            goto map_done;

        case '-':
            /*
             * "from" char is a hyphen.
             * IF we are beyond the first character AND
             *    the "to" character is a hyphen AND
             *    there is a "from" character after the hyphen
             *    there is a "to" character after the hyphen,
             * THEN map a character range
             */
            if (  (i > 0)
               && (tch     == '-')
               && (from[0] != NUL)
               && (to[0]   != NUL)) {
                tr_char_range(ch_map, (unsigned char *)from,
                              (unsigned char *)to);
                break;
            }

        default:
            ch_map[ fch ] = tch;
        }
    } map_done:;

    to = C(char *, scm_i_string_chars(str));
    i    = (int)scm_c_string_length(str);
    while (i-- > 0) {
        *to = (char)ch_map[ (int)*to ];
        to++;
    }
    return str;
}

/*=gfunc string_tr
 *
 * what:  convert characters with new result
 * general_use:
 *
 *  exparg:  source, string to transform
 *  exparg:  match,  characters to be converted
 *  exparg:  translation, conversion list
 *
 * doc: This is identical to @code{string-tr!}, except that it does not
 *      over-write the previous value.
=*/
SCM
ag_scm_string_tr(SCM Str, SCM From, SCM To)
{
    size_t lenz  = scm_c_string_length(Str);
    SCM    res   = scm_from_latin1_stringn(scm_i_string_chars(Str), lenz);
    return ag_scm_string_tr_x(res, From, To);
}

/*=gfunc string_substitute
 *
 * what:  multiple global replacements
 * general_use:
 *
 *  exparg:  source, string to transform
 *  exparg:  match,  substring or substring list to be replaced
 *  exparg:  repl,   replacement strings or substrings
 *
 * doc: @code{match} and  @code{repl} may be either a single string or
 *      a list of strings.  Either way, they must have the same structure
 *      and number of elements.  For example, to replace all amphersands,
 *      less than and greater than characters, do something like this:
 *
 * @example
 *      (string-substitute source
 *          (list "&"     "<"    ">")
 *          (list "&amp;" "&lt;" "&gt;"))
 * @end example
=*/
SCM
ag_scm_string_substitute(SCM str, SCM Match, SCM Repl)
{
    char const *  text;
    ssize_t len;
    SCM     res;

    if (! scm_is_string(str))
        return SCM_UNDEFINED;

    text = scm_i_string_chars(str);
    len   = (ssize_t)scm_c_string_length(str);

    if (scm_is_string(Match))
        do_substitution(text, len, Match, Repl, (char **)&text, &len);
    else
        do_multi_subs((char **)&text, &len, Match, Repl);

    res = scm_from_latin1_stringn(text, (size_t)len);
    return res;
}

/*=gfunc time_string_to_number
 *
 * what:   duration string to seconds
 * general_use:
 * exparg: time_spec, string to parse
 *
 * doc:    Convert the argument string to a time period in seconds.
 *         The string may use multiple parts consisting of days, hours
 *         minutes and seconds.  These are indicated with a suffix of
 *         @code{d}, @code{h}, @code{m} and @code{s} respectively.
 *         Hours, minutes and seconds may also be represented with
 *         @code{HH:MM:SS} or, without hours, as @code{MM:SS}.
=*/
SCM
ag_scm_time_string_to_number(SCM time_spec)
{
    extern time_t parse_duration(char const * in_pz);

    char const * pz;
    time_t  time_period;

    if (! scm_is_string(time_spec))
        return SCM_UNDEFINED;

    pz = scm_i_string_chars(time_spec);
    time_period = parse_duration(pz);

    return scm_from_int((int)time_period);
}

/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/expString.c */
