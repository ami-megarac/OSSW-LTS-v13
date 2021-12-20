
/**
 *  @file autogen.c
 *
 *  This is the main routine for autogen.
 *  @addtogroup autogen
 *  @{
 */

/*  This file is part of AutoGen.
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
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif

#ifdef SIGRTMIN
# define MAX_AG_SIG SIGRTMIN-1
#else
# define MAX_AG_SIG NSIG
#endif
#ifndef SIGCHLD
#  define SIGCHLD SIGCLD
#endif

typedef void (void_main_proc_t)(int, char **);

typedef enum {
    EXIT_PCLOSE_WAIT,
    EXIT_PCLOSE_NOWAIT
} wait_for_pclose_enum_t;

#define _State_(n)  #n,
static char const * const state_names[] = { STATE_TABLE };
#undef _State_

typedef void (sighandler_proc_t)(int sig);
static sighandler_proc_t ignore_signal, catch_sig_and_bail;

/* = = = START-STATIC-FORWARD = = = */
static void
inner_main(void * closure, int argc, char ** argv);

static void
exit_cleanup(wait_for_pclose_enum_t cl_wait);

static _Noreturn void
cleanup_and_abort(int sig);

static void
catch_sig_and_bail(int sig);

static void
ignore_signal(int sig);

static void
done_check(void);

static void
setup_signals(sighandler_proc_t * hdl_chld,
              sighandler_proc_t * hdl_abrt,
              sighandler_proc_t * hdl_dflt);
/* = = = END-STATIC-FORWARD = = = */

#ifndef HAVE_CHMOD
#  include "compat/chmod.c"
#endif

/**
 * main routine under Guile guidance
 *
 * @param  closure Guile closure parameter.  Not used.
 * @param  argc    argument count
 * @param  argv    argument vector
 */
static void
inner_main(void * closure, int argc, char ** argv)
{
    atexit(done_check);
    initialize(argc, argv);

    processing_state = PROC_STATE_LOAD_DEFS;
    read_defs();

    /*
     *  Activate the AutoGen specific Scheme functions.
     *  Then load, process and unload the main template
     */
    processing_state = PROC_STATE_LOAD_TPL;

    {
        templ_t * tpl = tpl_load(tpl_fname, NULL);

        processing_state = PROC_STATE_EMITTING;
        process_tpl(tpl);

        processing_state = PROC_STATE_CLEANUP;
        cleanup(tpl);
    }

    processing_state = PROC_STATE_DONE;
    setup_signals(SIG_DFL, SIG_IGN, SIG_DFL);
    ag_exit_code = AUTOGEN_EXIT_SUCCESS;
    done_check();
    /* NOTREACHED */
    (void)closure;
}

/**
 * main() called from _start()
 *
 * @param  argc    argument count
 * @param  argv    argument vector
 * @return nothing -- Guile never returns, but calls exit(2).
 */
int
main(int argc, char ** argv)
{
    {
        char const * lc = getenv("LC_ALL");
        if ((lc != NULL) && (*lc != NUL) && (strcmp(lc, "C") != 0)) {
            static char lc_all[] = "LC_ALL=C";
            putenv(lc_all);
            execvp(argv[0], argv);
            fserr(AUTOGEN_EXIT_FS_ERROR, "execvp", argv[0]);
        }
    }

    // setlocale(LC_ALL, "");
    setup_signals(ignore_signal, SIG_DFL, catch_sig_and_bail);
    optionSaveState(&autogenOptions);
    trace_fp = stderr;
    prep_env();

    AG_SCM_BOOT_GUILE(argc, argv, inner_main);

    /* NOTREACHED */
    return EXIT_FAILURE;
}

/**
 * This code must run regardless of which exit path is taken
 *
 * @param [in] cl_wait Whether or not a child process should be waited for.
 */
