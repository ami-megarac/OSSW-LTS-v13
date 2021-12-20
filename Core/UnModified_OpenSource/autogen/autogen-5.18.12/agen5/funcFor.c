
/**
 * @file funcFor.c
 *
 *  This module implements the FOR text macro.
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

#define ENTRY_END  INT_MAX
#define UNASSIGNED 0x7BAD0BAD

/* = = = START-STATIC-FORWARD = = = */
static for_state_t *
find_for_state(SCM which_scm);

static bool
next_def(bool invert, def_ent_t ** de_lst);

static inline t_word
set_loop_limit(def_ent_t * found);

static int
for_by_step(templ_t * pT, macro_t * pMac, def_ent_t * found);

static int
for_each(templ_t * tpl, macro_t * mac, def_ent_t * def_ent);

static void
load_for_in(char const * pzSrc, size_t srcLen, templ_t * pT, macro_t * pMac);

static for_state_t *
new_for_context(void);
/* = = = END-STATIC-FORWARD = = = */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  Operational Functions */

static for_state_t *
find_for_state(SCM which_scm)
{
    ivk_info_t * srch = curr_ivk_info;
    char const * which = scm_is_string(which_scm)
        ? ag_scm2zchars(which_scm, "which for") : NULL;

    for (; srch != NULL; srch = srch->ii_prev) {
        int ix;

        if (srch->ii_for_depth <= 0)
            continue;

        /*
         * If "which" is not specified, then accept first found
         */
        ix = srch->ii_for_depth - 1;
        if (which == NULL)
            return srch->ii_for_data + ix;

        for (; ix >= 0; ix--) {
            for_state_t * fs = srch->ii_for_data + ix;
            if (strcmp(fs->for_name, which) == 0)
                return fs;
        }
    }
    return NULL;
}

/*=gfunc first_for_p
 *
 * what:    detect first iteration
 * exparg:  for_var, which for loop, opt
 * doc:
 *
 *    Returns @code{SCM_BOOL_T} if the named FOR loop (or, if not named, the
 *    current innermost loop) is on the first pass through the data.  Outside
 *    of any @code{FOR} loop, it returns @code{SCM_UNDEFINED}, @pxref{FOR}.
=*/
SCM
ag_scm_first_for_p(SCM which)
{
    for_state_t * p = find_for_state(which);
    if (p == NULL)
        return SCM_UNDEFINED;

    if (! scm_is_string(which))
        return (for_state->for_isfirst) ? SCM_BOOL_T : SCM_BOOL_F;

    which = p->for_isfirst ? SCM_BOOL_T : SCM_BOOL_F;
    return which;
}

/*=gfunc last_for_p
 *
 * what:    detect last iteration
 * exparg:  for_var, which for loop, opt
 * doc:     Returns SCM_BOOL_T if the named FOR loop (or, if not named, the
 *          current innermost loop) is on the last pass through the data.
 *          Outside of any FOR loop, it returns SCM_UNDEFINED.
 *          @xref{FOR}.
=*/
SCM
ag_scm_last_for_p(SCM which)
{
    for_state_t * p = find_for_state(which);
    if (p == NULL)
        return SCM_UNDEFINED;

    which = p->for_islast ? SCM_BOOL_T : SCM_BOOL_F;
    return which;
}

/*=gfunc found_for_p
 *
 * what:    is current index in list?
 * exparg:  for_var, which for loop, opt
 * doc:
 *  Returns SCM_BOOL_T if the currently indexed value is present,
 *  otherwise SCM_BOOL_F.  Outside of any FOR loop, it returns
 *  SCM_UNDEFINED. @xref{FOR}.
=*/
SCM
ag_scm_found_for_p(SCM which)
{
    for_state_t * p = find_for_state(which);
    if (p == NULL)
        return SCM_UNDEFINED;

    which = p->for_not_found ? SCM_BOOL_F : SCM_BOOL_T;
    return which;
}

/*=gfunc for_index
 *
 * what:    get current loop index
 * exparg:  for_var, which for loop, opt
 * doc:
 *
 *    Returns the current index for the named @code{FOR} loop.
 *    If not named, then the index for the innermost loop.
 *    Outside of any FOR loop, it returns @code{SCM_UNDEFINED}, @xref{FOR}.
=*/
SCM
ag_scm_for_index(SCM which)
{
    for_state_t * p = find_for_state(which);
    if (p == NULL)
        return SCM_UNDEFINED;

    which = scm_from_int(p->for_index);
    return which;
}

