/*
 * Copyright (c) 1999-2008 NOVELL (All rights reserved)
 * Copyright (c) 2010, Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

%option noyywrap
%option nounput
%option noyy_top_state
%option reentrant
%option prefix="aalogparse_"
%option bison-bridge
%option header-file="scanner.h"
%option outfile="scanner.c"
%option stack
%{

#include "grammar.h"
#include <aalogparse.h>
#include "parser.h"

#include <assert.h>

#define YY_NO_INPUT

unsigned int string_buf_alloc = 0;
unsigned int string_buf_len = 0;
char *string_buf = NULL;

void string_buf_reset()
{
	/* rewind buffer to zero, possibly doing initial allocation too */
	string_buf_len = 0;
 	if (string_buf == NULL) {
		string_buf_alloc = 128;
		string_buf = malloc(string_buf_alloc);
		assert(string_buf != NULL);
	}
	/* always start with a valid but empty string */
	string_buf[0] = '\0';
}

void string_buf_append(unsigned int length, char *text)
{
	unsigned int current_length = string_buf_len;

	/* handle calling ..._append before ..._reset */
	if (string_buf == NULL) string_buf_reset();

	string_buf_len += length;
	/* expand allocation if this append would exceed the allocation */
	while (string_buf_len >= string_buf_alloc) {
		string_buf_alloc *= 2;
		string_buf = realloc(string_buf, string_buf_alloc);
		assert(string_buf != NULL);
	}
	/* copy and unconditionally terminate */
	memcpy(string_buf+current_length, text, length);
	string_buf[string_buf_len] = '\0';
}

%}

ws		[ \t\r\n]

