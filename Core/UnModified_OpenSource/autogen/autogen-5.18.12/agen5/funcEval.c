
/**
 * @file funcEval.c
 *
 *  This module evaluates macro expressions.
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
static inline char const *
tpl_text(templ_t * tpl, macro_t * mac);

static void
tpl_warning(templ_t * tpl, macro_t * mac, char const * msg);

static char const *
is_mac_entry_ok(bool * allocated, def_ent_t * ent,
                int * code, templ_t * tpl, macro_t * mac);

static void
emit_insertion_file(char const * fname, FILE * outfp);

static int
expr_type(char * pz);
/* = = = END-STATIC-FORWARD = = = */

/**
 * Convert SCM to displayable string.
 * @param s the input scm
 * @returns a string a human can read
 */
LOCAL char const *
scm2display(SCM s)
{
    static char  z[48];
    char const * res = z;

    switch (ag_scm_type_e(s)) {
    case GH_TYPE_BOOLEAN:
        z[0] = scm_is_true(s) ? '1' : '0'; z[1] = NUL;
        break;

    case GH_TYPE_STRING:
    case GH_TYPE_SYMBOL:
        res = ag_scm2zchars(s, "SCM Result");
        break;

    case GH_TYPE_CHAR:
        z[0] = (char)SCM_CHAR(s); z[1] = NUL; break;

    case GH_TYPE_VECTOR:
        res = RESOLVE_SCM_VECTOR; break;

    case GH_TYPE_PAIR:
        res = RESOLVE_SCM_PAIR; break;

    case GH_TYPE_NUMBER:
        snprintf(z, sizeof(z), ULONG_FMT, AG_SCM_TO_ULONG(s)); break;

    case GH_TYPE_PROCEDURE:
#ifdef SCM_SUBR_ENTRY
    {
        void * x = &SCM_SUBR_ENTRY(s);

        snprintf(z, sizeof(z), RESOLVE_SCM_PROC,
                 (unsigned long)x);
        break;
    }
#else
        res = "** PROCEDURE **";
        break;
#endif

    case GH_TYPE_LIST:
        res = RESOLVE_SCM_LIST; break;

    case GH_TYPE_INEXACT:
        res = RESOLVE_SCM_INEXACT; break;

    case GH_TYPE_EXACT:
        res = RESOLVE_SCM_EXACT; break;

    case GH_TYPE_UNDEFINED:
        res = (char *)zNil; break;

    default:
        res = RESOLVE_SCM_UNKNOWN; break;
    }

    return res;
}

/**
 * Return the text associated with a macro.
 */
static inline char const *
tpl_text(templ_t * tpl, macro_t * mac)
{
    if (mac->md_txt_off == 0)
        return zNil;

    return tpl->td_text + mac->md_txt_off;
}

static void
tpl_warning(templ_t * tpl, macro_t * mac, char const * msg)
{
    fprintf(trace_fp, TPL_WARN_FMT, tpl->td_file, mac->md_line, msg);
}

/**
 * eval_mac_expr() helper function.  Determine if the entry found is
 * a text entry.  If not, we return an empty string.
 *
 * @param allocated  whether the returned string has been allocated
 * @param ent        The macro entry we found.  Now testing validity.
 * @param code       flag word to indicate we found a valid string
 * @param tpl        the current template being parsed
 * @param mac        the current macro being invoked
 *
 * @returns a pointer to the macro substitution text
 */
