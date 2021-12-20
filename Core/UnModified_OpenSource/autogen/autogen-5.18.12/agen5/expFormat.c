
/**
 * @file expFormat.c
 *
 *  This module implements formatting expression functions.
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

typedef enum {
    LSEG_INFO   = 1,
    LSEG_DESC   = 2,
    LSEG_FULL   = 3,
    LSEG_NAME   = 4
} lic_segment_e_t;

/**
 * Remove horizontal white space at the ends of lines.
 * "dne" and licensing text passes through this before
 * making an SCM out of the result.
 *
 * @param[in,out] text  the text to work on.
 */
static void
trim_trailing_white(char * text)
{
    char * start = text++;
    if (*start == NUL)
        return;

    for (;;) {
        switch (*text++) {
        case NUL:
            return;

        case NL:
            if (IS_HORIZ_WHITE_CHAR(text[-2]))
                goto doit;
        default:
            break;
        }
    }

 doit:
    start = SPN_HORIZ_WHITE_BACK(start, text - 2);
    *(start++) = NL;

    char * dest = start;

    for (;;) {
        switch (*(dest++) = *(text++)) {
        case NUL:
            return;

        case NL:
            if (IS_HORIZ_WHITE_CHAR(dest[-2])) {
                dest  = SPN_HORIZ_WHITE_BACK(start, dest - 2);
                start = dest;
                *(dest++) = NL;
            }

        default:
            break;
        }
    }
}

/*=gfunc dne
 *
 * what:  '"Do Not Edit" warning'
 *
 * exparg: prefix,       string for starting each output line
 * exparg: first_prefix, for the first output line, opt
 * exparg: optpfx,       shifted prefix, opt
 *
 * doc:
 *  Generate a "DO NOT EDIT" or "EDIT WITH CARE" warning string.
 *  Which depends on whether or not the @code{--writable} command line
 *  option was set.
 *
 *  The first argument may be an option: @samp{-D} or @samp{-d}, causing the
 *  second and (potentially) third arguments to be interpreted as the first
 *  and second arguments.  The only useful option is @samp{-D}:
 *
 *  @table @samp
 *  @item -D
 *    will add date, timestamp and version information.
 *  @item -d
 *    is ignored, but still accepted for compatibility with older versions
 *    of the "dne" function where emitting the date was the default.
 *  @end table
 *
 *  If one of these options is specified, then the "prefix" and "first"
 *  arguments are obtained from the following arguments.  The presence (or
 *  absence) of this option can be overridden with the environment variable,
 *  @samp{AUTOGEN_DNE_DATE}.  The date is disabled if the value is empty or
 *  starts with one of the characters, @samp{0nNfF} -- zero or the first
 *  letter of "no" or "false".
 *
 *  The @code{prefix} argument is a per-line string prefix.  The optional
 *  second argument is a prefix for the first line only and, in read-only
 *  mode, activates editor hints.
 *
 *  @example
 *  -*- buffer-read-only: t -*- vi: set ro:
 *  @end example
 *
 *  @noindent
 *  The warning string also includes information about the template used
 *  to construct the file and the definitions used in its instantiation.
=*/
SCM
ag_scm_dne(SCM prefix, SCM first, SCM opt)
{
    char const *  date_str;
    char const *  pzFirst;
    char const *  pzPrefix;

    if (! scm_is_string(prefix))
        return SCM_UNDEFINED;

    date_str = zNil;
    pzFirst  = zNil;

    {
        size_t pfxLen   = scm_c_string_length(prefix);
        pzPrefix = ag_scm2zchars(prefix, "dne-prefix");

        /*
         * Check for a -d option (ignored) or a -D option (emit date)
         * by default, "dne" will not emit a date in the output.
         */
        if ((pfxLen == 2) && (*pzPrefix == '-')) {
            switch (pzPrefix[1]) {
            case 'D':
                date_str = NULL;
                pzPrefix = ag_scm2zchars(first, "dne-prefix");
                first    = opt;
                break;

            case 'd':
                pzPrefix = ag_scm2zchars(first, "dne-prefix");
                first    = opt;
                break;
            }
        }
    }

    do  {
        char const * pz = getenv("AUTOGEN_DNE_DATE");
        if (pz == NULL) break; /* use selection from template */

        switch (*pz) {
        case NUL:
        case '0': /* zero */
        case 'n':
        case 'N': /* no */
        case 'f':
        case 'F': /* false */
            date_str = zNil; /* template user says "no DNE date" */
            break;

        default:
            date_str = NULL; /* template user says "INCLUDE DNE date" */
        }
    } while (0);

    /*
     *  IF we also have a 'first' prefix string,
     *  THEN we set it to something other than ``zNil'' and deallocate later.
     */
    if (scm_is_string(first))
        pzFirst = aprf(ENABLED_OPT(WRITABLE) ? "%s\n" : EXP_FMT_DNE1,
                       ag_scm2zchars(first, "pfx-1"), pzPrefix);

    if (date_str == NULL) {
        static char const tim_fmt[] =
            "  %B %e, %Y at %r by AutoGen " AUTOGEN_VERSION;

        size_t const  tsiz = sizeof(tim_fmt) + sizeof("september") * 2;
        time_t     curTime = time(NULL);
        struct tm *  pTime = localtime(&curTime);

        date_str = scribble_get((ssize_t)tsiz);
        strftime((char *)date_str, tsiz, tim_fmt, pTime);
    }

    {
        char const  * pz;
        out_stack_t * pfp = cur_fpstack;
        char const  * tpl_name = strrchr(tpl_fname, DIRCH);
        if (tpl_name == NULL)
            tpl_name = tpl_fname;
        else
            tpl_name++;

        while (pfp->stk_flags & FPF_UNLINK)  pfp = pfp->stk_prev;
        if (! ENABLED_OPT(DEFINITIONS))
            pz = "<<no definitions>>";

        else if (*oops_pfx != NUL)
            pz = "<<CGI-definitions>>";

        else {
            pz = OPT_ARG(DEFINITIONS);
            if (strcmp(pz, "-") == 0)
                pz = "stdin";
        }

        pz = aprf(ENABLED_OPT(WRITABLE) ? EXP_FMT_DNE2 : EXP_FMT_DNE,
                  pzPrefix, pfp->stk_fname, date_str,
                  pz, tpl_name, pzFirst);
        if (pz == NULL)
            AG_ABEND("Allocating Do-Not-Edit string");
        trim_trailing_white(C(char *, pz));
        date_str = pz;
    }

    /*
     *  Deallocate any temporary buffers.  pzFirst either points to
     *  the zNil string, or to an allocated buffer.
     */
    if (pzFirst != zNil)
        AGFREE(pzFirst);
    {
        SCM res = scm_from_latin1_string(date_str);
        AGFREE(date_str);

        return res;
    }
}


