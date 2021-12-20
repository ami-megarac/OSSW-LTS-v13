/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/apparmor.h>

#include <iomanip>
#include <string>
#include <sstream>


/* #define DEBUG */

#include "lib.h"
#include "parser.h"
#include "profile.h"
#include "libapparmor_re/apparmor_re.h"
#include "libapparmor_re/aare_rules.h"
#include "policydb.h"
#include "rule.h"

enum error_type {
	e_no_error,
	e_parse_error,
};


/* Filters out multiple slashes (except if the first two are slashes,
 * that's a distinct namespace in linux) and trailing slashes.
 * NOTE: modifies in place the contents of the path argument */

static void filter_slashes(char *path)
{
	char *sptr, *dptr;
	BOOL seen_slash = 0;

	if (!path || (strlen(path) < 2))
		return;

	sptr = dptr = path;

	/* special case for linux // namespace */
	if (sptr[0] == '/' && sptr[1] == '/' && sptr[2] != '/') {
		sptr += 2;
		dptr += 2;
	}

	while (*sptr) {
		if (*sptr == '/') {
			if (seen_slash) {
				++sptr;
			} else {
				*dptr++ = *sptr++;
				seen_slash = TRUE;
			}
		} else {
			seen_slash = 0;
			if (dptr < sptr) {
				*dptr++ = *sptr++;
			} else {
				dptr++;
				sptr++;
			}
		}
	}
	*dptr = 0;
}

static error_type append_glob(std::string &pcre, int glob,
			      const char *default_glob, const char *null_glob)
{
	switch (glob) {
	case glob_default:
		pcre.append(default_glob);
		break;
	case glob_null:
		pcre.append(null_glob);
		break;
	default:
		PERROR(_("%s: Invalid glob type %d\n"), progname, glob);
		return e_parse_error;
		break;
	}
	return e_no_error;
}

/* converts the apparmor regex in aare and appends pcre regex output
 * to pcre string */
