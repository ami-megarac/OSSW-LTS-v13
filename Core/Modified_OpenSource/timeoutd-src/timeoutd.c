/*
		"@(#) timeoutd.c 1.6 by Shane Alderton"
			based on:
		"@(#) autologout.c by David Dickson" 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Thanks to:
    	David Dickson for writing the original autologout.c
    	programme upon which this programme was based.

*/
#include    <fcntl.h>
#include    <stdio.h>
#include    <signal.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <utmp.h>
#include    <pwd.h>
#include    <grp.h>
#include    <sys/syslog.h>
#include    <time.h>
#include    <ctype.h>
#include    <termios.h>
#include    <fcntl.h>
#include    <sys/ioctl.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <sys/wait.h>

#ifdef DEBUG
#define DEBUG_WTMP
#endif
#define TELNET_WTMP_ISSUE

#define OPENLOG_FLAGS	LOG_CONS|LOG_PID
#define SYSLOG_DEBUG	LOG_DEBUG

/* For those systems (SUNOS) which don't define this: */
#ifndef WTMP_FILE
#define WTMP_FILE "/usr/adm/wtmp"
#endif

#ifdef SUNOS
#define ut_pid ut_time
#define ut_user ut_name
#define SEEK_CUR 1
#define SEEK_END 2
#define SURE_KILL 1

FILE	*utfile = NULL;
#define NEED_UTMP_UTILS
#define NEED_STRSEP
#endif


#ifdef NEED_UTMP_UTILS
void setutent()
{
	if (utfile == NULL)
	{
		if ((utfile = fopen("/etc/utmp", "r")) == NULL)
		{
			openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
			syslog(LOG_ERR, "Could not open /etc/utmp");
			closelog();
			exit(1);
		}
	}
	else fseek(utfile, 0L, 0);
}

struct utmp *getutent()    /* returns next utmp file entry */
{
	static struct utmp	uent;

	while (fread(&uent, sizeof(struct utmp), 1, utfile) == 1)
	{
		if (uent.ut_line[0] != 0 && uent.ut_name[0] != 0)
			return &uent;
	}
	return (struct utmp *) NULL;
}
#endif

#ifndef linux
#define N_TTY 1
#define N_SLIP 2
#endif

#ifndef CONFIG
#define CONFIG "/etc/timeouts"
#endif

#define MAXLINES 512
#define max(a,b) ((a)>(b)?(a):(b))

#define ACTIVE		1
#define IDLEMAX		2
#define SESSMAX		3
#define DAYMAX		4
#define NOLOGIN		5
#define	IDLEMSG		0
#define	SESSMSG		1
#define	DAYMSG		2
#define	NOLOGINMSG	3

#define KWAIT		10  /* Time to wait after sending a kill signal */

char	*limit_names[] = {"idle", "session", "daily", "nologin"};

