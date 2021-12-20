
/**
 * @file tpParse.c
 *
 *  This module will load a template and return a template structure.
 *
 * @addtogroup autogen
 * @{
 */
/*
 * This file is part of AutoGen.
 * Copyright (C) 1992-2016 Bruce Korb - all rights reserved
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

#if defined(DEBUG_ENABLED)
static char const zTUndef[] = "%-10s (%d) line %d - MARKER\n";

static int tpl_nest_lvl = 0;

static char const tpl_def_fmt[] = "%-10s (%d) line %d end=%d, strlen=%d\n";
#endif

/* = = = START-STATIC-FORWARD = = = */
static mac_func_t
func_code(char const ** pscan);

static char const *
find_mac_end(char const ** ppzMark);

static char const *
find_mac_start(char const * pz, macro_t ** ppm, templ_t * tpl);

static char const *
find_macro(templ_t * tpl, macro_t ** ppm, char const ** pscan);
/* = = = END-STATIC-FORWARD = = = */

/*
 *  Return the enumerated function type corresponding
 *  to a name pointed to by the input argument.
 */
static mac_func_t
func_code(char const ** pscan)
{
    fn_name_type_t const * pNT;
    char const *      pzFuncName = *pscan;
    int               hi, lo, av;
    int               cmp;

    /*
     *  IF the name starts with a punctuation, then it is some sort of
     *  alias.  Find the function in the alias portion of the table.
     */
    if (IS_PUNCTUATION_CHAR(*pzFuncName)) {
        hi = FUNC_ALIAS_HIGH_INDEX;
        lo = FUNC_ALIAS_LOW_INDEX;
        do  {
            av  = (hi + lo)/2;
            pNT = fn_name_types + av;
            cmp = (int)(*(pNT->pName)) - (int)(*pzFuncName);

            /*
             *  For strings that start with a punctuation, we
             *  do not need to test for the end of token
             *  We will not strip off the marker and the load function
             *  will figure out what to do with the code.
             */
            if (cmp == 0)
                return pNT->fType;
            if (cmp > 0)
                 hi = av - 1;
            else lo = av + 1;
        } while (hi >= lo);
        return FTYP_BOGUS;
    }

    if (! IS_VAR_FIRST_CHAR(*pzFuncName))
        return FTYP_BOGUS;

    hi = FUNC_NAMES_HIGH_INDEX;
    lo = FUNC_NAMES_LOW_INDEX;

    do  {
        av  = (hi + lo)/2;
        pNT = fn_name_types + av;
        cmp = strneqvcmp(pNT->pName, pzFuncName, (int)pNT->cmpLen);
        if (cmp == 0) {
            /*
             *  Make sure we matched to the end of the token.
             */
            if (IS_VARIABLE_NAME_CHAR(pzFuncName[pNT->cmpLen]))
                break;

            /*
             *  Advance the scanner past the macro name.
             *  The name is encoded in the "fType".
             */
            *pscan = pzFuncName + pNT->cmpLen;
            return pNT->fType;
        }
        if (cmp > 0)
             hi = av - 1;
        else lo = av + 1;
    } while (hi >= lo);

    /*
     *  Save the name for later lookup
     */
    cur_macro->md_name_off =
        (size_t)(current_tpl->td_scan - current_tpl->td_text);
    {
        char * pzCopy = current_tpl->td_scan;
        char * pe = SPN_VALUE_NAME_CHARS(pzFuncName);
        size_t l  = (size_t)(pe - pzFuncName);
        memcpy(pzCopy, pzFuncName, l);
        pzCopy     += l;
        pzFuncName += l;

        /*
         *  Names are allowed to contain colons, but not end with them.
         */
        if (pzCopy[-1] == ':')
            pzCopy--, pzFuncName--;

        *(pzCopy++) = NUL;
        *pscan = pzFuncName;
        current_tpl->td_scan = pzCopy;
    }

    /*
     *  "Unknown" means we have to check again before we
     *  know whether to assign it to "FTYP_INVOKE" or "FTYP_COND".
     *  That depends on whether or not we find a named template
     *  at template instantiation time.
     */
    return FTYP_UNKNOWN;
}