/*=gfunc warn
 *
 * what:  display warning message and continue
 *
 * exparg: @ message @ message to display @@
 * doc:
 *
 *  The argument is a string that printed out to stderr.
 *  The message is formed from the formatting string:
 *
 *  @example
 *  @code{WARNING:}  %s\n
 *  @end example
 *
 *  The template processing resumes after printing the message.
=*/
SCM
ag_scm_warn(SCM res)
{
    char const * msg = ag_scm2zchars(res, "warn str");
    if ((msg == NULL) || (*msg == NUL))
        AG_ABEND("warn called without a message string");
    fprintf(stderr, WARN_FMT, msg);
    return SCM_UNDEFINED;
}


/*=gfunc error
 *
 * what:  display message and exit
 *
 * exparg: @ message @ message to display before exiting @@
 * doc:
 *
 *  The argument is a string that printed out as part of an error
 *  message.  The message is formed from the formatting string:
 *
 *  @example
 *  DEFINITIONS ERROR in %s line %d for %s:  %s\n
 *  @end example
 *
 *  The first three arguments to this format are provided by the
 *  routine and are:  The name of the template file, the line within
 *  the template where the error was found, and the current output
 *  file name.
 *
 *  After displaying the message, the current output file is removed
 *  and autogen exits with the EXIT_FAILURE error code.  IF, however,
 *  the argument begins with the number 0 (zero), or the string is the
 *  empty string, then processing continues with the next suffix.
=*/
SCM
ag_scm_error(SCM res)
{
    char const *  msg;
    tSuccess      abrt = FAILURE;
    char          num_bf[16];
    size_t        msg_ln;

    switch (ag_scm_type_e(res)) {
    case GH_TYPE_BOOLEAN:
        if (scm_is_false(res))
            abrt = PROBLEM;
        msg = zNil;
        break;

    case GH_TYPE_NUMBER:
    {
        unsigned long val = AG_SCM_TO_ULONG(res);
        if (val == 0)
            abrt = PROBLEM;
        snprintf(num_bf, sizeof(num_bf), "%d", (int)val);
        msg = num_bf;
        break;
    }

    case GH_TYPE_CHAR:
        num_bf[0] = (char)SCM_CHAR(res);
        if ((num_bf[0] == NUL) || (num_bf[0] == '0'))
            abrt = PROBLEM;
        num_bf[1] = NUL;
        msg = num_bf;
        break;

    case GH_TYPE_STRING:
        msg  = ag_scm2zchars(res, "error string");
        msg  = SPN_WHITESPACE_CHARS(msg);
        msg_ln = strlen(msg);

        /*
         *  IF the message starts with the number zero,
         *    OR the message is the empty string,
         *  THEN this is just a warning that is ignored
         */
        if (msg_ln == 0)
            abrt = PROBLEM;
        else if (IS_DEC_DIGIT_CHAR(*msg) && (strtol(msg, NULL, 0) == 0))
            abrt = PROBLEM;
        break;

    default:
        msg = BAD_MSG_STR;
    }

    /*
     *  IF there is a message,
     *  THEN print it.
     */
    if (*msg != NUL) {
        char const * typ = (abrt != PROBLEM) ? ERROR_STR : WARN_STR;
        char * pz = aprf(DEF_NOTE_FMT, typ,
                         current_tpl->td_file, cur_macro->md_line,
                         cur_fpstack->stk_fname, msg);
        if (abrt != PROBLEM)
            AG_ABEND(pz);
        fputs(pz, trace_fp);
        AGFREE(pz);
    }

    longjmp(abort_jmp_buf, abrt);
    /* NOTREACHED */
    return SCM_UNDEFINED;
}