pattern_t convert_aaregex_to_pcre(const char *aare, int anchor, int glob,
				  std::string& pcre, int *first_re_pos)
{
#define update_re_pos(X) if (!(*first_re_pos)) { *first_re_pos = (X); }
#define MAX_ALT_DEPTH 50
	*first_re_pos = 0;

	int ret = TRUE;
	/* flag to indicate input error */
	enum error_type error;

	const char *sptr;
	pattern_t ptype;

	BOOL bEscape = 0;	/* flag to indicate escape */
	int ingrouping = 0;	/* flag to indicate {} context */
	int incharclass = 0;	/* flag to indicate [ ] context */
	int grouping_count[MAX_ALT_DEPTH] = {0};

	error = e_no_error;
	ptype = ePatternBasic;	/* assume no regex */

	sptr = aare;

	if (dfaflags & DFA_DUMP_RULE_EXPR)
		fprintf(stderr, "aare: %s   ->   ", aare);

	if (anchor)
		/* anchor beginning of regular expression */
		pcre.append("^");

	while (error == e_no_error && *sptr) {
		switch (*sptr) {

		case '\\':
			/* concurrent escapes are allowed now and
			 * output as two consecutive escapes so that
			 * pcre won't interpret them
			 * \\\{...\\\} will be emitted as \\\{...\\\}
			 * for pcre matching.  For string matching
			 * and globbing only one escape is output
			 * this is done by stripping later
			 */
			if (bEscape) {
				pcre.append("\\\\");
			} else {
				bEscape = TRUE;
				++sptr;
				continue;	/*skip turning bEscape off */
			}	/* bEscape */
			break;
		case '*':
			if (bEscape) {
				/* '*' is a PCRE special character */
				/* We store an escaped *, in case we
				 * end up using this regex buffer (i.e another
				 * non-escaped regex follows)
				 */
				pcre.append("\\*");
			} else {
				if ((pcre.length() > 0) && pcre[pcre.length() - 1]  == '/') {
					#if 0
					// handle comment containing use
					// of C comment characters
					// /* /*/ and /** to describe paths
					//
					// modify what is emitted for * and **
					// when used as the only path
					// component
					// ex.
					// /* /*/ /**/ /**
					// this prevents these expressions
					// from matching directories or
					// invalid paths
					// in these case * and ** must
					// match at least 1 character to
					// get a valid path element.
					// ex.
					// /foo/* -> should not match /foo/
					// /foo/*bar -> should match /foo/bar
					// /*/foo -> should not match //foo
					#endif
					const char *s = sptr;
					while (*s == '*')
						s++;
					if (*s == '/' || !*s)
						error = append_glob(pcre, glob, "[^/\\x00]", "[^/]");
				}
				if (*(sptr + 1) == '*') {
					/* is this the first regex form we
					 * have seen and also the end of
					 * pattern? If so, we can use
					 * optimised tail globbing rather
					 * than full regex.
					 */
					update_re_pos(sptr - aare);
					if (*(sptr + 2) == '\0' &&
					    ptype == ePatternBasic) {
						ptype = ePatternTailGlob;
					} else {
						ptype = ePatternRegex;
					}
					error = append_glob(pcre, glob, "[^\\x00]*", ".*");
					sptr++;
				} else {
					update_re_pos(sptr - aare);
					ptype = ePatternRegex;
					error = append_glob(pcre, glob, "[^/\\x00]*", "[^/]*");
				}	/* *(sptr+1) == '*' */
			}	/* bEscape */

			break;

		case '?':
			if (bEscape) {
				/* ? is not a PCRE regex character
				 * so no need to escape, just skip
				 * transform
				 */
				pcre.append(1, *sptr);
			} else {
				update_re_pos(sptr - aare);
				ptype = ePatternRegex;
				pcre.append("[^/\\x00]");
			}
			break;

		case '[':
			if (bEscape) {
				/* [ is a PCRE special character */
				pcre.append("\\[");
			} else {
				update_re_pos(sptr - aare);
				incharclass = 1;
				ptype = ePatternRegex;
				pcre.append(1, *sptr);
			}
			break;

		case ']':
			if (bEscape) {
				/* ] is a PCRE special character */
				pcre.append("\\]");
			} else {
				if (incharclass == 0) {
					error = e_parse_error;
					PERROR(_("%s: Regex grouping error: Invalid close ], no matching open [ detected\n"), progname);
				}
				incharclass = 0;
				pcre.append(1, *sptr);
			}
			break;

		case '{':
			if (bEscape) {
				/* { is a PCRE special character */
				pcre.append("\\{");
			} else {
				if (incharclass) {
					/* don't expand inside [] */
					pcre.append("{");
				} else {
					update_re_pos(sptr - aare);
					ingrouping++;
					if (ingrouping >= MAX_ALT_DEPTH) {
						error = e_parse_error;
						PERROR(_("%s: Regex grouping error: Exceeded maximum nesting of {}\n"), progname);

					} else {
						grouping_count[ingrouping] = 0;
						ptype = ePatternRegex;
						pcre.append("(");
					}
				}	/* incharclass */
			}
			break;

		case '}':
			if (bEscape) {
				/* { is a PCRE special character */
				pcre.append("\\}");
			} else {
				if (incharclass) {
					/* don't expand inside [] */
					pcre.append("}");
				} else {
					if (grouping_count[ingrouping] == 0) {
						error = e_parse_error;
						PERROR(_("%s: Regex grouping error: Invalid number of items between {}\n"), progname);

					}
					ingrouping--;
					if (ingrouping < 0) {
						error = e_parse_error;
						PERROR(_("%s: Regex grouping error: Invalid close }, no matching open { detected\n"), progname);
						ingrouping = 0;
					}
					pcre.append(")");
				}	/* incharclass */
			}	/* bEscape */

			break;

		case ',':
			if (bEscape) {
				if (incharclass) {
					/* escape inside char class is a
					 * valid matching char for '\'
					 */
					pcre.append("\\,");
				} else {
					/* ',' is not a PCRE regex character
					 * so no need to escape, just skip
					 * transform
					 */
					pcre.append(1, *sptr);
				}
			} else {
				if (ingrouping && !incharclass) {
					grouping_count[ingrouping]++;
					pcre.append("|");
				} else {
					pcre.append(1, *sptr);
				}
			}
			break;

			/* these are special outside of character
			 * classes but not in them */
		case '^':
		case '$':
			if (incharclass) {
				pcre.append(1, *sptr);
			} else {
				pcre.append("\\");
				pcre.append(1, *sptr);
			}
			break;

			/*
			 * Not a subdomain regex, but needs to be
			 * escaped as it is a pcre metacharacter which
			 * we don't want to support. We always escape
			 * these, so no need to check bEscape
			 */
		case '.':
		case '+':
		case '|':
		case '(':
		case ')':
			pcre.append("\\");
			// fall through to default

		default:
			if (bEscape) {
				const char *pos = sptr;
				int c;
				if ((c = str_escseq(&pos, "")) != -1) {
					/* valid escape we don't want to
					 * interpret here */
					pcre.append("\\");
					pcre.append(sptr, pos - sptr);
					sptr += (pos - sptr) - 1;
				} else {
					/* quoting mark used for something that
					 * does not need to be quoted; give a
					 * warning */
					pwarn("Character %c was quoted "
					      "unnecessarily, dropped preceding"
					      " quote ('\\') character\n",
					      *sptr);
					pcre.append(1, *sptr);
				}
			} else
				pcre.append(1, *sptr);
			break;
		}	/* switch (*sptr) */

		bEscape = FALSE;
		++sptr;
	}		/* while error == e_no_error && *sptr) */

	if (ingrouping > 0 || incharclass) {
		error = e_parse_error;

		PERROR(_("%s: Regex grouping error: Unclosed grouping or character class, expecting close }\n"),
		       progname);
	}

	if ((error == e_no_error) && bEscape) {
		/* trailing backslash quote */
		error = e_parse_error;
		PERROR(_("%s: Regex error: trailing '\\' escape character\n"),
		       progname);
	}
	/* anchor end and terminate pattern string */
	if ((error == e_no_error) && anchor) {
		pcre.append("$");
	}
	/* check error  again, as above STORE may have set it */
	if (error != e_no_error) {
		PERROR(_("%s: Unable to parse input line '%s'\n"),
		       progname, aare);

		ret = FALSE;
		goto out;
	}

out:
	if (ret == FALSE)
		ptype = ePatternInvalid;

	if (dfaflags & DFA_DUMP_RULE_EXPR)
		fprintf(stderr, "%s\n", pcre.c_str());

	return ptype;
}