static char const *
find_mac_end(char const ** ppzMark)
{
    char const * pzMark = *ppzMark + st_mac_len;
    char const * pzFunc;
    char const * pzNextMark;
    char const * pzEndMark;

    /*
     *  Set our pointers to the start of the macro text
     */
    for (;;) {
        pzMark = SPN_NON_NL_WHITE_CHARS(pzMark);
        if (*pzMark != NL)
            break;
        tpl_line++;
        pzMark++;
    }

    pzFunc             = pzMark;
    cur_macro->md_code = func_code(&pzMark);
    cur_macro->md_line = tpl_line;
    *ppzMark           = pzMark;

    /*
     *  Find the end.  (We must.)  If the thing is empty, treat as a comment,
     *  but warn about it.
     */
    pzEndMark = strstr(pzMark, end_mac_mark);
    if (pzEndMark == NULL)
        AG_ABEND(FIND_MAC_END_NOPE);

    if (pzEndMark == pzFunc) {
        cur_macro->md_code = FTYP_COMMENT;
        fprintf(trace_fp, FIND_MAC_END_EMPTY,
                current_tpl->td_file, tpl_line);
        return pzEndMark;
    }

    /*
     *  Back up over a preceding backslash.  It is a flag to indicate the
     *  removal of the end of line white space.
     */
    if (pzEndMark[-1] == '\\')
        pzEndMark--;

    pzNextMark = strstr(pzMark, st_mac_mark);
    if (pzNextMark == NULL)
        return pzEndMark;

    if (pzEndMark > pzNextMark)
        AG_ABEND(FIND_MAC_END_NESTED);

    return pzEndMark;
}

static char const *
find_mac_start(char const * pz, macro_t ** ppm, templ_t * tpl)
{
    char *       pzCopy;
    char const * pzEnd;
    char const * res = strstr(pz, st_mac_mark);
    macro_t *    mac = *ppm;

    if (res == pz)
        return res;

    /*
     *  There is some text here.  Make a text macro entry.
     */
    pzCopy      = tpl->td_scan;
    pzEnd       = (res != NULL) ? res : pz + strlen(pz);
    mac->md_txt_off = (uintptr_t)(pzCopy - tpl->td_text);
    mac->md_code = FTYP_TEXT;
    mac->md_line = tpl_line;

#if defined(DEBUG_ENABLED)
    if (HAVE_OPT(SHOW_DEFS)) {
        int ct = tpl_nest_lvl;
        fprintf(trace_fp, "%3u ", (unsigned int)(mac - tpl->td_macros));
        do { fputs("  ", trace_fp); } while (--ct > 0);

        fprintf(trace_fp, tpl_def_fmt, ag_fun_names[ FTYP_TEXT ], FTYP_TEXT,
                mac->md_line, mac->md_end_idx, (unsigned int)(pzEnd - pz));
    }
#endif

    do  {
        if ((*(pzCopy++) = *(pz++)) == NL)
            tpl_line++;
    } while (pz < pzEnd);

    *(pzCopy++)   = NUL;
    *ppm          = mac + 1;
    tpl->td_scan = pzCopy;

    return res;  /* may be NULL, if there are no more macros */
}

static char const *
find_macro(templ_t * tpl, macro_t ** ppm, char const ** pscan)
{
    char const * scan = *pscan;
    char const * pzMark;

    pzMark = find_mac_start(scan, ppm, tpl);

    /*
     *  IF no more macro marks are found, THEN we are done...
     */
    if (pzMark == NULL)
        return pzMark;

    /*
     *  Find the macro code and the end of the macro invocation
     */
    cur_macro = *ppm;
    scan    = find_mac_end(&pzMark);

    /*
     *  Count the lines in the macro text and advance the
     *  text pointer to after the marker.
     */
    {
        char const *  pzMacEnd = scan;
        char const *  pz       = pzMark;

        for (;;pz++) {
            pz = strchr(pz, NL);
            if ((pz == NULL) || (pz > pzMacEnd))
                break;
            tpl_line++;
        }

        /*
         *  Strip white space from the macro
         */
        pzMark = SPN_WHITESPACE_CHARS(pzMark);

        if (pzMark != pzMacEnd) {
            pzMacEnd = SPN_WHITESPACE_BACK( pzMark, pzMacEnd);
            (*ppm)->md_txt_off = (uintptr_t)pzMark;
            (*ppm)->md_res     = (uintptr_t)(pzMacEnd - pzMark);
        }
    }

    /*
     *  IF the end macro mark was preceded by a backslash, then we remove
     *  trailing white space from there to the end of the line.
     */
    if ((*scan != '\\') || (strncmp(end_mac_mark, scan, end_mac_len) == 0))
        scan += end_mac_len;

    else {
        char const * pz;
        scan += end_mac_len + 1;
        pz = SPN_NON_NL_WHITE_CHARS(scan);
        if (*pz == NL) {
            scan = pz + 1;
            tpl_line++;
        }
    }

    *pscan = scan;
    return pzMark;
}