static void
exit_cleanup(wait_for_pclose_enum_t cl_wait)
{
    /*
     *  There are contexts wherein this function can get run twice.
     */
    {
        static int exit_cleanup_done = 0;
        if (exit_cleanup_done) {
            if (OPT_VALUE_TRACE > TRACE_NOTHING)
                fprintf(trace_fp, EXIT_CLEANUP_MULLIGAN);
            return;
        }

        exit_cleanup_done = 1;
    }

#ifdef SHELL_ENABLED
    SCM_EVAL_CONST(EXIT_CLEANUP_STR);
#endif

    close_server_shell();

    if (OPT_VALUE_TRACE > TRACE_NOTHING)
        fprintf(trace_fp, EXIT_CLEANUP_DONE_FMT,
                (cl_wait == EXIT_PCLOSE_WAIT)
                ? EXIT_CLEANUP_WAITED : EXIT_CLEANUP_NOWAIT,
                (unsigned int)getpid());

    do  {
        if (trace_fp == stderr)
            break;

        if (! trace_is_to_pipe) {
            fclose(trace_fp);
            break;
        }


        fflush(trace_fp);
        pclose(trace_fp);
        if (cl_wait == EXIT_PCLOSE_WAIT) {
            int status;
            while (wait(&status) > 0)  ;
        }
    } while (false);

    trace_fp = stderr;
    fflush(stdout);
    fflush(stderr);
}

/**
 *  A signal was caught, siglongjmp called and main() has called this.
 *  We do not deallocate stuff so it can be found in the core dump.
 *
 *  @param[in] sig the signal number
 */
static _Noreturn void
cleanup_and_abort(int sig)
{
    if (processing_state == PROC_STATE_INIT) {
        fprintf(stderr, AG_NEVER_STARTED, sig, strsignal(sig));
        exit(AUTOGEN_EXIT_SIGNAL + sig);
    }

    if (*oops_pfx != NUL) {
        fputs(oops_pfx, stderr);
        oops_pfx = zNil;
    }

    fprintf(stderr, AG_SIG_ABORT_FMT, sig, strsignal(sig),
            ((unsigned)processing_state <= PROC_STATE_DONE)
            ? state_names[ processing_state ] : BOGUS_TAG);

    if (processing_state == PROC_STATE_ABORTING) {
        exit_cleanup(EXIT_PCLOSE_NOWAIT);
        abort();
    }

    processing_state = PROC_STATE_ABORTING;
    setup_signals(SIG_DFL, SIG_DFL, SIG_DFL);

    /*
     *  IF there is a current template, then we should report where we are
     *  so that the template writer knows where to look for their problem.
     */
    if (current_tpl != NULL) {
        int line;
        mac_func_t fnCd;
        char const * pzFn;
        char const * pzFl;

        if (cur_macro == NULL) {
            pzFn = PSEUDO_MACRO_NAME_STR;
            line = 0;
            fnCd = (mac_func_t)-1;

        } else {
            mac_func_t f =
                (cur_macro->md_code > FUNC_CT)
                    ? FTYP_SELECT : cur_macro->md_code;
            pzFn = ag_fun_names[ f ];
            line = cur_macro->md_line;
            fnCd = cur_macro->md_code;
        }
        pzFl = current_tpl->td_file;
        if (pzFl == NULL)
            pzFl = NULL_FILE_NAME_STR;
        fprintf(stderr, AG_ABORT_LOC_FMT, pzFl, line, pzFn, fnCd);
    }

#ifdef HAVE_SYS_RESOURCE_H
    /*
     *  If `--core' has been specified and if we have get/set rlimit,
     *  then try to set the core limit to the "hard" maximum before aborting.
     */
    if (HAVE_OPT(CORE)) {
        struct rlimit rlim;
        getrlimit(RLIMIT_CORE, &rlim);
        rlim.rlim_cur = rlim.rlim_max;
        setrlimit(RLIMIT_CORE, &rlim);
    }
#endif

    exit_cleanup(EXIT_PCLOSE_NOWAIT);
    abort();
}


/**
 *  catch_sig_and_bail catches signals we abend on.  The "siglongjmp"
 *  goes back to the real "main()" procedure and it will call
 *  "cleanup_and_abort()", above.
 *
 *  @param[in] sig the signal number
 */
static void
catch_sig_and_bail(int sig)
{
    switch (processing_state) {
    case PROC_STATE_ABORTING:
        ag_exit_code = AUTOGEN_EXIT_SIGNAL + sig;

    case PROC_STATE_DONE:
        break;

    default:
        cleanup_and_abort(sig);
    }
}


/**
 *  ignore_signal is the handler for SIGCHLD.  If we set it to default,
 *  it will kill us.  If we set it to ignore, it will be inherited.
 *  Therefore, always in all programs set it to call a procedure.
 *  The "wait(3)" call will do magical things, but will not override SIGIGN.
 *
 *  @param[in] sig the signal number
 */
static void
ignore_signal(int sig)
{
    (void)sig;
    return;
}


