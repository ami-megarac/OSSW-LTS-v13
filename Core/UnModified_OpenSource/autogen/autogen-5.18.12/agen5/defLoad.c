
/**
 * @file defLoad.c
 *
 *  This module loads the definitions, calls yyparse to decipher them,
 *  and then makes a fixup pass to point all children definitions to
 *  their parent definition.
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
    INPUT_DONE,
    INPUT_STDIN,
    INPUT_FILE
} def_input_mode_t;

static def_ent_t * free_de_list = NULL;
static void *      de_blocks    = NULL;

#define ENTRY_SPACE        (4096 - sizeof(void *))
#define ENTRY_ALLOC_CT     (ENTRY_SPACE / sizeof(def_ent_t))
#define ENTRY_ALLOC_SIZE   \
    ((ENTRY_ALLOC_CT * sizeof(def_ent_t)) + sizeof(void *))

/* = = = START-STATIC-FORWARD = = = */
static void
free_def_ent(def_ent_t * de);

static def_ent_t *
insert_ent(def_ent_t * de);

static def_input_mode_t
ready_def_input(char const ** ppzfile, size_t * psz);
/* = = = END-STATIC-FORWARD = = = */

LOCAL def_ent_t *
new_def_ent(void)
{
    def_ent_t * res = free_de_list;

    if (res != NULL) {
        free_de_list = res->de_next;

    } else {
        int    ct = ENTRY_ALLOC_CT-1;
        void * p  = AGALOC(ENTRY_ALLOC_SIZE, "def headers");

        *((void **)p) = de_blocks;
        de_blocks     = p;
        res = free_de_list = (def_ent_t *)((void **)p + 1);

        /*
         *  This is a post-loop test loop.  It will cycle one fewer times
         *  than there are 'def_ent_t' structs in the memory we just alloced.
         */
        do  {
            def_ent_t * next = res+1;
            res->de_next = next;

            /*
             *  When the loop ends, "res" will point to the last allocated
             *  structure instance.  That is the one we will return.
             */
            res = next;
        } while (--ct > 0);

        /*
         *  Unlink the last entry from the chain.  The next time this
         *  routine is called, the *FIRST* structure in this list will
         *  be returned.
         */
        res[-1].de_next = NULL;
    }

    memset(VOIDP(res), 0, sizeof(*res));
    return res;
}

static void
free_def_ent(def_ent_t * de)
{
    de->de_next  = free_de_list;
    free_de_list = de;
}

/**
 * Append a new entry at the end of a sibling (or twin) list.
 * @param de  new definition
 */
LOCAL void
print_ent(def_ent_t * de)
{
    int ix = 32 - (2 * ent_stack_depth);
    char const * space = PRINT_DEF_SPACES + ((ix < 0) ? 0 : ix);

    char const * vtyp;

    switch (de->de_type) {
    case VALTYP_UNKNOWN: vtyp = DEF_TYPE_UNKNOWN; break;
    case VALTYP_TEXT:    vtyp = DEF_TYPE_TEXT;    break;
    case VALTYP_BLOCK:   vtyp = DEF_TYPE_BLOCK;   break;
    default:             vtyp = DEF_TYPE_INVALID; break;
    }

    fprintf(trace_fp, PRINT_DEF_SHOW_FMT,
            space,
            de->de_name, (unsigned int)de->de_index,
            vtyp,
            de->de_file, de->de_line, de);
}

/**
 *  Remove a new entry from a sibling (or twin) list.
 *
 * @param[in]  de  dead definition
 */
