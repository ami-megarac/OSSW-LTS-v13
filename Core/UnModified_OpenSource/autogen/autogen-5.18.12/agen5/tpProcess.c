
/**
 * @file tpProcess.c
 *
 *  Parse and process the template data descriptions
 *
 * @addtogroup autogen
 * @{
 */
/*
 * This file is part of AutoGen.
 * AutoGen Copyright (C) 1992-2016 by Bruce Korb - all rights reserved
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
trace_macro(templ_t * tpl, macro_t * mac);

static void
do_stdout_tpl(templ_t * tpl);

static void
open_output(out_spec_t * spec);
/* = = = END-STATIC-FORWARD = = = */

/**
 *  Generate all the text within a block.
 *  The caller must know the exact bounds of the block.
 *
 * @param tpl   template containing block of macros
 * @param mac   first macro in series
 * @param emac  one past last macro in series
 */
LOCAL void
gen_block(templ_t * tpl, macro_t * mac, macro_t * emac)
{
    /*
     *  Set up the processing context for this block of macros.
     *  It is used by the Guile callback routines and the exception
     *  handling code.  It is all for user friendly diagnostics.
     */
    current_tpl = tpl;

    while ((mac != NULL) && (mac < emac)) {
        mac_func_t fc = mac->md_code;
        if (fc >= FUNC_CT)
            fc = FTYP_BOGUS;

        scribble_free();
        if (OPT_VALUE_TRACE >= TRACE_EVERYTHING)
            trace_macro(tpl, mac);

        cur_macro = mac;
        mac = (*(load_procs[ fc ]))(tpl, mac);
    }
}

/**
 *  Print out information about the invocation of a macro.
 *  Print up to the first 32 characters in the macro, for context.
 *
 * @param tpl   template containing macros
 * @param mac   first macro in series
 */
static void
trace_macro(templ_t * tpl, macro_t * mac)
{
    mac_func_t fc = mac->md_code;
    if (fc >= FUNC_CT)
        fc = FTYP_BOGUS;

    fprintf(trace_fp, TRACE_MACRO_FMT, ag_fun_names[fc], mac->md_code,
            tpl->td_file, mac->md_line);

    if (mac->md_txt_off > 0) {
        char * pz = tpl->td_text + mac->md_txt_off;
        char * pe = BRK_NEWLINE_CHARS(pz);
        if (pe > pz + 32)
            pz = pz + 32;

        putc(' ', trace_fp); putc(' ', trace_fp);
        fwrite(pz, (size_t)(pe - pz), 1, trace_fp);
        putc(NL, trace_fp);
    }
}

/**
 *  The template output goes to stdout.  Perhaps because output
 *  is for a CGI script.  In any case, this case must be handled
 *  specially.
 *
 * @param tpl   template to be processed
 */
static void
do_stdout_tpl(templ_t * tpl)
{
    SCM res;

    last_scm_cmd = NULL; /* We cannot be in Scheme processing */

    switch (setjmp(abort_jmp_buf)) {
    case SUCCESS:
        break;

    case PROBLEM:
        if (*oops_pfx != NUL) {
            fprintf(stdout, DO_STDOUT_TPL_ABANDONED, oops_pfx);
            oops_pfx = zNil;
        }
        fclose(stdout);
        return;

    default:
        fserr(AUTOGEN_EXIT_FS_ERROR, DO_STDOUT_TPL_BADR, oops_pfx);

    case FAILURE:
        exit(EXIT_FAILURE);
        /* NOTREACHED */
    }

    curr_sfx           = DO_STDOUT_TPL_NOSFX;
    curr_def_ctx       = root_def_ctx;
    cur_fpstack        = &out_root;
    out_root.stk_fp    = stdout;
    out_root.stk_fname = DO_STDOUT_TPL_STDOUT;
    out_root.stk_flags = FPF_NOUNLINK | FPF_STATIC_NM;
    if (OPT_VALUE_TRACE >= TRACE_EVERYTHING)
        fputs(DO_STDOUT_TPL_START_STD, trace_fp);

    /*
     *  IF there is a CGI prefix for error messages,
     *  THEN divert all output to a temporary file so that
     *  the output will be clean for any error messages we have to emit.
     */
    if (*oops_pfx == NUL)
        gen_block(tpl, tpl->td_macros, tpl->td_macros + tpl->td_mac_ct);

    else {
        char const * pzRes;
        (void)ag_scm_out_push_new(SCM_UNDEFINED);

        gen_block(tpl, tpl->td_macros, tpl->td_macros + tpl->td_mac_ct);

        /*
         *  Read back in the spooled output.  Make sure it starts with
         *  a content-type: prefix.  If not, we supply our own HTML prefix.
         */
        res   = ag_scm_out_pop(SCM_BOOL_T);
        pzRes = scm_i_string_chars(res);

        /* 13 char prefix is:  "content-type:" */
        if (strneqvcmp(pzRes, DO_STDOUT_TPL_CONTENT, 13) != 0)
            fputs(DO_STDOUT_TPL_CONTENT, stdout);

        fwrite(pzRes, scm_c_string_length(res), 1, stdout);
    }

    fclose(stdout);
}