char	*daynames[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA", "WK", "AL", NULL};
char	daynums[] = {   1  ,  2  ,  4  ,  8  ,  16 ,  32 ,  64 ,  62 ,  127, 0};

struct utmp *utmpp;         /* pointer to utmp file entry */
char        *ctime();       /* returns pointer to time string */
struct utmp *getutent();    /* returns next utmp file entry */
void	    shutdown();
void	    reread_config();
void        reapchild();
void        free_wtmp();
void        read_wtmp();
char	    chk_timeout();
void	    logoff_msg();
int	    getdisc();
void        read_config();
void        check_idle();
void        bailout(char *message, int status);
void        killit(int pid, char *user, char *dev);

struct ut_list {
	struct utmp	elem;
	struct ut_list	*next;
};

struct ut_list	*wtmplist = (struct ut_list *) NULL;
struct ut_list	*ut_list_p;

struct time_ent {
	int	days;
	int	starttime;
	int	endtime;
};

struct config_ent {
	struct time_ent	*times;
	char	*ttys;
	char	*users;
	char	*groups;
	char	login_allowed;
	int	idlemax;
	int	sessmax;
	int	daymax;
	int	warntime;
	char	*messages[10];
};

struct config_ent	*config[MAXLINES + 1];
char	errmsg[256];
char	dev[64] = {0};
char	limit_type;
int	configline = 0;
int	pending_reread = 0;
int	allow_reread = 0;
time_t	time_now;
struct tm	now;
int		now_hhmm;
int	daytime = 0;	/* Amount of time a user has been on in current day */

#ifdef NEED_STRCASECMP
int strcasecmp(char *s1, char *s2)
{
	while (*s1 && *s2)
	{
	  if (tolower(*s1) < tolower(*s2))
	  	return -1;
	  else if (tolower(*s1) > tolower(*s2))
	  	return 1;
	  s1++;
	  s2++;
	}
	if (*s1)
		return -1;
	if (*s2)
		return 1;
	return 0;
}
#endif

#ifdef NEED_STRSEP
char *strsep (stringp, delim)
char **stringp;
char *delim;
{
	char	*retp = *stringp;
	char	*p;

	if (!**stringp) return NULL;

	while (**stringp)
	{
		p = delim;
		while (*p)
		{
		  if (*p == **stringp)
		  {
		    **stringp = '\0';
		    (*stringp)++;
		    return retp;
		  }
		  p++;
	  	}
	  	(*stringp)++;
	}
	return retp;
}
#endif

int main(argc, argv)
int	argc;
char	*argv[];
{
    char *duplicate={0};
    signal(SIGTERM, shutdown);
    signal(SIGHUP, reread_config);
    signal(SIGCHLD, reapchild);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);

/* The only valid invocations are "timeoutd" or "timeoutd user tty" */
    if (argc != 1 && argc != 3)
    {
      openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
      syslog(LOG_ERR, "Incorrect invocation of timeoutd (argc=%d) by UID %d.", argc, getuid());
      closelog();
      exit(5);
    }

    /* read config file into memory */
    read_config();

/* Handle the "timeoutd user tty" invocation */
/* This is a bit of a shameless hack, but, well, it works. */
    if (argc == 3)
    {
#ifdef DEBUG
        openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        syslog(SYSLOG_DEBUG, "Running in user check mode.  Checking user %s on %s.", argv[1], argv[2]);
        closelog();
#endif
        if ((argv[2] != NULL) && (strlen(argv[2]) < 20))
        {
            strcpy(dev, "/dev/");
            if((sizeof(dev) - strlen(dev) - 1) >= strlen(argv[2])) /*Source Size should be less than Destination Size*/            
                strcat(dev, argv[2]);
            else
            {
                printf("Error: Source size= %s is larger than Destination Size\n",argv[2]);
                return -1;
            }
        }
        else
        {
            openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
            syslog(LOG_ERR, "Incorrect invocation of timeoutd, invalid device(%s).", argv[2]);/*SCA Fix [Unbounded source buffer]:: False Positive */
	    /*Reason For False Positive - The arguments received by this function are not user inputs*/
            closelog();
            exit(5);
        }
        time_now = time((time_t *)0);  /* get current time */
        now = *(localtime(&time_now));  /* Break it into bits */
        now_hhmm = now.tm_hour * 100 + now.tm_min;
        allow_reread = 0;
        read_wtmp(); /* Read in today's wtmp entries */
        switch(chk_timeout(argv[1], dev, "",  0, 0))
        {
            case DAYMAX:
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        duplicate= strdup(argv[1]);
        if (duplicate == 0)
        {
           printf("Out of memory\n");
        }//…process out of memory error; do not continue……

		syslog(LOG_NOTICE,
		       "User %s on %s exceeded maximum daily limit (%d minutes).  Login check failed.",
		       duplicate, argv[2], config[configline]->daymax);
            free(duplicate);
    		closelog();
    		/*
    		printf("\r\nLogin not permitted.  You have exceeded your maximum daily limit.\r\n");
    		printf("Please try again tomorrow.\r\n");
    		*/
    		logoff_msg(1);
    		exit(10);
            case NOLOGIN:
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        duplicate= strdup(argv[1]);
        if (duplicate == 0)
        {
           printf("Out of memory\n");
        }//…process out of memory error; do not continue……

		syslog(LOG_NOTICE,
		       "User %s not allowed to login on %s at this time.  Login check failed.",
		       duplicate, argv[2]);
    		closelog();
            free(duplicate);
    		/*
    		printf("\r\nLogin not permitted at this time.  Please try again later.\r\n");
    		*/
    		logoff_msg(1);
    		exit(20);
            case ACTIVE:
#ifdef DEBUG
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        duplicate= strdup(argv[1]);
        if (duplicate == 0)
        {
           printf("Out of memory\n");
        }//…process out of memory error; do not continue……

		syslog(SYSLOG_DEBUG, "User %s on %s passed login check.", duplicate, argv[2]);
            free(duplicate);
            closelog();
#endif
            free_wtmp();
		exit(0);
            default:
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        duplicate= strdup(argv[1]);
        if (duplicate == 0)
        {
           printf("Out of memory\n");
        }//…process out of memory error; do not continue……

		syslog(LOG_ERR, "Internal error checking user %s on %s - unexpected return from chk_timeout",
			duplicate, argv[2]);
            free(duplicate);
    		closelog();
    		exit(30);
        }
    }

    /* If running in daemon mode (no parameters) */
    if (fork())             /* the parent process */
        exit(0);            /* exits */

    close(0);
    close(1);
    close(2);
    setsid();

    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(LOG_NOTICE, "Daemon started.");
    closelog();

    /* the child processes all utmp file entries: */
    while (1)
    {
        /* Record time at which we woke up & started checking */
        time_now = time((time_t *)0);  /* get current time */
        now = *(localtime(&time_now));  /* Break it into bits */
        now_hhmm = now.tm_hour * 100 + now.tm_min;
        allow_reread = 0;
        read_wtmp(); /* Read in today's wtmp entries */
    	setutent();
#ifdef DEBUG
	openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    	syslog(SYSLOG_DEBUG, "Time to check utmp for exceeded limits.");
    	closelog();
#endif
        while ((utmpp = getutent()) != (struct utmp *) NULL)
            check_idle();
        free_wtmp();  /* Free up memory used by today's wtmp entries */
        allow_reread = 1;
        if (pending_reread)
           reread_config(SIGHUP);
#ifdef DEBUG
	openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        syslog(SYSLOG_DEBUG, "Finished checking utmp... sleeping for 1 minute.");
	closelog();
#endif
        sleep(5);
    }
}

