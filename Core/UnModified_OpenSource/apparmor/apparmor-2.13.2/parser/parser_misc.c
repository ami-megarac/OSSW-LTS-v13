/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
 *
 *   Copyright (c) 2013
 *   Canonical Ltd. (All rights reserved)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, contact Novell, Inc.
 */

/* assistance routines */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <linux/capability.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/apparmor.h>
#include <sys/apparmor_private.h>

#include "lib.h"
#include "parser.h"
#include "profile.h"
#include "parser_yacc.h"
#include "mount.h"
#include "dbus.h"

/* #define DEBUG */
#ifdef DEBUG
#undef PDEBUG
#define PDEBUG(fmt, args...) fprintf(stderr, "Lexer: " fmt, ## args)
#else
#undef PDEBUG
#define PDEBUG(fmt, args...)	/* Do nothing */
#endif
#define NPDEBUG(fmt, args...)	/* Do nothing */

int is_blacklisted(const char *name, const char *path)
{
	int retval = _aa_is_blacklisted(name);

	if (retval == -1)
		PERROR("Ignoring: '%s'\n", path ? path : name);

	return !retval ? 0 : 1;
}

struct keyword_table {
	const char *keyword;
	int token;
};

static struct keyword_table keyword_table[] = {
	/* network */
	{"network",		TOK_NETWORK},
	{"unix",		TOK_UNIX},
	/* misc keywords */
	{"capability",		TOK_CAPABILITY},
	{"if",			TOK_IF},
	{"else",		TOK_ELSE},
	{"not",			TOK_NOT},
	{"defined",		TOK_DEFINED},
	{"change_profile",	TOK_CHANGE_PROFILE},
	{"unsafe",		TOK_UNSAFE},
	{"safe",		TOK_SAFE},
	{"link",		TOK_LINK},
	{"owner",		TOK_OWNER},
	{"user",		TOK_OWNER},
	{"other",		TOK_OTHER},
	{"subset",		TOK_SUBSET},
	{"audit",		TOK_AUDIT},
	{"deny",		TOK_DENY},
	{"allow",		TOK_ALLOW},
	{"set",			TOK_SET},
	{"rlimit",		TOK_RLIMIT},
	{"alias",		TOK_ALIAS},
	{"rewrite",		TOK_ALIAS},
	{"ptrace",		TOK_PTRACE},
	{"file",		TOK_FILE},
	{"mount",		TOK_MOUNT},
	{"remount",		TOK_REMOUNT},
	{"umount",		TOK_UMOUNT},
	{"unmount",		TOK_UMOUNT},
	{"pivot_root",		TOK_PIVOTROOT},
	{"in",			TOK_IN},
	{"dbus",		TOK_DBUS},
	{"signal",		TOK_SIGNAL},
	{"send",                TOK_SEND},
	{"receive",             TOK_RECEIVE},
	{"bind",                TOK_BIND},
	{"read",                TOK_READ},
	{"write",               TOK_WRITE},
	{"eavesdrop",		TOK_EAVESDROP},
	{"peer",		TOK_PEER},
	{"trace",		TOK_TRACE},
	{"tracedby",		TOK_TRACEDBY},
	{"readby",		TOK_READBY},
	{"abi",			TOK_ABI},

	/* terminate */
	{NULL, 0}
};

static struct keyword_table rlimit_table[] = {
	{"cpu",			RLIMIT_CPU},
	{"fsize",		RLIMIT_FSIZE},
	{"data",		RLIMIT_DATA},
	{"stack",		RLIMIT_STACK},
	{"core",		RLIMIT_CORE},
	{"rss",			RLIMIT_RSS},
	{"nofile",		RLIMIT_NOFILE},
#ifdef RLIMIT_OFILE
	{"ofile",		RLIMIT_OFILE},
#endif
	{"as",			RLIMIT_AS},
	{"nproc",		RLIMIT_NPROC},
	{"memlock",		RLIMIT_MEMLOCK},
	{"locks",		RLIMIT_LOCKS},
	{"sigpending",		RLIMIT_SIGPENDING},
	{"msgqueue",		RLIMIT_MSGQUEUE},
#ifdef RLIMIT_NICE
	{"nice",		RLIMIT_NICE},
#endif
#ifdef RLIMIT_RTPRIO
	{"rtprio",		RLIMIT_RTPRIO},
#endif
#ifdef RLIMIT_RTTIME
	{"rttime",		RLIMIT_RTTIME},
#endif
	/* terminate */
	{NULL, 0}
};

/* for alpha matches, check for keywords */
static int get_table_token(const char *name unused, struct keyword_table *table,
			   const char *keyword)
{
	int i;

	for (i = 0; table[i].keyword; i++) {
		PDEBUG("Checking %s %s\n", name, table[i].keyword);
		if (strcmp(keyword, table[i].keyword) == 0) {
			PDEBUG("Found %s %s\n", name, table[i].keyword);
			return table[i].token;
		}
	}

	PDEBUG("Unable to find %s %s\n", name, keyword);
	return -1;
}

