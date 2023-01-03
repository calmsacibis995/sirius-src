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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/resource.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/sid.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/sdt.h>
#include <sys/fs/zfs.h>
#include <sys/mode.h>
#include <sys/policy.h>
#include <sys/zfs_znode.h>
#include <sys/zfs_fuid.h>
#include <sys/zfs_acl.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_vfsops.h>
#include <sys/dmu.h>
#include <sys/dnode.h>
#include <sys/zap.h>
#include "fs/fs_subr.h"
#include <acl/acl_common.h>

#define	ALLOW	ACE_ACCESS_ALLOWED_ACE_TYPE
#define	DENY	ACE_ACCESS_DENIED_ACE_TYPE
#define	MAX_ACE_TYPE	ACE_SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE

#define	OWNING_GROUP		(ACE_GROUP|ACE_IDENTIFIER_GROUP)
#define	EVERYONE_ALLOW_MASK (ACE_READ_ACL|ACE_READ_ATTRIBUTES | \
    ACE_READ_NAMED_ATTRS|ACE_SYNCHRONIZE)
#define	EVERYONE_DENY_MASK (ACE_WRITE_ACL|ACE_WRITE_OWNER | \
    ACE_WRITE_ATTRIBUTES|ACE_WRITE_NAMED_ATTRS)
#define	OWNER_ALLOW_MASK (ACE_WRITE_ACL | ACE_WRITE_OWNER | \
    ACE_WRITE_ATTRIBUTES|ACE_WRITE_NAMED_ATTRS)
#define	WRITE_MASK_DATA (ACE_WRITE_DATA|ACE_APPEND_DATA|ACE_WRITE_NAMED_ATTRS)

#define	ZFS_CHECKED_MASKS (ACE_READ_ACL|ACE_READ_ATTRIBUTES|ACE_READ_DATA| \
    ACE_READ_NAMED_ATTRS|ACE_WRITE_DATA|ACE_WRITE_ATTRIBUTES| \
    ACE_WRITE_NAMED_ATTRS|ACE_APPEND_DATA|ACE_EXECUTE|ACE_WRITE_OWNER| \
    ACE_WRITE_ACL|ACE_DELETE|ACE_DELETE_CHILD|ACE_SYNCHRONIZE)

#define	WRITE_MASK (WRITE_MASK_DATA|ACE_WRITE_ATTRIBUTES|ACE_WRITE_ACL|\
    ACE_WRITE_OWNER|ACE_DELETE|ACE_DELETE_CHILD)

#define	OGE_CLEAR	(ACE_READ_DATA|ACE_LIST_DIRECTORY|ACE_WRITE_DATA| \
    ACE_ADD_FILE|ACE_APPEND_DATA|ACE_ADD_SUBDIRECTORY|ACE_EXECUTE)

#define	OKAY_MASK_BITS (ACE_READ_DATA|ACE_LIST_DIRECTORY|ACE_WRITE_DATA| \
    ACE_ADD_FILE|ACE_APPEND_DATA|ACE_ADD_SUBDIRECTORY|ACE_EXECUTE)

#define	ALL_INHERIT	(ACE_FILE_INHERIT_ACE|ACE_DIRECTORY_INHERIT_ACE | \
    ACE_NO_PROPAGATE_INHERIT_ACE|ACE_INHERIT_ONLY_ACE|ACE_INHERITED_ACE)

#define	RESTRICTED_CLEAR	(ACE_WRITE_ACL|ACE_WRITE_OWNER)

#define	V4_ACL_WIDE_FLAGS (ZFS_ACL_AUTO_INHERIT|ZFS_ACL_DEFAULTED|\
    ZFS_ACL_PROTECTED)

#define	ZFS_ACL_WIDE_FLAGS (V4_ACL_WIDE_FLAGS|ZFS_ACL_TRIVIAL|ZFS_INHERIT_ACE|\
    ZFS_ACL_OBJ_ACE)

static uint16_t
zfs_ace_v0_get_type(void *acep)
{
	return (((zfs_oldace_t *)acep)->z_type);
}

static uint16_t
zfs_ace_v0_get_flags(void *acep)
{
	return (((zfs_oldace_t *)acep)->z_flags);
}

static uint32_t
zfs_ace_v0_get_mask(void *acep)
{
	return (((zfs_oldace_t *)acep)->z_access_mask);
}

static uint64_t
zfs_ace_v0_get_who(void *acep)
{
	return (((zfs_oldace_t *)acep)->z_fuid);
}

static void
zfs_ace_v0_set_type(void *acep, uint16_t type)
{
	((zfs_oldace_t *)acep)->z_type = type;
}

static void
zfs_ace_v0_set_flags(void *acep, uint16_t flags)
{
	((zfs_oldace_t *)acep)->z_flags = flags;
}

static void
zfs_ace_v0_set_mask(void *acep, uint32_t mask)
{
	((zfs_oldace_t *)acep)->z_access_mask = mask;
}

static void
zfs_ace_v0_set_who(void *acep, uint64_t who)
{
	((zfs_oldace_t *)acep)->z_fuid = who;
}

/*ARGSUSED*/
static size_t
zfs_ace_v0_size(void *acep)
{
	return (sizeof (zfs_oldace_t));
}

static size_t
zfs_ace_v0_abstract_size(void)
{
	return (sizeof (zfs_oldace_t));
}

static int
zfs_ace_v0_mask_off(void)
{
	return (offsetof(zfs_oldace_t, z_access_mask));
}

/*ARGSUSED*/
static int
zfs_ace_v0_data(void *acep, void **datap)
{
	*datap = NULL;
	return (0);
}

static acl_ops_t zfs_acl_v0_ops = {
	zfs_ace_v0_get_mask,
	zfs_ace_v0_set_mask,
	zfs_ace_v0_get_flags,
	zfs_ace_v0_set_flags,
	zfs_ace_v0_get_type,
	zfs_ace_v0_set_type,
	zfs_ace_v0_get_who,
	zfs_ace_v0_set_who,
	zfs_ace_v0_size,
	zfs_ace_v0_abstract_size,
	zfs_ace_v0_mask_off,
	zfs_ace_v0_data
};

static uint16_t
zfs_ace_fuid_get_type(void *acep)
{
	return (((zfs_ace_hdr_t *)acep)->z_type);
}

static uint16_t
zfs_ace_fuid_get_flags(void *acep)
{
	return (((zfs_ace_hdr_t *)acep)->z_flags);
}

static uint32_t
zfs_ace_fuid_get_mask(void *acep)
{
	return (((zfs_ace_hdr_t *)acep)->z_access_mask);
}

static uint64_t
zfs_ace_fuid_get_who(void *args)
{
	uint16_t entry_type;
	zfs_ace_t *acep = args;

	entry_type = acep->z_hdr.z_flags & ACE_TYPE_FLAGS;

	if (entry_type == ACE_OWNER || entry_type == OWNING_GROUP ||
	    entry_type == ACE_EVERYONE)
		return (-1);
	return (((zfs_ace_t *)acep)->z_fuid);
}

static void
zfs_ace_fuid_set_type(void *acep, uint16_t type)
{
	((zfs_ace_hdr_t *)acep)->z_type = type;
}

static void
zfs_ace_fuid_set_flags(void *acep, uint16_t flags)
{
	((zfs_ace_hdr_t *)acep)->z_flags = flags;
}

static void
zfs_ace_fuid_set_mask(void *acep, uint32_t mask)
{
	((zfs_ace_hdr_t *)acep)->z_access_mask = mask;
}

static void
zfs_ace_fuid_set_who(void *arg, uint64_t who)
{
	zfs_ace_t *acep = arg;

	uint16_t entry_type = acep->z_hdr.z_flags & ACE_TYPE_FLAGS;

	if (entry_type == ACE_OWNER || entry_type == OWNING_GROUP ||
	    entry_type == ACE_EVERYONE)
		return;
	acep->z_fuid = who;
}

static size_t
zfs_ace_fuid_size(void *acep)
{
	zfs_ace_hdr_t *zacep = acep;
	uint16_t entry_type;

	switch (zacep->z_type) {
	case ACE_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
	case ACE_ACCESS_DENIED_OBJECT_ACE_TYPE:
	case ACE_SYSTEM_AUDIT_OBJECT_ACE_TYPE:
	case ACE_SYSTEM_ALARM_OBJECT_ACE_TYPE:
		return (sizeof (zfs_object_ace_t));
	case ALLOW:
	case DENY:
		entry_type =
		    (((zfs_ace_hdr_t *)acep)->z_flags & ACE_TYPE_FLAGS);
		if (entry_type == ACE_OWNER ||
		    entry_type == (ACE_GROUP | ACE_IDENTIFIER_GROUP) ||
		    entry_type == ACE_EVERYONE)
			return (sizeof (zfs_ace_hdr_t));
		/*FALLTHROUGH*/
	default:
		return (sizeof (zfs_ace_t));
	}
}

static size_t
zfs_ace_fuid_abstract_size(void)
{
	return (sizeof (zfs_ace_hdr_t));
}

static int
zfs_ace_fuid_mask_off(void)
{
	return (offsetof(zfs_ace_hdr_t, z_access_mask));
}

static int
zfs_ace_fuid_data(void *acep, void **datap)
{
	zfs_ace_t *zacep = acep;
	zfs_object_ace_t *zobjp;

	switch (zacep->z_hdr.z_type) {
	case ACE_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
	case ACE_ACCESS_DENIED_OBJECT_ACE_TYPE:
	case ACE_SYSTEM_AUDIT_OBJECT_ACE_TYPE:
	case ACE_SYSTEM_ALARM_OBJECT_ACE_TYPE:
		zobjp = acep;
		*datap = (caddr_t)zobjp + sizeof (zfs_ace_t);
		return (sizeof (zfs_object_ace_t) - sizeof (zfs_ace_t));
	default:
		*datap = NULL;
		return (0);
	}
}

static acl_ops_t zfs_acl_fuid_ops = {
	zfs_ace_fuid_get_mask,
	zfs_ace_fuid_set_mask,
	zfs_ace_fuid_get_flags,
	zfs_ace_fuid_set_flags,
	zfs_ace_fuid_get_type,
	zfs_ace_fuid_set_type,
	zfs_ace_fuid_get_who,
	zfs_ace_fuid_set_who,
	zfs_ace_fuid_size,
	zfs_ace_fuid_abstract_size,
	zfs_ace_fuid_mask_off,
	zfs_ace_fuid_data
};

static int
zfs_acl_version(int version)
{
	if (version < ZPL_VERSION_FUID)
		return (ZFS_ACL_VERSION_INITIAL);
	else
		return (ZFS_ACL_VERSION_FUID);
}

static int
zfs_acl_version_zp(znode_t *zp)
{
	return (zfs_acl_version(zp->z_zfsvfs->z_version));
}

static zfs_acl_t *
zfs_acl_alloc(int vers)
{
	zfs_acl_t *aclp;

	aclp = kmem_zalloc(sizeof (zfs_acl_t), KM_SLEEP);
	list_create(&aclp->z_acl, sizeof (zfs_acl_node_t),
	    offsetof(zfs_acl_node_t, z_next));
	aclp->z_version = vers;
	if (vers == ZFS_ACL_VERSION_FUID)
		aclp->z_ops = zfs_acl_fuid_ops;
	else
		aclp->z_ops = zfs_acl_v0_ops;
	return (aclp);
}

