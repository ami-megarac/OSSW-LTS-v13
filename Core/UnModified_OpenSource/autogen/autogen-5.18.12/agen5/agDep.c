
/**
 * @file agDep.c
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

typedef struct flist flist_t;
struct flist {
    flist_t *   next;
    char        fname[1];
};

static flist_t * src_flist  = NULL;
static flist_t * targ_flist = NULL;

/**
 *  Add a source file to the dependency list
 *
 * @param pz pointer to file name
 */
LOCAL void
add_source_file(char const * pz)
{
    flist_t ** lp;

    /*
     * If a source is also a target, then we've created it.
     * Do not list in source dependencies.
     */
    lp = &targ_flist;
    while (*lp != NULL) {
        if (strcmp(pz, (*lp)->fname) == 0)
            return;
        lp = &((*lp)->next);
    }

    /*
     * No check for duplicate in source list.  Add if not found.
     */
    lp = &src_flist;
    while (*lp != NULL) {
        if (strcmp(pz, (*lp)->fname) == 0)
            return;
        lp = &((*lp)->next);
    }

    {
        size_t    l = strlen(pz);
        flist_t * p = AGALOC(sizeof(*p) + l, "sfile");
        *lp = p;
        p->next = NULL;
        memcpy(p->fname, pz, l + 1);
        if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
            fprintf(trace_fp, TRACE_ADD_SRC_FILE_FMT, p->fname);
    }
}

/**
 *  remove a source file from the dependency list
 *
 * @param pz pointer to file name
 */
LOCAL void
rm_source_file(char const * pz)
{
    flist_t ** pp = &src_flist; //!< point to where to stash removed "next"
    flist_t ** lp = &src_flist; //!< list scanning pointer

    for (;;) {
        if (*lp == NULL)
            return;
        if (strcmp(pz, (*lp)->fname) == 0)
            break;
        pp = lp;
        lp = &((*lp)->next);
    }
    {
        flist_t * p = *lp;
        *pp = p->next;
        if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
            fprintf(trace_fp, TRACE_RM_SRC_FILE_FMT, p->fname);
        AGFREE(p);
    }
}

/**
 *  Add a target file to the dependency list.  Avoid files in temp directories.
 *
 * @param pz pointer to file name
 */
LOCAL void
add_target_file(char const * pz)
{
    flist_t ** lp;

    /*
     *  Skip anything stashed in the temp directory.
     */
    if (  (temp_tpl_dir_len > 0)
       && (strncmp(pz, pz_temp_tpl, temp_tpl_dir_len) == 0)
       && (pz[temp_tpl_dir_len] == NUL))
        return;

    /*
     *  Target files override sources, just in case.
     *  (We sometimes extract from files we are about to replace.)
     */
    rm_source_file(pz);

    /*
     *  avoid duplicates and add to end of list
     */
    lp = &targ_flist;
    while (*lp != NULL) {
        if (strcmp(pz, (*lp)->fname) == 0)
            return;
        lp = &((*lp)->next);
    }

    {
        size_t    l = strlen(pz);
        flist_t * p = AGALOC(sizeof(*p) + l, "tfile");
        *lp = p;
        p->next = NULL;
        memcpy(p->fname, pz, l + 1);
        if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
            fprintf(trace_fp, TRACE_ADD_TARG_FILE_FMT, p->fname);
    }
}

/**
 *  Remove a target file from the dependency list
 *
 * @param pz pointer to file name
 */
LOCAL void
rm_target_file(char const * pz)
{
    flist_t ** lp = &targ_flist; //!< list scanning pointer

    for (;;) {
        if (*lp == NULL)
            return;
        if (strcmp(pz, (*lp)->fname) == 0)
            break;
        lp = &((*lp)->next);
    }
    {
        flist_t * p = *lp;
        *lp = p->next;
        if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
            fprintf(trace_fp, TRACE_RM_TARG_FILE_FMT, p->fname);
        AGFREE(p);
    }
}

/**
 * Create a dependency output file
 */
