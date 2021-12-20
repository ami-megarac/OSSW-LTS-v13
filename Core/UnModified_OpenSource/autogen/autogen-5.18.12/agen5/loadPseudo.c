
/**
 * @file loadPseudo.c
 *
 *  Find the start and end macro markers.  In btween we must find the
 *  "autogen" and "template" keywords, followed by any suffix specs.
 *
 *  This module processes the "pseudo" macro.
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

#define DEFINE_FSM
#include "pseudo-fsm.h"

/* = = = START-STATIC-FORWARD = = = */
static char const *
do_scheme_expr(char const * text, char const * fname);

static char const *
handle_hash_line(char const * pz);

static te_pm_event
next_pm_token(char const ** ptext, te_pm_state fsm_state, char const * fnm);

static char const *
copy_mark(char const * text, char * marker, size_t * ret_ct);
/* = = = END-STATIC-FORWARD = = = */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  do_scheme_expr
 *
 *  Process a scheme specification
 */
static char const *
do_scheme_expr(char const * text, char const * fname)
{
    char *    pzEnd = (char *)text + strlen(text);
    char      ch;
    macro_t * pCM   = cur_macro;
    macro_t   mac   = { (mac_func_t)~0, 0, 0, 0, 0, 0, 0, NULL };

    mac.md_line = tpl_line;
    pzEnd       = (char *)skip_scheme(text, pzEnd);
    ch          = *pzEnd;
    *pzEnd      = NUL;
    cur_macro   = &mac;

    ag_scm_c_eval_string_from_file_line(
          (char *)text, fname, tpl_line );

    cur_macro = pCM;
    *pzEnd    = ch;
    while (text < pzEnd)
        if (*(text++) == NL)
            tpl_line++;
    return (char const *)pzEnd;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  do_suffix
 *
 *  Process a suffix specification
 */
LOCAL char const *
do_suffix(char const * const text, char const * fname, int lineNo)
{
    /*
     *  The following is the complete list of POSIX required-to-be-legal
     *  file name characters.  These are the only characters we allow to
     *  appear in a suffix.  We do, however, add '=' and '%' because we
     *  also allow a format specification to follow the suffix,
     *  separated by an '=' character.
     */
    out_spec_t *       pOS;
    char const *       pzSfxFmt;
    char const *       pzResult;
    size_t             spn;
    static out_spec_t ** ppOSList = &output_specs;

    /*
     *  Skip over the suffix construct
     */
    pzSfxFmt = SPN_SUFFIX_CHARS(text);

    if (*pzSfxFmt != '=') {
        pzResult = pzSfxFmt;
        pzSfxFmt = NULL;

    } else {
        pzSfxFmt++;

        if (*pzSfxFmt == '(') {
            char const *pe  = pzSfxFmt + strlen(pzSfxFmt);
            pzResult = skip_scheme(pzSfxFmt, pe);

        } else {
            pzResult = SPN_SUFFIX_FMT_CHARS(pzSfxFmt);

            if (pzSfxFmt == pzResult)
                AG_ABEND(DO_SUFFIX_EMPTY);
        }
    }

    /*
     *  If fname is NULL, then we are called by --select-suffix.
     *  Otherwise, the suffix construct is saved only for the main template,
     *  and only when the --select-suffix option was not specified.
     */
    if (  (fname != NULL)
       && (  (processing_state != PROC_STATE_LOAD_TPL)
          || HAVE_OPT(SELECT_SUFFIX)))
        return pzResult;

    /*
     *  Allocate Output Spec and link into the global list.  Copy all the
     *  "spanned" text, including any '=' character, scheme expression or
     *  file name format string.
     */
    spn = (size_t)(pzResult - text);
    {
        size_t sz = sizeof(*pOS) + spn + 1;
        pOS = AGALOC(sz, "Output Specification");
        memset(pOS, NUL, sz);
    }

    *ppOSList  = pOS;
    ppOSList   = &pOS->os_next; /* NULL, from memset */
    memcpy(pOS->os_sfx, text, spn);
    pOS->os_sfx[spn] = NUL;

    /*
     *  IF the suffix contains its own formatting construct,
     *  THEN split it off from the suffix and set the formatting ptr.
     *  ELSE supply a default.
     */
    if (pzSfxFmt != NULL) {
        size_t sfx_len = (size_t)(pzSfxFmt - text);
        pOS->os_sfx[sfx_len-1] = NUL;
        pOS->os_file_fmt = pOS->os_sfx + sfx_len;

        if (*pOS->os_file_fmt == '(') {
            SCM str =
                ag_scm_c_eval_string_from_file_line(
                    pOS->os_file_fmt, fname, lineNo );
            size_t str_length;
            char const * pz;

            pzSfxFmt = pz = scm2display(str);
            str_length = strlen(pzSfxFmt);

            if (str_length == 0)
                AG_ABEND(DO_SUFFIX_EMPTY);
            pz = SPN_SUFFIX_FMT_CHARS(pz);

            if ((unsigned)(pz - pzSfxFmt) != str_length)
                AG_ABEND(aprf(DO_SUFFIX_BAD_CHARS, pz));

            /*
             *  IF the scheme replacement text fits in the space, don't
             *  mess with allocating another string.
             */
            if (str_length < spn - sfx_len)
                strcpy(pOS->os_sfx + sfx_len, pzSfxFmt);
            else {
                AGDUPSTR(pOS->os_file_fmt, pzSfxFmt, "suffix format");
                pOS->os_dealloc_fmt = true;
            }
        }

    } else {
        /*
         *  IF the suffix does not start with punctuation,
         *  THEN we will insert a '.' of our own.
         */
        pOS->os_file_fmt = IS_VAR_FIRST_CHAR(pOS->os_sfx[0])
            ? DOT_SFX_FMT : SFX_FMT;
    }

    return pzResult;
}

static char const *
handle_hash_line(char const * pz)
{
    char const * res = strchr(pz, NL);
    if (res == NULL)
        AG_ABEND(HANDLE_HASH_BAD_TPL);

    /*
     *  If the comment starts with "#!/", then see if it names
     *  an executable.  If it does, it is specifying a shell to use.
     */
    if ((pz[1] == '!') && (pz[2] == '/')) {
        char const * pzScn = pz + 3;
        char *  nmbuf;
        size_t len;

        pzScn = SPN_FILE_NAME_CHARS(pzScn);

        len   = (size_t)(pzScn - (pz + 2));
        nmbuf = scribble_get((ssize_t)len);
        memcpy(nmbuf, pz+2, len);
        nmbuf[len] = NUL;

        /*
         *  If we find the executable, then change the configured shell and
         *  the SHELL environment variable to this executable.
         */
        if (access(nmbuf, X_OK) == 0) {
            char * sp = AGALOC(len + HANDLE_HASH_SHELL_LEN + 1, "set shell");
            memcpy(sp, HANDLE_HASH_SHELL, HANDLE_HASH_SHELL_LEN);
            memcpy(sp + HANDLE_HASH_SHELL_LEN, nmbuf, len + 1);
            putenv(sp);
            AGDUPSTR(shell_program,  nmbuf, "prog shell");
            AGDUPSTR(server_args[0], nmbuf, "shell name");
        }
    }

    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  next_pm_token
 *
 *  Skiping leading white space, figure out what sort of token is under
 *  the scan pointer (text).
 */
static te_pm_event
next_pm_token(char const ** ptext, te_pm_state fsm_state, char const * fnm)
{
    char const * text = *ptext;

    /*
     *  At the start of processing in this function, we can never be at
     *  the "start of a line".  A '#' type comment before the initial
     *  start macro marker is illegal.  Otherwise, our scan pointer is
     *  after some valid token, which won't be the start of a line, either.
     */
    bool line_start = false;

 skipWhiteSpace:
    while (IS_WHITESPACE_CHAR(*text)) {
        if (*(text++) == NL) {
            line_start = true;
            tpl_line++;

            /*
             *  IF we are done with the macro markers,
             *  THEN we skip white space only thru the first new line.
             */
            if (fsm_state == PM_ST_END_MARK) {
                *ptext = text;
                return PM_EV_END_PSEUDO;
            }
        }
    }

    if (line_start && (*text == '#')) {
        text = handle_hash_line(text);
        goto skipWhiteSpace;
    }

    *ptext = text; /* in case we return */

    /*
     *  After the end marker has been found,
     *  anything else is really the start of the data.
     */
    if (fsm_state == PM_ST_END_MARK)
        return PM_EV_END_PSEUDO;

    /*
     *  IF the token starts with an alphanumeric,
     *  THEN it must be "autogen5" or "template" or a suffix specification
     */
    if (IS_VAR_FIRST_CHAR(*text)) {
        if (strneqvcmp(text, AG_MARK, AG_MARK_LEN) == 0) {
            if (IS_WHITESPACE_CHAR(text[ AG_MARK_LEN ])) {
                *ptext = text + AG_MARK_LEN + 1;
                return PM_EV_AUTOGEN;
            }

            return PM_EV_SUFFIX;
        }

        if (  (strneqvcmp(text, TPL_MARK, TPL_MARK_LEN) == 0)
           && (IS_WHITESPACE_CHAR(text[ TPL_MARK_LEN ])) ) {
            *ptext = text + TPL_MARK_LEN;
            return PM_EV_TEMPLATE;
        }

        return PM_EV_SUFFIX;
    }

    /*
     *  Handle emacs mode markers and scheme expressions only once we've
     *  gotten past "init" state.
     */
    if (fsm_state > PM_ST_INIT)
        switch (*text) {
        case '-':
            if ((text[1] == '*') && (text[2] == '-'))
                return PM_EV_ED_MODE;
            break;

        case '(':
            return PM_EV_SCHEME;
        }

    /*
     *  Alphanumerics and underscore are already handled.  Thus, it must be
     *  a punctuation character that may introduce a suffix:  '.' '-' '_'
     */
    if (IS_SUFFIX_CHAR(*text))
        return PM_EV_SUFFIX;

    /*
     *  IF it is some other punctuation,
     *  THEN it must be a start/end marker.
     */
    if (IS_PUNCTUATION_CHAR(*text))
        return PM_EV_MARKER;

    /*
     *  Otherwise, it is just junk.
     */
    AG_ABEND(aprf(NEXT_PM_TOKEN_INVALID, fnm));
    /* NOTREACHED */
    return PM_EV_INVALID;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Some sort of marker is under the scan pointer.  Copy it for as long
 *  as we find punctuation characters.
 */
static char const *
copy_mark(char const * text, char * marker, size_t * ret_ct)
{
    size_t ct = 0;

    for (;;) {
        char ch = *text;
        if (! IS_PUNCTUATION_CHAR(ch))
            break;
        *(marker++) = ch;
        if (++ct >= sizeof(st_mac_mark))
            return NULL;

        text++;
    }

    *ret_ct = ct;
    *marker = NUL;

    if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
        fprintf(trace_fp, TRACE_COPY_MARK, marker - ct);

    return text;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Using a finite state machine, scan over the tokens that make up the
 *  "pseudo macro" at the start of every template.
 *
 *  @param[in,out]  text    text of template
 *  @param[in]      fname   name of template file
 *  @returns the address of the byte following the pseudo macro
 */
LOCAL char const *
load_pseudo_mac(char const * text, char const * fname)
{
    char const * pzBadness;
#   define BAD_MARKER(t) { pzBadness = t; goto abort_load; }

    te_pm_state fsm_state  = PM_ST_INIT;

    tpl_line  = 1;

    while (fsm_state != PM_ST_DONE) {
        te_pm_event fsm_tkn = next_pm_token(&text, fsm_state, fname);
        te_pm_state nxt_state;
        te_pm_trans trans;

        nxt_state  = pm_trans_table[ fsm_state ][ fsm_tkn ].next_state;
        trans      = pm_trans_table[ fsm_state ][ fsm_tkn ].transition;

        /*
         *  There are only so many "PM_TR_<state-name>_<token-name>"
         *  transitions that are legal.  See which one we got.
         *  It is legal to alter "nxt_state" while processing these.
         */
        switch (trans) {
        case PM_TR_SKIP_ED_MODE:
        {
            char * pzEnd = strstr(text + 3, PSEUDO_MAC_MODE_MARK);
            char * pzNL  = strchr(text + 3, NL);
            if ((pzEnd == NULL) || (pzNL < pzEnd))
                BAD_MARKER(PSEUDO_MAC_BAD_MODE);

            text = pzEnd + 3;
            break;
        }

        case PM_TR_INIT_MARKER:
            text = copy_mark(text, st_mac_mark, &st_mac_len);
            if (text == NULL)
                BAD_MARKER(PSEUDO_MAC_BAD_LENGTH);

            break;

        case PM_TR_TEMPL_MARKER:
            text = copy_mark(text, end_mac_mark, &end_mac_len);
            if (text == NULL)
                BAD_MARKER(PSEUDO_MAC_BAD_LENGTH);

            /*
             *  IF the end macro seems to end with the start macro and
             *  it is exactly twice as long as the start macro, then
             *  presume that someone ran the two markers together.
             */
            if (  (end_mac_len == 2 * st_mac_len)
               && (strcmp(st_mac_mark, end_mac_mark + st_mac_len) == 0))  {
                text -= st_mac_len;
                end_mac_mark[ st_mac_len ] = NUL;
                end_mac_len = st_mac_len;
            }

            if (strstr(end_mac_mark, st_mac_mark) != NULL)
                BAD_MARKER(PSEUDO_MAC_BAD_ENDER);
            if (strstr(st_mac_mark, end_mac_mark) != NULL)
                BAD_MARKER(PSEUDO_MAC_BAD_STARTER);
            break;

        case PM_TR_TEMPL_SUFFIX:
            text = do_suffix(text, fname, tpl_line);
            break;

        case PM_TR_TEMPL_SCHEME:
            text = do_scheme_expr(text, fname);
            break;

        case PM_TR_INVALID:
            pm_invalid_transition(fsm_state, fsm_tkn);
            switch (fsm_state) {
            case PM_ST_INIT:     BAD_MARKER(PSEUDO_MAC_BAD_NOSTART);
            case PM_ST_ST_MARK:  BAD_MARKER(PSEUDO_MAC_BAD_NOAG5);
            case PM_ST_AGEN:     BAD_MARKER(PSEUDO_MAC_BAD_NOTPL);
            case PM_ST_TEMPL:    BAD_MARKER(PSEUDO_MAC_BAD_NOEND);
            case PM_ST_END_MARK: BAD_MARKER(PSEUDO_MAC_BAD_NOEOL);
            default:             BAD_MARKER(PSEUDO_MAC_BAD_FSM);
            }

        case PM_TR_NOOP:
            break;

        default:
            BAD_MARKER(PSEUDO_MAC_BAD_PSEUDO);
        }

        fsm_state = nxt_state;
    }

    return text;

 abort_load:
    AG_ABEND(aprf(PSEUDO_MAC_ERR_FMT, fname, tpl_line, pzBadness));
#   undef BAD_MARKER
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
 * end of agen5/loadPseudo.c */
