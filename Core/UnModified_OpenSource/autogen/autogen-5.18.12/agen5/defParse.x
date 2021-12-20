
/**
 * @file defParse.x
 *
 * Definition parser functions.
 *
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

#ifdef FSM_USER_HEADERS
static char * pz_new_name = NULL;
#endif /* FSM_USER_HEADERS */

#ifdef FSM_FIND_TRANSITION
trans_evt = yylex();
#endif /* FSM_FIND_TRANSITION */

#ifdef FSM_HANDLER_CODE
/*
 *  Print out an invalid transition message and return EXIT_FAILURE
 */
static int
dp_invalid_transition(te_dp_state st, te_dp_event evt)
{
    char const * fmt_pz = zDpStrings + DpFsmErr_off;
    fprintf(stderr, fmt_pz, st, DP_STATE_NAME(st), evt, DP_EVT_NAME(evt));

    return EXIT_FAILURE;
}

static te_dp_state
dp_do_empty_val(te_dp_state initial, te_dp_state maybe_next,
                te_dp_event trans_evt)
{
    /*
     *  Our state is either "have-name" or "indx-name" and we found a ';',
     *  end of statement.  It is a string value with an empty string.
     */
    def_ent_t * pDE = number_and_insert_ent(pz_new_name, NULL);

    (void)initial; (void)trans_evt;

    pDE->de_val.dvu_text = (char *)zNil;
    pDE->de_type    = VALTYP_TEXT;
    if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
        print_ent(pDE);
    return maybe_next;
}

static te_dp_state
dp_do_end_block(te_dp_state initial, te_dp_state maybe_next,
                te_dp_event trans_evt)
{
    if (ent_stack_depth <= 0)
        yyerror(VOIDP("Too many close braces"));

    (void)initial; (void)trans_evt;

    curr_ent = ent_stack[ ent_stack_depth-- ];
    return maybe_next;
}

static te_dp_state
dp_do_need_value_delete_ent(te_dp_state initial, te_dp_state maybe_next,
                            te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    delete_ent(curr_ent);
    return maybe_next;
}

static te_dp_state
dp_do_have_name_lit_eq(te_dp_state initial, te_dp_state maybe_next,
                       te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    /*
     *  Create a new entry but defer "makeMacro" call until we have the
     *  assigned value.
     */
    number_and_insert_ent(pz_new_name, NULL);
    return maybe_next;
}

static te_dp_state
dp_do_indexed_name(te_dp_state initial, te_dp_state maybe_next,
                   te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    /*
     *  Create a new entry with a specified indes, but defer "makeMacro" call
     *  until we have the assigned value.
     */
    number_and_insert_ent(pz_new_name, token_str);
    return maybe_next;
}

static te_dp_state
dp_do_invalid(te_dp_state initial, te_dp_state maybe_next,
              te_dp_event trans_evt)
{
    (void)maybe_next;
    dp_invalid_transition(initial, trans_evt);
    yyerror(VOIDP("invalid transition"));
    /* NOTREACHED */
    return DP_ST_INVALID;
}

static te_dp_state
dp_do_need_name_end(te_dp_state initial, te_dp_state maybe_next,
                    te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    if (ent_stack_depth != 0)
        yyerror(VOIDP("definition blocks were left open"));

    /*
     *  We won't be using the parse stack any more.
     *  We only process definitions once.
     */
    if (ent_stack != dft_ent_stack)
        AGFREE(ent_stack);

    /*
     *  The seed has now done its job.  The real root of the
     *  definitions is the first entry off of the seed.
     */
    root_def_ctx.dcx_defent = root_def_ctx.dcx_defent->de_val.dvu_entry;
    return maybe_next;
}

static te_dp_state
dp_do_need_name_var_name(te_dp_state initial, te_dp_state maybe_next,
                         te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    pz_new_name = token_str;
    return maybe_next;
}

static te_dp_state
dp_do_next_val(te_dp_state initial, te_dp_state maybe_next,
               te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    /*
     *  Clone the entry name of the current entry.
     */
    number_and_insert_ent(curr_ent->de_name, NULL);
    return maybe_next;
}

static te_dp_state
dp_do_start_block(te_dp_state initial, te_dp_state maybe_next,
                  te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    if (curr_ent->de_type == VALTYP_TEXT)
        yyerror(VOIDP("assigning a block value to text name"));
    curr_ent->de_type = VALTYP_BLOCK; /* in case not yet determined */

    if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
        print_ent(curr_ent);

    if (++ent_stack_depth >= ent_stack_sz) {
        def_ent_t ** ppDE;
        ent_stack_sz += ent_stack_sz / 2;

        if (ent_stack == dft_ent_stack) {
            ppDE = AGALOC(ent_stack_sz * sizeof(void *), "def stack");
            memcpy(ppDE, dft_ent_stack, sizeof(dft_ent_stack));
        } else {
            ppDE = AGREALOC(ent_stack, ent_stack_sz * sizeof(void *),
                            "stretch def stack");
        }
        ent_stack = ppDE;
    }
    ent_stack[ ent_stack_depth ] = curr_ent;
    curr_ent = NULL;
    return maybe_next;
}

static te_dp_state
dp_do_str_value(te_dp_state initial, te_dp_state maybe_next,
                te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    if (curr_ent->de_type == VALTYP_BLOCK)
        yyerror(VOIDP("assigning a block value to text name"));

    curr_ent->de_val.dvu_text = token_str;
    curr_ent->de_type = VALTYP_TEXT;

    if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
        print_ent(curr_ent);

    /*
     *  The "here string" marker is the line before where the text starts.
     */
    if (trans_evt == DP_EV_HERE_STRING)
        curr_ent->de_line++;
    return maybe_next;
}

static te_dp_state
dp_do_tpl_name(te_dp_state initial, te_dp_state maybe_next,
               te_dp_event trans_evt)
{
    (void)initial; (void)trans_evt;
    /*
     *  Allow this routine to be called multiple times.
     *  This may happen if we include another definition file.
     */
    if (root_def_ctx.dcx_defent == NULL) {
        static char const zBogus[] = "@BOGUS@";
        static def_ent_t  seed = {
            NULL, NULL, NULL, NULL, (char *)zBogus, 0, { NULL },
            (char *)zBogus, 0, VALTYP_BLOCK };

        root_def_ctx.dcx_defent = &seed;

        if (! HAVE_OPT(OVERRIDE_TPL))
             tpl_fname = token_str;

        ent_stack_depth = 0;
        ent_stack[0] = &seed;
    }
    return maybe_next;
}
#endif /* FSM_HANDLER_CODE */

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/defParse.x */
