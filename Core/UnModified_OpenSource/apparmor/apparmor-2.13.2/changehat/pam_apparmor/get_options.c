/*
 * Written by Steve Beattie <sbeattie@suse.de> 2006/10/25
 *
 * Modeled after the option parsing code in pam_unix2 by:
 *	Copyright (c) 2006 SUSE Linux Products GmbH, Nuernberg, Germany.
 *	Copyright (c) 2002, 2003, 2004 SuSE GmbH Nuernberg, Germany.
 *	Author: Thorsten Kukuk <kukuk@suse.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE	/* for strndup() */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <security/pam_ext.h>

#define PAM_SM_SESSION
#include <security/pam_modules.h>

#include "pam_apparmor.h"

#define DEBUG_STRING "debug"
#define ORDER_PREFIX "order="

static int parse_option(pam_handle_t *pamh, struct config **config, const char *argv)
{
	const char *opts;

	if (argv == NULL || argv[0] == '\0')
		return 0;

	if (strcasecmp(argv, DEBUG_STRING) == 0) {
		debug_flag = 1;
		return 0;
	} else if (strncasecmp(argv, ORDER_PREFIX, strlen(ORDER_PREFIX)) != 0) {
		pam_syslog (pamh, LOG_ERR, "Unknown option: `%s'\n", argv);
		return PAM_SESSION_ERR;
	}

	opts = argv + strlen(ORDER_PREFIX);

	while (*opts != '\0') {
		hat_t hat;
		char *opt, *comma;
		int i;

		comma = index(opts, ',');
		if (comma)
			opt = strndup(opts, comma - opts);
		else
			opt = strdup(opts);

		if (!opt) {
			pam_syslog(pamh, LOG_ERR, "Memory allocation error: %s",
					strerror(errno));
			return PAM_SESSION_ERR;
		}

		if (strcasecmp(opt, "group") == 0)
			hat = eGroupname;
		else if (strcasecmp(opt, "user") == 0)
			hat = eUsername;
		else if (strcasecmp(opt, "default") == 0)
			hat = eDefault;
		else {
			pam_syslog (pamh, LOG_ERR, "Unknown option: `%s'\n", opt);
			free(opt);
			return PAM_SESSION_ERR;
		}

		if (!(*config)) {
			struct config *new_cfg = malloc(sizeof(**config));
			if (!new_cfg) {
				pam_syslog(pamh, LOG_ERR, "Memory allocation error: %s",
						strerror(errno));
				free(opt);
				return PAM_SESSION_ERR;
			}
			new_cfg->hat_type[0] = eNoEntry;
			new_cfg->hat_type[1] = eNoEntry;
			new_cfg->hat_type[2] = eNoEntry;
			(*config) = new_cfg;
		}

		/* Find free table entry, looking for duplicates */
		for (i = 0; i < MAX_HAT_TYPES && (*config)->hat_type[i] != eNoEntry; i++) {
			if ((*config)->hat_type[i] == hat) {
				pam_syslog(pamh, LOG_ERR, "Duplicate hat type: %s\n", opt);
				free(opt);
				free(*config);
				(*config) = NULL;
				return PAM_SESSION_ERR;
			}
		}
		if (i >= MAX_HAT_TYPES) {
			pam_syslog(pamh, LOG_ERR, "Unable to add hat type '%s'\n", opt);
			return PAM_SESSION_ERR;
		}
		(*config)->hat_type[i] = hat;
		free(opt);

		if (comma)
			opts = comma + 1;
		else
			opts += strlen(opts);

	}

	return 0;
}

int get_options(pam_handle_t *pamh, struct config **config, int argc, const char **argv)
{
	int retval = 0;

	/* Parse parameters for module */
	for ( ; argc-- > 0; argv++) {
		int rc = parse_option(pamh, config, *argv);
		if (rc != 0)
			retval = rc;
	}

	return retval;
}
