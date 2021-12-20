/* pam_apparmor module */

/*
 * Copyright (c) 2006
 * NOVELL (All rights reserved)
 *
 * Copyright (c) 2010
 * Canonical, Ltd. (All rights reserved)
 *
 * Written by Jesse Michael <jmichael@suse.de> 2006/08/24
 *	  and Steve Beattie <sbeattie@ubuntu.com> 2006/10/25
 *
 * Based off of pam_motd by:
 *   Ben Collins <bcollins@debian.org> 2005/10/04
 *   Michael K. Johnson <johnsonm@redhat.com> 1996/10/24
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <errno.h>
#include <sys/apparmor.h>
#include <security/pam_ext.h>
#include <security/pam_modutil.h>

/*
 * here, we make a definition for the externally accessible function
 * in this file (this definition is required for static a module
 * but strongly encouraged generally) it is used to instruct the
 * modules include file to define the function prototypes.
 */

#define PAM_SM_SESSION
#include <security/pam_modules.h>

#include "pam_apparmor.h"

int debug_flag = 0;

static struct config default_config = {
	.hat_type[0] = eGroupname,
	.hat_type[1] = eDefault,
	.hat_type[2] = eNoEntry,
};

/* --- session management functions (only) --- */

PAM_EXTERN int
pam_sm_close_session (pam_handle_t *pamh, int flags,
		      int argc, const char **argv)
{
	return PAM_IGNORE;
}

PAM_EXTERN
int pam_sm_open_session(pam_handle_t *pamh, int flags,
			int argc, const char **argv)
{
	int fd, retval, pam_retval = PAM_SUCCESS;
	unsigned int magic_token;
	const char *user;
	struct passwd *pw;
	struct group *gr;
	struct config *config = NULL;
	int i;

	if ((retval = get_options(pamh, &config, argc, argv)) != 0)
		return retval;

	if (!config)
		config = &default_config;

	/* grab the target user name */
	retval = pam_get_user(pamh, &user, NULL);
	if (retval != PAM_SUCCESS || user == NULL || *user == '\0') {
		pam_syslog(pamh, LOG_ERR, "Can't determine user\n");
		return PAM_USER_UNKNOWN;
	}

	pw = pam_modutil_getpwnam(pamh, user);
	if (!pw) {
		pam_syslog(pamh, LOG_ERR, "Can't determine group for user %s\n", user);
		return PAM_PERM_DENIED;
	}

	gr = pam_modutil_getgrgid(pamh, pw->pw_gid);
	if (!gr || !gr->gr_name) {
		pam_syslog(pamh, LOG_ERR, "Can't read info for group %d\n", pw->pw_gid);
		return PAM_PERM_DENIED;
	}

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		pam_syslog(pamh, LOG_ERR, "Can't open /dev/urandom\n");
		return PAM_PERM_DENIED;
	}

	/* the magic token needs to be non-zero otherwise, we won't be able
	 * to probe for hats */
	do {
		retval = pam_modutil_read(fd,
					  (void *)&magic_token,
					  sizeof(magic_token));
		if (retval < 0) {
			pam_syslog(pamh, LOG_ERR, "Can't read from /dev/urandom\n");
			close(fd);
			return PAM_PERM_DENIED;
		}
	} while ((magic_token == 0) || (retval != sizeof(magic_token)));

	close(fd);

	pam_retval = PAM_SUCCESS;
	for (i = 0; i < MAX_HAT_TYPES && config->hat_type[i] != eNoEntry; i++) {
		const char *hat = NULL;
		switch (config->hat_type[i]) {
		case eGroupname:
			hat = gr->gr_name;
			if (debug_flag)
				pam_syslog(pamh, LOG_DEBUG, "Using groupname '%s'\n", hat);
			break;
		case eUsername:
			hat = user;
			if (debug_flag)
				pam_syslog(pamh, LOG_DEBUG, "Using username '%s'\n", hat);
			break;
		case eDefault:
			if (debug_flag)
				pam_syslog(pamh, LOG_DEBUG, "Using DEFAULT\n");
			hat = "DEFAULT";
			break;
		default:
			pam_syslog(pamh, LOG_ERR, "Unknown value in hat table: %x\n",
					config->hat_type[i]);
			goto nodefault;
			break;
		}

		retval = change_hat(hat, magic_token);
		if (retval == 0) {
			/* success, let's bail */
			if (debug_flag)
				pam_syslog(pamh, LOG_DEBUG, "Successfully changed to hat '%s'\n", hat);
			goto out;
		}

		switch (errno) {

		/* case EPERM: */ /* Can't enable until ECHILD patch gets accepted, and we can
				   * distinguish between unconfined and confined-but-no-hats */
		case EINVAL:
			/* apparmor is not loaded or application is unconfined,
			 * stop attempting to use change_hat */
			if (debug_flag)
				pam_syslog(pamh, LOG_DEBUG,
				   "AppArmor not loaded, or application is unconfined\n");
			pam_retval = PAM_SUCCESS;
			goto out;
			break;
		case ECHILD:
			/* application is confined but has no hats,
			 * stop attempting to use change_hat */
			goto nodefault;
			break;
		case EACCES:
		case ENOENT:
			/* failed to change into attempted hat, so we'll
			 * jump back out and try the next one */
			break;
		default:
			pam_syslog(pamh, LOG_ERR, "Unknown error occurred changing to %s hat: %s\n",
					hat, strerror(errno));
			/* give up? */
			pam_retval = PAM_SYSTEM_ERR;
			goto out;
		}

		retval = change_hat(NULL, magic_token);
		if (retval != 0) {
			/* changing into the specific hat and attempting to
	 		 * jump back out both failed.  that most likely
	 		 * means that either apparmor is not loaded or we
	 		 * don't have a profile loaded for this application.
	 		 * in this case, we want to allow the pam operation
	 		 * to succeed. */
			goto out;
		}

	}

nodefault:
	/* if we got here, we were unable to change into any of the hats
	 * we attempted. */
	pam_syslog(pamh, LOG_ERR, "Can't change to any hat\n");
	pam_retval = PAM_SESSION_ERR;

out:
	/* zero out the magic token so an attacker wouldn't be able to
	 * just grab it out of process memory and instead would need to
	 * brute force it */
	memset(&magic_token, 0, sizeof(magic_token));
	if (config && config != &default_config)
		free(config);
	return pam_retval;
}

#ifdef PAM_STATIC

/* static module data */

struct pam_module _pam_apparmor_modstruct = {
     "pam_apparmor",
     NULL,
     NULL,
     NULL,
     pam_sm_open_session,
     pam_sm_close_session,
     NULL,
};

#endif

/* end of module definition */