static struct keyword_table capability_table[] = {
	/* capabilities */
	#include "cap_names.h"
#ifndef CAP_SYSLOG
	{"syslog", 34},
#endif
	/* terminate */
	{NULL, 0}
};

/* for alpha matches, check for keywords */
int get_keyword_token(const char *keyword)
{
	return get_table_token("keyword", keyword_table, keyword);
}

int name_to_capability(const char *keyword)
{
	return get_table_token("capability", capability_table, keyword);
}

int get_rlimit(const char *name)
{
	return get_table_token("rlimit", rlimit_table, name);
}

char *processunquoted(const char *string, int len)
{
	char *buffer, *s;

	s = buffer = (char *) malloc(len + 1);
	if (!buffer)
		return NULL;

	while (len > 0) {
		const char *pos = string + 1;
		long c;
		if (*string == '\\' && len > 1 &&
		    (c = strn_escseq(&pos, "", len)) != -1) {
			/* catch \\ or \134 and other aare special chars and
			 * pass it through to be handled by the backend
			 * pcre conversion
			 */
			if (c == 0) {
				strncpy(s, string, pos - string);
				s += pos - string;
			} else if (strchr("*?[]{}^,\\", c) != NULL) {
				*s++ = '\\';
				*s++ = c;
			} else
				*s++ = c;
			len -= pos - string;
			string = pos;
		} else {
			/* either unescaped char OR
			 * unsupported escape sequence resulting in char being
			 * copied.
			 */
			*s++ = *string++;
			len--;
		}
	}
	*s = 0;

	return buffer;
}

/* rewrite a quoted string substituting escaped characters for the
 * real thing.  Strip the quotes around the string */
char *processquoted(const char *string, int len)
{
	/* skip leading " and eat trailing " */
	if (*string == '"') {
		len -= 2;
		if (len < 0)	/* start and end point to same quote */
			len = 0;
		return processunquoted(string + 1, len);
	}

	/* no quotes? treat as unquoted */
	return processunquoted(string, len);
}

char *processid(const char *string, int len)
{
	/* lexer should never call this fn if len <= 0 */
	assert(len > 0);

	if (*string == '"')
		return processquoted(string, len);
	return processunquoted(string, len);
}

/* strip off surrounding delimiters around variables */
char *process_var(const char *var)
{
	const char *orig = var;
	int len = strlen(var);

	if (*orig == '@' || *orig == '$') {
		orig++;
		len--;
	} else {
		PERROR("ASSERT: Found var '%s' without variable prefix\n",
		       var);
		return NULL;
	}

	if (*orig == '{') {
		orig++;
		len--;
		if (orig[len - 1] != '}') {
			PERROR("ASSERT: No matching '}' in variable '%s'\n",
		       		var);
			return NULL;
		} else
			len--;
	}

	return strndup(orig, len);
}

/* returns -1 if value != true or false, otherwise 0 == false, 1 == true */
int str_to_boolean(const char *value)
{
	int retval = -1;

	if (strcasecmp("TRUE", value) == 0)
		retval = 1;
	if (strcasecmp("FALSE", value) == 0)
		retval = 0;
	return retval;
}

static int warned_uppercase = 0;

void warn_uppercase(void)
{
	if (!warned_uppercase) {
		pwarn(_("Uppercase qualifiers \"RWLIMX\" are deprecated, please convert to lowercase\n"
			"See the apparmor.d(5) manpage for details.\n"));
		warned_uppercase = 1;
	}
}

