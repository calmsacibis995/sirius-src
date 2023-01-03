/*
 * CDDL HEADER SART
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

#ifndef	_LIBFS_IMPL_H
#define	_LIBFS_IMPL_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/dmu.h>
#include <sys/fs/zfs.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_acl.h>
#include <sys/spa.h>
#include <sys/nvpair.h>

#include <libuutil.h>
#include <libzfs.h>
#include <libshare.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	VERIFY
#undef	VERIFY
#endif
#define	VERIFY	verify

struct libzfs_handle {
	int libzfs_error;
	int libzfs_fd;
	FILE *libzfs_mnttab;
	FILE *libzfs_sharetab;
	zpool_handle_t *libzfs_pool_handles;
	uu_avl_pool_t *libzfs_ns_avlpool;
	uu_avl_t *libzfs_ns_avl;
	uint64_t libzfs_ns_gen;
	int libzfs_desc_active;
	char libzfs_action[1024];
	char libzfs_desc[1024];
	char *libzfs_log_str;
	int libzfs_printerr;
	void *libzfs_sharehdl; /* libshare handle */
	uint_t libzfs_shareflags;
};
#define	ZFSSHARE_MISS	0x01	/* Didn't find entry in cache */

struct zfs_handle {
	libzfs_handle_t *zfs_hdl;
	zpool_handle_t *zpool_hdl;
	char zfs_name[ZFS_MAXNAMELEN];
	zfs_type_t zfs_type; /* type including snapshot */
	zfs_type_t zfs_head_type; /* type excluding snapshot */
	dmu_objset_stats_t zfs_dmustats;
	nvlist_t *zfs_props;
	nvlist_t *zfs_user_props;
	boolean_t zfs_mntcheck;
	char *zfs_mntopts;
};

/*
 * This is different from checking zfs_type, because it will also catch
 * snapshots of volumes.
 */
#define	ZFS_IS_VOLUME(zhp) ((zhp)->zfs_head_type == ZFS_TYPE_VOLUME)

struct zpool_handle {
	libzfs_handle_t *zpool_hdl;
	zpool_handle_t *zpool_next;
	char zpool_name[ZPOOL_MAXNAMELEN];
	int zpool_state;
	size_t zpool_config_size;
	nvlist_t *zpool_config;
	nvlist_t *zpool_old_config;
	nvlist_t *zpool_props;
	diskaddr_t zpool_start_block;
};

typedef  enum {
	PROTO_NFS = 0,
	PROTO_SMB = 1,
	PROTO_END = 2
} zfs_share_proto_t;

/*
 * The following can be used as a bitmask and any new values
 * added must preserve that capability.
 */
typedef enum {
	SHARED_NOT_SHARED = 0x0,
	SHARED_ISCSI = 0x1,
	SHARED_NFS = 0x2,
	SHARED_SMB = 0x4
} zfs_share_type_t;

int zfs_error(libzfs_handle_t *, int, const char *);
int zfs_error_fmt(libzfs_handle_t *, int, const char *, ...);
void zfs_error_aux(libzfs_handle_t *, const char *, ...);
void *zfs_alloc(libzfs_handle_t *, size_t);
void *zfs_realloc(libzfs_handle_t *, void *, size_t, size_t);
char *zfs_strdup(libzfs_handle_t *, const char *);
int no_memory(libzfs_handle_t *);

int zfs_standard_error(libzfs_handle_t *, int, const char *);
int zfs_standard_error_fmt(libzfs_handle_t *, int, const char *, ...);
int zpool_standard_error(libzfs_handle_t *, int, const char *);
int zpool_standard_error_fmt(libzfs_handle_t *, int, const char *, ...);

int get_dependents(libzfs_handle_t *, boolean_t, const char *, char ***,
    size_t *);


int zprop_parse_value(libzfs_handle_t *, nvpair_t *, int, zfs_type_t,
    nvlist_t *, char **, uint64_t *, const char *);
int zprop_expand_list(libzfs_handle_t *hdl, zprop_list_t **plp,
    zfs_type_t type);

typedef struct prop_changelist prop_changelist_t;

int zcmd_alloc_dst_nvlist(libzfs_handle_t *, zfs_cmd_t *, size_t);
int zcmd_write_src_nvlist(libzfs_handle_t *, zfs_cmd_t *, nvlist_t *);
int zcmd_write_conf_nvlist(libzfs_handle_t *, zfs_cmd_t *, nvlist_t *);
int zcmd_expand_dst_nvlist(libzfs_handle_t *, zfs_cmd_t *);
int zcmd_read_dst_nvlist(libzfs_handle_t *, zfs_cmd_t *, nvlist_t **);
void zcmd_free_nvlists(zfs_cmd_t *);

int changelist_prefix(prop_changelist_t *);
int changelist_postfix(prop_changelist_t *);
void changelist_rename(prop_changelist_t *, const char *, const char *);
void changelist_remove(prop_changelist_t *, const char *);
void changelist_free(prop_changelist_t *);
prop_changelist_t *changelist_gather(zfs_handle_t *, zfs_prop_t, int);
int changelist_unshare(prop_changelist_t *, zfs_share_proto_t *);
int changelist_haszonedchild(prop_changelist_t *);

void remove_mountpoint(zfs_handle_t *);
int create_parents(libzfs_handle_t *, char *, int);
boolean_t isa_child_of(const char *dataset, const char *parent);

zfs_handle_t *make_dataset_handle(libzfs_handle_t *, const char *);

int zpool_open_silent(libzfs_handle_t *, const char *, zpool_handle_t **);

int zvol_create_link(libzfs_handle_t *, const char *);
int zvol_remove_link(libzfs_handle_t *, const char *);
int zpool_iter_zvol(zpool_handle_t *, int (*)(const char *, void *), void *);
boolean_t zpool_name_valid(libzfs_handle_t *, boolean_t, const char *);

void namespace_clear(libzfs_handle_t *);

/*
 * libshare (sharemgr) interfaces used internally.
 */

extern int zfs_init_libshare(libzfs_handle_t *, int);
extern void zfs_uninit_libshare(libzfs_handle_t *);
extern int zfs_parse_options(char *, zfs_share_proto_t);

extern int zfs_unshare_proto(zfs_handle_t *zhp,
    const char *, zfs_share_proto_t *);
#ifdef	__cplusplus
}
#endif

#endif	/* _LIBFS_IMPL_H */