static zfs_acl_node_t *
zfs_acl_node_alloc(size_t bytes)
{
	zfs_acl_node_t *aclnode;

	aclnode = kmem_zalloc(sizeof (zfs_acl_node_t), KM_SLEEP);
	if (bytes) {
		aclnode->z_acldata = kmem_alloc(bytes, KM_SLEEP);
		aclnode->z_allocdata = aclnode->z_acldata;
		aclnode->z_allocsize = bytes;
		aclnode->z_size = bytes;
	}

	return (aclnode);
}

static void
zfs_acl_node_free(zfs_acl_node_t *aclnode)
{
	if (aclnode->z_allocsize)
		kmem_free(aclnode->z_allocdata, aclnode->z_allocsize);
	kmem_free(aclnode, sizeof (zfs_acl_node_t));
}

static void
zfs_acl_release_nodes(zfs_acl_t *aclp)
{
	zfs_acl_node_t *aclnode;

	while (aclnode = list_head(&aclp->z_acl)) {
		list_remove(&aclp->z_acl, aclnode);
		zfs_acl_node_free(aclnode);
	}
	aclp->z_acl_count = 0;
	aclp->z_acl_bytes = 0;
}

void
zfs_acl_free(zfs_acl_t *aclp)
{
	zfs_acl_release_nodes(aclp);
	list_destroy(&aclp->z_acl);
	kmem_free(aclp, sizeof (zfs_acl_t));
}

static boolean_t
zfs_ace_valid(vtype_t obj_type, zfs_acl_t *aclp, uint16_t type, uint16_t iflags)
{
	/*
	 * first check type of entry
	 */

	switch (iflags & ACE_TYPE_FLAGS) {
	case ACE_OWNER:
	case (ACE_IDENTIFIER_GROUP | ACE_GROUP):
	case ACE_IDENTIFIER_GROUP:
	case ACE_EVERYONE:
	case 0:	/* User entry */
		break;
	default:
		return (B_FALSE);

	}

	/*
	 * next check inheritance level flags
	 */

	if (type != ALLOW && type > MAX_ACE_TYPE) {
		return (B_FALSE);
	}

	switch (type) {
	case ACE_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
	case ACE_ACCESS_DENIED_OBJECT_ACE_TYPE:
	case ACE_SYSTEM_AUDIT_OBJECT_ACE_TYPE:
	case ACE_SYSTEM_ALARM_OBJECT_ACE_TYPE:
		if (aclp->z_version < ZFS_ACL_VERSION_FUID)
			return (B_FALSE);
		aclp->z_hints |= ZFS_ACL_OBJ_ACE;
	}

	if (obj_type == VDIR &&
	    (iflags & (ACE_FILE_INHERIT_ACE|ACE_DIRECTORY_INHERIT_ACE)))
		aclp->z_hints |= ZFS_INHERIT_ACE;

	if (iflags & (ACE_INHERIT_ONLY_ACE|ACE_NO_PROPAGATE_INHERIT_ACE)) {
		if ((iflags & (ACE_FILE_INHERIT_ACE|
		    ACE_DIRECTORY_INHERIT_ACE)) == 0) {
			return (B_FALSE);
		}
	}

	return (B_TRUE);
}

static void *
zfs_acl_next_ace(zfs_acl_t *aclp, void *start, uint64_t *who,
    uint32_t *access_mask, uint16_t *iflags, uint16_t *type)
{
	zfs_acl_node_t *aclnode;

	if (start == NULL) {
		aclnode = list_head(&aclp->z_acl);
		if (aclnode == NULL)
			return (NULL);

		aclp->z_next_ace = aclnode->z_acldata;
		aclp->z_curr_node = aclnode;
		aclnode->z_ace_idx = 0;
	}

	aclnode = aclp->z_curr_node;

	if (aclnode == NULL)
		return (NULL);

	if (aclnode->z_ace_idx >= aclnode->z_ace_count) {
		aclnode = list_next(&aclp->z_acl, aclnode);
		if (aclnode == NULL)
			return (NULL);
		else {
			aclp->z_curr_node = aclnode;
			aclnode->z_ace_idx = 0;
			aclp->z_next_ace = aclnode->z_acldata;
		}
	}

	if (aclnode->z_ace_idx < aclnode->z_ace_count) {
		void *acep = aclp->z_next_ace;
		*iflags = aclp->z_ops.ace_flags_get(acep);
		*type = aclp->z_ops.ace_type_get(acep);
		*access_mask = aclp->z_ops.ace_mask_get(acep);
		*who = aclp->z_ops.ace_who_get(acep);
		aclp->z_next_ace = (caddr_t)aclp->z_next_ace +
		    aclp->z_ops.ace_size(acep);
		aclnode->z_ace_idx++;
		return ((void *)acep);
	}
	return (NULL);
}

/*ARGSUSED*/
static uint64_t
zfs_ace_walk(void *datap, uint64_t cookie, int aclcnt,
    uint16_t *flags, uint16_t *type, uint32_t *mask)
{
	zfs_acl_t *aclp = datap;
	zfs_ace_hdr_t *acep = (zfs_ace_hdr_t *)(uintptr_t)cookie;
	uint64_t who;

	acep = zfs_acl_next_ace(aclp, acep, &who, mask,
	    flags, type);
	return ((uint64_t)(uintptr_t)acep);
}

static zfs_acl_node_t *
zfs_acl_curr_node(zfs_acl_t *aclp)
{
	ASSERT(aclp->z_curr_node);
	return (aclp->z_curr_node);
}

/*
 * Copy ACE to internal ZFS format.
 * While processing the ACL each ACE will be validated for correctness.
 * ACE FUIDs will be created later.
 */
int
zfs_copy_ace_2_fuid(vtype_t obj_type, zfs_acl_t *aclp, void *datap,
    zfs_ace_t *z_acl, int aclcnt, size_t *size)
{
	int i;
	uint16_t entry_type;
	zfs_ace_t *aceptr = z_acl;
	ace_t *acep = datap;
	zfs_object_ace_t *zobjacep;
	ace_object_t *aceobjp;

	for (i = 0; i != aclcnt; i++) {
		aceptr->z_hdr.z_access_mask = acep->a_access_mask;
		aceptr->z_hdr.z_flags = acep->a_flags;
		aceptr->z_hdr.z_type = acep->a_type;
		entry_type = aceptr->z_hdr.z_flags & ACE_TYPE_FLAGS;
		if (entry_type != ACE_OWNER && entry_type != OWNING_GROUP &&
		    entry_type != ACE_EVERYONE) {
			if (!aclp->z_has_fuids)
				aclp->z_has_fuids = IS_EPHEMERAL(acep->a_who);
			aceptr->z_fuid = (uint64_t)acep->a_who;
		}

		/*
		 * Make sure ACE is valid
		 */
		if (zfs_ace_valid(obj_type, aclp, aceptr->z_hdr.z_type,
		    aceptr->z_hdr.z_flags) != B_TRUE)
			return (EINVAL);

		switch (acep->a_type) {
		case ACE_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
		case ACE_ACCESS_DENIED_OBJECT_ACE_TYPE:
		case ACE_SYSTEM_AUDIT_OBJECT_ACE_TYPE:
		case ACE_SYSTEM_ALARM_OBJECT_ACE_TYPE:
			zobjacep = (zfs_object_ace_t *)aceptr;
			aceobjp = (ace_object_t *)acep;

			bcopy(aceobjp->a_obj_type, zobjacep->z_object_type,
			    sizeof (aceobjp->a_obj_type));
			bcopy(aceobjp->a_inherit_obj_type,
			    zobjacep->z_inherit_type,
			    sizeof (aceobjp->a_inherit_obj_type));
			acep = (ace_t *)((caddr_t)acep + sizeof (ace_object_t));
			break;
		default:
			acep = (ace_t *)((caddr_t)acep + sizeof (ace_t));
		}

		aceptr = (zfs_ace_t *)((caddr_t)aceptr +
		    aclp->z_ops.ace_size(aceptr));
	}

	*size = (caddr_t)aceptr - (caddr_t)z_acl;

	return (0);
}

/*
 * Copy ZFS ACEs to fixed size ace_t layout
 */
static void
zfs_copy_fuid_2_ace(zfsvfs_t *zfsvfs, zfs_acl_t *aclp, cred_t *cr,
    void *datap, int filter)
{
	uint64_t who;
	uint32_t access_mask;
	uint16_t iflags, type;
	zfs_ace_hdr_t *zacep = NULL;
	ace_t *acep = datap;
	ace_object_t *objacep;
	zfs_object_ace_t *zobjacep;
	size_t ace_size;
	uint16_t entry_type;

	while (zacep = zfs_acl_next_ace(aclp, zacep,
	    &who, &access_mask, &iflags, &type)) {

		switch (type) {
		case ACE_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
		case ACE_ACCESS_DENIED_OBJECT_ACE_TYPE:
		case ACE_SYSTEM_AUDIT_OBJECT_ACE_TYPE:
		case ACE_SYSTEM_ALARM_OBJECT_ACE_TYPE:
			if (filter) {
				continue;
			}
			zobjacep = (zfs_object_ace_t *)zacep;
			objacep = (ace_object_t *)acep;
			bcopy(zobjacep->z_object_type,
			    objacep->a_obj_type,
			    sizeof (zobjacep->z_object_type));
			bcopy(zobjacep->z_inherit_type,
			    objacep->a_inherit_obj_type,
			    sizeof (zobjacep->z_inherit_type));
			ace_size = sizeof (ace_object_t);
			break;
		default:
			ace_size = sizeof (ace_t);
			break;
		}

		entry_type = (iflags & ACE_TYPE_FLAGS);
		if ((entry_type != ACE_OWNER &&
		    entry_type != (ACE_GROUP | ACE_IDENTIFIER_GROUP) &&
		    entry_type != ACE_EVERYONE)) {
			acep->a_who = zfs_fuid_map_id(zfsvfs, who,
			    cr, (entry_type & ACE_IDENTIFIER_GROUP) ?
			    ZFS_ACE_GROUP : ZFS_ACE_USER);
		} else {
			acep->a_who = (uid_t)(int64_t)who;
		}
		acep->a_access_mask = access_mask;
		acep->a_flags = iflags;
		acep->a_type = type;
		acep = (ace_t *)((caddr_t)acep + ace_size);
	}
}

static int
zfs_copy_ace_2_oldace(vtype_t obj_type, zfs_acl_t *aclp, ace_t *acep,
    zfs_oldace_t *z_acl, int aclcnt, size_t *size)
{
	int i;
	zfs_oldace_t *aceptr = z_acl;

	for (i = 0; i != aclcnt; i++, aceptr++) {
		aceptr->z_access_mask = acep[i].a_access_mask;
		aceptr->z_type = acep[i].a_type;
		aceptr->z_flags = acep[i].a_flags;
		aceptr->z_fuid = acep[i].a_who;
		/*
		 * Make sure ACE is valid
		 */
		if (zfs_ace_valid(obj_type, aclp, aceptr->z_type,
		    aceptr->z_flags) != B_TRUE)
			return (EINVAL);
	}
	*size = (caddr_t)aceptr - (caddr_t)z_acl;
	return (0);
}

/*
 * convert old ACL format to new
 */