static int parse_sub_mode(const char *str_mode, const char *mode_desc unused)
{

#define IS_DIFF_QUAL(mode, q) (((mode) & AA_MAY_EXEC) && (((mode) & AA_EXEC_TYPE) != ((q) & AA_EXEC_TYPE)))

	int mode = 0;
	const char *p;

	PDEBUG("Parsing mode: %s\n", str_mode);

	if (!str_mode)
		return 0;

	p = str_mode;
	while (*p) {
		char thisc = *p;
		char next = *(p + 1);
		char lower;
		int tmode = 0;

reeval:
		switch (thisc) {
		case COD_READ_CHAR:
			if (read_implies_exec) {
				PDEBUG("Parsing mode: found %s READ imply X\n", mode_desc);
				mode |= AA_MAY_READ | AA_OLD_EXEC_MMAP;
			} else {
				PDEBUG("Parsing mode: found %s READ\n", mode_desc);
				mode |= AA_MAY_READ;
			}
			break;

		case COD_WRITE_CHAR:
			PDEBUG("Parsing mode: found %s WRITE\n", mode_desc);
			if ((mode & AA_MAY_APPEND) && !(mode & AA_MAY_WRITE))
				yyerror(_("Conflict 'a' and 'w' perms are mutually exclusive."));
			mode |= AA_MAY_WRITE | AA_MAY_APPEND;
			break;

		case COD_APPEND_CHAR:
			PDEBUG("Parsing mode: found %s APPEND\n", mode_desc);
			if (mode & AA_MAY_WRITE)
				yyerror(_("Conflict 'a' and 'w' perms are mutually exclusive."));
			mode |= AA_MAY_APPEND;
			break;

		case COD_LINK_CHAR:
			PDEBUG("Parsing mode: found %s LINK\n", mode_desc);
			mode |= AA_OLD_MAY_LINK;
			break;

		case COD_LOCK_CHAR:
			PDEBUG("Parsing mode: found %s LOCK\n", mode_desc);
			mode |= AA_OLD_MAY_LOCK;
			break;

		case COD_INHERIT_CHAR:
			PDEBUG("Parsing mode: found INHERIT\n");
			if (mode & AA_EXEC_MODIFIERS) {
				yyerror(_("Exec qualifier 'i' invalid, conflicting qualifier already specified"));
			} else {
				if (next != tolower(next))
					warn_uppercase();
				mode |= (AA_EXEC_INHERIT | AA_MAY_EXEC);
				p++;	/* skip 'x' */
			}
			break;

		case COD_UNSAFE_UNCONFINED_CHAR:
			tmode = AA_EXEC_UNSAFE;
			pwarn(_("Unconfined exec qualifier (%c%c) allows some dangerous environment variables "
				"to be passed to the unconfined process; 'man 5 apparmor.d' for details.\n"),
			      COD_UNSAFE_UNCONFINED_CHAR, COD_EXEC_CHAR);
			/* fall through */
		case COD_UNCONFINED_CHAR:
			tmode |= AA_EXEC_UNCONFINED | AA_MAY_EXEC;
			PDEBUG("Parsing mode: found UNCONFINED\n");
			if (IS_DIFF_QUAL(mode, tmode)) {
				yyerror(_("Exec qualifier '%c' invalid, conflicting qualifier already specified"),
					thisc);
			} else {
				if (next != tolower(next))
					warn_uppercase();
				mode |=  tmode;
				p++;	/* skip 'x' */
			}
			tmode = 0;
			break;

		case COD_UNSAFE_PROFILE_CHAR:
		case COD_UNSAFE_LOCAL_CHAR:
			tmode = AA_EXEC_UNSAFE;
			/* fall through */
		case COD_PROFILE_CHAR:
		case COD_LOCAL_CHAR:
			if (tolower(thisc) == COD_UNSAFE_PROFILE_CHAR)
				tmode |= AA_EXEC_PROFILE | AA_MAY_EXEC;
			else
			{
				tmode |= AA_EXEC_LOCAL | AA_MAY_EXEC;
			}
			PDEBUG("Parsing mode: found PROFILE\n");
			if (tolower(next) == COD_INHERIT_CHAR) {
				tmode |= AA_EXEC_INHERIT;
				if (IS_DIFF_QUAL(mode, tmode)) {
					yyerror(_("Exec qualifier '%c%c' invalid, conflicting qualifier already specified"), thisc, next);
				} else {
					mode |= tmode;
					p += 2;		/* skip x */
				}
			} else if (tolower(next) == COD_UNSAFE_UNCONFINED_CHAR) {
				tmode |= AA_EXEC_PUX;
				if (IS_DIFF_QUAL(mode, tmode)) {
					yyerror(_("Exec qualifier '%c%c' invalid, conflicting qualifier already specified"), thisc, next);
				} else {
					mode |= tmode;
					p += 2;		/* skip x */
				}
			} else if (IS_DIFF_QUAL(mode, tmode)) {
				yyerror(_("Exec qualifier '%c' invalid, conflicting qualifier already specified"), thisc);

			} else {
				if (next != tolower(next))
					warn_uppercase();
				mode |= tmode;
				p++;	/* skip 'x' */
			}
			tmode = 0;
			break;

		case COD_MMAP_CHAR:
			PDEBUG("Parsing mode: found %s MMAP\n", mode_desc);
			mode |= AA_OLD_EXEC_MMAP;
			break;

		case COD_EXEC_CHAR:
			/* thisc is valid for deny rules, and named transitions
			 * but invalid for regular x transitions
			 * sort it out later.
			 */
			mode |= AA_MAY_EXEC;
			break;

 		/* error cases */

		default:
			lower = tolower(thisc);
			switch (lower) {
			case COD_READ_CHAR:
			case COD_WRITE_CHAR:
			case COD_APPEND_CHAR:
			case COD_LINK_CHAR:
			case COD_INHERIT_CHAR:
			case COD_MMAP_CHAR:
			case COD_EXEC_CHAR:
				PDEBUG("Parsing mode: found invalid upper case char %c\n", thisc);
				warn_uppercase();
				thisc = lower;
				goto reeval;
				break;
			default:
				yyerror(_("Internal: unexpected mode character '%c' in input"),
					thisc);
				break;
			}
			break;
		}

		p++;
	}

	PDEBUG("Parsed mode: %s 0x%x\n", str_mode, mode);

	return mode;
}