LOCAL void
start_dep_file(void)
{
    /*
     * Set dep_file to a temporary file name
     */
    {
        char * tfile_name;
        size_t dep_name_len;
        int    fd;

        if (dep_file == NULL) {
            dep_name_len = strlen(OPT_ARG(BASE_NAME));
            tfile_name = AGALOC(dep_name_len + TEMP_SUFFIX_LEN + 1, "dfileb");
            memcpy(tfile_name, OPT_ARG(BASE_NAME), dep_name_len);
            memcpy(tfile_name + dep_name_len, TEMP_SUFFIX,
                   TEMP_SUFFIX_LEN + 1);

        } else {
            dep_name_len = strlen(dep_file);
            tfile_name = AGALOC(dep_name_len + TEMP_SUFFIX_LEN, "dfile");
            memcpy(tfile_name, dep_file, dep_name_len);
            memcpy(tfile_name + dep_name_len, TEMP_SUFFIX + 2,
                   TEMP_SUFFIX_LEN - 1);
        }

        if (dep_target == NULL) {
            /*
             * If there is no target name, then the target is our output file.
             */
            char * q = AGALOC(dep_name_len + 1, "t-name");
            dep_target = q;
            memcpy(q, tfile_name, dep_name_len);
            q[dep_name_len] = NUL;
        }

        fd = mkstemp(tfile_name);
        if (fd < 0)
            AG_CANT(START_DEP_FOPEN_MSG, tfile_name);
        dep_file = tfile_name;

        /*
         * Create the file and write the leader.
         */
        dep_fp = fdopen(fd, "w");
    }

    if (dep_fp == NULL)
        AG_CANT(START_DEP_FOPEN_MSG, dep_file);

    fprintf(dep_fp, START_DEP_FILE_FMT, autogenOptions.pzProgPath);

    {
        int     ac = (int)autogenOptions.origArgCt - 1;
        char ** av = autogenOptions.origArgVect + 1;

        for (;;) {
            char * arg = *(av++);
            fprintf(dep_fp, START_DEP_ARG_FMT, arg);
            if (--ac == 0) break;
            fputs(DEP_FILE_SPLICE_STR, dep_fp);
        }
        putc('\n', dep_fp);
    }

    {
        char const * pnm = autogenOptions.pzPROGNAME;
        char const * bnm = strchr(dep_target, '/');
        char * pz;

        if (bnm != NULL)
            bnm++;
        else
            bnm = dep_target;

        {
            size_t pnm_sz = strlen(pnm);
            size_t bnm_sz = strlen(bnm);

            pz_targ_base = pz = AGALOC(pnm_sz + bnm_sz + 2, "t list");
            memcpy(pz, pnm, pnm_sz);
            pz += pnm_sz;
            *(pz++) = '_';
            memcpy(pz, bnm, bnm_sz + 1);
        }

        /*
         * Now scan over the characters in "pz_targ_base".  Anything that
         * is not a legal name character gets replaced with an underscore.
         */
        for (;;) {
            unsigned int ch = (unsigned int)*(pz++);
            if (ch == NUL)
                break;
            if (! IS_ALPHANUMERIC_CHAR(ch))
                pz[-1] = '_';
        }
    }
}

/**
 *  Set modification time and rename into result file name.
 */
static void
tidy_dep_file(void)
{
    static mode_t const fil_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    /*
     * Trim off the temporary suffix and rename the dependency file into
     * place.  We used a mkstemp name in case autogen failed.
     */
    do  {
        char * pze = strrchr(dep_file, '-');
        char * pzn;
        size_t len;

        if (pze == NULL) break;

        len = (size_t)(pze - dep_file);
        pzn = AGALOC(len + 1, "dep file");
        memcpy(pzn, dep_file, len);
        pzn[len] = NUL;

        unlink(pzn);
        rename(dep_file, pzn);
        AGFREE(dep_file);
        dep_file = pzn;
    } while (false);

#ifdef HAVE_FCHMOD
    fchmod(fileno(dep_fp), fil_mode);
    fclose(dep_fp);
#else
    fclose(dep_fp);
    chmod(dep_file, fil_mode);
#endif
    dep_fp = NULL;

    {
        struct utimbuf tbuf = {
            .actime  = time(NULL),
            .modtime = start_time
        };

        utime(dep_file, &tbuf);

        /*
         * If the target is not the dependency file, then ensure that the
         * file exists and set its time to the same time.  Ignore all errors.
         */
        if (strcmp(dep_file, dep_target) != 0) {
            if (access(dep_target, R_OK) != 0)
                close( open(dep_target, O_CREAT, fil_mode));

            utime(dep_target, &tbuf);
            AGFREE(dep_target);
        }
    }

    AGFREE(dep_file);
}

