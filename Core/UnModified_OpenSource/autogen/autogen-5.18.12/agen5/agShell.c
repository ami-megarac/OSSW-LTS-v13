
/**
 * @file agShell.c
 *
 *  Manage a server shell process
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
static char * cur_dir = NULL;

/*=gfunc chdir
 *
 * what:   Change current directory
 *
 * exparg: dir, new directory name
 *
 * doc:  Sets the current directory for AutoGen.  Shell commands will run
 *       from this directory as well.  This is a wrapper around the Guile
 *       native function.  It returns its directory name argument and
 *       fails the program on failure.
=*/
SCM
ag_scm_chdir(SCM dir)
{
    static char const zChdirDir[] = "chdir directory";

    scm_chdir(dir);

    /*
     *  We're still here, so we have a valid argument.
     */
    if (cur_dir != NULL)
        free(cur_dir);
    {
        char const * pz = ag_scm2zchars(dir, zChdirDir);
        cur_dir = malloc(scm_c_string_length(dir) + 1);
        strcpy((char *)cur_dir, pz);
    }
    return dir;
}

/*=gfunc shell
 *
 * what:  invoke a shell script
 * general_use:
 *
 * exparg: command, shell command - the result is from stdout, , list
 * This may be a list of strings and they will all be concatenated.
 *
 * doc:
 *  Generate a string by writing the value to a server shell and reading the
 *  output back in.  The template programmer is responsible for ensuring that
 *  it completes within 10 seconds.  If it does not, the server will be
 *  killed, the output tossed and a new server started.
 *
 *  Please note: This is the same server process used by the '#shell'
 *  definitions directive and backquoted @code{`} definitions.  There may be
 *  left over state from previous shell expressions and the @code{`}
 *  processing in the declarations.  However, a @code{cd} to the original
 *  directory is always issued before the new command is issued.
 *
 *  Also note:  When initializing, autogen will set the environment
 *  variable "AGexe" to the full path of the autogen executable.
=*/
SCM
ag_scm_shell(SCM cmd)
{
#ifndef SHELL_ENABLED
    return cmd;
#else
    if (! scm_is_string(cmd)) {
        static SCM joiner = SCM_UNDEFINED;
        if (joiner == SCM_UNDEFINED)
            joiner = scm_gc_protect_object(scm_from_latin1_string(""));

        cmd = ag_scm_join(joiner, cmd);
        if (! scm_is_string(cmd))
            return SCM_UNDEFINED;
    }

    {
        char * pz = shell_cmd(ag_scm2zchars(cmd, "command"));
        cmd   = scm_from_latin1_string(pz);
        AGFREE(pz);
        return cmd;
    }
#endif
}

/*=gfunc shellf
 *
 * what:  format a string, run shell
 * general_use:
 *
 * exparg: format, formatting string
 * exparg: format-arg, list of arguments to formatting string, opt, list
 *
 * doc:  Format a string using arguments from the alist,
 *       then send the result to the shell for interpretation.
=*/
SCM
ag_scm_shellf(SCM fmt, SCM alist)
{
    int    len = (int)scm_ilength(alist);
    char * pz  = ag_scm2zchars(fmt, "format");
    fmt = run_printf(pz, len, alist);

#ifdef SHELL_ENABLED
    pz = shell_cmd(ag_scm2zchars(fmt, "shell script"));
    if (pz == NULL)
        AG_ABEND(SHELL_RES_NULL_MSG);
    fmt = scm_from_latin1_string(pz);
    AGFREE(pz);
#endif
    return fmt;
}

#ifndef SHELL_ENABLED
HIDE_FN(void closeServer(void) {;})

HIDE_FN(char * shell_cmd(char const * pzCmd)) {
     char * pz;
     AGDUPSTR(pz, pzCmd, "dummy shell command");
     return pz;
}
#else

/*
 *  Dual pipe opening of a child process
 */
static fp_pair_t    serv_pair     = { NULL, NULL };
static pid_t        serv_id       = NULLPROCESS;
static bool         was_close_err = false;
static int          log_ct        = 0;
static char const * last_cmd      = NULL;

