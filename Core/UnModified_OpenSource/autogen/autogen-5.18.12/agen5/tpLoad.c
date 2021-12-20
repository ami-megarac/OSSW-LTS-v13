
/**
 * @file tpLoad.c
 *
 *  This module will load a template and return a template structure.
 *
 * @addtogroup autogen
 * @{
 */
/*
 * This file is part of AutoGen.
 * Copyright (C) 1992-2016 Bruce Korb - all rights reserved
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
static bool
read_okay(char const * fname);

static char const *
expand_dir(char const ** dir_pp, char * name_buf);

static inline bool
file_search_dirs(
    char const * in_name,
    char *       res_name,
    char const * const * sfx_list,
    char const * referring_tpl,
    size_t       nm_len,
    bool         no_suffix);

static size_t
cnt_macros(char const * pz);

static void
load_macs(templ_t * tpl, char const * fname, char const * pzData);

static templ_t *
digest_tpl(tmap_info_t * minfo, char * fname);
/* = = = END-STATIC-FORWARD = = = */

/**
 * Return the template structure matching the name passed in.
 */
LOCAL templ_t *
find_tpl(char const * tpl_name)
{
    templ_t * pT = named_tpls;
    while (pT != NULL) {
        if (streqvcmp(tpl_name, pT->td_name) == 0)
            break;
        pT = C(templ_t *, (pT->td_scan));
    }
    return pT;
}

/**
 * the name is a regular file with read access.
 * @param[in] fname  file name to check
 * @returns \a true when the named file exists and is a regular file
 * @returns \a false otherwise.
 */
static bool
read_okay(char const * fname)
{
    struct stat stbf;
    if (stat(fname, &stbf) != 0)
        return false;
    if (! S_ISREG(stbf.st_mode))
        return false;
    return (access(fname, R_OK) == 0) ? true : false;
}

/**
 * Expand a directory name that starts with '$'.
 *
 * @param[in,out] dir_pp pointer to pointer to directory name
 * @returns the resulting pointer
 */
static char const *
expand_dir(char const ** dir_pp, char * name_buf)
{
    char * res = VOIDP(*dir_pp);

    if (res[1] == NUL)
        AG_ABEND(aprf(LOAD_FILE_SHORT_NAME, res));

    if (! optionMakePath(name_buf, (int)AG_PATH_MAX, res,
                         autogenOptions.pzProgPath)) {
        /*
         * The name expanded to "empty", so substitute curdir.
         */
        strcpy(res, FIND_FILE_CURDIR);

    } else {
        free(res);
        AGDUPSTR(res, name_buf, "find dir name");
       *dir_pp = res; /* save computed name for later */
    }

    return res;
}

static inline bool
file_search_dirs(
    char const * in_name,
    char *       res_name,
    char const * const * sfx_list,
    char const * referring_tpl,
    size_t       nm_len,
    bool         no_suffix)
{
    /*
     *  Search each directory in our directory search list for the file.
     *  We always force two copies of this option, so we know it exists.
     *  Later entries are more recently added and are searched first.
     *  We start the "dirlist" pointing to the real last entry.
     */
    int  ct = STACKCT_OPT(TEMPL_DIRS);
    char const ** dirlist = STACKLST_OPT(TEMPL_DIRS) + ct - 1;
    char const *  c_dir   = FIND_FILE_CURDIR;

    /*
     *  IF the file name starts with a directory separator,
     *  then we only search once, looking for the exact file name.
     */
    if (*in_name == '/')
        ct = -1;

    for (;;) {
        char * pzEnd;

        /*
         *  c_dir is always FIND_FILE_CURDIR the first time through
         *  and is never that value after that.
         */
        if (c_dir == FIND_FILE_CURDIR) {

            memcpy(res_name, in_name, nm_len);
            pzEnd  = res_name + nm_len;
            *pzEnd = NUL;

        } else {
            unsigned int fmt_len;

            /*
             *  IF one of our template paths starts with '$', then expand it
             *  and replace it now and forever (the rest of this run, anyway).
             */
            if (*c_dir == '$')
                c_dir = expand_dir(dirlist+1, res_name);

            fmt_len = (unsigned)snprintf(
                res_name, AG_PATH_MAX - MAX_SUFFIX_LEN,
                FIND_FILE_DIR_FMT, c_dir, in_name);
            if (fmt_len >= AG_PATH_MAX - MAX_SUFFIX_LEN)
                break; // fail-return
            pzEnd = res_name + fmt_len;
        }

        if (read_okay(res_name))
            return true;

        /*
         *  IF the file does not already have a suffix,
         *  THEN try the ones that are okay for this file.
         */
        if (no_suffix && (sfx_list != NULL)) {
            char const * const * sfxl = sfx_list;
            *(pzEnd++) = '.';

            do  {
                strcpy(pzEnd, *(sfxl++)); /* must fit */
                if (read_okay(res_name))
                    return true;

            } while (*sfxl != NULL);
        }

        /*
         *  IF we've exhausted the search list,
         *  THEN see if we're done, else go through search dir list.
         *
         *  We try one more thing if there is a referrer.
         *  If the searched-for file is a full path, "ct" will
         *  start at -1 and we will leave the loop here and now.
         */
        if (--ct < 0) {
            if ((referring_tpl == NULL) || (ct != -1))
                break;
            c_dir = referring_tpl;

        } else {
            c_dir = *(dirlist--);
        }
    }

    return false;
}

