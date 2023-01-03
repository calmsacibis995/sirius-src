/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/types.h>
#include <sys/varargs.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>

#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/pam_impl.h>

#include <libintl.h>

#include <passwdutil.h>

#define	SUNW_OLDRPCPASS "SUNW-OLD-RPC-PASSWORD"

/*PRINTFLIKE3*/
static void
error(int nowarn, pam_handle_t *pamh, char *fmt, ...)
{
	va_list ap;
	char messages[PAM_MAX_NUM_MSG][PAM_MAX_MSG_SIZE];

	va_start(ap, fmt);
	(void) vsnprintf(messages[0], sizeof (messages[0]), fmt, ap);
	if (nowarn == 0)
		(void) __pam_display_msg(pamh, PAM_ERROR_MSG, 1, messages,
		    NULL);
	va_end(ap);
}

/*PRINTFLIKE3*/
static void
info(int nowarn, pam_handle_t *pamh, char *fmt, ...)
{
	va_list ap;
	char messages[PAM_MAX_NUM_MSG][PAM_MAX_MSG_SIZE];

	va_start(ap, fmt);
	(void) vsnprintf(messages[0], sizeof (messages[0]), fmt, ap);
	if (nowarn == 0)
		(void) __pam_display_msg(pamh, PAM_TEXT_INFO, 1, messages,
		    NULL);
	va_end(ap);
}

#if defined(ENABLE_AGING)
/*
 * test if authtok is aged.
 * returns 1 if it is, 0 otherwise
 */
static int
authtok_is_aged(pam_handle_t *pamh)
{
	unix_authtok_data *status;

	if (pam_get_data(pamh, UNIX_AUTHTOK_DATA,
			(const void **)status) != PAM_SUCCESS)
		return (0);

	return (status->age_status == PAM_NEW_AUTHTOK_REQD)
}
#endif

int
pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int i;
	int debug = 0;
	int nowarn = 0;
	attrlist l;
	pwu_repository_t *pwu_rep;
	char *user;
	char *oldpw;
	char *oldrpcpw = "*NP*";
	char *newpw;
	char *service;
	struct pam_repository *auth_rep;
	int res;
	char msg[PAM_MAX_NUM_MSG][PAM_MAX_MSG_SIZE];
	int updated_reps = 0;
	int server_policy = 0;

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "debug") == 0)
			debug = 1;
		else if (strcmp(argv[i], "nowarn") == 0)
			nowarn = 1;
		else if (strcmp(argv[i], "server_policy") == 0)
			server_policy = 1;
	}

	if ((flags & PAM_PRELIM_CHECK) != 0)
		return (PAM_IGNORE);

	if ((flags & PAM_UPDATE_AUTHTOK) == 0)
		return (PAM_SYSTEM_ERR);

	if ((flags & PAM_SILENT) != 0)
		nowarn = 1;

	if (debug)
		syslog(LOG_DEBUG, "pam_authtok_store: storing authtok");

#if defined(ENABLE_AGING)
	if ((flags & PAM_CHANGE_EXPIRED_AUTHTOK) && !authtok_is_aged(pamh)) {
		syslog(LOG_DEBUG, "pam_authtok_store: System password young");
		return (PAM_IGNORE)
	}