/**
 *  This is _always_ called for exit processing.
 *  This only returns if "exit(3C)" is called during option processing.
 *  We have no way of determining the correct exit code.
 *  (Requested version or help exits EXIT_SUCCESS.  Option failures
 *  are EXIT_FAILURE.  We cannot determine here.)
 */
static void
done_check(void)
{
    /*
     *  There are contexts wherein this function can get called twice.
     */
    {
        static int done_check_done = 0;
        if (done_check_done) {
            if (OPT_VALUE_TRACE > TRACE_NOTHING)
                fprintf(trace_fp, DONE_CHECK_REDONE);
            return;
        }
        done_check_done = 1;
    }

    switch (processing_state) {
    case PROC_STATE_EMITTING:
    case PROC_STATE_INCLUDING:
        /*
         * A library (viz., Guile) procedure has called exit(3C).  The
         * AutoGen abort paths all set processing_state to PROC_STATE_ABORTING.
         */
        if (*oops_pfx != NUL) {
            /*
             *  Emit the CGI page header for an error message.  We will rewind
             *  stderr and write the contents to stdout momentarily.
             */
            fputs(oops_pfx, stdout);
            oops_pfx = zNil;
        }

        fprintf(stderr, SCHEME_EVAL_ERR_FMT, current_tpl->td_file,
                cur_macro->md_line);

        /*
         *  We got here because someone called exit early.
         */
        do  {
#ifndef DEBUG_ENABLED
            out_close(false);
#else
            out_close(true);
#endif
        } while (cur_fpstack->stk_prev != NULL);
        ag_exit_code = AUTOGEN_EXIT_BAD_TEMPLATE;
        break; /* continue failure exit */

    case PROC_STATE_LOAD_DEFS:
        ag_exit_code = AUTOGEN_EXIT_BAD_DEFINITIONS;
        /* FALLTHROUGH */

    default:
        fprintf(stderr, AG_ABEND_STATE_FMT, state_names[processing_state]);
        goto autogen_aborts;

    case PROC_STATE_ABORTING:
        ag_exit_code = AUTOGEN_EXIT_BAD_TEMPLATE;

    autogen_aborts:
        if (*oops_pfx != NUL) {
            /*
             *  Emit the CGI page header for an error message.  We will rewind
             *  stderr and write the contents to stdout momentarily.
             */
            fputs(oops_pfx, stdout);
            oops_pfx = zNil;
        }
        break; /* continue failure exit */

    case PROC_STATE_OPTIONS:
        /* Exiting in option processing state is verbose enough */
        break;

    case PROC_STATE_DONE:
        break; /* continue normal exit */
    }

    if (last_scm_cmd != NULL)
        fprintf(stderr, GUILE_CMD_FAIL_FMT, last_scm_cmd);

    /*
     *  IF we diverted stderr, then now is the time to copy the text to stdout.
     *  This is done for CGI mode wherein we produce an error page in case of
     *  an error, but otherwise discard stderr.
     */
    if (cgi_stderr != NULL) {
        do {
            long pos = ftell(stderr);
            char * pz;
            size_t rdlen;

            /*
             *  Don't bother with the overhead if there is no work to do.
             */
            if (pos <= 0)
                break;
            pz = AGALOC(pos, "stderr redir");
            rewind(stderr);
            rdlen = fread( pz, (size_t)1, (size_t)pos, stderr);
            fwrite(pz, (size_t)1, rdlen, stdout);
            AGFREE(pz);
        } while (false);

        unlink(cgi_stderr);
        AGFREE(cgi_stderr);
        cgi_stderr = NULL;
    }

    scribble_deinit();

    if (OPT_VALUE_TRACE > TRACE_NOTHING)
        fprintf(trace_fp, DONE_CHECK_DONE);

    exit_cleanup(EXIT_PCLOSE_WAIT);

    /*
     *  When processing options, return so that the option processing exit code
     *  is used.  Otherwise, terminate execution now with the decided upon
     *  exit code.  (Always EXIT_FAILURE, unless this was called at the end
     *  of inner_main().)
     */
    if (processing_state != PROC_STATE_OPTIONS)
        exit(ag_exit_code);
}