/**
 * Assemble the copyright preamble and long license description.
 *
 * @param txt a pointer to the first of two newlines separating
 *            copyright information from the description.
 */
static void
assemble_full_desc(char * txt, char const * pfx)
{
    char * pd;
    char * md;

    size_t prefix_len = strlen(pfx) + 1;
    while (  (prefix_len > 0)
          && IS_WHITESPACE_CHAR(pfx[prefix_len - 2]))
        prefix_len--;

    /*
     *  Preserve the first newline.  Set the move destination
     *  out past where we will be inserting the "<PFX>\n" marker.
     */
    pd = txt + 1;          /* prefix destination */
    md = pd  + prefix_len; /* move destination */

    while (*txt == NL) txt++;
    /*
     *  Maybe there were exactly enough NL characters we don't need to move
     */
    if (md != txt)
        memmove(md, txt, strlen(txt) + 1);
    memmove(pd, pfx, --prefix_len);
    pd[prefix_len] = NL;

    /*
     *  Look for a trailing license name and trim it and trailing white space
     */
    txt = strstr(md, "\n\n");
    if (txt == NULL)
        txt = md + strlen(md);

    while (  (txt > md)
          && IS_WHITESPACE_CHAR(txt[-1]))  txt--;
    *txt = NUL;
}

/**
 * Trim off the license name.  It is the third double-newline stanza
 * in the license file.
 *
 * @param p  a pointer to the first of two newlines separating
 *            copyright information from the description.
 * @return pointer to second stanza, sans the license name trailer.
 */
static char *
trim_lic_name(char * p)
{
    char * res;
    /* skip the leading white space.  It starts with NL. */
    p = SPN_WHITESPACE_CHARS(p + 1);
    if (*p == NUL)
        return p;

    res = p;

    /*
     *  The last section ends with two consecutive new lines.
     *  All trailing newlines are trimmed (not all white space).
     */
    p = strstr(p, "\n\n");
    if (p == NULL)
        p = res + strlen(res);
    while (  (p > res)
          && IS_WHITESPACE_CHAR(p[-1]))  p--;
    *p = NUL;

    return res;
}

/**
 * Extract the license name.  It is the third double-newline stanza
 * in the license file.
 *
 * @param txt a pointer to the first of two newlines separating
 *            copyright information from the description.
 * @return pointer to the license name trailer.
 */
static char *
get_lic_name(char * p)
{
    char * scan = p;
    while (*(++scan) == NL)   ; /* skip the leading NL's. */

    /*
     * Find the third stanza.  If there.  If not, we supply some static
     * text:  "an unknown license"
     */
    scan = strstr(scan, "\n\n");
    if (scan == NULL) {
        strcpy(p, EXP_FMT_BAD_LIC);
        return p;
    }
    while (*scan == NL) scan++;
    return scan;
}