/* = = = START-STATIC-FORWARD = = = */
static void
handle_signal(int signo);

static void
set_orig_dir(void);

static bool
send_cmd_ok(char const * cmd);

static void
start_server_cmd_trace(void);

static void
send_server_init_cmds(void);

static void
server_setup(void);

static int
chain_open(int in_fd, char const ** arg_v, pid_t * child_pid);

static pid_t
server_open(fd_pair_t * fd_pair, char const ** ppArgs);

static pid_t
server_fp_open(fp_pair_t * fp_pair, char const ** ppArgs);

static inline void
realloc_text(char ** p_txt, size_t * p_sz, size_t need_len);

static char *
load_data(void);
/* = = = END-STATIC-FORWARD = = = */

LOCAL void
close_server_shell(void)
{
    if (serv_id == NULLPROCESS)
        return;

    (void)kill(serv_id, SIGTERM);
#ifdef HAVE_USLEEP
    usleep(100000); /* 1/10 of a second */
#else
    sleep(1);
#endif
    (void)kill(serv_id, SIGKILL);
    serv_id = NULLPROCESS;
    if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
        fprintf(trace_fp, "close_server_shell in %u state\n",
                processing_state);

    /*
     *  This guard should not be necessary.  However, sometimes someone
     *  holds an allocation pthread lock when a seg fault occurrs.  fclose
     *  needs that lock, so we hang waiting for it.  Oops.  So, when we
     *  are aborting, we just let the OS close these file descriptors.
     */
    if (processing_state != PROC_STATE_ABORTING) {
        (void)fclose(serv_pair.fp_read);
        /*
         *  This is _completely_ wrong, but sometimes there are data left
         *  hanging about that gets sucked up by the _next_ server shell
         *  process.  That should never, ever be in any way possible, but
         *  it is the only explanation for a second server shell picking up
         *  the initialization string twice.  It must be a broken timing
         *  issue in the Linux stdio code.  I have no other explanation.
         */
        fflush(serv_pair.fp_write);
        (void)fclose(serv_pair.fp_write);
    }

    serv_pair.fp_read = serv_pair.fp_write = NULL;
}

/**
 * handle SIGALRM and SIGPIPE signals while waiting for server shell
 * responses.
 */
static void
handle_signal(int signo)
{
    static int timeout_limit = 5;
    if ((signo == SIGALRM) && (--timeout_limit <= 0))
        AG_ABEND(TOO_MANY_TIMEOUTS_MSG);

    fprintf(trace_fp, SHELL_DIE_FMT, strsignal(signo), signo);
    was_close_err = true;

    (void)fputs(SHELL_LAST_CMD_MSG, trace_fp);
    {
        char const * pz = (last_cmd == NULL)
            ? SHELL_UNK_LAST_CMD_MSG : last_cmd;
        fprintf(trace_fp, SHELL_CMD_FMT, cur_dir, pz, SH_DONE_MARK, log_ct);
    }
    last_cmd = NULL;
    close_server_shell();
}

/**
 * first time starting a server shell, we get our current directory.
 * That value is kept, but may be changed via a (chdir "...") scheme call.
 */
static void
set_orig_dir(void)
{
    char * p = malloc(AG_PATH_MAX);
    if (p == NULL)
        AG_ABEND(SET_ORIG_DIR_NO_MEM_MSG);

    cur_dir = getcwd(p, AG_PATH_MAX);

    if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
        fputs(TRACE_SHELL_FIRST_START, trace_fp);
}

/**
 * Send a command string down to the server shell
 */
static bool
send_cmd_ok(char const * cmd)
{
    last_cmd = cmd;
    fprintf(serv_pair.fp_write, SHELL_CMD_FMT, cur_dir, last_cmd,
            SH_DONE_MARK, ++log_ct);

    if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL) {
        fprintf(trace_fp, LOG_SEP_FMT, log_ct);
        fprintf(trace_fp, SHELL_CMD_FMT, cur_dir, last_cmd,
                SH_DONE_MARK, log_ct);
    }

    (void)fflush(serv_pair.fp_write);
    if (was_close_err)
        fprintf(trace_fp, CMD_FAIL_FMT, cmd);
    return ! was_close_err;
}