/*=gfunc for_from
 *
 * what:   set initial index
 * exparg: from, the initial index for the AutoGen FOR macro
 *
 * doc:  This function records the initial index information
 *       for an AutoGen FOR function.
 *       Outside of the FOR macro itself, this function will emit an error.
 *       @xref{FOR}.
=*/
SCM
ag_scm_for_from(SCM from)
{
    if ((! for_state->for_loading) || (! scm_is_number(from)))
        return SCM_UNDEFINED;

    for_state->for_from = scm_to_int(from);
    return SCM_BOOL_T;
}


/*=gfunc for_to
 *
 * what:   set ending index
 * exparg: to, the final index for the AutoGen FOR macro
 *
 * doc:  This function records the terminating value information
 *       for an AutoGen FOR function.
 *       Outside of the FOR macro itself, this function will emit an error.
 *       @xref{FOR}.
=*/
SCM
ag_scm_for_to(SCM to)
{
    if ((! for_state->for_loading) || (! scm_is_number(to)))
        return SCM_UNDEFINED;

    for_state->for_to = scm_to_int(to);
    return SCM_BOOL_T;
}


/*=gfunc for_by
 *
 * what:   set iteration step
 * exparg: by, the iteration increment for the AutoGen FOR macro
 *
 * doc:  This function records the "step by" information
 *       for an AutoGen FOR function.
 *       Outside of the FOR macro itself, this function will emit an error.
 *       @xref{FOR}.
=*/
SCM
ag_scm_for_by(SCM by)
{
    if ((! for_state->for_loading) || (! scm_is_number(by)))
        return SCM_UNDEFINED;

    for_state->for_by = scm_to_int(by);
    return SCM_BOOL_T;
}

/*=gfunc for_sep
 *
 * what:   set loop separation string
 * exparg: separator, the text to insert between the output of
 *         each FOR iteration
 *
 * doc:  This function records the separation string that is to be inserted
 *       between each iteration of an AutoGen FOR function.  This is often
 *       nothing more than a comma.
 *       Outside of the FOR macro itself, this function will emit an error.
=*/
SCM
ag_scm_for_sep(SCM obj)
{
    if ((! for_state->for_loading) || (! scm_is_string(obj)))
        return SCM_UNDEFINED;

    AGDUPSTR(for_state->for_sep_str, ag_scm2zchars(obj, "sep"), "seps");
    return SCM_BOOL_T;
}

/**
 * search for matching definition entry.
 * Only the current level is traversed, via the "de_twin" link.
 *
 * @param[in] invert  invert the sense of the search
 *                    ("FOR" is running backwards)
 * @param[in,out] de_lst  linked list of definitions
 */
static bool
next_def(bool invert, def_ent_t ** de_lst)
{
    bool     matched = false;
    def_ent_t * ent  = *de_lst;

    while (ent != NULL) {
        /*
         *  Loop until we find or pass the current index value
         *
         *  IF we found an entry for the current index,
         *  THEN break out and use it
         */
        if (ent->de_index == for_state->for_index) {
            matched = true;
            break;
        }

        /*
         *  IF the next definition is beyond our current index,
         *       (that is, the current index is inside of a gap),
         *  THEN we have no current definition and will use
         *       only the set passed in.
         */
        if ((invert)
            ? (ent->de_index < for_state->for_index)
            : (ent->de_index > for_state->for_index)) {

            /*
             *  When the "by" step is zero, force syncronization.
             */
            if (for_state->for_by == 0) {
                for_state->for_index = (int)ent->de_index;
                matched = true;
            }
            break;
        }

        /*
         *  The current index (for_state->for_index) is past the current value
         *  (pB->de_index), so advance to the next entry and test again.
         */
        ent = (invert) ? ent->de_ptwin : ent->de_twin;
    }

    /*
     *  Save our restart point and return the find indication
     */
    *de_lst = ent;
    return matched;
}

/**
 *  IF the for-from and for-to values have not been set,
 *  THEN we set them from the indices of the first and last
 *       entries of the twin set.
 */
