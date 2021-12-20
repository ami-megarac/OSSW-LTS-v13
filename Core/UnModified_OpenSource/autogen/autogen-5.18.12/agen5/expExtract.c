/**
 * @file expExtract.c
 *
 *  This module implements a file extraction function.
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

/* = = = START-STATIC-FORWARD = = = */
static char const *
load_extract_file(char const * new_fil);

static SCM
mk_empty_text(char const * start, char const * end, SCM def);

static SCM
get_text(char const * text, char const * start, char const * end, SCM def);
/* = = = END-STATIC-FORWARD = = = */

LOCAL char *
load_file(char const * fname)
{
    char * res = NULL;
    FILE * fp  = fopen(fname, "r");
    size_t fsz;

    if (fp == NULL)
        return NULL;

    {
        struct stat stbf;
        if (fstat(fileno(fp), &stbf) != 0) {
            int err = errno;
            fclose(fp);
            errno = err;
            return NULL;
        }

        fsz = stbf.st_size;
        res = (char *)AGALOC(fsz + 1, "load_file");
        if (outfile_time < stbf.st_mtime)
            outfile_time = stbf.st_mtime;
        if (maxfile_time < stbf.st_mtime)
            maxfile_time = stbf.st_mtime;
    }

    {
        char * scan = res;
        do  {
            size_t sz = fread(scan, 1, fsz, fp);
            if (sz == 0)
                fserr(AUTOGEN_EXIT_FS_ERROR, "fread", fname);

            scan += sz;
            fsz  -= sz;
        } while (fsz > 0);

        *scan = NUL;
    }

    fclose(fp);

    if (dep_fp != NULL)
        add_source_file(fname);
    return res;
}

/**
 *  Load a file into memory.  Keep it in memory and try to reuse it
 *  if we get called again.  Likely, there will be several extractions
 *  from a single file.
 *
 * @param[in] new_fil  name of extraction file to load
 * @returns the contents of the file or NULL
 */
static char const *
load_extract_file(char const * new_fil)
{
    static char const * last_fname = NULL;
    static char const * file_text  = NULL;

    /*
     *  Make sure that we:
     *
     *  o got the file name from the SCM value
     *  o return the old text if we are searching the same file
     *  o have a regular file with some data
     *  o can allocate the space we need...
     *
     *  If we don't know about the current file, we leave the data
     *  from any previous file we may have loaded.
     *
     *  DO *NOT* include this file in dependency output.  The output may vary
     *  based on its contents, but since it is always optional input, it cannot
     *  be made to be required by make.
     */
    if (new_fil == NULL)
        return NULL;

    if (  (last_fname != NULL)
       && (strcmp(last_fname, new_fil) == 0))
        return file_text;

    {
        struct stat sbuf;
        if (  (stat(new_fil, &sbuf) != 0)
           || (! S_ISREG(sbuf.st_mode))
           || (sbuf.st_size < 10) )
            return NULL;
    }

    if (last_fname != NULL) {
        AGFREE(last_fname);
        AGFREE(file_text);
        last_fname = file_text = NULL;
    }

    AGDUPSTR(last_fname, new_fil, "extract file");

    if (! HAVE_OPT(WRITABLE))
        SET_OPT_WRITABLE;

    file_text = load_file(last_fname);
    if (file_text != NULL)
        return file_text;

    AGFREE(last_fname);
    last_fname = NULL;
    if (file_text != NULL) {
        AGFREE(file_text);
        file_text = NULL;
    }
    return NULL;
}


/**
 *  Could not find the file or could not find the markers.
 *  Either way, emit an empty enclosure.
 */
static SCM
mk_empty_text(char const * start, char const * end, SCM def)
{
    ssize_t mlen = (ssize_t)(strlen(start) + strlen(end) + 3);
    char * pzOut;

    if (! scm_is_string(def)) {
        pzOut = scribble_get(mlen);
        sprintf(pzOut, LINE_CONCAT3_FMT+3, start, end);

    } else {
        char const * pzDef = ag_scm2zchars(def, "dft extr str");
        mlen += (ssize_t)scm_c_string_length(def) + 1;
        pzOut = scribble_get(mlen);
        sprintf(pzOut, LINE_CONCAT3_FMT, start, pzDef, end);
    }

    return scm_from_latin1_string(pzOut);
}


/*
 *  If we got it, emit it.
 */
static SCM
get_text(char const * text, char const * start, char const * end, SCM def)
{
    char const * pzS = strstr(text, start);
    char const * pzE;

    if (pzS == NULL)
        return mk_empty_text(start, end, def);

    pzE = strstr(pzS, end);
    if (pzE == NULL)
        return mk_empty_text(start, end, def);

    pzE += strlen(end);

    return scm_from_latin1_stringn(pzS, (size_t)(pzE - pzS));
}


