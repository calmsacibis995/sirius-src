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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <assert.h>
#include <strings.h>
#include <sys/stat.h>
#include <smbsrv/libsmb.h>
#include <smbsrv/libmlsvc.h>
#include <smbsrv/smbinfo.h>

#define	SMB_AUTOHOME_KEYSIZ	128
#define	SMB_AUTOHOME_MAXARG	4
#define	SMB_AUTOHOME_BUFSIZ	2048

typedef struct smb_autohome_info {
	struct smb_autohome_info *magic1;
	FILE *fp;
	smb_autohome_t autohome;
	char buf[SMB_AUTOHOME_BUFSIZ];
	char *argv[SMB_AUTOHOME_MAXARG];
	int lineno;
	struct smb_autohome_info *magic2;
} smb_autohome_info_t;

static smb_autohome_info_t smb_ai;

static smb_autohome_t *smb_autohome_make_entry(smb_autohome_info_t *);
static char *smb_autohome_keysub(const char *, char *, int);
static smb_autohome_info_t *smb_autohome_getinfo(void);
static smb_autohome_t *smb_autohome_lookup(const char *);
static void smb_autohome_setent(void);
static void smb_autohome_endent(void);
static smb_autohome_t *smb_autohome_getent(const char *);

/*
 * Add an autohome share.  See smb_autohome(4) for details.
 *
 * If share directory contains backslash path separators, they will
 * be converted to forward slash to support NT/DOS path style for
 * autohome shares.
 */
void
smb_autohome_add(const char *username)
{
	smb_share_t si;
	smb_autohome_t *ai;

	assert(username);

	if (smb_shr_get((char *)username, &si) == NERR_Success) {
		/*
		 * autohome shares will be added for each login attempt
		 * even if they already exist
		 */
		if ((si.shr_flags & SMB_SHRF_AUTOHOME) == 0)
			return;

		(void) smb_shr_add(&si, 0);
		return;
	}

	if ((ai = smb_autohome_lookup(username)) == NULL)
		return;

	bzero(&si, sizeof (smb_share_t));
	(void) strlcpy(si.shr_path, ai->ah_path, MAXPATHLEN);
	(void) strsubst(si.shr_path, '\\', '/');

	if (smb_shr_is_dir(si.shr_path) == 0)
		return;

	(void) strlcpy(si.shr_name, username, MAXNAMELEN);
	(void) strlcpy(si.shr_container, ai->ah_container, MAXPATHLEN);
	si.shr_flags = SMB_SHRF_TRANS | SMB_SHRF_AUTOHOME;

	(void) smb_shr_add(&si, 0);
}

/*
 * Remove an autohome share.
 */
void
smb_autohome_remove(const char *username)
{
	smb_share_t si;

	assert(username);

	if (smb_shr_get((char *)username, &si) == NERR_Success) {
		if (si.shr_flags & SMB_SHRF_AUTOHOME) {
			(void) smb_shr_del((char *)username, 0);
		}
	}
}

/*
 * Find out if a share is an autohome share.
 */
boolean_t
smb_is_autohome(const smb_share_t *si)
{
	return (si && (si->shr_flags & SMB_SHRF_AUTOHOME));
}

/*
 * Search the autohome database for the specified name. The name cannot
 * be an empty string or begin with * or +.
 * 1. Search the file for the specified name.
 * 2. Check for the wildcard rule and, if present, treat it as a match.
 * 3. Check for the nsswitch rule and, if present, lookup the name
 *    via the name services. Note that the nsswitch rule will never
 *    be applied if the wildcard rule is present.
 *
 * Returns a pointer to the entry on success or null on failure.
 */
static smb_autohome_t *
smb_autohome_lookup(const char *name)
{
	struct passwd *pw;
	smb_autohome_t *ah = NULL;

	if (name == NULL)
		return (NULL);

	if (*name == '\0' || *name == '*' || *name == '+')
		return (NULL);

	smb_autohome_setent();

	while ((ah = smb_autohome_getent(name)) != NULL) {
		if (strcasecmp(ah->ah_name, name) == 0)
			break;
	}

	if (ah == NULL) {
		smb_autohome_setent();

		while ((ah = smb_autohome_getent(name)) != NULL) {
			if (strcasecmp(ah->ah_name, "*") == 0) {
				ah->ah_name = (char *)name;
				break;
			}
		}
	}

	if (ah == NULL) {
		smb_autohome_setent();

		while ((ah = smb_autohome_getent("+nsswitch")) != NULL) {
			if (strcasecmp("+nsswitch", ah->ah_name) != 0)
				continue;
			if ((pw = getpwnam(name)) == NULL) {
				ah = NULL;
				break;
			}

			ah->ah_name = pw->pw_name;

			if (ah->ah_path)
				ah->ah_container = ah->ah_path;

			ah->ah_path = pw->pw_dir;
			break;
		}
	}

	smb_autohome_endent();
	return (ah);
}

/*
 * Open or rewind the autohome database.
 */