void
zfs_acl_xform(znode_t *zp, zfs_acl_t *aclp)
{
	zfs_oldace_t *oldaclp;
	int i;
	uint16_t type, iflags;
	uint32_t access_mask;
	uint64_t who;
	void *cookie = NULL;
	zfs_acl_node_t *newaclnode;

	ASSERT(aclp->z_version == ZFS_ACL_VERSION_INITIAL);
	/*
	 * First create the ACE in a contiguous piece of memory
	 * for zfs_copy_ace_2_fuid().
	 *
	 * We only convert an ACL once, so this won't happen
	 * everytime.
	 */
	oldaclp = kmem_alloc(sizeof (zfs_oldace_t) * aclp->z_acl_count,
	    KM_SLEEP);
	i = 0;
	while (cookie = zfs_acl_next_ace(aclp, cookie, &who,
	    &access_mask, &iflags, &type)) {
		oldaclp[i].z_flags = iflags;
		oldaclp[i].z_type = type;
		oldaclp[i].z_fuid = who;
		oldaclp[i++].z_access_mask = access_mask;
	}

	newaclnode = zfs_acl_node_alloc(aclp->z_acl_count *
	    sizeof (zfs_object_ace_t));
	aclp->z_ops = zfs_acl_fuid_ops;
	VERIFY(zfs_copy_ace_2_fuid(ZTOV(zp)->v_type, aclp, oldaclp,
	    newaclnode->z_acldata, aclp->z_acl_count,
	    &newaclnode->z_size) == 0);
	newaclnode->z_ace_count = aclp->z_acl_count;
	aclp->z_version = ZFS_ACL_VERSION;
	kmem_free(oldaclp, aclp->z_acl_count * sizeof (zfs_oldace_t));

	/*
	 * Release all previous ACL nodes
	 */

	zfs_acl_release_nodes(aclp);

	list_insert_head(&aclp->z_acl, newaclnode);

	aclp->z_acl_bytes = newaclnode->z_size;
	aclp->z_acl_count = newaclnode->z_ace_count;

}

/*
 * Convert unix access mask to v4 access mask
 */
static uint32_t
zfs_unix_to_v4(uint32_t access_mask)
{
	uint32_t new_mask = 0;

	if (access_mask & S_IXOTH)
		new_mask |= ACE_EXECUTE;
	if (access_mask & S_IWOTH)
		new_mask |= ACE_WRITE_DATA;
	if (access_mask & S_IROTH)
		new_mask |= ACE_READ_DATA;
	return (new_mask);
}

static void
zfs_set_ace(zfs_acl_t *aclp, void *acep, uint32_t access_mask,
    uint16_t access_type, uint64_t fuid, uint16_t entry_type)
{
	uint16_t type = entry_type & ACE_TYPE_FLAGS;

	aclp->z_ops.ace_mask_set(acep, access_mask);
	aclp->z_ops.ace_type_set(acep, access_type);
	aclp->z_ops.ace_flags_set(acep, entry_type);
	if ((type != ACE_OWNER && type != (ACE_GROUP | ACE_IDENTIFIER_GROUP) &&
	    type != ACE_EVERYONE))
		aclp->z_ops.ace_who_set(acep, fuid);
}

/*
 * Determine mode of file based on ACL.
 * Also, create FUIDs for any User/Group ACEs
 */
static uint64_t
zfs_mode_fuid_compute(znode_t *zp, zfs_acl_t *aclp, cred_t *cr,
    zfs_fuid_info_t **fuidp, dmu_tx_t *tx)
{
	int		entry_type;
	mode_t		mode;
	mode_t		seen = 0;
	zfs_ace_hdr_t 	*acep = NULL;
	uint64_t	who;
	uint16_t	iflags, type;
	uint32_t	access_mask;

	mode = (zp->z_phys->zp_mode & (S_IFMT | S_ISUID | S_ISGID | S_ISVTX));

	while (acep = zfs_acl_next_ace(aclp, acep, &who,
	    &access_mask, &iflags, &type)) {

		/*
		 * Skip over inherit only ACEs
		 */
		if (iflags & ACE_INHERIT_ONLY_ACE)
			continue;

		entry_type = (iflags & ACE_TYPE_FLAGS);

		if (entry_type == ACE_OWNER) {
			if ((access_mask & ACE_READ_DATA) &&
			    (!(seen & S_IRUSR))) {
				seen |= S_IRUSR;
				if (type == ALLOW) {
					mode |= S_IRUSR;
				}
			}
			if ((access_mask & ACE_WRITE_DATA) &&
			    (!(seen & S_IWUSR))) {
				seen |= S_IWUSR;
				if (type == ALLOW) {
					mode |= S_IWUSR;
				}
			}
			if ((access_mask & ACE_EXECUTE) &&
			    (!(seen & S_IXUSR))) {
				seen |= S_IXUSR;
				if (type == ALLOW) {
					mode |= S_IXUSR;
				}
			}
		} else if (entry_type == OWNING_GROUP) {
			if ((access_mask & ACE_READ_DATA) &&
			    (!(seen & S_IRGRP))) {
				seen |= S_IRGRP;
				if (type == ALLOW) {
					mode |= S_IRGRP;
				}
			}
			if ((access_mask & ACE_WRITE_DATA) &&
			    (!(seen & S_IWGRP))) {
				seen |= S_IWGRP;
				if (type == ALLOW) {
					mode |= S_IWGRP;
				}
			}
			if ((access_mask & ACE_EXECUTE) &&
			    (!(seen & S_IXGRP))) {
				seen |= S_IXGRP;
				if (type == ALLOW) {
					mode |= S_IXGRP;
				}
			}
		} else if (entry_type == ACE_EVERYONE) {
			if ((access_mask & ACE_READ_DATA)) {
				if (!(seen & S_IRUSR)) {
					seen |= S_IRUSR;
					if (type == ALLOW) {
						mode |= S_IRUSR;
					}
				}
				if (!(seen & S_IRGRP)) {
					seen |= S_IRGRP;
					if (type == ALLOW) {
						mode |= S_IRGRP;
					}
				}
				if (!(seen & S_IROTH)) {
					seen |= S_IROTH;
					if (type == ALLOW) {
						mode |= S_IROTH;
					}
				}
			}
			if ((access_mask & ACE_WRITE_DATA)) {
				if (!(seen & S_IWUSR)) {
					seen |= S_IWUSR;
					if (type == ALLOW) {
						mode |= S_IWUSR;
					}
				}
				if (!(seen & S_IWGRP)) {
					seen |= S_IWGRP;
					if (type == ALLOW) {
						mode |= S_IWGRP;
					}
				}
				if (!(seen & S_IWOTH)) {
					seen |= S_IWOTH;
					if (type == ALLOW) {
						mode |= S_IWOTH;
					}
				}
			}
			if ((access_mask & ACE_EXECUTE)) {
				if (!(seen & S_IXUSR)) {
					seen |= S_IXUSR;
					if (type == ALLOW) {
						mode |= S_IXUSR;
					}
				}
				if (!(seen & S_IXGRP)) {
					seen |= S_IXGRP;
					if (type == ALLOW) {
						mode |= S_IXGRP;
					}
				}
				if (!(seen & S_IXOTH)) {
					seen |= S_IXOTH;
					if (type == ALLOW) {
						mode |= S_IXOTH;
					}
				}
			}
		}
		/*
		 * Now handle FUID create for user/group ACEs
		 */
		if (entry_type == 0 || entry_type == ACE_IDENTIFIER_GROUP) {
			aclp->z_ops.ace_who_set(acep,
			    zfs_fuid_create(zp->z_zfsvfs, who, cr,
			    (entry_type == 0) ? ZFS_ACE_USER : ZFS_ACE_GROUP,
			    tx, fuidp));
		}
	}
	return (mode);
}

static zfs_acl_t *
zfs_acl_node_read_internal(znode_t *zp, boolean_t will_modify)
{
	zfs_acl_t	*aclp;
	zfs_acl_node_t	*aclnode;

	aclp = zfs_acl_alloc(zp->z_phys->zp_acl.z_acl_version);

	/*
	 * Version 0 to 1 znode_acl_phys has the size/count fields swapped.
	 * Version 0 didn't have a size field, only a count.
	 */
	if (zp->z_phys->zp_acl.z_acl_version == ZFS_ACL_VERSION_INITIAL) {
		aclp->z_acl_count = zp->z_phys->zp_acl.z_acl_size;
		aclp->z_acl_bytes = ZFS_ACL_SIZE(aclp->z_acl_count);
	} else {
		aclp->z_acl_count = zp->z_phys->zp_acl.z_acl_count;
		aclp->z_acl_bytes = zp->z_phys->zp_acl.z_acl_size;
	}

	aclnode = zfs_acl_node_alloc(will_modify ? aclp->z_acl_bytes : 0);
	aclnode->z_ace_count = aclp->z_acl_count;
	if (will_modify) {
		bcopy(zp->z_phys->zp_acl.z_ace_data, aclnode->z_acldata,
		    aclp->z_acl_bytes);
	} else {
		aclnode->z_size = aclp->z_acl_bytes;
		aclnode->z_acldata = &zp->z_phys->zp_acl.z_ace_data[0];
	}

	list_insert_head(&aclp->z_acl, aclnode);

	return (aclp);
}

/*
 * Read an external acl object.
 */
static int
zfs_acl_node_read(znode_t *zp, zfs_acl_t **aclpp, boolean_t will_modify)
{
	uint64_t extacl = zp->z_phys->zp_acl.z_acl_extern_obj;
	zfs_acl_t	*aclp;
	size_t		aclsize;
	size_t		acl_count;
	zfs_acl_node_t	*aclnode;
	int error;

	ASSERT(MUTEX_HELD(&zp->z_acl_lock));

	if (zp->z_phys->zp_acl.z_acl_extern_obj == 0) {
		*aclpp = zfs_acl_node_read_internal(zp, will_modify);
		return (0);
	}

	aclp = zfs_acl_alloc(zp->z_phys->zp_acl.z_acl_version);
	if (zp->z_phys->zp_acl.z_acl_version == ZFS_ACL_VERSION_INITIAL) {
		zfs_acl_phys_v0_t *zacl0 =
		    (zfs_acl_phys_v0_t *)&zp->z_phys->zp_acl;

		aclsize = ZFS_ACL_SIZE(zacl0->z_acl_count);
		acl_count = zacl0->z_acl_count;
	} else {
		aclsize = zp->z_phys->zp_acl.z_acl_size;
		acl_count = zp->z_phys->zp_acl.z_acl_count;
		if (aclsize == 0)
			aclsize = acl_count * sizeof (zfs_ace_t);
	}
	aclnode = zfs_acl_node_alloc(aclsize);
	list_insert_head(&aclp->z_acl, aclnode);
	error = dmu_read(zp->z_zfsvfs->z_os, extacl, 0,
	    aclsize, aclnode->z_acldata);
	aclnode->z_ace_count = acl_count;
	aclp->z_acl_count = acl_count;
	aclp->z_acl_bytes = aclsize;

	if (error != 0) {
		zfs_acl_free(aclp);
		return (error);
	}

	*aclpp = aclp;
	return (0);
}

/*
 * common code for setting ACLs.
 *
 * This function is called from zfs_mode_update, zfs_perm_init, and zfs_setacl.
 * zfs_setacl passes a non-NULL inherit pointer (ihp) to indicate that it's
 * already checked the acl and knows whether to inherit.
 */