/**
 * Tracing level is TRACE_EVERYTHING, so send the server shell
 * various commands to start "set -x" tracing and display the
 * trap actions.
 */
static void
start_server_cmd_trace(void)
{
    fputs(TRACE_XTRACE_MSG, trace_fp);
    if (send_cmd_ok(SHELL_XTRACE_CMDS)) {
        char * pz = load_data();
        fputs(SHELL_RES_DISCARD_MSG, trace_fp);
        fprintf(trace_fp, TRACE_TRAP_STATE_FMT, pz);
        AGFREE(pz);
    }
}

/**
 * Send down the initialization string with our PID in it, as well
 * as the full path name of the autogen executable.
 */
static void
send_server_init_cmds(void)
{
    was_close_err = false;

    {
        char * pzc = AGALOC(SHELL_INIT_STR_LEN
                            + 11 // log10(1 << 32) + 1
                            + strlen(autogenOptions.pzProgPath),
                            "server init");
        sprintf(pzc, SHELL_INIT_STR, (unsigned int)getpid(),
                autogenOptions.pzProgPath,
                (dep_fp == NULL) ? "" : dep_file);

        if (send_cmd_ok(pzc))
            AGFREE(load_data());
        AGFREE(pzc);
    }

    if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
        fputs(SHELL_RES_DISCARD_MSG, trace_fp);

    if (OPT_VALUE_TRACE >= TRACE_EVERYTHING)
        start_server_cmd_trace();
}

/**
 * Perform various initializations required when starting
 * a new server shell process.
 */
static void
server_setup(void)
{
    if (cur_dir == NULL)
        set_orig_dir();
    else if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
        fputs(SHELL_RESTART_MSG, trace_fp);

    {
        struct sigaction new_sa;
        new_sa.sa_handler = handle_signal;
        new_sa.sa_flags   = 0;
        (void)sigemptyset(&new_sa.sa_mask);
        (void)sigaction(SIGPIPE, &new_sa, NULL);
        (void)sigaction(SIGALRM, &new_sa, NULL);
    }

    send_server_init_cmds();

    last_cmd = NULL;
}

/**
 *  Given an FD for an inferior process to use as stdin,
 *  start that process and return a NEW FD that that process
 *  will use for its stdout.  Requires the argument vector
 *  for the new process and, optionally, a pointer to a place
 *  to store the child's process id.
 *
 * @param stdinFd the file descriptor for the process' stdin
 * @param ppArgs  The program and argument vector
 * @param pChild  where to stash the child process PID
 *
 * @returns the read end of a pipe the child process uses for stdout
 */
static int
chain_open(int in_fd, char const ** arg_v, pid_t * child_pid)
{
    fd_pair_t out_pair = { -1, -1 };
    pid_t     ch_pid;
    char const * shell;

    /*
     *  If we did not get an arg list, use the default
     */
    if (arg_v == NULL)
        arg_v = server_args;

    /*
     *  If the arg list does not have a program,
     *  assume the zShellProg from the environment, or, failing
     *  that, then sh.  Set argv[0] to whatever we decided on.
     */
    if (shell = *arg_v,
       (shell == NULL) || (*shell == NUL))

        *arg_v = shell = shell_program;

    /*
     *  Create a pipe it will be the child process' stdout,
     *  and the parent will read from it.
     */
    if (pipe((int *)&out_pair) < 0) {
        if (child_pid != NULL)
            *child_pid = NOPROCESS;
        return -1;
    }

    /*
     *  Make sure our standard streams are all flushed out before forking.
     *  (avoid duplicate output). Call fork() and see which process we become
     */
    fflush(stdout);
    fflush(stderr);
    if (trace_fp != stderr)
        fflush(trace_fp);

    ch_pid = fork();
    switch (ch_pid) {
    case NOPROCESS:    /* parent - error in call */
        close(in_fd);
        close(out_pair.fd_read);
        close(out_pair.fd_write);
        if (child_pid != NULL)
            *child_pid = NOPROCESS;
        return -1;

    default:           /* parent - return opposite FD's */
        if (child_pid != NULL)
            *child_pid = ch_pid;

        close(in_fd);
        close(out_pair.fd_write);
        if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL) {
            fprintf(trace_fp, TRACE_SHELL_PID_FMT, (unsigned int)ch_pid);
            fflush(trace_fp);
        }

        return out_pair.fd_read;

    case NULLPROCESS:  /* child - continue processing */
        break;
    }

    /*
     *  Close the pipe end handed back to the parent process,
     *  plus stdin and stdout.
     */
    close(out_pair.fd_read);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);

    /*
     *  Set stdin/out to the fd passed in and the write end of our new pipe.
     */
    fcntl(out_pair.fd_write, F_DUPFD, STDOUT_FILENO);
    fcntl(in_fd, F_DUPFD, STDIN_FILENO);

    /*
     *  set stderr to our trace file (if not stderr).
     */
    if (trace_fp != stderr) {
        close(STDERR_FILENO);
        fcntl(fileno(trace_fp), F_DUPFD, STDERR_FILENO);
    }

    /*
     *  Make the output file unbuffered only.
     *  We do not want to wait for shell output buffering.
     */
    setvbuf(stdout, NULL, _IONBF, (size_t)0);

    if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL) {
        fprintf(trace_fp, TRACE_SHELL_STARTS_FMT, shell);

        fflush(trace_fp);
    }

    execvp((char *)shell, (char **)arg_v);
    AG_CANT("execvp", shell);
    /* NOTREACHED */
    return -1;
}

