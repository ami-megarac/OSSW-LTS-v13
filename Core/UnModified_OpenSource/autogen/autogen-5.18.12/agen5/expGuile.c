
/**
 * @file expGuile.c
 *
 *  This module implements the expression functions that should
 *  be part of Guile.
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

/**
 *  As of Guile 1.7.x, access to the NUL terminated string referenced by
 *  an SCM is no longer guaranteed.  Therefore, we must extract the string
 *  into one of our "scribble" buffers.
 *
 * @param  s     the string to convert
 * @param  type  a string describing the string
 * @return a NUL terminated string, or it aborts.
 */
LOCAL char *
ag_scm2zchars(SCM s, const char * type)
{
    size_t len;
    char * buf;
    char * str;

    if (! scm_is_string(s))
        AG_ABEND(aprf(NOT_STR_FMT, type));

    str = scm_to_locale_stringn(s, &len);
    if (len == 0) {
        static char z = NUL;
        return &z;
    }

    buf = scribble_get((ssize_t)len + 1);
    memmove(buf, str, len);
    free(str);
    buf[len] = NUL;
    return buf;
}

/**
 * convert complex Guile type to an enum value.
 * @param typ the SCM for which we wish to know the type
 * @returns teGuileType -- our own enumeration, since Guile does not have one.
 */
LOCAL teGuileType
ag_scm_type_e(SCM typ)
{
    if (scm_is_bool(    typ)) return GH_TYPE_BOOLEAN;
    if (scm_is_symbol(   typ)) return GH_TYPE_SYMBOL;
    if (scm_is_string(typ)) return GH_TYPE_STRING;
    if (AG_SCM_IS_PROC( typ)) return GH_TYPE_PROCEDURE;
    if (AG_SCM_CHAR_P(  typ)) return GH_TYPE_CHAR;
    if (scm_is_vector(   typ)) return GH_TYPE_VECTOR;
    if (AG_SCM_PAIR_P(  typ)) return GH_TYPE_PAIR;
    if (scm_is_number(  typ)) return GH_TYPE_NUMBER;
    if (AG_SCM_LIST_P(  typ)) return GH_TYPE_LIST;

    return GH_TYPE_UNDEFINED;
}


LOCAL SCM
ag_scm_c_eval_string_from_file_line(
    char const * pzExpr, char const * pzFile, int line)
{
    SCM port = scm_open_input_string(scm_from_latin1_string(pzExpr));

    if (OPT_VALUE_TRACE >= TRACE_EVERYTHING)
        fprintf(trace_fp, TRACE_EVAL_STRING, pzFile, line, pzExpr);

    {
        static SCM    file      = SCM_UNDEFINED;
        static char * pzOldFile = NULL;

        if ((pzOldFile == NULL) || (strcmp(pzOldFile, pzFile) != 0)) {
            if (pzOldFile != NULL)
                AGFREE(pzOldFile);
            AGDUPSTR(pzOldFile, pzFile, "scheme source");
            file = scm_from_latin1_string(pzFile);
        }

        {
            SCM ln = scm_from_int(line);
            scm_set_port_filename_x(port, file);
            scm_set_port_line_x(port, ln);
            scm_set_port_column_x(port, SCM_INUM0);
        }
    }

    {
        SCM ans = SCM_UNSPECIFIED;

        /* Read expressions from that port; ignore the values.  */
        for (;;) {
            SCM form = scm_read(port);
            if (SCM_EOF_OBJECT_P(form))
                break;
            ans = scm_primitive_eval_x(form);
        }

        return ans;
    }
}


/*=gfunc max
 *
 * what:   maximum value in list
 * general_use:
 *
 * exparg: list , list of values.  Strings are converted to numbers ,, list
 *
 * doc:  Return the maximum value in the list
=*/
SCM
ag_scm_max(SCM list)
{
    int   len;
    SCM   car;
    long  max_val = LONG_MIN;
#   ifndef MAX
#     define MAX(m,v) ((v > m) ? v : m)
#   endif
    len = (int)scm_ilength(list);
    if (len <= 0)
        return SCM_UNDEFINED;

    while (--len >= 0) {
        long val;

        car  = SCM_CAR(list);
        list = SCM_CDR(list);

        switch (ag_scm_type_e(car)) {
        case GH_TYPE_BOOLEAN:
            if (car == SCM_BOOL_F) {
                val = 0;
            } else {
                val = 1;
            }
            break;

        case GH_TYPE_CHAR:
            val = (int)SCM_CHAR(car);
            break;

        case GH_TYPE_NUMBER:
            val = AG_SCM_TO_LONG(car);
            break;

        case GH_TYPE_STRING:
            val = strtol(ag_scm2zchars(car, "number-in-string"), NULL, 0);
            break;

        default:
            continue;
        }
        max_val = MAX(max_val, val);
    }

    return scm_from_long(max_val);
}