int
zfs_aclset_common(znode_t *zp, zfs_acl_t *aclp, cred_t *cr,
    zfs_fuid_info_t **fuidp, dmu_tx_t *tx)
{
	int		error;
	znode_phys_t	*zphys = zp->z_phys;
	zfs_acl_phys_t	*zacl = &zphys->zp_acl;
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	uint64_t	aoid = zphys->zp_acl.z_acl_extern_obj;
	uint64_t	off = 0;
	dmu_object_type_t otype;
	zfs_acl_node_t	*aclnode;

	ASSERT(MUTEX_HELD(&zp->z_lock));
	ASSERT(MUTEX_HELD(&zp->z_acl_lock));

	dmu_buf_will_dirty(zp->z_dbuf, tx);

	zphys->zp_mode = zfs_mode_fuid_compute(zp, aclp, cr, fuidp, tx);

	/*
	 * Decide which opbject type to use.  If we are forced to
	 * use old ACL format than transform ACL into zfs_oldace_t
	 * layout.
	 */
	if (!zfsvfs->z_use_fuids) {
		otype = DMU_OT_OLDACL;
	} else {
		if ((aclp->z_version == ZFS_ACL_VERSION_INITIAL) &&
		    (zfsvfs->z_version >= ZPL_VERSION_FUID))
			zfs_acl_xform(zp, aclp);
		ASSERT(aclp->z_version >= ZFS_ACL_VERSION_FUID);
		otype = DMU_OT_ACL;
	}

	if (aclp->z_acl_bytes > ZFS_ACE_SPACE) {
		/*
		 * If ACL was previously external and we are now
		 * converting to new ACL format then release old
		 * ACL object and create a new one.
		 */
		if (aoid && aclp->z_version != zacl->z_acl_version) {
			error = dmu_object_free(zfsvfs->z_os,
			    zp->z_phys->zp_acl.z_acl_extern_obj, tx);
			if (error)
				return (error);
			aoid = 0;
		}
		if (aoid == 0) {
			aoid = dmu_object_alloc(zfsvfs->z_os,
			    otype, aclp->z_acl_bytes,
			    otype == DMU_OT_ACL ? DMU_OT_SYSACL : DMU_OT_NONE,
			    otype == DMU_OT_ACL ? DN_MAX_BONUSLEN : 0, tx);
		} else {
			(void) dmu_object_set_blocksize(zfsvfs->z_os, aoid,
			    aclp->z_acl_bytes, 0, tx);
		}
		zphys->zp_acl.z_acl_extern_obj = aoid;
		for (aclnode = list_head(&aclp->z_acl); aclnode;
		    aclnode = list_next(&aclp->z_acl, aclnode)) {
			if (aclnode->z_ace_count == 0)
				continue;
			dmu_write(zfsvfs->z_os, aoid, off,
			    aclnode->z_size, aclnode->z_acldata, tx);
			off += aclnode->z_size;
		}
	} else {
		void *start = zacl->z_ace_data;
		/*
		 * Migrating back embedded?
		 */
		if (zphys->zp_acl.z_acl_extern_obj) {
			error = dmu_object_free(zfsvfs->z_os,
			    zp->z_phys->zp_acl.z_acl_extern_obj, tx);
			if (error)
				return (error);
			zphys->zp_acl.z_acl_extern_obj = 0;
		}

		for (aclnode = list_head(&aclp->z_acl); aclnode;
		    aclnode = list_next(&aclp->z_acl, aclnode)) {
			if (aclnode->z_ace_count == 0)
				continue;
			bcopy(aclnode->z_acldata, start, aclnode->z_size);
			start = (caddr_t)start + aclnode->z_size;
		}
	}

	/*
	 * If Old version then swap count/bytes to match old
	 * layout of znode_acl_phys_t.
	 */
	if (aclp->z_version == ZFS_ACL_VERSION_INITIAL) {
		zphys->zp_acl.z_acl_size = aclp->z_acl_count;
		zphys->zp_acl.z_acl_count = aclp->z_acl_bytes;
	} else {
		zphys->zp_acl.z_acl_size = aclp->z_acl_bytes;
		zphys->zp_acl.z_acl_count = aclp->z_acl_count;
	}

	zphys->zp_acl.z_acl_version = aclp->z_version;

	/*
	 * Replace ACL wide bits, but first clear them.
	 */
	zp->z_phys->zp_flags &= ~ZFS_ACL_WIDE_FLAGS;

	zp->z_phys->zp_flags |= aclp->z_hints;

	if (ace_trivial_common(aclp, 0, zfs_ace_walk) == 0)
		zp->z_phys->zp_flags |= ZFS_ACL_TRIVIAL;

	zfs_time_stamper_locked(zp, STATE_CHANGED, tx);
	return (0);
}

/*
 * Update access mask for prepended ACE
 *
 * This applies the "groupmask" value for aclmode property.
 */
static void
zfs_acl_prepend_fixup(zfs_acl_t *aclp, void  *acep, void  *origacep,
    mode_t mode, uint64_t owner)
{
	int	rmask, wmask, xmask;
	int	user_ace;
	uint16_t aceflags;
	uint32_t origmask, acepmask;
	uint64_t fuid;

	aceflags = aclp->z_ops.ace_flags_get(acep);
	fuid = aclp->z_ops.ace_who_get(acep);
	origmask = aclp->z_ops.ace_mask_get(origacep);
	acepmask = aclp->z_ops.ace_mask_get(acep);

	user_ace = (!(aceflags &
	    (ACE_OWNER|ACE_GROUP|ACE_IDENTIFIER_GROUP)));

	if (user_ace && (fuid == owner)) {
		rmask = S_IRUSR;
		wmask = S_IWUSR;
		xmask = S_IXUSR;
	} else {
		rmask = S_IRGRP;
		wmask = S_IWGRP;
		xmask = S_IXGRP;
	}

	if (origmask & ACE_READ_DATA) {
		if (mode & rmask) {
			acepmask &= ~ACE_READ_DATA;
		} else {
			acepmask |= ACE_READ_DATA;
		}
	}

	if (origmask & ACE_WRITE_DATA) {
		if (mode & wmask) {
			acepmask &= ~ACE_WRITE_DATA;
		} else {
			acepmask |= ACE_WRITE_DATA;
		}
	}

	if (origmask & ACE_APPEND_DATA) {
		if (mode & wmask) {
			acepmask &= ~ACE_APPEND_DATA;
		} else {
			acepmask |= ACE_APPEND_DATA;
		}
	}

	if (origmask & ACE_EXECUTE) {
		if (mode & xmask) {
			acepmask &= ~ACE_EXECUTE;
		} else {
			acepmask |= ACE_EXECUTE;
		}
	}
	aclp->z_ops.ace_mask_set(acep, acepmask);
}

/*
 * Apply mode to canonical six ACEs.
 */
static void
zfs_acl_fixup_canonical_six(zfs_acl_t *aclp, mode_t mode)
{
	zfs_acl_node_t *aclnode = list_tail(&aclp->z_acl);
	void	*acep;
	int	maskoff = aclp->z_ops.ace_mask_off();
	size_t abstract_size = aclp->z_ops.ace_abstract_size();

	ASSERT(aclnode != NULL);

	acep = (void *)((caddr_t)aclnode->z_acldata +
	    aclnode->z_size - (abstract_size * 6));

	/*
	 * Fixup final ACEs to match the mode
	 */

	adjust_ace_pair_common(acep, maskoff, abstract_size,
	    (mode & 0700) >> 6);	/* owner@ */

	acep = (caddr_t)acep + (abstract_size * 2);

	adjust_ace_pair_common(acep, maskoff, abstract_size,
	    (mode & 0070) >> 3);	/* group@ */

	acep = (caddr_t)acep + (abstract_size * 2);
	adjust_ace_pair_common(acep, maskoff,
	    abstract_size, mode);	/* everyone@ */
}


static int
zfs_acl_ace_match(zfs_acl_t *aclp, void *acep, int allow_deny,
    int entry_type, int accessmask)
{
	uint32_t mask = aclp->z_ops.ace_mask_get(acep);
	uint16_t type = aclp->z_ops.ace_type_get(acep);
	uint16_t flags = aclp->z_ops.ace_flags_get(acep);

	return (mask == accessmask && type == allow_deny &&
	    ((flags & ACE_TYPE_FLAGS) == entry_type));
}

/*
 * Can prepended ACE be reused?
 */
static int
zfs_reuse_deny(zfs_acl_t *aclp, void *acep, void *prevacep)
{
	int okay_masks;
	uint16_t prevtype;
	uint16_t prevflags;
	uint16_t flags;
	uint32_t mask, prevmask;

	if (prevacep == NULL)
		return (B_FALSE);

	prevtype = aclp->z_ops.ace_type_get(prevacep);
	prevflags = aclp->z_ops.ace_flags_get(prevacep);
	flags = aclp->z_ops.ace_flags_get(acep);
	mask = aclp->z_ops.ace_mask_get(acep);
	prevmask = aclp->z_ops.ace_mask_get(prevacep);

	if (prevtype != DENY)
		return (B_FALSE);

	if (prevflags != (flags & ACE_IDENTIFIER_GROUP))
		return (B_FALSE);

	okay_masks = (mask & OKAY_MASK_BITS);

	if (prevmask & ~okay_masks)
		return (B_FALSE);

	return (B_TRUE);
}


/*
 * Insert new ACL node into chain of zfs_acl_node_t's
 *
 * This will result in two possible results.
 * 1. If the ACL is currently just a single zfs_acl_node and
 *    we are prepending the entry then current acl node will have
 *    a new node inserted above it.
 *
 * 2. If we are inserting in the middle of current acl node then
 *    the current node will be split in two and new node will be inserted
 *    in between the two split nodes.
 */
static zfs_acl_node_t *
zfs_acl_ace_insert(zfs_acl_t *aclp, void  *acep)
{
	zfs_acl_node_t 	*newnode;
	zfs_acl_node_t 	*trailernode = NULL;
	zfs_acl_node_t 	*currnode = zfs_acl_curr_node(aclp);
	int		curr_idx = aclp->z_curr_node->z_ace_idx;
	int		trailer_count;
	size_t		oldsize;

	newnode = zfs_acl_node_alloc(aclp->z_ops.ace_size(acep));
	newnode->z_ace_count = 1;

	oldsize = currnode->z_size;

	if (curr_idx != 1) {
		trailernode = zfs_acl_node_alloc(0);
		trailernode->z_acldata = acep;

		trailer_count = currnode->z_ace_count - curr_idx + 1;
		currnode->z_ace_count = curr_idx - 1;
		currnode->z_size = (caddr_t)acep - (caddr_t)currnode->z_acldata;
		trailernode->z_size = oldsize - currnode->z_size;
		trailernode->z_ace_count = trailer_count;
	}

	aclp->z_acl_count += 1;
	aclp->z_acl_bytes += aclp->z_ops.ace_size(acep);

	if (curr_idx == 1)
		list_insert_before(&aclp->z_acl, currnode, newnode);
	else
		list_insert_after(&aclp->z_acl, currnode, newnode);
	if (trailernode) {
		list_insert_after(&aclp->z_acl, newnode, trailernode);
		aclp->z_curr_node = trailernode;
		trailernode->z_ace_idx = 1;
	}

	return (newnode);
}

/*
 * Prepend deny ACE
 */
static void *
zfs_acl_prepend_deny(znode_t *zp, zfs_acl_t *aclp, void *acep,
    mode_t mode)
{
	zfs_acl_node_t *aclnode;
	void  *newacep;
	uint64_t fuid;
	uint16_t flags;

	aclnode = zfs_acl_ace_insert(aclp, acep);
	newacep = aclnode->z_acldata;
	fuid = aclp->z_ops.ace_who_get(acep);
	flags = aclp->z_ops.ace_flags_get(acep);
	zfs_set_ace(aclp, newacep, 0, DENY, fuid, (flags & ACE_TYPE_FLAGS));
	zfs_acl_prepend_fixup(aclp, newacep, acep, mode, zp->z_phys->zp_uid);

	return (newacep);
}