int parse_mode(const char *str_mode)
{
	int tmp, mode = 0;
	tmp = parse_sub_mode(str_mode, "");
	mode = SHIFT_MODE(tmp, AA_USER_SHIFT);
	mode |= SHIFT_MODE(tmp, AA_OTHER_SHIFT);
	if (mode & ~AA_VALID_PERMS)
		yyerror(_("Internal error generated invalid perm 0x%llx\n"), mode);
	return mode;
}

static int parse_X_sub_mode(const char *X, const char *str_mode, int *result, int fail, const char *mode_desc unused)
{
	int mode = 0;
	const char *p;

	PDEBUG("Parsing %s mode: %s\n", X, str_mode);

	if (!str_mode)
		return 0;

	p = str_mode;
	while (*p) {
		char current = *p;
		char lower;

reeval:
		switch (current) {
		case COD_READ_CHAR:
			PDEBUG("Parsing %s mode: found %s READ\n", X, mode_desc);
			mode |= AA_DBUS_RECEIVE;
			break;

		case COD_WRITE_CHAR:
			PDEBUG("Parsing %s mode: found %s WRITE\n", X,
			       mode_desc);
			mode |= AA_DBUS_SEND;
			break;

		/* error cases */

		default:
			lower = tolower(current);
			switch (lower) {
			case COD_READ_CHAR:
			case COD_WRITE_CHAR:
				PDEBUG("Parsing %s mode: found invalid upper case char %c\n",
				       X, current);
				warn_uppercase();
				current = lower;
				goto reeval;
				break;
			default:
				if (fail)
					yyerror(_("Internal: unexpected %s mode character '%c' in input"),
						X, current);
				else
					return 0;
				break;
			}
			break;
		}
		p++;
	}

	PDEBUG("Parsed %s mode: %s 0x%x\n", X, str_mode, mode);

	*result = mode;
	return 1;
}

int parse_X_mode(const char *X, int valid, const char *str_mode, int *mode, int fail)
{
	*mode = 0;
	if (!parse_X_sub_mode(X, str_mode, mode, fail, ""))
		return 0;
	if (*mode & ~valid) {
		if (fail)
			yyerror(_("Internal error generated invalid %s perm 0x%x\n"),
				X, mode);
		else
			return 0;
	}
	return 1;
}

/**
 * parse_label - break a label down to the namespace and profile name
 * @stack: Will be true if the first char in @label is '&' to indicate stacking
 * @ns: Will point to the first char in the namespace upon return or NULL
 *      if no namespace is present
 * @ns_len: Number of chars in the namespace string or 0 if no namespace
 *          is present
 * @name: Will point to the first char in the profile name upon return
 * @name_len: Number of chars in the name string
 * @label: The label to parse into namespace and profile name
 *
 * The returned pointers will point to locations within the original
 * @label string. No new strings are allocated.
 *
 * Returns 0 upon success or non-zero with @ns, @ns_len, @name, and
 * @name_len undefined upon error. Error codes are:
 *
 * 1) Namespace is not terminated despite @label starting with ':'
 * 2) Namespace is empty meaning @label starts with "::"
 * 3) Profile name is empty
 */
static int _parse_label(bool *stack,
			char **ns, size_t *ns_len,
			char **name, size_t *name_len,
			const char *label)
{
	const char *name_start = NULL;
	const char *ns_start = NULL;
	const char *ns_end = NULL;

	if (label[0] == '&') {
		/**
		 * Leading ampersand means that the current profile should
		 * be stacked with the rest of the label
		 */
		*stack = true;
		label++;
	} else {
		*stack = false;
	}

	if (label[0] != ':') {
		/* There is no namespace specified in the label */
		name_start = label;
	} else {
		/* A leading ':' indicates that a namespace is specified */
		ns_start = label + 1;
		ns_end = strstr(ns_start, ":");

		if (!ns_end)
			return 1;

		/**
		 * Handle either of the two namespace formats:
		 *  1) :ns:name
		 *  2) :ns://name
		 */
		name_start = ns_end + 1;
		if (!strncmp(name_start, "//", 2))
			name_start += 2;
	}

	/**
	 * The casts below are to allow @label to be const, signifying
	 * that this function doesn't modify it, while allowing callers to
	 * decide if they want to pass in pointers to const or non-const
	 * strings.
	 */
	*ns = (char *)ns_start;
	*name = (char *)name_start;
	*ns_len = ns_end - ns_start;
	*name_len = strlen(name_start);

	if (*ns && *ns_len == 0)
		return 2;
	else if (*name_len == 0)
		return 3;

	return 0;
}

