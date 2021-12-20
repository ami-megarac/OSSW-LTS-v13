
/**
 * @file functions.c
 *
 *  This module implements text functions.
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

/*=macfunc CONTINUE
 *
 *  handler-proc:   Break
 *  load-proc:      Leave
 *  what:           Skip to end of a FOR or WHILE macro.
 *
 *  desc:
 *  This will skip the remainder of the loop and start the next.
=*/

/*=macfunc BREAK
 *
 *  handler-proc:   Break
 *  load-proc:      Leave
 *  what:           Leave a FOR or WHILE macro
 *
 *  desc:
 *  This will unwind the loop context and resume after ENDFOR/ENDWHILE.
 *  Note that unless this happens to be the last iteration anyway,
 *  the (last-for?) function will never yield "#t".
=*/
macro_t *
mFunc_Break(templ_t * tpl, macro_t * mac)
{
    for_state_t * fst = curr_ivk_info->ii_for_data;
    int code = (mac->md_code == FTYP_BREAK) ? LOOP_JMP_BREAK : LOOP_JMP_NEXT;
    if (fst == NULL) {
        char const * which =
            (mac->md_code == FTYP_BREAK) ? BREAK_STR : CONTINUE_STR;
        AG_ABEND(aprf(BAD_BREAK_FMT, which));
    }
    fst += curr_ivk_info->ii_for_depth - 1;

    (void)tpl;
    (void)mac;
    longjmp(fst->for_env, code);
}

/**
 * wrapper function for calling gen_block in a loop.
 * It sets up and handles the jump buffer, returning the jump result.
 *
 * @param[in,out] jbuf     the jump buffer
 * @param[in]     tpl      the new active template
 * @param[in]     mac      the looping macro
 * @param[in]     end_mac  pointer to the first macro after the block
 *
 * @returns either LOOP_JMP_OKAY (0) or LOOP_JMP_BREAK (the caller should
 * exit the loop).
 */
LOCAL loop_jmp_type_t
call_gen_block(jmp_buf jbuf, templ_t * tpl, macro_t * mac, macro_t * end_mac)
{
    switch (setjmp(jbuf)) {
    case LOOP_JMP_OKAY: // 0
        gen_block(tpl, mac, end_mac);
        /* FALLTHROUGH */

    case LOOP_JMP_NEXT:
        return LOOP_JMP_OKAY;

    case LOOP_JMP_BREAK:
    default:
        return LOOP_JMP_BREAK;
    }
}

/*=macfunc RETURN
 *
 *  handler-proc:
 *  load-proc:      Leave
 *
 *  what:           Leave an INVOKE-d (DEFINE) macro
 *
 *  desc:
 *  This will unwind looping constructs inside of a DEFINE-d macro and
 *  return to the invocation point.  The output files and diversions
 *  @i{are left alone}.  This means it is unwise to start diversions
 *  in a DEFINEd macro and RETURN from it before you have handled the
 *  diversion.  Unless you are careful.  Here is some rope for you.
 *  Please be careful using it.
=*/
macro_t *
mFunc_Return(templ_t * tpl, macro_t * mac)
{
    (void)tpl;
    (void)mac;
    free_for_context(INT_MAX);
    if (curr_ivk_info->ii_prev == NULL)
        AG_ABEND_IN(tpl, mac, RETURN_FROM_NOWHERE);
    longjmp(curr_ivk_info->ii_env, 1);
}

/**
 * Generate a block with a new template context.  It may be either
 * an @code{INCLUDE}-d template or a user @code{DEFINE}-d macro.
 * If @code{gen_block} returns with a long jump, the long jump value
 * is ignored.  It was terminated early with a @code{RETURN}.
 *
 * @param[in] tpl new template block (included or invoked).
 */
LOCAL void
gen_new_block(templ_t * tpl)
{
    templ_t *   oldt = current_tpl;
    ivk_info_t    ii = IVK_INFO_INITIALIZER(curr_ivk_info);

    curr_ivk_info = &ii;

    if (setjmp(ii.ii_env) == 0) {
        macro_t * m = tpl->td_macros;
        gen_block(tpl, m, m + tpl->td_mac_ct);
    }

    current_tpl   = oldt;
    curr_ivk_info = ii.ii_prev;
}