/**
 * Find the kind of text being requested.  It may be "full" (the first
 * two stanzas), "info" (the first -- copyright info + license name),
 * "description" (the second -- a one paragraph description), or
 * "name" -- the third stanza.
 *
 * @param txt a pointer to the first of two newlines separating
 *            copyright information from the description.
 * @return pointer to the requested text.
 */
static char *
find_lic_text(
    lic_segment_e_t segment, SCM lic, ssize_t * txt_len, char const * pfx)
{
    static char const * const lic_sfx[] = { FIND_LIC_TEXT_SFX, NULL };

    char const * lic_pz = ag_scm2zchars(lic, "license");
    char    fname[ AG_PATH_MAX ];
    char *  ftext;
    ssize_t flen;

    /*
     * auto-convert "bsd" into "mbsd" for compatibility.
     */
    if (strcmp(lic_pz, FIND_LIC_TEXT_MBSD+1) == 0)
        lic_pz = FIND_LIC_TEXT_MBSD;

    if (! SUCCESSFUL(find_file(lic_pz, fname, lic_sfx, NULL)))
        return NULL;

    {
        struct stat stbf;
        if (stat(fname, &stbf) != 0)
            AG_CANT(FIND_LIC_TEXT_NO_LIC, fname);
        if (! S_ISREG(stbf.st_mode)) {
            errno = EINVAL;
            AG_CANT(FIND_LIC_TEXT_BAD_FILE, fname);
        }
        flen = stbf.st_size;
    }

    ftext    = scribble_get(flen + EXP_FMT_BAD_LIC_LEN + 1);
    *txt_len = flen;

    {
        FILE * fp = fopen(fname, "r");

        if (fp == NULL)
            AG_CANT(FIND_LIC_TEXT_OPEN, fname);

        if (fread(ftext, 1, (size_t)flen, fp) != (size_t)flen)
            AG_CANT(FIND_LIC_TEXT_BAD_FILE, fname);

        ftext[flen] = NUL;
        fclose(fp);
    }

    if (dep_fp != NULL)
        add_source_file(fname);

    {
        char * p = strstr(ftext, DOUBLE_NEWLINE);

        if (p == NULL)
            AG_ABEND(aprf(FIND_LIC_TEXT_INVAL, fname));

        switch (segment) {
        case LSEG_INFO: p[1]  = NUL;                break;
        case LSEG_DESC: ftext = trim_lic_name(p);   break;
        case LSEG_NAME: ftext = get_lic_name(p);    break;
        case LSEG_FULL: assemble_full_desc(p, pfx); break;
        }
    }

    return ftext;
}

/**
 * Construct an SCM for the kind of text being requested.
 *
 * It may be "full" (the first two stanzas), "info" (the first -- copyright
 * info + license name), "description" (the second -- a one paragraph
 * description), or "name" -- the third stanza.
 *
 * @param seg    which segment of license is desired
 * @param lic    The name of the license
 * @param prog   the name of the program
 * @param pfx    a per-line prefix
 * @param owner  who owns the copyright
 * @param years  the copyright years
 *
 * @return the SCM-ized string
 */
static SCM
construct_license(
    lic_segment_e_t seg, SCM lic, SCM prog, SCM pfx, SCM owner, SCM years)
{
    static SCM subs  = SCM_UNDEFINED;
    static SCM empty = SCM_UNDEFINED;

    SCM     vals = SCM_UNDEFINED;
    char *  lic_text;
    ssize_t text_len;
    char const * pfx_pz = ag_scm2zchars(pfx, "lic-prefix");

    if (subs == SCM_UNDEFINED) {
        static char const * const slst[] = {
            MK_LIC_PROG, MK_LIC_PFX, MK_LIC_OWN, MK_LIC_YRS
        };
        subs = scm_gc_protect_object(
            scm_list_4(scm_from_latin1_string(slst[0]),
                       scm_from_latin1_string(slst[1]),
                       scm_from_latin1_string(slst[2]),
                       scm_from_latin1_string(slst[3])));

        empty = scm_gc_protect_object(scm_from_latin1_string(""));
    }

    if (! scm_is_string(lic))
        AG_ABEND(MK_LIC_NOT_STR);

    lic_text = find_lic_text(seg, lic, &text_len, pfx_pz);
    if (lic_text == NULL)
        AG_ABEND(aprf(MK_LIC_NO_LIC, ag_scm2zchars(lic, "lic")));

    if (! scm_is_string(owner))   owner = empty;
    if (! scm_is_string(years))   years = empty;
    vals = scm_list_4(prog, pfx, owner, years);

    do_multi_subs(&lic_text, &text_len, subs, vals);

    trim_trailing_white(lic_text);
    return scm_from_latin1_string(lic_text);
}