/* Read in today's wtmp entries */

void read_wtmp()
{
    FILE	*fp;
    struct utmp	ut;
    struct tm	*tm;
#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "Reading today's wtmp entries.");
    closelog();
#endif

    if ((fp = fopen(WTMP_FILE, "r")) == NULL)
      bailout("Could not open wtmp file!", 1);

#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "Seek to end of wtmp");
    closelog();
#endif
    /* Go to end of file minus one structure */
    if ( 0 != fseek(fp, -1L * sizeof(struct utmp), SEEK_END))
    {
        printf("fseek: Unable to reachour EOF\n");
        fclose(fp);
        return;
    }

    while (fread(&ut, sizeof(struct utmp), 1, fp) == 1)
    {
      tm = localtime((const time_t *)&ut.ut_time);

      if (tm->tm_year != now.tm_year || tm->tm_yday != now.tm_yday)
        break;

#ifndef SUNOS
      if (ut.ut_type == USER_PROCESS ||
          ut.ut_type == DEAD_PROCESS ||
          ut.ut_type == UT_UNKNOWN ||	/* SA 19940703 */
	  ut.ut_type == LOGIN_PROCESS ||
          ut.ut_type == BOOT_TIME)
#endif
      {
        if ((ut_list_p = (struct ut_list *) malloc(sizeof(struct ut_list))) == NULL)
          bailout("Out of memory in read_wtmp.", 1);
        ut_list_p->elem = ut;
        ut_list_p->next = wtmplist;
        wtmplist = ut_list_p;
      }

      /* Position the file pointer 2 structures back */
      if (fseek(fp, -2 * sizeof(struct utmp), SEEK_CUR) < 0) break;
    }
    fclose(fp);
#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "Finished reading today's wtmp entries.");
    closelog();
#endif
}

/* Free up memory used by today's wtmp entries */

void free_wtmp()
{
#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "Freeing list of today's wtmp entries.");
    closelog();
#endif

    while (wtmplist)
    {
#ifdef DEBUG_WTMP
    struct tm	*tm;
    tm = localtime(&(wtmplist->elem.ut_time));
    printf("%d:%d %s %s %s\n", 
    	tm->tm_hour,tm->tm_min, wtmplist->elem.ut_line,
    	wtmplist->elem.ut_user,
#ifndef SUNOS
    	wtmplist->elem.ut_type == LOGIN_PROCESS?"login":wtmplist->elem.ut_type == BOOT_TIME?"reboot":"logoff"
#else
	""
#endif
	);
#endif
        ut_list_p = wtmplist;
        wtmplist = wtmplist->next;
        free(ut_list_p);
    }
#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "Finished freeing list of today's wtmp entries.");
    closelog();
#endif
}

void	store_times(t, time_str)
struct time_ent **t;
char *time_str;
{
    int	i = 0;
    int	ar_size = 2;
    char	*p;
    struct time_ent	*te;

    while (time_str[i])
      if (time_str[i++] == ',')
        ar_size++;

    if ((*t = (struct time_ent *) malloc (ar_size * sizeof(struct time_ent))) == NULL)
	bailout("Out of memory", 1);
    te = *t;

    p = strtok(time_str, ",");
/* For each day/timerange set, */
    while (p)
    {
/* Store valid days */
      te->days = 0;
      while (isalpha(*p))
      {
        if (!p[1] || !isalpha(p[1]))
        {
	  openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
          syslog(LOG_ERR, "Malformed day name (%c%c) in time field of config file (%s).  Entry ignored.", p[0], p[1], CONFIG);
          closelog();
          (*t)->days = 0;
          return;
        }
        *p = toupper(*p);
        p[1] = toupper(p[1]);

      	i = 0;
      	while (daynames[i])
      	{
      	  if (!strncmp(daynames[i], p, 2))
      	  {
      	    te->days |= daynums[i];
      	    break;
      	  }
      	  i++;
      	}
      	if (!daynames[i])
      	{
	  openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
          syslog(LOG_ERR, "Malformed day name (%c%c) in time field of config file (%s).  Entry ignored.", p[0], p[1], CONFIG);
          closelog();
          (*t)->days = 0;
          return;
      	}
      	p += 2;
      }

/* Store start and end times */
      if (*p)
      {
        if (strlen(p) != 9 || p[4] != '-')
        {
	  openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
          syslog(LOG_ERR, "Malformed time (%s) in time field of config file (%s).  Entry ignored.", p, CONFIG);
          closelog();
          (*t)->days = 0;
          return;
        }
        te->starttime = atoi(p);
        te->endtime = atoi(p+5);
            if ((te->starttime == 0 && strncmp(p, "0000-", 5)) || (te->endtime == 0 && strcmp(p+5, "0000")))
        {
	  openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
          syslog(LOG_ERR, "Invalid range (%s) in time field of config file (%s).  Entry ignored.", p, CONFIG);
          closelog();
          (*t)->days = 0;
          return;
        }
      }
      else
      {
      	te->starttime = 0;
      	te->endtime = 2359;
      }
      p = strtok(NULL, ",");
      te++;
    }
    te->days = 0;
}

