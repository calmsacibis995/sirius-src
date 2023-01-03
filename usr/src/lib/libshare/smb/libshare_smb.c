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

/*
 * SMB specific functions
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <zone.h>
#include <errno.h>
#include <locale.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include "libshare.h"
#include "libshare_impl.h"
#include <pwd.h>
#include <limits.h>
#include <libscf.h>
#include <strings.h>
#include "libshare_smb.h"
#include <rpcsvc/daemon_utils.h>
#include <smbsrv/smb_share.h>
#include <smbsrv/smbinfo.h>
#include <smbsrv/libsmb.h>

/* internal functions */
static int smb_share_init(void);
static void smb_share_fini(void);
static int smb_enable_share(sa_share_t);
static int smb_share_changed(sa_share_t);
static int smb_resource_changed(sa_resource_t);
static int smb_rename_resource(sa_handle_t, sa_resource_t, char *);
static int smb_disable_share(sa_share_t share, char *);
static int smb_validate_property(sa_handle_t, sa_property_t, sa_optionset_t);
static int smb_set_proto_prop(sa_property_t);
static sa_protocol_properties_t smb_get_proto_set(void);
static char *smb_get_status(void);
static int smb_parse_optstring(sa_group_t, char *);
static char *smb_format_options(sa_group_t, int);

static int smb_enable_service(void);

static int range_check_validator(int, char *);
static int range_check_validator_zero_ok(int, char *);
static int string_length_check_validator(int, char *);
static int true_false_validator(int, char *);
static int ip_address_validator_empty_ok(int, char *);
static int ip_address_csv_list_validator_empty_ok(int, char *);
static int path_validator(int, char *);

static int smb_enable_resource(sa_resource_t);
static int smb_disable_resource(sa_resource_t);
static uint64_t smb_share_features(void);
static int smb_list_transient(sa_handle_t);

/* size of basic format allocation */
#define	OPT_CHUNK	1024

/* size of string for types - big enough to hold "dependency" */
#define	SCFTYPE_LEN	32

/*
 * Indexes of entries in smb_proto_options table.
 * Changes to smb_proto_options table may require
 * an update to these values.
 */
#define	PROTO_OPT_WINS1			6
#define	PROTO_OPT_WINS_EXCLUDE		8


/*
 * ops vector that provides the protocol specific info and operations
 * for share management.
 */

struct sa_plugin_ops sa_plugin_ops = {
	SA_PLUGIN_VERSION,
	SMB_PROTOCOL_NAME,
	smb_share_init,
	smb_share_fini,
	smb_enable_share,
	smb_disable_share,
	smb_validate_property,
	NULL,	/* valid_space */
	NULL,	/* security_prop */
	smb_parse_optstring,
	smb_format_options,
	smb_set_proto_prop,
	smb_get_proto_set,
	smb_get_status,
	NULL,	/* space_alias */
	NULL,	/* update_legacy */
	NULL,	/* delete_legacy */
	smb_share_changed,
	smb_enable_resource,
	smb_disable_resource,
	smb_share_features,
	smb_list_transient,
	smb_resource_changed,
	smb_rename_resource,
	NULL,	/* run_command */
	NULL,	/* command_help */
	NULL	/* delete_proto_section */
};

/*
 * option definitions.  Make sure to keep the #define for the option
 * index just before the entry it is the index for. Changing the order
 * can cause breakage.
 */

struct option_defs optdefs[] = {
	{SMB_SHROPT_AD_CONTAINER, OPT_TYPE_STRING},
	{SMB_SHROPT_NAME, OPT_TYPE_NAME},
	{NULL, NULL},
};

/*
 * findopt(name)
 *
 * Lookup option "name" in the option table and return the table
 * index.
 */

static int
findopt(char *name)
{
	int i;
	if (name != NULL) {
		for (i = 0; optdefs[i].tag != NULL; i++) {
			if (strcmp(optdefs[i].tag, name) == 0)
				return (i);
		}
	}
	return (-1);
}

/*
 * is_a_number(number)
 *
 * is the string a number in one of the forms we want to use?
 */

static int
is_a_number(char *number)
{
	int ret = 1;
	int hex = 0;

	if (strncmp(number, "0x", 2) == 0) {
		number += 2;
		hex = 1;
	} else if (*number == '-') {
		number++; /* skip the minus */
	}

	while (ret == 1 && *number != '\0') {
		if (hex) {
			ret = isxdigit(*number++);
		} else {
			ret = isdigit(*number++);
		}
	}
	return (ret);
}

/*
 * validresource(name)
 *
 * Check that name only has valid characters in it. The current valid
 * set are the printable characters but not including:
 *	" / \ [ ] : | < > + ; , ? * = \t
 * Note that space is included and there is a maximum length.
 */
static int
validresource(const char *name)
{
	const char *cp;
	size_t len;

	if (name == NULL)
		return (B_FALSE);

	len = strlen(name);
	if (len == 0 || len > SA_MAX_RESOURCE_NAME)
		return (B_FALSE);

	if (strpbrk(name, "\"/\\[]:|<>+;,?*=\t") != NULL) {
		return (B_FALSE);
	}

	for (cp = name; *cp != '\0'; cp++)
		if (iscntrl(*cp))
			return (B_FALSE);

	return (B_TRUE);
}

/*
 * smb_isonline()
 *
 * Determine if the SMF service instance is in the online state or
 * not. A number of operations depend on this state.
 */
static boolean_t
smb_isonline(void)
{
	char *str;
	boolean_t ret = B_FALSE;

	if ((str = smf_get_state(SMBD_DEFAULT_INSTANCE_FMRI)) != NULL) {
		ret = (strcmp(str, SCF_STATE_STRING_ONLINE) == 0);
		free(str);
	}
	return (ret);
}

/*
 * smb_isdisabled()
 *
 * Determine if the SMF service instance is in the disabled state or
 * not. A number of operations depend on this state.
 */
static boolean_t
smb_isdisabled(void)
{
	char *str;
	boolean_t ret = B_FALSE;

	if ((str = smf_get_state(SMBD_DEFAULT_INSTANCE_FMRI)) != NULL) {
		ret = (strcmp(str, SCF_STATE_STRING_DISABLED) == 0);
		free(str);
	}
	return (ret);
}