/**
 * Validate the context for leaving early.  @code{FOR} and @code{WHILE} loops
 * may leave an interation early with @code{CONTINUE} or @code{BREAK}.
 * @code{DEFINE}-d macros and @code{INCLUDE}-d files may leave early with
 * @code{RETURN}.  Loops may not be left early from an @code{INVOKE}-d macro
 * or an @code{INCLUDE}-d template.
 *
 * This load function handles @code{BREAK}, @code{CONTINUE} and @code{RETURN}.
 * It is always defined, so it must check for itself whether the
 * context is correct or not.
 *
 *  @param tpl     template being loaded
 *  @param mac     the macro descriptor
 *  @param p_scan  the input text scanning pointer
 *
 *  @returns the macro table entry after mac
 */
macro_t *
mLoad_Leave(templ_t * tpl, macro_t * mac, char const ** p_scan)
{
    (void) tpl;

    if (mac->md_code == FTYP_RETURN) {
        /*
         * Check returns at load time.  "break" and "continue" at
         * instantiation time.
         */
        if (! defining_macro && (include_depth == 0))
            (void)mLoad_Bogus(tpl, mac, p_scan);
    }
    return mac + 1;
}

/*=macfunc INCLUDE
 *
 *  what:   Read in and emit a template block
 *  handler_proc:
 *  load_proc:    Expr
 *
 *  desc:
 *
 *  The entire contents of the named file is inserted at this point.
 *  The contents of the file are processed for macro expansion.  The
 *  arguments are eval-ed, so you may compute the name of the file to
 *  be included.  The included file must not contain any incomplete
 *  function blocks.  Function blocks are template text beginning with
 *  any of the macro functions @samp{CASE}, @samp{DEFINE}, @samp{FOR},
 *  @samp{IF} and @samp{WHILE}; extending through their respective
 *  terminating macro functions.
=*/
macro_t *
mFunc_Include(templ_t * tpl, macro_t * mac)
{
    bool         allocated_name;
    char const * fname = eval_mac_expr(&allocated_name);

    include_depth++;
    if (*fname != NUL) {
        templ_t * new_tpl  = tpl_load(fname, tpl->td_file);
        macro_t * last_mac = new_tpl->td_macros + (new_tpl->td_mac_ct - 1);

        if (last_mac->md_code == FTYP_TEXT) {
            /*
             *  Strip off trailing white space from included templates
             */
            char * pz = new_tpl->td_text + last_mac->md_txt_off;
            char * pe = SPN_WHITESPACE_BACK(pz, pz);

            /*
             *  IF there is no text left, remove the macro entirely
             */
            if (pe > pz) {
                *pe = NUL;
            } else {
                new_tpl->td_mac_ct--;
            }
        }

        if (OPT_VALUE_TRACE > TRACE_DEBUG_MESSAGE) {
            fprintf(trace_fp, TRACE_FN_INC_TPL, new_tpl->td_file);
            if (OPT_VALUE_TRACE == TRACE_EVERYTHING)
                fprintf(trace_fp, TRACE_FN_INC_LINE, current_tpl->td_file,
                        mac->md_line);
        }

        gen_new_block(new_tpl);
        tpl_unload(new_tpl);
        current_tpl = tpl;
    }
    include_depth--;

    if (allocated_name)
        AGFREE(fname);

    return mac + 1;
}