void alloc_cp(a, b)
char **a;
char *b;
{
	if ((*a = (char *) malloc(strlen(b)+1)) == NULL)
		bailout("Out of memory", 1);
	else	strcpy(*a, b);
}

void read_config()
{
    FILE	*config_file;
    char	*p;
    char	*lstart;
    int		i = 0;
    char	line[256];
    char	*tok;
    int		linenum = 0;
#ifdef DEBUG
    int j;
#endif

    if ((config_file = fopen(CONFIG, "r")) == NULL)
      bailout("Cannot open config file", 1);

    while (fgets(line, 256, config_file) != NULL)
    {
      linenum++;
      p = line;
      while  (*p && (*p == ' ' || *p == '\t'))
        p++;
      lstart = p;
      while (*p && *p != '#' && *p != '\n')
        p++;
      *p = '\0';
      if (*lstart)
      {
      	if (i == MAXLINES)
      	  bailout("Too many lines in timeouts config file.", 1);
        if ((config[i] = (struct config_ent *) malloc(sizeof(struct config_ent))) == NULL)
          bailout("Out of memory", 1);
  	config[i]->times = NULL;
  	config[i]->ttys = NULL;
  	config[i]->users = NULL;
  	config[i]->groups = NULL;
  	config[i]->login_allowed = 1;
  	config[i]->idlemax = -1;
  	config[i]->sessmax = -1;
  	config[i]->daymax = -1;
  	config[i]->warntime = 5;
  	config[i]->messages[IDLEMSG] = NULL;
  	config[i]->messages[SESSMSG] = NULL;
  	config[i]->messages[DAYMSG] = NULL;
  	config[i]->messages[NOLOGINMSG] = NULL;
	if ((tok = strsep(&lstart, ":")) != NULL) store_times(&config[i]->times, tok);
	if ((tok = strsep(&lstart, ":")) != NULL) alloc_cp(&config[i]->ttys, tok);
	if ((tok = strsep(&lstart, ":")) != NULL) alloc_cp(&config[i]->users, tok);
	if ((tok = strsep(&lstart, ":")) != NULL) alloc_cp(&config[i]->groups, tok);
	tok = strsep(&lstart, ":");
	if (tok != NULL && !strncasecmp(tok, "NOLOGIN", 7))
	{
		config[i]->login_allowed=0;
		if (tok[7] == ';') alloc_cp(&config[i]->messages[NOLOGINMSG], tok+8);
		else if ((tok = strsep(&lstart, ":")) != NULL) alloc_cp(&config[i]->messages[NOLOGINMSG], tok);
	}
	else
	if (tok != NULL && !strcasecmp(tok, "LOGIN")) config[i]->login_allowed=1;
	else
	{
		if (tok != NULL)
		{
		    config[i]->idlemax = atoi(tok);
        if ((p = strchr(tok, ';'))) alloc_cp(&config[i]->messages[IDLEMSG], p+1);
		}
		if ((tok = strsep(&lstart, ":")) != NULL)
		{
		    config[i]->sessmax = atoi(tok);
        if ((p = strchr(tok, ';'))) alloc_cp(&config[i]->messages[SESSMSG], p+1);
	    	}
		if ((tok = strsep(&lstart, ":")) != NULL)
		{
		    config[i]->daymax = atoi(tok);
        if ((p = strchr(tok, ';'))) alloc_cp(&config[i]->messages[DAYMSG], p+1);
		}
		if ((tok = strsep(&lstart, ":")) != NULL)
		{
		    config[i]->warntime = atoi(tok);
		}
	}
	if (!config[i]->times || !config[i]->ttys  ||
	    !config[i]->users || !config[i]->groups)
        {
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        	syslog(LOG_ERR,
        		"Error on line %d of config file (%s).  Line ignored.",
        		linenum, CONFIG);
        	closelog();
	}
	else i++;
      }
    }
    config[i] = NULL;

    if (fclose(config_file) == EOF)
      bailout("Cannot close config file", 1);

#ifdef DEBUG
	i = 0;
	while (config[i])
	{
		printf("line %d: ", i);
		j = 0;
	  	while (config[i]->times[j].days)
		  printf("%d(%d-%d):", config[i]->times[j].days,
		  			config[i]->times[j].starttime,
		  			config[i]->times[j].endtime),j++;
		printf("%s:%s:%s:%s:%d;%s:%d;%s:%d;%s:%d\n",
			config[i]->ttys,
			config[i]->users,
			config[i]->groups,
			config[i]->login_allowed?"LOGIN":"NOLOGIN",
			config[i]->idlemax,
			config[i]->messages[IDLEMSG] == NULL?"builtin":config[i]->messages[IDLEMSG],
			config[i]->sessmax,
			config[i]->messages[SESSMSG] == NULL?"builtin":config[i]->messages[SESSMSG],
			config[i]->daymax,
			config[i]->messages[DAYMSG] == NULL?"builtin":config[i]->messages[DAYMSG],
			config[i]->warntime
            );
        i++;
	}
printf("End debug output.\n");
#endif /* DEBUG */
}