static const char *local_name(const char *name)
{
	const char *t;

	for (t = strstr(name, "//") ; t ; t = strstr(name, "//"))
		name = t + 2;

	return name;
}

static int process_profile_name_xmatch(Profile *prof)
{
	std::string tbuf;
	pattern_t ptype;
	const char *name;

	/* don't filter_slashes for profile names */
	if (prof->attachment)
		name = prof->attachment;
	else
		name = local_name(prof->name);
	ptype = convert_aaregex_to_pcre(name, 0, glob_default, tbuf,
					&prof->xmatch_len);
	if (ptype == ePatternBasic)
		prof->xmatch_len = strlen(name);

	if (ptype == ePatternInvalid) {
		PERROR(_("%s: Invalid profile name '%s' - bad regular expression\n"), progname, name);
		return FALSE;
	} else if (ptype == ePatternBasic && !(prof->altnames || prof->attachment)) {
		/* no regex so do not set xmatch */
		prof->xmatch = NULL;
		prof->xmatch_len = 0;
		prof->xmatch_size = 0;
	} else {
		/* build a dfa */
		aare_rules *rules = new aare_rules();
		if (!rules)
			return FALSE;
		if (!rules->add_rule(tbuf.c_str(), 0, AA_MAY_EXEC, 0, dfaflags)) {
			delete rules;
			return FALSE;
		}
		if (prof->altnames) {
			struct alt_name *alt;
			list_for_each(prof->altnames, alt) {
				int len;
				tbuf.clear();
				ptype = convert_aaregex_to_pcre(alt->name, 0,
								glob_default,
								tbuf, &len);
				if (ptype == ePatternBasic)
					len = strlen(alt->name);
				if (len < prof->xmatch_len)
					prof->xmatch_len = len;
				if (!rules->add_rule(tbuf.c_str(), 0, AA_MAY_EXEC, 0, dfaflags)) {
					delete rules;
					return FALSE;
				}
			}
		}
		prof->xmatch = rules->create_dfa(&prof->xmatch_size, dfaflags);
		delete rules;
		if (!prof->xmatch)
			return FALSE;
	}

	return TRUE;
}

static int warn_change_profile = 1;

static bool is_change_profile_mode(int mode)
{
	/**
	 * A change_profile entry will have the AA_CHANGE_PROFILE bit set.
	 * It could also have the (AA_EXEC_BITS | ALL_AA_EXEC_UNSAFE) bits
	 * set by the frontend parser. That means that it is incorrect to
	 * identify change_profile modes using a test like this:
	 *
	 *   (mode & ~AA_CHANGE_PROFILE)
	 *
	 * The above test would incorrectly return true on a
	 * change_profile mode that has the
	 * (AA_EXEC_BITS | ALL_AA_EXEC_UNSAFE) bits set.
	 */
	return mode & AA_CHANGE_PROFILE;
}

