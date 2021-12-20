
/**
 * @file expMake.c
 *
 *  This module implements Makefile construction functions.
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

/**
 * Figure out how to handle the line continuation.
 * If the line we just finished ends with a backslash, we're done.
 * Just add the newline character.  If it ends with a semi-colon or a
 * doubled amphersand or doubled or-bar, then escape the newline with a
 * backslash.  If the line ends with one of the keywords "then", "in" or
 * "else", also only add the escaped newline.  Otherwise, add a
 * semi-colon, backslash and newline.
 *
 * @param  ppzi  pointer to pointer to input text
 * @param  ppzo  pointer to pointer to output text
 * @param  tabch line prefix (tab) character
 * @param  bol   pointer to start of currently-being-output line
 *
 * @returns false to say the newline is dropped becase we're done
 *          true  to say the line was appended with the newline.
 */
static bool
handle_eol(char ** ppzi, char ** ppzo, char tabch, char * bol)
{
    char * pzScn = *ppzi;
    char * pzOut = *ppzo;
    int    l_len = (int)(pzOut - bol);

    /*
     *  Backup past trailing white space (other than newline).
     */
    while (IS_NON_NL_WHITE_CHAR(pzOut[-1]))
        pzOut--;

    /*
     *  Skip over empty lines, but leave leading white space
     *  on the next non-empty line.
     */
    {
        char * pz = pzScn;
        while (IS_WHITESPACE_CHAR(*pz)) {
            if (*(pz++) == NL)
                pzScn = pz;
        }
    }

    /*
     *  The final newline is dropped.
     */
    if (*pzScn == NUL)
        return false;

    switch (pzOut[-1]) {
    case '\\':
        /*
         *  The newline is already escaped, so don't
         *  insert our extra command termination.
         */
        *(pzOut++) = NL;
        break;

    case '&':
        /*
         *  A single ampersand is a backgrounded command.  We must terminate
         *  those statements, but not statements conjoined with '&&'.
         */
        if ('&' != pzOut[-2])
            goto append_statement_end;
        /* FALLTHROUGH */

    case '|':
    case ';':
    skip_semi_colon:
        /*
         *  Whatever the reason for a final '|', '&' or ';',
         *  we will not add a semi-colon after it.
         */
        memcpy(pzOut, MAKE_SCRIPT_NL + 2, MAKE_SCRIPT_NL_LEN - 2);
        pzOut += MAKE_SCRIPT_NL_LEN - 2;
        break;

    case 'n': // "then" or "in"
        if (l_len < 3)
            goto append_statement_end;

        if (pzOut[-2] == 'i') {
            if ((l_len == 3) || IS_WHITESPACE_CHAR(pzOut[-3]))
                goto skip_semi_colon;
            goto append_statement_end;
        }

        if (  (l_len < 5)
           || (  (l_len > 5)
              && ! IS_WHITESPACE_CHAR(pzOut[-5]) ))
            goto append_statement_end;
        if (strncmp(pzOut-4, HANDLE_EOL__THE, HANDLE_EOL__THE_LEN) == 0)
            goto skip_semi_colon;
        goto append_statement_end;

    case 'e': // "else"
        if (  (l_len < 5)
           || (  (l_len > 5)
              && ! IS_WHITESPACE_CHAR(pzOut[-5]) ))
            goto append_statement_end;
        if (strncmp(pzOut-4, HANDLE_EOL__ELS, HANDLE_EOL__ELS_LEN) == 0)
            goto skip_semi_colon;
        goto append_statement_end;

    default:
    append_statement_end:
        /*
         *  Terminate the current command and escape the newline.
         */
        memcpy(pzOut, MAKE_SCRIPT_NL, MAKE_SCRIPT_NL_LEN);
        pzOut += MAKE_SCRIPT_NL_LEN;
    }

    /*
     *  We have now started our next output line and there are still data.
     *  Indent with a tab, if called for.  If we do insert a tab, then skip
     *  leading tabs on the line.
     */
    if (tabch) {
        *(pzOut++) = tabch;
        while (*pzScn == tabch)  pzScn++;
    }

    *ppzi = pzScn;
    *ppzo = pzOut;
    return true;
}