equals		"="
digit		[[:digit:]]
digits		{digit}+
hex		[A-F0-9]
colon		":"
minus		"-"
plus		"+"
open_paren	"("
close_paren	")"
ID		[^ \t\n\(\)="'!]
hexstring	({hex}{hex})+
period		"\."
question_mark	"?"
single_quote	"'"
mode_chars      ([RrWwaLlMmkXx])|([Pp][Xx])|([Uu][Xx])|([Ii][Xx])|([Pp][Ii][Xx])
modes		({mode_chars}+)|({mode_chars}+::{mode_chars}*)|(::{mode_chars}*)
/* New message types */

aa_reject_type		"APPARMOR_DENIED"
aa_audit_type		"APPARMOR_AUDIT"
aa_complain_type	"APPARMOR_ALLOWED"
aa_hint_type		"APPARMOR_HINT"
aa_status_type		"APPARMOR_STATUS"
aa_error_type		"APPARMOR_ERROR"
reject_type		"\"DENIED\""
audit_type		"\"AUDIT\""
complain_type		"\"ALLOWED\""
hint_type		"\"HINT\""
status_type		"\"STATUS\""
error_type		"\"ERROR\""
lsm_avc_type		"AVC"
user_avc_type		"USER_AVC"
unknown_type		UNKNOWN\[{digits}+\]
other_audit_type	[[:alnum:]\[\]_-]+

/* Key tokens */

key_apparmor		"apparmor"
key_type		"type"
key_msg			"msg"
key_operation		"operation"
key_name		"name"
key_name2		"name2"
key_namespace		"namespace"
key_mask		"mask"
key_denied_mask		"denied_mask"
key_requested_mask	"requested_mask"
key_attribute		"attribute"
key_task		"task"
key_parent		"parent"
key_magic_token		"magic_token"
key_info		"info"
key_peer_info		"peer_info"
key_pid			"pid"
key_peer_pid		"peer_pid"
key_profile		"profile"
key_peer_profile	"peer_profile"
key_family		"family"
key_sock_type		"sock_type"
key_protocol		"protocol"
key_error		"error"
key_fsuid		"fsuid"
key_ouid		"ouid"
key_uid			"uid"
key_auid		"auid"
key_sauid		"sauid"
key_ses			"ses"
key_hostname		"hostname"
key_addr		"addr"
key_terminal		"terminal"
key_exe			"exe"
key_comm		"comm"
key_capability		"capability"
key_capname		"capname"
key_offset		"offset"
key_target		"target"
key_laddr		"laddr"
key_faddr		"faddr"
key_lport		"lport"
key_fport		"fport"
key_bus			"bus"
key_dest		"dest"
key_path		"path"
key_interface		"interface"
key_member		"member"
key_signal		"signal"
key_peer		"peer"
key_fstype		"fstype"
key_flags		"flags"
key_srcname		"srcname"
audit			"audit"

/* network addrs */
ip_addr			[a-f[:digit:].:]{3,}

/* syslog tokens */
syslog_kernel		kernel{colon}
syslog_user		[[:alnum:]_-]+\[[[:digit:]]+\]{colon}
syslog_yyyymmdd		{digit}{4}{minus}{digit}{2}{minus}{digit}{2}
syslog_date		{syslog_yyyymmdd}
syslog_month 		Jan(uary)?|Feb(ruary)?|Mar(ch)?|Apr(il)?|May|Jun(e)?|Jul(y)?|Aug(ust)?|Sep(tember)?|Oct(ober)?|Nov(ember)?|Dec(ember)?
hhmmss			{digit}{2}{colon}{digit}{2}{colon}{digit}{2}
timezone		({plus}|{minus}){digit}{2}{colon}{digit}{2}
syslog_time 		{hhmmss}({period}{digits})?{timezone}?
syslog_hostname		[[:alnum:]._-]+
dmesg_timestamp		\[[[:digit:] ]{5,}\.[[:digit:]]{6,}\]

%x single_quoted_string
%x quoted_string
%x sub_id
%x audit_id
%x hostname
%x dmesg_timestamp
%x safe_string
%x audit_types
%x ip_addr
%x other_audit
%x unknown_message

%%
%{
yy_flex_debug = 0;
%}


{ws}+			{ /* Skip whitespace */ }

<audit_id>{
	{digits}		{ yylval->t_str = strdup(yytext); return(TOK_AUDIT_DIGITS);}
	{colon}{ws}		{ yy_pop_state(yyscanner); return(TOK_COLON); }
	{colon}			{ return(TOK_COLON); }
	{period}		{ return(TOK_PERIOD); }
	{open_paren}		{ return(TOK_OPEN_PAREN); }
	{close_paren}		{ yy_pop_state(yyscanner); return(TOK_CLOSE_PAREN); }
	.			{ BEGIN(unknown_message); yyless(0); /* dump the rest */ }
}

<sub_id>{
	{open_paren}		{ return(TOK_OPEN_PAREN); }
	{close_paren}		{ BEGIN(INITIAL); return(TOK_CLOSE_PAREN); }
	{ws}		{ }
	\"			{ string_buf_reset(); BEGIN(quoted_string); }
	{ID}+	{
			yylval->t_str = strdup(yytext);
			BEGIN(INITIAL);
			return(TOK_ID);
		}
	{equals}		{ return(TOK_EQUALS); }
	}

\"			{ string_buf_reset(); BEGIN(quoted_string); }
<quoted_string>\"	{ /* End of the quoted string */
				BEGIN(INITIAL);
				yylval->t_str = strdup(string_buf);
				return(TOK_QUOTED_STRING);
			}


<quoted_string>\\(.|\n) { string_buf_append(1, &yytext[1]); }

