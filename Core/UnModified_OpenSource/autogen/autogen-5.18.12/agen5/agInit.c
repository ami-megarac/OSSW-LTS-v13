
/**
 * @file agInit.c
 *
 *  Do all the initialization stuff.  For daemon mode, only
 *  children will return.
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

static char const * ld_lib_path = NULL;

/* = = = START-STATIC-FORWARD = = = */
static void
init_scm(void);

static char const *
make_quote_str(char const * str);

static void
dep_usage(char const * fmt, ...);

static void
add_sys_env(char * env_name);
/* = = = END-STATIC-FORWARD = = = */

#include "expr.ini"

/**
 * Various initializations.
 *
 * @param arg_ct  the count of program arguments, plus 1.
 * @param arg_vec the program name plus its arguments
 */
LOCAL void
initialize(int arg_ct, char ** arg_vec)
{
    putenv(C(char *, ld_lib_path));

    scribble_init();

    /*
     *  Initialize all the Scheme functions.
     */
    ag_init();
    init_scm();

    last_scm_cmd = NULL;
    processing_state = PROC_STATE_OPTIONS;

    process_ag_opts(arg_ct, arg_vec);
    ag_exit_code = AUTOGEN_EXIT_LOAD_ERROR;

    if (OPT_VALUE_TRACE > TRACE_NOTHING)
        SCM_EVAL_CONST(INIT_SCM_DEBUG_FMT);
}

static void
init_scm(void)
{
    last_scm_cmd = SCHEME_INIT_TEXT;

    {
        SCM ini_res = ag_scm_c_eval_string_from_file_line(
            SCHEME_INIT_TEXT, AG_TEXT_STRTABLE_FILE, SCHEME_INIT_TEXT_LINENO);
        AGDUPSTR(libguile_ver, scm2display(ini_res), "ini res");
    }

    {
        unsigned int maj, min, mic;
        switch (sscanf(libguile_ver, "%u.%u.%u", &maj, &min, &mic)) {
        case 2:
        case 3: break;
        default:
            AG_ABEND(aprf(GUILE_VERSION_BAD, libguile_ver));
            /* NOT_REACHED */
        }
        maj = min + (100 * maj);
        if ((GUILE_VERSION / 1000) != maj)
            AG_ABEND(aprf(GUILE_VERSION_WRONG, libguile_ver,
                          MK_STR(GUILE_VERSION)));
    }

    {
#       if GUILE_VERSION >= 200000
#         define SCHEME_INIT_DEBUG SCHEME_INIT_DEBUG_2_0
#       else
#         define SCHEME_INIT_DEBUG SCHEME_INIT_DEBUG_1_6
#       endif
        char * p = aprf(INIT_SCM_ERRS_FMT, SCHEME_INIT_DEBUG);
#       undef  SCHEME_INIT_DEBUG

        last_scm_cmd = p;
        ag_scm_c_eval_string_from_file_line(p, __FILE__, __LINE__);
        AGFREE(p);
    }
}

/**
 * make a name resilient to machinations made by 'make'.
 * Basically, dollar sign characters are doubled.
 *
 * @param str the input string
 * @returns a newly allocated string with the '$' characters doubled
 */
static char const *
make_quote_str(char const * str)
{
    size_t sz = strlen(str) + 1;
    char const * scan = str;
    char * res;

    for (;;) {
        char * p = strchr(scan, '$');
        if (p == NULL)
            break;
        sz++;
        scan = scan + 1;
    }

    res  = AGALOC(sz, "q name");
    scan = res;

    for (;;) {
        char * p = strchr(str, '$');

        if (p == NULL)
            break;
        sz = (size_t)(p - str) + 1;
        memcpy(res, str, sz);
        res += sz;
        str += sz;
        *(res++) = '$';
    }

    strcpy(res, str);
    return scan;
}

/**
 * Error in dependency specification
 *
 * @param fmt the error message format
 */
static void
dep_usage(char const * fmt, ...)
{
    char * msg;

    {
        va_list ap;
        va_start(ap, fmt);
        (void)vasprintf(&msg, fmt, ap);
        va_end(ap);
    }

    usage_message(USAGE_INVAL_DEP_OPT_FMT, msg);
    /* NOTREACHED */
}

/**
 * Configure a dependency option.
 * Handles any of these letters:  MFQTPGD as the first part of the option
 * argument.
 *
 * @param opts the autogen options data structure
 * @param pOptDesc the option descriptor for this option.
 */
LOCAL void
config_dep(tOptions * opts, tOptDesc * od)
{
    char const * opt_arg = od->optArg.argString;
    (void)opts;

    /*
     *  The option argument is optional.  Make sure we have one.
     */
    if (opt_arg == NULL)
        return;

    while (*opt_arg == 'M')  opt_arg++;
    opt_arg = SPN_WHITESPACE_CHARS(opt_arg);

    switch (*opt_arg) {
    case 'F':
        if (dep_file != NULL)
            dep_usage(CFGDEP_DUP_TARGET_MSG);

        opt_arg = SPN_WHITESPACE_CHARS(opt_arg + 1);
        AGDUPSTR(dep_file, opt_arg, "f name");
        break;

    case 'Q':
    case 'T':
    {
        bool quote_it = (*opt_arg == 'Q');

        if (dep_target != NULL)
            dep_usage(CFGDEP_DUP_TARGET_MSG);

        opt_arg = SPN_WHITESPACE_CHARS(opt_arg + 1);
        if (quote_it)
            dep_target = make_quote_str(opt_arg);
        else
            AGDUPSTR(dep_target, opt_arg, "t name");
        break;
    }

    case 'P':
        dep_phonies = true;
        break;

    case 'D':
    case 'G':
    case NUL:
        /*
         *  'D' and 'G' make sense to GCC, not us.  Ignore 'em.  If we
         *  found a NUL byte, then act like we found -MM on the command line.
         */
        break;

    default:
        dep_usage(CFGDEP_UNKNOWN_DEP_FMT, opt_arg);
    }
}