/*
 * smb_isautoenable()
 *
 * Determine if the SMF service instance auto_enabled set or not. A
 * number of operations depend on this state.  The property not being
 * set or being set to true means autoenable.  Only being set to false
 * is not autoenabled.
 */
static boolean_t
smb_isautoenable(void)
{
	boolean_t ret = B_TRUE;
	scf_simple_prop_t *prop;
	uint8_t *retstr;

	prop = scf_simple_prop_get(NULL, SMBD_DEFAULT_INSTANCE_FMRI,
	    "application", "auto_enable");
	if (prop != NULL) {
		retstr = scf_simple_prop_next_boolean(prop);
		ret = *retstr != 0;
		scf_simple_prop_free(prop);
	}
	return (ret);
}

/*
 * smb_ismaint()
 *
 * Determine if the SMF service instance is in the disabled state or
 * not. A number of operations depend on this state.
 */
static boolean_t
smb_ismaint(void)
{
	char *str;
	boolean_t ret = B_FALSE;

	if ((str = smf_get_state(SMBD_DEFAULT_INSTANCE_FMRI)) != NULL) {
		ret = (strcmp(str, SCF_STATE_STRING_MAINT) == 0);
		free(str);
	}
	return (ret);
}

/*
 * smb_enable_share tells the implementation that it is to enable the share.
 * This entails converting the path and options into the appropriate ioctl
 * calls. It is assumed that all error checking of paths, etc. were
 * done earlier.
 */
static int
smb_enable_share(sa_share_t share)
{
	char *path;
	char *rname;
	smb_share_t si;
	sa_resource_t resource;
	boolean_t iszfs;
	boolean_t privileged;
	int err = SA_OK;
	priv_set_t *priv_effective;
	boolean_t online;

	/*
	 * We only start in the global zone and only run if we aren't
	 * running Trusted Extensions.
	 */
	if (getzoneid() != GLOBAL_ZONEID) {
		(void) printf(dgettext(TEXT_DOMAIN,
		    "SMB: service not supported in local zone\n"));
		return (SA_NOT_SUPPORTED);
	}
	if (is_system_labeled()) {
		(void) printf(dgettext(TEXT_DOMAIN,
		    "SMB: service not supported with Trusted Extensions\n"));
		return (SA_NOT_SUPPORTED);
	}

	priv_effective = priv_allocset();
	(void) getppriv(PRIV_EFFECTIVE, priv_effective);
	privileged = (priv_isfullset(priv_effective) == B_TRUE);
	priv_freeset(priv_effective);

	/* get the path since it is important in several places */
	path = sa_get_share_attr(share, "path");
	if (path == NULL)
		return (SA_NO_SUCH_PATH);

	/*
	 * If administratively disabled, don't try to start anything.
	 */
	online = smb_isonline();
	if (!online && !smb_isautoenable() && smb_isdisabled())
		goto done;

	iszfs = sa_path_is_zfs(path);

	if (iszfs) {

		if (privileged == B_FALSE && !online) {

			if (!online) {
				(void) printf(dgettext(TEXT_DOMAIN,
				    "SMB: Cannot share remove "
				    "file system: %s\n"), path);
				(void) printf(dgettext(TEXT_DOMAIN,
				    "SMB: Service needs to be enabled "
				    "by a privileged user\n"));
				err = SA_NO_PERMISSION;
				errno = EPERM;
			}
			if (err) {
				sa_free_attr_string(path);
				return (err);
			}

		}
	}

	if (privileged == B_TRUE && !online) {
		err = smb_enable_service();
		if (err != SA_OK) {
			(void) printf(dgettext(TEXT_DOMAIN,
			    "SMB: Unable to enable service\n"));
			/*
			 * For now, it is OK to not be able to enable
			 * the service.
			 */
			if (err == SA_BUSY || err == SA_SYSTEM_ERR)
				err = SA_OK;
		} else {
			online = B_TRUE;
		}
	}

	/*
	 * Don't bother trying to start shares if the service isn't
	 * running.
	 */
	if (!online)
		goto done;

	/* Each share can have multiple resources */
	for (resource = sa_get_share_resource(share, NULL);
	    resource != NULL;
	    resource = sa_get_next_resource(resource)) {
		bzero(&si, sizeof (smb_share_t));
		rname = sa_get_resource_attr(resource, "name");
		if (rname == NULL) {
			sa_free_attr_string(path);
			return (SA_NO_SUCH_RESOURCE);
		}

		smb_build_lmshare_info(rname, path, resource, &si);
		sa_free_attr_string(rname);

		if (!iszfs) {
			err = smb_share_add(&si);
		} else {
			share_t sh;

			sa_sharetab_fill_zfs(share, &sh, "smb");
			err = sa_share_zfs(share, (char *)path, &sh,
			    &si, ZFS_SHARE_SMB);

			sa_emptyshare(&sh);
		}
	}
	if (!iszfs)
		(void) sa_update_sharetab(share, "smb");
done:
	sa_free_attr_string(path);

	return (err == NERR_DuplicateShare ? 0 : err);
}

/*
 * This is the share for CIFS all shares have resource names.
 * Enable tells the smb server to update its hash. If it fails
 * because smb server is down, we just ignore as smb server loads
 * the resources from sharemanager at startup.
 */

static int
smb_enable_resource(sa_resource_t resource)
{
	char *path;
	char *rname;
	sa_share_t share;
	smb_share_t si;
	int ret = SA_OK;
	int err;
	boolean_t isonline;

	share = sa_get_resource_parent(resource);
	if (share == NULL)
		return (SA_NO_SUCH_PATH);

	/*
	 * If administratively disabled, don't try to start anything.
	 */
	isonline = smb_isonline();
	if (!isonline && !smb_isautoenable() && smb_isdisabled())
		goto done;

	if (!isonline)
		ret = smb_enable_service();
	if (!smb_isonline()) {
		ret = SA_OK;
		goto done;
	}

	path = sa_get_share_attr(share, "path");
	if (path == NULL)
		return (SA_SYSTEM_ERR);
	rname = sa_get_resource_attr(resource, "name");
	if (rname == NULL) {
		sa_free_attr_string(path);
		return (SA_NO_SUCH_RESOURCE);
	}

	smb_build_lmshare_info(rname, path, resource, &si);
	sa_free_attr_string(path);
	sa_free_attr_string(rname);

	/*
	 * Attempt to add the share. Any error that occurs if it was
	 * online is an error but don't count NERR_DuplicateName if
	 * smb/server had to be brought online since bringing the
	 * service up will enable the share that was just added prior
	 * to the attempt to enable.
	 */

	err = smb_share_add(&si);
	if (err == NERR_Success || !(!isonline && err == NERR_DuplicateName))
		(void) sa_update_sharetab(share, "smb");
	else
		return (SA_NOT_SHARED);

done:
	return (ret);
}