/*=gfunc min
 *
 * what:   minimum value in list
 * general_use:
 *
 * exparg: list , list of values.  Strings are converted to numbers ,, list
 *
 * doc:  Return the minimum value in the list
=*/
SCM
ag_scm_min(SCM list)
{
    int   len;
    SCM   car;
    long  min_val = LONG_MAX;
#   ifndef MIN
#     define MIN(m,v) ((v < m) ? v : m)
#   endif
    len = (int)scm_ilength(list);
    if (len <= 0)
        return SCM_UNDEFINED;

    while (--len >= 0) {
        long val;

        car  = SCM_CAR(list);
        list = SCM_CDR(list);

        switch (ag_scm_type_e(car)) {
        case GH_TYPE_BOOLEAN:
            if (car == SCM_BOOL_F) {
                val = 0;
            } else {
                val = 1;
            }
            break;

        case GH_TYPE_CHAR:
            val = (int)SCM_CHAR(car);
            break;

        case GH_TYPE_NUMBER:
            val = AG_SCM_TO_LONG(car);
            break;

        case GH_TYPE_STRING:
            val = strtol(ag_scm2zchars(car, "number-in-string"), NULL, 0);
            break;

        default:
            continue;
        }
        min_val = MIN(min_val, val);
    }

    return scm_from_long(min_val);
}


/*=gfunc sum
 *
 * what:   sum of values in list
 * general_use:
 *
 * exparg: list , list of values.  Strings are converted to numbers ,, list
 *
 * doc:  Compute the sum of the list of expressions.
=*/
SCM
ag_scm_sum(SCM list)
{
    int  len = (int)scm_ilength(list);
    long sum = 0;

    if (len <= 0)
        return scm_from_int(0);

    do  {
        SCM  car = SCM_CAR(list);
        list = SCM_CDR(list);
        switch (ag_scm_type_e(car)) {
        default:
            return SCM_UNDEFINED;

        case GH_TYPE_CHAR:
            sum += (long)(unsigned char)SCM_CHAR(car);
            break;

        case GH_TYPE_NUMBER:
            sum += AG_SCM_TO_LONG(car);
            break;

        case GH_TYPE_STRING:
            sum += strtol(ag_scm2zchars(car, "number-in-string"), NULL, 0);
        }
    } while (--len > 0);

    return scm_from_long(sum);
}


/*=gfunc string_to_c_name_x
 *
 * what:   map non-name chars to underscore
 * general_use:
 *
 * exparg: str , input/output string
 *
 * doc:  Change all the graphic characters that are invalid in a C name token
 *       into underscores.  Whitespace characters are ignored.  Any other
 *       character type (i.e. non-graphic and non-white) will cause a failure.
=*/
SCM
ag_scm_string_to_c_name_x(SCM str)
{
    int    len;
    char * pz;

    if (! scm_is_string(str))
        scm_wrong_type_arg(STR_TO_C_NAME, 1, str);

    for (pz  = C(char *, scm_i_string_chars(str)),
         len = (int)scm_c_string_length(str);

         --len >= 0;

         pz++) {

        char ch = *pz;
        if (IS_ALPHANUMERIC_CHAR(ch) || IS_WHITESPACE_CHAR(ch))
            continue;

        if (! IS_GRAPHIC_CHAR(ch))
            scm_misc_error(STR_TO_C_NAME, STR_TO_C_MAP_FAIL, str);

        *pz = '_';
    }

    return str;
}


/*=gfunc string_upcase_x
 *
 * what:   make a string be upper case
 * general_use:
 *
 * exparg: str , input/output string
 *
 * doc:  Change to upper case all the characters in an SCM string.
=*/
SCM
ag_scm_string_upcase_x(SCM str)
{
    int    len;
    char * pz;

    if (! scm_is_string(str))
        return SCM_UNDEFINED;

    len = (int)scm_c_string_length(str);
    pz  = C(char *, scm_i_string_chars(str));
    while (--len >= 0) {
         char ch = *pz;
        if (IS_LOWER_CASE_CHAR(ch))
            *pz = (char)toupper((int)ch);
        pz++;
    }

    return str;
}


