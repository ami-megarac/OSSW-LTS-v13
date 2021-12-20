
/**
 * @file funcCase.c
 *
 *  This module implements the CASE text function.
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

#ifndef _toupper
#  ifdef __toupper
#    define _toupper(c)     __toupper(c)
#  else
#    define _toupper(c)     toupper(c)
#  endif
#endif

typedef tSuccess (tSelectProc)(char const * sample, char const * pattern);
static tSelectProc
    Select_Compare,
    Select_Compare_End,
    Select_Compare_Start,
    Select_Compare_Full,
    Select_Equivalent,
    Select_Equivalent_End,
    Select_Equivalent_Start,
    Select_Equivalent_Full,
    Select_Match,
    Select_Match_End,
    Select_Match_Start,
    Select_Match_Full,
    Select_Match_Always;

/*
 *  This is global data used to keep track of the current CASE
 *  statement being processed.  When CASE statements nest,
 *  these data are copied onto the stack and restored when
 *  the nested CASE statement's ESAC function is found.
 */
typedef struct case_stack tCaseStack;
struct case_stack {
    macro_t *  pCase;
    macro_t *  pSelect;
};

static tCaseStack current_case;
static load_proc_t  mLoad_Select;

static load_proc_p_t apCaseLoad[ FUNC_CT ]   = { NULL };
static load_proc_p_t apSelectOnly[ FUNC_CT ] = { NULL };

/* = = = START-STATIC-FORWARD = = = */
static void
compile_re(regex_t * re, char const * pat, int flags);

static inline void
up_case(char * pz);

static tSuccess
Select_Compare(char const * sample, char const * pattern);

static tSuccess
Select_Compare_End(char const * sample, char const * pattern);

static tSuccess
Select_Compare_Start(char const * sample, char const * pattern);

static tSuccess
Select_Compare_Full(char const * sample, char const * pattern);

static tSuccess
Select_Equivalent(char const * sample, char const * pattern);

static tSuccess
Select_Equivalent_End(char const * sample, char const * pattern);

static tSuccess
Select_Equivalent_Start(char const * sample, char const * pattern);

static tSuccess
Select_Equivalent_Full(char const * sample, char const * pattern);

static tSuccess
Select_Match(char const * sample, char const * pattern);

static tSuccess
Select_Match_End(char const * sample, char const * pattern);

static tSuccess
Select_Match_Start(char const * sample, char const * pattern);

static tSuccess
Select_Match_Full(char const * sample, char const * pattern);

static tSuccess
Select_Match_Always(char const * sample, char const * pattern);

static tSuccess
Select_Match_Existence(char const * sample, char const * pattern);

static tSuccess
Select_Match_NonExistence(char const * sample, char const * pattern);

static bool
selection_type_complete(templ_t * tpl, macro_t * mac, char const ** psrc);

static macro_t *
mLoad_Select(templ_t * tpl, macro_t * mac, char const ** pscan);
/* = = = END-STATIC-FORWARD = = = */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void
compile_re(regex_t * re, char const * pat, int flags)
{
    int rerr = regcomp(re, VOIDP(pat), flags);
    if (rerr != 0) {
        char erbf[ SCRIBBLE_SIZE ];
        regerror(rerr, re, erbf, sizeof(erbf));
        fprintf(stderr, BAD_RE_FMT, rerr, erbf, pat);
        AG_ABEND(COMPILE_RE_BAD);
    }
}