/*
 * Remove it from smb server hash.
 */
static int
smb_disable_resource(sa_resource_t resource)
{
	char *rname;
	uint32_t res;
	sa_share_t share;

	rname = sa_get_resource_attr(resource, "name");
	if (rname == NULL)
		return (SA_NO_SUCH_RESOURCE);

	if (smb_isonline()) {
		res = smb_share_del(rname);
		if (res != NERR_Success) {
			sa_free_attr_string(rname);
			return (SA_CONFIG_ERR);
		}
	}

	sa_free_attr_string(rname);

	share = sa_get_resource_parent(resource);
	if (share != NULL) {
		rname = sa_get_share_attr(share, "path");
		if (rname != NULL) {
			sa_handle_t handle;

			handle = sa_find_group_handle((sa_group_t)resource);
			(void) sa_delete_sharetab(handle, rname, "smb");
			sa_free_attr_string(rname);
		}
	}
	/*
	 * Always return OK as smb/server may be down and
	 * Shares will be picked up when loaded.
	 */
	return (SA_OK);
}

/*
 * smb_share_changed(sa_share_t share)
 *
 * The specified share has changed.
 */
static int
smb_share_changed(sa_share_t share)
{
	char *path;
	sa_resource_t resource;

	/* get the path since it is important in several places */
	path = sa_get_share_attr(share, "path");
	if (path == NULL)
		return (SA_NO_SUCH_PATH);
	for (resource = sa_get_share_resource(share, NULL);
	    resource != NULL;
	    resource = sa_get_next_resource(resource))
		(void) smb_resource_changed(resource);

	sa_free_attr_string(path);

	return (SA_OK);
}

/*
 * smb_resource_changed(sa_resource_t resource)
 *
 * The specified resource has changed.
 */
static int
smb_resource_changed(sa_resource_t resource)
{
	uint32_t res;
	smb_share_t si;
	smb_share_t new_si;
	char *rname, *path;
	sa_share_t share;

	rname = sa_get_resource_attr(resource, "name");
	if (rname == NULL)
		return (SA_NO_SUCH_RESOURCE);

	share = sa_get_resource_parent(resource);
	if (share == NULL) {
		sa_free_attr_string(rname);
		return (SA_CONFIG_ERR);
	}

	path = sa_get_share_attr(share, "path");
	if (path == NULL) {
		sa_free_attr_string(rname);
		return (SA_NO_SUCH_PATH);
	}

	if (!smb_isonline()) {
		sa_free_attr_string(rname);
		return (SA_OK);
	}

	/* Update the share cache in smb/server */
	res = smb_share_get(rname, &si);
	if (res != NERR_Success) {
		sa_free_attr_string(path);
		sa_free_attr_string(rname);
		return (SA_CONFIG_ERR);
	}

	smb_build_lmshare_info(rname, path, resource, &new_si);
	sa_free_attr_string(path);
	sa_free_attr_string(rname);

	/*
	 * Update all fields from sa_share_t
	 * Get derived values.
	 */
	if (smb_share_set(&new_si) != SMB_SHARE_DSUCCESS)
		return (SA_CONFIG_ERR);
	return (smb_enable_service());
}

/*
 * smb_disable_share(sa_share_t share, char *path)
 *
 * Unshare the specified share. Note that "path" is the same
 * path as what is in the "share" object. It is passed in to avoid an
 * additional lookup. A missing "path" value makes this a no-op
 * function.
 */
static int
smb_disable_share(sa_share_t share, char *path)
{
	char *rname;
	sa_resource_t resource;
	sa_group_t parent;
	boolean_t iszfs;
	int err = SA_OK;
	sa_handle_t handle;

	if (path == NULL)
		return (err);

	/*
	 * If the share is in a ZFS group we need to handle it
	 * differently.  Just being on a ZFS file system isn't
	 * enough since we may be in a legacy share case.
	 */
	parent = sa_get_parent_group(share);
	iszfs = sa_group_is_zfs(parent);

	if (!smb_isonline())
		goto done;

	for (resource = sa_get_share_resource(share, NULL);
	    resource != NULL;
	    resource = sa_get_next_resource(resource)) {
		rname = sa_get_resource_attr(resource, "name");
		if (rname == NULL) {
			continue;
		}
		if (!iszfs) {
			err = smb_share_del(rname);
			switch (err) {
			case NERR_NetNameNotFound:
			case NERR_Success:
				err = SA_OK;
				break;
			default:
				err = SA_CONFIG_ERR;
				break;
			}
		} else {
			share_t sh;

			sa_sharetab_fill_zfs(share, &sh, "smb");
			err = sa_share_zfs(share, (char *)path, &sh,
			    rname, ZFS_UNSHARE_SMB);
			sa_emptyshare(&sh);
		}
		sa_free_attr_string(rname);
	}
done:
	if (!iszfs) {
		handle = sa_find_group_handle((sa_group_t)share);
		if (handle != NULL)
			(void) sa_delete_sharetab(handle, path, "smb");
		else
			err = SA_SYSTEM_ERR;
	}
	return (err);
}

/*
 * smb_validate_property(handle, property, parent)
 *
 * Check that the property has a legitimate value for its type.
 * Handle isn't currently used but may need to be in the future.
 */