/**
 * Pass through untreated sedable lines.  Sometimes it is just very useful
 * to post-process Makefile files with sed(1) to clean it up.
 *
 * @param txt   pointer to text.  We skip initial white space.
 * @param tab   pointer to where we stash the tab character to use
 * @returns     true to say this was a sed line and was emitted,
 *              false to say it was not and needs to be copied out.
 */
static bool
handle_sed_expr(char ** src_p, char ** out_p)
{
    char * src = *src_p;
    char * out = *out_p;

    switch (src[1]) {
    case 'i':
        if (strncmp(src+2, HANDLE_SED_IFNDEF, HANDLE_SED_IFNDEF_LEN) == 0)
            break;
        if (strncmp(src+2, HANDLE_SED_IFDEF, HANDLE_SED_IFDEF_LEN) == 0)
            break;
        return false;

    case 'e':
        if (strncmp(src+2, HANDLE_SED_ELSE, HANDLE_SED_ELSE_LEN) == 0)
            break;
        if (strncmp(src+2, HANDLE_SED_ENDIF, HANDLE_SED_ENDIF_LEN) == 0)
            break;
        /* FALLTHROUGH */
    default:
        return false;
    }

    {
        char * p = BRK_NEWLINE_CHARS(src);
        size_t l;
        if (*p == NL) /* do not skip NUL */
            p++;
        l = (size_t)(p - src);
        memcpy(out, src, l);
        *src_p = src + l;
        *out_p = out + l;
    }

    return true;
}

/**
 * Compute a maximal size for the output script.  Leading and trailing white
 * space are trimmed.  Dollar characters will likely be doubled and newlines
 * may get as many as MAKE_SCRIPT_NL_LEN characters inserted.  Make sure
 * there's space.
 *
 * @param txt   pointer to text.  We skip initial white space.
 * @param tab   pointer to where we stash the tab character to use
 * @returns     the maximum number of bytes required to store result.
 */
static size_t
script_size(char ** txt_p, char * tab)
{
    char * txt = *txt_p;
    char * ptxte;
    size_t sz  = 0;

    /*
     *  skip all blank lines and other initial white space
     *  in the source string.
     */
    if (! IS_WHITESPACE_CHAR(*txt))
        *tab = TAB;
    else {
        txt = SPN_WHITESPACE_CHARS(txt + 1);
        *tab  = (txt[-1] == TAB) ? NUL : TAB;
    }

    /*
     *  Do nothing with empty input.
     */
    if (*txt == NUL)
        return 0;

    /*
     *  "txt" is now our starting point.  Do not modify it any more.
     */
    *txt_p = txt;

    for (ptxte = txt - 1;;)  {
        ptxte = BRK_MAKE_SCRIPT_CHARS(ptxte+1);
        if (*ptxte == NUL)
            break;
        sz += (*ptxte == '$') ? 1 : MAKE_SCRIPT_NL_LEN;
    }

    ptxte = SPN_WHITESPACE_BACK(txt, ptxte);
    *ptxte = NUL;
    sz += (size_t)(ptxte - txt);
    return sz;
}

