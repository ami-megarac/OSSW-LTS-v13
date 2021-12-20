
/**
 * @file expPrint.c
 *
 *  The following code is necessary because the user can give us
 *  a printf format requiring a string pointer yet fail to provide
 *  a valid pointer, thus it will fault.  This code protects
 *  against the fault so an error message can be emitted instead of
 *  a core dump :-)
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
#ifndef DEBUG_ENABLED
static sigjmp_buf printJumpEnv;
static int        printJumpSignal = 0;
#endif

/* = = = START-STATIC-FORWARD = = = */
static ssize_t
safePrintf(char ** ppzBuf, char const * pzFmt, void ** argV);
/* = = = END-STATIC-FORWARD = = = */

#ifndef DEBUG_ENABLED
 static void printFault(int sig)
{
    printJumpSignal = sig;
    siglongjmp(printJumpEnv, sig);
}
#endif

static ssize_t
safePrintf(char ** ppzBuf, char const * pzFmt, void ** argV)
{
#if ! defined(DEBUG_ENABLED)
    /*
     *  In normal operation (or during AutoGen testing), seg faults during the
     *  printf operation are caused by bad input data.  During AutoGen
     *  development, we do not supply bad printf arguments, so we want to
     *  capture any segfaults when they happen with the correct stack trace.
     *  Therefore, during AutoGen development, we do not protect against seg
     *  faults.
     */
    struct sigaction  saSave1;
    struct sigaction  saSave2;

    {
        struct sigaction  sa;
        sa.sa_handler = printFault;
        sa.sa_flags   = 0;
        sigemptyset(&sa.sa_mask);

        sigaction(SIGBUS,  &sa, &saSave1);
        sigaction(SIGSEGV, &sa, &saSave2);
    }

    /*
     *  IF the sprintfv call below is going to address fault,
     *  THEN ...
     */
    if (sigsetjmp(printJumpEnv, 0) != 0) {
#ifndef HAVE_STRSIGNAL
        extern char * strsignal(int signo);
#endif
        /*
         *  IF the fprintf command in the then clause has not failed yet,
         *  THEN perform that fprintf
         */
        if (sigsetjmp(printJumpEnv, 0) == 0)
            fprintf(trace_fp, SAFE_PRINTF_BAD_FMT, ag_pname,
                    strsignal(printJumpSignal), pzFmt);

        /*
         *  The "sprintfv" command below faulted, so we exit
         */
        AG_ABEND(SAFE_PRINTF_BAD_ARGS);
    }
#endif /* ! defined(DEBUG_ENABLED) */

    {
        int p_sz = asprintfv(ppzBuf, pzFmt, (snv_constpointer *)argV);
        if (((unsigned)p_sz & ~0xFFFFFU) != 0) /* 1MB max */
            AG_ABEND(aprf(ASPRINTFV_FAIL_FMT, p_sz));

#if ! defined(DEBUG_ENABLED)
        sigaction(SIGBUS,  &saSave1, NULL);
        sigaction(SIGSEGV, &saSave2, NULL);
#endif
        return p_sz;
    }
}


LOCAL SCM
run_printf(char const * pzFmt, int len, SCM alist)
{
    SCM     res;
    void *  args[8];
    void ** arglist;
    void ** argp;

    if (len < 8)
        arglist = argp = args;
    else {
        size_t sz = (size_t)((unsigned)(len+1) * sizeof(void *));
        arglist = argp = malloc(sz);
    }

    while (len-- > 0) {
        SCM  car = SCM_CAR(alist);
        alist = SCM_CDR(alist);
        switch (ag_scm_type_e(car)) {
        default:
        case GH_TYPE_UNDEFINED:
            *(argp++) = (char *)RUN_PRINTF_HUH;
            break;

        case GH_TYPE_BOOLEAN:
            *(argp++) = VOIDP((car == SCM_BOOL_F)
                              ? SCM_FALSE_STR : SCM_TRUE_STR);
            break;

        case GH_TYPE_CHAR:
            *(char *)(argp++) = (char)SCM_CHAR(car);
            break;

        case GH_TYPE_PAIR:
            *(argp++) = (char *)(SCM_LIST_STR+1);
            break;

        case GH_TYPE_NUMBER:
            *(unsigned long *)(argp++) = AG_SCM_TO_ULONG(car);
            break;

        case GH_TYPE_SYMBOL:
        case GH_TYPE_STRING:
            *(argp++) = ag_scm2zchars(car, "printf str");
            break;

        case GH_TYPE_PROCEDURE:
            *(argp++) = (char *)SCM_PROC_CAST;
            break;

        case GH_TYPE_VECTOR:
        case GH_TYPE_LIST:
            *(argp++) = (char *)SCM_LIST_STR;
            break;
        }
    }

    /*
     *  Do the formatting and allocate a new SCM to hold the result.
     *  Free up any allocations made by ``gh_scm2newstr''
     */
    {
        char *  bf;
        size_t  bf_sz = (size_t)safePrintf(&bf, pzFmt, arglist);
        res = scm_from_latin1_stringn(bf, bf_sz);
        free(bf);
    }

    if (arglist != args)
        AGFREE(arglist);

    return res;
}