static int process_dfa_entry(aare_rules *dfarules, struct cod_entry *entry)
{
	std::string tbuf;
	pattern_t ptype;
	int pos;

	if (!entry) 		/* shouldn't happen */
		return TRUE;


	if (!is_change_profile_mode(entry->mode))
		filter_slashes(entry->name);
	ptype = convert_aaregex_to_pcre(entry->name, 0, glob_default, tbuf, &pos);
	if (ptype == ePatternInvalid)
		return FALSE;

	entry->pattern_type = ptype;

	/* ix implies m but the apparmor module does not add m bit to
	 * dfa states like it does for pcre
	 */
	if ((entry->mode >> AA_OTHER_SHIFT) & AA_EXEC_INHERIT)
		entry->mode |= AA_OLD_EXEC_MMAP << AA_OTHER_SHIFT;
	if ((entry->mode >> AA_USER_SHIFT) & AA_EXEC_INHERIT)
		entry->mode |= AA_OLD_EXEC_MMAP << AA_USER_SHIFT;

	/* the link bit on the first pair entry should not get masked
	 * out by a deny rule, as both pieces of the link pair must
	 * match.  audit info for the link is carried on the second
	 * entry of the pair
	 *
	 * So if a deny rule only record it if there are permissions other
	 * than link in the entry.
	 * TODO: split link and change_profile entries earlier
	 */
	if (entry->deny) {
		if ((entry->mode & ~AA_LINK_BITS) &&
		    !is_change_profile_mode(entry->mode) &&
		    !dfarules->add_rule(tbuf.c_str(), entry->deny,
					entry->mode & ~(AA_LINK_BITS | AA_CHANGE_PROFILE),
					entry->audit & ~(AA_LINK_BITS | AA_CHANGE_PROFILE),
					dfaflags))
			return FALSE;
	} else if (!is_change_profile_mode(entry->mode)) {
		if (!dfarules->add_rule(tbuf.c_str(), entry->deny, entry->mode,
					entry->audit, dfaflags))
			return FALSE;
	}

	if (entry->mode & (AA_LINK_BITS)) {
		/* add the pair rule */
		std::string lbuf;
		int perms = AA_LINK_BITS & entry->mode;
		const char *vec[2];
		int pos;
		vec[0] = tbuf.c_str();
		if (entry->link_name) {
			ptype = convert_aaregex_to_pcre(entry->link_name, 0, glob_default, lbuf, &pos);
			if (ptype == ePatternInvalid)
				return FALSE;
			if (entry->subset)
				perms |= LINK_TO_LINK_SUBSET(perms);
			vec[1] = lbuf.c_str();
		} else {
			perms |= LINK_TO_LINK_SUBSET(perms);
			vec[1] = "/[^/].*";
		}
		if (!dfarules->add_rule_vec(entry->deny, perms, entry->audit & AA_LINK_BITS, 2, vec, dfaflags))
			return FALSE;
	}
	if (is_change_profile_mode(entry->mode)) {
		const char *vec[3];
		std::string lbuf, xbuf;
		autofree char *ns = NULL;
		autofree char *name = NULL;
		int index = 1;
		uint32_t onexec_perms = AA_ONEXEC;

		if ((warnflags & WARN_RULE_DOWNGRADED) && entry->audit && warn_change_profile) {
			/* don't have profile name here, so until this code
			 * gets refactored just throw out a generic warning
			 */
			fprintf(stderr, "Warning kernel does not support audit modifier for change_profile rule.\n");
			warn_change_profile = 0;
		}

		if (entry->onexec) {
			ptype = convert_aaregex_to_pcre(entry->onexec, 0, glob_default, xbuf, &pos);
			if (ptype == ePatternInvalid)
				return FALSE;
			vec[0] = xbuf.c_str();
		} else
			/* allow change_profile for all execs */
			vec[0] = "/[^/\\x00][^\\x00]*";

		if (!kernel_supports_stacking) {
			bool stack;

			if (!parse_label(&stack, &ns, &name,
					 tbuf.c_str(), false)) {
				return FALSE;
			}

			if (stack) {
				fprintf(stderr,
					_("The current kernel does not support stacking of named transitions: %s\n"),
					tbuf.c_str());
				return FALSE;
			}

			if (ns)
				vec[index++] = ns;
			vec[index++] = name;
		} else {
			vec[index++] = tbuf.c_str();
		}

		/* regular change_profile rule */
		if (!dfarules->add_rule_vec(entry->deny,
					    AA_CHANGE_PROFILE | onexec_perms,
					    0, index - 1, &vec[1], dfaflags))
			return FALSE;

		/* onexec rules - both rules are needed for onexec */
		if (!dfarules->add_rule_vec(entry->deny, onexec_perms,
					    0, 1, vec, dfaflags))
			return FALSE;

		/**
		 * pick up any exec bits, from the frontend parser, related to
		 * unsafe exec transitions
		 */
		onexec_perms |= (entry->mode & (AA_EXEC_BITS | ALL_AA_EXEC_UNSAFE));
		if (!dfarules->add_rule_vec(entry->deny, onexec_perms,
					    0, index, vec, dfaflags))
			return FALSE;
	}
	return TRUE;
}

int post_process_entries(Profile *prof)
{
	int ret = TRUE;
	struct cod_entry *entry;

	list_for_each(prof->entries, entry) {
		if (!process_dfa_entry(prof->dfa.rules, entry))
			ret = FALSE;
	}

	return ret;
}

