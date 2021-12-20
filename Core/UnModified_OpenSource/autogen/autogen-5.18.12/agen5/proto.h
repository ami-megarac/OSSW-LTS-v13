/* -*- buffer-read-only: t -*- vi: set ro:
 *
 * Prototypes for agen5
 * Generated Mon Aug 29 14:35:43 PDT 2016
 */
#ifndef AGEN5_PROTO_H_GUARD
#define AGEN5_PROTO_H_GUARD 1

/*
 *  Extracted from agCgi.c
 */
static void
load_cgi(void);

/*
 *  Extracted from agDep.c
 */
static void
add_source_file(char const * pz);

static void
rm_source_file(char const * pz);

static void
add_target_file(char const * pz);

static void
rm_target_file(char const * pz);

static void
start_dep_file(void);

static void
wrap_up_depends(void);

/*
 *  Extracted from agInit.c
 */
static void
initialize(int arg_ct, char ** arg_vec);

static void
config_dep(tOptions * opts, tOptDesc * od);

static void
prep_env(void);

/*
 *  Extracted from agShell.c
 */
static void
close_server_shell(void);

static char *
shell_cmd(char const * cmd);

/*
 *  Extracted from agUtils.c
 */
static void
fswarn(char const * op, char const * fname);

static char *
aprf(char const * pzFmt, ...);

static void
process_ag_opts(int arg_ct, char ** arg_vec);

static char const *
get_define_str(char const * de_name, bool check_env);

static char *
span_quote(char * in_q);

static char const *
skip_scheme(char const * scan,  char const * end);

static char const *
skip_expr(char const * src, size_t len);

/*
 *  Extracted from autogen.c
 */
static _Noreturn void
ag_abend_at(char const * msg
#ifdef DEBUG_ENABLED
            , char const * fname, int line
#endif
    );

static void *
ao_malloc (size_t sz);

static void *
ao_realloc (void *p, size_t sz);

static char *
ao_strdup (char const * str);

/*
 *  Extracted from defDirect.c
 */
static char *
processDirective(char * scan);

/*
 *  Extracted from defFind.c
 */
static int
canonical_name(char * pzD, char const * pzS, int srcLen);

static def_ent_t *
find_def_ent(char * name, bool * indexed);

static void
print_used_defines(void);

static def_ent_t **
find_def_ent_list(char * name);

/*
 *  Extracted from defLex.c
 */
static te_dp_event
yylex(void);

static void
yyerror(char * s);

/*
 *  Extracted from defLoad.c
 */
static def_ent_t *
new_def_ent(void);

static void
print_ent(def_ent_t * de);

static void
delete_ent(def_ent_t * de);

static def_ent_t *
number_and_insert_ent(char * name, char const * idx_str);

static void
read_defs(void);

static void
unload_defs(void);

/*
 *  Extracted from expExtract.c
 */
static char *
load_file(char const * fname);

/*
 *  Extracted from expGuile.c
 */
static char *
ag_scm2zchars(SCM s, const char * type);

static teGuileType
ag_scm_type_e(SCM typ);

static SCM
ag_scm_c_eval_string_from_file_line(
    char const * pzExpr, char const * pzFile, int line);

/*
 *  Extracted from expOutput.c
 */
static void
make_readonly(void);

static void
open_output_file(char const * fname, size_t nmsz, char const * mode, int flags);

/*
 *  Extracted from expPrint.c
 */
static SCM
run_printf(char const * pzFmt, int len, SCM alist);

/*
 *  Extracted from expString.c
 */
static void
do_multi_subs(char ** ppzStr, ssize_t * pStrLen, SCM match, SCM repl);

/*
 *  Extracted from funcDef.c
 */
static void
parse_mac_args(templ_t * pT, macro_t * mac);

/*
 *  Extracted from funcEval.c
 */
static char const *
scm2display(SCM s);

static char const *
eval_mac_expr(bool * allocated);

static SCM
eval(char const * expr);

/*
 *  Extracted from funcFor.c
 */
static void
free_for_context(int pop_ct);

/*
 *  Extracted from functions.c
 */
static loop_jmp_type_t
call_gen_block(jmp_buf jbuf, templ_t * tpl, macro_t * mac, macro_t * end_mac);

static void
gen_new_block(templ_t * tpl);

/*
 *  Extracted from loadPseudo.c
 */
static char const *
do_suffix(char const * const text, char const * fname, int lineNo);

static char const *
load_pseudo_mac(char const * text, char const * fname);

/*
 *  Extracted from tpLoad.c
 */
static templ_t *
find_tpl(char const * tpl_name);

static tSuccess
find_file(char const * in_name,
          char *       res_name,
          char const * const * sfx_list,
          char const * referring_tpl);

static templ_t *
tpl_load(char const * fname, char const * referrer);

static void
tpl_unload(templ_t * tpl);

static void
cleanup(templ_t * tpl);

/*
 *  Extracted from tpParse.c
 */
static macro_t *
parse_tpl(macro_t * mac, char const ** p_scan);

/*
 *  Extracted from tpProcess.c
 */
static void
gen_block(templ_t * tpl, macro_t * mac, macro_t * emac);

static out_spec_t *
next_out_spec(out_spec_t * os);

static void
process_tpl(templ_t * tpl);

static void
out_close(bool purge);

#endif /* AGEN5_PROTO_H_GUARD */