LOCAL void
delete_ent(def_ent_t * de)
{
    def_ent_t * de_list = ent_stack[ ent_stack_depth ];
    def_ent_t ** list_p = &(de_list->de_val.dvu_entry);

    if (*list_p == NULL)
        AG_ABEND(RM_MISSING_DE);

    de_list = de_list->de_val.dvu_entry;

    while (strcmp(de->de_name, de_list->de_name) != 0) {
        if (de_list->de_next == NULL)
            AG_ABEND(RM_MISSING_DE);
        list_p  = &(de_list->de_next);
        de_list = de_list->de_next;
    }

    /*
     * Check for single entry list.  I.e. point the single pointer to
     * this entry to the next, possibly NULL, entry.
     */
    if (de_list->de_etwin == NULL) {
        if (de_list != de)
            AG_ABEND(RM_MISSING_DE);
        *list_p = de->de_next;
    }

    /*
     * Multiple entry list.  Check for list head.
     */
    else if (de_list == de) {
        /*
         * The list head now becomes the second "twin".
         * Adjust all the links *except* de_twin.
         */
        de_list = *list_p = de->de_twin;
        de_list->de_etwin = de->de_etwin;
        de_list->de_ptwin = NULL;
        de_list->de_next  = de->de_next;
    }

    /*
     * Somewhere in the middle.  Fix up the de_twin and de_ptwin pointers
     * around the current entry.  If the current entry is actually last,
     * then fix up the de_etwin pointer from the twin list head.
     */
    else {
        def_ent_t * pde = de->de_ptwin;
        def_ent_t * nde = de->de_twin;
        pde->de_twin = de->de_twin;
        if (nde != NULL)
            nde->de_ptwin = pde;
        else if (de_list->de_etwin != de)
            AG_ABEND(RM_MISSING_DE);
        else
            de_list->de_etwin = pde;
    }

    free_def_ent(de);
}

/**
 *  Append a new entry into a sibling (or twin) list.
 *
 * @param[in]  de new definition
 * @returns usually, the input, but sometimes it is necessary to move
 *  the data, so returns the address of the incoming data regardless.
 */
static def_ent_t *
insert_ent(def_ent_t * de)
{
    def_ent_t * de_list = ent_stack[ ent_stack_depth ];

    /*
     *  If the current level is empty, then just insert this one and quit.
     */
    if (de_list->de_val.dvu_entry == NULL) {
        if (de->de_index == NO_INDEX)
            de->de_index = 0;
        de_list->de_val.dvu_entry = de;

        return de;
    }
    de_list = de_list->de_val.dvu_entry;

    /*
     *  Scan the list looking for a "twin" (same-named entry).
     */
    while (strcmp(de->de_name, de_list->de_name) != 0) {
        /*
         *  IF we are at the end of the list,
         *  THEN put the new entry at the end of the list.
         *       This is a new name in the current context.
         *       The value type is forced to be the same type.
         */
        if (de_list->de_next == NULL) {
            de_list->de_next = de;

            if (de->de_index == NO_INDEX)
                de->de_index = 0;

            return de;
        }

        /*
         *  Check the next sibling for a twin value.
         */
        de_list = de_list->de_next;
    }

    /*  * * * * *  WE HAVE FOUND A TWIN
     *
     *  Link in the new twin chain entry into the list.
     */
    if (de->de_index == NO_INDEX) {
        def_ent_t * pT = de_list->de_etwin;
        if (pT == NULL)
            pT = de_list;

        de->de_index = pT->de_index + 1;
        pT->de_twin  = de;
        de->de_ptwin = pT;
        de_list->de_etwin = de;

    } else if (de_list->de_index > de->de_index) {

        /*
         *  Insert the new entry before any other in the list.  We
         *  actually do this by leaving the de_list pointer alone and
         *  swapping the contents of the definition entry.
         */
        def_ent_t def = *de;

        memcpy(&(de->de_name), &(de_list->de_name),
               sizeof(def) - ag_offsetof(def_ent_t, de_name));

        memcpy(&(de_list->de_name), &(def.de_name),
               sizeof(def) - ag_offsetof(def_ent_t, de_name));

        /*
         * Contents are swapped.  Link "de" after "de_list" & return "de_list".
         */
        de->de_twin = de_list->de_twin;
        if (de->de_twin != NULL)
            de->de_twin->de_ptwin = de;

        de->de_ptwin = de_list;
        de_list->de_twin  = de;

        /*
         *  IF this is the first twin, then the original list head is now
         *  the "end twin".
         */
        if (de_list->de_etwin == NULL)
            de_list->de_etwin = de;

        de = de_list;  /* Return the replacement structure address */

    } else {
        def_ent_t * scn = de_list;
        def_ent_t * twn = scn->de_twin;

        /*
         *  Insert someplace after the first entry.  Scan the list until
         *  we either find a larger index or we get to the end.
         */
        while (twn != NULL) {
            if (twn->de_index >= de->de_index) {
                if (twn->de_index == de->de_index)
                    AG_ABEND(aprf(DUP_VALUE_INDEX, de->de_name, de->de_index));
                break;
            }

            scn = twn;
            twn = twn->de_twin;
        }

        de->de_twin  = twn;
        scn->de_twin = de;
        de->de_ptwin = scn;
        if (twn == NULL)
            de_list->de_etwin = de;
        else
            twn->de_ptwin = de;
    }

    return de; /* sometimes will change */
}