char chktimes(te)
struct time_ent *te;
{
    while (te->days)
    {
        if (daynums[now.tm_wday] & te->days &&	/* Date within range */
              ((te->starttime <= te->endtime && /* Time within range */
               now_hhmm >= te->starttime &&	
               now_hhmm <= te->endtime)
               ||
               (te->starttime > te->endtime &&
               (now_hhmm >= te->starttime ||
                now_hhmm <= te->endtime))
              )
           )
               return 1;
        te++;
    }
    return 0;
}

char chkmatch(element, in_set)
char *element;
char *in_set;
{
    char	*t;
    char	*set = (char *) malloc(strlen(in_set) + 1);

    if (set == NULL) bailout("Out of memory", 1);
    else strcpy(set, in_set);

    t = strtok(set, " ,");
    if(strlen(t) > strlen(in_set)) /* Avoid Buffer overflow */
    {
        printf("timeoutd.c:Line no-717: Buffer overflow\n");
        free(set);
        return 0;
    }
    while (t)
    {
        if (t[strlen(t)-1] == '*')
        {
            if (!strncmp(t, element, strlen(t) - 1))
            {
              free(set);
              return 1;
	    }
	}
        else if (!strcmp(t, element))
        {
          free(set);
          return 1;
	}
        t = strtok(NULL, " ,");
    }
    free(set);
    return 0;
}

/* Return the number of minutes which user has been logged in for on
 * any of the ttys specified in config[configline] during the current day.
 */

void get_day_time(user)
char *user;
{
    struct ut_list	*login_p;
    struct ut_list	*logout_p;
    struct ut_list	*prev_p;

    daytime = 0;
    login_p = wtmplist;
    while (login_p)
    {
        /* For each login on a matching tty find its logout */
        if (
#ifndef SUNOS
	    login_p->elem.ut_type == USER_PROCESS &&
#endif
            !strncmp(login_p->elem.ut_user, user, 8) &&
            chkmatch(login_p->elem.ut_line, config[configline]->ttys))
        {
#ifdef DEBUG_WTMP
    struct tm	*tm;
    tm = localtime(&(login_p->elem.ut_time));
    fprintf(stderr, "%d:%d %s %s %s\n", 
    	tm->tm_hour,tm->tm_min, login_p->elem.ut_line,
    	login_p->elem.ut_user,
    	"login");
#endif
            prev_p = logout_p = login_p->next;
            while (logout_p)
            {
/*
 * SA19931128
 * If there has been a crash, then be reasonably fair and use the
 * last recorded login/logout as the user's logout time.  This will
 * potentially allow them slightly more online time than usual,
 * but is better than marking them as logged in for the time the machine
 * was down.
 */
#ifndef SUNOS
                if (logout_p->elem.ut_type == BOOT_TIME)
                {
                      logout_p = prev_p;
                      break;
	        }
#endif
                if (/*logout_p->elem.ut_type == DEAD_PROCESS &&*/
                  !strncmp(login_p->elem.ut_line, logout_p->elem.ut_line,sizeof(login_p->elem.ut_line)))
                      break;
                prev_p = logout_p;
                logout_p = logout_p->next;
            }

#ifdef DEBUG_WTMP
    if (logout_p)
    {
      tm = localtime(&(logout_p->elem.ut_time));
      fprintf(stderr, "%d:%d %s %s %s\n", 
        (int)tm->tm_hour,tm->tm_min, logout_p->elem.ut_line,
    	logout_p->elem.ut_user, "logout");
      fprintf(stderr, "%s %d minutes\n", user, (int)((logout_p?logout_p->elem.ut_time:time_now) - login_p->elem.ut_time)/60);
            }
#endif
            daytime += (logout_p?logout_p->elem.ut_time:time_now) - login_p->elem.ut_time;
        }
        login_p = login_p->next;
    }
    daytime /= 60;
#ifdef DEBUG
fprintf(stderr, "%s has been logged in for %d minutes today.\n", user, daytime);
#endif
    return;
}

void warnpending(tty, time_remaining)
char *tty;
int time_remaining;
{
    FILE	*ttyf;

#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "Warning user on %s of pending logoff in %d minutes.",
    	tty, time_remaining);
    closelog();