#endif

	res = pam_get_item(pamh, PAM_SERVICE, (void **)&service);
	if (res != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_authtok_store: error getting SERVICE");
		return (PAM_SYSTEM_ERR);
	}

	res = pam_get_item(pamh, PAM_USER, (void **)&user);
	if (res != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_authtok_store: error getting USER");
		return (PAM_SYSTEM_ERR);
	}

	if (user == NULL || *user == '\0') {
		syslog(LOG_ERR, "pam_authtok_store: username is empty");
		return (PAM_USER_UNKNOWN);
	}

	res = pam_get_item(pamh, PAM_OLDAUTHTOK, (void **)&oldpw);
	if (res != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_authtok_store: error getting OLDAUTHTOK");
		return (PAM_SYSTEM_ERR);
	}

	res = pam_get_item(pamh, PAM_AUTHTOK, (void **)&newpw);
	if (res != PAM_SUCCESS || newpw == NULL) {
		/*
		 * A module on the stack has removed PAM_AUTHTOK. We fail
		 */
		return (PAM_SYSTEM_ERR);
	}

	l.data.val_s = newpw;
	/*
	 * If the server_policy option is specified,
	 * use the special attribute, ATTR_PASSWD_SERVER_POLICY,
	 * to tell the update routine for each repository
	 * to perform the necessary special operations.
	 * For now, only the LDAP routine treats this attribute
	 * differently that ATTR_PASSWD. It will skip the
	 * crypting of the password before storing it in the LDAP
	 * server. NIS, NISPLUS, and FILES will handle
	 * ATTR_PASSWD_SERVER_POLICY the same as ATTR_PASSWD.
	 */
	if (server_policy)
		l.type = ATTR_PASSWD_SERVER_POLICY;
	else
		l.type = ATTR_PASSWD;
	l.next = NULL;

	res = pam_get_item(pamh, PAM_REPOSITORY, (void **)&auth_rep);
	if (res != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_authtok_store: error getting repository");
		return (PAM_SYSTEM_ERR);
	}

	if (auth_rep == NULL) {
		pwu_rep = PWU_DEFAULT_REP;
	} else {
		if ((pwu_rep = calloc(1, sizeof (*pwu_rep))) == NULL)
			return (PAM_BUF_ERR);
		pwu_rep->type = auth_rep->type;
		pwu_rep->scope = auth_rep->scope;
		pwu_rep->scope_len = auth_rep->scope_len;
	}

	/*
	 * The pam_dhkeys module might have set SUNW_OLDRPCPASS if it
	 * discovered that the user's old password doesn't decrypt the
	 * user's secure RPC credentials. In that case, the
	 * item SUNW_OLDRPCPASS contains the correct password to
	 * decrypt these credentials.
	 */

	res = pam_get_data(pamh, SUNW_OLDRPCPASS, (const void **)&oldrpcpw);
	if (res != PAM_SUCCESS && res != PAM_NO_MODULE_DATA) {
		syslog(LOG_ERR, "pam_authtok_store: error getting OLDRPCPASS");
		return (PAM_SYSTEM_ERR);
	}

	res = __set_authtoken_attr(user, oldpw, oldrpcpw, pwu_rep, &l,
	    &updated_reps);

	if (pwu_rep != PWU_DEFAULT_REP)
		free(pwu_rep);
	/*
	 * now map the various passwdutil return states to user messages
	 * and PAM return codes.
	 */
	switch (res) {
	case PWU_SUCCESS:
		for (i = 1; i <= REP_LAST; i <<= 1) {
			if ((updated_reps & i) == 0)
				continue;
			info(nowarn, pamh, dgettext(TEXT_DOMAIN,
			    "%s: password successfully changed for %s"),
			    service, user);
		}

		/*
		 * If we have updated NIS+, and we got SUCCESS (not one of
		 * the partial failures), this indicates that the credential
		 * update went well too... Inform the user
		 */
		if (updated_reps & REP_NISPLUS)
			info(nowarn, pamh, dgettext(TEXT_DOMAIN,
			    "%s: credential information changed for %s"),
			    service, user);
		res = PAM_SUCCESS;
		break;
	case PWU_BUSY:
		error(nowarn, pamh, dgettext(TEXT_DOMAIN,
			"%s: Password database busy. Try again later."),
			service);
		res = PAM_AUTHTOK_LOCK_BUSY;
		break;
	case PWU_STAT_FAILED:
		syslog(LOG_ERR, "%s: stat of password file failed", service);
		res = PAM_AUTHTOK_ERR;
		break;
	case PWU_OPEN_FAILED:
	case PWU_WRITE_FAILED:
	case PWU_CLOSE_FAILED:
	case PWU_UPDATE_FAILED:
		error(nowarn, pamh, dgettext(TEXT_DOMAIN,
		    "%s: Unexpected failure. Password database unchanged."),
		    service);
		res = PAM_SYSTEM_ERR;
		break;
	case PWU_NOT_FOUND:
		/* Different error if repository was explicitly specified */
		if (auth_rep != NULL) {
			error(nowarn, pamh, dgettext(TEXT_DOMAIN,
				"%s: System error: no %s password for %s."),
				service, auth_rep->type, user);
		} else {
			error(nowarn, pamh, dgettext(TEXT_DOMAIN,
			    "%s: %s does not exist."), service, user);
		}
		res = PAM_USER_UNKNOWN;
		break;
	case PWU_NOMEM:
		error(nowarn, pamh, dgettext(TEXT_DOMAIN,
			"%s: Internal memory allocation failure."), service);
		res = PAM_BUF_ERR;
		break;
	case PWU_SERVER_ERROR:
		res = PAM_SYSTEM_ERR;
		break;
	case PWU_SYSTEM_ERROR:
		res = PAM_SYSTEM_ERR;
		break;
	case PWU_DENIED:
		res = PAM_PERM_DENIED;
		break;
	case PWU_NO_CHANGE:
		/*
		 * yppasswdd detected that we're not changing anything.
		 */
		info(nowarn, pamh, dgettext(TEXT_DOMAIN,
		    "%s: Password information unchanged."), service);
		res = PAM_SUCCESS;
		break;
	case PWU_REPOSITORY_ERROR:
		syslog(LOG_NOTICE, "pam_authtok_store: detected "
		    "unsupported configuration in /etc/nsswitch.conf.");
		error(nowarn, pamh, dgettext(TEXT_DOMAIN,
		    "%s: System error: repository out of range."), service);
		res = PAM_SYSTEM_ERR;
		break;
	case PWU_RECOVERY_ERR:
		res = PAM_AUTHTOK_RECOVERY_ERR;
		break;
	case PWU_NO_PRIV_CRED_UPDATE:
		/*
		 * A privileged process has updated a user's password.
		 * In this case, the password will be updated, but the
		 * credentials won't. This is not a failure, but we need
		 * to inform the user about it, and return PAM_SUCCESS
		 */

		/* First inform the user about the passsword update */
		info(nowarn, pamh, dgettext(TEXT_DOMAIN,
			"%s: password successfully changed for %s"),
			service, user);

		/* and now the bad news */
		(void) sprintf(msg[0], " ");
		(void) snprintf(msg[1], sizeof (msg[1]),
		    dgettext(TEXT_DOMAIN,
			"The Secure RPC credential information for %s "
			"will not be changed."), user);
		(void) snprintf(msg[2], sizeof (msg[2]),
		    dgettext(TEXT_DOMAIN, "User %s must do the following to "
		    "update his/her"), user);
		(void) snprintf(msg[3], sizeof (msg[3]),
		    dgettext(TEXT_DOMAIN, "credential information:"));
		(void) snprintf(msg[4], sizeof (msg[4]),
		    dgettext(TEXT_DOMAIN, "Use NEW passwd for login and OLD "
		    "passwd for keylogin."));
		(void) snprintf(msg[5], sizeof (msg[5]),
		    dgettext(TEXT_DOMAIN, "Use \"chkey -p\" to reencrypt the "
		    "credentials with the"));
		(void) snprintf(msg[6], sizeof (msg[6]),
		    dgettext(TEXT_DOMAIN, "new login passwd."));
		(void) snprintf(msg[7], sizeof (msg[7]),
		    dgettext(TEXT_DOMAIN, "The user must keylogin explicitly "
		    "after their next login."));
		(void) sprintf(msg[8], " ");
		(void) __pam_display_msg(pamh, PAM_ERROR_MSG, 9, msg, NULL);
		res = PAM_SUCCESS;
		break;
	case PWU_UPDATED_SOME_CREDS:
		info(nowarn, pamh, dgettext(TEXT_DOMAIN,
			"%s: password successfully changed for %s"),
			service, user);

		error(nowarn, pamh, dgettext(TEXT_DOMAIN,
		    "WARNING: some but not all credentials were reencrypted "
		    "for user %s"), user);
		res = PAM_SUCCESS;
		break;
	case PWU_PWD_TOO_SHORT:
		(void) snprintf(msg[0], sizeof (msg[0]),
		    dgettext(TEXT_DOMAIN, "%s: Password too short."), service);
		(void) __pam_display_msg(pamh, PAM_ERROR_MSG, 1, msg, NULL);
		res = PAM_AUTHTOK_ERR;
		break;
	case PWU_PWD_INVALID:
		(void) snprintf(msg[0], sizeof (msg[0]),
		    dgettext(TEXT_DOMAIN, "%s: Invalid password syntax."),
			service);
		(void) __pam_display_msg(pamh, PAM_ERROR_MSG, 1, msg, NULL);
		res = PAM_AUTHTOK_ERR;
		break;
	case PWU_PWD_IN_HISTORY:
		(void) snprintf(msg[0], sizeof (msg[0]),
		    dgettext(TEXT_DOMAIN, "%s: Reuse of old passwords not "
			"allowed, the new password is in the history list."),
			service);
		(void) __pam_display_msg(pamh, PAM_ERROR_MSG, 1, msg, NULL);
		res = PAM_AUTHTOK_ERR;
		break;
	case PWU_CHANGE_NOT_ALLOWED:
		(void) snprintf(msg[0], sizeof (msg[0]),
		    dgettext(TEXT_DOMAIN, "%s: You may not change "
			"this password."), service);
		(void) __pam_display_msg(pamh, PAM_ERROR_MSG, 1, msg, NULL);
		res = PAM_PERM_DENIED;
		break;
	case PWU_WITHIN_MIN_AGE:
		(void) snprintf(msg[0], sizeof (msg[0]),
		    dgettext(TEXT_DOMAIN,
			"%s: Password can not be changed yet, "
			"not enough time has passed."), service);
		(void) __pam_display_msg(pamh, PAM_ERROR_MSG, 1, msg, NULL);
		res = PAM_PERM_DENIED;
		break;
	default:
		res = PAM_SYSTEM_ERR;
		break;
	}

	return (res);
}
