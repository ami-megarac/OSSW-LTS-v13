
/**
 * @file agUtils.c
 *
 * Various utilities for AutoGen.
 *
 * @addtogroup autogen
 * @{
 */
/*
 *  This file is part of AutoGen.
 *  Copyright (C) 1992-2016 Bruce Korb - all rights reserved
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
static void
define_base_name(void);

static void
put_defines_into_env(void);

static void
open_trace_file(char ** av, tOptDesc * odsc);

static void
check_make_dep_env(void);

static char const *
skip_quote(char const * qstr);
/* = = = END-STATIC-FORWARD = = = */

/**
 * Print a file system error fatal error message and die.
 *
 * @param[in] op         the operation that failed.
 * @param[in] fname      the file name the operation was on.
 * @noreturn
 */
LOCAL void
fswarn(char const * op, char const * fname)
{
    fprintf(stderr, FS_ERR_WARNING, errno, strerror(errno), op, fname);
}

/**
 * Allocating printf function.  It either works or kills the program.
 * @param[in] pzFmt the input format
 * @returns the allocated, formatted result string.
 */
LOCAL char *
aprf(char const * pzFmt, ...)
{
    char * pz;
    va_list ap;
    va_start(ap, pzFmt);
    (void)vasprintf(&pz, pzFmt, ap);
    va_end(ap);

    if (pz == NULL) {
        char z[ 2 * SCRIBBLE_SIZE ];
        snprintf(z, sizeof(z), APRF_ALLOCATE_FAIL, pzFmt);
        AG_ABEND(z);
    }
    return pz;
}

/**
 * Figure out what base name to use.  --base-name was not specified.
 * Base it on the definitions file, if available.
 */
static void
define_base_name(void)
{
    char const *  pz;
    char * pzD;

    if (! ENABLED_OPT(DEFINITIONS)) {
        OPT_ARG(BASE_NAME) = DFT_BASE_NAME;
        return;
    }

    pz = strrchr(OPT_ARG(DEFINITIONS), '/');
    /*
     *  Point to the character after the last '/', or to the full
     *  definition file name, if there is no '/'.
     */
    pz = (pz == NULL) ? OPT_ARG(DEFINITIONS) : (pz + 1);

    /*
     *  IF input is from stdin, then use "stdin"
     */
    if ((pz[0] == '-') && (pz[1] == NUL)) {
        OPT_ARG(BASE_NAME) = STDIN_FILE_NAME;
        return;
    }

    /*
     *  Otherwise, use the basename of the definitions file
     */
    OPT_ARG(BASE_NAME) = \
        pzD = AGALOC(strlen(pz)+1, "derived base");

    while ((*pz != NUL) && (*pz != '.'))  *(pzD++) = *(pz++);
    *pzD = NUL;
}

/**
 * Put the -D option arguments into the environment.
 * This makes them accessible to Guile/Scheme code, too.
 */
static void
put_defines_into_env(void)
{
    int     ct  = STACKCT_OPT(DEFINE);
    char const **   ppz = STACKLST_OPT(DEFINE);

    do  {
        char const * pz = *(ppz++);
        /*
         *  IF there is no associated value,  THEN set it to '1'.
         *  There are weird problems with empty defines.
         *  FIXME:  we loose track of this memory.  Don't know what to do,
         *  really, there is no good recovery mechanism for environment
         *  data.
         */
        if (strchr(pz, '=') == NULL) {
            size_t siz = strlen(pz)+3;
            char * p   = AGALOC(siz, "env define");

            strcpy(p, pz);
            strcpy(p+siz-3, DFT_ENV_VAL);
            pz = p;
        }

        /*
         *  Now put it in the environment
         */
        putenv((char *)pz);
    } while (--ct > 0);
}

/**
 *  Open trace output file.
 *
 *  If the name starts with a pipe character (vertical bar), then
 *  use popen on the command.  If it starts with ">>", then append
 *  to the file name that  follows that.
 *
 *  The trace output starts with the command and arguments used to
 *  start autogen.
 *
 * @param[in] av    autogen's argument vector
 * @param[in] odsc  option descriptor with file name string argument
 */
static void
open_trace_file(char ** av, tOptDesc * odsc)
{
    char const * fname = odsc->optArg.argString;

    trace_is_to_pipe = (*fname == '|');
    if (trace_is_to_pipe)
        trace_fp = popen(++fname, "w");

    else if ((fname[0] == '>') && (fname[1] == '>')) {
        fname = SPN_WHITESPACE_CHARS(fname + 2);
        trace_fp = fopen(fname, "a");
    }

    else
        trace_fp = fopen(fname, "w");

    if (trace_fp == NULL)
        fserr(AUTOGEN_EXIT_FS_ERROR, "fopen", fname);

#ifdef _IONBF
    setvbuf(trace_fp, NULL, _IONBF, 0);
#endif

    fprintf(trace_fp, TRACE_START_FMT, (unsigned int)getpid(), *av);
    while (*(++av) != NULL)
        fprintf(trace_fp, TRACE_AG_ARG_FMT, *av);
    fprintf(trace_fp, TRACE_START_GUILE, libguile_ver);
}