/*=gfunc makefile_script
 *
 * what:  create makefile script
 * general_use:
 *
 * exparg: text, the text of the script
 *
 * doc:
 *  This function will take ordinary shell script text and reformat it
 *  so that it will work properly inside of a makefile shell script.
 *  Not every shell construct can be supported; the intent is to have
 *  most ordinary scripts work without much, if any, alteration.
 *
 *  The following transformations are performed on the source text:
 *
 *  @enumerate
 *  @item
 *  Trailing whitespace on each line is stripped.
 *
 *  @item
 *  Except for the last line, the string, " ; \\" is appended to the end of
 *  every line that does not end with certain special characters or keywords.
 *  Note that this will mutilate multi-line quoted strings, but @command{make}
 *  renders it impossible to use multi-line constructs anyway.
 *
 *  @item
 *  If the line ends with a backslash, it is left alone.
 *
 *  @item
 *  If the line ends with a semi-colon, conjunction operator, pipe (vertical
 *  bar) or one of the keywords "then", "else" or "in", then a space and a
 *  backslash is added, but no semi-colon.
 *
 *  @item
 *  The dollar sign character is doubled, unless it immediately precedes an
 *  opening parenthesis or the single character make macros '*', '<', '@@',
 *  '?' or '%'.  Other single character make macros that do not have enclosing
 *  parentheses will fail.  For shell usage of the "$@@", "$?" and "$*"
 *  macros, you must enclose them with curly braces, e.g., "$@{?@}".
 *  The ksh construct @code{$(<command>)} will not work.  Though some
 *  @command{make}s accept @code{$@{var@}} constructs, this function will
 *  assume it is for shell interpretation and double the dollar character.
 *  You must use @code{$(var)} for all @command{make} substitutions.
 *
 *  @item
 *  Double dollar signs are replaced by four before the next character
 *  is examined.
 *
 *  @item
 *  Every line is prefixed with a tab, unless the first line
 *  already starts with a tab.
 *
 *  @item
 *  The newline character on the last line, if present, is suppressed.
 *
 *  @item
 *  Blank lines are stripped.
 *
 *  @item
 *  Lines starting with "@@ifdef", "@@ifndef", "@@else" and "@@endif" are
 *  presumed to be autoconf "sed" expression tags.  These lines will be
 *  emitted as-is, with no tab prefix and no line splicing backslash.
 *  These lines can then be processed at configure time with
 *  @code{AC_CONFIG_FILES} sed expressions, similar to:
 *
 *  @example
 *  sed "/^@@ifdef foo/d;/^@@endif foo/d;/^@@ifndef foo/,/^@@endif foo/d"
 *  @end example
 *  @end enumerate
 *
 *  @noindent
 *  This function is intended to be used approximately as follows:
 *
 *  @example
 *  $(TARGET) : $(DEPENDENCIES)
 *  <+ (out-push-new) +>
 *  ....mostly arbitrary shell script text....
 *  <+ (makefile-script (out-pop #t)) +>
 *  @end example
=*/
SCM
ag_scm_makefile_script(SCM text_scm)
{
    char * res_str; /*@< result string */
    char * out;     /*@< output scanning ptr */
    char * bol;     /*@< start of last output line */
    char   tabch;   /*@< char to use for start-of-line tab */

    char * text = ag_scm2zchars(text_scm, "make script");
    size_t sz   = script_size(&text, &tabch);

    if (sz == 0)
        return scm_from_latin1_string(zNil);

    bol = out = res_str = scribble_get((ssize_t)sz);

    /*
     *  Force the initial line to start with a real tab.
     */
    *(out++) = TAB;

    for (;;) {
        char * p = BRK_MAKE_SCRIPT_CHARS(text);
        size_t l = (size_t)(p - text);
        if (l > 0) {
            memcpy(out, text, l);
            text  = p;
            out += l;
        }

        /*
         * "text" now points to one of three characters:
         * a newline, a dollar or a NUL byte.
         */
        if (*text == NUL)
            break;

        if (*text == NL) {
            if (! handle_eol(&text, &out, tabch, bol))
                break;

            bol = out;

            /*
             * As a special "hack", if a line starts with "@ifdef", "@ifndef",
             * "@else" or "@endif", then we assume post processing sed will
             * fix it up.  Those lines get left alone.
             */
            if (*text == '@') {
                if (handle_sed_expr(&text, &out))
                    bol = out;
            }

        } else {
            /*
             * Quadruple a double dollar, leave alone make-interesting
             * dollars, and double it otherwise.
             */
            switch (text[1]) {
            case '(': case '*': case '@': case '<': case '%': case '?':
                /* one only */
                break;

            case '$':
                /*
                 *  $$ in the shell means process id.  Avoid having to do a
                 *  backward scan on the second '$' by handling the next '$'
                 *  now.  We get FOUR '$' chars.
                 */
                text++;
                *(out++) = '$';
                *(out++) = '$';
                *(out++) = '$';
                /* quadruple */
                break;

            default:
                *(out++) = '$'; /* double */
            }

            *(out++) = *(text++);
        }
    }

    {
        SCM res = scm_from_latin1_stringn(res_str, (size_t)(out - res_str));
        return res;
    }
}

/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/expMake.c */