/*=macfunc UNKNOWN
 *
 *  what:  Either a user macro or a value name.
 *  handler_proc:
 *  load_proc:
 *  unnamed:
 *
 *  desc:
 *
 *  The macro text has started with a name not known to AutoGen.  If, at run
 *  time, it turns out to be the name of a defined macro, then that macro is
 *  invoked.  If it is not, then it is a conditional expression that is
 *  evaluated only if the name is defined at the time the macro is invoked.
 *
 *  You may not specify @code{UNKNOWN} explicitly.
=*/
macro_t *
mFunc_Unknown(templ_t * pT, macro_t * pMac)
{
    templ_t * pInv = find_tpl(pT->td_text + pMac->md_name_off);
    if (pInv != NULL) {
        if (OPT_VALUE_TRACE >= TRACE_EVERYTHING)
            fprintf(trace_fp, TRACE_FN_REMAPPED, TRACE_FN_REMAP_INVOKE,
                    pMac->md_code, pT->td_file, pMac->md_line);
        pMac->md_code    = FTYP_DEFINE;
        pMac->md_pvt = VOIDP(pInv);
        parse_mac_args(pT, pMac);
        return mFunc_Define(pT, pMac);
    }

    if (OPT_VALUE_TRACE >= TRACE_EVERYTHING) {
        fprintf(trace_fp, TRACE_FN_REMAPPED, TRACE_FN_REMAP_EXPR,
                pMac->md_code, pT->td_file, pMac->md_line);
        fprintf(trace_fp, TRACE_FN_REMAP_BASE,
                pT->td_text + pMac->md_name_off);
    }

    pMac->md_code = FTYP_EXPR;
    if (pMac->md_txt_off == 0) {
        pMac->md_res = EMIT_VALUE;

    } else {
        char * pzExpr = pT->td_text + pMac->md_txt_off;
        switch (*pzExpr) {
        case ';':
        case '(':
            pMac->md_res = EMIT_EXPRESSION;
            break;

        case '`':
            pMac->md_res = EMIT_SHELL;
            span_quote(pzExpr);
            break;

        case '"':
        case '\'':
            span_quote(pzExpr);
            /* FALLTHROUGH */

        default:
            pMac->md_res = EMIT_STRING;
        }

        if (OPT_VALUE_TRACE >= TRACE_EVERYTHING)
            fprintf(trace_fp, TRACE_UNKNOWN_FMT, pMac->md_res, pzExpr);
    }

    return mFunc_Expr(pT, pMac);
}


/*=macfunc BOGUS
 *
 *  what:  Out-of-context or unknown functions are bogus.
 *  handler_proc:
 *  load_proc:
 *  unnamed:
=*/
macro_t *
mFunc_Bogus(templ_t * pT, macro_t * pMac)
{
    char * pz = aprf(FN_BOGUS_FMT, pMac->md_code,
                     (pMac->md_code < FUNC_CT)
                     ? ag_fun_names[ pMac->md_code ]
                     : FN_BOGUS_HUH);

    AG_ABEND_IN(pT, pMac, pz);
    /* NOTREACHED */
    return pMac;
}


/*=macfunc TEXT
 *
 *  what:  A block of text to be emitted.
 *  handler_proc:
 *  unnamed:
=*/
macro_t *
mFunc_Text(templ_t * pT, macro_t * pMac)
{
    fputs(pT->td_text + pMac->md_txt_off, cur_fpstack->stk_fp);
    fflush(cur_fpstack->stk_fp);
    return pMac + 1;
}


/*=macfunc COMMENT
 *
 *  what:  A block of comment to be ignored
 *  load_proc:
 *  alias: "#"
 *
 *  desc:
 *    This function can be specified by the user, but there will
 *    never be a situation where it will be invoked at emit time.
 *    The macro is actually removed from the internal representation.
 *
 *    If the native macro name code is @code{#}, then the
 *    entire macro function is treated as a comment and ignored.
 *
 *    @example
 *    [+ # say what you want, but no '+' before any ']' chars +]
 *    @end example
=*/
macro_t *
mLoad_Comment(templ_t * tpl, macro_t * mac, char const ** p_scan)
{
    (void)tpl;
    (void)p_scan;
    memset(VOIDP(mac), 0, sizeof(*mac));
    return mac;
}

/**
 *  The default (unknown) load function.
 *
 *  Move any text into the text offset field.  This macro will change to
 *  either INVOKE or an expression function, depending on whether or not a
 *  DEFINE macro corresponds to the name.  This is determined at instantiation
 *  time.  This is used as the default load mechanism.
 *
 *  @param tpl     template being loaded
 *  @param mac     the macro descriptor
 *  @param p_scan  the input text scanning pointer
 *
 *  @returns the macro table entry after mac
 */