/**
 * Check the environment for make dependency info.  We look for
 * AUTOGEN_MAKE_DEP, but if that is not found, we also look for
 * DEPENDENCIES_OUTPUT.  To do dependency tracking at all, we
 * must find one of these environment variables and it must
 *
 * * be non-empty
 * * not contain a variation on "no"
 * * not contain a variation on "false"
 *
 * Furthermore, to specify a file name, the contents must not contain
 * some variation on "yes" or "true".
 */
static void
check_make_dep_env(void)
{
    bool have_opt_string = false;
    bool set_opt         = false;

    char const * mdep = getenv(AG_MAKE_DEP_NAME);
    if (mdep == NULL) {
        mdep = getenv(DEP_OUT_NAME);
        if (mdep == NULL)
            return;
    }
    switch (*mdep) {
    case NUL: break;
    case '1':
        set_opt = (mdep[0] == '1');
        /* FALLTHROUGH */

    case '0':
        if (mdep[1] != NUL)
            have_opt_string = true;
        break;

    case 'y':
    case 'Y':
        set_opt = true;
        have_opt_string = (streqvcmp(mdep + 1, YES_NAME_STR+1) != 0);
        break;

    case 'n':
    case 'N':
        set_opt = \
            have_opt_string = (streqvcmp(mdep + 1, NO_NAME_STR+1) != 0);
        break;

    case 't':
    case 'T':
        set_opt = true;
        have_opt_string = (streqvcmp(mdep + 1, TRUE_NAME_STR+1) != 0);
        break;

    case 'f':
    case 'F':
        set_opt = \
            have_opt_string = (streqvcmp(mdep + 1, FALSE_NAME_STR+1) != 0);
        break;

    default:
        have_opt_string = \
            set_opt = true;
    }
    if (! set_opt) return;
    if (! have_opt_string) {
        SET_OPT_MAKE_DEP("");
        return;
    }

    {
        char * pz = AGALOC(strlen(mdep) + 5, "mdep");
        char * fp = pz;

        *(pz++) = 'F';
        for (;;) {
            int ch = *(unsigned char *)(mdep++);
            if (IS_END_TOKEN_CHAR(ch))
                break;

            *(pz++) = (char)ch;
        }
        *pz = NUL;

        SET_OPT_SAVE_OPTS(fp);
        mdep = SPN_WHITESPACE_CHARS(mdep);
        if (*mdep == NUL)
            return;

        pz = fp;
        *(pz++) = 'T';
        for (;;) {
            int ch = *(unsigned char *)(mdep++);
            if (IS_END_TOKEN_CHAR(ch))
                break;

            *(pz++) = (char)ch;
        }
        *pz = NUL;
        SET_OPT_SAVE_OPTS(fp);
        AGFREE(fp);
    }
}

LOCAL void
process_ag_opts(int arg_ct, char ** arg_vec)
{
    /*
     *  Advance the argument counters and pointers past any
     *  command line options
     */
    {
        int  optCt = optionProcess(&autogenOptions, arg_ct, arg_vec);

        /*
         *  Make sure we have a source file, even if it is "-" (stdin)
         */
        switch (arg_ct - optCt) {
        case 1:
            if (! HAVE_OPT(DEFINITIONS)) {
                OPT_ARG(DEFINITIONS) = *(arg_vec + optCt);
                break;
            }
            /* FALLTHROUGH */

        default:
            usage_message(DOOPT_TOO_MANY_DEFS, ag_pname);
            /* NOTREACHED */

        case 0:
            if (! HAVE_OPT(DEFINITIONS))
                OPT_ARG(DEFINITIONS) = DFT_DEF_INPUT_STR;
            break;
        }
    }

    if ((OPT_VALUE_TRACE > TRACE_NOTHING) && HAVE_OPT(TRACE_OUT))
        open_trace_file(arg_vec, &DESC(TRACE_OUT));

    start_time = time(NULL) - 1;

    if (! HAVE_OPT(TIMEOUT))
        OPT_ARG(TIMEOUT) = (char const *)AG_DEFAULT_TIMEOUT;

    /*
     *  IF the definitions file has been disabled,
     *  THEN a template *must* have been specified.
     */
    if (  (! ENABLED_OPT(DEFINITIONS))
       && (! HAVE_OPT(OVERRIDE_TPL)) )
        AG_ABEND(NO_TEMPLATE_ERR_MSG);

    /*
     *  IF we do not have a base-name option, then we compute some value
     */
    if (! HAVE_OPT(BASE_NAME))
        define_base_name();

    check_make_dep_env();

    if (HAVE_OPT(MAKE_DEP))
        start_dep_file();

    strequate(OPT_ARG(EQUATE));

    /*
     *  IF we have some defines to put in our environment, ...
     */
    if (HAVE_OPT(DEFINE))
        put_defines_into_env();
}

