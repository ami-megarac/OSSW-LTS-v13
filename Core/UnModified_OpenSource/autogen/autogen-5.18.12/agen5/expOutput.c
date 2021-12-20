
/**
 * @file expOutput.c
 *
 *  This module implements the output file manipulation function
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

#ifndef S_IAMB
/*
 *  Access Mask Bits (3 special plus RWX for User Group & Others (9))
 */
#  define S_IAMB      (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
#endif

#define NO_WRITE_MASK ((unsigned)(~(S_IWUSR|S_IWGRP|S_IWOTH) & S_IAMB))

typedef struct {
    char const *  pzSuspendName;
    out_stack_t * pOutDesc;
} tSuspendName;

static int            suspendCt   = 0;
static int            suspAllocCt = 0;
static tSuspendName * pSuspended  = NULL;
static int            outputDepth = 1;

/**
 * return the current line number
 */
static int
current_line(FILE * fp)
{
    int lnno = 1;

    while (! feof(fp)) {
        int ch = getc(fp);
        if (ch == NL)
            lnno++;
    }

    return lnno;
}

/**
 * guts of the output file/line function
 */
static SCM
do_output_file_line(int line_delta, char const * fmt)
{
    char * buf;
    char const * fname = cur_fpstack->stk_fname;

    if (cur_fpstack->stk_flags & FPF_TEMPFILE) {
        fname = OUTPUT_TEMP_FILE;
        line_delta = 0;

    } else if (fseek(cur_fpstack->stk_fp, 0, SEEK_SET) == 0) {
        line_delta += current_line(cur_fpstack->stk_fp);

    } else {
        line_delta = 0;
    }

    {
        ssize_t sz = (ssize_t)(strlen(fmt) + strlen(fname) + 24);
        buf = scribble_get(sz);
    }

    {
        void * args[2] = {
            VOIDP(fname),
            VOIDP(line_delta)
        };
        sprintfv(buf, fmt, (snv_constpointer *)args);
    }

    return scm_from_latin1_string(buf);
}

/**
 * chmod a-w on a file descriptor.
 */
LOCAL void
make_readonly(void)
{
#if defined(HAVE_FSTAT) || defined(HAVE_FCHMOD)
    int fd = fileno(cur_fpstack->stk_fp);
#endif
    struct stat sbuf;

    /*
     *  If the output is supposed to be writable, then also see if
     *  it is a temporary condition (set vs. a command line option).
     */
    if (ENABLED_OPT(WRITABLE)) {
        if (! HAVE_OPT(WRITABLE))
            CLEAR_OPT(WRITABLE);
        return;
    }

    /*
     *  Set our usage mask to all all the access
     *  bits that do not provide for write access
     */
#ifdef HAVE_FSTAT
    fstat(fd, &sbuf);
#else
    stat(cur_fpstack->stk_fname, &sbuf);
#endif

    /*
     *  Mask off the write permission bits, but ensure that
     *  the user read bit is set.
     */
    {
        mode_t f_mode = (NO_WRITE_MASK & sbuf.st_mode) | S_IRUSR;

#ifdef HAVE_FCHMOD
        fchmod(fd, f_mode);
#else
        chmod(cur_fpstack->stk_fname, f_mode);
#endif
    }
}

/**
 * Some common code for creating a new file
 */
LOCAL void
open_output_file(char const * fname, size_t nmsz, char const * mode, int flags)
{
    char *    pz;
    out_stack_t * p  = AGALOC(sizeof(*p), "out file stack");

    pz = (char *)AGALOC(nmsz + 1, "file name string");
    memcpy(pz, fname, nmsz);
    pz[ nmsz ] = NUL;
    memset(p, NUL, sizeof(*p));
    p->stk_fname = pz;

    /*
     *  IF we are creating the file and we are allowed to unlink the output,
     *  then start by unlinking the thing.
     */
    if ((*mode == 'w') && ((flags & FPF_NOUNLINK) == 0)) {
        if ((unlink(pz) != 0) && (errno != ENOENT))
            AG_CANT(OUTPUT_NO_UNLINK, pz);
    }

    /*
     * If we cannot write to the file, try to change permissions.
     */
    if (  (access(fname, W_OK) != 0)
       && (errno != ENOENT)) {
        struct stat sbuf;
        if (stat(fname, &sbuf) == 0) {
            mode_t m = (sbuf.st_mode & 07777) | S_IWUSR;
            chmod(fname, m);
        }
    }

    p->stk_fp = fopen(pz, mode);
    if (p->stk_fp == NULL)
        AG_CANT(OUTPUT_NO_OPEN, pz);

    p->stk_prev  = cur_fpstack;
    cur_fpstack  = p;
    p->stk_flags = FPF_FREE | flags;
    outputDepth++;

    if (OPT_VALUE_TRACE > TRACE_DEBUG_MESSAGE)
        fprintf(trace_fp, TRACE_OPEN_OUT, __func__, fname, mode);

    /*
     * Avoid printing temporary file names in the dependency file
     */
    if ((dep_fp != NULL) && ((flags & FPF_TEMPFILE) == 0))
        add_target_file(fname);
}

