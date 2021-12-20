
/**
 * @file defLex.c
 *
 *  This module scans the template variable declarations and passes
 *  tokens back to the parser.
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
/*
 *  This keyword table must match those found in agParse.y.
 *  You will find them in a %token statement that follows
 *  a comment  "Keywords"
 */
#define KEYWORD_TABLE                           \
  _KW_( AUTOGEN )                               \
  _KW_( DEFINITIONS )

#define _KW_(w) static char const z ## w [] = #w;
KEYWORD_TABLE
#undef _KW_

#define _KW_(w) z ## w,
static char const * const kword_table[] = { KEYWORD_TABLE };
#undef _KW_

#define _KW_(w) DP_EV_ ## w,
te_dp_event kword_tkns[] = { KEYWORD_TABLE };
#undef _KW_

#define KEYWORD_CT  (sizeof(kword_table) / sizeof(kword_table[0]))

#define ERROR  (-1)
#define FINISH (-1)

/* = = = START-STATIC-FORWARD = = = */
static void
pop_context(void);

static int
count_nl(char const * pz);

static void
trim_whitespace(void);

static void
lex_escaped_char(void);

static tSuccess
lex_backquote(void);

static tSuccess
lex_comment(void);

static tSuccess
lex_dollar(void);

static tSuccess
lex_here_string(void);

static void
loadScheme(void);

static void
alist_to_autogen_def(void);

static char *
gather_name(char * scan, te_dp_event * ret_val);

static char *
build_here_str(char * scan);
/* = = = END-STATIC-FORWARD = = = */

/**
 *  Pop off an include context and resume from the including file.
 */
static void
pop_context(void)
{
    scan_ctx_t * pCX = cctx;
    cctx          = cctx->scx_next;
    pCX->scx_next = end_ctx;
    end_ctx       = pCX;
}

/**
 * Count the newlines in a NUL terminated string
 */
static int
count_nl(char const * pz)
{
    int ct = 0;
    for (;;) {
        char const * p = strchr(pz, NL);
        if (p == NULL)
            break;
        ct++;
        pz = p + 1;
    }
    return ct;
}

/**
 * Remove leading white space from the current scan point.
 */
static void
trim_whitespace(void)
{
    char * pz = cctx->scx_scan;

    /*
     *  This ensures that any names found previously
     *  are NUL terminated.
     */
    if (*pz == NL)
        cctx->scx_line++;
    *pz = NUL;

    for (;;) {
        pz = SPN_HORIZ_WHITE_CHARS(pz+1);
        if (*pz != NL)
            break;
        cctx->scx_line++;
    }

    cctx->scx_scan = pz;
}

static void
lex_escaped_char(void)
{
    static int const semi_colon = ';';

    char * pz = strchr(cctx->scx_scan, semi_colon);

    for (;;) {
        if (pz == NULL) {
            pz = cctx->scx_scan + strlen(cctx->scx_scan);
            break;
        }
        if (IS_WHITESPACE_CHAR(pz[1])) {
            *pz = NUL;
            pz[1] = (char)semi_colon;
            break;
        }
        pz = strchr(pz+1, semi_colon);
    }

    token_code = DP_EV_STRING;
    token_str = pz;
}

static tSuccess
lex_backquote(void)
{
    int line_no = cctx->scx_line;
    char * pz = ao_string_cook(cctx->scx_scan, &line_no);

    if (pz == NULL)
        return FAILURE;

    token_str = cctx->scx_scan;

    cctx->scx_scan = pz;

    token_code = DP_EV_STRING;
    pz = shell_cmd((char const *)token_str);
    cctx->scx_line = line_no;

    if (pz == NULL)
        return PROBLEM;

    token_str = pz;
    return SUCCESS;
}