LOCAL _Noreturn void
ag_abend_at(char const * msg
#ifdef DEBUG_ENABLED
            , char const * fname, int line
#endif
    )
{
    if (*oops_pfx != NUL) {
        fputs(oops_pfx, stderr);
        oops_pfx = zNil;
    }

#ifdef DEBUG_ENABLED
    fprintf(stderr, "Giving up in %s line %d\n", fname, line);
#endif

    if ((processing_state >= PROC_STATE_LIB_LOAD) && (current_tpl != NULL)) {
        int l_no = (cur_macro == NULL) ? -1 : cur_macro->md_line;
        fprintf(stderr, ERROR_IN_TPL_FMT, current_tpl->td_file, l_no);
    }
    fputs(msg, stderr);
    msg += strlen(msg);
    if (msg[-1] != NL)
        fputc(NL, stderr);

#ifdef DEBUG_ENABLED
    abort();
#else
    {
        proc_state_t o_state = processing_state;
        processing_state = PROC_STATE_ABORTING;

        switch (o_state) {
        case PROC_STATE_EMITTING:
        case PROC_STATE_INCLUDING:
        case PROC_STATE_CLEANUP:
            longjmp(abort_jmp_buf, FAILURE);
            /* NOTREACHED */
        default:
            abort();
            /* NOTREACHED */
        }
    }
#endif
}

static void
setup_signals(sighandler_proc_t * hdl_chld,
              sighandler_proc_t * hdl_abrt,
              sighandler_proc_t * hdl_dflt)
{
    struct sigaction  sa;
    int    sigNo  = 1;
    int const maxSig = MAX_AG_SIG;

    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);

    do  {
        switch (sigNo) {
            /*
             *  Signal handling for SIGCHLD needs to be ignored.  Do *NOT* use
             *  SIG_IGN to do this.  That gets inherited by programs that need
             *  to be able to use wait(2) calls.  At the same time, we don't
             *  want our children to become zombies.  We may run out of zombie
             *  space.  Therefore, set the handler to an empty procedure.
             *  POSIX oversight.  Allowed to be fixed for next POSIX rev, tho
             *  it is "optional" to reset SIGCHLD on exec(2).
             */
        case SIGCHLD:
            sa.sa_handler = hdl_chld;
            break;

        case SIGABRT:
            sa.sa_handler = hdl_abrt;
            break;

            /*
             *  Signals we must leave alone.
             */
        case SIGKILL:
        case SIGSTOP:

            /*
             *  Signals we choose to leave alone.
             */
#ifdef SIGTSTP
        case SIGTSTP:
#endif
#ifdef SIGPROF
        case SIGPROF:
#endif
            continue;

#if defined(DEBUG_ENABLED)
        case SIGBUS:
        case SIGSEGV:
            /*
             *  While debugging AutoGen, we want seg faults to happen and
             *  trigger core dumps.  Make sure this happens.
             */
            sa.sa_handler = SIG_DFL;
            break;
#endif

            /*
             *  Signals to ignore with SIG_IGN.
             */
        case 0: /* cannot happen, but the following might not be defined */
#ifdef SIGWINCH
        case SIGWINCH:
#endif
#ifdef SIGTTIN
        case SIGTTIN:  /* tty input  */
#endif
#ifdef SIGTTOU
        case SIGTTOU:  /* tty output */
#endif
            sa.sa_handler = SIG_IGN;
            break;

        default:
            sa.sa_handler = hdl_dflt;
        }
        sigaction(sigNo,  &sa, NULL);
    } while (++sigNo < maxSig);
}

#ifndef HAVE_STRFTIME
#  include <compat/strftime.c>
#endif

#ifndef HAVE_STRSIGNAL
#  include <compat/strsignal.c>
#endif

LOCAL void *
ao_malloc (size_t sz)
{
    void * res = malloc(sz);
    if (res == NULL)
        die(AUTOGEN_EXIT_FS_ERROR, MALLOC_FAIL_FMT, sz);
    return res;
}


LOCAL void *
ao_realloc (void *p, size_t sz)
{
    void * res = (p == NULL) ? malloc(sz) : realloc(p, sz);
    if (res == NULL)
        die(AUTOGEN_EXIT_FS_ERROR, REALLOC_FAIL_FMT, sz, p);
    return res;
}

LOCAL char *
ao_strdup (char const * str)
{
    char * res = strdup(str);
    if (res == NULL)
        die(AUTOGEN_EXIT_FS_ERROR, STRDUP_FAIL_FMT, (int)strlen(str));
    return res;
}

#ifdef __GNUC__
    void ignore_vars(void);
    void ignore_vars(void) {
        (void)option_load_mode, (void)program_pkgdatadir;
    }
#endif
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/autogen.c */