/*=gfunc out_delete
 *
 * what: delete current output file
 * doc:
 *  Remove the current output file.  Cease processing the template for
 *  the current suffix.  It is an error if there are @code{push}-ed
 *  output files.  Use the @code{(error "0")} scheme function instead.
 *  @xref{output controls}.
=*/
SCM
ag_scm_out_delete(void)
{
    /*
     *  Delete the current output file
     */
    if (OPT_VALUE_TRACE > TRACE_DEBUG_MESSAGE)
        fprintf(trace_fp, TRACE_OUT_DELETE, cur_fpstack->stk_fname);
    rm_target_file(cur_fpstack->stk_fname);
    outputDepth = 1;
    longjmp(abort_jmp_buf, PROBLEM);
    /* NOTREACHED */
    return SCM_UNDEFINED;
}


/*=gfunc out_move
 *
 * what:   change name of output file
 * exparg: new-name, new name for the current output file
 *
 * doc:
 *
 *  Rename current output file.  @xref{output controls}.
 *  Please note: changing the name will not save a temporary file from
 *  being deleted.  It @i{may}, however, be used on the root output file.
 *
 *  NOTE: if you are creating a dependency file, @i{both} the original
 *  file name @i{and} the new file name will be listed as derived files.
=*/
SCM
ag_scm_out_move(SCM new_file)
{
    size_t sz = scm_c_string_length(new_file);
    char * pz = (char *)AGALOC(sz + 1, "file name");
    memcpy(pz, scm_i_string_chars(new_file), sz);
    pz[sz] = NUL;

    if (OPT_VALUE_TRACE > TRACE_DEBUG_MESSAGE)
        fprintf(trace_fp, TRACE_MOVE_FMT, __func__,
                cur_fpstack->stk_fname, pz);

    if (strcmp(pz, cur_fpstack->stk_fname) != 0) {

        rename(cur_fpstack->stk_fname, pz);

        if (dep_fp != NULL) {
            rm_target_file(cur_fpstack->stk_fname);
            add_target_file(pz);
        }

        if ((cur_fpstack->stk_flags & FPF_STATIC_NM) == 0) {
            AGFREE(cur_fpstack->stk_fname);
            cur_fpstack->stk_flags &= ~FPF_STATIC_NM;
        }

        AGDUPSTR(cur_fpstack->stk_fname, pz, "file name");
    }

    return SCM_UNDEFINED;
}


/*=gfunc out_pop
 *
 * what:   close current output file
 * exparg: disp, return contents of the file, optional
 * doc:
 *  If there has been a @code{push} on the output, then close that
 *  file and go back to the previously open file.  It is an error
 *  if there has not been a @code{push}.  @xref{output controls}.
 *
 *  If there is no argument, no further action is taken.  Otherwise,
 *  the argument should be @code{#t} and the contents of the file
 *  are returned by the function.
=*/
SCM
ag_scm_out_pop(SCM ret_contents)
{
    SCM res = SCM_UNDEFINED;

    if (cur_fpstack->stk_prev == NULL)
        AG_ABEND(SCM_OUT_POP_EMPTY);

    if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
        fprintf(trace_fp, TRACE_POP_FMT, __func__, cur_fpstack->stk_fname,
                (ret_contents == SCM_UNDEFINED) ? "" : " #t");

    if (scm_is_bool(ret_contents) && scm_is_true(ret_contents)) {
        long  pos = ftell(cur_fpstack->stk_fp);
        char * pz;

        if (pos <= 0) {
            pz  = VOIDP(zNil); // const-ness not important
            pos = 0;

        } else {
            pz = scribble_get((ssize_t)pos + 1);
            rewind(cur_fpstack->stk_fp);
            if (fread(pz, (size_t)pos, (size_t)1, cur_fpstack->stk_fp) != 1)
                AG_CANT(SCM_OUT_POP_NO_REREAD, cur_fpstack->stk_fname);
        }

        res = scm_from_latin1_stringn(pz, (size_t)pos);
    }

    outputDepth--;
    out_close(false);
    return res;
}