static inline t_word
set_loop_limit(def_ent_t * found)
{
    t_word res  = OPT_VALUE_LOOP_LIMIT;
    bool invert = (for_state->for_by < 0) ? true : false;

    def_ent_t * lde = (found->de_etwin != NULL) ? found->de_etwin : found;

    if (for_state->for_from == UNASSIGNED)
        for_state->for_from =
            invert ? (int)lde->de_index : (int)found->de_index;

    if (for_state->for_to == UNASSIGNED)
        for_state->for_to = invert ? (int)found->de_index : (int)lde->de_index;

    /*
     *  "loop limit" is intended to catch runaway ending conditions.
     *  However, if you really have a gazillion entries, who am I
     *  to stop you?
     */
    if (res <  lde->de_index - found->de_index)
        res = (lde->de_index - found->de_index) + 1;
    return res;
}

static int
for_by_step(templ_t * pT, macro_t * pMac, def_ent_t * found)
{
    int         loopCt    = 0;
    bool        invert    = (for_state->for_by < 0) ? true : false;
    macro_t *   end_mac   = pT->td_macros + pMac->md_end_idx;
    t_word      loop_lim = set_loop_limit(found);

    /*
     *  Make sure we have some work to do before we start.
     */
    if (invert) {
        if (for_state->for_from < for_state->for_to)
            return 0;
    } else {
        if (for_state->for_from > for_state->for_to)
            return 0;
    }

    for_state->for_index = for_state->for_from;

    /*
     *  FROM `from' THROUGH `to' BY `by',
     *  DO...
     */
    for (;;) {
        int  next_ix;
        def_ent_t textDef;

        for_state->for_not_found = ! next_def(invert, &found);

        if (loop_lim-- < 0) {
            fprintf(trace_fp, TRACE_FOR_STEP_TOO_FAR,
                    pT->td_name, pMac->md_line);
            fprintf(trace_fp, TRACE_FOR_BY_STEP,
                    pT->td_text + pMac->md_txt_off,
                    for_state->for_from, for_state->for_to, for_state->for_by,
                    (int)OPT_VALUE_LOOP_LIMIT);
            break;
        }

        if (for_state->for_by != 0) {
            next_ix = for_state->for_index + for_state->for_by;

        } else if (invert) {
            next_ix = (found->de_ptwin == NULL)
                ? (int)(for_state->for_to - 1)  /* last iteration !! */
                : (int)found->de_ptwin->de_index;

        } else {
            next_ix = (found->de_twin == NULL)
                ? (int)(for_state->for_to + 1)  /* last iteration !! */
                : (int)found->de_twin->de_index;
        }

        /*
         *  IF we have a non-base definition, use the old def context
         */
        if (for_state->for_not_found)
            curr_def_ctx = for_state->for_ctx;

        /*
         *  ELSE IF this macro is a text type
         *  THEN create an un-twinned version of it to be found first
         */
        else if (found->de_type == VALTYP_TEXT) {
            textDef = *found;
            textDef.de_next    = textDef.de_twin = NULL;

            curr_def_ctx.dcx_defent = &textDef;
            curr_def_ctx.dcx_prev = &for_state->for_ctx;
        }

        /*
         *  ELSE the current definitions are based on the block
         *       macro's values
         */
        else {
            curr_def_ctx.dcx_defent = found->de_val.dvu_entry;
            curr_def_ctx.dcx_prev = &for_state->for_ctx;
        }

        for_state->for_islast = invert
            ? ((next_ix < for_state->for_to) ? true : false)
            : ((next_ix > for_state->for_to) ? true : false);

        if (OPT_VALUE_TRACE == TRACE_EVERYTHING)
            fprintf(trace_fp, FOR_ITERATION_FMT, "by-step",
                    for_state->for_name,
                    for_state->for_index, loopCt, next_ix,
                    for_state->for_isfirst ? "yes" : "no",
                    for_state->for_islast  ? "yes" : "no");

        switch (call_gen_block(for_state->for_env, pT, pMac+1, end_mac)) {
        case LOOP_JMP_OKAY:
        case LOOP_JMP_NEXT:
            break;

        case LOOP_JMP_BREAK:
            goto leave_loop;
        }

        loopCt++;
        for_state = curr_ivk_info->ii_for_data
            + (curr_ivk_info->ii_for_depth - 1);

        if (for_state->for_islast)
            break;

        if (  (for_state->for_sep_str != NULL)
           && (*for_state->for_sep_str != NUL))
            fputs(for_state->for_sep_str, cur_fpstack->stk_fp);

        fflush(cur_fpstack->stk_fp);
        for_state->for_isfirst = false;
        for_state->for_index   = next_ix;
    } leave_loop:

    return loopCt;
}