/*ARGSUSED*/
static int
smb_validate_property(sa_handle_t handle, sa_property_t property,
    sa_optionset_t parent)
{
	int ret = SA_OK;
	char *propname;
	int optindex;
	sa_group_t parent_group;
	char *value;

	propname = sa_get_property_attr(property, "type");

	if ((optindex = findopt(propname)) < 0)
		ret = SA_NO_SUCH_PROP;

	/* need to validate value range here as well */
	if (ret == SA_OK) {
		parent_group = sa_get_parent_group((sa_share_t)parent);
		if (optdefs[optindex].share && !sa_is_share(parent_group))
			ret = SA_PROP_SHARE_ONLY;
	}
	if (ret != SA_OK) {
		if (propname != NULL)
			sa_free_attr_string(propname);
		return (ret);
	}

	value = sa_get_property_attr(property, "value");
	if (value != NULL) {
		/* first basic type checking */
		switch (optdefs[optindex].type) {
		case OPT_TYPE_NUMBER:
			/* check that the value is all digits */
			if (!is_a_number(value))
				ret = SA_BAD_VALUE;
			break;
		case OPT_TYPE_BOOLEAN:
			if (strlen(value) == 0 ||
			    strcasecmp(value, "true") == 0 ||
			    strcmp(value, "1") == 0 ||
			    strcasecmp(value, "false") == 0 ||
			    strcmp(value, "0") == 0) {
				ret = SA_OK;
			} else {
				ret = SA_BAD_VALUE;
			}
			break;
		case OPT_TYPE_NAME:
			/*
			 * Make sure no invalid characters
			 */
			if (validresource(value) == B_FALSE)
				ret = SA_BAD_VALUE;
			break;
		case OPT_TYPE_STRING:
			/* whatever is here should be ok */
			break;
		default:
			break;
		}
	}

	if (value != NULL)
		sa_free_attr_string(value);
	if (ret == SA_OK && optdefs[optindex].check != NULL)
		/* do the property specific check */
		ret = optdefs[optindex].check(property);

	if (propname != NULL)
		sa_free_attr_string(propname);
	return (ret);
}

/*
 * Protocol management functions
 *
 * properties defined in the default files are defined in
 * proto_option_defs for parsing and validation.
 */

struct smb_proto_option_defs {
	int smb_index;
	int32_t minval;
	int32_t maxval; /* In case of length of string this should be max */
	int (*validator)(int, char *);
	int32_t	refresh;
} smb_proto_options[] = {
	{ SMB_CI_SYS_CMNT, 0, MAX_VALUE_BUFLEN,
	    string_length_check_validator, SMB_REFRESH_REFRESH },
	{ SMB_CI_MAX_WORKERS, 64, 1024, range_check_validator,
	    SMB_REFRESH_REFRESH },
	{ SMB_CI_NBSCOPE, 0, MAX_VALUE_BUFLEN,
	    string_length_check_validator, 0 },
	{ SMB_CI_LM_LEVEL, 2, 5, range_check_validator, 0 },
	{ SMB_CI_KEEPALIVE, 20, 5400, range_check_validator_zero_ok,
	    SMB_REFRESH_REFRESH },
	{ SMB_CI_WINS_SRV1, 0, MAX_VALUE_BUFLEN,
	    ip_address_validator_empty_ok, SMB_REFRESH_REFRESH },
	{ SMB_CI_WINS_SRV2, 0, MAX_VALUE_BUFLEN,
	    ip_address_validator_empty_ok, SMB_REFRESH_REFRESH },
	{ SMB_CI_WINS_EXCL, 0, MAX_VALUE_BUFLEN,
	    ip_address_csv_list_validator_empty_ok, SMB_REFRESH_REFRESH },
	{ SMB_CI_SIGNING_ENABLE, 0, 0, true_false_validator,
	    SMB_REFRESH_REFRESH },
	{ SMB_CI_SIGNING_REQD, 0, 0, true_false_validator,
	    SMB_REFRESH_REFRESH },
	{ SMB_CI_RESTRICT_ANON, 0, 0, true_false_validator,
	    SMB_REFRESH_REFRESH },
	{ SMB_CI_DOMAIN_SRV, 0, MAX_VALUE_BUFLEN,
	    ip_address_validator_empty_ok, 0 },
	{ SMB_CI_ADS_SITE, 0, MAX_VALUE_BUFLEN,
	    string_length_check_validator, SMB_REFRESH_REFRESH },
	{ SMB_CI_DYNDNS_ENABLE, 0, 0, true_false_validator, 0 },
	{ SMB_CI_AUTOHOME_MAP, 0, MAX_VALUE_BUFLEN, path_validator, 0 },
};

#define	SMB_OPT_NUM \
	(sizeof (smb_proto_options) / sizeof (smb_proto_options[0]))

/*
 * Check the range of value as int range.
 */
static int
range_check_validator(int index, char *value)
{
	int ret = SA_OK;

	if (!is_a_number(value)) {
		ret = SA_BAD_VALUE;
	} else {
		int val;
		val = strtoul(value, NULL, 0);
		if (val < smb_proto_options[index].minval ||
		    val > smb_proto_options[index].maxval)
			ret = SA_BAD_VALUE;
	}
	return (ret);
}

/*
 * Check the range of value as int range.
 */
static int
range_check_validator_zero_ok(int index, char *value)
{
	int ret = SA_OK;

	if (!is_a_number(value)) {
		ret = SA_BAD_VALUE;
	} else {
		int val;
		val = strtoul(value, NULL, 0);
		if (val == 0)
			ret = SA_OK;
		else {
			if (val < smb_proto_options[index].minval ||
			    val > smb_proto_options[index].maxval)
			ret = SA_BAD_VALUE;
		}
	}
	return (ret);
}

/*
 * Check the length of the string
 */
static int
string_length_check_validator(int index, char *value)
{
	int ret = SA_OK;

	if (value == NULL)
		return (SA_BAD_VALUE);
	if (strlen(value) > smb_proto_options[index].maxval)
		ret = SA_BAD_VALUE;
	return (ret);
}

/*
 * Check yes/no
 */
/*ARGSUSED*/
static int
true_false_validator(int index, char *value)
{
	if (value == NULL)
		return (SA_BAD_VALUE);
	if ((strcasecmp(value, "true") == 0) ||
	    (strcasecmp(value, "false") == 0))
		return (SA_OK);
	return (SA_BAD_VALUE);
}

/*
 * Check IP address.
 */
/*ARGSUSED*/
static int
ip_address_validator_empty_ok(int index, char *value)
{
	char sbytes[16];
	int len;

	if (value == NULL)
		return (SA_OK);
	len = strlen(value);
	if (len == 0)
		return (SA_OK);
	if (inet_pton(AF_INET, value, (void *)sbytes) != 1)
		return (SA_BAD_VALUE);

	return (SA_OK);
}