bool label_contains_ns(const char *label)
{
	bool stack = false;
	char *ns = NULL;
	char *name = NULL;
	size_t ns_len = 0;
	size_t name_len = 0;

	return _parse_label(&stack, &ns, &ns_len, &name, &name_len, label) == 0 && ns;
}

bool parse_label(bool *_stack, char **_ns, char **_name,
		 const char *label, bool yyerr)
{
	const char *err = NULL;
	char *ns = NULL;
	char *name = NULL;
	size_t ns_len = 0;
	size_t name_len = 0;
	int res;

	res = _parse_label(_stack, &ns, &ns_len, &name, &name_len, label);
	if (res == 1) {
		err = _("Namespace not terminated: %s\n");
	} else if (res == 2) {
		err = _("Empty namespace: %s\n");
	} else if (res == 3) {
		err = _("Empty named transition profile name: %s\n");
	} else if (res != 0) {
		err = _("Unknown error while parsing label: %s\n");
	}

	if (err) {
		if (yyerr)
			yyerror(err, label);
		else
			fprintf(stderr, err, label);

		return false;
	}

	if (ns) {
		*_ns = strndup(ns, ns_len);
		if (!*_ns)
			goto alloc_fail;
	} else {
		*_ns = NULL;
	}

	*_name = strndup(name, name_len);
	if (!*_name) {
		free(*_ns);
		goto alloc_fail;
	}

	return true;

alloc_fail:
	err = _("Memory allocation error.");
	if (yyerr)
		yyerror(err);
	else
		fprintf(stderr, "%s", err);

	return false;
}

struct cod_entry *new_entry(char *id, int mode, char *link_id)
{
	struct cod_entry *entry = NULL;

	entry = (struct cod_entry *)calloc(1, sizeof(struct cod_entry));
	if (!entry)
		return NULL;

	entry->name = id;
	entry->link_name = link_id;
	entry->mode = mode;
	entry->audit = 0;
	entry->deny = FALSE;

	entry->pattern_type = ePatternInvalid;
	entry->pat.regex = NULL;

	entry->next = NULL;

	PDEBUG(" Insertion of: (%s)\n", entry->name);
	return entry;
}

struct cod_entry *copy_cod_entry(struct cod_entry *orig)
{
	struct cod_entry *entry = NULL;

	entry = (struct cod_entry *)calloc(1, sizeof(struct cod_entry));
	if (!entry)
		return NULL;

	DUP_STRING(orig, entry, name, err);
	DUP_STRING(orig, entry, link_name, err);
	DUP_STRING(orig, entry, nt_name, err);
	entry->mode = orig->mode;
	entry->audit = orig->audit;
	entry->deny = orig->deny;

	/* XXX - need to create copies of the patterns, too */
	entry->pattern_type = orig->pattern_type;
	entry->pat.regex = NULL;

	entry->next = orig->next;

	return entry;

err:
	free_cod_entries(entry);
	return NULL;
}

void free_cod_entries(struct cod_entry *list)
{
	if (!list)
		return;
	if (list->next)
		free_cod_entries(list->next);
	if (list->name)
		free(list->name);
	if (list->link_name)
		free(list->link_name);
	if (list->nt_name)
		free(list->nt_name);
	if (list->pat.regex)
		free(list->pat.regex);
	free(list);
}

static void debug_base_perm_mask(int mask)
{
	if (HAS_MAY_READ(mask))
		printf("%c", COD_READ_CHAR);
	if (HAS_MAY_WRITE(mask))
		printf("%c", COD_WRITE_CHAR);
	if (HAS_MAY_APPEND(mask))
		printf("%c", COD_APPEND_CHAR);
	if (HAS_MAY_LINK(mask))
		printf("%c", COD_LINK_CHAR);
	if (HAS_MAY_LOCK(mask))
		printf("%c", COD_LOCK_CHAR);
	if (HAS_EXEC_MMAP(mask))
		printf("%c", COD_MMAP_CHAR);
	if (HAS_MAY_EXEC(mask))
		printf("%c", COD_EXEC_CHAR);
}