static int
for_each(templ_t * tpl, macro_t * mac, def_ent_t * def_ent)
{
    int       loopCt  = 0;
    macro_t * end_mac = tpl->td_macros + mac->md_end_idx;

    mac++;

    for (;;) {
        def_ent_t  txt_def_ent;

        /*
         *  IF this loops over a text macro,
         *  THEN create a definition that will be found *before*
         *       the repeated (twinned) copy.  That way, when it
         *       is found as a macro invocation, the current value
         *       will be extracted, instead of the value list.
         */
        if (def_ent->de_type == VALTYP_TEXT) {
            txt_def_ent = *def_ent;
            txt_def_ent.de_next = txt_def_ent.de_twin = NULL;

            curr_def_ctx.dcx_defent = &txt_def_ent;
        } else {
            curr_def_ctx.dcx_defent = def_ent->de_val.dvu_entry;
        }

        /*
         *  Set the global current index
         */
        for_state->for_index = (int)def_ent->de_index;

        /*
         *  Advance to the next twin
         */
        def_ent = def_ent->de_twin;
        if (def_ent == NULL)
            for_state->for_islast = true;

        if (OPT_VALUE_TRACE == TRACE_EVERYTHING) {
            int next = (def_ent == NULL)
                ? (for_state->for_index + 1) : def_ent->de_index;
            fprintf(trace_fp, FOR_ITERATION_FMT, "each",
                    for_state->for_name,
                    for_state->for_index, loopCt, next,
                    for_state->for_isfirst ? "yes" : "no",
                    for_state->for_islast  ? "yes" : "no");
        }

        switch (call_gen_block(for_state->for_env, tpl, mac, end_mac)) {
        case LOOP_JMP_OKAY:
        case LOOP_JMP_NEXT:
            break;

        case LOOP_JMP_BREAK:
            goto leave_loop;
        }

        loopCt++;
        for_state = curr_ivk_info->ii_for_data
            + (curr_ivk_info->ii_for_depth - 1);

        if (def_ent == NULL)
            break;
        for_state->for_isfirst = false;

        /*
         *  Emit the iteration separation
         */
        if (  ( for_state->for_sep_str != NULL)
           && (*for_state->for_sep_str != NUL))
            fputs(for_state->for_sep_str, cur_fpstack->stk_fp);
        fflush(cur_fpstack->stk_fp);
    } leave_loop:;

    return loopCt;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void
load_for_in(char const * pzSrc, size_t srcLen, templ_t * pT, macro_t * pMac)
{
    char * pzName = pT->td_text + pMac->md_name_off;
    int    ix     = 0;
    char * pz;
    def_ent_t * prev_ent = NULL;

    /*
     *  Find the first text value
     */
    pz = SPN_WHITESPACE_CHARS(pzSrc + 3);
    if (*pz == NUL)
        AG_ABEND_IN(pT, pMac, FOR_IN_LISTLESS);
    srcLen -= (size_t)(pz - pzSrc);
    pzSrc = pz;

    {
        size_t nmlen = strlen(pzName);

        pz = AGALOC(srcLen + 2 + nmlen, "copy FOR text");
        strcpy(pz, pzName);
        pzName = pz;
        pz += nmlen + 1;
    }

    memcpy(pz, pzSrc, srcLen);
    pz[srcLen] = NUL;

    do  {
        def_ent_t * pDef = new_def_ent();

        pDef->de_name    = pzName;
        pDef->de_index   = ix++;
        pDef->de_type    = VALTYP_TEXT;
        pDef->de_val.dvu_text = pz;

        switch (*pz) {
        case '\'':
        case '"':
            pz = span_quote(pz);
            /*
             *  Clean up trailing commas
             */
            pz = SPN_WHITESPACE_CHARS(pz);
            if (*pz == ',')
                pz++;
            break;

        default:
            for (;;) {
                char ch = *(pz++);
                switch (ch) {
                case ' ': case TAB: case '\f': case '\v': case NL:
                    pz[-1] = NUL;
                    if (*pz != ',')
                        break;
                    pz++;
                    /* FALLTHROUGH */

                case ',':
                    pz[-1] = NUL;
                    break;

                case NUL:
                    pz--;
                    break;

                default:
                    continue;
                }
                break;
            }
            break;
        }

        /*
         *  Clean up trailing white space
         */
        pz = SPN_WHITESPACE_CHARS(pz);

        /*
         *  IF there is a previous entry, link its twin to this one.
         *  OTHERWISE, it is the head of the twin list.
         *  Link to md_pvt.
         */
        if (prev_ent != NULL)
            prev_ent->de_twin = pDef;
        else
            pMac->md_pvt = pDef;

        prev_ent = pDef;
    } while (*pz != NUL);

    pMac->md_txt_off = 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*=macfunc FOR
 *
 *  what:    Emit a template block multiple times
 *  cindex:  looping, for
 *  cindex:  for loop
 *  handler_proc:
 *  load_proc:
 *
 *  desc:
 *  This macro has a slight variation on the standard syntax:
 *  @example
 *  FOR <value-name> [ <separator-string> ]
 *
 *  FOR <value-name> (...Scheme expression list)
 *
 *  FOR <value-name> IN "string" [ ... ]
 *  @end example
 *
 *  Other than for the last form, the first macro argument must be the name of
 *  an AutoGen value.  If there is no value associated with the name, the
 *  @code{FOR} template block is skipped entirely.  The scope of the @code{FOR}
 *  macro extends to the corresponding @code{ENDFOR} macro.  The last form will
 *  create an array of string values named @code{<value-name>} that only exists
 *  within the context of this @code{FOR} loop.  With this form, in order to
 *  use a @code{separator-string}, you must code it into the end of the
 *  template block using the @code{(last-for?)} predicate function
 *  (@pxref{SCM last-for?}).
 *
 *  If there are any arguments after the @code{value-name}, the initial
 *  characters are used to determine the form.  If the first character is
 *  either a semi-colon (@code{;}) or an opening parenthesis (@code{(}), then
 *  it is presumed to be a Scheme expression containing the FOR macro specific
 *  functions @code{for-from}, @code{for-by}, @code{for-to}, and/or
 *  @code{for-sep}.  @xref{AutoGen Functions}.  If it consists of an '@code{i}'
 *  an '@code{n}' and separated by white space from more text, then the
 *  @code{FOR x IN} form is processed.  Otherwise, the remaining text is
 *  presumed to be a string for inserting between each iteration of the loop.
 *  This string will be emitted one time less than the number of iterations of
 *  the loop.  That is, it is emitted after each loop, excepting for the last
 *  iteration.
 *
 *  If the from/by/to functions are invoked, they will specify which copies of
 *  the named value are to be processed.  If there is no copy of the named
 *  value associated with a particular index, the @code{FOR} template block
 *  will be instantiated anyway.  The template must use @code{found-for?}
 *  (@pxref{SCM found-for?}) or other methods for detecting missing
 *  definitions and emitting default text.  In this fashion, you can insert
 *  entries from a sparse or non-zero based array into a dense, zero based
 *  array.
 *
 *  @strong{NB:} the @code{for-from}, @code{for-to}, @code{for-by} and
 *  @code{for-sep} functions are disabled outside of the context of the
 *  @code{FOR} macro.  Likewise, the @code{first-for?}, @code{last-for?}
 *  @code{for-index}, and @code{found-for?} functions are disabled outside
 *  of the range of a @code{FOR} block.
 *
 *  @strong{Also:} the @code{<value-name>} must be a single level name,
 *  not a compound name (@pxref{naming values}).
 *
 *  @example
 *  [+FOR var (for-from 0) (for-to <number>) (for-sep ",") +]
 *  ... text with @code{var}ious substitutions ...[+
 *  ENDFOR var+]
 *  @end example
 *
 *  @noindent
 *  this will repeat the @code{... text with @code{var}ious
 *  substitutions ...} <number>+1 times.  Each repetition,
 *  except for the last, will have a comma @code{,} after it.
 *
 *  @example
 *  [+FOR var ",\n" +]
 *  ... text with @code{var}ious substitutions ...[+
 *  ENDFOR var +]
 *  @end example
 *
 *  @noindent
 *  This will do the same thing, but only for the index
 *  values of @code{var} that have actually been defined.
=*/
/*=macfunc ENDFOR
 *
 *  what:   Terminates the @code{FOR} function template block
 *  in-context:
 *
 *  desc:
 *    This macro ends the @code{FOR} function template block.
 *    For a complete description @xref{FOR}.
=*/
macro_t *
mFunc_For(templ_t * tpl, macro_t * mac)
{
    macro_t *   ret_mac = tpl->td_macros + mac->md_end_idx;
    def_ent_t * def;
    int         lp_ct;

    /*
     *  "md_pvt" is used by the "FOR x IN ..." variation of the macro.
     *  When parsed, a chain of text definitions are hung off of "md_pvt".
     *  "for_each()" will then chase through the linked list of text
     *  values.  This winds up being identical to [+ FOR var +] where
     *  a list of "var" values has been set.
     */
    if (mac->md_pvt != NULL)
        def = mac->md_pvt;
    else {
        bool indexed;
        def = find_def_ent(tpl->td_text + mac->md_name_off, &indexed);
        if (def == NULL) {
            if (OPT_VALUE_TRACE >= TRACE_BLOCK_MACROS) {
                fprintf(trace_fp, TRACE_FN_FOR_SKIP,
                        tpl->td_text + mac->md_name_off);

                if (OPT_VALUE_TRACE == TRACE_EVERYTHING)
                    fprintf(trace_fp, TAB_FILE_LINE_FMT,
                            tpl->td_file, mac->md_line);
            }

            return ret_mac;
        }
    }

    for_state = new_for_context();
    for_state->for_name = tpl->td_text + mac->md_name_off;

    if (OPT_VALUE_TRACE >= TRACE_BLOCK_MACROS)
        fprintf(trace_fp, TRACE_FN_FOR, tpl->td_text + mac->md_name_off,
                tpl->td_file, mac->md_line);

    /*
     *  Check for a FOR iterating based on scheme macros
     */
    if (tpl->td_text[ mac->md_txt_off ] == '(') {
        for_state->for_from  = \
        for_state->for_to    = UNASSIGNED;

        for_state->for_loading = true;
        (void) eval(tpl->td_text + mac->md_txt_off);
        for_state->for_loading = false;
        lp_ct = for_by_step(tpl, mac, def);

    } else {
        /*
         *  The FOR iterates over a list of definitions
         */
        AGDUPSTR(for_state->for_sep_str, tpl->td_text + mac->md_txt_off, "ss");
        lp_ct = for_each(tpl, mac, def);
    }

    if (OPT_VALUE_TRACE >= TRACE_BLOCK_MACROS) {
        fprintf(trace_fp, TRACE_FN_FOR_REPEAT, for_state->for_name, lp_ct);

        if (OPT_VALUE_TRACE == TRACE_EVERYTHING)
            fprintf(trace_fp, TAB_FILE_LINE_FMT, tpl->td_file, mac->md_line);
    }

    free_for_context(1);

    return ret_mac;
}

/**
 * allocate and zero out a FOR macro context.
 *
 * @returns the top of thecurr_ivk_info stack of contexts.
 */
static for_state_t *
new_for_context(void)
{
    for_state_t * res;

    /*
     *  Push the current FOR context onto a stack.
     */
    if (++(curr_ivk_info->ii_for_depth) > curr_ivk_info->ii_for_alloc) {
        void * dta = curr_ivk_info->ii_for_data;
        size_t sz;
        curr_ivk_info->ii_for_alloc += 5;
        sz  = (size_t)curr_ivk_info->ii_for_alloc * sizeof(for_state_t);

        if (dta == NULL)
             dta = AGALOC(sz, "Init FOR sate");
        else dta = AGREALOC(dta, sz, "FOR state");
        curr_ivk_info->ii_for_data = dta;
    }

    res = curr_ivk_info->ii_for_data + (curr_ivk_info->ii_for_depth - 1);
    memset(VOIDP(res), 0, sizeof(for_state_t));
    res->for_isfirst = true;
    res->for_ctx     = curr_def_ctx;
    curr_def_ctx.dcx_prev = &res->for_ctx;
    return res;
}

/**
 * Free up for loop contexts.
 * @param [in] pop_ct  the maximum number to free up
 */
LOCAL void
free_for_context(int pop_ct)
{
    if (curr_ivk_info->ii_for_depth == 0)
        return;

    for_state = curr_ivk_info->ii_for_data + (curr_ivk_info->ii_for_depth - 1);

    while (pop_ct-- > 0) {
        if (for_state->for_sep_str != NULL)
            AGFREE(for_state->for_sep_str);

        curr_def_ctx = (for_state--)->for_ctx;

        if (--(curr_ivk_info->ii_for_depth) <= 0) {
            AGFREE(curr_ivk_info->ii_for_data);
            curr_ivk_info->ii_for_data  = NULL;
            curr_ivk_info->ii_for_alloc = 0;
            curr_ivk_info->ii_for_depth = 0;
            for_state = NULL;
            break;
        }
    }
}

/**
 * Function to initiate loading of FOR block.
 * It must replace the dispatch table to handle the ENDFOR function,
 * which will unload the dispatch table.
 */
macro_t *
mLoad_For(templ_t * tpl, macro_t * mac, char const ** p_scan)
{
    char *        scan_out = tpl->td_scan; /* next text dest   */
    char const *  scan_in  = (char const *)mac->md_txt_off; /* macro text */
    size_t        in_len   = (size_t)mac->md_res;           /* macro len  */

    /*
     *  Save the global macro loading mode
     */
    load_proc_p_t const * save_load_procs = load_proc_table;

    static load_proc_p_t load_for_macro_procs[ FUNC_CT ] = { NULL };

    load_proc_table = load_for_macro_procs;

    /*
     *  IF this is the first time here, THEN set up the "FOR" mode callout
     *  table.  It is the standard table, except entries are inserted for
     *  functions that are enabled only while processing a FOR macro
     */
    if (load_for_macro_procs[0] == NULL) {
        memcpy(VOIDP(load_for_macro_procs), base_load_table,
               sizeof(base_load_table));
        load_for_macro_procs[ FTYP_ENDFOR ] = &mLoad_Ending;
    }

    if (in_len == 0)
        AG_ABEND_IN(tpl, mac, LD_FOR_NAMELESS);

    /*
     *  src points to the name of the iteration "variable"
     *  Special hack:  if the name is preceded by a `.',
     *  then the lookup is local-only and we will accept it.
     */
    mac->md_name_off = (uintptr_t)(tpl->td_scan - tpl->td_text);
    if (*scan_in == '.') {
        *(scan_out++) = *(scan_in++);
        if (! IS_VAR_FIRST_CHAR(*scan_in))
            scan_out--; /* force an error */
    }

    while (IS_VALUE_NAME_CHAR(*scan_in)) *(scan_out++) = *(scan_in++);
    *(scan_out++) = NUL;

    if (tpl->td_text[ mac->md_name_off ] == NUL)
        AG_ABEND_IN(tpl, mac, LD_FOR_INVALID_VAR);

    /*
     *  Skip space to the start of the text following the iterator name
     */
    scan_in = SPN_WHITESPACE_CHARS(scan_in);
    in_len -= (size_t)(scan_in - (char *)mac->md_txt_off);

    /* * * * *
     *
     *  No source -> zero offset to text
     */
    if ((ssize_t)in_len <= 0) {
        mac->md_txt_off = 0;
    }

    /* * * * *
     *
     *  FOR foo IN ...  -> no text, but we create an array of text values
     */
    else if (  (strneqvcmp(scan_in, LD_FOR_IN, 2) == 0)
            && IS_WHITESPACE_CHAR(scan_in[2])) {
        load_for_in(scan_in, in_len, tpl, mac);
    }

    /* * * * *
     *
     *  *EITHER* a:  FOR foo "<<separator>>"
     *  *OR*         FOR foo (scheme ...) ...
     */
    else {
        mac->md_txt_off = (uintptr_t)(scan_out - tpl->td_text);
        memmove(scan_out, scan_in, in_len);
        scan_out[in_len] = NUL;
        if (IS_QUOTE_CHAR(*scan_out))
            span_quote(scan_out);
        scan_out += in_len + 1;
    }
    /*
     * * * * */

    tpl->td_scan = scan_out;
    {
        macro_t * next_mac = parse_tpl(mac + 1, p_scan);
        if (*p_scan == NULL)
            AG_ABEND_IN(tpl, mac, LD_FOR_NO_ENDFOR);

        mac->md_end_idx = mac->md_sib_idx = (int)(next_mac - tpl->td_macros);

        load_proc_table = save_load_procs;
        return next_mac;
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
 * end of agen5/funcFor.c */