/*
 * Check IP address list
 */
/*ARGSUSED*/
static int
ip_address_csv_list_validator_empty_ok(int index, char *value)
{
	char sbytes[16];
	char *ip, *tmp, *ctx;

	if (value == NULL || *value == '\0')
		return (SA_OK);

	if (strlen(value) > MAX_VALUE_BUFLEN)
		return (SA_BAD_VALUE);

	if ((tmp = strdup(value)) == NULL)
		return (SA_NO_MEMORY);

	ip = strtok_r(tmp, ",", &ctx);
	while (ip) {
		if (strlen(ip) == 0) {
			free(tmp);
			return (SA_BAD_VALUE);
		}
		if (*ip != 0) {
			if (inet_pton(AF_INET, ip,
			    (void *)sbytes) != 1) {
				free(tmp);
				return (SA_BAD_VALUE);
			}
		}
		ip = strtok_r(0, ",", &ctx);
	}

	free(tmp);
	return (SA_OK);
}

/*
 * Check path
 */
/*ARGSUSED*/
static int
path_validator(int index, char *value)
{
	struct stat buffer;
	int fd, status;

	if (value == NULL)
		return (SA_BAD_VALUE);

	fd = open(value, O_RDONLY);
	if (fd < 0)
		return (SA_BAD_VALUE);

	status = fstat(fd, &buffer);
	(void) close(fd);

	if (status < 0)
		return (SA_BAD_VALUE);

	if (buffer.st_mode & S_IFDIR)
		return (SA_OK);
	return (SA_BAD_VALUE);
}

/*
 * the protoset holds the defined options so we don't have to read
 * them multiple times
 */
static sa_protocol_properties_t protoset;

static int
findprotoopt(char *name)
{
	int i;
	char *sc_name;

	for (i = 0; i < SMB_OPT_NUM; i++) {
		sc_name = smb_config_getname(smb_proto_options[i].smb_index);
		if (strcasecmp(sc_name, name) == 0)
			return (i);
	}

	return (-1);
}

/*
 * smb_load_proto_properties()
 *
 * read the smb config values from SMF.
 */

static int
smb_load_proto_properties()
{
	sa_property_t prop;
	char value[MAX_VALUE_BUFLEN];
	char *name;
	int index;
	int ret = SA_OK;
	int rc;

	protoset = sa_create_protocol_properties(SMB_PROTOCOL_NAME);
	if (protoset == NULL)
		return (SA_NO_MEMORY);

	for (index = 0; index < SMB_OPT_NUM && ret == SA_OK; index++) {
		rc = smb_config_get(smb_proto_options[index].smb_index,
		    value, sizeof (value));
		if (rc != SMBD_SMF_OK)
			continue;
		name = smb_config_getname(smb_proto_options[index].smb_index);
		prop = sa_create_property(name, value);
		if (prop != NULL)
			ret = sa_add_protocol_property(protoset, prop);
		else
			ret = SA_NO_MEMORY;
	}
	return (ret);
}

/*
 * smb_share_init()
 *
 * Initialize the smb plugin.
 */

static int
smb_share_init(void)
{
	int ret = SA_OK;

	if (sa_plugin_ops.sa_init != smb_share_init)
		return (SA_SYSTEM_ERR);

	ret = smb_load_proto_properties();

	return (ret);
}

/*
 * smb_share_fini()
 *
 */
static void
smb_share_fini(void)
{
	xmlFreeNode(protoset);
	protoset = NULL;

	(void) smb_share_dclose();
}

/*
 * smb_get_proto_set()
 *
 * Return an optionset with all the protocol specific properties in
 * it.
 */
static sa_protocol_properties_t
smb_get_proto_set(void)
{
	return (protoset);
}

/*
 * smb_enable_dependencies()
 *
 * SMBD_DEFAULT_INSTANCE_FMRI may have some dependencies that aren't
 * enabled. This will attempt to enable all of them.
 */
static void
smb_enable_dependencies(const char *fmri)
{
	scf_handle_t *handle;
	scf_service_t *service;
	scf_instance_t *inst = NULL;
	scf_iter_t *iter;
	scf_property_t *prop;
	scf_value_t *value;
	scf_propertygroup_t *pg;
	scf_scope_t *scope;
	char type[SCFTYPE_LEN];
	char *dependency;
	char *servname;
	int maxlen;

	/*
	 * Get all required handles and storage.
	 */
	handle = scf_handle_create(SCF_VERSION);
	if (handle == NULL)
		return;

	if (scf_handle_bind(handle) != 0) {
		scf_handle_destroy(handle);
		return;
	}

	maxlen = scf_limit(SCF_LIMIT_MAX_VALUE_LENGTH);
	if (maxlen == (ssize_t)-1)
		maxlen = MAXPATHLEN;

	dependency = malloc(maxlen);

	service = scf_service_create(handle);

	iter = scf_iter_create(handle);

	pg = scf_pg_create(handle);

	prop = scf_property_create(handle);

	value = scf_value_create(handle);

	scope = scf_scope_create(handle);

	if (service == NULL || iter == NULL || pg == NULL || prop == NULL ||
	    value == NULL || scope == NULL || dependency == NULL)
		goto done;

	/*
	 *  We passed in the FMRI for the default instance but for
	 *  some things we need the simple form so construct it. Since
	 *  we reuse the storage that dependency points to, we need to
	 *  use the servname early.
	 */
	(void) snprintf(dependency, maxlen, "%s", fmri + sizeof ("svc:"));
	servname = strrchr(dependency, ':');
	if (servname == NULL)
		goto done;
	*servname = '\0';
	servname = dependency;

	/*
	 * Setup to iterate over the service property groups, only
	 * looking at those that are "dependency" types. The "entity"
	 * property will have the FMRI of the service we are dependent
	 * on.
	 */
	if (scf_handle_get_scope(handle, SCF_SCOPE_LOCAL, scope) != 0)
		goto done;

	if (scf_scope_get_service(scope, servname, service) != 0)
		goto done;

	if (scf_iter_service_pgs(iter, service) != 0)
		goto done;

	while (scf_iter_next_pg(iter, pg) > 0) {
		char *services[2];
		/*
		 * Have a property group for the service. See if it is
		 * a dependency pg and only do operations on those.
		 */
		if (scf_pg_get_type(pg, type, SCFTYPE_LEN) <= 0)
			continue;

		if (strncmp(type, SCF_GROUP_DEPENDENCY, SCFTYPE_LEN) != 0)
			continue;
		/*
		 * Have a dependency.  Attempt to enable it.
		 */
		if (scf_pg_get_property(pg, SCF_PROPERTY_ENTITIES, prop) != 0)
			continue;

		if (scf_property_get_value(prop, value) != 0)
			continue;

		services[1] = NULL;

		if (scf_value_get_as_string(value, dependency, maxlen) > 0) {
			services[0] = dependency;
			_check_services(services);
		}
	}

done:
	if (dependency != NULL)
		free(dependency);
	if (value != NULL)
		scf_value_destroy(value);
	if (prop != NULL)
		scf_property_destroy(prop);
	if (pg != NULL)
		scf_pg_destroy(pg);
	if (iter != NULL)
		scf_iter_destroy(iter);
	if (scope != NULL)
		scf_scope_destroy(scope);
	if (inst != NULL)
		scf_instance_destroy(inst);
	if (service != NULL)
		scf_service_destroy(service);

	(void) scf_handle_unbind(handle);
	scf_handle_destroy(handle);
}