#endif
    if ((ttyf = fopen(tty, "w")) == NULL)
    {
	openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        syslog(LOG_ERR, "Could not open %s to warn of impending logoff.\n",tty);
        closelog();
        return;
    }
    fprintf(ttyf, "\r\nWARNING:\r\nYou will be logged out in %d minute%s when your %s limit expires.\r\n",
            time_remaining, time_remaining==1?"":"s", limit_names[(int)limit_type]);
    fclose(ttyf);
}

char chk_timeout(user, dev, host, idle, session)
char *user;
char *dev;
char *host;
int idle;
int session;
{
    char	timematch = 0;
    char	ttymatch = 0;
    char	usermatch = 0;
#ifdef PWD_SUPPORT
    struct passwd   *pw;
    char	groupmatch = 0;
    char    **p;
    struct group    *gr;
    struct group    *secgr;
#endif
    char	*tty = dev + 5; /* Skip over the /dev/ */

    int		disc;

    configline = 0;
    if(0){
        host=host;
    }

#ifdef PWD_SUPPORT
/* Find primary group for specified user */
    if ((pw = getpwnam(user)) == NULL)
    {
      openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
      syslog(LOG_ERR, "Could not get password entry for %s.", user);
      closelog();
      return 0;
    }

    if ((gr = getgrgid(pw->pw_gid)) == NULL)
    {
      openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
      syslog(LOG_ERR, "Could not get group name for %s.", user);
      closelog();
      return 0;
    }
#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "Checking user %s group %s tty %s.", user, gr->gr_name, tty);
    closelog();
#endif
#endif

/* Check to see if current user matches any entry based on tty/user/group */
    while (config[configline])
    {
        timematch = chktimes(config[configline]->times);
        ttymatch = chkmatch(tty, config[configline]->ttys);
        usermatch = chkmatch(user, config[configline]->users);
#ifdef PWD_SUPPORT
        groupmatch = chkmatch(gr->gr_name, config[configline]->groups);

/* If the primary group doesn't match this entry, check secondaries */
	setgrent();
	while (!groupmatch && (secgr = getgrent()) != NULL)
	{
	    p = secgr->gr_mem;
	    while (*p && !groupmatch)
	    {
/*
printf("Group %s member %s\n", secgr->gr_name, *p);
*/
	    	if (!strcmp(*p, user))
	    	    groupmatch = chkmatch(secgr->gr_name, config[configline]->groups);
	    	p++;
	    }
/*
	    free(gr);
*/
	}
#endif

/*
	endgrent();
*/
/* If so, then check their idle, daily and session times in turn */
        if (timematch && ttymatch && usermatch
#ifdef PWD_SUPPORT
                && groupmatch
#endif
                )
        {
          get_day_time(user);
#ifdef DEBUG
	  openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
	  syslog(SYSLOG_DEBUG, "Matched entry %d", configline);
	  syslog(SYSLOG_DEBUG, "Idle=%d (max=%d) Sess=%d (max=%d) Daily=%d (max=%d) warntime=%d", idle, config[configline]->idlemax, session, config[configline]->sessmax, daytime, config[configline]->daymax, config[configline]->warntime);
	  closelog();
#endif
	  disc = getdisc(dev);

	  limit_type = NOLOGINMSG;
	  if (!config[configline]->login_allowed)
	  	return NOLOGIN;

	  limit_type = IDLEMSG;
	  if (disc == N_TTY && config[configline]->idlemax > 0 && idle >= config[configline]->idlemax)
	  	return IDLEMAX;

	  limit_type = SESSMSG;
	  if (config[configline]->sessmax > 0 && session >= config[configline]->sessmax)
	  	return SESSMAX;

	  limit_type = DAYMSG;
	  if (config[configline]->daymax > 0 && daytime >= config[configline]->daymax)
	  	return DAYMAX;

/* If none of those have been exceeded, then warn users of upcoming logouts */
	  limit_type = DAYMSG;
	  if (config[configline]->daymax > 0 && daytime >= config[configline]->daymax - config[configline]->warntime)
	  	warnpending(dev, config[configline]->daymax - daytime);
	  else
	  {
	    limit_type = SESSMSG;
	    if (config[configline]->sessmax > 0 && session >= config[configline]->sessmax - config[configline]->warntime)
		warnpending(dev, config[configline]->sessmax - session);
	  }

/* Otherwise, leave the poor net addict alone */
          return ACTIVE;
        }
        configline++;
    }

/* If they do not match any entries, then they can stay on forever */
    return ACTIVE;
}