/*=gfunc output_file_next_line
 *
 * what:   print the file name and next line number
 *
 * exparg: line_off, offset to line number,   optional
 * exparg: alt_fmt,  alternate format string, optional
 *
 * doc:
 *  Returns a string with the current output file name and line number.
 *  The default format is: # <line+1> "<output-file-name>" The argument may be
 *  either a number indicating an offset from the current output line number
 *  or an alternate formatting string.  If both are provided, then the first
 *  must be a numeric offset.
 *
 *  Be careful that you are directing output to the final output file.
 *  Otherwise, you will get the file name and line number of the temporary
 *  file.  That won't be what you want.
=*/
SCM
ag_scm_output_file_next_line(SCM num_or_str, SCM str)
{
    char const * fmt;
    int  line_off = 1;

    if (scm_is_number(num_or_str))
        line_off = (int)AG_SCM_TO_LONG(num_or_str);
    else
        str = num_or_str;

    if (scm_is_string(str))
        fmt = ag_scm2zchars(str, "file/line format");
    else
        fmt = FILE_LINE_FMT;

    return do_output_file_line(line_off, fmt);
}


/*=gfunc out_suspend
 *
 * what:   suspend current output file
 * exparg: suspName, A name tag for reactivating
 *
 * doc:
 *  If there has been a @code{push} on the output, then set aside the output
 *  descriptor for later reactiviation with @code{(out-resume "xxx")}.  The
 *  tag name need not reflect the name of the output file.  In fact, the
 *  output file may be an anonymous temporary file.  You may also change the
 *  tag every time you suspend output to a file, because the tag names are
 *  forgotten as soon as the file has been "resumed".
=*/
SCM
ag_scm_out_suspend(SCM susp_nm)
{
    if (cur_fpstack->stk_prev == NULL)
        AG_ABEND(OUT_SUSPEND_CANNOT);

    if (++suspendCt > suspAllocCt) {
        suspAllocCt += 8;
        if (pSuspended == NULL)
            pSuspended = (tSuspendName *)
                AGALOC(suspAllocCt * sizeof(tSuspendName), "susp file list");
        else
            pSuspended = (tSuspendName *)
                AGREALOC(VOIDP(pSuspended),
                         suspAllocCt * sizeof(tSuspendName), "add to susp f");
    }

    pSuspended[ suspendCt-1 ].pzSuspendName = scm_to_latin1_string(susp_nm);
    pSuspended[ suspendCt-1 ].pOutDesc      = cur_fpstack;
    if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
        fprintf(trace_fp, TRACE_SUSPEND, __func__, cur_fpstack->stk_fname,
                pSuspended[ suspendCt-1 ].pzSuspendName);

    cur_fpstack = cur_fpstack->stk_prev;
    outputDepth--;

    return SCM_UNDEFINED;
}


/*=gfunc out_resume
 *
 * what:   resume suspended output file
 * exparg: susp_nm, A name tag for reactivating
 * doc:
 *  If there has been a suspended output, then make that output descriptor
 *  current again.  That output must have been suspended with the same tag
 *  name given to this routine as its argument.
=*/
SCM
ag_scm_out_resume(SCM susp_nm)
{
    int  ix  = 0;
    char const * pzName = ag_scm2zchars(susp_nm, "resume name");

    for (; ix < suspendCt; ix++) {
        if (strcmp(pSuspended[ ix ].pzSuspendName, pzName) == 0) {
            pSuspended[ ix ].pOutDesc->stk_prev = cur_fpstack;
            cur_fpstack = pSuspended[ ix ].pOutDesc;
            free(VOIDP(pSuspended[ ix ].pzSuspendName)); /* Guile alloc */
            if (ix < --suspendCt)
                pSuspended[ ix ] = pSuspended[ suspendCt ];
            ++outputDepth;
            if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
                fprintf(trace_fp, TRACE_RESUME, __func__,
                        cur_fpstack->stk_fname, pzName);
            return SCM_UNDEFINED;
        }
    }

    AG_ABEND(aprf(OUT_RESUME_CANNOT, pzName));
    /* NOTREACHED */
    return SCM_UNDEFINED;
}