/**
 *  Given a pointer to an argument vector, start a process and
 *  place its stdin and stdout file descriptors into an fd pair
 *  structure.  The "fd_write" connects to the inferior process
 *  stdin, and the "fd_read" connects to its stdout.  The calling
 *  process should write to "fd_write" and read from "fd_read".
 *  The return value is the process id of the created process.
 */
static pid_t
server_open(fd_pair_t * fd_pair, char const ** ppArgs)
{
    pid_t chId = NOPROCESS;

    /*
     *  Create a bi-directional pipe.  Writes on 0 arrive on 1
     *  and vice versa, so the parent and child processes will
     *  read and write to opposite FD's.
     */
    if (pipe((int *)fd_pair) < 0)
        return NOPROCESS;

    fd_pair->fd_read = chain_open(fd_pair->fd_read, ppArgs, &chId);
    if (chId == NOPROCESS)
        close(fd_pair->fd_write);

    return chId;
}


/**
 *  Identical to "server_open()", except that the "fd"'s are "fdopen(3)"-ed
 *  into file pointers instead.
 */
static pid_t
server_fp_open(fp_pair_t * fp_pair, char const ** ppArgs)
{
    fd_pair_t   fd_pair;
    pid_t     chId = server_open(&fd_pair, ppArgs);

    if (chId == NOPROCESS)
        return chId;

    fp_pair->fp_read  = fdopen(fd_pair.fd_read,  "r" FOPEN_BINARY_FLAG);
    fp_pair->fp_write = fdopen(fd_pair.fd_write, "w" FOPEN_BINARY_FLAG);
    return chId;
}

static inline void
realloc_text(char ** p_txt, size_t * p_sz, size_t need_len)
{
    *p_sz  = (*p_sz + need_len + 0xFFF) & (size_t)~0xFFF;
    *p_txt = AGREALOC(VOIDP(*p_txt), *p_sz, "expand text");
}

/**
 *  Read data from a file pointer (a pipe to a process in this context)
 *  until we either get EOF or we get a marker line back.
 *  The read data are stored in a malloc-ed string that is truncated
 *  to size at the end.  Input is assumed to be an ASCII string.
 */
