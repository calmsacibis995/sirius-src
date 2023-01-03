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

#include <pthread.h>
#include <synch.h>
#include <strings.h>
#include <stdlib.h>
#include <libshare.h>
#include <smbsrv/smb_share.h>

#include <syslog.h>

static pthread_mutex_t smb_group_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Sharemanager shared API */

void
smb_build_lmshare_info(char *share_name, char *path,
    sa_resource_t resource, smb_share_t *si)
{
	sa_property_t prop;
	char *val = NULL;
	sa_optionset_t opts;
	sa_share_t share;

	bzero(si, sizeof (smb_share_t));
	/* Share is read from SMF so it should be permanent */
	si->shr_flags = SMB_SHRF_PERM;

	(void) strlcpy(si->shr_path, path, sizeof (si->shr_path));
	(void) strlcpy(si->shr_name, share_name, sizeof (si->shr_name));

	val = sa_get_resource_description(resource);
	if (val == NULL) {
		share = sa_get_resource_parent(resource);
		val = sa_get_resource_description(share);
	}

	if (val != NULL) {
		(void) strlcpy(si->shr_cmnt, val, sizeof (si->shr_cmnt));
		sa_free_share_description(val);
	}

	opts = sa_get_derived_optionset(resource, SMB_PROTOCOL_NAME, 1);
	if (opts == NULL)
		return;

	prop = (sa_property_t)sa_get_property(opts, SMB_SHROPT_AD_CONTAINER);
	if (prop != NULL) {
		if ((val = sa_get_property_attr(prop, "value")) != NULL) {
			(void) strlcpy(si->shr_container, val,
			    sizeof (si->shr_container));
			free(val);
		}
	}
	sa_free_derived_optionset(opts);
}

/*
 * smb_get_smb_share_group
 *
 * Creates "smb" share group for putting in shares
 * created by windows client.
 */
sa_group_t
smb_get_smb_share_group(sa_handle_t handle)
{
	sa_group_t group = NULL;
	int err;

	(void) pthread_mutex_lock(&smb_group_mutex);
	group = sa_get_group(handle, SMB_DEFAULT_SHARE_GROUP);
	if (group != NULL) {
		(void) pthread_mutex_unlock(&smb_group_mutex);
		return (group);
	}
	group = sa_create_group(handle, SMB_DEFAULT_SHARE_GROUP, &err);
	if (group == NULL) {
		(void) pthread_mutex_unlock(&smb_group_mutex);
		return (NULL);
	}
	if (group != NULL) {
		if (sa_create_optionset(group,
		    SMB_DEFAULT_SHARE_GROUP) == NULL) {
			(void) sa_remove_group(group);
			group = NULL;
		}
	}
	(void) pthread_mutex_unlock(&smb_group_mutex);
	return (group);
}