static tSuccess
lex_comment(void)
{
    /*
     *  Allow for a comment, C or C++ style
     */
    switch (cctx->scx_scan[1]) {
    case '*':
    {
        char * pz = strstr(cctx->scx_scan+2, END_C_COMMENT);
        if (pz == NULL)
            break;

        char * p = cctx->scx_scan+1;
        for (;;) {
            p = strchr(p+1, NL);
            if ((p == NULL) || (p > pz))
                break;
            cctx->scx_line++;
        }
        cctx->scx_scan = pz+2;
        return SUCCESS;
    }

    case '/':
    {
        char * pz = strchr(cctx->scx_scan+2, NL);
        if (pz != NULL) {
            cctx->scx_scan = pz+1;
            cctx->scx_line++;
            return SUCCESS;
        }
        break;
    }
    }

    return FAILURE;
}

/**
 * process a dollar character introduced value.
 *
 * '$(<...)' load a file that must be there.
 * '$(?...)' load a file that may be there. If not there, then delete
 *           the named value associated with it.
 */
static tSuccess
lex_dollar(void)
{
    char * next;

    /*
     * First, ensure the string starts with what we recognize
     */
    if (cctx->scx_scan[1] != '(')
        return FAILURE;
    switch (cctx->scx_scan[2]) {
    case '<':
    case '?': break;
    default: return FAILURE;
    }

    /*
     * Find the start and end of the file name and load the file text.
     */
    do {
        char * fns = SPN_WHITESPACE_CHARS(cctx->scx_scan+3);
        char * fne = strchr(fns, ')');
        char   svc;

        if (fne == NULL)
            return FAILURE;
        next = fne + 1;
        fne  = SPN_WHITESPACE_BACK(fns, fne-1);
        svc  = *fne;
        *fne = NUL;
        token_str = load_file(fns);
        *fne = svc;
        if (token_str == NULL)
            break;
        token_code = DP_EV_STRING;
        cctx->scx_scan = next;
        return SUCCESS; /* done -- we've loaded the file. */
    } while (0);

    if (cctx->scx_scan[2] == '<')
        return FAILURE;

    cctx->scx_scan = next;
    token_code = DP_EV_DELETE_ENT;
    return SUCCESS;
}