/*=gfunc out_emit_suspended
 *
 * what:   emit the text of suspended output
 * exparg: susp_nm, A name tag of suspended output
 * doc:
 *  This function is equivalent to
 *  @code{(begin (out-resume <name>) (out-pop #t))}
=*/
SCM
ag_scm_out_emit_suspended(SCM susp_nm)
{
    (void)ag_scm_out_resume(susp_nm);
    return ag_scm_out_pop(SCM_BOOL_T);
}


/*=gfunc ag_fprintf
 *
 * what:  format to autogen stream
 *
 * exparg: ag-diversion, AutoGen diversion name or number
 * exparg: format,       formatting string
 * exparg: format-arg,   list of arguments to formatting string, opt, list
 *
 * doc:  Format a string using arguments from the alist.
 *       Write to a specified AutoGen diversion.
 *       That may be either a specified suspended output stream
 *       (@pxref{SCM out-suspend}) or an index into the output stack
 *       (@pxref{SCM out-push-new}).  @code{(ag-fprintf 0 ...)} is
 *       equivalent to @code{(emit (sprintf ...))}, and
 *       @code{(ag-fprintf 1 ...)} sends output to the most recently
 *       suspended output stream.
=*/
SCM
ag_scm_ag_fprintf(SCM port, SCM fmt, SCM alist)
{
    int   list_len = (int)scm_ilength(alist);
    SCM   res =
        run_printf(ag_scm2zchars(fmt, WORD_FORMAT), list_len, alist);

    /*
     *  If "port" is a string, it must match one of the suspended outputs.
     *  Otherwise, we'll fall through to the abend.
     */
    if (scm_is_string(port)) {
        int  ix  = 0;
        char const * pzName = ag_scm2zchars(port, "resume name");

        for (; ix < suspendCt; ix++) {
            if (strcmp(pSuspended[ ix ].pzSuspendName, pzName) == 0) {
                out_stack_t * pSaveFp = cur_fpstack;
                cur_fpstack = pSuspended[ ix ].pOutDesc;
                (void) ag_scm_emit(res);
                cur_fpstack = pSaveFp;
                return SCM_UNDEFINED;
            }
        }
    }

    /*
     *  If "port" is a number, it is an index into the output stack with "0"
     *  (zero) representing the current output and "1" the last suspended
     *  output.  If the number is out of range, we'll fall through to the
     *  abend.
     */
    else if (scm_is_number(port)) {
        out_stack_t * pSaveFp = cur_fpstack;
        long val = AG_SCM_TO_LONG(port);

        if (val < 0) {
            char const * txt = ag_scm2zchars(res, "f-chars");
            fputs(txt, stderr);
            putc('\n', stderr);
            return SCM_UNDEFINED;
        }

        for (; val > 0; val--) {
            cur_fpstack = cur_fpstack->stk_prev;
            if (cur_fpstack == NULL) {
                cur_fpstack = pSaveFp;
                goto fprintf_woops;
            }
        }

        (void) ag_scm_emit(res);
        cur_fpstack  = pSaveFp;
        return SCM_UNDEFINED;
    }

    /*
     *  Still here?  We have a bad "port" specification.
     */
    fprintf_woops:

    AG_ABEND(AG_FPRINTF_BAD_PORT);
    /* NOTREACHED */
    return SCM_UNDEFINED;
}

/*=gfunc out_push_add
 *
 * what:   append output to file
 * exparg: file-name, name of the file to append text to
 *
 * doc:
 *  Identical to @code{push-new}, except the contents are @strong{not}
 *  purged, but appended to.  @xref{output controls}.
=*/
SCM
ag_scm_out_push_add(SCM new_file)
{
    static char const append_mode[] = "a" FOPEN_BINARY_FLAG "+";

    if (! scm_is_string(new_file))
        AG_ABEND(OUT_ADD_INVALID);

    open_output_file(scm_i_string_chars(new_file),
                     scm_c_string_length(new_file), append_mode, 0);

    return SCM_UNDEFINED;
}