/**
 *  Print out and free a list of files.
 */
static void
print_list(flist_t * flist, char const * TMPDIR, size_t tmpdir_len)
{
    /*
     *  Omit temporary sources.  They are identified several ways:
     *  1. the file must be accessible
     *  2. the file must not match our temporary file template
     *  3. the file must not match TMPDIR from the environment
     */
    while (flist != NULL) {
        flist_t * p = flist;

        do  {
            if (access(p->fname, R_OK) != 0)
                break; // no longer accessible

            if (  (temp_tpl_dir_len > 0)
               && (strncmp(pz_temp_tpl, p->fname, temp_tpl_dir_len) == 0)
               && (p->fname[temp_tpl_dir_len] == DIRCH))
                break; // autogen temp file

            if (  (strncmp(TMPDIR, p->fname, tmpdir_len) == 0)
               && (p->fname[tmpdir_len] == DIRCH) )
                break; // TMPDIR directory file

            fprintf(dep_fp, DEP_List, p->fname);
        } while (false);

        flist = p->next;
        AGFREE(p);
    }
}

/**
 *  Finish off the dependency file.  Write out the lists of files,
 *  a rule to fulfill make's needs and, optionally, clean up rules.
 *  then close the file and tidy up.
 */
LOCAL void
wrap_up_depends(void)
{
    char const * TMPDIR = getenv("TMPDIR");
    size_t       tmpdir_len;
    if (TMPDIR != NULL) {
        tmpdir_len = strlen(TMPDIR);
    } else {
        TMPDIR = "/tmp";
        tmpdir_len = 4;
    }

    fprintf(dep_fp, DEP_TList, pz_targ_base);
    print_list(targ_flist, TMPDIR, tmpdir_len);

    fprintf(dep_fp, DEP_SList, pz_targ_base);
    print_list(src_flist, TMPDIR, tmpdir_len);

    targ_flist = src_flist = NULL;
    fprintf(dep_fp, DEP_FILE_WRAP_FMT, pz_targ_base, dep_target);

    if (dep_phonies) {
        /*
         *  Remove the target file name IFF it is different from
         *  the dependency file name.  The dependency file will not be
         *  removed, but it will be sent waaay back in time.
         */
        char * p, *q;
        AGDUPSTR(p, dep_file, "xx");

        q = p + strlen(p) - (TEMP_SUFFIX_LEN - 2);
        if ((q > p) && (*q == '-'))
            *q = NUL;
        q = p;

        /* DO NOT REMOVE DEPENDENCY FILE */
        if (strcmp(dep_target, p) == 0)
            p = (char *)zNil;
        fprintf(dep_fp, DEP_FILE_CLEAN_FMT, dep_target, pz_targ_base, p);
        AGFREE(q);
    }

#if 0
    if (serv_id != NULLPROCESS) {
        char * pz = shell_cmd("echo ${AG_Dep_File}");
        if (*pz != NUL) {
            /*
             * The target we are crating will now depend upon the target
             * created by the spawned autogen run.  That spawned run script
             * is responsible for ensuring that if there are multiple targets,
             * then they are all chained together so we only worry about one.
             */
            static char const incfmt[] =
                "\n%s : %s\ninclude %s\n";
            static char const targ[] = ".targ";
            size_t ln = strlen(pz);
            char * pt = AGALOC(ln + sizeof(targ), targ);
            if (strcmp(pz + ln - 4, ".dep") == 0)
                ln -= 4;
            memcpy(pt, pz, ln);
            memcpy(pt + ln, targ, sizeof(targ));
            fprintf(dep_fp, incfmt, dep_target, pt, pz);
            AGFREE(pt);
        }

        AGFREE(pz);
    }
#endif
    tidy_dep_file();
}

/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/agDep.c */