/*
 * Split an inherited ACE into inherit_only ACE
 * and original ACE with inheritance flags stripped off.
 */
static void
zfs_acl_split_ace(zfs_acl_t *aclp, zfs_ace_hdr_t *acep)
{
	zfs_acl_node_t *aclnode;
	zfs_acl_node_t *currnode;
	void  *newacep;
	uint16_t type, flags;
	uint32_t mask;
	uint64_t fuid;

	type = aclp->z_ops.ace_type_get(acep);
	flags = aclp->z_ops.ace_flags_get(acep);
	mask = aclp->z_ops.ace_mask_get(acep);
	fuid = aclp->z_ops.ace_who_get(acep);

	aclnode = zfs_acl_ace_insert(aclp, acep);
	newacep = aclnode->z_acldata;

	aclp->z_ops.ace_type_set(newacep, type);
	aclp->z_ops.ace_flags_set(newacep, flags | ACE_INHERIT_ONLY_ACE);
	aclp->z_ops.ace_mask_set(newacep, mask);
	aclp->z_ops.ace_type_set(newacep, type);
	aclp->z_ops.ace_who_set(newacep, fuid);
	aclp->z_next_ace = acep;
	flags &= ~ALL_INHERIT;
	aclp->z_ops.ace_flags_set(acep, flags);
	currnode = zfs_acl_curr_node(aclp);
	ASSERT(currnode->z_ace_idx >= 1);
	currnode->z_ace_idx -= 1;
}

/*
 * Are ACES started at index i, the canonical six ACES?
 */
static int
zfs_have_canonical_six(zfs_acl_t *aclp)
{
	void *acep;
	zfs_acl_node_t *aclnode = list_tail(&aclp->z_acl);
	int		i = 0;
	size_t abstract_size = aclp->z_ops.ace_abstract_size();

	ASSERT(aclnode != NULL);

	if (aclnode->z_ace_count < 6)
		return (0);

	acep = (void *)((caddr_t)aclnode->z_acldata +
	    aclnode->z_size - (aclp->z_ops.ace_abstract_size() * 6));

	if ((zfs_acl_ace_match(aclp, (caddr_t)acep + (abstract_size * i++),
	    DENY, ACE_OWNER, 0) &&
	    zfs_acl_ace_match(aclp, (caddr_t)acep + (abstract_size * i++),
	    ALLOW, ACE_OWNER, OWNER_ALLOW_MASK) &&
	    zfs_acl_ace_match(aclp, (caddr_t)acep + (abstract_size * i++), DENY,
	    OWNING_GROUP, 0) && zfs_acl_ace_match(aclp, (caddr_t)acep +
	    (abstract_size * i++),
	    ALLOW, OWNING_GROUP, 0) &&
	    zfs_acl_ace_match(aclp, (caddr_t)acep + (abstract_size * i++),
	    DENY, ACE_EVERYONE, EVERYONE_DENY_MASK) &&
	    zfs_acl_ace_match(aclp, (caddr_t)acep + (abstract_size * i++),
	    ALLOW, ACE_EVERYONE, EVERYONE_ALLOW_MASK))) {
		return (1);
	} else {
		return (0);
	}
}


/*
 * Apply step 1g, to group entries
 *
 * Need to deal with corner case where group may have
 * greater permissions than owner.  If so then limit
 * group permissions, based on what extra permissions
 * group has.
 */
static void
zfs_fixup_group_entries(zfs_acl_t *aclp, void *acep, void *prevacep,
    mode_t mode)
{
	uint32_t prevmask = aclp->z_ops.ace_mask_get(prevacep);
	uint32_t mask = aclp->z_ops.ace_mask_get(acep);
	uint16_t prevflags = aclp->z_ops.ace_flags_get(prevacep);
	mode_t extramode = (mode >> 3) & 07;
	mode_t ownermode = (mode >> 6);

	if (prevflags & ACE_IDENTIFIER_GROUP) {

		extramode &= ~ownermode;

		if (extramode) {
			if (extramode & S_IROTH) {
				prevmask &= ~ACE_READ_DATA;
				mask &= ~ACE_READ_DATA;
			}
			if (extramode & S_IWOTH) {
				prevmask &= ~(ACE_WRITE_DATA|ACE_APPEND_DATA);
				mask &= ~(ACE_WRITE_DATA|ACE_APPEND_DATA);
			}
			if (extramode & S_IXOTH) {
				prevmask  &= ~ACE_EXECUTE;
				mask &= ~ACE_EXECUTE;
			}
		}
	}
	aclp->z_ops.ace_mask_set(acep, mask);
	aclp->z_ops.ace_mask_set(prevacep, prevmask);
}

/*
 * Apply the chmod algorithm as described
 * in PSARC/2002/240
 */
static void
zfs_acl_chmod(znode_t *zp, uint64_t mode, zfs_acl_t *aclp)
{
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	void		*acep = NULL, *prevacep = NULL;
	uint64_t	who;
	int 		i;
	int 		entry_type;
	int 		reuse_deny;
	int 		need_canonical_six = 1;
	uint16_t	iflags, type;
	uint32_t	access_mask;

	ASSERT(MUTEX_HELD(&zp->z_acl_lock));
	ASSERT(MUTEX_HELD(&zp->z_lock));

	aclp->z_hints = (zp->z_phys->zp_flags & V4_ACL_WIDE_FLAGS);

	/*
	 * If discard then just discard all ACL nodes which
	 * represent the ACEs.
	 *
	 * New owner@/group@/everone@ ACEs will be added
	 * later.
	 */
	if (zfsvfs->z_acl_mode == ZFS_ACL_DISCARD)
		zfs_acl_release_nodes(aclp);

	while (acep = zfs_acl_next_ace(aclp, acep, &who, &access_mask,
	    &iflags, &type)) {

		entry_type = (iflags & ACE_TYPE_FLAGS);
		iflags = (iflags & ALL_INHERIT);

		if ((type != ALLOW && type != DENY) ||
		    (iflags & ACE_INHERIT_ONLY_ACE)) {
			if (iflags)
				aclp->z_hints |= ZFS_INHERIT_ACE;
			switch (type) {
			case ACE_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
			case ACE_ACCESS_DENIED_OBJECT_ACE_TYPE:
			case ACE_SYSTEM_AUDIT_OBJECT_ACE_TYPE:
			case ACE_SYSTEM_ALARM_OBJECT_ACE_TYPE:
				aclp->z_hints |= ZFS_ACL_OBJ_ACE;
				break;
			}
			goto nextace;
		}

		/*
		 * Need to split ace into two?
		 */
		if ((iflags & (ACE_FILE_INHERIT_ACE|
		    ACE_DIRECTORY_INHERIT_ACE)) &&
		    (!(iflags & ACE_INHERIT_ONLY_ACE))) {
			zfs_acl_split_ace(aclp, acep);
			aclp->z_hints |= ZFS_INHERIT_ACE;
			goto nextace;
		}

		if (entry_type == ACE_OWNER || entry_type == ACE_EVERYONE ||
		    (entry_type == OWNING_GROUP)) {
			access_mask &= ~OGE_CLEAR;
			aclp->z_ops.ace_mask_set(acep, access_mask);
			goto nextace;
		} else {
			reuse_deny = B_TRUE;
			if (type == ALLOW) {

				/*
				 * Check preceding ACE if any, to see
				 * if we need to prepend a DENY ACE.
				 * This is only applicable when the acl_mode
				 * property == groupmask.
				 */
				if (zfsvfs->z_acl_mode == ZFS_ACL_GROUPMASK) {

					reuse_deny = zfs_reuse_deny(aclp, acep,
					    prevacep);

					if (!reuse_deny) {
						prevacep =
						    zfs_acl_prepend_deny(zp,
						    aclp, acep, mode);
					} else {
						zfs_acl_prepend_fixup(
						    aclp, prevacep,
						    acep, mode,
						    zp->z_phys->zp_uid);
					}
					zfs_fixup_group_entries(aclp, acep,
					    prevacep, mode);

				}
			}
		}
nextace:
		prevacep = acep;
	}

	/*
	 * Check out last six aces, if we have six.
	 */

	if (aclp->z_acl_count >= 6) {
		if (zfs_have_canonical_six(aclp)) {
			need_canonical_six = 0;
		}
	}

	if (need_canonical_six) {
		size_t abstract_size = aclp->z_ops.ace_abstract_size();
		void *zacep;
		zfs_acl_node_t *aclnode =
		    zfs_acl_node_alloc(abstract_size * 6);

		aclnode->z_size = abstract_size * 6;
		aclnode->z_ace_count = 6;
		aclp->z_acl_bytes += aclnode->z_size;
		list_insert_tail(&aclp->z_acl, aclnode);

		zacep = aclnode->z_acldata;

		i = 0;
		zfs_set_ace(aclp, (caddr_t)zacep + (abstract_size * i++),
		    0, DENY, -1, ACE_OWNER);
		zfs_set_ace(aclp, (caddr_t)zacep + (abstract_size * i++),
		    OWNER_ALLOW_MASK, ALLOW, -1, ACE_OWNER);
		zfs_set_ace(aclp, (caddr_t)zacep + (abstract_size * i++), 0,
		    DENY, -1, OWNING_GROUP);
		zfs_set_ace(aclp, (caddr_t)zacep + (abstract_size * i++), 0,
		    ALLOW, -1, OWNING_GROUP);
		zfs_set_ace(aclp, (caddr_t)zacep + (abstract_size * i++),
		    EVERYONE_DENY_MASK, DENY, -1, ACE_EVERYONE);
		zfs_set_ace(aclp, (caddr_t)zacep + (abstract_size * i++),
		    EVERYONE_ALLOW_MASK, ALLOW, -1, ACE_EVERYONE);
		aclp->z_acl_count += 6;
	}

	zfs_acl_fixup_canonical_six(aclp, mode);
}

int
zfs_acl_chmod_setattr(znode_t *zp, zfs_acl_t **aclp, uint64_t mode)
{
	int error;

	mutex_enter(&zp->z_lock);
	mutex_enter(&zp->z_acl_lock);
	*aclp = NULL;
	error = zfs_acl_node_read(zp, aclp, B_TRUE);
	if (error == 0)
		zfs_acl_chmod(zp, mode, *aclp);
	mutex_exit(&zp->z_acl_lock);
	mutex_exit(&zp->z_lock);
	return (error);
}

/*
 * strip off write_owner and write_acl
 */
static void
zfs_restricted_update(zfsvfs_t *zfsvfs, zfs_acl_t *aclp, void *acep)
{
	uint32_t mask = aclp->z_ops.ace_mask_get(acep);

	if ((zfsvfs->z_acl_inherit == ZFS_ACL_RESTRICTED) &&
	    (aclp->z_ops.ace_type_get(acep) == ALLOW)) {
		mask &= ~RESTRICTED_CLEAR;
		aclp->z_ops.ace_mask_set(acep, mask);
	}
}

/*
 * Should ACE be inherited?
 */
static int
zfs_ace_can_use(znode_t *zp, uint16_t acep_flags)
{
	int vtype = ZTOV(zp)->v_type;
	int	iflags = (acep_flags & 0xf);

	if ((vtype == VDIR) && (iflags & ACE_DIRECTORY_INHERIT_ACE))
		return (1);
	else if (iflags & ACE_FILE_INHERIT_ACE)
		return (!((vtype == VDIR) &&
		    (iflags & ACE_NO_PROPAGATE_INHERIT_ACE)));
	return (0);
}

/*
 * inherit inheritable ACEs from parent
 */