/*=gfunc extract
 *
 * what:   extract text from another file
 * general_use:
 * exparg: file-name,  name of file with text
 * exparg: marker-fmt, format for marker text
 * exparg: caveat,     warn about changing marker, opt
 * exparg: default,    default initial text,       opt
 *
 * doc:
 *
 * This function is used to help construct output files that may contain
 * text that is carried from one version of the output to the next.
 *
 * The first two arguments are required, the second are optional:
 *
 * @itemize @bullet
 * @item
 *      The @code{file-name} argument is used to name the file that
 *      contains the demarcated text.
 * @item
 *      The @code{marker-fmt} is a formatting string that is used to construct
 *      the starting and ending demarcation strings.  The sprintf function is
 *      given the @code{marker-fmt} with two arguments.  The first is either
 *      "START" or "END".  The second is either "DO NOT CHANGE THIS COMMENT"
 *      or the optional @code{caveat} argument.
 * @item
 *      @code{caveat} is presumed to be absent if it is the empty string
 *      (@code{""}).  If absent, ``DO NOT CHANGE THIS COMMENT'' is used
 *      as the second string argument to the @code{marker-fmt}.
 * @item
 *      When a @code{default} argument is supplied and no pre-existing text
 *      is found, then this text will be inserted between the START and END
 *      markers.
 * @end itemize
 *
 * @noindent
 * The resulting strings are presumed to be unique within
 * the subject file.  As a simplified example:
 *
 * @example
 * [+ (extract "fname" "// %s - SOMETHING - %s" ""
 *             "example default") +]
 * @end example
 * @noindent
 * will result in the following text being inserted into the output:
 *
 * @example
 * // START - SOMETHING - DO NOT CHANGE THIS COMMENT
 * example default
 * // END   - SOMETHING - DO NOT CHANGE THIS COMMENT
 * @end example
 *
 * @noindent
 * The ``@code{example default}'' string can then be carried forward to
 * the next generation of the output, @strong{@i{provided}} the output
 * is not named "@code{fname}" @i{and} the old output is renamed to
 * "@code{fname}" before AutoGen-eration begins.
 *
 * @table @strong
 * @item NB:
 * You can set aside previously generated source files inside the pseudo
 * macro with a Guile/scheme function, extract the text you want to keep
 * with this extract function.  Just remember you should delete it at the
 * end, too.  Here is an example from my Finite State Machine generator:
 *
 * @example
 * [+ AutoGen5 Template  -*- Mode: text -*-
 * h=%s-fsm.h   c=%s-fsm.c
 * (shellf
 * "test -f %1$s-fsm.h && mv -f %1$s-fsm.h .fsm.head
 *  test -f %1$s-fsm.c && mv -f %1$s-fsm.c .fsm.code" (base-name))
 * +]
 * @end example
 *
 * This code will move the two previously produced output files to files
 * named ".fsm.head" and ".fsm.code".  At the end of the 'c' output
 * processing, I delete them.
 *
 * @item also NB:
 * This function presumes that the output file ought to be editable so
 * that the code between the @code{START} and @code{END} marks can be edited
 * by the template user.  Consequently, when the @code{(extract ...)} function
 * is invoked, if the @code{writable} option has not been specified, then
 * it will be set at that point.  If this is not the desired behavior, the
 * @code{--not-writable} command line option will override this.
 * Also, you may use the guile function @code{(chmod "file" mode-value)}
 * to override whatever AutoGen is using for the result mode.
 * @end table
=*/
SCM
ag_scm_extract(SCM file, SCM marker, SCM caveat, SCM def)
{
    char const * start;
    char const * end;
    char const * text;

    if (! scm_is_string(file) || ! scm_is_string(marker))
        return SCM_UNDEFINED;

    text = load_extract_file(ag_scm2zchars(file, "extr"));

    {
        char const * mark    = ag_scm2zchars(marker, "mark");
        char const * careful = EXTRACT_CAVEAT;

        if (scm_is_string(caveat) && (scm_c_string_length(caveat) > 0))
            careful = ag_scm2zchars(caveat, "caveat");

        start = aprf(mark, EXTRACT_START, careful);
        end   = aprf(mark, EXTRACT_END,   careful);
    }

    {
        SCM res;

        if (text == NULL)
             res = mk_empty_text(start, end, def);
        else res = get_text(text, start, end, def);

        AGFREE(start);
        AGFREE(end);
        return res;
    }
}


/*=gfunc find_file
 *
 * what:   locate a file in the search path
 * exparg: file-name,  name of file with text
 * exparg: @suffix  @  file suffix to try, too   @ opt @
 *
 * doc:
 *
 * AutoGen has a search path that it uses to locate template and definition
 * files.  This function will search the same list for @file{file-name}, both
 * with and without the @file{.suffix}, if provided.
=*/
SCM
ag_scm_find_file(SCM file, SCM suffix)
{
    SCM res = SCM_UNDEFINED;

    if (! scm_is_string(file))
        scm_wrong_type_arg(FIND_FILE_NAME, 1, file);

    {
        char z[ AG_PATH_MAX+1 ];
        char const * pz = ag_scm2zchars(file, "file-name");

        /*
         *  The suffix is optional.  If provided, it will be a string.
         */
        if (scm_is_string(suffix)) {
            char * apz[2];
            apz[0] = (char *)ag_scm2zchars(suffix, "file suffix");
            apz[1] = NULL;
            if (SUCCESSFUL(find_file(pz, z, (char const **)apz, NULL)))
                res = scm_from_latin1_string(z);

        } else if (SUCCESSFUL(find_file(pz, z, NULL, NULL)))
            res = scm_from_latin1_string(z);
    }

    return res;
}

/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/expExtract.c */