static char const *
is_mac_entry_ok(bool * allocated, def_ent_t * ent,
                int * code, templ_t * tpl, macro_t * mac)
{
    char const * res = (char *)zNil;

    if ((*code & EMIT_IF_ABSENT) != 0)
        return res;

    if (  (ent->de_type != VALTYP_TEXT)
       && ((*code & EMIT_PRIMARY_TYPE) == EMIT_VALUE)  ) {
        tpl_warning(tpl, mac, EVAL_EXPR_BLOCK_IN_EVAL);
        return res;
    }

    /*
     *  Compute the expression string.  There are three possibilities:
     *  1.  There is an expression string in the macro, but it must
     *      be formatted with the text value.
     *      Make sure we have a value.
     *  2.  There is an expression string in the macro, but it is *NOT*
     *      to be formatted.  Use it as is.  Do *NOT* verify that
     *      the define value is text.
     *  3.  There is no expression with the macro invocation.
     *      The define value *must* be text.
     */
    if ((*code & EMIT_FORMATTED) != 0) {
        /*
         *  And make sure what we found is a text value
         */
        if (ent->de_type != VALTYP_TEXT) {
            tpl_warning(tpl, mac, EVAL_EXPR_BLOCK_IN_EVAL);
            return res;
        }

        *allocated = true;
        res = aprf(tpl_text(tpl, mac), ent->de_val.dvu_text);
    }

    else if (mac->md_txt_off != 0)
        res = tpl->td_text + mac->md_txt_off;

    else {
        /*
         *  And make sure what we found is a text value
         */
        if (ent->de_type != VALTYP_TEXT) {
            tpl_warning(tpl, mac, EVAL_EXPR_BLOCK_IN_EVAL);
            return res;
        }

        res = ent->de_val.dvu_text;
    }

    *code &= EMIT_PRIMARY_TYPE;
    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Evaluate an expression and return a string pointer.  Always.
 *  It may need to be deallocated, so a boolean pointer is used
 *  to tell the caller.  Also, the match existence and non-existence
 *  pay attention to the address of the empty string that gets
 *  returned.  If it is, specifically, "zNil", then this code is
 *  saying, "we could not compute a value, so we're returning this
 *  empty string".  This is used by the Select_Match_Existence and
 *  Select_Match_NonExistence code to distinguish non-existence from
 *  an empty string value.
 *
 * @param allocated tell caller if result has been allocated
 * @returns a string representing the result.
 */
LOCAL char const *
eval_mac_expr(bool * allocated)
{
    templ_t *     tpl   = current_tpl;
    macro_t *     mac   = cur_macro;
    int           code  = (int)mac->md_res;
    char const *  text  = NULL; /* warning patrol */
    def_ent_t *   ent;

    *allocated = false;

    if ((code & EMIT_NO_DEFINE) != 0) {
        text  = tpl_text(tpl, mac);
        code &= EMIT_PRIMARY_TYPE;
        ent   = NULL; /* warning patrol */
    }

    else {
        /*
         *  Get the named definition entry, maybe
         */
        bool indexed;
        ent = find_def_ent(tpl->td_text + mac->md_name_off, &indexed);

        if (ent == NULL) {
            switch (code & (EMIT_IF_ABSENT | EMIT_ALWAYS)) {
            case EMIT_IF_ABSENT:
                /*
                 *  There is only one expression.  It applies because
                 *  we did not find a definition.
                 */
                text = tpl_text(tpl, mac);
                code &= EMIT_PRIMARY_TYPE;
                break;

            case EMIT_ALWAYS:
                /*
                 *  There are two expressions.  Take the second one.
                 */
                text = tpl->td_text + mac->md_end_idx;
                code = ((code & EMIT_SECONDARY_TYPE)
                          >> EMIT_SECONDARY_SHIFT);
                break;

            case 0:
                /*
                 *  Emit only if found
                 */
                return no_def_str;

            case (EMIT_IF_ABSENT | EMIT_ALWAYS):
                /*
                 *  Emit inconsistently :-}
                 */
                AG_ABEND_IN(tpl, mac, EVAL_EXPR_PROG_ERR);
                /* NOTREACHED */
            }
        }

        /*
         *  OTHERWISE, we found an entry.  Make sure we were supposed to.
         */
        else {
            text = is_mac_entry_ok(allocated, ent, &code, tpl, mac);
            if (text == zNil)
                return text;
        }
    }

    /*
     *  The "code" tells us how to handle the expression
     */
    switch (code) {
    case EMIT_VALUE:
        assert(ent != NULL);
        if (*allocated) {
            AGFREE(text);
            *allocated = false;
        }

        text = ent->de_val.dvu_text;
        break;

    case EMIT_EXPRESSION:
    {
        SCM res = ag_eval(text);

        if (*allocated) {
            AGFREE(text);
            *allocated = false;
        }

        text = scm2display(res);
        break;
    }

    case EMIT_SHELL:
    {
        char * pz = shell_cmd(text);

        if (*allocated)
            AGFREE(text);

        if (pz != NULL) {
            *allocated = true;
            text = pz;
        }
        else {
            *allocated = false;
            text = (char *)zNil;
        }
        break;
    }

    case EMIT_STRING:
        break;
    }

    return text;
}

/*=gfunc error_source_line
 *
 * what: display of file & line
 * general_use:
 * doc:  This function is only invoked just before Guile displays
 *       an error message.  It displays the file name and line number
 *       that triggered the evaluation error.  You should not need to
 *       invoke this routine directly.  Guile will do it automatically.
=*/
SCM
ag_scm_error_source_line(void)
{
    fprintf(stderr, SCM_ERROR_FMT, current_tpl->td_name, cur_macro->md_line,
            current_tpl->td_text + cur_macro->md_txt_off);
    fflush(stderr);

    return SCM_UNDEFINED;
}

/*=gfunc emit
 *
 * what: emit the text for each argument
 *
 * exparg: alist, list of arguments to stringify and emit, , list
 *
 * doc:  Walk the tree of arguments, displaying the values of displayable
 *       SCM types.  EXCEPTION: if the first argument is a number, then
 *       that number is used to index the output stack.  "0" is the default,
 *       the current output.
=*/
SCM
ag_scm_emit(SCM val)
{
    static int depth = 0;
    static FILE * fp;

    switch (depth) {
    case 1:
    {
        out_stack_t * pSaveFp;
        unsigned long pnum;

        if (! scm_is_number(val))
            break;

        pSaveFp = cur_fpstack;
        pnum    = AG_SCM_TO_ULONG(val);

        for (; pnum > 0; pnum--) {
            pSaveFp = pSaveFp->stk_prev;
            if (pSaveFp == NULL)
                AG_ABEND(aprf(EMIT_INVAL_PORT, AG_SCM_TO_ULONG(val)));
        }

        fp = pSaveFp->stk_fp;
        return SCM_UNDEFINED;
    }

    case 0:
        fp = cur_fpstack->stk_fp; // initialize the first time through
        break;
    }

    depth++;
    for (;;) {
        if (val == SCM_UNDEFINED)
            break;

        if (scm_is_null(val))
            break;

        if (scm_is_string(val)) {
            fputs((char *)ag_scm2zchars(val, "emit val"), fp);
            fflush(fp);
            break;
        }

        switch (ag_scm_type_e(val)) {
        case GH_TYPE_LIST:
        case GH_TYPE_PAIR:
            ag_scm_emit(SCM_CAR(val));
            val = SCM_CDR(val);
            continue;

        default:
            fputs(scm2display(val), fp);
            fflush(fp);
            break;
        }

        break;
    }

    depth--;
    return SCM_UNDEFINED;
}

/**
 * read in a file and write it to the passed in file pointer.
 *
 * @param [in]  fname  the name of the file to read
 * @param [in]  outfp  the file pointer to write the data to.
 */
static void
emit_insertion_file(char const * fname, FILE * outfp)
{
    char * txt = load_file(fname);
    if (txt == NULL) {
        if (errno == 0)
            errno = ENOENT;
        fswarn("loading", fname);
        return;
    }
    if (fputs(txt, outfp) < 0)
        fserr(AUTOGEN_EXIT_FS_ERROR, "fputs", cur_fpstack->stk_fname);

    if (fflush(outfp) < 0)
        fserr(AUTOGEN_EXIT_FS_ERROR, "fflush", cur_fpstack->stk_fname);
    AGFREE(txt);
}

/*=gfunc insert_file
 *
 * what: insert the contents of a (list of) files.
 *
 * exparg: alist, list of files to emit, , list
 *
 * doc:  Insert the contents of one or more files.
=*/
SCM
ag_scm_insert_file(SCM val)
{
    static int depth = 0;
    static FILE * fp;

    switch (depth) {
    case 1:
    {
        out_stack_t * pSaveFp;
        unsigned long pnum;

        if (! scm_is_number(val))
            break;

        pSaveFp = cur_fpstack;
        pnum    = AG_SCM_TO_ULONG(val);

        for (; pnum > 0; pnum--) {
            pSaveFp = pSaveFp->stk_prev;
            if (pSaveFp == NULL)
                AG_ABEND(aprf(EMIT_INVAL_PORT, AG_SCM_TO_ULONG(val)));
        }

        fp = pSaveFp->stk_fp;
        return SCM_UNDEFINED;
    }

    case 0:
        fp = cur_fpstack->stk_fp; // initialize the first time through
        break;
    }

    depth++;
    for (;;) {
        if (val == SCM_UNDEFINED)
            break;

        if (scm_is_null(val))
            break;

        if (scm_is_string(val)) {
            emit_insertion_file(ag_scm2zchars(val, "emit val"), fp);
            break;
        }

        switch (ag_scm_type_e(val)) {
        case GH_TYPE_LIST:
        case GH_TYPE_PAIR:
            ag_scm_insert_file(SCM_CAR(val));
            val = SCM_CDR(val);
            continue;

        default: break;
        }

        break;
    }

    depth--;
    return SCM_UNDEFINED;
}

/**
 *  The global evaluation function.
 *
 *  The string to "evaluate" may be a literal string, or may need Scheme
 *  interpretation.  So, we do one of three things: if the string starts with
 *  a Scheme comment character or evaluation character (';' or '('), then run
 *  a Scheme eval.  If it starts with a quote character ('\'' or '"'), then
 *  digest the string and return that.  Otherwise, just return the string.
 *
 * @param expr input expression string
 * @returns an SCM value representing the result
 */
LOCAL SCM
eval(char const * expr)
{
    bool   allocated = false;
    char * pzTemp;
    SCM    res;

    switch (*expr) {
    case '(':
    case ';':
        res = ag_eval((char *)expr);
        break;

    case '`':
        AGDUPSTR(pzTemp, expr, "shell script");
        (void)span_quote(pzTemp);
        expr = shell_cmd(pzTemp);
        AGFREE(pzTemp);
        res = scm_from_latin1_string((char *)expr);
        AGFREE(expr);
        break;

    case '"':
    case '\'':
        AGDUPSTR(pzTemp, expr, "quoted string");
        (void)span_quote(pzTemp);
        allocated = true;
        expr = pzTemp;
        /* FALLTHROUGH */

    default:
        res = scm_from_latin1_string((char *)expr);
        if (allocated)
            AGFREE(expr);
    }

    return res;
}


/*=macfunc EXPR
 *
 *  what:  Evaluate and emit an Expression
 *  alias: + - + ? + % + ; + ( + '`' + '"' + "'" + . + { +
 *
 *  handler_proc:
 *  load_proc:
 *
 *  desc:
 *   This macro does not have a name to cause it to be invoked
 *   explicitly, though if a macro starts with one of the apply codes
 *   or one of the simple expression markers, then an expression
 *   macro is inferred.  The result of the expression evaluation
 *   (@pxref{expression syntax}) is written to the current output.
=*/
macro_t *
mFunc_Expr(templ_t * tpl, macro_t * mac)
{
    bool allocated_str;
    char const * pz = eval_mac_expr(&allocated_str);

    (void)tpl;

    if (*pz != NUL) {
        fputs(pz, cur_fpstack->stk_fp);
        fflush(cur_fpstack->stk_fp);
    }

    if (allocated_str)
        AGFREE(pz);

    return mac + 1;
}

/**
 * Determine the expression type.  It may be Scheme (starts with a semi-colon
 * or an opening parenthesis), a shell command (starts with a back tick),
 * a quoted string (either single or double), or it is some sort of plain
 * string.  In that case, just return the text.
 *
 * @param pz pointer to string to diagnose
 * @returns the EMIT_* compound value, though actually only
 * EXPRESSION, SHELL or STRING can really ever be returned.
 */
static int
expr_type(char * pz)
{
    switch (*pz) {
    case ';':
    case '(':
        return EMIT_EXPRESSION;

    case '`':
        span_quote(pz);
        return EMIT_SHELL;

    case '"':
    case '\'':
        span_quote(pz);
        /* FALLTHROUGH */

    default:
        return EMIT_STRING;
    }
}


/**
 *  mLoad_Expr
 */
macro_t *
mLoad_Expr(templ_t * tpl, macro_t * mac, char const ** ppzScan)
{
    char *        copy; /* next text dest   */
    char const *  src     = (char const *)mac->md_txt_off; /* macro text */
    size_t        src_len = (size_t)mac->md_res;          /* macro len  */
    char const *  end_src = src + src_len;

    if (src_len == 0) {
        if (mac->md_code == FTYP_INCLUDE)
            AG_ABEND_IN(tpl, mac, LD_INC_NO_FNAME);
        mac->md_res = EMIT_VALUE;
        mac->md_txt_off = 0;
        return mac + 1;
    }

    switch (*src) {
    case '-':
        mac->md_res = EMIT_IF_ABSENT;
        src++;
        break;

    case '?':
        mac->md_res = EMIT_ALWAYS;
        src++;
        if (*src == '%') {
            mac->md_res |= EMIT_FORMATTED;
            src++;
        }
        break;

    case '%':
        mac->md_res = EMIT_FORMATTED;
        src++;
        break;

    case '`':
        (void) mLoad_Unknown(tpl, mac, ppzScan);
        mac->md_res = EMIT_NO_DEFINE | EMIT_SHELL;
        span_quote(tpl->td_text + mac->md_txt_off);
        return mac + 1;

    case '{':
        (void) mLoad_Unknown(tpl, mac, ppzScan);
        mac->md_res = EMIT_NO_DEFINE | EMIT_SHELL;
        return mac + 1;

    case '"':
    case '\'':
        (void) mLoad_Unknown(tpl, mac, ppzScan);
        mac->md_res = EMIT_NO_DEFINE | EMIT_STRING;
        span_quote(tpl->td_text + mac->md_txt_off);
        return mac + 1;

    case '(':
    case ';':
        (void) mLoad_Unknown(tpl, mac, ppzScan);
        mac->md_res = EMIT_NO_DEFINE | EMIT_EXPRESSION;
        return mac + 1;

    default:
        mac->md_res = EMIT_VALUE; /* zero */
        break;
    }

    copy = tpl->td_scan;
    mac->md_name_off = (uintptr_t)(copy - tpl->td_text);
    {
        size_t remLen = (size_t)canonical_name(copy, src, (int)src_len);
        if (remLen > src_len)
            AG_ABEND_IN(tpl, mac, LD_EXPR_BAD_NAME);
        src    += src_len - remLen;
        src_len = remLen;
        copy   += strlen(copy) + 1;
    }

    if (src >= end_src) {
        if (mac->md_res != EMIT_VALUE)
            AG_ABEND_IN(tpl, mac, LD_EXPR_NO_TEXT);

        mac->md_txt_off = 0;

    } else {
        char * pz = copy;
        src_len = (size_t)(end_src - src);

        mac->md_txt_off = (uintptr_t)(copy - tpl->td_text);
        /*
         *  Copy the expression
         */
        memcpy(copy, src, src_len);
        copy += src_len;
        *(copy++) = NUL; *(copy++) = NUL; /* double terminate */

        /*
         *  IF this expression has an "if-present" and "if-not-present"
         *  THEN find the ending expression...
         */
        if ((mac->md_res & EMIT_ALWAYS) != 0) {
            char * pzNextExpr = (char *)skip_expr(pz, src_len);

            /*
             *  The next expression must be within bounds and space separated
             */
            if (pzNextExpr >= pz + src_len)
                AG_ABEND_IN(tpl, mac, LD_EXPR_NEED_TWO);

            if (! IS_WHITESPACE_CHAR(*pzNextExpr))
                AG_ABEND_IN(tpl, mac, LD_EXPR_NO_SPACE);

            /*
             *  NUL terminate the first expression, skip intervening
             *  white space and put the secondary expression's type
             *  into the macro type code as the "secondary type".
             */
            *(pzNextExpr++) = NUL;
            pzNextExpr = SPN_WHITESPACE_CHARS(pzNextExpr);
            mac->md_res |= (uintptr_t)(
                (expr_type(pzNextExpr) << EMIT_SECONDARY_SHIFT));
            mac->md_end_idx = (int)(pzNextExpr - tpl->td_text);
        }

        mac->md_res |= (uintptr_t)expr_type(pz);
    }

    tpl->td_scan = copy;
    return mac + 1;
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/funcEval.c */