void debug_cod_entries(struct cod_entry *list)
{
	struct cod_entry *item = NULL;

	printf("--- Entries ---\n");

	list_for_each(list, item) {
		printf("Mode:\t");
		if (HAS_CHANGE_PROFILE(item->mode))
			printf(" change_profile");
		if (HAS_EXEC_UNSAFE(item->mode))
			printf(" unsafe");
		debug_base_perm_mask(SHIFT_TO_BASE(item->mode, AA_USER_SHIFT));
		printf(":");
		debug_base_perm_mask(SHIFT_TO_BASE(item->mode, AA_OTHER_SHIFT));
		if (item->name)
			printf("\tName:\t(%s)\n", item->name);
		else
			printf("\tName:\tNULL\n");

		if (AA_LINK_BITS & item->mode)
			printf("\tlink:\t(%s)\n", item->link_name ? item->link_name : "/**");

	}
}


static const char *capnames[] = {
	"chown",
	"dac_override",
	"dac_read_search",
	"fowner",
	"fsetid",
	"kill",
	"setgid",
	"setuid",
	"setpcap",
	"linux_immutable",
	"net_bind_service",
	"net_broadcast",
	"net_admin",
	"net_raw",
	"ipc_lock",
	"ipc_owner",
	"sys_module",
	"sys_rawio",
	"sys_chroot",
	"sys_ptrace",
	"sys_pacct",
	"sys_admin",
	"sys_boot",
	"sys_nice",
	"sys_resource",
	"sys_time",
	"sys_tty_config",
	"mknod",
	"lease",
	"audit_write",
	"audit_control",
	"setfcap",
	"mac_override",
	"syslog",
};

const char *capability_to_name(unsigned int cap)
{
	const char *capname;

	capname = (cap < (sizeof(capnames) / sizeof(char *))
		   ? capnames[cap] : "invalid-capability");

	return capname;
}

void __debug_capabilities(uint64_t capset, const char *name)
{
	unsigned int i;

	printf("%s:", name);
	for (i = 0; i < (sizeof(capnames)/sizeof(char *)); i++) {
		if (((1ull << i) & capset) != 0) {
			printf (" %s", capability_to_name(i));
		}
	}
	printf("\n");
}

struct value_list *new_value_list(char *value)
{
	struct value_list *val = (struct value_list *) calloc(1, sizeof(struct value_list));
	if (val)
		val->value = value;
	return val;
}

void free_value_list(struct value_list *list)
{
	struct value_list *next;

	while (list) {
		next = list->next;
		if (list->value)
			free(list->value);
		free(list);
		list = next;
	}
}

struct value_list *dup_value_list(struct value_list *list)
{
	struct value_list *entry, *dup, *head = NULL;
	char *value;

	list_for_each(list, entry) {
		value = NULL;
		if (list->value) {
			value = strdup(list->value);
			if (!value)
				goto fail2;
		}
		dup = new_value_list(value);
		if (!dup)
			goto fail;
		if (head)
			list_append(head, dup);
		else
			head = dup;
	}

	return head;

fail:
	free(value);
fail2:
	free_value_list(head);

	return NULL;
}

void print_value_list(struct value_list *list)
{
	struct value_list *entry;

	if (!list)
		return;

	fprintf(stderr, "%s", list->value);
	list = list->next;
	list_for_each(list, entry) {
		fprintf(stderr, ", %s", entry->value);
	}
}

void move_conditional_value(const char *rulename, char **dst_ptr,
			    struct cond_entry *cond_ent)
{
	if (*dst_ptr)
		yyerror("%s conditional \"%s\" can only be specified once\n",
			rulename, cond_ent->name);

	*dst_ptr = cond_ent->vals->value;
	cond_ent->vals->value = NULL;
}

struct cond_entry *new_cond_entry(char *name, int eq, struct value_list *list)
{
	struct cond_entry *ent = (struct cond_entry *) calloc(1, sizeof(struct cond_entry));
	if (ent) {
		ent->name = name;
		ent->vals = list;
		ent->eq = eq;
	}

	return ent;
}

void free_cond_entry(struct cond_entry *ent)
{
	if (ent) {
		free(ent->name);
		free_value_list(ent->vals);
		free(ent);
	}
}

void free_cond_list(struct cond_entry *ents)
{
	struct cond_entry *entry, *tmp;

	list_for_each_safe(ents, entry, tmp) {
		free_cond_entry(entry);
	}
}

void print_cond_entry(struct cond_entry *ent)
{
	if (ent) {
		fprintf(stderr, "%s=(", ent->name);
		print_value_list(ent->vals);
		fprintf(stderr, ")\n");
	}
}


struct time_units {
	const char *str;
	long long value;
};