int process_profile_regex(Profile *prof)
{
	int error = -1;

	if (!process_profile_name_xmatch(prof))
		goto out;

	prof->dfa.rules = new aare_rules();
	if (!prof->dfa.rules)
		goto out;

	if (!post_process_entries(prof))
		goto out;

	if (prof->dfa.rules->rule_count > 0) {
		prof->dfa.dfa = prof->dfa.rules->create_dfa(&prof->dfa.size,
							    dfaflags);
		delete prof->dfa.rules;
		prof->dfa.rules = NULL;
		if (!prof->dfa.dfa)
			goto out;
/*
		if (prof->dfa_size == 0) {
			PERROR(_("profile %s: has merged rules (%s) with "
				 "multiple x modifiers\n"),
			       prof->name, (char *) prof->dfa);
			free(prof->dfa);
			prof->dfa = NULL;
			goto out;
		}
*/
	}

	error = 0;

out:
	return error;
}

int build_list_val_expr(std::string& buffer, struct value_list *list)
{
	struct value_list *ent;
	pattern_t ptype;
	int pos;

	if (!list) {
		buffer.append(default_match_pattern);
		return TRUE;
	}

	buffer.append("(");

	ptype = convert_aaregex_to_pcre(list->value, 0, glob_default, buffer, &pos);
	if (ptype == ePatternInvalid)
		goto fail;

	list_for_each(list->next, ent) {
		buffer.append("|");
		ptype = convert_aaregex_to_pcre(ent->value, 0, glob_default, buffer, &pos);
		if (ptype == ePatternInvalid)
			goto fail;
	}
	buffer.append(")");

	return TRUE;
fail:
	return FALSE;
}

int convert_entry(std::string& buffer, char *entry)
{
	pattern_t ptype;
	int pos;

	if (entry) {
		ptype = convert_aaregex_to_pcre(entry, 0, glob_default, buffer, &pos);
		if (ptype == ePatternInvalid)
			return FALSE;
	} else {
		buffer.append(default_match_pattern);
	}

	return TRUE;
}

int clear_and_convert_entry(std::string& buffer, char *entry)
{
	buffer.clear();
	return convert_entry(buffer, entry);
}

int post_process_policydb_ents(Profile *prof)
{
	for (RuleList::iterator i = prof->rule_ents.begin(); i != prof->rule_ents.end(); i++) {
		if ((*i)->gen_policy_re(*prof) == RULE_ERROR)
			return FALSE;
	}

	return TRUE;
}

#define MAKE_STR(X) #X
#define CLASS_STR(X) "\\d" MAKE_STR(X)
#define MAKE_SUB_STR(X) "\\000" MAKE_STR(X)
#define CLASS_SUB_STR(X, Y) MAKE_STR(X) MAKE_SUB_STR(Y)

static const char *mediates_file = CLASS_STR(AA_CLASS_FILE);
static const char *mediates_mount = CLASS_STR(AA_CLASS_MOUNT);
static const char *mediates_dbus =  CLASS_STR(AA_CLASS_DBUS);
static const char *mediates_signal =  CLASS_STR(AA_CLASS_SIGNAL);
static const char *mediates_ptrace =  CLASS_STR(AA_CLASS_PTRACE);
static const char *mediates_extended_net = CLASS_STR(AA_CLASS_NET);
static const char *mediates_net_unix = CLASS_SUB_STR(AA_CLASS_NET, AF_UNIX);

int process_profile_policydb(Profile *prof)
{
	int error = -1;

	prof->policy.rules = new aare_rules();
	if (!prof->policy.rules)
		goto out;

	if (!post_process_policydb_ents(prof))
		goto out;

	/* insert entries to show indicate what compiler/policy expects
	 * to be supported
	 */

	/* note: this activates fs based unix domain sockets mediation on connect */
	if (kernel_abi_version > 5 &&
	    !prof->policy.rules->add_rule(mediates_file, 0, AA_MAY_READ, 0, dfaflags))
		goto out;
	if (kernel_supports_mount &&
	    !prof->policy.rules->add_rule(mediates_mount, 0, AA_MAY_READ, 0, dfaflags))
			goto out;
	if (kernel_supports_dbus &&
	    !prof->policy.rules->add_rule(mediates_dbus, 0, AA_MAY_READ, 0, dfaflags))
		goto out;
	if (kernel_supports_signal &&
	    !prof->policy.rules->add_rule(mediates_signal, 0, AA_MAY_READ, 0, dfaflags))
		goto out;
	if (kernel_supports_ptrace &&
	    !prof->policy.rules->add_rule(mediates_ptrace, 0, AA_MAY_READ, 0, dfaflags))
		goto out;
	if (kernel_supports_unix &&
	    (!prof->policy.rules->add_rule(mediates_extended_net, 0, AA_MAY_READ, 0, dfaflags) ||
	     !prof->policy.rules->add_rule(mediates_net_unix, 0, AA_MAY_READ, 0, dfaflags)))
		goto out;

	if (prof->policy.rules->rule_count > 0) {
		prof->policy.dfa = prof->policy.rules->create_dfa(&prof->policy.size, dfaflags);
		delete prof->policy.rules;

		prof->policy.rules = NULL;
		if (!prof->policy.dfa)
			goto out;
	} else {
		delete prof->policy.rules;
		prof->policy.rules = NULL;
	}

	error = 0;

out:
	delete prof->policy.rules;
	prof->policy.rules = NULL;

	return error;
}