/**
 *  look for a define string.  It may be in our DEFINE option list
 *  (preferred result) or in the environment.  Look up both.
 *
 *  @param[in] de_name   name to look for
 *  @param[in] check_env whether or not to look in environment
 *
 *  @returns a pointer to the string, if found, or NULL.
 */
LOCAL char const *
get_define_str(char const * de_name, bool check_env)
{
    char const **   ppz;
    int     ct;
    if (HAVE_OPT(DEFINE)) {
        ct  = STACKCT_OPT( DEFINE);
        ppz = STACKLST_OPT(DEFINE);

        while (ct-- > 0) {
            char const * pz   = *(ppz++);
            char * pzEq = strchr(pz, '=');
            int    res;

            if (pzEq != NULL)
                *pzEq = NUL;

            res = strcmp(de_name, pz);
            if (pzEq != NULL)
                *pzEq = '=';

            if (res == 0)
                return (pzEq != NULL) ? pzEq+1 : zNil;
        }
    }
    return check_env ? getenv(de_name) : NULL;
}

/**
 *  The following routine scans over quoted text, shifting it in the process
 *  and eliminating the starting quote, ending quote and any embedded
 *  backslashes.  They may be used to embed the quote character in the quoted
 *  text.  The quote character is whatever character the argument is pointing
 *  at when this procedure is called.
 *
 *  @param[in,out] in_q   input quoted string/output unquoted
 *  @returns the address of the byte after the original closing quote.
 */
LOCAL char *
span_quote(char * in_q)
{
    char   qc = *in_q;          /*  Save the quote character type */
    char * dp = in_q++;         /*  Destination pointer           */

    while (*in_q != qc) {
        switch (*dp++ = *in_q++) {
        case NUL:
            return in_q-1;      /* Return address of terminating NUL */

        case '\\':
            if (qc != '\'') {
                int ct = (int)ao_string_cook_escape_char(in_q, dp-1, 0x7F);
                if (dp[-1] == 0x7F)  dp--;
                in_q += ct;

            } else
                switch (*in_q) {
                case '\\':
                case '\'':
                case '#':
                    dp[-1] = *in_q++;
                }
            break;

        default:
            ;
        }
    }

    *dp = NUL;
    return in_q+1; /* Return addr of char after the terminating quote */
}

/**
 *  The following routine skips over quoted text.  The quote character is
 *  whatever character the argument is pointing at when this procedure is
 *  called.
 *
 *  @param[in] qstr   input quoted string/output unquoted
 *  @returns the address of the byte after the original closing quote.
 */
static char const *
skip_quote(char const * qstr)
{
    char qc = *qstr++;        /*  Save the quote character type */

    while (*qstr != qc) {
        switch (*qstr++) {
        case NUL:
            return qstr-1;      /* Return address of terminating NUL */

        case '\\':
            if (qc == '\'') {
                /*
                 *  Single quoted strings process the backquote specially
                 *  only in front of these three characters:
                 */
                switch (*qstr) {
                case '\\':
                case '\'':
                case '#':
                    qstr++;
                }

            } else {
                char p[10];  /* provide a scratch pad for escape processing */
                qstr += ao_string_cook_escape_char(qstr, p, 0x7F);
            } /* if (q == '\'')      */
        }     /* switch (*qstr++)   */
    }         /* while (*qstr != q) */

    return qstr+1; /* Return addr of char after the terminating quote */
}

/**
 * Skip over scheme expression.  We need to find what follows it.
 * Guile will carefully parse it later.
 *
 * @param[in]  scan  where to start search
 * @param[in]  end   point beyond which not to go
 * @returns    character after closing parenthesis or "end",
 * which ever comes first.
 */
LOCAL char const *
skip_scheme(char const * scan,  char const * end)
{
    int  level = 0;

    for (;;) {
        scan = BRK_SCHEME_NOTE_CHARS(scan);
        if (scan >= end)
            return end;

        switch (*(scan++)) {
        case '(':
            level++;
            break;

        case ')':
            if (--level == 0)
                return scan;
            break;

        case '"':
            scan = skip_quote(scan-1);
        }
    }
}

/**
 * scan past an expression.  An expression is either a Scheme
 * expression starting and ending with balanced parentheses,
 * or a quoted string or a sequence of non-whitespace characters.
 * semicolons denote a comment that extends to the next newline.
 *
 * @param [in]  src  input text
 * @param [in]  len  the maximum length to scan over
 *
 * @returns a pointer to the character next after the expression end.
 */
LOCAL char const *
skip_expr(char const * src, size_t len)
{
    char const * end = src + len;

 guess_again:

    src = SPN_WHITESPACE_CHARS(src);
    if (src >= end)
        return end;
    switch (*src) {
    case ';':
        src = strchr(src, NL);
        if (src == NULL)
            return end;
        goto guess_again;

    case '(':
        return skip_scheme(src, end);

    case '"':
    case '\'':
    case '`':
        src = skip_quote(src);
        return (src > end) ? end : src;

    default:
        break;
    }

    src = BRK_WHITESPACE_CHARS(src);
    return (src > end) ? end : src;
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/agUtils.c */