static struct time_units time_units[] = {
	{ "us", 1LL },
	{ "microsecond", 1LL },
	{ "microseconds", 1LL },
	{ "ms", 1000LL },
	{ "millisecond", 1000LL },
	{ "milliseconds", 1000LL },
	{ "s", 1000LL * 1000LL },
	{ "sec", SECONDS_P_MS },
	{ "second", SECONDS_P_MS },
	{ "seconds", SECONDS_P_MS },
	{ "min" , 60LL * SECONDS_P_MS },
	{ "minute", 60LL * SECONDS_P_MS },
	{ "minutes", 60LL * SECONDS_P_MS },
	{ "h", 60LL * 60LL * SECONDS_P_MS },
	{ "hour", 60LL * 60LL * SECONDS_P_MS },
	{ "hours", 60LL * 60LL * SECONDS_P_MS },
	{ "d", 24LL * 60LL * 60LL * SECONDS_P_MS },
	{ "day", 24LL * 60LL * 60LL * SECONDS_P_MS },
	{ "days", 24LL * 60LL * 60LL * SECONDS_P_MS },
	{ "week", 7LL * 24LL * 60LL * 60LL * SECONDS_P_MS },
	{ "weeks", 7LL * 24LL * 60LL * 60LL * SECONDS_P_MS },
	{ NULL, 0 }
};

long long convert_time_units(long long value, long long base, const char *units)
{
	struct time_units *ent;
	if (!units)
		/* default to base if no units */
		return value;

	for (ent = time_units; ent->str; ent++) {
		if (strcmp(ent->str, units) == 0) {
			if (value * ent->value < base)
				return -1LL;
			return value * ent->value / base;
		}
	}
	return -2LL;
}

#ifdef UNIT_TEST

#include "unit_test.h"

int test_str_to_boolean(void)
{
	int rc = 0;
	int retval;

	retval = str_to_boolean("TRUE");
	MY_TEST(retval == 1, "str2bool for TRUE");

	retval = str_to_boolean("TrUe");
	MY_TEST(retval == 1, "str2bool for TrUe");

	retval = str_to_boolean("false");
	MY_TEST(retval == 0, "str2bool for false");

	retval = str_to_boolean("flase");
	MY_TEST(retval == -1, "str2bool for flase");

	return rc;
}

#define MY_TEST_UNQUOTED(input, expected, description) \
	do { 										\
		char *result_str = NULL;						\
		char *output_str = NULL;						\
											\
		result_str = processunquoted((input), strlen((input)));			\
		asprintf(&output_str, "processunquoted: %s\tinput = '%s'\texpected = '%s'\tresult = '%s'", \
				(description), (input), (expected), result_str);	\
		MY_TEST(strcmp((expected), result_str) == 0, output_str);		\
											\
		free(output_str);							\
		free(result_str); 							\
	}										\
	while(0)

int test_processunquoted(void)
{
	int rc = 0;

	MY_TEST_UNQUOTED("", "", "empty string");
	MY_TEST_UNQUOTED("\\1", "\001", "one digit octal");
	MY_TEST_UNQUOTED("\\8", "\\8", "invalid octal digit \\8");
	MY_TEST_UNQUOTED("\\18", "\0018", "one digit octal followed by invalid octal digit");
	MY_TEST_UNQUOTED("\\1a", "\001a", "one digit octal followed by hex digit a");
	MY_TEST_UNQUOTED("\\1z", "\001z", "one digit octal follow by char z");
	MY_TEST_UNQUOTED("\\11", "\011", "two digit octal");
	MY_TEST_UNQUOTED("\\118", "\0118", "two digit octal followed by invalid octal digit");
	MY_TEST_UNQUOTED("\\11a", "\011a", "two digit octal followed by hex digit a");
	MY_TEST_UNQUOTED("\\11z", "\011z", "two digit octal followed by char z");
	MY_TEST_UNQUOTED("\\111", "\111", "three digit octal");
	MY_TEST_UNQUOTED("\\378", "\0378", "three digit octal two large, taken as 2 digit octal plus trailing char");
	MY_TEST_UNQUOTED("123\\421123", "123\0421123", "two character octal followed by valid octal digit \\421");
	MY_TEST_UNQUOTED("123\\109123", "123\109123", "octal 109");
	MY_TEST_UNQUOTED("123\\1089123", "123\1089123", "octal 108");

	return rc;
}