static tSuccess
lex_here_string(void)
{
    char * pz;
    if (cctx->scx_scan[1] != '<')
        return FAILURE;

    pz = build_here_str(cctx->scx_scan + 2);
    if (pz == NULL) {
        token_code = DP_EV_INVALID;
        return PROBLEM;
    }

    token_code = DP_EV_HERE_STRING;
    cctx->scx_scan = pz;
    return SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   LEXICAL SCANNER
 */
LOCAL te_dp_event
yylex(void)
{
#define SET_LIT_TKN(t) \
    token_code = DP_EV_LIT_ ## t; *(cctx->scx_scan++) = NUL;

    token_code = DP_EV_INVALID;

scan_again:
    /*
     *  Start the process of locating a token.
     *  We branch here after skipping over a comment
     *  or processing a directive (which may change our context).
     */
    if (IS_WHITESPACE_CHAR(*cctx->scx_scan))
        trim_whitespace();

    switch (*cctx->scx_scan) {
    case NUL:
        /*
         *  IF we are not inside an include context,
         *  THEN go finish.
         */
        if (cctx->scx_next == NULL)
            goto lex_done;

        pop_context();
        goto scan_again;

    case '#':
    {
        extern char * processDirective(char *);
        char * pz = processDirective(cctx->scx_scan+1);
        /*
         *  Ensure that the compiler doesn't try to save a copy of
         *  "cctx" in a register.  It must be reloaded from memory.
         */
        cctx->scx_scan = pz;
        goto scan_again;
    }

    case '{': SET_LIT_TKN(O_BRACE);   break;
    case '=': SET_LIT_TKN(EQ);        break;
    case '}': SET_LIT_TKN(C_BRACE);   break;
    case '[': SET_LIT_TKN(OPEN_BKT);  break;
    case ']': SET_LIT_TKN(CLOSE_BKT); break;
    case ';': SET_LIT_TKN(SEMI);      break;
    case ',': SET_LIT_TKN(COMMA);     break;

    case '\'':
    case '"':
    {
        char * pz = ao_string_cook(cctx->scx_scan, &(cctx->scx_line));
        if (pz == NULL)
            goto NUL_error;

        token_str = cctx->scx_scan;

        token_code = DP_EV_STRING;
        cctx->scx_scan = pz;
        break;
    }

    case '$':
        if (lex_dollar() == SUCCESS)
            break;
        goto bad_token;
        
    case '<':
        switch (lex_here_string()) {
        case SUCCESS: break;
        case FAILURE: goto bad_token;
        case PROBLEM: return DP_EV_INVALID;
        }
        break;

    case '(':
        loadScheme();
        break;

    case '\\':
        if (strncmp(cctx->scx_scan+1, START_SCHEME_LIST, (size_t)2) == 0) {
            alist_to_autogen_def();
            goto scan_again;
        }
        lex_escaped_char();
        break;

    case '`':
        switch (lex_backquote()) {
        case FAILURE: goto NUL_error;
        case PROBLEM: goto scan_again;
        case SUCCESS: break;
        }
        break;

    case '/':
        if (lex_comment() == SUCCESS)
            goto scan_again;
        /* FALLTHROUGH */ /* to Invalid input char */

    default:
    bad_token:
        cctx->scx_scan = gather_name(cctx->scx_scan, &token_code);
        break;
    }   /* switch (*cctx->scx_scan) */

    return token_code;

 NUL_error:

    AG_ABEND(aprf(DEF_ERR_FMT, ag_pname, YYLEX_UNENDING_QUOTE,
                  cctx->scx_fname, cctx->scx_line));
    return DP_EV_INVALID;

 lex_done:
    /*
     *  First time through, return the DP_EV_END token.
     *  Second time through, we really finish.
     */
    if (cctx->scx_scan == zNil) {
        cctx->scx_next = end_ctx;
        end_ctx        = cctx;

        return DP_EV_INVALID;
    }

    cctx->scx_scan = (char *)zNil;
    return DP_EV_END;
#undef SET_LIT_TKN
}


LOCAL void
yyerror(char * s)
{
    char * pz;

    if (strlen(cctx->scx_scan) > 64 )
        cctx->scx_scan[64] = NUL;

    switch (token_code) {
    case DP_EV_VAR_NAME:
    case DP_EV_OTHER_NAME:
    case DP_EV_STRING:
    case DP_EV_NUMBER:
        if (strlen(token_str) > 64 )
            token_str[64] = NUL;

        pz = aprf(YYLEX_TOKEN_STR, DP_EVT_NAME(token_code), token_str);
        break;

    default:
        pz = aprf(YYLEX_DF_STR, DP_EVT_NAME(token_code));
    }
    AG_ABEND(aprf(YYLEX_ERR_FMT, s, cctx->scx_fname, cctx->scx_line,
                  pz, cctx->scx_scan));
}


static void
loadScheme(void)
{
    char *   pzText    = cctx->scx_scan;
    char *   pzEnd     = (char *)skip_scheme(pzText, pzText + strlen(pzText));
    char     endCh     = *pzEnd;
    int      schemeLen = (int)(pzEnd - pzText);
    int      next_ln;
    SCM      res;

    /*
     *  NUL terminate the Scheme expression, run it, then restore
     *  the NUL-ed character.
     */
    if (*pzEnd == NUL)
        AG_ABEND(aprf(DEF_ERR_FMT, ag_pname, LOAD_SCM_ENDLESS,
                      cctx->scx_fname, cctx->scx_line));

    *pzEnd  = NUL;
    next_ln = cctx->scx_line + count_nl(pzText);

    processing_state = PROC_STATE_GUILE_PRELOAD;
    res = ag_scm_c_eval_string_from_file_line(
        pzText, cctx->scx_fname, cctx->scx_line );
    processing_state = PROC_STATE_LOAD_DEFS;
    *pzEnd = endCh;

    cctx->scx_scan = pzEnd;
    pzEnd = (char *)scm2display(res); /* ignore const-ness */
    cctx->scx_line = next_ln;

    if ((int)strlen(pzEnd) >= schemeLen) {
        AGDUPSTR(pzEnd, pzEnd, "SCM Result");

        token_str = pzEnd;
    }

    else {
        /*
         *  We know the result is smaller than the source.  Copy in place.
         */
        strcpy(pzText, pzEnd);
        token_str = pzText;
    }

    token_code = DP_EV_STRING;
}

/*
 *  process a single scheme expression, yielding text that gets processed
 *  into AutoGen definitions.
 */
static void
alist_to_autogen_def(void)
{
    char * pzText  = ++(cctx->scx_scan);
    char * pzEnd   = (char *)skip_scheme(pzText, pzText + strlen(pzText));

    SCM    res;
    size_t res_len;
    scan_ctx_t * pCtx;

    /*
     *  Wrap the scheme expression with the `alist->autogen-def' function
     */
    {
        char endCh = *pzEnd;
        *pzEnd = NUL;
        pzText = aprf(ALIST_TO_AG_WRAP, pzText);
        *pzEnd = endCh;
    }

    /*
     *  Run the scheme expression.  The result is autogen definition text.
     */
    processing_state = PROC_STATE_GUILE_PRELOAD;
    res = ag_scm_c_eval_string_from_file_line(
        pzText, cctx->scx_fname, cctx->scx_line );

    /*
     *  The result *must* be a string, or we choke.
     */
    if (! scm_is_string(res))
        AG_ABEND(ALIST_TO_AG_ERR);

    res_len   = scm_c_string_length(res);
    processing_state = PROC_STATE_LOAD_DEFS;
    cctx->scx_scan = pzEnd;
    AGFREE(pzText);

    /*
     *  Now, push the resulting string onto the input stack
     *  and link the new scan data into the context stack
     */
    pCtx = (scan_ctx_t *)AGALOC(sizeof(scan_ctx_t) + 4 + res_len, "lex ctx");
    pCtx->scx_next = cctx;
    cctx           = pCtx;

    /*
     *  Set up the rest of the context structure
     */
    AGDUPSTR(pCtx->scx_fname, ALIST_TO_AG_TEXT, "scheme text");
    pCtx->scx_scan = \
    pCtx->scx_data = (char *)(pCtx + 1);
    pCtx->scx_line = 0;
    memcpy(VOIDP(pCtx->scx_scan), VOIDP(scm_i_string_chars(res)), res_len);
    pCtx->scx_scan[ res_len ] = NUL;

    /*
     *  At this point, the next token will be obtained from the newly
     *  allocated context structure.  When empty, input will resume
     *  from the '}' that we left as the next input token in the old
     *  context.
     */
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  It may be a number, a name, a keyword or garbage.
 *  Figure out which.
 */
static char *
gather_name(char * scan, te_dp_event * ret_val)
{
    /*
     *  Check for a number.  We don't care about leading zeros, that
     *  is a user's problem, so no distinction between octal and decimal.
     *  However, we look for "0x" and "0X" prefixes for hex numbers.
     */
    if (  IS_DEC_DIGIT_CHAR(*scan)
       || ((*scan == '-') && IS_DEC_DIGIT_CHAR(scan[1])) ) {
        token_str = scan;
        *ret_val  = DP_EV_NUMBER;

        if (  (scan[0] == '0')
           && ((scan[1] == 'x') || (scan[1] == 'X')) )
            return SPN_HEX_DIGIT_CHARS(scan + 2);

        return SPN_DEC_DIGIT_CHARS(scan + 1);
    }

    if (! IS_UNQUOTABLE_CHAR(*scan))
        AG_ABEND(aprf(ASSEMBLE_NAME_ERR, ag_pname, *scan, cctx->scx_fname,
                      cctx->scx_line));

    {
        char * pz = SPN_VALUE_NAME_CHARS(scan);

        if (IS_UNQUOTABLE_CHAR(*pz)) {
            *ret_val = DP_EV_OTHER_NAME;
            pz = SPN_UNQUOTABLE_CHARS(pz+1);

        } else
            *ret_val = DP_EV_VAR_NAME;

        /*
         *  Return a NAME token, maybe.
         *  If the name is actually a keyword,
         *  we will return that token code instead.
         */
        token_str = scan;
        scan   = (char *)pz;
    }

    /*
     *  Now scan the keyword table.
     */
    if (*ret_val == DP_EV_VAR_NAME) {
        char sv_ch = *scan;  /* preserve the following character */
        int  kw_ix = 0;
        *scan = NUL;         /* NUL terminate the name           */

        do  {
            if (streqvcmp(kword_table[ kw_ix ], (char *)token_str) == 0) {
                /*
                 *  Return the keyword token code instead of DP_EV_NAME
                 */
                *ret_val = kword_tkns[ kw_ix ];
                break;
            }
        } while (++kw_ix < (int)KEYWORD_CT);

        *scan = sv_ch;         /* restore the following character  */
    }

    return scan;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  A quoted string has been found.
 *  Find the end of it and compress any escape sequences.
 */
static char *
build_here_str(char * scan)
{
    bool     no_tabs = false;
    size_t   markLen = 0;
    char *   dest;
    int      here_str_line;
    char     here_mark[MAX_HEREMARK_LEN];

    /*
     *  See if we are to strip leading tab chars
     */
    if (*scan == '-') {
        no_tabs = true;
        scan++;
    }

    /*
     *  Skip white space up to the marker or EOL
     */
    scan = SPN_NON_NL_WHITE_CHARS(scan);
    if (*scan == NL)
        AG_ABEND(aprf(DEF_ERR_FMT, ag_pname, HERE_MISS_MARK_STR,
                      cctx->scx_fname, cctx->scx_line));

    /*
     *  Copy the marker, noting its length
     */
    {
        char * pz = SPN_VARIABLE_NAME_CHARS(scan);
        markLen = (size_t)(pz - scan);

        if (markLen == 0)
            AG_ABEND(aprf(DEF_ERR_FMT, ag_pname, HERE_MISS_MARK_STR,
                          cctx->scx_fname, cctx->scx_line));

        if (markLen >= sizeof(here_mark))
            AG_ABEND(aprf(DEF_ERR_FMT, ag_pname, HERE_MARK_TOO_LONG,
                          cctx->scx_fname, cctx->scx_line));

        memcpy(here_mark, scan, markLen);
        here_mark[markLen] = NUL;
    }

    token_str = dest = scan;

    /*
     *  Skip forward to the EOL after the marker.
     */
    scan = strchr(scan, NL);
    if (scan == NULL)
        AG_ABEND(aprf(DEF_ERR_FMT, ag_pname, HERE_ENDLESS_STR,
                      cctx->scx_fname, cctx->scx_line));

    /*
     *  And skip the first new line + conditionally skip tabs
     */
    here_str_line = cctx->scx_line++;
    scan++;

    for (;;) {
    next_line:
        if (no_tabs) {
            while (*scan == TAB)  ++scan;
            if ((scan[0] == '\\') && IS_HORIZ_WHITE_CHAR(scan[1]))
                ++scan;
        }

        /*
         *  If we recognize the end mark, break out.
         */
        if (! IS_VARIABLE_NAME_CHAR(scan[markLen]))
            if (strncmp(scan, here_mark, markLen) == 0)
                break;

        for (;;) {
            switch (*(dest++) = *(scan++)) {
            case NL:
                cctx->scx_line++;
                goto next_line;

            case NUL:
                AG_ABEND(aprf(DEF_ERR_FMT, ag_pname, HERE_ENDLESS_STR,
                              cctx->scx_fname, here_str_line));
            }
        }
    } /* while strncmp ... */

    /*
     *  dest may still equal token_str, if no data were copied
     */
    if (dest > (char *)token_str)
         dest[-1] = NUL;
    else dest[0]  = NUL;

    return scan + markLen;
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/defLex.c */