static zfs_acl_t *
zfs_acl_inherit(znode_t *zp, zfs_acl_t *paclp, boolean_t *need_chmod)
{
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	void		*pacep;
	void		*acep, *acep2;
	zfs_acl_node_t  *aclnode, *aclnode2;
	zfs_acl_t	*aclp = NULL;
	uint64_t	who;
	uint32_t	access_mask;
	uint16_t	iflags, newflags, type;
	size_t		ace_size;
	void		*data1, *data2;
	size_t		data1sz, data2sz;
	enum vtype	vntype = ZTOV(zp)->v_type;

	*need_chmod = B_TRUE;
	pacep = NULL;
	aclp = zfs_acl_alloc(zfs_acl_version_zp(zp));
	if (zfsvfs->z_acl_inherit != ZFS_ACL_DISCARD) {
		while (pacep = zfs_acl_next_ace(paclp, pacep, &who,
		    &access_mask, &iflags, &type)) {

			if (zfsvfs->z_acl_inherit == ZFS_ACL_NOALLOW &&
			    type == ALLOW)
				continue;

			ace_size = aclp->z_ops.ace_size(pacep);

			if (!zfs_ace_can_use(zp, iflags))
				continue;

			/*
			 * If owner@, group@, or everyone@ inheritable
			 * then zfs_acl_chmod() isn't needed.
			 */
			if (zfsvfs->z_acl_inherit ==
			    ZFS_ACL_PASSTHROUGH &&
			    ((iflags & (ACE_OWNER|ACE_EVERYONE)) ||
			    ((iflags & OWNING_GROUP) ==
			    OWNING_GROUP)) && (vntype == VREG ||
			    (vntype == VDIR &&
			    (iflags & ACE_DIRECTORY_INHERIT_ACE))))
				*need_chmod = B_FALSE;

			aclnode = zfs_acl_node_alloc(ace_size);
			list_insert_tail(&aclp->z_acl, aclnode);
			acep = aclnode->z_acldata;
			zfs_set_ace(aclp, acep, access_mask, type,
			    who, iflags|ACE_INHERITED_ACE);

			/*
			 * Copy special opaque data if any
			 */
			if ((data1sz = paclp->z_ops.ace_data(pacep,
			    &data1)) != 0) {
				VERIFY((data2sz = aclp->z_ops.ace_data(acep,
				    &data2)) == data1sz);
				bcopy(data1, data2, data2sz);
			}
			aclp->z_acl_count++;
			aclnode->z_ace_count++;
			aclp->z_acl_bytes += aclnode->z_size;
			newflags = aclp->z_ops.ace_flags_get(acep);

			if (vntype == VDIR)
				aclp->z_hints |= ZFS_INHERIT_ACE;

			if ((iflags & ACE_NO_PROPAGATE_INHERIT_ACE) ||
			    (vntype != VDIR)) {
				newflags &= ~ALL_INHERIT;
				aclp->z_ops.ace_flags_set(acep,
				    newflags|ACE_INHERITED_ACE);
				zfs_restricted_update(zfsvfs, aclp, acep);
				continue;
			}

			ASSERT(vntype == VDIR);

			newflags = aclp->z_ops.ace_flags_get(acep);
			if ((iflags & (ACE_FILE_INHERIT_ACE |
			    ACE_DIRECTORY_INHERIT_ACE)) !=
			    ACE_FILE_INHERIT_ACE) {
				aclnode2 = zfs_acl_node_alloc(ace_size);
				list_insert_tail(&aclp->z_acl, aclnode2);
				acep2 = aclnode2->z_acldata;
				zfs_set_ace(aclp, acep2,
				    access_mask, type, who,
				    iflags|ACE_INHERITED_ACE);
				newflags |= ACE_INHERIT_ONLY_ACE;
				aclp->z_ops.ace_flags_set(acep, newflags);
				newflags &= ~ALL_INHERIT;
				aclp->z_ops.ace_flags_set(acep2,
				    newflags|ACE_INHERITED_ACE);

				/*
				 * Copy special opaque data if any
				 */
				if ((data1sz = aclp->z_ops.ace_data(acep,
				    &data1)) != 0) {
					VERIFY((data2sz =
					    aclp->z_ops.ace_data(acep2,
					    &data2)) == data1sz);
					bcopy(data1, data2, data1sz);
				}
				aclp->z_acl_count++;
				aclnode2->z_ace_count++;
				aclp->z_acl_bytes += aclnode->z_size;
				zfs_restricted_update(zfsvfs, aclp, acep2);
			} else {
				newflags |= ACE_INHERIT_ONLY_ACE;
				aclp->z_ops.ace_flags_set(acep,
				    newflags|ACE_INHERITED_ACE);
			}
		}
	}
	return (aclp);
}

/*
 * Create file system object initial permissions
 * including inheritable ACEs.
 */
void
zfs_perm_init(znode_t *zp, znode_t *parent, int flag,
    vattr_t *vap, dmu_tx_t *tx, cred_t *cr,
    zfs_acl_t *setaclp, zfs_fuid_info_t **fuidp)
{
	uint64_t	mode, fuid, fgid;
	int		error;
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	zfs_acl_t	*aclp = NULL;
	zfs_acl_t	*paclp;
	xvattr_t	*xvap = (xvattr_t *)vap;
	gid_t		gid;
	boolean_t	need_chmod = B_TRUE;

	if (setaclp)
		aclp = setaclp;

	mode = MAKEIMODE(vap->va_type, vap->va_mode);

	/*
	 * Determine uid and gid.
	 */
	if ((flag & (IS_ROOT_NODE | IS_REPLAY)) ||
	    ((flag & IS_XATTR) && (vap->va_type == VDIR))) {
		fuid = zfs_fuid_create(zfsvfs, vap->va_uid, cr,
		    ZFS_OWNER, tx, fuidp);
		fgid = zfs_fuid_create(zfsvfs, vap->va_gid, cr,
		    ZFS_GROUP, tx, fuidp);
		gid = vap->va_gid;
	} else {
		fuid = zfs_fuid_create_cred(zfsvfs, ZFS_OWNER, tx, cr, fuidp);
		fgid = 0;
		if (vap->va_mask & AT_GID)  {
			fgid = zfs_fuid_create(zfsvfs, vap->va_gid, cr,
			    ZFS_GROUP, tx, fuidp);
			gid = vap->va_gid;
			if (fgid != parent->z_phys->zp_gid &&
			    !groupmember(vap->va_gid, cr) &&
			    secpolicy_vnode_create_gid(cr) != 0)
				fgid = 0;
		}
		if (fgid == 0) {
			if (parent->z_phys->zp_mode & S_ISGID) {
				fgid = parent->z_phys->zp_gid;
				gid = zfs_fuid_map_id(zfsvfs, fgid,
				    cr, ZFS_GROUP);
			} else {
				fgid = zfs_fuid_create_cred(zfsvfs,
				    ZFS_GROUP, tx, cr, fuidp);
				gid = crgetgid(cr);
			}
		}
	}

	/*
	 * If we're creating a directory, and the parent directory has the
	 * set-GID bit set, set in on the new directory.
	 * Otherwise, if the user is neither privileged nor a member of the
	 * file's new group, clear the file's set-GID bit.
	 */

	if ((parent->z_phys->zp_mode & S_ISGID) && (vap->va_type == VDIR)) {
		mode |= S_ISGID;
	} else {
		if ((mode & S_ISGID) &&
		    secpolicy_vnode_setids_setgids(cr, gid) != 0)
			mode &= ~S_ISGID;
	}

	zp->z_phys->zp_uid = fuid;
	zp->z_phys->zp_gid = fgid;
	zp->z_phys->zp_mode = mode;

	if (aclp == NULL) {
		mutex_enter(&parent->z_lock);
		if ((ZTOV(parent)->v_type == VDIR &&
		    parent->z_phys->zp_flags & ZFS_INHERIT_ACE)) {
			mutex_enter(&parent->z_acl_lock);
			VERIFY(0 == zfs_acl_node_read(parent, &paclp, B_FALSE));
			mutex_exit(&parent->z_acl_lock);
			aclp = zfs_acl_inherit(zp, paclp, &need_chmod);
			zfs_acl_free(paclp);
		} else {
			aclp = zfs_acl_alloc(zfs_acl_version_zp(zp));
		}
		mutex_exit(&parent->z_lock);
		mutex_enter(&zp->z_lock);
		mutex_enter(&zp->z_acl_lock);
		if (need_chmod)
			zfs_acl_chmod(zp, mode, aclp);
	} else {
		mutex_enter(&zp->z_lock);
		mutex_enter(&zp->z_acl_lock);
	}

	/* Force auto_inherit on all new directory objects */
	if (vap->va_type == VDIR)
		aclp->z_hints |= ZFS_ACL_AUTO_INHERIT;

	error = zfs_aclset_common(zp, aclp, cr, fuidp, tx);

	/* Set optional attributes if any */
	if (vap->va_mask & AT_XVATTR)
		zfs_xvattr_set(zp, xvap);

	mutex_exit(&zp->z_lock);
	mutex_exit(&zp->z_acl_lock);
	ASSERT3U(error, ==, 0);

	if (aclp != setaclp)
		zfs_acl_free(aclp);
}

/*
 * Retrieve a files ACL
 */
int
zfs_getacl(znode_t *zp, vsecattr_t *vsecp, boolean_t skipaclchk, cred_t *cr)
{
	zfs_acl_t	*aclp;
	ulong_t		mask;
	int		error;
	int 		count = 0;
	int		largeace = 0;

	mask = vsecp->vsa_mask & (VSA_ACE | VSA_ACECNT |
	    VSA_ACE_ACLFLAGS | VSA_ACE_ALLTYPES);

	if (error = zfs_zaccess(zp, ACE_READ_ACL, 0, skipaclchk, cr))
		return (error);

	if (mask == 0)
		return (ENOSYS);

	mutex_enter(&zp->z_acl_lock);

	error = zfs_acl_node_read(zp, &aclp, B_FALSE);
	if (error != 0) {
		mutex_exit(&zp->z_acl_lock);
		return (error);
	}

	/*
	 * Scan ACL to determine number of ACEs
	 */
	if ((zp->z_phys->zp_flags & ZFS_ACL_OBJ_ACE) &&
	    !(mask & VSA_ACE_ALLTYPES)) {
		void *zacep = NULL;
		uint64_t who;
		uint32_t access_mask;
		uint16_t type, iflags;

		while (zacep = zfs_acl_next_ace(aclp, zacep,
		    &who, &access_mask, &iflags, &type)) {
			switch (type) {
			case ACE_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
			case ACE_ACCESS_DENIED_OBJECT_ACE_TYPE:
			case ACE_SYSTEM_AUDIT_OBJECT_ACE_TYPE:
			case ACE_SYSTEM_ALARM_OBJECT_ACE_TYPE:
				largeace++;
				continue;
			default:
				count++;
			}
		}
		vsecp->vsa_aclcnt = count;
	} else
		count = aclp->z_acl_count;

	if (mask & VSA_ACECNT) {
		vsecp->vsa_aclcnt = count;
	}

	if (mask & VSA_ACE) {
		size_t aclsz;

		zfs_acl_node_t *aclnode = list_head(&aclp->z_acl);

		aclsz = count * sizeof (ace_t) +
		    sizeof (ace_object_t) * largeace;

		vsecp->vsa_aclentp = kmem_alloc(aclsz, KM_SLEEP);
		vsecp->vsa_aclentsz = aclsz;

		if (aclp->z_version == ZFS_ACL_VERSION_FUID)
			zfs_copy_fuid_2_ace(zp->z_zfsvfs, aclp, cr,
			    vsecp->vsa_aclentp, !(mask & VSA_ACE_ALLTYPES));
		else {
			bcopy(aclnode->z_acldata, vsecp->vsa_aclentp,
			    count * sizeof (ace_t));
		}
	}
	if (mask & VSA_ACE_ACLFLAGS) {
		vsecp->vsa_aclflags = 0;
		if (zp->z_phys->zp_flags & ZFS_ACL_DEFAULTED)
			vsecp->vsa_aclflags |= ACL_DEFAULTED;
		if (zp->z_phys->zp_flags & ZFS_ACL_PROTECTED)
			vsecp->vsa_aclflags |= ACL_PROTECTED;
		if (zp->z_phys->zp_flags & ZFS_ACL_AUTO_INHERIT)
			vsecp->vsa_aclflags |= ACL_AUTO_INHERIT;
	}

	mutex_exit(&zp->z_acl_lock);

	zfs_acl_free(aclp);

	return (0);
}