#ifdef UNIT_TEST

#include "unit_test.h"

#define MY_FILTER_TEST(input, expected_str)	\
	do {												\
		char *test_string = NULL;								\
		char *output_string = NULL;								\
													\
		test_string = strdup((input)); 								\
		filter_slashes(test_string); 								\
		asprintf(&output_string, "simple filter / conversion for '%s'\texpected = '%s'\tresult = '%s'", \
				(input), (expected_str), test_string);					\
		MY_TEST(strcmp(test_string, (expected_str)) == 0, output_string);			\
													\
		free(test_string);									\
		free(output_string);									\
	}												\
	while (0)

static int test_filter_slashes(void)
{
	int rc = 0;

	MY_FILTER_TEST("///foo//////f//oo////////////////", "/foo/f/oo/");
	MY_FILTER_TEST("/foo/f/oo", "/foo/f/oo");
	MY_FILTER_TEST("/", "/");
	MY_FILTER_TEST("", "");

	/* tests for "//" namespace */
	MY_FILTER_TEST("//usr", "//usr");
	MY_FILTER_TEST("//", "//");

	/* tests for not "//" namespace */
	MY_FILTER_TEST("///usr", "/usr");
	MY_FILTER_TEST("///", "/");

	MY_FILTER_TEST("/a/", "/a/");

	return rc;
}

#define MY_REGEX_EXT_TEST(glob, input, expected_str, expected_type)	\
	do {												\
		std::string tbuf;									\
		std::string tbuf2 = "testprefix";							\
		char *output_string = NULL;								\
		std::string expected_str2;								\
		pattern_t ptype;									\
		int pos;										\
													\
		ptype = convert_aaregex_to_pcre((input), 0, glob, tbuf, &pos); \
		asprintf(&output_string, "simple regex conversion for '%s'\texpected = '%s'\tresult = '%s'", \
				(input), (expected_str), tbuf.c_str());					\
		MY_TEST(strcmp(tbuf.c_str(), (expected_str)) == 0, output_string);			\
		MY_TEST(ptype == (expected_type), "simple regex conversion type check for '" input "'"); \
		free(output_string);									\
		/* ensure convert_aaregex_to_pcre appends only to passed ref string */			\
		expected_str2 = tbuf2;									\
		expected_str2.append((expected_str));							\
		ptype = convert_aaregex_to_pcre((input), 0, glob, tbuf2, &pos); \
		asprintf(&output_string, "simple regex conversion %sfor '%s'\texpected = '%s'\tresult = '%s'", \
			 glob == glob_null ? "with null allowed in glob " : "",\
				(input), expected_str2.c_str(), tbuf2.c_str());				\
		MY_TEST((tbuf2 == expected_str2), output_string);					\
		free(output_string);									\
	}												\
	while (0)

#define MY_REGEX_TEST(input, expected_str, expected_type) MY_REGEX_EXT_TEST(glob_default, input, expected_str, expected_type)


#define MY_REGEX_FAIL_TEST(input)						\
	do {												\
		std::string tbuf;									\
		pattern_t ptype;									\
		int pos;										\
													\
		ptype = convert_aaregex_to_pcre((input), 0, glob_default, tbuf, &pos); \
		MY_TEST(ptype == ePatternInvalid, "simple regex conversion invalid type check for '" input "'"); \
	}												\
	while (0)