/**
 * Figure out where to insert an entry in a list of twins.
 */
LOCAL def_ent_t *
number_and_insert_ent(char * name, char const * idx_str)
{
    def_ent_t * ent = new_def_ent();

    ent->de_name = name;

    if (idx_str == NULL)
        ent->de_index = NO_INDEX;

    else if (IS_SIGNED_NUMBER_CHAR(*idx_str))
        ent->de_index = strtol(idx_str, NULL, 0);

    else {
        idx_str = get_define_str(idx_str, true);
        if (idx_str != NULL)
             ent->de_index = strtol(idx_str, NULL, 0);
        else ent->de_index = NO_INDEX;
    }

    strtransform(ent->de_name, ent->de_name);
    ent->de_type  = VALTYP_UNKNOWN;
    ent->de_file  = (char *)cctx->scx_fname;
    ent->de_line  = cctx->scx_line;
    return (curr_ent = insert_ent(ent));
}

/**
 * figure out which file descriptor to use for reading definitions.
 */
static def_input_mode_t
ready_def_input(char const ** ppzfile, size_t * psz)
{
    struct stat stbf;

    if (! ENABLED_OPT(DEFINITIONS)) {
        base_ctx = (scan_ctx_t *)AGALOC(sizeof(scan_ctx_t), "scan context");
        memset(VOIDP(base_ctx), 0, sizeof(scan_ctx_t));
        base_ctx->scx_line  = 1;
        base_ctx->scx_fname = READY_INPUT_NODEF;

        if (! ENABLED_OPT(SOURCE_TIME))
            outfile_time = time(NULL);
        return INPUT_DONE;
    }

    *ppzfile = OPT_ARG(DEFINITIONS);

    if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
        fprintf(trace_fp, TRACE_DEF_LOAD);

    /*
     *  Check for stdin as the input file.  We use the current time
     *  as the modification time for stdin.  We also note it so we
     *  do not try to open it and we try to allocate more memory if
     *  the stdin input exceeds our initial allocation of 16K.
     */
    if (strcmp(*ppzfile, "-") == 0) {
        *ppzfile = OPT_ARG(DEFINITIONS) = "stdin";
        if (getenv(REQUEST_METHOD) != NULL) {
            load_cgi();
            cctx = base_ctx;
            dp_run_fsm();
            return INPUT_DONE;
        }

    accept_fifo:
        maxfile_time = outfile_time = time(NULL);
        *psz = 0x4000 - (4+sizeof(*base_ctx));
        return INPUT_STDIN;
    }

    /*
     *  This, then, must be a regular file.  Make sure of that and
     *  find out how big it was and when it was last modified.
     */
    if (stat(*ppzfile, &stbf) != 0)
        AG_CANT(READY_INPUT_STAT, *ppzfile);

    if (! S_ISREG(stbf.st_mode)) {
        if (S_ISFIFO(stbf.st_mode))
            goto accept_fifo;

        errno = EINVAL;
        AG_CANT(READY_INPUT_NOT_REG, *ppzfile);
    }

    /*
     *  IF the source-time option has been enabled, then
     *  our output file mod time will start as one second after
     *  the mod time on this file.  If any of the template files
     *  are more recent, then it will be adjusted.
     */
    *psz = (size_t)stbf.st_size;

    maxfile_time = stbf.st_mtime;
    outfile_time = ENABLED_OPT(SOURCE_TIME) ? stbf.st_mtime : time(NULL);

    return INPUT_FILE;
}