/*
 * How long to wait for service to come online
 */
#define	WAIT_FOR_SERVICE	15

/*
 * smb_enable_service()
 *
 */
static int
smb_enable_service(void)
{
	int i;
	int ret = SA_OK;
	char *service[] = { SMBD_DEFAULT_INSTANCE_FMRI, NULL };

	if (!smb_isonline()) {
		/*
		 * Attempt to start the idmap, and other dependent
		 * services, first.  If it fails, the SMB service will
		 * ultimately fail so we use that as the error.  If we
		 * don't try to enable idmap, smb won't start the
		 * first time unless the admin has done it
		 * manually. The service could be administratively
		 * disabled so we won't always get started.
		 */
		smb_enable_dependencies(SMBD_DEFAULT_INSTANCE_FMRI);
		_check_services(service);

		/* Wait for service to come online */
		for (i = 0; i < WAIT_FOR_SERVICE; i++) {
			if (smb_isonline()) {
				ret =  SA_OK;
				break;
			} else if (smb_ismaint()) {
				/* maintenance requires help */
				ret = SA_SYSTEM_ERR;
				break;
			} else if (smb_isdisabled()) {
				/* disabled is ok */
				ret = SA_OK;
				break;
			} else {
				/* try another time */
				ret = SA_BUSY;
				(void) sleep(1);
			}
		}
	}
	return (ret);
}

/*
 * smb_validate_proto_prop(index, name, value)
 *
 * Verify that the property specified by name can take the new
 * value. This is a sanity check to prevent bad values getting into
 * the default files.
 */
static int
smb_validate_proto_prop(int index, char *name, char *value)
{
	if ((name == NULL) || (index < 0))
		return (SA_BAD_VALUE);

	if (smb_proto_options[index].validator == NULL)
		return (SA_OK);

	if (smb_proto_options[index].validator(index, value) == SA_OK)
		return (SA_OK);
	return (SA_BAD_VALUE);
}

/*
 * smb_set_proto_prop(prop)
 *
 * check that prop is valid.
 */
/*ARGSUSED*/
static int
smb_set_proto_prop(sa_property_t prop)
{
	int ret = SA_OK;
	char *name;
	char *value;
	int index = -1;
	struct smb_proto_option_defs *opt;

	name = sa_get_property_attr(prop, "type");
	value = sa_get_property_attr(prop, "value");
	if (name != NULL && value != NULL) {
		index = findprotoopt(name);
		if (index >= 0) {
			/* should test for valid value */
			ret = smb_validate_proto_prop(index, name, value);
			if (ret == SA_OK) {
				opt = &smb_proto_options[index];

				/* Save to SMF */
				(void) smb_config_set(opt->smb_index, value);
				/*
				 * Specialized refresh mechanisms can
				 * be flagged in the proto_options and
				 * processed here.
				 */
				if (opt->refresh & SMB_REFRESH_REFRESH)
					(void) smf_refresh_instance(
					    SMBD_DEFAULT_INSTANCE_FMRI);
				else if (opt->refresh & SMB_REFRESH_RESTART)
					(void) smf_restart_instance(
					    SMBD_DEFAULT_INSTANCE_FMRI);
			}
		}
	}

	if (name != NULL)
		sa_free_attr_string(name);
	if (value != NULL)
		sa_free_attr_string(value);

	return (ret);
}

/*
 * smb_get_status()
 *
 * What is the current status of the smbd? We use the SMF state here.
 * Caller must free the returned value.
 */

static char *
smb_get_status(void)
{
	char *state = NULL;
	state = smf_get_state(SMBD_DEFAULT_INSTANCE_FMRI);
	return (state != NULL ? state : "-");
}

/*
 * This protocol plugin require resource names
 */
static uint64_t
smb_share_features(void)
{
	return (SA_FEATURE_RESOURCE | SA_FEATURE_ALLOWSUBDIRS |
	    SA_FEATURE_ALLOWPARDIRS | SA_FEATURE_SERVER);
}

/*
 * This should be used to convert lmshare_info to sa_resource_t
 * Should only be needed to build temp shares/resources to be
 * supplied to sharemanager to display temp shares.
 */