void check_idle()    /* Check for exceeded time limits & logoff exceeders */
{
    char        user[12] = {0};
    char	host[17] = {0};
    struct stat status, *pstat;
    time_t      idle, sesstime, time();
    char AuditFile[100] = {0};
    FILE *fp = NULL;
    int ret = 0;

    pstat = &status;    /* point to status structure */
#ifndef SUNOS
    if (utmpp->ut_type != USER_PROCESS || !utmpp->ut_user[0]) /* if not user process */
        return;                      /* skip the utmp entry */
#endif
    ret = snprintf(user,sizeof(user),"%s", utmpp->ut_user);   /* get user name */
    if(ret < 0 || ret >= (signed)sizeof(user))
    {
	    printf("Buffer Overflow\n");
	    return;
    }
    ret = snprintf(host,sizeof(host),"%s", utmpp->ut_host);	/* get host name */
    if(ret < 0 || ret >= (signed)sizeof(host))
    {
	    printf("Buffer Overflow\n");
	    return;
    }
    ret = snprintf(dev,sizeof(dev),"/dev/%s",utmpp->ut_line);   /* append basename of port */
    if(ret < 0 || ret >= (signed)sizeof(dev))
    {
	    printf("Buffer Overflow\n");    
            return;
    }
    if (stat(dev, pstat))   /* if can't get status for port */
    {
#ifdef TELNET_WTMP_ISSUE
        return;
#else
        sprintf(errmsg, "Can't get status of user %s's terminal (%s)\n",
        	user, dev);
        bailout(errmsg, 1);
#endif
    }
    /* idle time is the lesser of:
     * current time less last access time OR
     * current time less last modified time
     */
#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "dev : %s, time now : %ld, atime : %ld mtime : %ld, ctime : %ld\n", 
		dev, time_now, pstat->st_atime, pstat->st_mtime, pstat->st_ctime);
    closelog();
#endif

    idle = (time_now - max(pstat->st_atime, pstat->st_mtime)) / 60;
    sesstime = (time_now - utmpp->ut_time) / 60;
    switch(chk_timeout(user, dev, host, idle, sesstime))
    {
    	case ACTIVE:
#ifdef DEBUG
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
		syslog(SYSLOG_DEBUG, "User %s is active.", user);
		closelog();
#endif
    		break;
    	case IDLEMAX:
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    		syslog(LOG_NOTICE,
    		       "User %s exceeded idle limit (idle for %d minutes, max=%d).\n",
                   user, (int)idle, config[configline]->idlemax);
    		closelog();
    		sprintf (AuditFile, "%s","/tmp/audit/autoLogoutPIDEntryTable");
    		struct stat stbuf;
    		if (stat(AuditFile, &stbuf) == 0)
    		{
    			fp = fopen (AuditFile, "a+");
    			if (NULL != fp)
    			{
    				fprintf(fp,"%d\n",utmpp->ut_pid);
    				fclose(fp);
    			}
    		}
    		else
    		{
    			fp = fopen(AuditFile,"w");
    			if(NULL!=fp){
    				if(-1 == chmod(AuditFile,S_IRWXU|S_IRGRP|S_IRWXO)){
    					openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    					syslog(LOG_NOTICE,
    							"User %s failed to chmod file /tmp/audit/autoLogoutPIDEntryTable.\n",
    							user);
    					closelog();
    				}
    				fprintf(fp,"%d\n",utmpp->ut_pid);
    				fclose(fp);
    			}
    		}
		killit(utmpp->ut_pid, user, dev);
    		break;
    	case SESSMAX:
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
		syslog(LOG_NOTICE,
		       "User %s exceeded maximum session limit (on for %d minutes, max=%d).\n",
                    user, (int)sesstime, config[configline]->sessmax);
    		closelog();
		killit(utmpp->ut_pid, user, dev);
    		break;
    	case DAYMAX:
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
		syslog(LOG_NOTICE,
		       "User %s exceeded maximum daily limit (on for %d minutes, max=%d).\n",
		       user, daytime, config[configline]->daymax);
    		closelog();
		killit(utmpp->ut_pid, user, dev);
    		break;
	case NOLOGIN:
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
		syslog(LOG_NOTICE, "NOLOGIN period reached for user %s.", user);
		closelog();
		killit(utmpp->ut_pid, user, dev);
		break;
	default:
		openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
            syslog(LOG_ERR, "Internal error - unexpected return from chk_timeout");
    		closelog();
    }
}

void bailout(message, status) /* display error message and exit */
int     status;     /* exit status */
char    *message;   /* pointer to the error message */
{
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(LOG_ERR, "Exiting - %s", message);
    closelog();
    exit(status);
}

void shutdown(signum)
int signum;
{
    if(0){
        signum=signum;
    }
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(LOG_NOTICE, "Received SIGTERM.. exiting.");
    closelog();
    exit(0);
}

void logoff_msg(tty)
int tty;
{
    FILE	*msgfile = NULL;
    char	msgbuf[1024];
    int		cnt;

    if (config[configline]->messages[(int)limit_type])
        msgfile = fopen(config[configline]->messages[(int)limit_type], "r");

    if (msgfile)
    {
    	while ((cnt = read(tty, msgbuf, 1024)) > 0)
    	    if(write(tty, msgbuf, cnt) < 0)
                printf("write failed in tty\n");
    	fclose(msgfile);
    }
    else
    {
        if (limit_type == NOLOGINMSG)
            snprintf(msgbuf,sizeof(msgbuf), "\r\n\r\nLogins not allowed at this time.  Please try again later.\r\n");
        else
            snprintf(msgbuf,sizeof(msgbuf), "\r\n\r\nYou have exceeded your %s time limit.  Logging you off now.\r\n\r\n", limit_names[(int)limit_type]);
        if(write(tty, msgbuf, strlen(msgbuf)) < 0)
            printf("write failed in tty\n");
    }
}