static int test_aaregex_to_pcre(void)
{
	int rc = 0;

	MY_REGEX_TEST("/most/basic/test", "/most/basic/test", ePatternBasic);

	MY_REGEX_FAIL_TEST("\\");
	MY_REGEX_TEST("\\\\", "\\\\", ePatternBasic);
	MY_REGEX_TEST("\\blort", "blort", ePatternBasic);
	MY_REGEX_TEST("\\\\blort", "\\\\blort", ePatternBasic);
	MY_REGEX_FAIL_TEST("blort\\");
	MY_REGEX_TEST("blort\\\\", "blort\\\\", ePatternBasic);
	MY_REGEX_TEST("*", "[^/\\x00]*", ePatternRegex);
	MY_REGEX_TEST("blort*", "blort[^/\\x00]*", ePatternRegex);
	MY_REGEX_TEST("*blort", "[^/\\x00]*blort", ePatternRegex);
	MY_REGEX_TEST("\\*", "\\*", ePatternBasic);
	MY_REGEX_TEST("blort\\*", "blort\\*", ePatternBasic);
	MY_REGEX_TEST("\\*blort", "\\*blort", ePatternBasic);

	/* simple quoting */
	MY_REGEX_TEST("\\[", "\\[", ePatternBasic);
	MY_REGEX_TEST("\\]", "\\]", ePatternBasic);
	MY_REGEX_TEST("\\?", "?", ePatternBasic);
	MY_REGEX_TEST("\\{", "\\{", ePatternBasic);
	MY_REGEX_TEST("\\}", "\\}", ePatternBasic);
	MY_REGEX_TEST("\\,", ",", ePatternBasic);
	MY_REGEX_TEST("^", "\\^", ePatternBasic);
	MY_REGEX_TEST("$", "\\$", ePatternBasic);
	MY_REGEX_TEST(".", "\\.", ePatternBasic);
	MY_REGEX_TEST("+", "\\+", ePatternBasic);
	MY_REGEX_TEST("|", "\\|", ePatternBasic);
	MY_REGEX_TEST("(", "\\(", ePatternBasic);
	MY_REGEX_TEST(")", "\\)", ePatternBasic);
	MY_REGEX_TEST("\\^", "\\^", ePatternBasic);
	MY_REGEX_TEST("\\$", "\\$", ePatternBasic);
	MY_REGEX_TEST("\\.", "\\.", ePatternBasic);
	MY_REGEX_TEST("\\+", "\\+", ePatternBasic);
	MY_REGEX_TEST("\\|", "\\|", ePatternBasic);
	MY_REGEX_TEST("\\(", "\\(", ePatternBasic);
	MY_REGEX_TEST("\\)", "\\)", ePatternBasic);

	/* simple character class tests */
	MY_REGEX_TEST("[blort]", "[blort]", ePatternRegex);
	MY_REGEX_FAIL_TEST("[blort");
	MY_REGEX_FAIL_TEST("b[lort");
	MY_REGEX_FAIL_TEST("blort[");
	MY_REGEX_FAIL_TEST("blort]");
	MY_REGEX_FAIL_TEST("blo]rt");
	MY_REGEX_FAIL_TEST("]blort");
	MY_REGEX_TEST("b[lor]t", "b[lor]t", ePatternRegex);

	/* simple alternation tests */
	MY_REGEX_TEST("{alpha,beta}", "(alpha|beta)", ePatternRegex);
	MY_REGEX_TEST("baz{alpha,beta}blort", "baz(alpha|beta)blort", ePatternRegex);
	MY_REGEX_FAIL_TEST("{beta}");
	MY_REGEX_FAIL_TEST("biz{beta");
	MY_REGEX_FAIL_TEST("biz}beta");
	MY_REGEX_FAIL_TEST("biz{be,ta");
	MY_REGEX_FAIL_TEST("biz,be}ta");
	MY_REGEX_FAIL_TEST("biz{}beta");

	/* nested alternations */
	MY_REGEX_TEST("{{alpha,blort,nested},beta}", "((alpha|blort|nested)|beta)", ePatternRegex);
	MY_REGEX_FAIL_TEST("{{alpha,blort,nested}beta}");
	MY_REGEX_TEST("{{alpha,{blort,nested}},beta}", "((alpha|(blort|nested))|beta)", ePatternRegex);
	MY_REGEX_TEST("{{alpha,alpha{blort,nested}}beta,beta}", "((alpha|alpha(blort|nested))beta|beta)", ePatternRegex);
	MY_REGEX_TEST("{{alpha,alpha{blort,nested}}beta,beta}", "((alpha|alpha(blort|nested))beta|beta)", ePatternRegex);
	MY_REGEX_TEST("{{a,b{c,d}}e,{f,{g,{h{i,j,k},l}m},n}o}", "((a|b(c|d))e|(f|(g|(h(i|j|k)|l)m)|n)o)", ePatternRegex);
	/* max nesting depth = 50 */
	MY_REGEX_TEST("{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a,b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b}b,blort}",
			"(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a(a|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)|b)b|blort)", ePatternRegex);
	MY_REGEX_FAIL_TEST("{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a{a,b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b},b}b,blort}");

	/* simple single char */
	MY_REGEX_TEST("blor?t", "blor[^/\\x00]t", ePatternRegex);

	/* simple globbing */
	MY_REGEX_TEST("/*", "/[^/\\x00][^/\\x00]*", ePatternRegex);
	MY_REGEX_TEST("/blort/*", "/blort/[^/\\x00][^/\\x00]*", ePatternRegex);
	MY_REGEX_TEST("/*/blort", "/[^/\\x00][^/\\x00]*/blort", ePatternRegex);
	MY_REGEX_TEST("/*/", "/[^/\\x00][^/\\x00]*/", ePatternRegex);
	MY_REGEX_TEST("/**", "/[^/\\x00][^\\x00]*", ePatternTailGlob);
	MY_REGEX_TEST("/blort/**", "/blort/[^/\\x00][^\\x00]*", ePatternTailGlob);
	MY_REGEX_TEST("/**/blort", "/[^/\\x00][^\\x00]*/blort", ePatternRegex);
	MY_REGEX_TEST("/**/", "/[^/\\x00][^\\x00]*/", ePatternRegex);

	/* more complicated quoting */
	MY_REGEX_FAIL_TEST("\\\\[");
	MY_REGEX_FAIL_TEST("\\\\]");
	MY_REGEX_TEST("\\\\?", "\\\\[^/\\x00]", ePatternRegex);
	MY_REGEX_FAIL_TEST("\\\\{");
	MY_REGEX_FAIL_TEST("\\\\}");
	MY_REGEX_TEST("\\\\,", "\\\\,", ePatternBasic);
	MY_REGEX_TEST("\\\\^", "\\\\\\^", ePatternBasic);
	MY_REGEX_TEST("\\\\$", "\\\\\\$", ePatternBasic);
	MY_REGEX_TEST("\\\\.", "\\\\\\.", ePatternBasic);
	MY_REGEX_TEST("\\\\+", "\\\\\\+", ePatternBasic);
	MY_REGEX_TEST("\\\\|", "\\\\\\|", ePatternBasic);
	MY_REGEX_TEST("\\\\(", "\\\\\\(", ePatternBasic);
	MY_REGEX_TEST("\\\\)", "\\\\\\)", ePatternBasic);
	MY_REGEX_TEST("\\000", "\\000", ePatternBasic);
	MY_REGEX_TEST("\\x00", "\\x00", ePatternBasic);
	MY_REGEX_TEST("\\d000", "\\d000", ePatternBasic);

	/* more complicated character class tests */
	/*   -- embedded alternations */
	MY_REGEX_TEST("b[\\lor]t", "b[lor]t", ePatternRegex);
	MY_REGEX_TEST("b[{a,b}]t", "b[{a,b}]t", ePatternRegex);
	MY_REGEX_TEST("{alpha,b[{a,b}]t,gamma}", "(alpha|b[{a,b}]t|gamma)", ePatternRegex);

	/* pcre will ignore the '\' before '\{', but it should be okay
	 * for us to pass this on to pcre as '\{' */
	MY_REGEX_TEST("b[\\{a,b\\}]t", "b[\\{a,b\\}]t", ePatternRegex);
	MY_REGEX_TEST("{alpha,b[\\{a,b\\}]t,gamma}", "(alpha|b[\\{a,b\\}]t|gamma)", ePatternRegex);
	MY_REGEX_TEST("{alpha,b[\\{a\\,b\\}]t,gamma}", "(alpha|b[\\{a\\,b\\}]t|gamma)", ePatternRegex);

	/* test different globbing behavior conversion */
	MY_REGEX_EXT_TEST(glob_default, "/foo/**", "/foo/[^/\\x00][^\\x00]*", ePatternTailGlob);
	MY_REGEX_EXT_TEST(glob_null, "/foo/**", "/foo/[^/].*", ePatternTailGlob);
	MY_REGEX_EXT_TEST(glob_default, "/foo/f**", "/foo/f[^\\x00]*", ePatternTailGlob);
	MY_REGEX_EXT_TEST(glob_null, "/foo/f**", "/foo/f.*", ePatternTailGlob);

	MY_REGEX_EXT_TEST(glob_default, "/foo/*", "/foo/[^/\\x00][^/\\x00]*", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_null, "/foo/*", "/foo/[^/][^/]*", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_default, "/foo/f*", "/foo/f[^/\\x00]*", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_null, "/foo/f*", "/foo/f[^/]*", ePatternRegex);

	MY_REGEX_EXT_TEST(glob_default, "/foo/**.ext", "/foo/[^\\x00]*\\.ext", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_null, "/foo/**.ext", "/foo/.*\\.ext", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_default, "/foo/f**.ext", "/foo/f[^\\x00]*\\.ext", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_null, "/foo/f**.ext", "/foo/f.*\\.ext", ePatternRegex);

	MY_REGEX_EXT_TEST(glob_default, "/foo/*.ext", "/foo/[^/\\x00]*\\.ext", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_null, "/foo/*.ext", "/foo/[^/]*\\.ext", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_default, "/foo/f*.ext", "/foo/f[^/\\x00]*\\.ext", ePatternRegex);
	MY_REGEX_EXT_TEST(glob_null, "/foo/f*.ext", "/foo/f[^/]*\\.ext", ePatternRegex);

	return rc;
}

int main(void)
{
	int rc = 0;
	int retval;

	retval = test_filter_slashes();
	if (retval != 0)
		rc = retval;

	retval = test_aaregex_to_pcre();
	if (retval != 0)
		rc = retval;

	return rc;
}
#endif /* UNIT_TEST */