/**
 * pop the current output spec structure.  Deallocate it and the
 * file name, too, if necessary.
 */
LOCAL out_spec_t *
next_out_spec(out_spec_t * os)
{
    out_spec_t * res = os->os_next;

    if (os->os_dealloc_fmt)
        AGFREE(os->os_file_fmt);

    AGFREE(os);
    return res;
}

LOCAL void
process_tpl(templ_t * tpl)
{
    /*
     *  IF the template file does not specify any output suffixes,
     *  THEN we will generate to standard out with the suffix set to zNoSfx.
     *  With output going to stdout, we don't try to remove output on errors.
     */
    if (output_specs == NULL) {
        do_stdout_tpl(tpl);
        return;
    }

    do  {
        out_spec_t * os;

        /*
         * We cannot be in Scheme processing.  We've either just started
         * or we've made a long jump from our own code.  If we've made a
         * long jump, we've printed a message that is sufficient and we
         * don't need to print any scheme expressions.
         */
        last_scm_cmd = NULL;

        /*
         *  HOW was that we got here?
         */
        switch (setjmp(abort_jmp_buf)) {
        case SUCCESS:
            os = output_specs;

            if (OPT_VALUE_TRACE >= TRACE_EVERYTHING) {
                fprintf(trace_fp, PROC_TPL_START, os->os_sfx);
                fflush(trace_fp);
            }
            /*
             *  Set the output file name buffer.
             *  It may get switched inside open_output.
             */
            open_output(os);
            memcpy(&out_root, cur_fpstack, sizeof(out_root));
            AGFREE(cur_fpstack);
            cur_fpstack    = &out_root;
            curr_sfx       = os->os_sfx;
            curr_def_ctx   = root_def_ctx;
            cur_fpstack->stk_flags &= ~FPF_FREE;
            cur_fpstack->stk_prev   = NULL;
            gen_block(tpl, tpl->td_macros, tpl->td_macros+tpl->td_mac_ct);

            do  {
                out_close(false);  /* keep output */
            } while (cur_fpstack->stk_prev != NULL);

            output_specs = next_out_spec(os);
            break;

        case PROBLEM:
            os = output_specs;
            /*
             *  We got here by a long jump.  Close/purge the open files
             *  and go on to the next output.
             */
            do  {
                out_close(true);  /* discard output */
            } while (cur_fpstack->stk_prev != NULL);
            last_scm_cmd = NULL; /* "problem" means "drop current output". */
            output_specs = next_out_spec(os);
            break;

        default:
            fprintf(trace_fp, PROC_TPL_BOGUS_RET, oops_pfx);
            oops_pfx = zNil;
            /* FALLTHROUGH */

        case FAILURE:
            os = output_specs;

            /*
             *  We got here by a long jump.  Close/purge the open files.
             */
            do  {
                out_close(true);  /* discard output */
            } while (cur_fpstack->stk_prev != NULL);

            /*
             *  On failure (or unknown jump type), we quit the program, too.
             */
            processing_state = PROC_STATE_ABORTING;
            while (os != NULL)
                os = next_out_spec(os);

            exit(EXIT_FAILURE);
            /* NOTREACHED */
        }
    } while (output_specs != NULL);
}