static void
smb_autohome_setent(void)
{
	smb_autohome_info_t *si;
	char path[MAXNAMELEN];
	char filename[MAXNAMELEN];
	int rc;

	if ((si = smb_autohome_getinfo()) != 0) {
		(void) fseek(si->fp, 0L, SEEK_SET);
		si->lineno = 0;
		return;
	}

	if ((si = &smb_ai) == 0)
		return;

	rc = smb_config_getstr(SMB_CI_AUTOHOME_MAP, path, sizeof (path));
	if (rc != SMBD_SMF_OK)
		return;

	(void) snprintf(filename, MAXNAMELEN, "%s/%s", path,
	    SMB_AUTOHOME_FILE);

	if ((si->fp = fopen(filename, "r")) == NULL)
		return;

	si->magic1 = si;
	si->magic2 = si;
	si->lineno = 0;
}

/*
 * Close the autohome database and invalidate the autohome info.
 * We can't zero the whole info structure because the application
 * should still have access to the data after the file is closed.
 */
static void
smb_autohome_endent(void)
{
	smb_autohome_info_t *si;

	if ((si = smb_autohome_getinfo()) != 0) {
		(void) fclose(si->fp);
		si->fp = 0;
		si->magic1 = 0;
		si->magic2 = 0;
	}
}

/*
 * Return the next entry in the autohome database, opening the file
 * if necessary.  Returns null on EOF or error.
 *
 * Note that we are not looking for the specified name. The name is
 * only used for key substitution, so that the caller sees the entry
 * in expanded form.
 */
static smb_autohome_t *
smb_autohome_getent(const char *name)
{
	smb_autohome_info_t *si;
	char *bp;

	if ((si = smb_autohome_getinfo()) == 0) {
		smb_autohome_setent();

		if ((si = smb_autohome_getinfo()) == 0)
			return (0);
	}

	/*
	 * Find the next non-comment, non-empty line.
	 * Anything after a # is a comment and can be discarded.
	 * Discard a newline to avoid it being included in the parsing
	 * that follows.
	 * Leading and training whitespace is discarded, and replicated
	 * whitespace is compressed to simplify the token parsing,
	 * although strsep() deals with that better than strtok().
	 */
	do {
		if (fgets(si->buf, SMB_AUTOHOME_BUFSIZ, si->fp) == 0)
			return (0);

		++si->lineno;

		if ((bp = strpbrk(si->buf, "#\r\n")) != 0)
			*bp = '\0';

		(void) trim_whitespace(si->buf);
		bp = strcanon(si->buf, " \t");
	} while (*bp == '\0');

	(void) smb_autohome_keysub(name, si->buf, SMB_AUTOHOME_BUFSIZ);
	return (smb_autohome_make_entry(si));
}

/*
 * Set up an autohome entry from the line buffer. The line should just
 * contain tokens separated by single whitespace. The line format is:
 *	<username> <home-dir-path> <ADS container>
 */
static smb_autohome_t *
smb_autohome_make_entry(smb_autohome_info_t *si)
{
	char *bp;
	int i;

	bp = si->buf;

	for (i = 0; i < SMB_AUTOHOME_MAXARG; ++i)
		si->argv[i] = 0;

	for (i = 0; i < SMB_AUTOHOME_MAXARG; ++i) {
		do {
			if ((si->argv[i] = strsep((char **)&bp, " \t")) == 0)
				break;
		} while (*(si->argv[i]) == '\0');

		if (si->argv[i] == 0)
			break;
	}

	if ((si->autohome.ah_name = si->argv[0]) == NULL) {
		/*
		 * Sanity check: the name could be an empty
		 * string but it can't be a null pointer.
		 */
		return (0);
	}

	if ((si->autohome.ah_path = si->argv[1]) == NULL)
		si->autohome.ah_path = "";

	if ((si->autohome.ah_container = si->argv[2]) == NULL)
		si->autohome.ah_container = "";

	return (&si->autohome);
}

/*
 * Substitute the ? and & map keys.
 * ? is replaced by the first character of the name
 * & is replaced by the whole name.
 */
static char *
smb_autohome_keysub(const char *name, char *buf, int buflen)
{
	char key[SMB_AUTOHOME_KEYSIZ];
	char *ampersand;
	char *tmp;
	int bufsize = buflen;

	(void) strlcpy(key, buf, SMB_AUTOHOME_KEYSIZ);

	if ((tmp = strpbrk(key, " \t")) == NULL)
		return (NULL);

	*tmp = '\0';

	/*
	 * Substitution characters are not allowed in the key.
	 */
	if (strpbrk(key, "?&") != NULL)
		return (NULL);

	if (strcmp(key, "*") == 0 && name != NULL)
		(void) strlcpy(key, name, SMB_AUTOHOME_KEYSIZ);

	(void) strsubst(buf, '?', *key);

	while ((ampersand = strchr(buf, '&')) != NULL) {
		if ((tmp = strdup(ampersand + 1)) == NULL)
			return (0);

		bufsize = buflen - (ampersand - buf);
		(void) strlcpy(ampersand, key, bufsize);
		(void) strlcat(ampersand, tmp, bufsize);
		free(tmp);
	}

	return (buf);
}

/*
 * Get a pointer to the context buffer and validate it.
 */
static smb_autohome_info_t *
smb_autohome_getinfo(void)
{
	smb_autohome_info_t *si;

	if ((si = &smb_ai) == 0)
		return (0);

	if ((si->magic1 == si) && (si->magic2 == si) && (si->fp != NULL))
		return (si);

	return (0);
}