/**
 *  Search for a file.
 *
 *  Starting with the current directory, search the directory list trying to
 *  find the base template file name.  If there is a referring template (a
 *  template with an "INCLUDE" macro), then try that, too, before giving up.
 *
 *  @param[in]  in_name    the file name we are looking for.
 *  @param[out] res_name   where we stash the file name we found.
 *  @param[in]  sfx_list   a list of suffixes to try, if \a in_name has none.
 *  @param[in]  referring_tpl  file name of the template with a INCLUDE macro.
 *
 *  @returns  \a SUCCESS when \a res_name is valid
 *  @returns  \a FAILURE when the file is not found.
 */
LOCAL tSuccess
find_file(char const * in_name,
          char *       res_name,
          char const * const * sfx_list,
          char const * referring_tpl)
{
    bool   no_suffix;
    void * free_me = NULL;
    tSuccess   res = SUCCESS;

    size_t nm_len = strlen(in_name);
    if (nm_len >= AG_PATH_MAX - MAX_SUFFIX_LEN)
        return FAILURE;

    /*
     *  Expand leading environment variables.
     *  We will not mess with embedded ones.
     */
    if (*in_name == '$') {
        if (! optionMakePath(res_name, (int)AG_PATH_MAX, in_name,
                             autogenOptions.pzProgPath))
            return FAILURE;

        AGDUPSTR(in_name, res_name, "find file name");
        free_me = VOIDP(in_name);

        /*
         *  in_name now points to the name the file system can use.
         *  It must _not_ point to res_name because we will likely
         *  rewrite that value using this pointer!
         */
        nm_len = strlen(in_name);
    }

    /*
     *  Not a complete file name.  If there is not already
     *  a suffix for the file name, then append ".tpl".
     *  Check for immediate access once again.
     */
    {
        char * bf = strrchr(in_name, '/');
        bf = (bf != NULL) ? strchr(bf, '.') : strchr(in_name,  '.');
        no_suffix = (bf == NULL);
    }

    /*
     *  The referrer is useful only if it includes a directory name.
     *  If not NULL, referring_tpl becomes an allocated directory name.
     */
    if (referring_tpl != NULL) {
        char * pz = strrchr(referring_tpl, '/');
        if (pz == NULL)
            referring_tpl = NULL;
        else {
            AGDUPSTR(referring_tpl, referring_tpl, "refer tpl");
            pz = strrchr(referring_tpl, '/');
            *pz = NUL;
        }
    }

    if (! file_search_dirs(in_name, res_name, sfx_list, referring_tpl,
                           nm_len, no_suffix))
        res = FAILURE;

    AGFREE(free_me);
    AGFREE(referring_tpl);
    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Count the macros in a template.
 *  We need to allocate the right number of pointers.
 */
static size_t
cnt_macros(char const * pz)
{
    size_t  ct = 2;
    for (;;) {
        pz = strstr(pz, st_mac_mark);
        if (pz == NULL)
            break;
        ct += 2;
        if (strncmp(pz - end_mac_len, end_mac_mark, end_mac_len) == 0)
            ct--;
        pz += st_mac_len;
    }
    return ct;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Load the macro array and file name.
 *  @param[in,out]  tpl     the template to load
 *  @param[in]      fname   the source file name of the template
 *  @param[in]      pzN     someting
 *  @param[in]      data    the template text
 */
static void
load_macs(templ_t * tpl, char const * fname, char const * pzData)
{
    macro_t * pMac = tpl->td_macros;

    {
        char *  txt = (char *)(pMac + tpl->td_mac_ct);

        AGDUPSTR(tpl->td_file, fname, "templ file");

        memcpy(txt, PSEUDO_MAC_TPL_FILE, PSEUDO_MAC_TPL_FILE_LEN+1);
        tpl->td_name = txt;
        tpl->td_text = (txt += PSEUDO_MAC_TPL_FILE_LEN);
        tpl->td_scan = txt + 1;
    }

    current_tpl = tpl;

    {
        macro_t * e_mac = parse_tpl(pMac, &pzData);
        int     ct;

        /*
         *  Make sure all of the input string was scanned.
         */
        if (pzData != NULL)
            AG_ABEND(LOAD_MACS_BAD_PARSE);

        ct = (int)(e_mac - pMac);

        /*
         *  IF there are empty macro slots,
         *  THEN pack the text
         */
        if (ct < tpl->td_mac_ct) {
            int     delta =
                (int)(sizeof(macro_t) * (size_t)(tpl->td_mac_ct - ct));
            void *  data  =
                (tpl->td_name == NULL) ? tpl->td_text : tpl->td_name;
            size_t  size  = (size_t)(tpl->td_scan - (char *)data);
            memmove(VOIDP(e_mac), data, size);

            tpl->td_text  -= delta;
            tpl->td_scan  -= delta;
            tpl->td_name  -= delta;
            tpl->td_mac_ct = ct;
        }
    }

    tpl->td_size = (size_t)(tpl->td_scan - (char *)tpl);
    tpl->td_scan = NULL;

    /*
     *  We cannot reallocate a smaller array because
     *  the entries are all linked together and
     *  realloc-ing it may cause it to move.
     */
#if defined(DEBUG_ENABLED)
    if (HAVE_OPT(SHOW_DEFS)) {
        static char const zSum[] =
            "loaded %d macros from %s\n"
            "\tBinary template size:  0x%zX\n\n";
        fprintf(trace_fp, zSum, tpl->td_mac_ct, fname, tpl->td_size);
    }
#endif
}

/**
 * Load a template from mapped memory.  Load up the pseudo macro,
 * count the macros, allocate the data, and parse all the macros.
 *
 * @param[in] minfo  information about the mapped memory.
 * @param[in] fname  the full path input file name.
 *
 * @returns the digested data
 */
static templ_t *
digest_tpl(tmap_info_t * minfo, char * fname)
{
    templ_t * res;

    /*
     *  Count the number of macros in the template.  Compute
     *  the output data size as a function of the number of macros
     *  and the size of the template data.  These may get reduced
     *  by comments.
     */
    char const * dta =
        load_pseudo_mac((char const *)minfo->txt_data, fname);

    size_t mac_ct   = cnt_macros(dta);
    size_t alloc_sz = (sizeof(*res) + (mac_ct * sizeof(macro_t))
                       + minfo->txt_size
                       - (size_t)(dta - (char const *)minfo->txt_data)
                       + strlen(fname) + 0x10)
                    & (size_t)(~0x0F);

    res = (templ_t *)AGALOC(alloc_sz, "main template");
    memset(VOIDP(res), 0, alloc_sz);

    /*
     *  Initialize the values:
     */
    res->td_magic  = magic_marker;
    res->td_size   = alloc_sz;
    res->td_mac_ct = (int)mac_ct;

    strcpy(res->td_start_mac, st_mac_mark); /* must fit */
    strcpy(res->td_end_mac,   end_mac_mark);   /* must fit */
    load_macs(res, fname, dta);

    res->td_name -= (long)res;
    res->td_text -= (long)res;
    res = (templ_t *)AGREALOC(VOIDP(res), res->td_size,
                                "resize template");
    res->td_name += (long)res;
    res->td_text += (long)res;

    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Starting with the current directory, search the directory
 *  list trying to find the base template file name.
 */
LOCAL templ_t *
tpl_load(char const * fname, char const * referrer)
{
    static tmap_info_t map_info;
    static char        tpl_file[ AG_PATH_MAX ];

    /*
     *  Find the template file somewhere
     */
    {
        static char const * const sfx_list[] = {
            LOAD_TPL_SFX_TPL, LOAD_TPL_SFX_AGL, NULL };
        if (! SUCCESSFUL(find_file(fname, tpl_file, sfx_list, referrer))) {
            errno = ENOENT;
            AG_CANT(LOAD_TPL_CANNOT_MAP, fname);
        }
    }

    /*
     *  Make sure the specified file is a regular file.
     *  Make sure the output time stamp is at least as recent.
     */
    {
        struct stat stbf;
        if (stat(tpl_file, &stbf) != 0)
            AG_CANT(LOAD_TPL_CANNOT_STAT, fname);

        if (! S_ISREG(stbf.st_mode)) {
            errno = EINVAL;
            AG_CANT(LOAD_TPL_IRREGULAR, fname);
        }

        if (outfile_time < stbf.st_mtime)
            outfile_time = stbf.st_mtime;
        if (maxfile_time < stbf.st_mtime)
            maxfile_time = stbf.st_mtime;
    }

    text_mmap(tpl_file, PROT_READ|PROT_WRITE, MAP_PRIVATE, &map_info);
    if (TEXT_MMAP_FAILED_ADDR(map_info.txt_data))
        AG_ABEND(aprf(LOAD_TPL_CANNOT_OPEN, tpl_file));

    if (dep_fp != NULL)
        add_source_file(tpl_file);

    /*
     *  Process the leading pseudo-macro.  The template proper
     *  starts immediately after it.
     */
    {
        macro_t * sv_mac = cur_macro;
        templ_t * res;
        cur_macro = NULL;

        res = digest_tpl(&map_info, tpl_file);
        cur_macro = sv_mac;
        text_munmap(&map_info);

        return res;
    }
}

/**
 * Deallocate anything related to a template.
 * This includes the pointer passed in and any macros that have an
 * unload procedure associated with it.
 *
 *  @param[in] tpl  the template to unload
 */
LOCAL void
tpl_unload(templ_t * tpl)
{
    macro_t * mac = tpl->td_macros;
    int ct = tpl->td_mac_ct;

    while (--ct >= 0) {
        unload_proc_p_t proc;
        unsigned int ix = mac->md_code;

        /*
         * "select" functions get remapped, depending on the alias used for
         * the selection.  See the "mac_func_t" enumeration in functions.h.
         */
        if (ix >= FUNC_CT)
            ix = FTYP_SELECT;

        proc = unload_procs[ ix ];
        if (proc != NULL)
            (*proc)(mac);

        mac++;
    }

    AGFREE(tpl->td_file);
    AGFREE(tpl);
}

/**
 *  This gets called when all is well at the end.
 *  The supplied template and all named templates are unloaded.
 *
 *  @param[in] tpl  the last template standing
 */
LOCAL void
cleanup(templ_t * tpl)
{
    if (HAVE_OPT(USED_DEFINES))
        print_used_defines();

    if (dep_fp != NULL)
        wrap_up_depends();

    optionFree(&autogenOptions);

    for (;;) {
        tpl_unload(tpl);
        tpl = named_tpls;
        if (tpl == NULL)
            break;
        named_tpls = C(templ_t *, (tpl->td_scan));
    }

    free_for_context(INT_MAX);
    unload_defs();
}

/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/tpLoad.c */