static char *
load_data(void)
{
    char *  text;
    size_t  text_sz = 4096;
    size_t  used_ct = 0;
    char *  scan;
    char    zLine[ 1024 ];
    int     retry_ct = 0;
#define LOAD_RETRY_LIMIT 4

    scan  = text = AGALOC(text_sz, "Text Block");
    *text = NUL;

    for (;;) {
        char * line_p;

        /*
         *  Set a timeout so we do not wait forever.  Sometimes we don't wait
         *  at all and we should.  Retry in those cases (but not on EOF).
         */
        alarm((unsigned int)OPT_VALUE_TIMEOUT);
        line_p = fgets(zLine, (int)sizeof(zLine), serv_pair.fp_read);
        alarm(0);

        if (line_p == NULL) {
            /*
             *  Guard against a server timeout
             */
            if (serv_id == NULLPROCESS)
                break;

            if ((OPT_VALUE_TRACE >= TRACE_SERVER_SHELL) || (retry_ct++ > 0))
                fprintf(trace_fp, SHELL_READ_ERR_FMT, errno, strerror(errno));

            if (feof(serv_pair.fp_read) || (retry_ct > LOAD_RETRY_LIMIT))
                break;

            continue;  /* no data - retry */
        }

        /*
         *  Check for magic character sequence indicating 'DONE'
         */
        if (strncmp(zLine, SH_DONE_MARK, SH_DONE_MARK_LEN) == 0)
            break;

        {
            size_t llen = strlen(zLine);
            if (text_sz <= used_ct + llen) {
                realloc_text(&text, &text_sz, llen);
                scan = text + used_ct;
            }

            memcpy(scan, zLine, llen);
            used_ct += llen;
            scan    += llen;
        }

        /*
         *  Stop now if server timed out or if we are at EOF
         */
        if ((serv_id == NULLPROCESS) || feof(serv_pair.fp_read)) {
            fputs(SHELL_NO_END_MARK_MSG, trace_fp);
            break;
        }
    }

    /*
     *  Trim off all trailing white space and shorten the buffer
     *  to the size actually used.
     */
    while (  (scan > text)
          && IS_WHITESPACE_CHAR(scan[-1]))
        scan--;
    text_sz = (size_t)(scan - text) + 1;
    *scan  = NUL;

    if (OPT_VALUE_TRACE >= TRACE_SERVER_SHELL)
        fprintf(trace_fp, TRACE_SHELL_RESULT_MSG,
                (int)text_sz, text, zLine);

    return AGREALOC(VOIDP(text), text_sz, "resize out");
#undef LOAD_RETRY_LIMIT
}

/**
 *  Run a semi-permanent server shell.  The program will be the
 *  one named by the environment variable $SHELL, or default to "sh".
 *  If one of the commands we send to it takes too long or it dies,
 *  we will shoot it and restart one later.
 *
 *  @param cmd the input command string
 *  @returns an allocated string, even if it is empty.
 */
LOCAL char *
shell_cmd(char const * cmd)
{
    /*
     *  IF the shell server process is not running yet,
     *  THEN try to start it.
     */
    if (serv_id == NULLPROCESS) {
        static int was_started = 0;
        if (was_started++ > 0) {
            fputs(SERV_RESTART, stderr);
            if (trace_fp != stderr)
                fputs(SERV_RESTART, trace_fp);
        }

        putenv((char *)SHELL_SET_PS4_FMT);
        serv_id = server_fp_open(&serv_pair, server_args);
        if (serv_id > 0)
            server_setup();
    }

    /*
     *  IF it is still not running,
     *  THEN return the nil string.
     */
    if (serv_id <= 0) {
        char * pz = (char *)AGALOC(1, "Text Block");

        *pz = NUL;
        return pz;
    }

    /*
     *  Make sure the process will pay attention to us,
     *  send the supplied command, and then
     *  have it output a special marker that we can find.
     */
    if (! send_cmd_ok(cmd))
        return NULL;

    /*
     *  Now try to read back all the data.  If we fail due to either
     *  a sigpipe or sigalrm (timeout), we will return the nil string.
     */
    {
        char * pz = load_data();
        if (pz == NULL) {
            fprintf(trace_fp, CMD_FAIL_FMT, cmd);
            close_server_shell();
            pz = (char *)AGALOC(1, "Text Block");

            *pz = NUL;

        } else if (was_close_err)
            fprintf(trace_fp, CMD_FAIL_FMT, cmd);

        last_cmd = NULL;
        return pz;
    }
}

#endif /* ! SHELL_ENABLED */
/**
 * @}
 *
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/agShell.c */