int
zfs_vsec_2_aclp(zfsvfs_t *zfsvfs, vtype_t obj_type,
    vsecattr_t *vsecp, zfs_acl_t **zaclp)
{
	zfs_acl_t *aclp;
	zfs_acl_node_t *aclnode;
	int aclcnt = vsecp->vsa_aclcnt;
	int error;

	if (vsecp->vsa_aclcnt > MAX_ACL_ENTRIES || vsecp->vsa_aclcnt <= 0)
		return (EINVAL);

	aclp = zfs_acl_alloc(zfs_acl_version(zfsvfs->z_version));

	aclp->z_hints = 0;
	aclnode = zfs_acl_node_alloc(aclcnt * sizeof (zfs_object_ace_t));
	if (aclp->z_version == ZFS_ACL_VERSION_INITIAL) {
		if ((error = zfs_copy_ace_2_oldace(obj_type, aclp,
		    (ace_t *)vsecp->vsa_aclentp, aclnode->z_acldata,
		    aclcnt, &aclnode->z_size)) != 0) {
			zfs_acl_free(aclp);
			zfs_acl_node_free(aclnode);
			return (error);
		}
	} else {
		if ((error = zfs_copy_ace_2_fuid(obj_type, aclp,
		    vsecp->vsa_aclentp, aclnode->z_acldata, aclcnt,
		    &aclnode->z_size)) != 0) {
			zfs_acl_free(aclp);
			zfs_acl_node_free(aclnode);
			return (error);
		}
	}
	aclp->z_acl_bytes = aclnode->z_size;
	aclnode->z_ace_count = aclcnt;
	aclp->z_acl_count = aclcnt;
	list_insert_head(&aclp->z_acl, aclnode);

	/*
	 * If flags are being set then add them to z_hints
	 */
	if (vsecp->vsa_mask & VSA_ACE_ACLFLAGS) {
		if (vsecp->vsa_aclflags & ACL_PROTECTED)
			aclp->z_hints |= ZFS_ACL_PROTECTED;
		if (vsecp->vsa_aclflags & ACL_DEFAULTED)
			aclp->z_hints |= ZFS_ACL_DEFAULTED;
		if (vsecp->vsa_aclflags & ACL_AUTO_INHERIT)
			aclp->z_hints |= ZFS_ACL_AUTO_INHERIT;
	}

	*zaclp = aclp;

	return (0);
}

/*
 * Set a files ACL
 */
int
zfs_setacl(znode_t *zp, vsecattr_t *vsecp, boolean_t skipaclchk, cred_t *cr)
{
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	zilog_t		*zilog = zfsvfs->z_log;
	ulong_t		mask = vsecp->vsa_mask & (VSA_ACE | VSA_ACECNT);
	dmu_tx_t	*tx;
	int		error;
	zfs_acl_t	*aclp;
	zfs_fuid_info_t	*fuidp = NULL;

	if (mask == 0)
		return (ENOSYS);

	if (zp->z_phys->zp_flags & ZFS_IMMUTABLE)
		return (EPERM);

	if (error = zfs_zaccess(zp, ACE_WRITE_ACL, 0, skipaclchk, cr))
		return (error);

	error = zfs_vsec_2_aclp(zfsvfs, ZTOV(zp)->v_type, vsecp, &aclp);
	if (error)
		return (error);

	/*
	 * If ACL wide flags aren't being set then preserve any
	 * existing flags.
	 */
	if (!(vsecp->vsa_mask & VSA_ACE_ACLFLAGS)) {
		aclp->z_hints |= (zp->z_phys->zp_flags & V4_ACL_WIDE_FLAGS);
	}
top:
	if (error = zfs_zaccess(zp, ACE_WRITE_ACL, 0, skipaclchk, cr)) {
		zfs_acl_free(aclp);
		return (error);
	}

	mutex_enter(&zp->z_lock);
	mutex_enter(&zp->z_acl_lock);

	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_bonus(tx, zp->z_id);

	if (zp->z_phys->zp_acl.z_acl_extern_obj) {
		/* Are we upgrading ACL? */
		if (zfsvfs->z_version <= ZPL_VERSION_FUID &&
		    zp->z_phys->zp_acl.z_acl_version ==
		    ZFS_ACL_VERSION_INITIAL) {
			dmu_tx_hold_free(tx,
			    zp->z_phys->zp_acl.z_acl_extern_obj,
			    0, DMU_OBJECT_END);
			dmu_tx_hold_write(tx, DMU_NEW_OBJECT,
			    0, aclp->z_acl_bytes);
		} else {
			dmu_tx_hold_write(tx,
			    zp->z_phys->zp_acl.z_acl_extern_obj,
			    0, aclp->z_acl_bytes);
		}
	} else if (aclp->z_acl_bytes > ZFS_ACE_SPACE) {
		dmu_tx_hold_write(tx, DMU_NEW_OBJECT, 0, aclp->z_acl_bytes);
	}
	if (aclp->z_has_fuids) {
		if (zfsvfs->z_fuid_obj == 0) {
			dmu_tx_hold_bonus(tx, DMU_NEW_OBJECT);
			dmu_tx_hold_write(tx, DMU_NEW_OBJECT, 0,
			    FUID_SIZE_ESTIMATE(zfsvfs));
			dmu_tx_hold_zap(tx, MASTER_NODE_OBJ, FALSE, NULL);
		} else {
			dmu_tx_hold_bonus(tx, zfsvfs->z_fuid_obj);
			dmu_tx_hold_write(tx, zfsvfs->z_fuid_obj, 0,
			    FUID_SIZE_ESTIMATE(zfsvfs));
		}
	}

	error = dmu_tx_assign(tx, zfsvfs->z_assign);
	if (error) {
		mutex_exit(&zp->z_acl_lock);
		mutex_exit(&zp->z_lock);

		if (error == ERESTART && zfsvfs->z_assign == TXG_NOWAIT) {
			dmu_tx_wait(tx);
			dmu_tx_abort(tx);
			goto top;
		}
		dmu_tx_abort(tx);
		zfs_acl_free(aclp);
		return (error);
	}

	error = zfs_aclset_common(zp, aclp, cr, &fuidp, tx);
	ASSERT(error == 0);

	zfs_log_acl(zilog, tx, zp, vsecp, fuidp);

	if (fuidp)
		zfs_fuid_info_free(fuidp);
	zfs_acl_free(aclp);
	dmu_tx_commit(tx);
done:
	mutex_exit(&zp->z_acl_lock);
	mutex_exit(&zp->z_lock);

	return (error);
}

/*
 * working_mode returns the permissions that were not granted
 */
static int
zfs_zaccess_common(znode_t *zp, uint32_t v4_mode, uint32_t *working_mode,
    boolean_t *check_privs, boolean_t skipaclchk, cred_t *cr)
{
	zfs_acl_t	*aclp;
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	int		error;
	uid_t		uid = crgetuid(cr);
	uint64_t 	who;
	uint16_t	type, iflags;
	uint16_t	entry_type;
	uint32_t	access_mask;
	uint32_t	deny_mask = 0;
	zfs_ace_hdr_t	*acep = NULL;
	boolean_t	checkit;
	uid_t		fowner;
	uid_t		gowner;

	/*
	 * Short circuit empty requests
	 */
	if (v4_mode == 0)
		return (0);

	*check_privs = B_TRUE;

	if (zfsvfs->z_assign >= TXG_INITIAL) {		/* ZIL replay */
		*working_mode = 0;
		return (0);
	}

	*working_mode = v4_mode;

	if ((v4_mode & WRITE_MASK) &&
	    (zp->z_zfsvfs->z_vfs->vfs_flag & VFS_RDONLY) &&
	    (!IS_DEVVP(ZTOV(zp)))) {
		*check_privs = B_FALSE;
		return (EROFS);
	}

	/*
	 * Only check for READONLY on non-directories.
	 */
	if ((v4_mode & WRITE_MASK_DATA) &&
	    (((ZTOV(zp)->v_type != VDIR) &&
	    (zp->z_phys->zp_flags & (ZFS_READONLY | ZFS_IMMUTABLE))) ||
	    (ZTOV(zp)->v_type == VDIR &&
	    (zp->z_phys->zp_flags & ZFS_IMMUTABLE)))) {
		*check_privs = B_FALSE;
		return (EPERM);
	}

	if ((v4_mode & (ACE_DELETE | ACE_DELETE_CHILD)) &&
	    (zp->z_phys->zp_flags & ZFS_NOUNLINK)) {
		*check_privs = B_FALSE;
		return (EPERM);
	}

	if (((v4_mode & (ACE_READ_DATA|ACE_EXECUTE)) &&
	    (zp->z_phys->zp_flags & ZFS_AV_QUARANTINED))) {
		*check_privs = B_FALSE;
		return (EACCES);
	}

	/*
	 * The caller requested that the ACL check be skipped.  This
	 * would only happen if the caller checked VOP_ACCESS() with a
	 * 32 bit ACE mask and already had the appropriate permissions.
	 */
	if (skipaclchk) {
		*working_mode = 0;
		return (0);
	}

	zfs_fuid_map_ids(zp, cr, &fowner, &gowner);

	mutex_enter(&zp->z_acl_lock);

	error = zfs_acl_node_read(zp, &aclp, B_FALSE);
	if (error != 0) {
		mutex_exit(&zp->z_acl_lock);
		return (error);
	}

	while (acep = zfs_acl_next_ace(aclp, acep, &who, &access_mask,
	    &iflags, &type)) {

		if (ZTOV(zp)->v_type == VDIR && (iflags & ACE_INHERIT_ONLY_ACE))
			continue;

		entry_type = (iflags & ACE_TYPE_FLAGS);

		checkit = B_FALSE;

		switch (entry_type) {
		case ACE_OWNER:
			if (uid == fowner)
				checkit = B_TRUE;
			break;
		case OWNING_GROUP:
			who = gowner;
			/*FALLTHROUGH*/
		case ACE_IDENTIFIER_GROUP:
			checkit = zfs_groupmember(zfsvfs, who, cr);
			break;
		case ACE_EVERYONE:
			checkit = B_TRUE;
			break;

		/* USER Entry */
		default:
			if (entry_type == 0) {
				uid_t newid;

				newid = zfs_fuid_map_id(zfsvfs, who, cr,
				    ZFS_ACE_USER);
				if (newid != IDMAP_WK_CREATOR_OWNER_UID &&
				    uid == newid)
					checkit = B_TRUE;
				break;
			} else {
				zfs_acl_free(aclp);
				mutex_exit(&zp->z_acl_lock);
				return (EIO);
			}
		}

		if (checkit) {
			uint32_t mask_matched = (access_mask & *working_mode);

			if (mask_matched) {
				if (type == DENY)
					deny_mask |= mask_matched;

				*working_mode &= ~mask_matched;
			}
		}

		/* Are we done? */
		if (*working_mode == 0)
			break;
	}

	mutex_exit(&zp->z_acl_lock);
	zfs_acl_free(aclp);

	/* Put the found 'denies' back on the working mode */
	if (deny_mask) {
		*working_mode |= deny_mask;
		return (EACCES);
	} else if (*working_mode) {
		return (-1);
	}

	return (0);
}