/*=gfunc sprintf
 *
 * what:  format a string
 * general_use:
 *
 * exparg: format, formatting string
 * exparg: format-arg, list of arguments to formatting string, opt, list
 *
 * doc:  Format a string using arguments from the alist.
=*/
SCM
ag_scm_sprintf(SCM fmt, SCM alist)
{
    int    list_len = (int)scm_ilength(alist);
    char * pzFmt    = ag_scm2zchars(fmt, WORD_FORMAT);

    if (list_len <= 0)
        return fmt;

    return run_printf(pzFmt, list_len, alist);
}

/*=gfunc printf
 *
 * what:  format to stdout
 * general_use:
 *
 * exparg: format, formatting string
 * exparg: format-arg, list of arguments to formatting string, opt, list
 *
 * doc:  Format a string using arguments from the alist.
 *       Write to the standard out port.  The result will NOT appear in your
 *       output.  Use this to print information messages to a template user.
 *       Use ``(sprintf ...)'' to add text to your document.
=*/
SCM
ag_scm_printf(SCM fmt, SCM alist)
{
    int    list_len = (int)scm_ilength(alist);
    char * pzFmt    = ag_scm2zchars(fmt, WORD_FORMAT);

    AG_SCM_DISPLAY(run_printf(pzFmt, list_len, alist));
    return SCM_UNDEFINED;
}

/*=gfunc fprintf
 *
 * what:  format to a file
 * general_use:
 *
 * exparg: port, Guile-scheme output port
 * exparg: format, formatting string
 * exparg: format-arg, list of arguments to formatting string, opt, list
 *
 * doc:  Format a string using arguments from the alist.
 *       Write to a specified port.  The result will NOT appear in your
 *       output.  Use this to print information messages to a template user.
=*/
SCM
ag_scm_fprintf(SCM port, SCM fmt, SCM alist)
{
    int    list_len = (int)scm_ilength(alist);
    char * pzFmt    = ag_scm2zchars(fmt, WORD_FORMAT);
    SCM    res      = run_printf(pzFmt, list_len, alist);

    return  scm_display(res, port);
}

/*=gfunc hide_email
 *
 * what:  convert eaddr to javascript
 * general_use:
 *
 * exparg: display, display text
 * exparg: eaddr,   email address
 *
 * doc:    Hides an email address as a java scriptlett.
 *         The 'mailto:' tag and the email address are coded bytes
 *         rather than plain text.  They are also broken up.
=*/
SCM
ag_scm_hide_email(SCM display, SCM eaddr)
{
    char *  disp    = ag_scm2zchars(display, "fmt");
    char *  end_adr = ag_scm2zchars(eaddr,   "eaddr");

    ssize_t str_size = (ssize_t)(
        (strlen(end_adr) * 6) // e.g. "&#107;"
        + HIDE_EMAIL_START_STR_LEN
        + HIDE_EMAIL_END_FMT_LEN + strlen(disp));

    char *  res  = scribble_get(str_size);
    char *  scan = res;
    char *  end  = res + (str_size - 1);

    memcpy(scan, HIDE_EMAIL_START_STR, HIDE_EMAIL_START_STR_LEN);
    scan += HIDE_EMAIL_START_STR_LEN;

    do {
        if (*end_adr == NUL)
            break;
        scan += snprintf(scan, str_size - (scan - res),
                         HTML_DEC_DIGIT, *(end_adr++));
    } while (scan < end);

    scan += snprintf(scan, str_size - (scan - res), HIDE_EMAIL_END_FMT, disp);
    if (scan > end)
        AG_ABEND(BOGUS_TAG);

    return scm_from_latin1_stringn(res, (size_t)(scan - res));
}

/*=gfunc   format_arg_count
 *
 * what:   count the args to a format
 * general_use:
 *
 * exparg: format, formatting string
 *
 * doc:    "Sometimes, it is useful to simply be able to figure out how many\n"
 *         "arguments are required by a format string.  For example, if you\n"
 *         "are extracting a format string for the purpose of generating a\n"
 *         "macro to invoke a printf-like function, you can run the\n"
 *         "formatting string through this function to determine how many\n"
 *         "arguments to provide for in the macro. e.g. for this extraction\n"
 *         "text:\n"
 *         "@example\n\n"
 *         " /" "*=fumble bumble\n"
 *         "  * fmt: 'stumble %s: %d\\n'\n"
 *         " =*" "/\n"
 *         "@end example\n\n"
 *         "@noindent\n"
 *         "You may wish to generate a macro:\n"
 *         "@example\n\n"
 *         " #define BUMBLE(a1,a2) printf_like(something,(a1),(a2))\n"
 *         "@end example\n\n"
 *         "@noindent\n"
 *         "You can do this by knowing that the format needs two arguments.\n"
=*/
SCM
ag_scm_format_arg_count(SCM fmt)
{
    char * pzFmt = ag_scm2zchars(fmt, WORD_FORMAT);
    int   ct    = 0;
    for (;;) {
        switch (*(pzFmt++)) {
        case NUL: goto scanDone;
        case '%':
            if (*pzFmt == '%')
                 pzFmt++;
            else ct++;
        }
    } scanDone:;

    return scm_from_int(ct);
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/expPrint.c */
