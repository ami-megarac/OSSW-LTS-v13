
/**
 * @file autogen.h
 *
 *  Global header file for AutoGen
 */
/**
 * @mainpage
 * @section Introduction
 * Autogen is a multi-component project.  There is the basic engine itself
 * ("autogen"), a library ("libopts") and its support templates (collectively,
 * "AutoOpts"), several support and utility programs ("columns", "getdefs" and
 * "xml2ag"), *plus* several handy embedded utility templates.  They are all
 * bundled together because they all require each other.
 * They each do completely separate things, but they each are not useful
 * without the other.  Thus, they are bundled together.
 *
 * @group autogen
 * @{
 */
/*  This file is part of AutoGen.
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
#ifndef AUTOGEN_BUILD
#define AUTOGEN_BUILD 1
#include <stdnoreturn.h>
#include "compat/unlocked-io.h"

#include REGEX_HEADER
#if !defined(__GNUC__)
#define GCC_VERSION 0
#elif ! defined(GCC_VERSION)
#define GCC_VERSION (__GNUC__ * 10000 \
                    + __GNUC_MINOR__ * 100 \
                    + __GNUC_PATCHLEVEL__)
#endif

#if GCC_VERSION > 40400
#pragma  GCC diagnostic push
#pragma  GCC diagnostic ignored "-Wextra"
#pragma  GCC diagnostic ignored "-Wconversion"
#pragma  GCC diagnostic ignored "-Wsign-conversion"
#pragma  GCC diagnostic ignored "-Wstrict-overflow"
#endif

#include <libguile/scmconfig.h>
#include <libguile.h>

#if GCC_VERSION > 40400
#pragma  GCC diagnostic pop
#endif

#include "ag-text.h"
#include "opts.h"
#include "expr.h"
#include "autoopts/autoopts.h"
#include "directive.h"
#include "snprintfv/printf.h"
#include "scribble.h"

#define  LOG10_2to32  10  /* rounded up */

#if defined(SHELL_ENABLED)
#  ifndef HAVE_WORKING_FORK
#    error SHELL is enabled and fork() does not work
     choke me
#  endif
#endif

#ifndef DIRCH
# if defined(_WIN32) && !defined(__CYGWIN__)
#  define DIRCH                  '\\'
# else
#  define DIRCH                  '/'
# endif
#endif

#define YYSTYPE t_word

#define ag_offsetof(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)

/*
 *  Dual pipe opening of a child process
 */
typedef struct {
    int     fd_read;
    int     fd_write;
}  fd_pair_t;

typedef struct {
    FILE *  fp_read;  /* parent read fp  */
    FILE *  fp_write; /* parent write fp */
}  fp_pair_t;

#define NOPROCESS   ((pid_t)-1)
#define NULLPROCESS ((pid_t)0)
#define NL          '\n'
#define TAB         '\t'

#include "cgi-fsm.h"
#include "defParse-fsm.h"

typedef union {
    unsigned char * pzStr;
    unsigned char   ch;
} def_token_u_t;

#define STATE_TABLE           /* set up `atexit' and load Guile   */  \
    _State_( INIT )           /* processing command line options  */  \
    _State_( OPTIONS )        /* Loading guile at option time     */  \
    _State_( GUILE_PRELOAD )  /* Loading value definitions        */  \
    _State_( LOAD_DEFS )      /* Loading library template         */  \
    _State_( LIB_LOAD )       /* Loading primary template         */  \
    _State_( LOAD_TPL )       /* processing templates             */  \
    _State_( EMITTING )       /* loading an included template     */  \
    _State_( INCLUDING )      /* end of processing before exit()  */  \
    _State_( CLEANUP )        /* Clean up code in error response  */  \
    _State_( ABORTING )       /* `exit' has been called           */  \
    _State_( DONE )

#define _State_(n)  PROC_STATE_ ## n,
typedef enum { STATE_TABLE COUNT_PROC_STATE } proc_state_t;
#undef _State_

#define EXPORT

typedef struct out_stack        out_stack_t;
typedef struct out_spec         out_spec_t;
typedef struct scan_context     scan_ctx_t;
typedef struct def_entry        def_ent_t;
typedef struct macro_desc       macro_t;
typedef struct template_desc    templ_t;
typedef struct for_state        for_state_t;
typedef struct tlib_mark        tlib_mark_t;