/*=gfunc make_tmp_dir
 *
 * what:   create a temporary directory
 *
 * doc:
 *  Create a directory that will be cleaned up upon exit.
=*/
SCM
ag_scm_make_tmp_dir(void)
{
    if (pz_temp_tpl == NULL) {
        char * tmpdir = shell_cmd(MK_TMP_DIR_CMD);
        size_t tmp_sz = strlen(tmpdir);
        size_t bfsz   = SET_TMP_DIR_CMD_LEN + 2 * tmp_sz;
        char * cmdbf  = scribble_get(bfsz);

        pz_temp_tpl = tmpdir;
        temp_tpl_dir_len = tmp_sz - 9;    // "ag-XXXXXX"

        tmpdir[temp_tpl_dir_len - 1] = NUL;       // trim dir char
        if (snprintf(cmdbf, bfsz, SET_TMP_DIR_CMD, tmpdir) >= (int)bfsz)
            AG_ABEND(BOGUS_TAG);
        tmpdir[temp_tpl_dir_len - 1] = DIRCH;     // restore dir char

        ag_scm_c_eval_string_from_file_line(cmdbf, __FILE__, __LINE__);
    }

    return SCM_UNDEFINED;
}


/*=gfunc out_push_new
 *
 * what:   purge and create output file
 * exparg: file-name, name of the file to create, optional
 *
 * doc:
 *  Leave the current output file open, but purge and create
 *  a new file that will remain open until a @code{pop} @code{delete}
 *  or @code{switch} closes it.  The file name is optional and, if omitted,
 *  the output will be sent to a temporary file that will be deleted when
 *  it is closed.
 *  @xref{output controls}.
=*/
SCM
ag_scm_out_push_new(SCM new_file)
{
    static char const write_mode[] = "w" FOPEN_BINARY_FLAG "+";

    if (scm_is_string(new_file)) {
        open_output_file(scm_i_string_chars(new_file),
                         scm_c_string_length(new_file), write_mode, 0);
        return SCM_UNDEFINED;
    }

    /*
     *  "ENABLE_FMEMOPEN" is defined if we have either fopencookie(3GNU) or
     *  funopen(3BSD) is available and autogen was not configured with fmemopen
     *  disabled.  We cannot use the POSIX fmemopen.
     */
#if defined(ENABLE_FMEMOPEN)
    if (! HAVE_OPT(NO_FMEMOPEN)) {
        char *     pzNewFile;
        out_stack_t * p;

        /*
         *  This block is used IFF ENABLE_FMEMOPEN is defined and if
         *  --no-fmemopen is *not* selected on the command line.
         */
        p = (out_stack_t *)AGALOC(sizeof(out_stack_t), "out file stack");
        p->stk_prev  = cur_fpstack;
        p->stk_flags  = FPF_FREE;
        p->stk_fp  = ag_fmemopen(NULL, (ssize_t)0, "w" FOPEN_BINARY_FLAG "+");
        pzNewFile = (char *)MEM_FILE_STR;
        p->stk_flags |= FPF_STATIC_NM | FPF_NOUNLINK | FPF_NOCHMOD;

        if (p->stk_fp == NULL)
            AG_CANT(OUT_PUSH_NEW_FAIL, pzNewFile);

        p->stk_fname = pzNewFile;
        outputDepth++;
        cur_fpstack    = p;

        if (OPT_VALUE_TRACE > TRACE_DEBUG_MESSAGE)
            fprintf(trace_fp, TRACE_OUT_PUSH_NEW, __func__, pzNewFile);
        return SCM_UNDEFINED;
    }
#endif

    /*
     *  Either --no-fmemopen was specified or we cannot use ag_fmemopen().
     */
    {
        static size_t tmplen;
        char *  tmp_fnm;
        int     tmpfd;

        if (pz_temp_tpl == NULL)
            ag_scm_make_tmp_dir();

        tmplen  = temp_tpl_dir_len + 10;
        tmp_fnm = scribble_get((ssize_t)tmplen + 1);
        memcpy(tmp_fnm, pz_temp_tpl, tmplen + 1);
        tmpfd   = mkstemp(tmp_fnm);

        if (tmpfd < 0)
            AG_ABEND(aprf(OUT_PUSH_NEW_FAILED, pz_temp_tpl));

        open_output_file(tmp_fnm, tmplen, write_mode, FPF_TEMPFILE);
        close(tmpfd);
    }

    return SCM_UNDEFINED;
}