/**
 * Add a system name to the environment.  The input name is up-cased and
 * made to conform to environment variable names.  If not already in the
 * environment, it is added with the string value "1".
 *
 * @param env_name in/out: system name to export
 */
static void
add_sys_env(char * env_name)
{
    int i = 2;

    for (;;) {
        if (IS_UPPER_CASE_CHAR(env_name[i]))
            env_name[i] = (char)tolower(env_name[i]);
        else if (! IS_ALPHANUMERIC_CHAR(env_name[i]))
            env_name[i] = '_';

        if (env_name[ ++i ] == NUL)
            break;
    }

    /*
     *  If the user already has something in the environment, do not
     *  override it.
     */
    if (getenv(env_name) == NULL) {
        char * pz;

        if (OPT_VALUE_TRACE > TRACE_DEBUG_MESSAGE)
            fprintf(trace_fp, TRACE_ADD_TO_ENV_FMT, env_name);

        pz = aprf(ADD_SYS_ENV_VAL_FMT, env_name);
        putenv(pz);
    }
}

/**
 * Prepare the raft of environment variables.
 * This runs before Guile starts and grabs the value for LD_LIBRARY_PATH.
 * Guile likes to fiddle that.  When we run initialize(), we will force it
 * to match what we currently have.  Additionally, force our environment
 * to be "C" and export all the variables that describe our system.
 */
LOCAL void
prep_env(void)
{
    /*
     *  as of 2.0.2, Guile will fiddle with strings all on its own accord.
     *  Coerce the environment into being POSIX ASCII strings so it keeps
     *  its bloody stinking nose out of our data.
     */
    putenv(C(char *, LC_ALL_IS_C));

    /*
     *  If GUILE_WARN_DEPRECATED has not been defined, then likely we are
     *  not in a development environment and likely we don't want to give
     *  our users any angst.
     */
    if (getenv(GUILE_WARN_DEP_STR) == NULL)
        putenv(C(char *, GUILE_WARN_NO_ENV));

    ld_lib_path = getenv(LD_LIB_PATH_STR);
    if (ld_lib_path == NULL) {
        ld_lib_path = LD_LIB_PATH_PFX;

    } else {
        size_t psz = strlen(ld_lib_path) + 1;
        char * p = AGALOC(LD_LIB_PATH_PFX_LEN + psz, "lp");
        memcpy(p, LD_LIB_PATH_PFX, LD_LIB_PATH_PFX_LEN);
        memcpy(p + LD_LIB_PATH_PFX_LEN, ld_lib_path, psz);
        ld_lib_path = p;
    }

    /*
     *  Set the last resort search directories first (lowest priority)
     *  The lowest of the low is the config time install data dir.
     *  Next is the *current* directory of this executable.
     *  Last (highest priority) of the low priority is the library data dir.
     */
    SET_OPT_TEMPL_DIRS(DFT_TPL_DIR_DATA);
    SET_OPT_TEMPL_DIRS(DFT_TPL_DIR_RELATIVE);
    SET_OPT_TEMPL_DIRS(LIBDATADIR);

    {
        char z[ SCRIBBLE_SIZE+8 ] = "__autogen__";
#if defined(HAVE_SOLARIS_SYSINFO)
        static const int nm[] = {
            SI_SYSNAME, SI_HOSTNAME, SI_ARCHITECTURE, SI_HW_PROVIDER,
#ifdef      SI_PLATFORM
            SI_PLATFORM,
#endif
            SI_MACHINE };
        int  ix;
        long sz;

        add_sys_env(z);
        for (ix = 0; ix < sizeof(nm)/sizeof(nm[0]); ix++) {
            sz = sysinfo(nm[ix], z+2, sizeof(z) - 2);
            if (sz > 0) {
                sz += 2;
                while (z[sz-1] == NUL)  sz--;
                strcpy(z + sz, ADD_ENV_VARS_SUFFIX_FMT+2);
                add_sys_env(z);
            }
        }

#elif defined(HAVE_UNAME_SYSCALL)
        struct utsname unm;

        add_sys_env(z);
        if (uname(&unm) != 0)
            AG_CANT(UNAME_CALL_NAME, SYSCALL_NAME);

        if (snprintf(z+2, SCRIBBLE_SIZE, ADD_ENV_VARS_SUFFIX_FMT, unm.sysname)
            <= SCRIBBLE_SIZE)
            add_sys_env(z);

        if (snprintf(z+2, SCRIBBLE_SIZE, ADD_ENV_VARS_SUFFIX_FMT, unm.machine)
            <= SCRIBBLE_SIZE)
            add_sys_env(z);

        if (snprintf(z+2, SCRIBBLE_SIZE, ADD_ENV_VARS_SUFFIX_FMT, unm.nodename)
            <= SCRIBBLE_SIZE)
            add_sys_env(z);
#else

        add_sys_env(z);
#endif
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
 * end of agen5/agInit.c */