#define MAX_SUFFIX_LEN       8  /* maximum length of a file name suffix */
#define MAX_HEREMARK_LEN    64  /* max length of a here mark */
#define SCRIBBLE_SIZE      256  /* much larger than any short name */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  Template Library Layout
 *
 *  Procedure for loading a template function
 */
typedef macro_t * (load_proc_t)(templ_t *, macro_t *, char const ** ppzScan);
typedef load_proc_t * load_proc_p_t;

typedef void (unload_proc_t)(macro_t *);
typedef unload_proc_t * unload_proc_p_t;

/*
 *  Procedure for handling a template function
 *  during the text emission phase.
 */
typedef macro_t * (hdlr_proc_t)(templ_t *, macro_t *);
typedef hdlr_proc_t * hdlr_proc_p_t;

/*
 *  This must be included after the function prototypes
 *  (the prototypes are used in the generated tables),
 *  but before the macro descriptor structure (the function
 *  enumeration is generated here).
 */
#include "functions.h"

#define TEMPLATE_REVISION     1
#define TEMPLATE_MAGIC_MARKER {{{'A', 'G', 'L', 'B'}}, \
                               TEMPLATE_REVISION, FUNCTION_CKSUM }

struct tlib_mark {
    union {
        unsigned char   str[4];  /* {'A', 'G', 'L', 'B'} */
        unsigned int    i[1];
    }           tlm_magic;
    unsigned short  tlm_revision;   /* TEMPLATE_REVISION    */
    unsigned short  tlm_cksum;      /* FUNCTION_CKSUM       */
};

/**
 *  Defines for conditional expressions.  The first four are an enumeration
 *  that appear in the low four bits and the next-to-lowest four bits.
 *  "PRIMARY_TYPE" and "SECONDARY_TYPE" are masks for extracting this
 *  enumeration.  The rest are flags.
 */
#define EMIT_VALUE          0x0000  //!< emit value of variable
#define EMIT_EXPRESSION     0x0001  //!< Emit Scheme result
#define EMIT_SHELL          0x0002  //!< emit shell output
#define EMIT_STRING         0x0003  //!< emit content of expr
#define EMIT_PRIMARY_TYPE   0x0007  //!< mask for primary emission type
#define EMIT_SECONDARY_TYPE 0x0070  //!< mask for secondary emission type
#define EMIT_SECONDARY_SHIFT     4  //!< bit offset for secondary type
#define EMIT_IF_ABSENT      0x0100  //!< emit text when value non-existant
#define EMIT_ALWAYS         0x0200  //!< emit one of two exprs
#define EMIT_FORMATTED      0x0400  //!< format, if val present
#define EMIT_NO_DEFINE      0x0800  //!< don't get defined value

/**
 * template macro descriptor.
 */
struct macro_desc {
    mac_func_t    md_code;      //!< Macro function
    int           md_line;      //!< of macro def
    int           md_end_idx;   //!< End of block macro
    int           md_sib_idx;   //!< Sibling macro (ELIF or SELECT)

    uintptr_t     md_name_off;  //!< macro name (sometimes)
    uintptr_t     md_txt_off;   //!< associated text
    uintptr_t     md_res;       //!< some sort of result
    void *        md_pvt;       //!< private data for particular macro
};

/**
 * AutoGen template descriptor.
 * A full template or a defined macro is managed with this structure.
 */