/**
 *  Suck in the entire definitions file and parse it.
 */
LOCAL void
read_defs(void)
{
    char const *  pzDefFile;
    char *        pzData;
    size_t        dataSize;
    size_t        sizeLeft;
    FILE *        fp;
    def_input_mode_t in_mode = ready_def_input(&pzDefFile, &dataSize);

    if (in_mode == INPUT_DONE)
        return;

    /*
     *  Allocate the space we need for our definitions.
     */
    sizeLeft = dataSize+4+sizeof(*base_ctx);
    base_ctx = (scan_ctx_t *)AGALOC(sizeLeft, "file buf");
    memset(VOIDP(base_ctx), 0, sizeLeft);
    base_ctx->scx_line = 1;
    sizeLeft = dataSize;

    /*
     *  Our base context will have its currency pointer set to this
     *  input.  It is also a scanning pointer, but since this buffer
     *  is never deallocated, we do not have to remember the initial
     *  value.  (It may get reallocated here in this routine, tho...)
     */
    pzData =
        base_ctx->scx_scan =
        base_ctx->scx_data = (char *)(base_ctx + 1);
    base_ctx->scx_next     = NULL;

    /*
     *  Set the input file pointer, as needed
     */
    if (in_mode == INPUT_STDIN)
        fp = stdin;

    else {
        fp = fopen(pzDefFile, "r" FOPEN_TEXT_FLAG);
        if (fp == NULL)
            AG_CANT(READ_DEF_OPEN, pzDefFile);

        if (dep_fp != NULL)
            add_source_file(pzDefFile);
    }

    /*
     *  Read until done...
     */
    for (;;) {
        size_t rdct = fread(VOIDP(pzData), (size_t)1, sizeLeft, fp);

        /*
         *  IF we are done,
         */
        if (rdct == 0) {
            /*
             *  IF it is because we are at EOF, then break out
             *  ELSE abend.
             */
            if (feof(fp) || (in_mode == INPUT_STDIN))
                break;

            AG_CANT(READ_DEF_READ, pzDefFile);
        }

        /*
         *  Advance input pointer, decrease remaining count
         */
        pzData   += rdct;
        sizeLeft -= rdct;

        /*
         *  See if there is any space left
         */
        if (sizeLeft == 0) {
            scan_ctx_t * p;
            off_t dataOff;

            /*
             *  IF it is a regular file, then we are done
             */
            if (in_mode != INPUT_STDIN)
                break;

            /*
             *  We have more data and we are out of space.
             *  Try to reallocate our input buffer.
             */
            dataSize += (sizeLeft = 0x1000);
            dataOff = pzData - base_ctx->scx_data;
            p = AGREALOC(VOIDP(base_ctx), dataSize + 4 + sizeof(*base_ctx),
                         "expand f buf");

            /*
             *  The buffer may have moved.  Set the data pointer at an
             *  offset within the new buffer and make sure our base pointer
             *  has been corrected as well.
             */
            if (p != base_ctx) {
                p->scx_scan = \
                    p->scx_data = (char *)(p + 1);
                pzData = p->scx_data + dataOff;
                base_ctx = p;
            }
        }
    }

    if (pzData == base_ctx->scx_data)
        AG_ABEND(READ_DEF_NO_DEFS);

    *pzData = NUL;
    AGDUPSTR(base_ctx->scx_fname, pzDefFile, "def file name");

    /*
     *  Close the input file, parse the data
     *  and alphabetically sort the definition tree contents.
     */
    if (in_mode != INPUT_STDIN)
        fclose(fp);

    cctx = base_ctx;
    dp_run_fsm();
}


LOCAL void
unload_defs(void)
{
    return;
}
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/defLoad.c */