/*=gfunc string_upcase
 *
 * what:   upper case a new string
 * general_use:
 *
 * exparg: str , input string
 *
 * doc:  Create a new SCM string containing the same text as the original,
 *       only all the lower case letters are changed to upper case.
=*/
SCM
ag_scm_string_upcase(SCM str)
{
    SCM res;
    if (! scm_is_string(str))
        return SCM_UNDEFINED;

    res = scm_from_latin1_stringn(
        scm_i_string_chars(str), scm_c_string_length(str));
    scm_string_upcase_x(res);
    return res;
}


/*=gfunc string_capitalize_x
 *
 * what:   capitalize a string
 * general_use:
 *
 * exparg: str , input/output string
 *
 * doc:  capitalize all the words in an SCM string.
=*/
SCM
ag_scm_string_capitalize_x(SCM str)
{
    int     len;
    char *  pz;
    bool    w_start = true;

    if (! scm_is_string(str))
        return SCM_UNDEFINED;

    len = (int)scm_c_string_length(str);
    pz  = C(char *, scm_i_string_chars(str));

    while (--len >= 0) {
        char ch = *pz;

        if (! IS_ALPHANUMERIC_CHAR(ch)) {
            w_start = true;

        } else if (w_start) {
            w_start = false;
            if (IS_LOWER_CASE_CHAR(ch))
                *pz = (char)toupper((int)ch);

        } else if (IS_UPPER_CASE_CHAR(ch))
            *pz = (char)tolower(ch);

        pz++;
    }

    return str;
}


/*=gfunc string_capitalize
 *
 * what:   capitalize a new string
 * general_use:
 *
 * exparg: str , input string
 *
 * doc:  Create a new SCM string containing the same text as the original,
 *       only all the first letter of each word is upper cased and all
 *       other letters are made lower case.
=*/
SCM
ag_scm_string_capitalize(SCM str)
{
    SCM res;
    if (! scm_is_string(str))
        return SCM_UNDEFINED;

    res = scm_from_latin1_stringn(
        scm_i_string_chars(str), scm_c_string_length(str));
    ag_scm_string_capitalize_x(res);
    return res;
}


/*=gfunc string_downcase_x
 *
 * what:   make a string be lower case
 * general_use:
 *
 * exparg: str , input/output string
 *
 * doc:  Change to lower case all the characters in an SCM string.
=*/
SCM
ag_scm_string_downcase_x(SCM str)
{
    int    len;
    char * pz;

    if (! scm_is_string(str))
        return SCM_UNDEFINED;

    len = (int)scm_c_string_length(str);
    pz  = C(char *, scm_i_string_chars(str));
    while (--len >= 0) {
        char ch = *pz;
        if (IS_UPPER_CASE_CHAR(ch))
            *pz = (char)tolower(ch);
        pz++;
    }

    return str;
}


/*=gfunc string_downcase
 *
 * what:   lower case a new string
 * general_use:
 *
 * exparg: str , input string
 *
 * doc:  Create a new SCM string containing the same text as the original,
 *       only all the upper case letters are changed to lower case.
=*/
SCM
ag_scm_string_downcase(SCM str)
{
    SCM res;
    if (! scm_is_string(str))
        return SCM_UNDEFINED;

    res = scm_from_latin1_stringn(
        scm_i_string_chars(str), scm_c_string_length(str));
    ag_scm_string_downcase_x(res);
    return res;
}


/*=gfunc string_to_camelcase
 *
 * what:   make a string be CamelCase
 * general_use:
 *
 * exparg: str , input/output string
 *
 * doc:  Capitalize the first letter of each block of letters and numbers,
 *       and stripping out characters that are not alphanumerics.
 *       For example, "alpha-beta0gamma" becomes "AlphaBeta0gamma".
=*/
SCM
ag_scm_string_to_camelcase(SCM str)
{
    int    len;
    char * pzd, * res;
    char * pzs;
    bool   cap_done = false;

    if (! scm_is_string(str))
        return SCM_UNDEFINED;

    len = (int)scm_c_string_length(str);
    res = pzd = scribble_get(len + 1);
    pzs = C(char *, scm_i_string_chars(str));

    while (--len >= 0) {
        unsigned int ch = (unsigned int)*(pzs++);
        if (IS_LOWER_CASE_CHAR(ch)) {
            if (! cap_done) {
                ch = (unsigned)toupper((int)ch);
                cap_done = true;
            }

        } else if (IS_UPPER_CASE_CHAR(ch)) {
            if (cap_done)
                ch = (unsigned)tolower((int)ch);
            else
                cap_done = true;

        } else if (IS_DEC_DIGIT_CHAR(ch)) {
            cap_done = false;

        } else {
            cap_done = false;
            continue;
        }

        *(pzd++) = (char)ch;
    }

    return scm_from_latin1_stringn(res, (size_t)(pzd - res));
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/expGuile.c */