struct template_desc {
    tlib_mark_t   td_magic;     //!< TEMPLATE_MAGIC_MARKER
    size_t        td_size;      //!< Structure Size
    char *        td_scan;      //!< Next Pointer
    int           td_mac_ct;    //!< Count of Macros
    char const *  td_file;      //!< Name of template file
    char *        td_name;      //!< Defined Macro Name
    char *        td_text;      //!< base address of the text
    char          td_start_mac[MAX_SUFFIX_LEN];
    char          td_end_mac[  MAX_SUFFIX_LEN];
    macro_t       td_macros[1]; //!< Array of Macros
//  char          td_text[...];  * strings are at end of macros
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  Name/Value Definitions Layout
 */
typedef enum {
    VALTYP_UNKNOWN = 0,
    VALTYP_TEXT,
    VALTYP_BLOCK
} val_typ_t;


#define NO_INDEX ((short)0x80DEAD)

typedef struct def_ctx def_ctx_t;
struct def_ctx {
    def_ent_t *   dcx_defent;   //!< ptr to current def set
    def_ctx_t *   dcx_prev;     //!< ptr to previous def set
};

typedef union {
    def_ent_t *   dvu_entry;
    char *        dvu_text;
} def_val_u;

struct def_entry {
    def_ent_t *   de_next;      //!< next member of same level
    def_ent_t *   de_twin;      //!< next member with same name
    def_ent_t *   de_ptwin;     //!< previous memb. of level
    def_ent_t *   de_etwin;     //!< head of chain to end ptr
    char *        de_name;      //!< name of this member
    long          de_index;     //!< index among twins
    def_val_u     de_val;       //!< string or list of children
    char *        de_file;      //!< definition file name
    int           de_line;      //!< def file source line
    val_typ_t     de_type;      //!< text/block/not defined yet
};

struct scan_context {
    scan_ctx_t *  scx_next;
    char *        scx_scan;
    char const *  scx_fname;
    char *        scx_data;
    int           scx_line;
};

struct out_spec {
    out_spec_t *  os_next;
    char const *  os_file_fmt;
    bool          os_dealloc_fmt;
    char          os_sfx[ 1 ];
};

/**
 * Output stack handling flags.
 */
#define FPF_FREE        0x0001  /*!< free the fp structure   */
#define FPF_UNLINK      0x0002  /*!< unlink file (temp file) */
#define FPF_NOUNLINK    0x0004  /*!< do not unlink file      */
#define FPF_STATIC_NM   0x0008  /*!< name statically alloced */
#define FPF_NOCHMOD     0x0010  /*!< do not chmod(2) file    */
#define FPF_TEMPFILE    0x0020  /*!< the file is a temp      */

struct out_stack {
    int           stk_flags;
    out_stack_t * stk_prev;
    FILE *        stk_fp;
    char const *  stk_fname;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  FOR loop processing state
 */
/**
 * The current state of each active FOR loop.
 */
struct for_state {
    def_ctx_t     for_ctx;      //!< saved def context for for loop
    char *        for_sep_str;  //!< inter-iteration string (allocated)
    char *        for_name;     //!< name of iterator (not allocated)
    int           for_from;     //!< the first index of loop
    int           for_to;       //!< the last index of loop
    int           for_by;       //!< the loop increment (usually 1)
    int           for_index;    //!< the current index
    bool          for_loading;  //!< the FOR macro is getting ready
    bool          for_islast;   //!< true for last iteration
    bool          for_isfirst;  //!< true for first iteration
    bool          for_not_found;//!< usually false, true with sparse arrays
    jmp_buf       for_env;      //!< long jump buffer (BREAK, CONTINUE)
};

typedef enum {
    LOOP_JMP_OKAY    = 0,
    LOOP_JMP_NEXT    = 1,
    LOOP_JMP_BREAK   = 2
} loop_jmp_type_t;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Invocation of a defined macro processing state
 */
typedef struct ivk_info ivk_info_t;
struct ivk_info {
    ivk_info_t *  ii_prev;      //!< previous layer
    int           ii_depth;     //!< Invocation nesting depth
    jmp_buf       ii_env;       //!< long jump buffer (RETURN)
    int           ii_for_depth; //!< for depth for this invocation
    int           ii_for_alloc; //!< for state buffer allocation count
    for_state_t * ii_for_data;  //!< array of "for" macro states
};
#define IVK_INFO_INITIALIZER(_p) {                      \
        .ii_prev  = (_p),                               \
        .ii_depth = curr_ivk_info->ii_depth + 1,        \
        .ii_for_depth = 0,                              \
        .ii_for_alloc = 0,                              \
        .ii_for_data  = NULL                            \
    }

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  Parsing stuff
 */
#define _MkStr(_s) #_s
#define MK_STR(_s) _MkStr(_s)

#define SCM_EVAL_CONST(_s)                                      \
    do { static int const line   = __LINE__ - 1;                \
        static char const file[] = __FILE__;                    \
        static char const * text = _s;                          \
        last_scm_cmd = text;                                    \
        ag_scm_c_eval_string_from_file_line(text, file, line);  \
    } while (false)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  GLOBAL VARIABLES
 *
 *  General Processing Globals
 */
#define ag_pname    autogenOptions.pzProgName
MODE proc_state_t    processing_state VALUE( PROC_STATE_INIT );
MODE unsigned int   include_depth    VALUE( 0 );
MODE bool           defining_macro   VALUE( false );
MODE templ_t *      named_tpls       VALUE( NULL );
MODE char const *   oops_pfx         VALUE( "" );
/*
 *  "eval_mac_expr" must be able to return a distinct empty string so that
 *  the "CASE" function can distinguish an empty string due to it being a
 *  value from an empty string due to an absent definition.
 */
MODE char const     no_def_str[1]    VALUE( "" );

/*
 *  Template Processing Globals
 */
MODE char const *   curr_sfx         VALUE( NULL );
/**
 * The time to set for the modification times of the output files.
 */
MODE time_t         outfile_time     VALUE( 0 );
MODE time_t         maxfile_time     VALUE( 0 );
/**
 * The original time autogen started
 */
MODE time_t         start_time       VALUE( 0 );
MODE out_stack_t *  cur_fpstack      VALUE( NULL );
MODE out_spec_t *   output_specs     VALUE( NULL );
MODE jmp_buf        abort_jmp_buf;
MODE ivk_info_t     root_ivk_info    VALUE( { 0 } );
MODE ivk_info_t *   curr_ivk_info    VALUE( &root_ivk_info );
MODE for_state_t *  for_state        VALUE( NULL );
MODE FILE *         trace_fp         VALUE( NULL );
/**
 * temporary file name template
 */
MODE char const *   pz_temp_tpl      VALUE( NULL );
/**
 * Length of the template that is the temp directory
 */
MODE size_t         temp_tpl_dir_len VALUE( 0 );
/**
 * dependency file file pointer.
 */
MODE FILE *         dep_fp           VALUE( NULL );
/**
 * name of target of rule
 */
MODE char const *   dep_target       VALUE( NULL );
/**
 * name of dependency file
 */
MODE char const *   dep_file         VALUE( NULL );
/**
 * base name of both source and derived files.
 * Either "_TList" or "_SList" gets put on the end.
 */
MODE char const *   pz_targ_base     VALUE( NULL );
/**
 * The actual list of input (source) files.
 */
MODE char const *   source_list      VALUE( NULL );
MODE size_t         source_size      VALUE( 0 );
MODE size_t         source_used      VALUE( 0 );
MODE bool           dep_phonies      VALUE( false );
MODE char *         cgi_stderr       VALUE( NULL );

MODE char const *   server_args[2]   VALUE( { NULL } );
MODE char const *   shell_program    VALUE( MK_STR(CONFIG_SHELL) );
MODE char const *   libguile_ver     VALUE( NULL );

/*
 *  AutoGen definiton and template context
 *
 *  curr_def_ctx is the current, active list of name/value pairs.
 *  Points to its parent list for full search resolution.
 *
 *  current_tpl the template (and DEFINE macro) from which
 *  the current set of macros is being extracted.
 *
 *  These are set in exactly ONE place:
 *  On entry to the dispatch routine (gen_block).  Two routines, however,
 *  must restore the values: mFunc_Define and mFunc_For.  They are the only
 *  routines that dynamically push name/value pairs on the definition stack.
 */
MODE def_ctx_t      curr_def_ctx     VALUE( { NULL } );
MODE def_ctx_t      root_def_ctx     VALUE( { NULL } );
MODE templ_t *      current_tpl      VALUE( NULL );
MODE char const *   last_scm_cmd     VALUE( NULL );

/*
 *  Current Macro
 *
 *  This may be set in exactly three places:
 *  1.  The dispatch routine (gen_block) that steps through
 *      a list of macros
 *  2.  mFunc_If may transfer to one of its 'ELIF' or 'ELSE'
 *      alternation macros
 *  3.  mFunc_Case may transfer to one of its selection clauses.
 */
MODE macro_t *       cur_macro        VALUE( NULL );
MODE tlib_mark_t const magic_marker   VALUE( TEMPLATE_MAGIC_MARKER );

/*
 *  Template Parsing Globals
 */
MODE int            tpl_line         VALUE( 1 );
MODE scan_ctx_t *   base_ctx         VALUE( NULL );
MODE scan_ctx_t *   cctx             VALUE( NULL );
MODE scan_ctx_t *   end_ctx          VALUE( NULL );
MODE size_t         end_mac_len      VALUE( 0  );
MODE char           end_mac_mark[8]  VALUE( "" );
MODE size_t         st_mac_len       VALUE( 0  );
MODE char           st_mac_mark[8]   VALUE( "" );
MODE out_stack_t    out_root         VALUE({ 0 });

#define name_sep_ch '.'

/*
 *  Definition Parsing Globals
 */
MODE char *         token_str        VALUE( NULL );
MODE te_dp_event    token_code       VALUE( DP_EV_INVALID );

MODE int            ent_stack_depth  VALUE( 0 );
MODE int            ent_stack_sz     VALUE( 16 );
MODE def_ent_t *    dft_ent_stack[16] VALUE( { 0 } );
MODE def_ent_t **   ent_stack        VALUE( dft_ent_stack );
MODE def_ent_t *    curr_ent         VALUE( NULL );

MODE autogen_exit_code_t ag_exit_code VALUE( AUTOGEN_EXIT_OPTION_ERROR );

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  IF we have fopencookie or funopen, then we also have our own fmemopen
 */
#ifdef ENABLE_FMEMOPEN
extern FILE * ag_fmemopen(void *buf, ssize_t len, char const *mode);
extern int    ag_fmemioctl(FILE * fp, int req, ...);
#endif

typedef union {
    const void * cp;
    void *       p;
} v2c_t;
MODE v2c_t p2p VALUE( { NULL } );

#ifdef DEBUG_ENABLED
# define AG_ABEND(s)  ag_abend_at(s,__FILE__,__LINE__)
#else
# define AG_ABEND(s)  ag_abend_at(s)
#endif
#define  AG_CANT(_op, _wh) \
    AG_ABEND(aprf(CANNOT_FMT, errno, _op, _wh, strerror(errno)))

#ifdef DEBUG_FSM
# define DEBUG
#else
# undef  DEBUG
#endif

#define AG_ABEND_IN(t,m,s) \
    STMTS( current_tpl=(t); cur_macro=(m); AG_ABEND(s);)

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

/*
 *  Code variations based on the version of Guile:
 */
#include "guile-iface.h"

#include "proto.h"

/**
 * Evaluate a scheme expression, setting the file and line number from
 * the file and line of the currently active macro.
 *
 * @param[in] str  the scheme expression
 * @returns the SCM result.  That may be SCM_UNDEFINED.
 */
static inline SCM ag_eval(char const * str)
{
    SCM res;
    char const * sv = last_scm_cmd; /* Watch for nested calls */
    last_scm_cmd = str;

    res = ag_scm_c_eval_string_from_file_line(
        str, current_tpl->td_file, cur_macro->md_line);

    last_scm_cmd = sv;
    return res;
}

/**
 *  Extracted from guile-iface stuff.  Seems to be stable since for at least
 *  1.6.0 through 2.0.0.  1.4.x is thoroughly dead now (May, 2011).
 */
#define AG_SCM_DISPLAY(_s) \
    scm_display(_s, scm_current_output_port())

#define AG_SCM_BOOT_GUILE(_ac, _av, _im) \
    scm_boot_guile((_ac), (_av), (_im), NULL)

#define AG_SCM_APPLY2(_op, _f, _tst) \
    scm_apply(_op, _f, scm_cons(_tst, scm_list_1(SCM_EOL)))

#define AG_SCM_CHAR_P(_c)            SCM_CHARP(_c)

/**
 * Hide dummy functions from complexity measurement tools
 */
#define HIDE_FN(_t)  _t

#define LOCAL static
#endif /* AUTOGEN_BUILD */
/** @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/autogen.h */