static int
smb_build_tmp_sa_resource(sa_handle_t handle, smb_share_t *si)
{
	int err;
	sa_share_t share;
	sa_group_t group;
	sa_resource_t resource;

	if (si == NULL)
		return (SA_INVALID_NAME);

	/*
	 * First determine if the "share path" is already shared
	 * somewhere. If it is, we have to use it as the authority on
	 * where the transient share lives so will use it's parent
	 * group. If it doesn't exist, it needs to land in "smb".
	 */

	share = sa_find_share(handle, si->shr_path);
	if (share != NULL) {
		group = sa_get_parent_group(share);
	} else {
		group = smb_get_smb_share_group(handle);
		if (group == NULL)
			return (SA_NO_SUCH_GROUP);
		share = sa_get_share(group, si->shr_path);
		if (share == NULL) {
			share = sa_add_share(group, si->shr_path,
			    SA_SHARE_TRANSIENT, &err);
			if (share == NULL)
				return (SA_NO_SUCH_PATH);
		}
	}

	/*
	 * Now handle the resource. Make sure that the resource is
	 * transient and added to the share.
	 */
	resource = sa_get_share_resource(share, si->shr_name);
	if (resource == NULL) {
		resource = sa_add_resource(share,
		    si->shr_name, SA_SHARE_TRANSIENT, &err);
		if (resource == NULL)
			return (SA_NO_SUCH_RESOURCE);
	}

	/* set resource attributes now */
	(void) sa_set_resource_attr(resource, "description", si->shr_cmnt);
	(void) sa_set_resource_attr(resource, SMB_SHROPT_AD_CONTAINER,
	    si->shr_container);

	return (SA_OK);
}

/*
 * Return smb transient shares.  Note that we really want to look at
 * all current shares from SMB in order to determine this. Transient
 * shares should be those that don't appear in either the SMF or ZFS
 * configurations.  Those that are in the repositories will be
 * filtered out by smb_build_tmp_sa_resource.
 */
static int
smb_list_transient(sa_handle_t handle)
{
	int i, offset, num;
	smb_shrlist_t list;
	int res;

	num = smb_share_count();
	if (num <= 0)
		return (SA_OK);
	offset = 0;
	while (smb_share_list(offset, &list) != NERR_InternalError) {
		if (list.no == 0)
			break;
		for (i = 0; i < list.no; i++) {
			res = smb_build_tmp_sa_resource(handle,
			    &(list.smbshr[i]));
			if (res != SA_OK)
				return (res);
		}
		offset += list.no;
	}

	return (SA_OK);
}

/*
 * fix_resource_name(share, name,  prefix)
 *
 * Construct a name where the ZFS dataset has the prefix replaced with "name".
 */
static char *
fix_resource_name(sa_share_t share, char *name, char *prefix)
{
	char *dataset = NULL;
	char *newname = NULL;
	size_t psize;
	size_t nsize;

	dataset = sa_get_share_attr(share, "dataset");

	if (dataset != NULL && strcmp(dataset, prefix) != 0) {
		psize = strlen(prefix);
		if (strncmp(dataset, prefix, psize) == 0) {
			/* need string plus ',' and NULL */
			nsize = (strlen(dataset) - psize) + strlen(name) + 2;
			newname = calloc(nsize, 1);
			if (newname != NULL) {
				(void) snprintf(newname, nsize, "%s%s", name,
				    dataset + psize);
				sa_fix_resource_name(newname);
			}
			sa_free_attr_string(dataset);
			return (newname);
		}
	}
	if (dataset != NULL)
		sa_free_attr_string(dataset);
	return (strdup(name));
}

/*
 * smb_parse_optstring(group, options)
 *
 * parse a compact option string into individual options. This allows
 * ZFS sharesmb and sharemgr "share" command to work.  group can be a
 * group, a share or a resource.
 */
static int
smb_parse_optstring(sa_group_t group, char *options)
{
	char *dup;
	char *base;
	char *lasts;
	char *token;
	sa_optionset_t optionset;
	sa_group_t parent = NULL;
	sa_resource_t resource = NULL;
	int iszfs = 0;
	int persist = 0;
	int need_optionset = 0;
	int ret = SA_OK;
	sa_property_t prop;

	/*
	 * In order to not attempt to change ZFS properties unless
	 * absolutely necessary, we never do it in the legacy parsing
	 * so we need to keep track of this.
	 */
	if (sa_is_share(group)) {
		char *zfs;

		parent = sa_get_parent_group(group);
		if (parent != NULL) {
			zfs = sa_get_group_attr(parent, "zfs");
			if (zfs != NULL) {
				sa_free_attr_string(zfs);
				iszfs = 1;
			}
		}
	} else {
		iszfs = sa_group_is_zfs(group);
		/*
		 * If a ZFS group, then we need to see if a resource
		 * name is being set. If so, bail with
		 * SA_PROP_SHARE_ONLY, so we come back in with a share
		 * instead of a group.
		 */
		if (strncmp(options, "name=", sizeof ("name=") - 1) == 0 ||
		    strstr(options, ",name=") != NULL) {
			return (SA_PROP_SHARE_ONLY);
		}
	}

	/* do we have an existing optionset? */
	optionset = sa_get_optionset(group, "smb");
	if (optionset == NULL) {
		/* didn't find existing optionset so create one */
		optionset = sa_create_optionset(group, "smb");
		if (optionset == NULL)
			return (SA_NO_MEMORY);
	} else {
		/*
		 * If an optionset already exists, we've come through
		 * twice so ignore the second time.
		 */
		return (ret);
	}

	/* We need a copy of options for the next part. */
	dup = strdup(options);
	if (dup == NULL)
		return (SA_NO_MEMORY);

	/*
	 * SMB properties are straightforward and are strings,
	 * integers or booleans.  Properties are separated by
	 * commas. It will be necessary to parse quotes due to some
	 * strings not having a restricted characters set.
	 *
	 * Note that names will create a resource. For now, if there
	 * is a set of properties "before" the first name="", those
	 * properties will be placed on the group.
	 */
	persist = sa_is_persistent(group);
	base = dup;
	token = dup;
	lasts = NULL;
	while (token != NULL && ret == SA_OK) {
		ret = SA_OK;
		token = strtok_r(base, ",", &lasts);
		base = NULL;
		if (token != NULL) {
			char *value;
			/*
			 * All SMB properties have values so there
			 * MUST be an '=' character.  If it doesn't,
			 * it is a syntax error.
			 */
			value = strchr(token, '=');
			if (value != NULL) {
				*value++ = '\0';
			} else {
				ret = SA_SYNTAX_ERR;
				break;
			}
			/*
			 * We may need to handle a "name" property
			 * that is a ZFS imposed resource name. Each
			 * name would trigger getting a new "resource"
			 * to put properties on. For now, assume no
			 * "name" property for special handling.
			 */

			if (strcmp(token, "name") == 0) {
				char *prefix;
				char *name = NULL;
				/*
				 * We have a name, so now work on the
				 * resource level. We have a "share"
				 * in "group" due to the caller having
				 * added it. If we are called with a
				 * group, the check for group/share
				 * at the beginning of this function
				 * will bail out the parse if there is a
				 * "name" but no share.
				 */
				if (!iszfs) {
					ret = SA_SYNTAX_ERR;
					break;
				}
				/*
				 * Make sure the parent group has the
				 * "prefix" property since we will
				 * need to use this for constructing
				 * inherited name= values.
				 */
				prefix = sa_get_group_attr(parent, "prefix");
				if (prefix == NULL) {
					prefix = sa_get_group_attr(parent,
					    "name");
					if (prefix != NULL) {
						(void) sa_set_group_attr(parent,
						    "prefix", prefix);
					}
				}
				name = fix_resource_name((sa_share_t)group,
				    value, prefix);
				if (name != NULL) {
					resource = sa_add_resource(
					    (sa_share_t)group, name,
					    SA_SHARE_TRANSIENT, &ret);
					sa_free_attr_string(name);
				} else {
					ret = SA_NO_MEMORY;
				}
				if (prefix != NULL)
					sa_free_attr_string(prefix);

				/* A resource level optionset is needed */

				need_optionset = 1;
				if (resource == NULL) {
					ret = SA_NO_MEMORY;
					break;
				}
				continue;
			}

			if (need_optionset) {
				optionset = sa_create_optionset(resource,
				    "smb");
				need_optionset = 0;
			}

			prop = sa_create_property(token, value);
			if (prop == NULL)
				ret = SA_NO_MEMORY;
			else
				ret = sa_add_property(optionset, prop);
			if (ret != SA_OK)
				break;
			if (!iszfs)
				ret = sa_commit_properties(optionset, !persist);
		}
	}
	free(dup);
	return (ret);
}