/*=gfunc license_full
 *
 * what:  Emit the licensing information and description
 * general_use:
 *
 * exparg: license,   name of license type
 * exparg: prog-name, name of the program under the GPL
 * exparg: prefix,    String for starting each output line
 * exparg: owner,     owner of the program, optional
 * exparg: years,     copyright years, optional
 *
 * doc:
 *
 *  Emit all the text that @code{license-info} and @code{license-description}
 *  would emit (@pxref{SCM license-info, @code{license-info}},
 *  and @pxref{SCM license-description, @code{license-description}}),
 *  with all the same substitutions.
 *
 *  All of these depend upon the existence of a license file named after the
 *  @code{license} argument with a @code{.lic} suffix.  That file should
 *  contain three blocks of text, each separated by two or more consecutive
 *  newline characters (at least one completely blank line).
 *
 *  The first section describes copyright attribution and the name of the usage
 *  licence.  For GNU software, this should be the text that is to be displayed
 *  with the program version.  Four text markers can be replaced: <PFX>,
 *  <program>, <years> and <owner>.
 *
 *  The second section is a short description of the terms of the license.
 *  This is typically the kind of text that gets displayed in the header of
 *  source files.  Only the <PFX>, <owner> and <program> markers are
 *  substituted.
 *
 *  The third section is strictly the name of the license.
 *  No marker substitutions are performed.
 *
 *  @example
 *  <PFX>Copyright (C) <years> <owner>, all rights reserved.
 *  <PFX>
 *  <PFX>This is free software. It is licensed for use,
 *  <PFX>modification and redistribution under the terms
 *  <PFX>of the GNU General Public License, version 3 or later
 *  <PFX>    <http://gnu.org/licenses/gpl.html>
 *
 *  <PFX><program> is free software: you can redistribute it
 *  <PFX>and/or modify it under the terms of the GNU General
 *  <PFX>Public License as published by the Free Software ...
 *
 *  the GNU General Public License, version 3 or later
 *  @end example
=*/
SCM
ag_scm_license_full(SCM lic, SCM prog, SCM pfx, SCM owner, SCM years)
{
    return construct_license(LSEG_FULL, lic, prog, pfx, owner, years);
}

/*=gfunc license_description
 *
 * what:  Emit a license description
 * general_use:
 *
 * exparg: license,   name of license type
 * exparg: prog-name, name of the program under the GPL
 * exparg: prefix,    String for starting each output line
 * exparg: owner,     owner of the program, optional
 *
 * doc:
 *
 *  Emit a string that contains a detailed license description, with
 *  substitutions for program name, copyright holder and a per-line prefix.
 *  This is the text typically used as part of a source file header.
 *  For more details, @xref{SCM license-full, the license-full command}.
 *
=*/
SCM
ag_scm_license_description(SCM lic, SCM prog, SCM pfx, SCM owner)
{
    return construct_license(LSEG_DESC, lic, prog, pfx, owner, SCM_UNDEFINED);
}

/*=gfunc license_info
 *
 * what:  Emit the licensing information and copyright years
 * general_use:
 *
 * exparg: license,   name of license type
 * exparg: prog-name, name of the program under the GPL
 * exparg: prefix,    String for starting each output line
 * exparg: owner,     owner of the program, optional
 * exparg: years,     copyright years, optional
 *
 * doc:
 *
 *  Emit a string that contains the licensing description, with some
 *  substitutions for program name, copyright holder, a list of years when the
 *  source was modified, and a per-line prefix.  This text typically includes a
 *  brief license description and is often printed out when a program starts
 *  running or as part of the @code{--version} output.
 *  For more details, @xref{SCM license-full, the license-full command}.
 *
=*/
SCM
ag_scm_license_info(SCM lic, SCM prog, SCM pfx, SCM owner, SCM years)
{
    return construct_license(LSEG_INFO, lic, prog, pfx, owner, years);
}