macro_t *
mLoad_Unknown(templ_t * tpl, macro_t * mac, char const ** unused)
{
    char const * scan;
    ssize_t      src_len = (ssize_t)mac->md_res; /* macro len  */
    (void)unused;

    if (src_len <= 0)
        goto return_emtpy_expr;

    scan = (char const *)mac->md_txt_off; /* macro text */

    switch (*scan) {
    case ';':
    {
        char const * start = scan;

        /*
         *  Strip off scheme comments
         */
        do  {
            scan = strchr(scan, NL);
            if (scan == NULL)
                goto return_emtpy_expr;

            scan = SPN_WHITESPACE_CHARS(scan);
            if (*scan == NUL)
                goto return_emtpy_expr;

            src_len -= scan - start;
            if (src_len <= 0)
                goto return_emtpy_expr;

            start = scan;
        } while (*scan == ';');
        break;
    }

    case '[':
    case '.':
    {
        /*
         *  We are going to recopy the definition name, this time as a
         *  canonical name (i.e. including '[', ']' and '.'  characters,
         *  but with all blanks squeezed out)
         */
        char * cname     = tpl->td_text + mac->md_name_off;
        size_t cname_len = strlen(cname);

        /*
         *  Move back the source pointer.  We may have skipped blanks,
         *  so skip over however many first, then back up over the name.
         *  We have found a name, so we won't back up past the start.
         */
        while (IS_WHITESPACE_CHAR(scan[-1])) scan--, src_len++;
        scan    -= cname_len;
        src_len += (ssize_t)cname_len;

        /*
         *  Now copy over the full canonical name.  Check for errors.
         *  Advance the scan pointer to just past the name we've copied.
         */
        {
            ssize_t rem_len = canonical_name(cname, scan, (int)src_len);
            if (rem_len > src_len)
                AG_ABEND_IN(tpl, mac, LD_UNKNOWN_INVAL_DEF);

            scan   += src_len - rem_len;
            src_len = (ssize_t)rem_len;
        }

        /*
         *  Where we are stashing text ("td_scan") gets set to just past the
         *  NUL byte terminating the name.  "cname" is now longer than before.
         */
        tpl->td_scan = cname + strlen(cname) + 1;
        if (src_len <= 0)
            goto return_emtpy_expr;
        break;
    }
    }

    /*
     *  Copy the expression (the remaining text)
     */
    {
        char * dest = tpl->td_scan; /* next text dest   */
        mac->md_txt_off = (uintptr_t)(dest - tpl->td_text);
        mac->md_res = 0;
        memcpy(dest, scan, (size_t)src_len);
        dest       += src_len;
        *(dest++)   = NUL;
        *dest       = NUL; /* double terminate */
        tpl->td_scan = dest;
    }

    return mac + 1;

 return_emtpy_expr:
    mac->md_txt_off = 0;
    mac->md_res     = 0;
    return mac + 1;
}


/**
 *  Some functions are known to AutoGen, but invalid out of context.
 *  For example, ELIF, ELSE and ENDIF are all known to AutoGen.
 *  However, the load function pointer for those functions points
 *  here, until an "IF" function is encountered.
 *
 *  @param tpl     template being loaded
 *  @param mac     the macro descriptor
 *  @param p_scan  the input text scanning pointer
 *
 *  @returns the macro table entry after mac
 */
macro_t *
mLoad_Bogus(templ_t * tpl, macro_t * mac, char const ** p_scan)
{
    char const * pzSrc = (char const *)mac->md_txt_off; /* macro text */
    char const * pzMac;

    char z[ 64 ];
    (void)p_scan;

    if (pzSrc != NULL) {
        z[0] = ':';
        z[1] = z[2] = ' ';
        strncpy(z+3, pzSrc, (size_t)60);
        z[63] = NUL;
        pzSrc = z;
    }
    else
        pzSrc = zNil;

    {
        int ix = mac->md_code;
        if ((unsigned)ix >= FUNC_CT)
            ix = 0;

        pzMac = ag_fun_names[ ix ];
    }

    pzSrc = aprf(LD_BOGUS_UNKNOWN, tpl->td_file, mac->md_line, pzMac, pzSrc);

    AG_ABEND_IN(tpl, mac, pzSrc);
    /* NOTREACHED */
    return NULL;
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/functions.c */