/*=gfunc out_switch
 *
 * what:   close and create new output
 * exparg: file-name, name of the file to create
 *
 * doc:
 *  Switch output files - close current file and make the current
 *  file pointer refer to the new file.  This is equivalent to
 *  @code{out-pop} followed by @code{out-push-new}, except that
 *  you may not pop the base level output file, but you may
 *  @code{switch} it.  @xref{output controls}.
=*/
SCM
ag_scm_out_switch(SCM new_file)
{
    struct utimbuf tbuf;
    char *  pzNewFile;

    if (! scm_is_string(new_file))
        return SCM_UNDEFINED;
    {
        size_t sz = scm_c_string_length(new_file);
        pzNewFile = AGALOC(sz + 1, "new file name");
        memcpy(pzNewFile, scm_i_string_chars(new_file), sz);
        pzNewFile[ sz ] = NUL;
    }

    /*
     *  IF no change, THEN ignore this
     */
    if (strcmp(cur_fpstack->stk_fname, pzNewFile) == 0) {
        AGFREE(pzNewFile);
        return SCM_UNDEFINED;
    }

    make_readonly();

    /*
     *  Make sure we get a new file pointer!!
     *  and try to ensure nothing is in the way.
     */
    unlink(pzNewFile);
    if (  freopen(pzNewFile, "w" FOPEN_BINARY_FLAG "+", cur_fpstack->stk_fp)
       != cur_fpstack->stk_fp)

        AG_CANT(OUT_SWITCH_FAIL, pzNewFile);

    /*
     *  Set the mod time on the old file.
     */
    tbuf.actime  = time(NULL);
    tbuf.modtime = outfile_time;
    utime(cur_fpstack->stk_fname, &tbuf);
    if (OPT_VALUE_TRACE > TRACE_DEBUG_MESSAGE)
        fprintf(trace_fp, TRACE_OUT_SWITCH,
                __func__, cur_fpstack->stk_fname, pzNewFile);
    cur_fpstack->stk_fname = pzNewFile;  /* memory leak */

    return SCM_UNDEFINED;
}


/*=gfunc out_depth
 *
 * what: output file stack depth
 * doc:  Returns the depth of the output file stack.
 *       @xref{output controls}.
=*/
SCM
ag_scm_out_depth(void)
{
    return scm_from_int(outputDepth);
}


/*=gfunc out_name
 *
 * what: current output file name
 * doc:  Returns the name of the current output file.  If the current file
 *       is a temporary, unnamed file, then it will scan up the chain until
 *       a real output file name is found.
 *       @xref{output controls}.
=*/
SCM
ag_scm_out_name(void)
{
    out_stack_t * p = cur_fpstack;
    while (p->stk_flags & FPF_UNLINK)  p = p->stk_prev;
    return scm_from_latin1_string(VOIDP(p->stk_fname));
}


/*=gfunc out_line
 *
 * what: output file line number
 * doc:  Returns the current line number of the output file.
 *       It rewinds and reads the file to count newlines.
=*/
SCM
ag_scm_out_line(void)
{
    int lineNum = 1;

    do {
        long svpos = ftell(cur_fpstack->stk_fp);
        long pos   = svpos;

        if (pos == 0)
            break;

        rewind(cur_fpstack->stk_fp);
        do {
            int ch = fgetc(cur_fpstack->stk_fp);
            if (ch < 0)
                break;
            if (ch == (int)NL)
                lineNum++;
        } while (--pos > 0);
        fseek(cur_fpstack->stk_fp, svpos, SEEK_SET);
    } while(0);

    return scm_from_int(lineNum);
}