/* terminate process using SIGHUP, then SIGKILL */
void killit(pid, user, dev)
int pid;
char *user;
char *dev;
{
    int	tty;
#ifdef SUNOS
   struct passwd	*pw;
#endif

/* Tell user which limit they have exceeded and that they will be logged off */
    if ((tty = open(dev, O_WRONLY|O_NOCTTY|O_NONBLOCK)) < 0)
    {
	openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        syslog(LOG_ERR, "Could not write logoff message to %s.", dev);
        closelog();
	return;
    }

/*
    if (!strcmp(limit_name, s_nologin))
    	fprintf(tty, "\r\n\r\nLogins not allowed at this time.  Please try again later.\r\n");
    else
    	fprintf(tty, "\r\n\r\nYou have exceeded your %s time limit.  Logging you off now.\r\n\r\n",
    		limit_name);
 */
    logoff_msg(tty);
    close(tty);

#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(LOG_NOTICE, "Would normally kill pid %d user %s on %s",pid,user,dev);
    closelog();
#endif

    if (fork())             /* the parent process */
        return;            /* returns */

/* Wait a little while in case the above message gets lost during logout */
#ifdef SURE_KILL
    signal(SIGHUP, SIG_IGN);
    if ((pw = getpwnam(user)) == NULL)
    {
	openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        syslog(LOG_ERR, "Could not log user %s off line %s - unable to determine uid.", user, dev);
        closelog();
    }
    if (setuid(pw->pw_uid))
    {
	openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        syslog(LOG_ERR, "Could not log user %s off line %s - unable to setuid(%d).", user, dev, pw->pw_uid);
        closelog();
    }
    kill(-1, SIGHUP);
    sleep(KWAIT);
    kill(-1, SIGKILL);
#else

    kill(pid, SIGHUP);  /* first send "hangup" signal */
    sleep(KWAIT);
    if (!kill(pid, 0)) {    /* SIGHUP might be ignored */
        
        kill(pid, SIGKILL); /* then send sure "kill" signal */
        sleep(KWAIT);
        if (!kill(pid, 0))
        {
	    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
            syslog(LOG_ERR, "Could not log user %s off line %s.", user, dev);
            closelog();
        }
    }
#endif
    exit(0);
}

void reread_config(signum)
int signum;
{
    int i = 0;

    if(0){
        signum=signum;
    }
    if (!allow_reread)
        pending_reread = 1;
    else
    {
        pending_reread = 0;
	openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
        syslog(LOG_NOTICE, "Re-reading configuration file.");
        closelog();
        while (config[i])
        {
            free(config[i]->times);
            free(config[i]->ttys);
            free(config[i]->users);
            free(config[i]->groups);
            if (config[i]->messages[IDLEMSG]) free(config[i]->messages[IDLEMSG]);
            if (config[i]->messages[DAYMSG]) free(config[i]->messages[DAYMSG]);
            if (config[i]->messages[SESSMSG]) free(config[i]->messages[SESSMSG]);
            if (config[i]->messages[NOLOGINMSG]) free(config[i]->messages[NOLOGINMSG]);
            free(config[i]);
            i++;
        }
        read_config();
    }
    signal(SIGHUP, reread_config);
}

void reapchild(signum)
int signum;
{
    int st;

    if(0){
        signum=signum;
    }
    wait(&st);
    signal(SIGCHLD, reapchild);
}

int getdisc(d)
char *d;
{
    int	fd;
    int	disc;

#ifdef linux
    if ((fd = open(d, O_RDONLY|O_NONBLOCK|O_NOCTTY)) < 0)
    {
      openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
      syslog(LOG_WARNING, "Could not open %s for checking line discipline - idle limits will be enforced.", d);
      closelog();
        return -1;
    }

    if (ioctl(fd, TIOCGETD, &disc) < 0)
    {
      close(fd);
      openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
      syslog(LOG_WARNING, "Could not get line discipline for %s - idle limits will be enforced.", d);
      closelog();
        return -1;
    }

    close(fd);
#ifdef DEBUG
    openlog("timeoutd", OPENLOG_FLAGS, LOG_DAEMON);
    syslog(SYSLOG_DEBUG, "TTY %s: Discipline=%s.",d,disc==N_SLIP?"SLIP":disc==N_TTY?"TTY":disc==N_PPP?"PPP":disc==N_MOUSE?"MOUSE":"UNKNOWN");
    closelog();
#endif

    return disc;
#else
    return N_TTY;
#endif
}