<quoted_string>[^\\\n\"]+ { string_buf_append(yyleng, yytext); }

<safe_string>{
	\"		{ string_buf_reset(); BEGIN(quoted_string); }
	{hexstring}	{ yylval->t_str = hex_to_string(yytext); BEGIN(INITIAL); return(TOK_HEXSTRING);}
	{equals}	{ return(TOK_EQUALS); }
	.		{ /* eek, error! try another state */ BEGIN(INITIAL); yyless(0); }
	}

<ip_addr>{
	{ip_addr}	{ yylval->t_str = strdup(yytext); yy_pop_state(yyscanner); return(TOK_IP_ADDR); }
	{equals}	{ return(TOK_EQUALS); }
	.		{ /* eek, error! try another state */ BEGIN(INITIAL); yyless(0); }
	}

<audit_types>{
	{equals}	{ return(TOK_EQUALS); }
	{digits}	{ yylval->t_long = atol(yytext); BEGIN(INITIAL); return(TOK_DIGITS); }
	{reject_type}	{ BEGIN(INITIAL); return(TOK_TYPE_REJECT); }
	{audit_type}	{ BEGIN(INITIAL); return(TOK_TYPE_AUDIT); }
	{complain_type}	{ BEGIN(INITIAL); return(TOK_TYPE_COMPLAIN); }
	{hint_type}	{ BEGIN(INITIAL); return(TOK_TYPE_HINT); }
	{status_type}	{ BEGIN(INITIAL); return(TOK_TYPE_STATUS); }
	{error_type}	{ BEGIN(INITIAL); return(TOK_TYPE_ERROR); }
	{aa_reject_type}	{ BEGIN(INITIAL); return(TOK_TYPE_AA_REJECT); }
	{aa_audit_type}	{ BEGIN(INITIAL); return(TOK_TYPE_AA_AUDIT); }
	{aa_complain_type}	{ BEGIN(INITIAL); return(TOK_TYPE_AA_COMPLAIN); }
	{aa_hint_type}	{ BEGIN(INITIAL); return(TOK_TYPE_AA_HINT); }
	{aa_status_type}	{ BEGIN(INITIAL); return(TOK_TYPE_AA_STATUS); }
	{aa_error_type}	{ BEGIN(INITIAL); return(TOK_TYPE_AA_ERROR); }
	{lsm_avc_type}	{ BEGIN(INITIAL); return(TOK_TYPE_LSM_AVC); }
	{user_avc_type}	{ BEGIN(INITIAL); return(TOK_TYPE_USER_AVC); }
	{unknown_type}	{ char *yptr = yytext;
			  while (*yptr && *yptr != '[')
			  	yptr++;
			  if (*yptr)
			  	yylval->t_long = atol(yptr + 1); /* skip '[' */
			  BEGIN(INITIAL);
			  return(TOK_TYPE_UNKNOWN);
			}
	{other_audit_type}  { yylval->t_str = strdup(yytext);
			      BEGIN(other_audit);
			      return(TOK_TYPE_OTHER);
			}
	.		{ BEGIN(unknown_message); yyless(0); /* dump the rest */ }
	}

{equals}		{ return(TOK_EQUALS); }
{digits}		{ yylval->t_long = atol(yytext); return(TOK_DIGITS); }
{colon}			{ return(TOK_COLON); }
{minus}			{ return(TOK_MINUS); }
{open_paren}		{
			BEGIN(sub_id);
			return(TOK_OPEN_PAREN);
			}
{close_paren}		{ return(TOK_CLOSE_PAREN); }
{period}		{ return(TOK_PERIOD); }
{question_mark}		{ return(TOK_QUESTION_MARK); }
{single_quote}		{ return(TOK_SINGLE_QUOTE); }

{key_apparmor}		{ BEGIN(audit_types); return(TOK_KEY_APPARMOR); }
{key_type}		{ BEGIN(audit_types); return(TOK_KEY_TYPE); }
{key_msg}		{ return(TOK_KEY_MSG); }
{key_operation}		{ return(TOK_KEY_OPERATION); }
{key_name}		{ BEGIN(safe_string); return(TOK_KEY_NAME); }
{key_name2}		{ BEGIN(safe_string); return(TOK_KEY_NAME2); }
{key_namespace}		{ BEGIN(safe_string); return(TOK_KEY_NAMESPACE); }
{key_mask}		{ return(TOK_KEY_MASK); }
{key_denied_mask}	{ return(TOK_KEY_DENIED_MASK); }
{key_requested_mask}	{ return(TOK_KEY_REQUESTED_MASK); }
{key_attribute}		{ BEGIN(sub_id); return(TOK_KEY_ATTRIBUTE); }
{key_task}		{ return(TOK_KEY_TASK); }
{key_parent}		{ return(TOK_KEY_PARENT); }
{key_magic_token}	{ return(TOK_KEY_MAGIC_TOKEN); }
{key_info}		{ return(TOK_KEY_INFO); }
{key_peer_info}		{ return(TOK_KEY_PEER_INFO); }
{key_pid}		{ return(TOK_KEY_PID); }
{key_peer_pid}		{ return(TOK_KEY_PEER_PID); }
{key_profile}		{ BEGIN(safe_string); return(TOK_KEY_PROFILE); }
{key_peer_profile}	{ BEGIN(safe_string); return(TOK_KEY_PEER_PROFILE); }
{key_family}		{ return(TOK_KEY_FAMILY); }
{key_sock_type}		{ return(TOK_KEY_SOCK_TYPE); }
{key_protocol}		{ return(TOK_KEY_PROTOCOL); }
{key_error}		{ return(TOK_KEY_ERROR); }
{key_fsuid}		{ return(TOK_KEY_FSUID); }
{key_ouid}		{ return(TOK_KEY_OUID); }
{key_uid}		{ return(TOK_KEY_UID); }
{key_auid}		{ return(TOK_KEY_AUID); }
{key_sauid}		{ return(TOK_KEY_SAUID); }
{key_ses}		{ return(TOK_KEY_SES); }
{key_hostname}		{ return(TOK_KEY_HOSTNAME); }
{key_addr}		{ return(TOK_KEY_ADDR); }
{key_terminal}		{ return(TOK_KEY_TERMINAL); }
{key_exe}		{ BEGIN(safe_string); return(TOK_KEY_EXE); }
{key_comm}		{ BEGIN(safe_string); return(TOK_KEY_COMM); }
{key_capability}	{ return(TOK_KEY_CAPABILITY); }
{key_capname}		{ return(TOK_KEY_CAPNAME); }
{key_offset}		{ return(TOK_KEY_OFFSET); }
{key_target}		{ return(TOK_KEY_TARGET); }
{key_laddr}		{ yy_push_state(ip_addr, yyscanner); return(TOK_KEY_LADDR); }
{key_faddr}		{ yy_push_state(ip_addr, yyscanner); return(TOK_KEY_FADDR); }
{key_lport}		{ return(TOK_KEY_LPORT); }
{key_fport}		{ return(TOK_KEY_FPORT); }
{key_bus}		{ return(TOK_KEY_BUS); }
{key_path}		{ return(TOK_KEY_PATH); }
{key_interface}		{ return(TOK_KEY_INTERFACE); }
{key_member}		{ return(TOK_KEY_MEMBER); }
{key_signal}		{ BEGIN(sub_id); return(TOK_KEY_SIGNAL); }
{key_peer}		{ BEGIN(safe_string); return(TOK_KEY_PEER); }
{key_fstype}		{ return(TOK_KEY_FSTYPE); }
{key_flags}		{ BEGIN(safe_string); return(TOK_KEY_FLAGS); }
{key_srcname}		{ BEGIN(safe_string); return(TOK_KEY_SRCNAME); }

{syslog_kernel}		{ BEGIN(dmesg_timestamp); return(TOK_SYSLOG_KERNEL); }
{syslog_user}		{ return(TOK_SYSLOG_USER); }
{syslog_month}		{ yylval->t_str = strdup(yytext); return(TOK_DATE_MONTH); }
{syslog_date}		{ yylval->t_str = strdup(yytext); return(TOK_DATE); }
{syslog_date}T/{syslog_time}	{ yylval->t_str = strndup(yytext, strlen(yytext)-1); return(TOK_DATE); }
{syslog_time}		{ yylval->t_str = strdup(yytext); BEGIN(hostname); return(TOK_TIME); }

{audit}			{ yy_push_state(audit_id, yyscanner); return(TOK_AUDIT); }
{dmesg_timestamp}	{ yylval->t_str = strdup(yytext); return(TOK_DMESG_STAMP); }

.			{ /* ignore any non-matched input */ BEGIN(unknown_message); yyless(0); }

<hostname>{
	{ws}+		{ /* eat whitespace */ }
	{syslog_hostname} { yylval->t_str = strdup(yytext); BEGIN(INITIAL); return(TOK_ID); }
}

<dmesg_timestamp>{
	{ws}+		{ /* eat whitespace */ }
	{dmesg_timestamp} { yylval->t_str = strdup(yytext); BEGIN(INITIAL); return(TOK_DMESG_STAMP); }
	.		{ /* no timestamp in this message */ BEGIN(INITIAL); yyless(0); }
}

<other_audit>{
	{ws}+		{ /* eat whitespace */ }
	{audit}		{ yy_push_state(audit_id, yyscanner); return(TOK_AUDIT); }
	{key_msg}	{ return(TOK_KEY_MSG); }
	{equals}	{ return(TOK_EQUALS); }
	{colon}		{ return(TOK_COLON); }
	.		{ BEGIN(unknown_message); yyless(0); /* dump the rest */ }
}

<unknown_message>{
	.*		{ yylval->t_str = strdup(yytext); return(TOK_MSG_REST); }
	\n		{ /* not sure why needed here and not elsewhere */ }
	}


%%