/*
 * smb_sprint_option(rbuff, rbuffsize, incr, prop, sep)
 *
 * provides a mechanism to format SMB properties into legacy output
 * format. If the buffer would overflow, it is reallocated and grown
 * as appropriate. Special cases of converting internal form of values
 * to those used by "share" are done. this function does one property
 * at a time.
 */

static void
smb_sprint_option(char **rbuff, size_t *rbuffsize, size_t incr,
			sa_property_t prop, int sep)
{
	char *name;
	char *value;
	int curlen;
	char *buff = *rbuff;
	size_t buffsize = *rbuffsize;

	name = sa_get_property_attr(prop, "type");
	value = sa_get_property_attr(prop, "value");
	if (buff != NULL)
		curlen = strlen(buff);
	else
		curlen = 0;
	if (name != NULL) {
		int len;
		len = strlen(name) + sep;

		/*
		 * A future RFE would be to replace this with more
		 * generic code and to possibly handle more types.
		 *
		 * For now, everything else is treated as a string. If
		 * we get any properties that aren't exactly
		 * name/value pairs, we may need to
		 * interpret/transform.
		 */
		if (value != NULL)
			len += 1 + strlen(value);

		while (buffsize <= (curlen + len)) {
			/* need more room */
			buffsize += incr;
			buff = realloc(buff, buffsize);
			*rbuff = buff;
			*rbuffsize = buffsize;
			if (buff == NULL) {
				/* realloc failed so free everything */
				if (*rbuff != NULL)
					free(*rbuff);
				goto err;
			}
		}
		if (buff == NULL)
			goto err;
		(void) snprintf(buff + curlen, buffsize - curlen,
		    "%s%s=%s", sep ? "," : "",
		    name, value != NULL ? value : "\"\"");

	}
err:
	if (name != NULL)
		sa_free_attr_string(name);
	if (value != NULL)
		sa_free_attr_string(value);
}

/*
 * smb_format_resource_options(resource, hier)
 *
 * format all the options on the group into a flattened option
 * string. If hier is non-zero, walk up the tree to get inherited
 * options.
 */

static char *
smb_format_options(sa_group_t group, int hier)
{
	sa_optionset_t options = NULL;
	sa_property_t prop;
	int sep = 0;
	char *buff;
	size_t buffsize;


	buff = malloc(OPT_CHUNK);
	if (buff == NULL)
		return (NULL);

	buff[0] = '\0';
	buffsize = OPT_CHUNK;

	/*
	 * We may have a an optionset relative to this item. format
	 * these if we find them and then add any security definitions.
	 */

	options = sa_get_derived_optionset(group, "smb", hier);

	/*
	 * do the default set first but skip any option that is also
	 * in the protocol specific optionset.
	 */
	if (options != NULL) {
		for (prop = sa_get_property(options, NULL);
		    prop != NULL; prop = sa_get_next_property(prop)) {
			/*
			 * use this one since we skipped any
			 * of these that were also in
			 * optdefault
			 */
			smb_sprint_option(&buff, &buffsize, OPT_CHUNK,
			    prop, sep);
			if (buff == NULL) {
				/*
				 * buff could become NULL if there
				 * isn't enough memory for
				 * smb_sprint_option to realloc()
				 * as necessary. We can't really
				 * do anything about it at this
				 * point so we return NULL.  The
				 * caller should handle the
				 * failure.
				 */
				if (options != NULL)
					sa_free_derived_optionset(
					    options);
				return (buff);
			}
			sep = 1;
		}
	}

	if (options != NULL)
		sa_free_derived_optionset(options);
	return (buff);
}

/*
 * smb_rename_resource(resource, newname)
 *
 * Change the current exported name of the resource to newname.
 */
/*ARGSUSED*/
int
smb_rename_resource(sa_handle_t handle, sa_resource_t resource, char *newname)
{
	int ret = SA_OK;
	int err;
	char *oldname;

	oldname = sa_get_resource_attr(resource, "name");
	if (oldname == NULL)
		return (SA_NO_SUCH_RESOURCE);

	err = smb_share_ren(oldname, newname);

	/* improve error values somewhat */
	switch (err) {
	case NERR_Success:
		break;
	case NERR_InternalError:
		ret = SA_SYSTEM_ERR;
		break;
	case NERR_DuplicateShare:
		ret = SA_DUPLICATE_NAME;
		break;
	default:
		ret = SA_CONFIG_ERR;
		break;
	}

	return (ret);
}