#if defined(DEBUG_ENABLED)
 static void
print_indentation(templ_t * tpl, macro_t * mac, int idx)
{
    static char const fmt_fmt[] = "%%%us";
    char fmt[16];

    if (idx < 0)
        fputs("    ", trace_fp);
    else fprintf(trace_fp, "%3u ", (unsigned int)idx);
    snprintf(fmt, sizeof(fmt), fmt_fmt, tpl_nest_lvl);
    fprintf(trace_fp, fmt, "");
    (void)tpl;
    (void)mac;
}

 static void
print_ag_defs(templ_t * tpl, macro_t * mac)
{
    mac_func_t ft  = mac->md_code;
    int        ln  = mac->md_line;
    int idx = (mac->md_code == FTYP_BOGUS) ? -1 : (int)(mac - tpl->td_macros);

    print_indentation(tpl, mac, idx);

    if (mac->md_code == FTYP_BOGUS)
        fprintf(trace_fp, zTUndef, ag_fun_names[ ft ], ft, ln);
    else {
        char const * pz;
        if (ft >= FUNC_CT)
            ft = FTYP_SELECT;
        pz = (mac->md_txt_off == 0)
            ? zNil
            : (tpl->td_text + mac->md_txt_off);
        fprintf(trace_fp, tpl_def_fmt, ag_fun_names[ft], mac->md_code,
                ln, mac->md_end_idx, (unsigned int)strlen(pz));
    }
}
#endif

/**
 * Parse the template.
 * @param[out]    mac     array of macro descriptors to fill in
 * @param[in,out] p_scan  pointer to string scanning address
 */
LOCAL macro_t *
parse_tpl(macro_t * mac, char const ** p_scan)
{
    char const * scan = *p_scan;
    templ_t *    tpl  = current_tpl;

#if defined(DEBUG_ENABLED)

    #define DEBUG_DEC(l)  l--

    if (  ((tpl_nest_lvl++) > 0)
       && HAVE_OPT(SHOW_DEFS)) {
        int     idx = (int)(mac - tpl->td_macros);
        macro_t * m = mac - 1;

        print_indentation(tpl, m, idx);

        fprintf(trace_fp, zTUndef, ag_fun_names[m->md_code],
                m->md_code, m->md_line);
    }
#else
    #define DEBUG_DEC(l)
#endif

    while (find_macro(tpl, &mac, &scan) != NULL) {
        /*
         *  IF the called function returns a NULL next macro pointer,
         *  THEN some block has completed.  The returned scanning pointer
         *       will be non-NULL.
         */
        load_proc_p_t const fn = load_proc_table[mac->md_code];
        macro_t *   nxt_mac = fn(tpl, mac, &scan);

#if defined(DEBUG_ENABLED)
        if (HAVE_OPT(SHOW_DEFS))
            print_ag_defs(tpl, mac);
#endif

        if (nxt_mac == NULL) {
            *p_scan = scan;
            DEBUG_DEC(tpl_nest_lvl);
            return mac;
        }
        mac = nxt_mac;
    }

    DEBUG_DEC(tpl_nest_lvl);

    /*
     *  We reached the end of the input string.
     *  Return a NULL scanning pointer and a pointer to the end.
     */
    *p_scan = NULL;
    return mac;
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/tpParse.c */