LOCAL void
out_close(bool purge)
{
    if ((cur_fpstack->stk_flags & FPF_NOCHMOD) == 0)
        make_readonly();

    if (OPT_VALUE_TRACE > TRACE_DEBUG_MESSAGE)
        fprintf(trace_fp, OUT_CLOSE_TRACE_WRAP, __func__,
                cur_fpstack->stk_fname);

    fclose(cur_fpstack->stk_fp);

    /*
     *  Only stdout and /dev/null are marked, "NOUNLINK"
     */
    if ((cur_fpstack->stk_flags & FPF_NOUNLINK) == 0) {
        /*
         *  IF we are told to purge the file OR the file is an AutoGen temp
         *  file, then get rid of the output.
         */
        if (purge || ((cur_fpstack->stk_flags & FPF_UNLINK) != 0))
            unlink(cur_fpstack->stk_fname);

        else {
            struct utimbuf tbuf;

            tbuf.actime  = time(NULL);
            tbuf.modtime = outfile_time;

            /*
             *  The putative start time is one second earlier than the
             *  earliest output file time, regardless of when that is.
             */
            if (outfile_time <= start_time)
                start_time = outfile_time - 1;

            utime(cur_fpstack->stk_fname, &tbuf);
        }
    }

    /*
     *  Do not deallocate statically allocated names
     */
    if ((cur_fpstack->stk_flags & FPF_STATIC_NM) == 0)
        AGFREE(cur_fpstack->stk_fname);

    /*
     *  Do not deallocate the root entry.  It is not allocated!!
     */
    if ((cur_fpstack->stk_flags & FPF_FREE) != 0) {
        out_stack_t * p = cur_fpstack;
        cur_fpstack = p->stk_prev;
        AGFREE(p);
    }
}

/**
 *  Figure out what to use as the base name of the output file.
 *  If an argument is not provided, we use the base name of
 *  the definitions file.
 */
static void
open_output(out_spec_t * spec)
{
    static char const write_mode[] = "w" FOPEN_BINARY_FLAG "+";

    char const * out_file = NULL;

    if (strcmp(spec->os_sfx, OPEN_OUTPUT_NULL) == 0) {
        static int const flags = FPF_NOUNLINK | FPF_NOCHMOD | FPF_TEMPFILE;
    null_open:
        open_output_file(DEV_NULL, DEV_NULL_LEN, write_mode, flags);
        return;
    }

    /*
     *  IF we are to skip the current suffix,
     *  we will redirect the output to /dev/null and
     *  perform all the work.  There may be side effects.
     */
    if (HAVE_OPT(SKIP_SUFFIX)) {
        int     ct  = STACKCT_OPT(SKIP_SUFFIX);
        const char ** ppz = STACKLST_OPT(SKIP_SUFFIX);

        while (--ct >= 0) {
            if (strcmp(spec->os_sfx, *ppz++) == 0)
                goto null_open;
        }
    }

    /*
     *  Remove any suffixes in the last file name
     */
    {
        char const * def_file = OPT_ARG(BASE_NAME);
        char   z[AG_PATH_MAX];
        const char * pst = strrchr(def_file, '/');
        char * end;

        pst = (pst == NULL) ? def_file : (pst + 1);

        /*
         *  We allow users to specify a suffix with '-' and '_', but when
         *  stripping a suffix from the "base name", we do not recognize 'em.
         */
        end = strchr(pst, '.');
        if (end != NULL) {
            size_t len = (unsigned)(end - pst);
            if (len >= sizeof(z))
                AG_ABEND(BASE_NAME_TOO_LONG);

            memcpy(z, pst, len);
            z[ end - pst ] = NUL;
            pst = z;
        }

        /*
         *  Now formulate the output file name in the buffer
         *  provided as the input argument.
         */
        out_file = aprf(spec->os_file_fmt, pst, spec->os_sfx);
        if (out_file == NULL)
            AG_ABEND(aprf(OPEN_OUTPUT_BAD_FMT, spec->os_file_fmt, pst,
                          spec->os_sfx));
    }

    open_output_file(out_file, strlen(out_file), write_mode, 0);
    free(VOIDP(out_file));
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/tpProcess.c */