#if 0 /* for compilers that do not like C++ comments... */
// This is done so that comment delimiters can be included in the doc.
//
// /*=gfunc   make_header_guard
//  *
//  * what:   make self-inclusion guard
//  *
//  * exparg: name , header group name
//  *
//  * doc:
//  * This function will create a @code{#ifndef}/@code{#define}
//  * sequence for protecting a header from multiple evaluation.
//  * It will also set the Scheme variable @code{header-file}
//  * to the name of the file being protected and it will set
//  * @code{header-guard} to the name of the @code{#define} being
//  * used to protect it.  It is expected that this will be used
//  * as follows:
//  * @example
//  * [+ (make-header-guard "group_name") +]
//  * ...
//  * #endif /* [+ (. header-guard) +] */
//  *
//  * #include "[+ (. header-file)  +]"
//  * @end example
//  * @noindent
//  * The @code{#define} name is composed as follows:
//  *
//  * @enumerate
//  * @item
//  * The first element is the string argument and a separating underscore.
//  * @item
//  * That is followed by the name of the header file with illegal
//  * characters mapped to underscores.
//  * @item
//  * The end of the name is always, "@code{_GUARD}".
//  * @item
//  * Finally, the entire string is mapped to upper case.
//  * @end enumerate
//  *
//  * The final @code{#define} name is stored in an SCM symbol named
//  * @code{header-guard}.  Consequently, the concluding @code{#endif} for the
//  * file should read something like:
//  *
//  * @example
//  * #endif /* [+ (. header-guard) +] */
//  * @end example
//  *
//  * The name of the header file (the current output file) is also stored
//  * in an SCM symbol, @code{header-file}.  Therefore, if you are also
//  * generating a C file that uses the previously generated header file,
//  * you can put this into that generated file:
//  *
//  * @example
//  * #include "[+ (. header-file) +]"
//  * @end example
//  *
//  * Obviously, if you are going to produce more than one header file from
//  * a particular template, you will need to be careful how these SCM symbols
//  * get handled.
// =*/
#endif
SCM
ag_scm_make_header_guard(SCM name)
{

    char const * opz; // output file name string
    size_t       osz;

    char const * gpz; // guard name string
    size_t       gsz;

    {
        out_stack_t * p = cur_fpstack;
        while (p->stk_flags & FPF_UNLINK)  p = p->stk_prev;
        opz = p->stk_fname;
        osz = strlen(opz);
    }

    /*
     *  Construct the gard name using the leader (passed in or "HEADER")
     *  and the trailer (always "_GUARD") and the output file name in between.
     */
    {
        /*
         *  Leader string and its length.  Usually passed, but defaults
         *  to "HEADER"
         */
        char const * lpz =
            scm_is_string(name) ? scm_i_string_chars(name) : HEADER_STR;
        size_t lsz = (lpz == HEADER_STR)
            ? HEADER_STR_LEN : scm_c_string_length(name);

        /*
         *  Full, maximal length of output
         */
        size_t hsz = lsz + osz + GUARD_SFX_LEN + 2;
        char * scan_p;

        /*
         * Sanity:
         */
        if (*lpz == NUL) {
            lpz = HEADER_STR;
            lsz = HEADER_STR_LEN;
            hsz += lsz;
        }
        scan_p = AGALOC(hsz, "header guard string");

        gpz = scan_p;  // gpz must be freed
        do  {
            *(scan_p++) = (char)toupper(*(lpz++));
        } while (--lsz > 0);

        /*
         *  This copy converts non-alphanumerics to underscores,
         *  but never inserts more than one at a time.  Thus, we may
         *  not use all of the space in "gpz".
         */
        lpz = opz;
        do  {
            *(scan_p++) = '_';
            lpz = BRK_ALPHANUMERIC_CHARS(lpz);
            while (IS_ALPHANUMERIC_CHAR(*lpz))
                *(scan_p++) = (char)toupper((unsigned char)*(lpz++));
        } while (*lpz != NUL);

        memcpy(scan_p, GUARD_SFX, GUARD_SFX_LEN + 1);
        gsz = (size_t)(scan_p - gpz) + GUARD_SFX_LEN;
    }

    {
        size_t sz1 = MK_HEAD_GUARD_SCM_LEN + gsz + osz;
        size_t sz2 = MK_HEAD_GUARD_GUARD_LEN + 2 * gsz;
        size_t sz  = (sz1 < sz2) ? sz2 : sz1;
        char * p   = scribble_get((ssize_t)sz);
        if (snprintf(p, sz, MK_HEAD_GUARD_SCM, opz, gpz) >= (int)sz)
            AG_ABEND(BOGUS_TAG);
        (void)ag_scm_c_eval_string_from_file_line(p, __FILE__, __LINE__);

        if (snprintf(p, sz, MK_HEAD_GUARD_GUARD, gpz) >= (int)sz)
            AG_ABEND(BOGUS_TAG);
        name = scm_from_latin1_string(p);
    }

    AGFREE(gpz);
    return (name);
}

/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/expOutput.c */