static inline void
up_case(char * pz)
{
    while (*pz != NUL) {
        if (IS_LOWER_CASE_CHAR(*pz))
            *pz = (char)_toupper((int)*pz);
        pz++;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_contains_p
 *
 * what:   substring match
 * general_use:
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "*==*"
 *
 * doc:  Test to see if a string contains a substring.  "strstr(3)"
 *       will find an address.
=*/
static tSuccess
Select_Compare(char const * sample, char const * pattern)
{
    return (strstr(sample, pattern)) ? SUCCESS : FAILURE;
}

SCM
ag_scm_string_contains_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");

    return (strstr(pzText, pzSubstr) == NULL) ? SCM_BOOL_F : SCM_BOOL_T;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_ends_with_p
 *
 * what:   string ending
 * general_use:
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "*=="
 *
 * doc:  Test to see if a string ends with a substring.
 *       strcmp(3) returns zero for comparing the string ends.
=*/
static tSuccess
Select_Compare_End(char const * sample, char const * pattern)
{
    size_t   vlen = strlen(pattern);
    size_t   tlen = strlen(sample);
    tSuccess res;

    if (tlen < vlen)
        res = FAILURE;
    else if (strcmp(sample + (tlen - vlen), pattern) == 0)
         res = SUCCESS;
    else res = FAILURE;

    return res;
}

SCM
ag_scm_string_ends_with_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    return (SUCCESSFUL(Select_Compare_End(pzText, pzSubstr)))
        ? SCM_BOOL_T : SCM_BOOL_F;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_starts_with_p
 *
 * what:   string starting
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "==*"
 *
 * doc:  Test to see if a string starts with a substring.
=*/
static tSuccess
Select_Compare_Start(char const * sample, char const * pattern)
{
    size_t  vlen = strlen(pattern);
    tSuccess res;

    if (strncmp(sample, pattern, vlen) == 0)
         res = SUCCESS;
    else res = FAILURE;

    return res;
}

SCM
ag_scm_string_starts_with_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    return (SUCCESSFUL(Select_Compare_Start(pzText, pzSubstr)))
        ? SCM_BOOL_T : SCM_BOOL_F;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_equals_p
 *
 * what:   string matching
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "=="
 *
 * doc:  Test to see if two strings exactly match.
=*/
static tSuccess
Select_Compare_Full(char const * sample, char const * pattern)
{
    return (strcmp(sample, pattern) == 0) ? SUCCESS : FAILURE;
}

SCM
ag_scm_string_equals_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");

    return (strcmp(pzText, pzSubstr) == 0) ? SCM_BOOL_T : SCM_BOOL_F;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_contains_eqv_p
 *
 * what:   caseless substring
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "*=*"
 *
 * doc:  Test to see if a string contains an equivalent string.
 *       `equivalent' means the strings match, but without regard
 *       to character case and certain characters are considered `equivalent'.
 *       Viz., '-', '_' and '^' are equivalent.
=*/
static tSuccess
Select_Equivalent(char const * sample, char const * pattern)
{
    char *   pz;
    tSuccess res = SUCCESS;
    AGDUPSTR(pz, sample, "equiv chars");
    up_case(pz);
    if (strstr(pz, pattern) == NULL)
        res = FAILURE;
    AGFREE(pz);

    return res;
}

SCM
ag_scm_string_contains_eqv_p(SCM text, SCM substr)
{
    char * pzSubstr;
    SCM    res;

    AGDUPSTR(pzSubstr, ag_scm2zchars(substr, "search"), "substr");

    up_case(pzSubstr);
    if (SUCCESSFUL(Select_Equivalent(ag_scm2zchars(text, "sample"),
                                     pzSubstr)))
         res = SCM_BOOL_T;
    else res = SCM_BOOL_F;
    AGFREE(pzSubstr);
    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_ends_eqv_p
 *
 * what:   caseless string ending
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "*="
 *
 * doc:  Test to see if a string ends with an equivalent string.
=*/
static tSuccess
Select_Equivalent_End(char const * sample, char const * pattern)
{
    size_t   vlen = strlen(pattern);
    size_t   tlen = strlen(sample);

    if (tlen < vlen)
        return FAILURE;

    return (streqvcmp(sample + (tlen - vlen), pattern) == 0)
           ? SUCCESS
           : FAILURE;
}

SCM
ag_scm_string_ends_eqv_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    return (SUCCESSFUL(Select_Equivalent_End( pzText, pzSubstr )))
        ? SCM_BOOL_T : SCM_BOOL_F;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_starts_eqv_p
 *
 * what:   caseless string start
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "=*"
 *
 * doc:  Test to see if a string starts with an equivalent string.
=*/
static tSuccess
Select_Equivalent_Start(char const * sample, char const * pattern)
{
    size_t   vlen = strlen(pattern);

    return (strneqvcmp(sample, pattern, (int)vlen) == 0)
           ? SUCCESS
           : FAILURE;
}

SCM
ag_scm_string_starts_eqv_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    return (SUCCESSFUL(Select_Equivalent_Start(pzText, pzSubstr)))
        ? SCM_BOOL_T : SCM_BOOL_F;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_eqv_p
 *
 * what:   caseless match
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "="
 *
 * doc:  Test to see if two strings are equivalent.  `equivalent' means the
 *       strings match, but without regard to character case and certain
 *       characters are considered `equivalent'.  Viz., '-', '_' and '^' are
 *       equivalent.  If the arguments are not strings, then the result of the
 *       numeric comparison is returned.
 *
 *       This is an overloaded operation.  If the arguments are both
 *       numbers, then the query is passed through to @code{scm_num_eq_p()},
 *       otherwise the result depends on the SCMs being strictly equal.
=*/
static tSuccess
Select_Equivalent_Full(char const * sample, char const * pattern)
{
    return (streqvcmp(sample, pattern) == 0) ? SUCCESS : FAILURE;
}

SCM
ag_scm_string_eqv_p(SCM text, SCM substr)
{
    /*
     *  We are overloading the "=" operator.  Our arguments may be
     *  numbers or booleans...
     */
    teGuileType tt = ag_scm_type_e(text);
    {
        teGuileType st = ag_scm_type_e(substr);
        if (st != tt)
            return SCM_BOOL_F;
    }

    switch (tt) {
    case GH_TYPE_NUMBER:
        return scm_num_eq_p(text, substr);

    case GH_TYPE_STRING:
    {
        char * pzText   = ag_scm2zchars(text,   "text");
        char * pzSubstr = ag_scm2zchars(substr, "m expr");
        return (streqvcmp(pzText, pzSubstr) == 0) ? SCM_BOOL_T : SCM_BOOL_F;
    }

    case GH_TYPE_BOOLEAN:
    default:
        return (text == substr) ? SCM_BOOL_T : SCM_BOOL_F;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_has_match_p
 *
 * what:   contained regex match
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "*~~*"
 *
 * doc:  Test to see if a string contains a pattern.
 *       Case is significant.
=*/
/*=gfunc string_has_eqv_match_p
 *
 * what:   caseless regex contains
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "*~*"
 *
 * doc:  Test to see if a string contains a pattern.
 *       Case is not significant.
=*/
static tSuccess
Select_Match(char const * sample, char const * pattern)
{
    /*
     *  On the first call for this macro, compile the expression
     */
    if (cur_macro->md_pvt == NULL) {
        void *    mat = VOIDP(pattern);
        regex_t * re  = AGALOC(sizeof(*re), "select match re");
        compile_re(re, mat, (int)cur_macro->md_res);
        cur_macro->md_pvt = VOIDP(re);
    }

    if (regexec((regex_t *)cur_macro->md_pvt, sample, (size_t)0,
                 NULL, 0) != 0)
        return FAILURE;
    return SUCCESS;
}

SCM
ag_scm_string_has_match_p(SCM text, SCM substr)
{
    SCM      res;
    regex_t  re;

    compile_re(&re, ag_scm2zchars( substr, "match expr" ), REG_EXTENDED);

    if (regexec(&re, ag_scm2zchars(text, "text to match"), (size_t)0,
                 NULL, 0) == 0)
         res = SCM_BOOL_T;
    else res = SCM_BOOL_F;
    regfree(&re);

    return res;
}

SCM
ag_scm_string_has_eqv_match_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    SCM      res;
    regex_t  re;

    compile_re(&re, pzSubstr, REG_EXTENDED | REG_ICASE);

    if (regexec(&re, pzText, (size_t)0, NULL, 0) == 0)
         res = SCM_BOOL_T;
    else res = SCM_BOOL_F;
    regfree(&re);

    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_end_match_p
 *
 * what:   regex match end
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "*~~"
 *
 * doc:  Test to see if a string ends with a pattern.
 *       Case is significant.
=*/
/*=gfunc string_end_eqv_match_p
 *
 * what:   caseless regex ending
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "*~"
 *
 * doc:  Test to see if a string ends with a pattern.
 *       Case is not significant.
=*/
static tSuccess
Select_Match_End(char const * sample, char const * pattern)
{
    regmatch_t m[2];
    /*
     *  On the first call for this macro, compile the expression
     */
    if (cur_macro->md_pvt == NULL) {
        void *    mat = VOIDP(pattern);
        regex_t * re  = AGALOC(sizeof(*re), "select match end re");
        compile_re(re, mat, (int)cur_macro->md_res);
        cur_macro->md_pvt = VOIDP(re);
    }

    if (regexec((regex_t *)cur_macro->md_pvt, sample, (size_t)2, m, 0)
        != 0)
        return FAILURE;
    if (m[0].rm_eo != (int)strlen(sample))
        return FAILURE;
    return SUCCESS;
}

SCM
ag_scm_string_end_match_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    SCM        res;
    regex_t    re;
    regmatch_t m[2];

    compile_re(&re, pzSubstr, REG_EXTENDED);

    if (regexec(&re, pzText, (size_t)2, m, 0) != 0)
         res = SCM_BOOL_F;
    else if (m[0].rm_eo != (int)strlen(pzText))
         res = SCM_BOOL_F;
    else res = SCM_BOOL_T;

    regfree(&re);

    return res;
}

SCM
ag_scm_string_end_eqv_match_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    SCM        res;
    regex_t    re;
    regmatch_t m[2];

    compile_re(&re, pzSubstr, REG_EXTENDED | REG_ICASE);

    if (regexec(&re, pzText, (size_t)2, m, 0) != 0)
         res = SCM_BOOL_F;
    else if (m[0].rm_eo != (int)strlen(pzText))
         res = SCM_BOOL_F;
    else res = SCM_BOOL_T;

    regfree(&re);

    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_start_match_p
 *
 * what:   regex match start
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "~~*"
 *
 * doc:  Test to see if a string starts with a pattern.
 *       Case is significant.
=*/
/*=gfunc string_start_eqv_match_p
 *
 * what:   caseless regex start
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "~*"
 *
 * doc:  Test to see if a string starts with a pattern.
 *       Case is not significant.
=*/
static tSuccess
Select_Match_Start(char const * sample, char const * pattern)
{
    regmatch_t m[2];
    /*
     *  On the first call for this macro, compile the expression
     */
    if (cur_macro->md_pvt == NULL) {
        void *    mat = VOIDP(pattern);
        regex_t * re  = AGALOC(sizeof(*re), "select match start re");
        compile_re(re, mat, (int)cur_macro->md_res);
        cur_macro->md_pvt = VOIDP(re);
    }

    if (regexec((regex_t *)cur_macro->md_pvt, sample, (size_t)2, m, 0)
        != 0)
        return FAILURE;
    if (m[0].rm_so != 0)
        return FAILURE;
    return SUCCESS;
}

SCM
ag_scm_string_start_match_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    SCM        res;
    regex_t    re;
    regmatch_t m[2];

    compile_re(&re, pzSubstr, REG_EXTENDED);

    if (regexec(&re, pzText, (size_t)2, m, 0) != 0)
         res = SCM_BOOL_F;
    else if (m[0].rm_so != 0)
         res = SCM_BOOL_F;
    else res = SCM_BOOL_T;

    regfree(&re);

    return res;
}

SCM
ag_scm_string_start_eqv_match_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    SCM        res;
    regex_t    re;
    regmatch_t m[2];

    compile_re(&re, pzSubstr, REG_EXTENDED | REG_ICASE);

    if (regexec(&re, pzText, (size_t)2, m, 0) != 0)
         res = SCM_BOOL_F;
    else if (m[0].rm_so != 0)
         res = SCM_BOOL_F;
    else res = SCM_BOOL_T;

    regfree(&re);

    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=gfunc string_match_p
 *
 * what:   regex match
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "~~"
 *
 * doc:  Test to see if a string fully matches a pattern.
 *       Case is significant.
=*/
/*=gfunc string_eqv_match_p
 *
 * what:   caseless regex match
 * general_use:
 *
 * exparg: text, text to test for pattern
 * exparg: match, pattern/substring to search for
 *
 * string: "~"
 *
 * doc:  Test to see if a string fully matches a pattern.
 *       Case is not significant, but any character equivalences
 *       must be expressed in your regular expression.
=*/
static tSuccess
Select_Match_Full(char const * sample, char const * pattern)
{
    regmatch_t m[2];

    /*
     *  On the first call for this macro, compile the expression
     */
    if (cur_macro->md_pvt == NULL) {
        void *    mat = VOIDP(pattern);
        regex_t * re  = AGALOC(sizeof(*re), "select match full re");

        if (OPT_VALUE_TRACE > TRACE_EXPRESSIONS) {
            fprintf(trace_fp, TRACE_SEL_MATCH_FULL,
                     pattern, cur_macro->md_res);
        }
        compile_re(re, mat, (int)cur_macro->md_res);
        cur_macro->md_pvt = re;
    }

    if (regexec((regex_t *)cur_macro->md_pvt, sample, (size_t)2, m, 0)
        != 0)
        return FAILURE;

    if (  (m[0].rm_eo != (int)strlen( sample ))
       || (m[0].rm_so != 0))
        return FAILURE;
    return SUCCESS;
}

SCM
ag_scm_string_match_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    SCM        res;
    regex_t    re;
    regmatch_t m[2];

    compile_re(&re, pzSubstr, REG_EXTENDED);

    if (regexec(&re, pzText, (size_t)2, m, 0) != 0)
         res = SCM_BOOL_F;
    else if (  (m[0].rm_eo != (int)strlen(pzText))
            || (m[0].rm_so != 0) )
         res = SCM_BOOL_F;
    else res = SCM_BOOL_T;

    regfree(&re);

    return res;
}

SCM
ag_scm_string_eqv_match_p(SCM text, SCM substr)
{
    char * pzText   = ag_scm2zchars(text, "text to match");
    char * pzSubstr = ag_scm2zchars(substr, "match expr");
    SCM        res;
    regex_t    re;
    regmatch_t m[2];

    compile_re(&re, pzSubstr, REG_EXTENDED | REG_ICASE);

    if (regexec(&re, pzText, (size_t)2, m, 0) != 0)
         res = SCM_BOOL_F;
    else if (  (m[0].rm_eo != (int)strlen(pzText))
            || (m[0].rm_so != 0) )
         res = SCM_BOOL_F;
    else res = SCM_BOOL_T;

    regfree(&re);

    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 *  We don't bother making a Guile function for any of these :)
 */
static tSuccess
Select_Match_Always(char const * sample, char const * pattern)
{
    (void)sample;
    (void)pattern;
    return SUCCESS;
}

/**
 *  If the "sample" addresses "zNil", then we couldn't find a value and
 *  defaulted to an empty string.  So, the result is true if the sample
 *  address is anything except "zNil".
 */
static tSuccess
Select_Match_Existence(char const * sample, char const * pattern)
{
    (void)pattern;
    return (sample != no_def_str) ? SUCCESS : FAILURE;
}

/**
 *  If the "sample" addresses "zUndefined", then we couldn't find a value and
 *  defaulted to an empty string.  So, the result false if the sample address
 *  is anything except "zUndefined".
 */
static tSuccess
Select_Match_NonExistence(char const * sample, char const * pattern)
{
    (void)pattern;
    return (sample == no_def_str) ? SUCCESS : FAILURE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=macfunc CASE
 *
 *  what:   Select one of several template blocks
 *  handler_proc:
 *  load_proc:
 *
 *  desc:
 *
 *  The arguments are evaluated and converted to a string, if necessary.  A
 *  simple name will be interpreted as an AutoGen value name and its value will
 *  be used by the @code{SELECT} macros (see the example below and the
 *  expression evaluation function, @pxref{EXPR}).  The scope of the macro is
 *  up to the matching @code{ESAC} macro.  Within the scope of a @code{CASE},
 *  this string is matched against case selection macros.  There are sixteen
 *  match macros that are derived from four different ways matches may be
 *  performed, plus an "always true", "true if the AutoGen value was found",
 *  and "true if no AutoGen value was found" matches.  The codes for the
 *  nineteen match macros are formed as follows:
 *
 *  @enumerate
 *  @item
 *  Must the match start matching from the beginning of the string?
 *  If not, then the match macro code starts with an asterisk (@code{*}).
 *  @item
 *  Must the match finish matching at the end of the string?
 *  If not, then the match macro code ends with an asterisk (@code{*}).
 *  @item
 *  Is the match a pattern match or a string comparison?
 *  If a comparison, use an equal sign (@code{=}).
 *  If a pattern match, use a tilde (@code{~}).
 *  @item
 *  Is the match case sensitive?
 *  If alphabetic case is important, double the tilde or equal sign.
 *  @item
 *  Do you need a default match when none of the others match?
 *  Use a single asterisk (@code{*}).
 *  @item
 *  Do you need to distinguish between an empty string value and a value
 *  that was not found?  Use the non-existence test (@code{!E}) before
 *  testing a full match against an empty string (@code{== ''}).
 *  There is also an existence test (@code{+E}), more for symmetry than
 *  for practical use.
 *  @end enumerate
 *
 *  @noindent
 *  For example:
 *
 *  @example
 *  [+ CASE <full-expression> +]
 *  [+ ~~*  "[Tt]est" +]reg exp must match at start, not at end
 *  [+ ==   "TeSt"    +]a full-string, case sensitive compare
 *  [+ =    "TEST"    +]a full-string, case insensitive compare
 *  [+ !E             +]not exists - matches if no AutoGen value found
 *  [+ ==   ""        +]expression yielded a zero-length string
 *  [+ +E             +]exists - matches if there is any value result
 *  [+ *              +]always match - no testing
 *  [+ ESAC +]
 *  @end example
 *
 *  @code{<full-expression>} (@pxref{expression syntax}) may be any expression,
 *  including the use of apply-codes and value-names.  If the expression yields
 *  a number, it is converted to a decimal string.
 *
 *  These case selection codes have also been implemented as
 *  Scheme expression functions using the same codes.  They are documented
 *  in this texi doc as ``string-*?'' predicates (@pxref{Common Functions}).
=*/
/*=macfunc ESAC
 *
 *  what:   Terminate the @code{CASE} Template Block
 *  in-context:
 *
 *  desc:
 *    This macro ends the @code{CASE} function template block.
 *    For a complete description, @xref{CASE}.
=*/
macro_t *
mFunc_Case(templ_t * pT, macro_t * pMac)
{
    typedef tSuccess (t_match_proc)(char const *, char const *);
    /*
     *  There are only 15 procedures because the case insenstive matching
     *  get mapped into the previous four.  The last three are "match always",
     *  "match if a value was found" "match if no value found".
     */
    static t_match_proc * const match_procs[] = {
        &Select_Compare_Full,
        &Select_Compare_End,
        &Select_Compare_Start,
        &Select_Compare,

        &Select_Equivalent_Full,
        &Select_Equivalent_End,
        &Select_Equivalent_Start,
        &Select_Equivalent,

        &Select_Match_Full,
        &Select_Match_End,
        &Select_Match_Start,
        &Select_Match,

        &Select_Match_Always,
        &Select_Match_Existence,
        &Select_Match_NonExistence
    };

    static char const * const match_names[] = {
        "COMPARE_FULL",
        "COMPARE_END",
        "COMPARE_START",
        "CONTAINS",

        "EQUIVALENT_FULL",
        "EQUIVALENT_END",
        "EQUIVALENT_START",
        "EQUIV_CONTAINS",

        "MATCH_FULL",
        "MATCH_END",
        "MATCH_START",
        "MATCH_WITHIN",

        "MATCH_ALWAYS",
        "MATCH_EXISTENCE",
        "MATCH_NONEXISTENCE"
    };

    macro_t *    end_mac   = pT->td_macros + pMac->md_end_idx;
    bool free_txt;
    char const * samp_text = eval_mac_expr(&free_txt);

    /*
     *  Search through the selection clauses until we either
     *  reach the end of the list for this CASE macro, or we match.
     */
    for (;;) {
        tSuccess mRes;
        pMac = pT->td_macros + pMac->md_sib_idx;
        if (pMac >= end_mac) {
            if (OPT_VALUE_TRACE >= TRACE_BLOCK_MACROS) {
                fprintf(trace_fp, TRACE_CASE_FAIL, samp_text);

                if (OPT_VALUE_TRACE == TRACE_EVERYTHING)
                    fprintf(trace_fp, TAB_FILE_LINE_FMT,
                            current_tpl->td_file, pMac->md_line);
            }

            break;
        }

        /*
         *  The current macro becomes the selected selection macro
         */
        cur_macro = pMac;
        mRes = (*(match_procs[pMac->md_code & 0x0F])
               )(samp_text, pT->td_text + pMac->md_txt_off);

        /*
         *  IF match, THEN generate and stop looking for a match.
         */
        if (SUCCEEDED(mRes)) {
            if (OPT_VALUE_TRACE >= TRACE_BLOCK_MACROS) {
                fprintf(trace_fp, TRACE_CASE_MATCHED,
                        samp_text,
                        match_names[pMac->md_code & 0x0F],
                        pT->td_text + pMac->md_txt_off);

                if (OPT_VALUE_TRACE == TRACE_EVERYTHING)
                    fprintf(trace_fp, TAB_FILE_LINE_FMT,
                            current_tpl->td_file, pMac->md_line);
            }

            gen_block(pT, pMac + 1, pT->td_macros + pMac->md_sib_idx);
            break;
        }
        else if (OPT_VALUE_TRACE == TRACE_EVERYTHING) {
            fprintf(trace_fp, TRACE_CASE_NOMATCH,
                    samp_text,
                    match_names[pMac->md_code & 0x0F],
                    pT->td_text + pMac->md_txt_off);
        }
    }

    if (free_txt)
        AGFREE(samp_text);

    return end_mac;
}

/*
 *  mLoad_CASE
 *
 *  This function is called to set up (load) the macro
 *  when the template is first read in (before processing).
 */
macro_t *
mLoad_Case(templ_t * pT, macro_t * pMac, char const ** ppzScan)
{
    size_t         srcLen     = (size_t)pMac->md_res;   /* macro len  */
    tCaseStack     save_stack = current_case;
    macro_t *      pEsacMac;

    /*
     *  Save the global macro loading mode
     */
    load_proc_p_t const * papLP = load_proc_table;

    /*
     *  IF there is no associated text expression
     *  THEN woops!  what are we to case on?
     */
    if (srcLen == 0)
        AG_ABEND_IN(pT, pMac, LD_CASE_NO_EXPR);

    /*
     *  Load the expression
     */
    (void)mLoad_Expr(pT, pMac, ppzScan);

    /*
     *  IF this is the first time here,
     *  THEN set up the "CASE" mode callout tables.
     *  It is the standard table, except entries are inserted
     *  for SELECT and ESAC.
     */
    if (apCaseLoad[0] == NULL) {
        int i;

        /*
         *  Until there is a selection clause, only comment and select
         *  macros are allowed.
         */
        for (i=0; i < FUNC_CT; i++)
            apSelectOnly[i] = mLoad_Bogus;

        memcpy(VOIDP(apCaseLoad), base_load_table, sizeof( base_load_table ));
        apSelectOnly[ FTYP_COMMENT] = mLoad_Comment;
        apSelectOnly[ FTYP_SELECT ] = \
        apCaseLoad[   FTYP_SELECT ] = mLoad_Select;
        apCaseLoad[   FTYP_ESAC   ] = mLoad_Ending;
    }

    /*
     *  Set the "select macro only" loading mode
     */
    load_proc_table = apSelectOnly;

    /*
     *  Save global pointers to the current macro entry.
     *  We will need this to link the CASE, SELECT and ESAC
     *  functions together.
     */
    current_case.pCase = current_case.pSelect = pMac;

    /*
     *  Continue parsing the template from this nested level
     */
    pEsacMac = parse_tpl(pMac+1, ppzScan);
    if (*ppzScan == NULL)
        AG_ABEND_IN(pT, pMac, LD_CASE_NO_ESAC);

    /*
     *  Tell the last select macro where its end is.
     *  (It ends with the "next" sibling.  Since there
     *  is no "next" at the end, it is a tiny lie.)
     *
     *  Also, make sure the CASE macro knows where the end is.
     */
    pMac->md_end_idx =
        current_case.pSelect->md_sib_idx = (int)(pEsacMac - pT->td_macros);

    /*
     *  Restore any enclosing CASE function's context.
     */
    current_case = save_stack;

    /*
     *  Restore the global macro loading mode
     */
    load_proc_table  = papLP;

    /*
     *  Return the next available macro descriptor
     */
    return pEsacMac;
}

/**
 *  Figure out the selection type.  Return "true" (it is complete) if
 *  no argument is required.  That is, if it is a "match anything" or
 *  an existence/non-existence test.
 *
 *  @param[in]      tpl  The active template
 *  @param[in,out]  mac  The selection macro structure
 *  @param[out]     psrc The scan pointer for the selection argument
 */
static bool
selection_type_complete(templ_t * tpl, macro_t * mac, char const ** psrc)
{
    char const * src = (char *)mac->md_txt_off;
    mac_func_t fcode = FTYP_SELECT_COMPARE_FULL;

    /*
     *  IF the first character is an asterisk,
     *  THEN the match can start anywhere in the string
     */
    if (*src == '*') {
        src++;
        if (IS_END_TOKEN_CHAR(*src)) {
            mac->md_code = FTYP_SELECT_MATCH_ANYTHING;
            goto leave_done;
        }

        fcode = (mac_func_t)(
            (unsigned int)FTYP_SELECT_COMPARE_FULL |
            (unsigned int)FTYP_SELECT_COMPARE_SKP_START);
    }

    /*
     *  The next character must indicate whether we are
     *  pattern matching ('~') or doing string compares ('=')
     */
    switch (*src++) {
    case '~':
        /*
         *  Or in the pattern matching bit
         */
        fcode = (mac_func_t)(
            (unsigned int)fcode | (unsigned int)FTYP_SELECT_MATCH_FULL);
        mac->md_res = REG_EXTENDED;
        /* FALLTHROUGH */

    case '=':
        /*
         *  IF the '~' or '=' is doubled,
         *  THEN it is a case sensitive match.  Skip over the char.
         *  ELSE or in the case insensitive bit
         */
        if (src[0] == src[-1]) {
            src++;
        } else {
            fcode = (mac_func_t)(
                (unsigned int)fcode |
                (unsigned int)FTYP_SELECT_EQUIVALENT_FULL);
        }
        break;

    case '!':
    case '+':
        switch (*src) {
        case 'e':
        case 'E':
            break;
        default:
            goto bad_sel;
        }
        if (! IS_END_TOKEN_CHAR(src[1]))
            goto bad_sel;

        mac->md_code = (src[-1] == '!')
            ? FTYP_SELECT_MATCH_NONEXISTENCE
            : FTYP_SELECT_MATCH_EXISTENCE;

        goto leave_done;

    default:
    bad_sel:
        AG_ABEND_IN(tpl, mac, LD_SEL_INVAL);
    }

    /*
     *  IF the last character is an asterisk,
     *  THEN the match may end before the test string ends.
     *       OR in the "may end early" bit.
     */
    if (*src == '*') {
        src++;
        fcode = (mac_func_t)(
            (unsigned int)fcode |
            (unsigned int)FTYP_SELECT_COMPARE_SKP_END);
    }

    if (! IS_END_TOKEN_CHAR(*src))
        AG_ABEND_IN(tpl, mac, LD_SEL_INVAL);

    mac->md_code = fcode;
    *psrc = SPN_WHITESPACE_CHARS(src);
    return false;

 leave_done:

    /*
     *  md_code has been set.  Zero out md_txt_off to indicate
     *  no argument text.  NULL the selection argument pointer.
     */
    mac->md_txt_off = 0;
    *psrc = NULL;
    return true;
}

/*=macfunc SELECT
 *
 *  what:    Selection block for CASE function
 *  in-context:
 *  alias:   | ~ | = | * | ! | + |
 *  unload-proc:
 *
 *  desc:
 *    This macro selects a block of text by matching an expression
 *    against the sample text expression evaluated in the @code{CASE}
 *    macro.  @xref{CASE}.
 *
 *    You do not specify a @code{SELECT} macro with the word ``select''.
 *    Instead, you must use one of the 19 match operators described in
 *    the @code{CASE} macro description.
=*/
static macro_t *
mLoad_Select(templ_t * tpl, macro_t * mac, char const ** pscan)
{
    char const *  sel_arg;
    long          arg_len = (long)mac->md_res; /* macro len  */

    (void)pscan;
    /*
     *  Set the global macro loading mode
     */
    load_proc_table = apCaseLoad;
    mac->md_res    = 0;
    if (arg_len == 0)
        AG_ABEND_IN(tpl, mac, LD_SEL_EMPTY);

    if (selection_type_complete(tpl, mac, &sel_arg))
        goto selection_done;

    arg_len -= (intptr_t)(sel_arg - mac->md_txt_off);
    if (arg_len <= 0)
        AG_ABEND_IN(tpl, mac, LD_SEL_INVAL);

    /*
     *  See if we are doing case insensitive regular expressions
     *  Turn off the case comparison mode for regular expressions.
     *  We don't have to worry about it.  It is done for us.
     */
    if (  ((int)mac->md_code & (int)FTYP_SELECT_EQV_MATCH_FULL)
       == (int)FTYP_SELECT_EQV_MATCH_FULL) {

        static unsigned int const bits =
            ~( unsigned int)FTYP_SELECT_EQUIVALENT_FULL
            | (unsigned int)FTYP_SELECT_COMPARE_FULL;

        mac->md_res  = REG_EXTENDED | REG_ICASE;
        mac->md_code = (mac_func_t)((unsigned int)mac->md_code & bits);
    }

    /*
     *  Copy the selection expression, double NUL terminate.
     */
    {
        char *       dest   = tpl->td_scan;
        char const * svdest = dest;
        mac->md_txt_off = (uintptr_t)(dest - tpl->td_text);
        if (mac->md_code == FTYP_SELECT_EQUIVALENT) {
            do  {
                *(dest++) = (char)toupper((uint8_t)*(sel_arg++));
            } while (--arg_len > 0);
        } else {
            memcpy(dest, sel_arg, (size_t)arg_len);
            dest += arg_len;
        }
        *(dest++) = NUL;
        *(dest++) = NUL;
        tpl->td_scan = dest;

        /*
         * If the value is a quoted string, strip the quotes and
         * process the string (backslash fixup).
         */
        if ((*svdest == '"') || (*svdest == '\''))
            span_quote(VOIDP(svdest));
    }

 selection_done:
    /*
     *  Link this selection macro to the list of selectors for CASE.
     */
    current_case.pSelect->md_sib_idx = (int)(mac - tpl->td_macros);
    current_case.pSelect = (macro_t *)mac;

    return mac + 1;
}

/**
 * Free data for a selection macro.  Regular expression selections
 * allocate the compiled re.
 */
void
mUnload_Select(macro_t * mac)
{
    if (mac->md_pvt != NULL) {
        regex_t * regexp = (regex_t *)mac->md_pvt;
        regfree(regexp);
        AGFREE(regexp);
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
 * end of agen5/funcCase.c */
