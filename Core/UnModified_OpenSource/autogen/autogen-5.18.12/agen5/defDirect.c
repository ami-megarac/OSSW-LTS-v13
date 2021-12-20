
/**
 * @file defDirect.c
 *
 *  This module processes definition file directives.
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
#define NO_MATCH_ERR(_typ) \
AG_ABEND(aprf(DIRECT_NOMATCH_FMT, cctx->scx_fname, cctx->scx_line, _typ))

/**
 * "ifdef" processing level.  Blocks of text being skipped do not increment
 * the value.  Thus, transitioning from skip mode to process mode increments
 * it, and the reverse decrements it.
 */
static int  ifdef_lvl = 0;

/**
 * Set "true" when inside of a "#if" block making "#elif" directives
 * ignorable.  When "false", "#elif" triggers an error.
 */
static bool skip_if_block = false;

static char *
end_of_directive(char * scan)
{
    /*
     *  Search for the end of the #-directive.
     *  Replace "\\\n" sequences with "  ".
     */
    for (;;) {
        char * s = strchr(scan, NL);

        if (s == NULL)
            return scan + strlen(scan);
        
        cctx->scx_line++;

        if (s[-1] != '\\') {
            *(s++) = NUL;
            return s;
        }

        /*
         *  Replace the escape-newline pair with spaces and
         *  find the next end of line
         */
        s[-1] = s[0] = ' ';
        scan = s + 1;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  processDirective
 *
 *  THIS IS THE ONLY EXTERNAL ENTRY POINT
 *
 *  A directive character has been found.
 *  Decide what to do and return a pointer to the character
 *  where scanning is to resume.
 */
LOCAL char *
processDirective(char * scan)
{
    char * eodir = end_of_directive(scan);

    /*
     *  Ignore '#!' as a comment, enabling a definition file to behave
     *  as a script that gets interpreted by autogen.  :-)
     */
    if (*scan == '!')
        return eodir;

    scan = SPN_WHITESPACE_CHARS(scan);
    if (! IS_ALPHABETIC_CHAR(*scan))
        return doDir_invalid(DIR_INVALID, scan, eodir);

    return doDir_directive_disp(scan, eodir);
}

static int
count_lines(char const * start, char const * end)
{
    int res = 0;
    while (start < end) {
        char const * nxt = strchr(start, NL);
        if ((nxt == NULL) || (nxt > end))
            break;
        res++;
        start = nxt + 1;
    }
    return res;
}

static char *
next_directive(char * scan)
{
    if (*scan == '#')
        scan++;
    else {
        char * pz = strstr(scan, DIRECT_CK_LIST_MARK);
        if (pz == NULL)
            AG_ABEND(aprf(DIRECT_NOENDIF_FMT, cctx->scx_fname,
                          cctx->scx_line));

        scan = pz + 2;
    }

    return SPN_WHITESPACE_CHARS(scan);
}

/**
 *  Skip through the text to a matching "#endif".  We do this when we
 *  have processed the allowable text (found an "#else" after
 *  accepting the preceding text) or when encountering a "#if*def"
 *  while skipping a block of text due to a failed test.
 */
static char *
skip_to_endif(char * start)
{
    bool   skipping_if = skip_if_block;
    char * scan = start;
    char * dirv_end;

    for (;;) {
        scan = next_directive(scan);

        switch (find_directive(scan)) {
        case DIR_ENDIF:
        {
            /*
             *  We found the endif we are interested in
             */
            char * pz = strchr(scan, NL);
            if (pz != NULL)
                 dirv_end = pz+1;
            else dirv_end = scan + strlen(scan);
            goto leave_func;
        }

        case DIR_ELIF:
            if (! skip_if_block)
                AG_ABEND(ELIF_CONTEXT_MSG);
            break;

        case DIR_IF:
            skip_if_block = true;
            /* FALLTHROUGH */

        case DIR_IFDEF:
        case DIR_IFNDEF:
            /*
             *  We found a nested conditional, so skip to endif nested, too.
             */
            scan = skip_to_endif(scan);
            skip_if_block = skipping_if;
            break;

        default:
            /*
             *  We do not care what we found
             */
            break; /* ignore it */
        }  /* switch (find_directive(scan)) */
    }

 leave_func:
    cctx->scx_line += count_lines(start, dirv_end);
    return dirv_end;
}

/**
 *  Skip through the text to a "#endmac".
 */
static char *
skip_to_endmac(char * start)
{
    char * scan = start;
    char * res;

    for (;;) {
        scan = next_directive(scan);

        if (find_directive(scan) == DIR_ENDMAC) {
            /*
             *  We found the endmac we are interested in
             */
            char * pz = strchr(scan, NL);
            if (pz != NULL)
                 res = pz+1;
            else res = scan + strlen(scan);
            break;
        }
    }

    cctx->scx_line += count_lines(start, res);
    return res;
}


/**
 *  Skip through the text to a matching "#endif" or "#else" or
 *  "#elif*def".  We do this when we are skipping code due to a failed
 *  "#if*def" test.
 */
static char *
skip_to_else_end(char * start)
{
    char * scan = start;
    char * res;

    for (;;) {
        scan = next_directive(scan);

        switch (find_directive(scan)) {
        case DIR_ELSE:
            /*
             *  We found an "else" directive for an "ifdef"/"ifndef"
             *  that we were skipping over.  Start processing the text.
             *  Let the @code{DIR_ENDIF} advance the scanning pointer.
             */
            ifdef_lvl++;
            /* FALLTHROUGH */

        case DIR_ENDIF:
        {
            /*
             * We reached the end of the "ifdef"/"ifndef" (or transitioned to
             * process-the-text mode via "else").  Start processing the text.
             */
            char * pz = strchr(scan, NL);
            if (pz != NULL)
                 res = pz+1;
            else res = scan + strlen(scan);
            goto leave_func;
        }

        case DIR_IF:
            skip_if_block = true;
            scan = skip_to_endif(scan);
            skip_if_block = false;
            break;

        case DIR_IFDEF:
        case DIR_IFNDEF:
            scan = skip_to_endif(scan);
            break;

        default:
            /*
             *  We either don't know what it is or we do not care.
             */
            break;
        }  /* switch (find_directive(scan)) */
    }

 leave_func:
    cctx->scx_line += count_lines(start, res);
    return res;
}

static void
check_assert_str(char const * pz, char const * arg)
{
    pz = SPN_WHITESPACE_CHARS(pz);

    if (IS_FALSE_TYPE_CHAR(*pz))
        AG_ABEND(aprf(DIRECT_ASSERT_FMT, pz, arg));
}

static size_t
file_size(char const * fname)
{
    struct stat stbf;
    if (stat(fname, &stbf) != 0) {
        fswarn("stat", fname);
        return 0;
    }

    if (! S_ISREG(stbf.st_mode)) {
        fswarn("regular file check", fname);
        return 0;
    }

    if ((outfile_time < stbf.st_mtime) && ENABLED_OPT(SOURCE_TIME))
        outfile_time = stbf.st_mtime;

    if (maxfile_time < stbf.st_mtime)
        maxfile_time = stbf.st_mtime;

    return stbf.st_size;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  Special routines for each directive.  These routines are *ONLY*
 *  called from the table when the input is being processed.
 *  After this routine are either declarations or definitions of
 *  directive handling routines.  The documentation for these routines
 *  is extracted from this file.  See 'makedef.sh' for how it works.
 *  Each declared directive should have either a 'dummy:' section
 *  (if the directive is to be ignored) or a 'text:' section
 *  (if there is some form of implementation).  If the directive
 *  needs or may take arguments (e.g. '#define'), then there should
 *  also be an 'arg:' section describing the argument(s).
 */

/*!
 * an unrecognized directive has been encountered.  It need not be
 * #invalid, though that would get you here, too.
 */
char *
doDir_invalid(directive_enum_t id, char const * dir, char * scan_next)
{
    char z[32];

    strncpy(z, dir, 31);
    z[31] = NUL;
    fprintf(trace_fp, FIND_DIRECT_UNKNOWN, cctx->scx_fname,
            cctx->scx_line, z);

    (void)id;
    return scan_next;
}

/**
 * Ident, let and pragma directives are ignored.
 */
static char *
ignore_directive(directive_enum_t id, char const * arg, char * scan_next)
{
    (void)arg;
    (void)id;
    return scan_next;
}

/**
 * Ignored directive.
 */
char *
doDir_let(directive_enum_t id, char const * arg, char * scan_next)
{
    return ignore_directive(id, arg, scan_next);
}

/**
 * Ignored directive.
 */
char *
doDir_pragma(directive_enum_t id, char const * arg, char * scan_next)
{
    return ignore_directive(id, arg, scan_next);
}

/**
 * Ignored directive.
 */
char *
doDir_ident(directive_enum_t id, char const * arg, char * scan_next)
{
    return ignore_directive(id, arg, scan_next);
}

/**
 *  @code{#endshell}, @code{#elif} and @code{#endmac} directives, when found
 *  via a non-nested scan, are always in error.  They should only be found
 *  during the handling of @code{#shell}, @code{#if} and @code{#macdef}
 *  directives.  @code{#else} and @code{#endif} are in error if not matched
 *  with a previous @code{#ifdef} or @code{#ifndef} directive.
 */
static char *
bad_dirv_ctx(directive_enum_t id, char const * arg, char * scan_next)
{
    AG_ABEND(aprf(DIRECT_BAD_CTX_FMT, directive_name(id),
                  cctx->scx_fname, cctx->scx_line));
    (void)arg;
    (void)id;
    /* NOTREACHED */
    return scan_next;
}

/**
 * Marks the end of the #shell directive.  Error when out of context.
 */
char *
doDir_endshell(directive_enum_t id, char const * arg, char * scan_next)
{
    return bad_dirv_ctx(id, arg, scan_next);
}

/**
 * Marks a transition in the #if directive.  Error when out of context.
 * #if blocks are always ignored.
 */
char *
doDir_elif(directive_enum_t id, char const * arg, char * scan_next)
{
    return bad_dirv_ctx(id, arg, scan_next);
}

/**
 * Marks the end of the #macdef directive.  Error when out of context.
 */
char *
doDir_endmac(directive_enum_t id, char const * arg, char * scan_next)
{
    return bad_dirv_ctx(id, arg, scan_next);
}

/**
 *  This directive @i{is} processed, but only if the expression begins with
 *  either a back quote (@code{`}) or an open parenthesis (@code{(}).
 *  Text within the back quotes are handed off to the shell for processing
 *  and parenthesized text is handed off to Guile.  Multiple line expressions
 *  must be joined with backslashes.
 *
 *  If the @code{shell-script} or @code{scheme-expr} do not yield @code{true}
 *  valued results, autogen will be aborted.  If @code{<anything else>} or
 *  nothing at all is provided, then this directive is ignored.
 *
 *  The result is @code{false} (and fails) if the result is empty, the
 *  number zero, or a string that starts with the letters 'n' or 'f' ("no"
 *  or "false").
 */
char *
doDir_assert(directive_enum_t id, char const * dir, char * scan_next)
{
    (void)id;
    dir = SPN_WHITESPACE_CHARS(dir);
    switch (*dir) {
    case '`':
    {
        char * pzS = (char *)dir+1;
        char * pzR = SPN_WHITESPACE_BACK(pzS, NULL);

        if (*(--pzR) != '`')
            break; /* not a valid script */

        *pzR = NUL;
        pzS = shell_cmd((char const *)pzS);
        check_assert_str(pzS, dir);
        AGFREE(pzS);
        break;
    }

    case '(':
    {
        SCM res = ag_scm_c_eval_string_from_file_line(
            dir, cctx->scx_fname, cctx->scx_line);

        check_assert_str(scm2display(res), dir);
        break;
    }

    default:
        break;
    }

    return scan_next;
}

/**
 *  Will add the name to the define list as if it were a DEFINE program
 *  argument.  Its value will be the first non-whitespace token following
 *  the name.  Quotes are @strong{not} processed.
 *
 *  After the definitions file has been processed, any remaining entries
 *  in the define list will be added to the environment.
 */
char *
doDir_define(directive_enum_t id, char const * dir, char * scan_next)
{
    char * def_name = SPN_WHITESPACE_CHARS(dir);
    (void)id;

    /*
     *  Skip any #defines that do not look reasonable
     */
    if (! IS_VAR_FIRST_CHAR(*def_name))
        return scan_next;

    dir = SPN_VARIABLE_NAME_CHARS(def_name);

    /*
     *  IF this is a macro definition (rather than a value def),
     *  THEN we will ignore it.
     */
    if (*dir == '(')
        return scan_next;

    /*
     *  We have found the end of the name.
     *  IF there is no more data on the line,
     *  THEN we do not have space for the '=' required by PUTENV.
     *       Therefore, move the name back over the "#define"
     *       directive itself, giving us the space needed.
     */
    if (! IS_WHITESPACE_CHAR(*dir)) {
        char * pzS = (char *)dir;
        char * pzD = (def_name - 6);

        *pzS = NUL; // whatever it used to be, it is a NUL now.
        pzS = def_name;
        while ((*(pzD++) = *(pzS++)) != NUL)   ;
        pzD[-1] = '=';
        pzD[ 0] = NUL;
        def_name -= 6;

    } else {
        /*
         *  Otherwise, insert the '=' and move any data up against it.
         *  We only accept one name-type, space separated token.
         *  We are not ANSI-C.  ;-)
         */
        char * pz = (char *)(dir++);
        *pz++ = '=';
        dir = SPN_WHITESPACE_CHARS(dir);

        /*
         * Copy chars for so long as it is not NUL and does not require quoting
         */
        for (;;) {
            if ((*pz++ = *dir++) == NUL)
                break;
            if (! IS_UNQUOTABLE_CHAR(*dir)) {
                *pz = NUL;
                break;
            }
        }
    }

    SET_OPT_DEFINE(def_name);
    return scan_next;
}

/**
 *  This must follow an @code{#if}, @code{#ifdef} or @code{#ifndef}.
 *  If it follows the @code{#if}, then it will be ignored.  Otherwise,
 *  it will change the processing state to the reverse of what it was.
 */
char *
doDir_else(directive_enum_t id, char const * arg, char * scan_next)
{
    if (--ifdef_lvl < 0)
        return bad_dirv_ctx(id, arg, scan_next);

    return skip_to_endif(scan_next);
}

/**
 *  This must follow an @code{#if}, @code{#ifdef} or @code{#ifndef}.
 *  In all cases, this will resume normal processing of text.
 */
char *
doDir_endif(directive_enum_t id, char const * arg, char * scan_next)
{
    if (--ifdef_lvl < 0)
        return bad_dirv_ctx(id, arg, scan_next);

    return scan_next;
}

/**
 *  This directive will cause AutoGen to stop processing
 *  and exit with a status of EXIT_FAILURE.
 */
char *
doDir_error(directive_enum_t id, char const * arg, char * scan_next)
{
    arg = SPN_WHITESPACE_CHARS(arg);
    AG_ABEND(aprf(DIRECT_ERROR_FMT, cctx->scx_fname, cctx->scx_line, arg));
    /* NOTREACHED */
    (void)scan_next;
    (void)id;
    return NULL;
}

/**
 *  @code{#if} expressions are not analyzed.  @strong{Everything} from here
 *  to the matching @code{#endif} is skipped.
 */
char *
doDir_if(directive_enum_t id, char const * arg, char * scan_next)
{
    skip_if_block = true;
    scan_next = skip_to_endif(scan_next);
    skip_if_block = false;
    (void)arg;
    (void)id;
    return scan_next;
}

/**
 *  The definitions that follow, up to the matching @code{#endif} will be
 *  processed only if there is a corresponding @code{-Dname} command line
 *  option or if a @code{#define} of that name has been previously encountered.
 */
char *
doDir_ifdef(directive_enum_t id, char const * dir, char * scan_next)
{
    char const * defstr = get_define_str(SPN_WHITESPACE_CHARS(dir), false);
    bool ok = (id == DIR_IFDEF) ? (defstr != NULL) : (defstr == NULL);
    if (! ok)
        return skip_to_else_end(scan_next);

    ifdef_lvl++;
    return scan_next;
}


/**
 *  The definitions that follow, up to the matching @code{#endif} will be
 *  processed only if the named value has @strong{not} been defined.
 */
char *
doDir_ifndef(directive_enum_t id, char const * dir, char * scan_next)
{
    return doDir_ifdef(id, dir, scan_next);
}

/**
 *  This directive will insert definitions from another file into
 *  the current collection.  If the file name is adorned with
 *  double quotes or angle brackets (as in a C program), then the
 *  include is ignored.
 */
char *
doDir_include(directive_enum_t id, char const * dir, char * scan_next)
{
    static char const * const apzSfx[] = { DIRECT_INC_DEF_SFX, NULL };
    scan_ctx_t * new_ctx;
    size_t     inc_sz;
    char       full_name[ AG_PATH_MAX + 1 ];
    (void)id;

    dir = SPN_WHITESPACE_CHARS(dir);
    /*
     *  Ignore C-style includes.  This allows "C" files to be processed
     *  for their "#define"s.
     */
    if ((*dir == '"') || (*dir == '<'))
        return scan_next;

    if (! SUCCESSFUL(
            find_file(dir, full_name, apzSfx, cctx->scx_fname))) {
        errno = ENOENT;
        fswarn("search for", cctx->scx_fname);
        return scan_next;
    }

    /*
     *  Make sure the specified file is a regular file and we can get
     *  the correct size for it.
     */
    inc_sz = file_size(full_name);
    if (inc_sz == 0)
        return scan_next;

    /*
     *  Get the space for the output data and for context overhead.
     *  This is an extra allocation and copy, but easier than rewriting
     *  'loadData()' for this special context.
     */
    {
        size_t sz = sizeof(scan_ctx_t) + 4 + inc_sz;
        new_ctx = (scan_ctx_t *)AGALOC(sz, "inc def head");

        memset(VOIDP(new_ctx), 0, sz);
        new_ctx->scx_line = 1;
    }

    /*
     *  Link it into the context stack
     */
    cctx->scx_scan     = scan_next;
    new_ctx->scx_next  = cctx;
    cctx               = new_ctx;
    AGDUPSTR(new_ctx->scx_fname, full_name, "def file");

    new_ctx->scx_scan  =
    new_ctx->scx_data  =
    scan_next          = (char *)(new_ctx + 1);

    /*
     *  Read all the data.  Usually in a single read, but loop
     *  in case multiple passes are required.
     */
    {
        FILE * fp = fopen(full_name, "r" FOPEN_TEXT_FLAG);
        char * pz = scan_next;

        if (fp == NULL)
            AG_CANT(DIRECT_INC_CANNOT_OPEN, full_name);

        if (dep_fp != NULL)
            add_source_file(full_name);

        do  {
            size_t rdct = fread(VOIDP(pz), (size_t)1, inc_sz, fp);

            if (rdct == 0)
                AG_CANT(DIRECT_INC_CANNOT_READ, full_name);

            pz += rdct;
            inc_sz -= rdct;
        } while (inc_sz > 0);

        fclose(fp);
        *pz = NUL;
    }

    return scan_next;
}

/**
 *  Alters the current line number and/or file name.  You may wish to
 *  use this directive if you extract definition source from other files.
 *  @command{getdefs} uses this mechanism so AutoGen will report the correct
 *  file and approximate line number of any errors found in extracted
 *  definitions.
 */
char *
doDir_line(directive_enum_t id, char const * dir, char * scan_next)
{
    (void)id;
    /*
     *  The sequence must be:  #line <number> "file-name-string"
     *
     *  Start by scanning up to and extracting the line number.
     */
    dir = SPN_WHITESPACE_CHARS(dir);
    if (! IS_DEC_DIGIT_CHAR(*dir))
        return scan_next;

    cctx->scx_line = (int)strtol(dir, (char **)&dir, 0);

    /*
     *  Now extract the quoted file name string.
     *  We dup the string so it won't disappear on us.
     */
    dir = SPN_WHITESPACE_CHARS(dir);
    if (*(dir++) != '"')
        return scan_next;
    {
        char * pz = strchr(dir, '"');
        if (pz == NULL)
            return scan_next;
        *pz = NUL;
    }

    AGDUPSTR(cctx->scx_fname, dir, "#line");

    return scan_next;
}

/**
 *  This is a new AT&T research preprocessing directive.  Basically, it is
 *  a multi-line #define that may include other preprocessing directives.
 *  Text between this line and a #endmac directive are ignored.
 */
char *
doDir_macdef(directive_enum_t id, char const * arg, char * scan_next)
{
    (void)arg;
    (void)id;
    return skip_to_endmac(scan_next);
}

/**
 *  This directive will pass the option name and associated text to the
 *  AutoOpts optionLoadLine routine (@pxref{libopts-optionLoadLine}).  The
 *  option text may span multiple lines by continuing them with a backslash.
 *  The backslash/newline pair will be replaced with two space characters.
 *  This directive may be used to set a search path for locating template files
 *  For example, this:
 *
 *  @example
 *    #option templ-dirs $ENVVAR/dirname
 *  @end example
 *  @noindent
 *  will direct autogen to use the @code{ENVVAR} environment variable to find
 *  a directory named @code{dirname} that (may) contain templates.  Since these
 *  directories are searched in most recently supplied first order, search
 *  directories supplied in this way will be searched before any supplied on
 *  the command line.
 */
char *
doDir_option(directive_enum_t id, char const * dir, char * scan_next)
{
    dir = SPN_WHITESPACE_CHARS(dir);
    optionLoadLine(&autogenOptions, dir);
    (void)id;
    return scan_next;
}

/**
 *  Invokes @code{$SHELL} or @file{/bin/sh} on a script that should
 *  generate AutoGen definitions.  It does this using the same server
 *  process that handles the back-quoted @code{`} text.
 *  The block of text handed to the shell is terminated with
 *  the #endshell directive.
 *
 *  @strong{CAUTION}@:  let not your @code{$SHELL} be @code{csh}.
 */
char *
doDir_shell(directive_enum_t id, char const * arg, char * scan_next)
{
    static size_t const endshell_len = sizeof("\n#endshell") - 1;

    scan_ctx_t * pCtx;
    char *       pzText = scan_next;

    (void)arg;
    (void)id;

    /*
     *  The output time will always be the current time.
     *  The dynamic content is always current :)
     */
    maxfile_time = outfile_time = time(NULL);

    /*
     *  IF there are no data after the '#shell' directive,
     *  THEN we won't write any data
     *  ELSE we have to find the end of the data.
     */
    if (strncmp(pzText, DIRECT_SHELL_END_SHELL+1, endshell_len-1) == 0)
        return scan_next;

    {
        char * pz = strstr(scan_next, DIRECT_SHELL_END_SHELL);
        if (pz == NULL)
            AG_ABEND(aprf(DIRECT_SHELL_NOEND, cctx->scx_fname,
                          cctx->scx_line));

        while (scan_next < pz) {
            if (*(scan_next++) == NL) cctx->scx_line++;
        }

        *scan_next = NUL;
    }

    /*
     *  Advance the scan pointer to the next line after '#endshell'
     *  IF there is no such line,
     *  THEN the scan will resume on a zero-length string.
     */
    scan_next = strchr(scan_next + endshell_len, NL);
    if (scan_next == NULL)
        scan_next = VOIDP(zNil);

    /*
     *  Save the scan pointer into the current context
     */
    cctx->scx_scan  = scan_next;

    /*
     *  Run the shell command.  The output text becomes the
     *  "file text" that is used for more definitions.
     */
    pzText = shell_cmd(pzText);
    if (pzText == NULL)
        return scan_next;

    if (*pzText == NUL) {
        AGFREE(pzText);
        return scan_next;
    }

    /*
     *  Get the space for the output data and for context overhead.
     *  This is an extra allocation and copy, but easier than rewriting
     *  'loadData()' for this special context.
     */
    pCtx = (scan_ctx_t *)AGALOC(sizeof(scan_ctx_t) + strlen(pzText) + 4,
                             "shell output");

    /*
     *  Link the new scan data into the context stack
     */
    pCtx->scx_next = cctx;
    cctx           = pCtx;

    /*
     *  Set up the rest of the context structure
     */
    AGDUPSTR(pCtx->scx_fname, DIRECT_SHELL_COMP_DEFS, DIRECT_SHELL_COMP_DEFS);
    pCtx->scx_scan  =
    pCtx->scx_data  = (char *)(pCtx + 1);
    pCtx->scx_line  = 0;
    strcpy(pCtx->scx_scan, pzText);
    AGFREE(pzText);

    return pCtx->scx_scan;
}

/**
 *  Will remove any entries from the define list
 *  that match the undef name pattern.
 */
char *
doDir_undef(directive_enum_t id, char const * dir, char * scan_next)
{
    dir = SPN_WHITESPACE_CHARS(dir);
    SET_OPT_UNDEFINE(dir);
    (void)id;
    return scan_next;
}

/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/defDirect.c */