/*=gfunc license_name
 *
 * what:  Emit the name of the license
 * general_use:
 *
 * exparg: license,   name of license type
 *
 * doc:
 *
 *  Emit a string that contains the full name of the license.
=*/
SCM
ag_scm_license_name(SCM lic)
{
    ssize_t text_len;
    char * txt = find_lic_text(LSEG_NAME, lic, &text_len, "");
    char * e;

    if (txt != NULL) {
        txt = SPN_WHITESPACE_CHARS(txt);
        e   = SPN_WHITESPACE_BACK(txt, txt);
        *e  = NUL;
        lic = scm_from_latin1_string(txt);
    }
    return lic;
}

/*=gfunc gpl
 *
 * what:  GNU General Public License
 * general_use:
 *
 * exparg: prog-name, name of the program under the GPL
 * exparg: prefix, String for starting each output line
 *
 * doc:
 *
 *  Emit a string that contains the GNU General Public License.
 *  This function is now deprecated.  Please @xref{SCM license-description}.
=*/
SCM
ag_scm_gpl(SCM prog_name, SCM prefix)
{
    static SCM lic = SCM_UNDEFINED;

    if (lic == SCM_UNDEFINED)
        lic = scm_gc_protect_object(
            scm_from_latin1_string(FIND_LIC_TEXT_LGPL+1));
    return ag_scm_license_description(lic, prog_name, prefix, SCM_UNDEFINED);
}

/*=gfunc agpl
 *
 * what:  GNU Affero General Public License
 * general_use:
 *
 * exparg: prog-name, name of the program under the GPL
 * exparg: prefix, String for starting each output line
 *
 * doc:
 *
 *  Emit a string that contains the GNU Affero General Public License.
 *  This function is now deprecated.  Please @xref{SCM license-description}.
=*/
SCM
ag_scm_agpl(SCM prog_name, SCM prefix)
{
    static SCM lic = SCM_UNDEFINED;

    if (lic == SCM_UNDEFINED)
        lic = scm_gc_protect_object(
            scm_from_latin1_string(FIND_LIC_TEXT_AGPL));
    return ag_scm_license_description(lic, prog_name, prefix, SCM_UNDEFINED);
}

/*=gfunc lgpl
 *
 * what:  GNU Library General Public License
 * general_use:
 *
 * exparg: prog_name, name of the program under the LGPL
 * exparg: owner, Grantor of the LGPL
 * exparg: prefix, String for starting each output line
 *
 * doc:
 *
 *  Emit a string that contains the GNU Library General Public License.
 *  This function is now deprecated.  Please @xref{SCM license-description}.
=*/
SCM
ag_scm_lgpl(SCM prog_name, SCM owner, SCM prefix)
{
    static SCM lic = SCM_UNDEFINED;

    if (lic == SCM_UNDEFINED)
        lic = scm_gc_protect_object(
            scm_from_latin1_string(FIND_LIC_TEXT_LGPL));
    return ag_scm_license_description(lic, prog_name, prefix, owner);
}

/*=gfunc bsd
 *
 * what:  BSD Public License
 * general_use:
 *
 * exparg: prog_name, name of the program under the BSD
 * exparg: owner, Grantor of the BSD License
 * exparg: prefix, String for starting each output line
 *
 * doc:
 *
 *  Emit a string that contains the Free BSD Public License.
 *  This function is now deprecated.  Please @xref{SCM license-description}.
 *
=*/
SCM
ag_scm_bsd(SCM prog_name, SCM owner, SCM prefix)
{
    static SCM lic = SCM_UNDEFINED;

    if (lic == SCM_UNDEFINED)
        lic = scm_gc_protect_object(
            scm_from_latin1_string(FIND_LIC_TEXT_MBSD));
    return ag_scm_license_description(lic, prog_name, prefix, owner);
}