static int
zfs_zaccess_append(znode_t *zp, uint32_t *working_mode, boolean_t *check_privs,
    cred_t *cr)
{
	if (*working_mode != ACE_WRITE_DATA)
		return (EACCES);

	return (zfs_zaccess_common(zp, ACE_APPEND_DATA, working_mode,
	    check_privs, B_FALSE, cr));
}

/*
 * Determine whether Access should be granted/denied, invoking least
 * priv subsytem when a deny is determined.
 */
int
zfs_zaccess(znode_t *zp, int mode, int flags, boolean_t skipaclchk, cred_t *cr)
{
	uint32_t	working_mode;
	int		error;
	int		is_attr;
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	boolean_t 	check_privs;
	znode_t		*xzp;
	znode_t 	*check_zp = zp;

	is_attr = ((zp->z_phys->zp_flags & ZFS_XATTR) &&
	    (ZTOV(zp)->v_type == VDIR));

	/*
	 * If attribute then validate against base file
	 */
	if (is_attr) {
		if ((error = zfs_zget(zp->z_zfsvfs,
		    zp->z_phys->zp_parent, &xzp)) != 0)	{
			return (error);
		}

		check_zp = xzp;

		/*
		 * fixup mode to map to xattr perms
		 */

		if (mode & (ACE_WRITE_DATA|ACE_APPEND_DATA)) {
			mode &= ~(ACE_WRITE_DATA|ACE_APPEND_DATA);
			mode |= ACE_WRITE_NAMED_ATTRS;
		}

		if (mode & (ACE_READ_DATA|ACE_EXECUTE)) {
			mode &= ~(ACE_READ_DATA|ACE_EXECUTE);
			mode |= ACE_READ_NAMED_ATTRS;
		}
	}

	if ((error = zfs_zaccess_common(check_zp, mode, &working_mode,
	    &check_privs, skipaclchk, cr)) == 0) {
		if (is_attr)
			VN_RELE(ZTOV(xzp));
		return (0);
	}

	if (error && !check_privs) {
		if (is_attr)
			VN_RELE(ZTOV(xzp));
		return (error);
	}

	if (error && (flags & V_APPEND)) {
		error = zfs_zaccess_append(zp, &working_mode, &check_privs, cr);
	}

	if (error && check_privs) {
		uid_t		owner;
		mode_t		checkmode = 0;

		owner = zfs_fuid_map_id(zfsvfs, check_zp->z_phys->zp_uid, cr,
		    ZFS_OWNER);

		/*
		 * First check for implicit owner permission on
		 * read_acl/read_attributes
		 */

		error = 0;
		ASSERT(working_mode != 0);

		if ((working_mode & (ACE_READ_ACL|ACE_READ_ATTRIBUTES) &&
		    owner == crgetuid(cr)))
			working_mode &= ~(ACE_READ_ACL|ACE_READ_ATTRIBUTES);

		if (working_mode & (ACE_READ_DATA|ACE_READ_NAMED_ATTRS|
		    ACE_READ_ACL|ACE_READ_ATTRIBUTES))
			checkmode |= VREAD;
		if (working_mode & (ACE_WRITE_DATA|ACE_WRITE_NAMED_ATTRS|
		    ACE_APPEND_DATA|ACE_WRITE_ATTRIBUTES))
			checkmode |= VWRITE;
		if (working_mode & ACE_EXECUTE)
			checkmode |= VEXEC;

		if (checkmode)
			error = secpolicy_vnode_access(cr, ZTOV(check_zp),
			    owner, checkmode);

		if (error == 0 && (working_mode & ACE_WRITE_OWNER))
			error = secpolicy_vnode_create_gid(cr);
		if (error == 0 && (working_mode & ACE_WRITE_ACL))
			error = secpolicy_vnode_setdac(cr, owner);

		if (error == 0 && (working_mode &
		    (ACE_DELETE|ACE_DELETE_CHILD)))
			error = secpolicy_vnode_remove(cr);

		if (error == 0 && (working_mode & ACE_SYNCHRONIZE))
			error = secpolicy_vnode_owner(cr, owner);

		if (error == 0) {
			/*
			 * See if any bits other than those already checked
			 * for are still present.  If so then return EACCES
			 */
			if (working_mode & ~(ZFS_CHECKED_MASKS)) {
				error = EACCES;
			}
		}
	}

	if (is_attr)
		VN_RELE(ZTOV(xzp));

	return (error);
}

/*
 * Translate traditional unix VREAD/VWRITE/VEXEC mode into
 * native ACL format and call zfs_zaccess()
 */
int
zfs_zaccess_rwx(znode_t *zp, mode_t mode, int flags, cred_t *cr)
{
	return (zfs_zaccess(zp, zfs_unix_to_v4(mode >> 6), flags, B_FALSE, cr));
}

/*
 * Access function for secpolicy_vnode_setattr
 */
int
zfs_zaccess_unix(znode_t *zp, mode_t mode, cred_t *cr)
{
	int v4_mode = zfs_unix_to_v4(mode >> 6);

	return (zfs_zaccess(zp, v4_mode, 0, B_FALSE, cr));
}

static int
zfs_delete_final_check(znode_t *zp, znode_t *dzp,
    mode_t missing_perms, cred_t *cr)
{
	int error;
	uid_t downer;
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;

	downer = zfs_fuid_map_id(zfsvfs, dzp->z_phys->zp_uid, cr, ZFS_OWNER);

	error = secpolicy_vnode_access(cr, ZTOV(dzp), downer, missing_perms);

	if (error == 0)
		error = zfs_sticky_remove_access(dzp, zp, cr);

	return (error);
}

/*
 * Determine whether Access should be granted/deny, without
 * consulting least priv subsystem.
 *
 *
 * The following chart is the recommended NFSv4 enforcement for
 * ability to delete an object.
 *
 *      -------------------------------------------------------
 *      |   Parent Dir  |           Target Object Permissions |
 *      |  permissions  |                                     |
 *      -------------------------------------------------------
 *      |               | ACL Allows | ACL Denies| Delete     |
 *      |               |  Delete    |  Delete   | unspecified|
 *      -------------------------------------------------------
 *      |  ACL Allows   | Permit     | Permit    | Permit     |
 *      |  DELETE_CHILD |                                     |
 *      -------------------------------------------------------
 *      |  ACL Denies   | Permit     | Deny      | Deny       |
 *      |  DELETE_CHILD |            |           |            |
 *      -------------------------------------------------------
 *      | ACL specifies |            |           |            |
 *      | only allow    | Permit     | Permit    | Permit     |
 *      | write and     |            |           |            |
 *      | execute       |            |           |            |
 *      -------------------------------------------------------
 *      | ACL denies    |            |           |            |
 *      | write and     | Permit     | Deny      | Deny       |
 *      | execute       |            |           |            |
 *      -------------------------------------------------------
 *         ^
 *         |
 *         No search privilege, can't even look up file?
 *
 */
int
zfs_zaccess_delete(znode_t *dzp, znode_t *zp, cred_t *cr)
{
	uint32_t dzp_working_mode = 0;
	uint32_t zp_working_mode = 0;
	int dzp_error, zp_error;
	mode_t missing_perms;
	boolean_t dzpcheck_privs = B_TRUE;
	boolean_t zpcheck_privs = B_TRUE;

	/*
	 * We want specific DELETE permissions to
	 * take precedence over WRITE/EXECUTE.  We don't
	 * want an ACL such as this to mess us up.
	 * user:joe:write_data:deny,user:joe:delete:allow
	 *
	 * However, deny permissions may ultimately be overridden
	 * by secpolicy_vnode_access().
	 *
	 * We will ask for all of the necessary permissions and then
	 * look at the working modes from the directory and target object
	 * to determine what was found.
	 */

	if (zp->z_phys->zp_flags & (ZFS_IMMUTABLE | ZFS_NOUNLINK))
		return (EPERM);

	/*
	 * First row
	 * If the directory permissions allow the delete, we are done.
	 */
	if ((dzp_error = zfs_zaccess_common(dzp, ACE_DELETE_CHILD,
	    &dzp_working_mode, &dzpcheck_privs, B_FALSE, cr)) == 0)
		return (0);

	/*
	 * If target object has delete permission then we are done
	 */
	if ((zp_error = zfs_zaccess_common(zp, ACE_DELETE, &zp_working_mode,
	    &zpcheck_privs, B_FALSE, cr)) == 0)
		return (0);

	ASSERT(dzp_error && zp_error);

	if (!dzpcheck_privs)
		return (dzp_error);
	if (!zpcheck_privs)
		return (zp_error);

	/*
	 * Second row
	 *
	 * If directory returns EACCES then delete_child was denied
	 * due to deny delete_child.  In this case send the request through
	 * secpolicy_vnode_remove().  We don't use zfs_delete_final_check()
	 * since that *could* allow the delete based on write/execute permission
	 * and we want delete permissions to override write/execute.
	 */

	if (dzp_error == EACCES)
		return (secpolicy_vnode_remove(cr));

	/*
	 * Third Row
	 * only need to see if we have write/execute on directory.
	 */

	if ((dzp_error = zfs_zaccess_common(dzp, ACE_EXECUTE|ACE_WRITE_DATA,
	    &dzp_working_mode, &dzpcheck_privs, B_FALSE, cr)) == 0)
		return (zfs_sticky_remove_access(dzp, zp, cr));

	if (!dzpcheck_privs)
		return (dzp_error);

	/*
	 * Fourth row
	 */

	missing_perms = (dzp_working_mode & ACE_WRITE_DATA) ? VWRITE : 0;
	missing_perms |= (dzp_working_mode & ACE_EXECUTE) ? VEXEC : 0;

	ASSERT(missing_perms);

	return (zfs_delete_final_check(zp, dzp, missing_perms, cr));

}

int
zfs_zaccess_rename(znode_t *sdzp, znode_t *szp, znode_t *tdzp,
    znode_t *tzp, cred_t *cr)
{
	int add_perm;
	int error;

	if (szp->z_phys->zp_flags & ZFS_AV_QUARANTINED)
		return (EACCES);

	add_perm = (ZTOV(szp)->v_type == VDIR) ?
	    ACE_ADD_SUBDIRECTORY : ACE_ADD_FILE;

	/*
	 * Rename permissions are combination of delete permission +
	 * add file/subdir permission.
	 */

	/*
	 * first make sure we do the delete portion.
	 *
	 * If that succeeds then check for add_file/add_subdir permissions
	 */

	if (error = zfs_zaccess_delete(sdzp, szp, cr))
		return (error);

	/*
	 * If we have a tzp, see if we can delete it?
	 */
	if (tzp) {
		if (error = zfs_zaccess_delete(tdzp, tzp, cr))
			return (error);
	}

	/*
	 * Now check for add permissions
	 */
	error = zfs_zaccess(tdzp, add_perm, 0, B_FALSE, cr);

	return (error);
}