int test_processquoted(void)
{
	int rc = 0;
	const char *teststring, *processedstring;
	char *out;

	teststring = "";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(teststring, out) == 0,
			"processquoted on empty string");
	free(out);

	teststring = "\"abcdefg\"";
	processedstring = "abcdefg";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on simple string");
	free(out);

	teststring = "\"abcd\\tefg\"";
	processedstring = "abcd\tefg";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on string with tab");
	free(out);

	teststring = "\"abcdefg\\\"";
	processedstring = "abcdefg\\";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on trailing slash");
	free(out);

	teststring = "\"a\\\\bcdefg\"";
	processedstring = "a\\\\bcdefg";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on quoted slash");
	free(out);

	teststring = "\"a\\\"bcde\\\"fg\"";
	processedstring = "a\"bcde\"fg";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on quoted quotes");
	free(out);

	teststring = "\"\\rabcdefg\"";
	processedstring = "\rabcdefg";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on quoted \\r");
	free(out);

	teststring = "\"abcdefg\\n\"";
	processedstring = "abcdefg\n";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on quoted \\n");
	free(out);

	teststring = "\"\\Uabc\\Ndefg\\x\"";
	processedstring = "\\Uabc\\Ndefg\\x";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted passthrough on invalid quoted chars");
	free(out);

	teststring = "\"abc\\042defg\"";
	processedstring = "abc\"defg";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on quoted octal \\042");
	free(out);

	teststring = "\"abcdefg\\176\"";
	processedstring = "abcdefg~";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted on quoted octal \\176");
	free(out);

	teststring = "\"abc\\429defg\"";
	processedstring = "abc\0429defg";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted passthrough quoted invalid octal \\429");
	free(out);

	teststring = "\"abcdefg\\4\"";
	processedstring = "abcdefg\004";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted passthrough quoted one digit trailing octal \\4");
	free(out);

	teststring = "\"abcdefg\\04\"";
	processedstring = "abcdefg\004";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted passthrough quoted two digit trailing octal \\04");
	free(out);

	teststring = "\"abcdefg\\004\"";
	processedstring = "abcdefg\004";
	out = processquoted(teststring, strlen(teststring));
	MY_TEST(strcmp(processedstring, out) == 0,
			"processquoted passthrough quoted three digit trailing octal \\004");
	free(out);

	return rc;
}

#define TIME_TEST(V, B, U, R)					\
MY_TEST(convert_time_units((V), (B), U) == (R),		\
	"convert " #V " with base of " #B ", " #U " units")

int test_convert_time_units()
{
	int rc = 0;

	TIME_TEST(1LL, 1LL, NULL, 1LL);
	TIME_TEST(12345LL, 1LL, NULL, 12345LL);
	TIME_TEST(10LL, 10LL, NULL, 10LL);
	TIME_TEST(123450LL, 10LL, NULL, 123450LL);

	TIME_TEST(12345LL, 1LL, "us", 12345LL);
	TIME_TEST(12345LL, 1LL, "microsecond", 12345LL);
	TIME_TEST(12345LL, 1LL, "microseconds", 12345LL);

	TIME_TEST(12345LL, 1LL, "ms", 12345LL*1000LL);
	TIME_TEST(12345LL, 1LL, "millisecond", 12345LL*1000LL);
	TIME_TEST(12345LL, 1LL, "milliseconds", 12345LL*1000LL);

	TIME_TEST(12345LL, 1LL, "s", 12345LL*1000LL*1000LL);
	TIME_TEST(12345LL, 1LL, "sec", 12345LL*1000LL*1000LL);
	TIME_TEST(12345LL, 1LL, "second", 12345LL*1000LL*1000LL);
	TIME_TEST(12345LL, 1LL, "seconds", 12345LL*1000LL*1000LL);

	TIME_TEST(12345LL, 1LL, "min", 12345LL*1000LL*1000LL*60LL);
	TIME_TEST(12345LL, 1LL, "minute", 12345LL*1000LL*1000LL*60LL);
	TIME_TEST(12345LL, 1LL, "minutes", 12345LL*1000LL*1000LL*60LL);

	TIME_TEST(12345LL, 1LL, "h", 12345LL*1000LL*1000LL*60LL*60LL);
	TIME_TEST(12345LL, 1LL, "hour", 12345LL*1000LL*1000LL*60LL*60LL);
	TIME_TEST(12345LL, 1LL, "hours", 12345LL*1000LL*1000LL*60LL*60LL);

	TIME_TEST(12345LL, 1LL, "d", 12345LL*1000LL*1000LL*60LL*60LL*24LL);
	TIME_TEST(12345LL, 1LL, "day", 12345LL*1000LL*1000LL*60LL*60LL*24LL);
	TIME_TEST(12345LL, 1LL, "days", 12345LL*1000LL*1000LL*60LL*60LL*24LL);

	TIME_TEST(12345LL, 1LL, "week", 12345LL*1000LL*1000LL*60LL*60LL*24LL*7LL);
	TIME_TEST(12345LL, 1LL, "weeks", 12345LL*1000LL*1000LL*60LL*60LL*24LL*7LL);

	return rc;
}

int main(void)
{
	int rc = 0;
	int retval;

	retval = test_str_to_boolean();
	if (retval != 0)
		rc = retval;

	retval = test_processunquoted();
	if (retval != 0)
		rc = retval;

	retval = test_processquoted();
	if (retval != 0)
		rc = retval;

	retval = test_convert_time_units();
	if (retval != 0)
		rc = retval;

	return rc;
}
#endif /* UNIT_TEST */