/*=gfunc license
 *
 * what:  an arbitrary license
 * general_use:
 *
 * exparg: lic_name, file name of the license
 * exparg: prog_name, name of the licensed program or library
 * exparg: owner, Grantor of the License
 * exparg: prefix, String for starting each output line
 *
 * doc:
 *  Emit a string that contains the named license.
 *  This function is now deprecated.  Please @xref{SCM license-description}.
=*/
SCM
ag_scm_license(SCM license, SCM prog_name, SCM owner, SCM prefix)
{
    char const * prefx = ag_scm2zchars(prefix,    "line pfx");
    char const * pname = ag_scm2zchars(prog_name, "p name");
    char const * ownrz = ag_scm2zchars(owner,     "owner");
    static struct {
        char const * pzFN;
        tmap_info_t  mi;
    } lic = { NULL, { NULL, 0, 0, 0, 0, 0, 0, 0 }};

    char * pzRes;

    if (! scm_is_string(license))
        return SCM_UNDEFINED;

    {
        static char const * const apzSfx[] = { MK_LIC_SFX, NULL };
        static char fname[ AG_PATH_MAX ];
        char const * l_file = ag_scm2zchars(license, "lic file");

        /*
         *  Find the template file somewhere
         */
        if (! SUCCESSFUL(find_file(l_file, fname, apzSfx, NULL))) {
            errno = ENOENT;
            AG_CANT(MK_LIC_NO_LIC, l_file);
        }

        if ((lic.pzFN != NULL) && (strcmp(fname, lic.pzFN) != 0)) {
            text_munmap(&lic.mi);
            AGFREE(lic.pzFN);
            lic.pzFN = NULL;
        }

        if (lic.pzFN == NULL) {
            text_mmap(fname, PROT_READ|PROT_WRITE, MAP_PRIVATE, &lic.mi);
            if (TEXT_MMAP_FAILED_ADDR(lic.mi.txt_data))
                AG_ABEND(aprf(MK_LIC_NO_OPEN, l_file));

            if (dep_fp != NULL)
                add_source_file(l_file);

            AGDUPSTR(lic.pzFN, fname, "lic f name");
        }
    }

    /*
     *  Trim trailing white space.
     */
    {
        char * pz = (char *)lic.mi.txt_data + lic.mi.txt_size;
        while (  (pz > (char *)lic.mi.txt_data)
              && IS_WHITESPACE_CHAR(pz[-1]))
            pz--;
        *pz = NUL;
    }

    /*
     *  Get the addresses of the program name prefix and owner strings.
     *  Make sure they are reasonably sized (less than
     *  SCRIBBLE_SIZE).  Copy them to the scratch buffer.
     */
    if (scm_c_string_length(prog_name) >= SCRIBBLE_SIZE)
        AG_ABEND(aprf(MK_LIC_TOO_BIG_FMT, MK_LIC_BIG_PROG, SCRIBBLE_SIZE));

    if (scm_c_string_length(prefix) >= SCRIBBLE_SIZE)
        AG_ABEND(aprf(MK_LIC_TOO_BIG_FMT, MK_LIC_BIG_PFX, SCRIBBLE_SIZE));

    if (scm_c_string_length(owner) >= SCRIBBLE_SIZE)
        AG_ABEND(aprf(MK_LIC_TOO_BIG_FMT, MK_LIC_BIG_OWN, SCRIBBLE_SIZE));

    /*
     *  Reformat the string with the given arguments
     */
    pzRes = aprf((char *)lic.mi.txt_data, pname, ownrz);
    {
        int     pfx_size = (int)strlen(prefx);
        char *  pzScan   = pzRes;
        char *  pzOut;
        char *  pzSaveRes;
        ssize_t out_size = pfx_size;

        /*
         *  Figure out how much space we need (text size plus
         *  a prefix size for each newline)
         */
        for (;;) {
            switch (*(pzScan++)) {
            case NUL:
                goto exit_count;
            case NL:
                out_size += pfx_size;
                /* FALLTHROUGH */
            default:
                out_size++;
            }
        } exit_count:;

        /*
         *  Create our output buffer and insert the first prefix
         */
        pzOut = pzSaveRes = scribble_get(out_size);

        strcpy(pzOut, prefx);
        pzOut += pfx_size;
        pzScan = pzRes;

        for (;;) {
            switch (*(pzOut++) = *(pzScan++)) {
            case NUL:
                goto exit_copy;

            case NL:
                strcpy(pzOut, prefx);
                pzOut += pfx_size;
                break;

            default:
                break;
            }
        }
    exit_copy:;

        /*
         *  We allocated a temporary buffer that has all the
         *  formatting done, but need the prefixes on each line.
         */
        AGFREE(pzRes);

        return scm_from_latin1_stringn(
            pzSaveRes, (size_t)((pzOut - pzSaveRes) - 1));
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
 * end of agen5/expFormat.c */
