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
 * Virtual Device Labels
 * ---------------------
 *
 * The vdev label serves several distinct purposes:
 *
 *	1. Uniquely identify this device as part of a ZFS pool and confirm its
 *	   identity within the pool.
 *
 * 	2. Verify that all the devices given in a configuration are present
 *         within the pool.
 *
 * 	3. Determine the uberblock for the pool.
 *
 * 	4. In case of an import operation, determine the configuration of the
 *         toplevel vdev of which it is a part.
 *
 * 	5. If an import operation cannot find all the devices in the pool,
 *         provide enough information to the administrator to determine which
 *         devices are missing.
 *
 * It is important to note that while the kernel is responsible for writing the
 * label, it only consumes the information in the first three cases.  The
 * latter information is only consumed in userland when determining the
 * configuration to import a pool.
 *
 *
 * Label Organization
 * ------------------
 *
 * Before describing the contents of the label, it's important to understand how
 * the labels are written and updated with respect to the uberblock.
 *
 * When the pool configuration is altered, either because it was newly created
 * or a device was added, we want to update all the labels such that we can deal
 * with fatal failure at any point.  To this end, each disk has two labels which
 * are updated before and after the uberblock is synced.  Assuming we have
 * labels and an uberblock with the following transaction groups:
 *
 *              L1          UB          L2
 *           +------+    +------+    +------+
 *           |      |    |      |    |      |
 *           | t10  |    | t10  |    | t10  |
 *           |      |    |      |    |      |
 *           +------+    +------+    +------+
 *
 * In this stable state, the labels and the uberblock were all updated within
 * the same transaction group (10).  Each label is mirrored and checksummed, so
 * that we can detect when we fail partway through writing the label.
 *
 * In order to identify which labels are valid, the labels are written in the
 * following manner:
 *
 * 	1. For each vdev, update 'L1' to the new label
 * 	2. Update the uberblock
 * 	3. For each vdev, update 'L2' to the new label
 *
 * Given arbitrary failure, we can determine the correct label to use based on
 * the transaction group.  If we fail after updating L1 but before updating the
 * UB, we will notice that L1's transaction group is greater than the uberblock,
 * so L2 must be valid.  If we fail after writing the uberblock but before
 * writing L2, we will notice that L2's transaction group is less than L1, and
 * therefore L1 is valid.
 *
 * Another added complexity is that not every label is updated when the config
 * is synced.  If we add a single device, we do not want to have to re-write
 * every label for every device in the pool.  This means that both L1 and L2 may
 * be older than the pool uberblock, because the necessary information is stored
 * on another vdev.
 *
 *
 * On-disk Format
 * --------------
 *
 * The vdev label consists of two distinct parts, and is wrapped within the
 * vdev_label_t structure.  The label includes 8k of padding to permit legacy
 * VTOC disk labels, but is otherwise ignored.
 *
 * The first half of the label is a packed nvlist which contains pool wide
 * properties, per-vdev properties, and configuration information.  It is
 * described in more detail below.
 *
 * The latter half of the label consists of a redundant array of uberblocks.
 * These uberblocks are updated whenever a transaction group is committed,
 * or when the configuration is updated.  When a pool is loaded, we scan each
 * vdev for the 'best' uberblock.
 *
 *
 * Configuration Information
 * -------------------------
 *
 * The nvlist describing the pool and vdev contains the following elements:
 *
 * 	version		ZFS on-disk version
 * 	name		Pool name
 * 	state		Pool state
 * 	txg		Transaction group in which this label was written
 * 	pool_guid	Unique identifier for this pool
 * 	vdev_tree	An nvlist describing vdev tree.
 *
 * Each leaf device label also contains the following:
 *
 * 	top_guid	Unique ID for top-level vdev in which this is contained
 * 	guid		Unique ID for the leaf vdev
 *
 * The 'vs' configuration follows the format described in 'spa_config.c'.
 */

#include <sys/zfs_context.h>
#include <sys/spa.h>
#include <sys/spa_impl.h>
#include <sys/dmu.h>
#include <sys/zap.h>
#include <sys/vdev.h>
#include <sys/vdev_impl.h>
#include <sys/uberblock_impl.h>
#include <sys/metaslab.h>
#include <sys/zio.h>
#include <sys/fs/zfs.h>

/*
 * Basic routines to read and write from a vdev label.
 * Used throughout the rest of this file.
 */
uint64_t
vdev_label_offset(uint64_t psize, int l, uint64_t offset)
{
	ASSERT(offset < sizeof (vdev_label_t));
	ASSERT(P2PHASE_TYPED(psize, sizeof (vdev_label_t), uint64_t) == 0);

	return (offset + l * sizeof (vdev_label_t) + (l < VDEV_LABELS / 2 ?
	    0 : psize - VDEV_LABELS * sizeof (vdev_label_t)));
}

/*
 * Returns back the vdev label associated with the passed in offset.
 */
int
vdev_label_number(uint64_t psize, uint64_t offset)
{
	int l;

	if (offset >= psize - VDEV_LABEL_END_SIZE) {
		offset -= psize - VDEV_LABEL_END_SIZE;
		offset += (VDEV_LABELS / 2) * sizeof (vdev_label_t);
	}
	l = offset / sizeof (vdev_label_t);
	return (l < VDEV_LABELS ? l : -1);
}

static void
vdev_label_read(zio_t *zio, vdev_t *vd, int l, void *buf, uint64_t offset,
	uint64_t size, zio_done_func_t *done, void *private)
{
	ASSERT(vd->vdev_children == 0);

	zio_nowait(zio_read_phys(zio, vd,
	    vdev_label_offset(vd->vdev_psize, l, offset),
	    size, buf, ZIO_CHECKSUM_LABEL, done, private,
	    ZIO_PRIORITY_SYNC_READ,
	    ZIO_FLAG_CONFIG_HELD | ZIO_FLAG_CANFAIL | ZIO_FLAG_SPECULATIVE,
	    B_TRUE));
}

static void
vdev_label_write(zio_t *zio, vdev_t *vd, int l, void *buf, uint64_t offset,
	uint64_t size, zio_done_func_t *done, void *private, int flags)
{
	ASSERT(vd->vdev_children == 0);

	zio_nowait(zio_write_phys(zio, vd,
	    vdev_label_offset(vd->vdev_psize, l, offset),
	    size, buf, ZIO_CHECKSUM_LABEL, done, private,
	    ZIO_PRIORITY_SYNC_WRITE, flags, B_TRUE));
}

/*
 * Generate the nvlist representing this vdev's config.
 */
nvlist_t *
vdev_config_generate(spa_t *spa, vdev_t *vd, boolean_t getstats,
    boolean_t isspare, boolean_t isl2cache)
{
	nvlist_t *nv = NULL;

	VERIFY(nvlist_alloc(&nv, NV_UNIQUE_NAME, KM_SLEEP) == 0);

	VERIFY(nvlist_add_string(nv, ZPOOL_CONFIG_TYPE,
	    vd->vdev_ops->vdev_op_type) == 0);
	if (!isspare && !isl2cache)
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_ID, vd->vdev_id)
		    == 0);
	VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_GUID, vd->vdev_guid) == 0);

	if (vd->vdev_path != NULL)
		VERIFY(nvlist_add_string(nv, ZPOOL_CONFIG_PATH,
		    vd->vdev_path) == 0);

	if (vd->vdev_devid != NULL)
		VERIFY(nvlist_add_string(nv, ZPOOL_CONFIG_DEVID,
		    vd->vdev_devid) == 0);

	if (vd->vdev_physpath != NULL)
		VERIFY(nvlist_add_string(nv, ZPOOL_CONFIG_PHYS_PATH,
		    vd->vdev_physpath) == 0);

	if (vd->vdev_nparity != 0) {
		ASSERT(strcmp(vd->vdev_ops->vdev_op_type,
		    VDEV_TYPE_RAIDZ) == 0);

		/*
		 * Make sure someone hasn't managed to sneak a fancy new vdev
		 * into a crufty old storage pool.
		 */
		ASSERT(vd->vdev_nparity == 1 ||
		    (vd->vdev_nparity == 2 &&
		    spa_version(spa) >= SPA_VERSION_RAID6));

		/*
		 * Note that we'll add the nparity tag even on storage pools
		 * that only support a single parity device -- older software
		 * will just ignore it.
		 */
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_NPARITY,
		    vd->vdev_nparity) == 0);
	}

	if (vd->vdev_wholedisk != -1ULL)
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_WHOLE_DISK,
		    vd->vdev_wholedisk) == 0);

	if (vd->vdev_not_present)
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_NOT_PRESENT, 1) == 0);

	if (vd->vdev_isspare)
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_IS_SPARE, 1) == 0);

	if (!isspare && !isl2cache && vd == vd->vdev_top) {
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_METASLAB_ARRAY,
		    vd->vdev_ms_array) == 0);
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_METASLAB_SHIFT,
		    vd->vdev_ms_shift) == 0);
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_ASHIFT,
		    vd->vdev_ashift) == 0);
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_ASIZE,
		    vd->vdev_asize) == 0);
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_IS_LOG,
		    vd->vdev_islog) == 0);
	}

	if (vd->vdev_dtl.smo_object != 0)
		VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_DTL,
		    vd->vdev_dtl.smo_object) == 0);

	if (getstats) {
		vdev_stat_t vs;
		vdev_get_stats(vd, &vs);
		VERIFY(nvlist_add_uint64_array(nv, ZPOOL_CONFIG_STATS,
		    (uint64_t *)&vs, sizeof (vs) / sizeof (uint64_t)) == 0);
	}

	if (!vd->vdev_ops->vdev_op_leaf) {
		nvlist_t **child;
		int c;

		child = kmem_alloc(vd->vdev_children * sizeof (nvlist_t *),
		    KM_SLEEP);

		for (c = 0; c < vd->vdev_children; c++)
			child[c] = vdev_config_generate(spa, vd->vdev_child[c],
			    getstats, isspare, isl2cache);

		VERIFY(nvlist_add_nvlist_array(nv, ZPOOL_CONFIG_CHILDREN,
		    child, vd->vdev_children) == 0);

		for (c = 0; c < vd->vdev_children; c++)
			nvlist_free(child[c]);

		kmem_free(child, vd->vdev_children * sizeof (nvlist_t *));

	} else {
		if (vd->vdev_offline && !vd->vdev_tmpoffline)
			VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_OFFLINE,
			    B_TRUE) == 0);
		if (vd->vdev_faulted)
			VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_FAULTED,
			    B_TRUE) == 0);
		if (vd->vdev_degraded)
			VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_DEGRADED,
			    B_TRUE) == 0);
		if (vd->vdev_removed)
			VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_REMOVED,
			    B_TRUE) == 0);
		if (vd->vdev_unspare)
			VERIFY(nvlist_add_uint64(nv, ZPOOL_CONFIG_UNSPARE,
			    B_TRUE) == 0);
	}

	return (nv);
}

nvlist_t *
vdev_label_read_config(vdev_t *vd)
{
	spa_t *spa = vd->vdev_spa;
	nvlist_t *config = NULL;
	vdev_phys_t *vp;
	zio_t *zio;
	int l;

	ASSERT(spa_config_held(spa, RW_READER) ||
	    spa_config_held(spa, RW_WRITER));

	if (!vdev_readable(vd))
		return (NULL);

	vp = zio_buf_alloc(sizeof (vdev_phys_t));

	for (l = 0; l < VDEV_LABELS; l++) {

		zio = zio_root(spa, NULL, NULL, ZIO_FLAG_CANFAIL |
		    ZIO_FLAG_SPECULATIVE | ZIO_FLAG_CONFIG_HELD);

		vdev_label_read(zio, vd, l, vp,
		    offsetof(vdev_label_t, vl_vdev_phys),
		    sizeof (vdev_phys_t), NULL, NULL);

		if (zio_wait(zio) == 0 &&
		    nvlist_unpack(vp->vp_nvlist, sizeof (vp->vp_nvlist),
		    &config, 0) == 0)
			break;

		if (config != NULL) {
			nvlist_free(config);
			config = NULL;
		}
	}

	zio_buf_free(vp, sizeof (vdev_phys_t));

	return (config);
}

/*
 * Determine if a device is in use.  The 'spare_guid' parameter will be filled
 * in with the device guid if this spare is active elsewhere on the system.
 */
static boolean_t
vdev_inuse(vdev_t *vd, uint64_t crtxg, vdev_labeltype_t reason,
    uint64_t *spare_guid, uint64_t *l2cache_guid)
{
	spa_t *spa = vd->vdev_spa;
	uint64_t state, pool_guid, device_guid, txg, spare_pool;
	uint64_t vdtxg = 0;
	nvlist_t *label;

	if (spare_guid)
		*spare_guid = 0ULL;
	if (l2cache_guid)
		*l2cache_guid = 0ULL;

	/*
	 * Read the label, if any, and perform some basic sanity checks.
	 */
	if ((label = vdev_label_read_config(vd)) == NULL)
		return (B_FALSE);

	(void) nvlist_lookup_uint64(label, ZPOOL_CONFIG_CREATE_TXG,
	    &vdtxg);

	if (nvlist_lookup_uint64(label, ZPOOL_CONFIG_POOL_STATE,
	    &state) != 0 ||
	    nvlist_lookup_uint64(label, ZPOOL_CONFIG_GUID,
	    &device_guid) != 0) {
		nvlist_free(label);
		return (B_FALSE);
	}

	if (state != POOL_STATE_SPARE && state != POOL_STATE_L2CACHE &&
	    (nvlist_lookup_uint64(label, ZPOOL_CONFIG_POOL_GUID,
	    &pool_guid) != 0 ||
	    nvlist_lookup_uint64(label, ZPOOL_CONFIG_POOL_TXG,
	    &txg) != 0)) {
		nvlist_free(label);
		return (B_FALSE);
	}

	nvlist_free(label);

	/*
	 * Check to see if this device indeed belongs to the pool it claims to
	 * be a part of.  The only way this is allowed is if the device is a hot
	 * spare (which we check for later on).
	 */
	if (state != POOL_STATE_SPARE && state != POOL_STATE_L2CACHE &&
	    !spa_guid_exists(pool_guid, device_guid) &&
	    !spa_spare_exists(device_guid, NULL, NULL) &&
	    !spa_l2cache_exists(device_guid, NULL))
		return (B_FALSE);

	/*
	 * If the transaction group is zero, then this an initialized (but
	 * unused) label.  This is only an error if the create transaction
	 * on-disk is the same as the one we're using now, in which case the
	 * user has attempted to add the same vdev multiple times in the same
	 * transaction.
	 */
	if (state != POOL_STATE_SPARE && state != POOL_STATE_L2CACHE &&
	    txg == 0 && vdtxg == crtxg)
		return (B_TRUE);

	/*
	 * Check to see if this is a spare device.  We do an explicit check for
	 * spa_has_spare() here because it may be on our pending list of spares
	 * to add.  We also check if it is an l2cache device.
	 */
	if (spa_spare_exists(device_guid, &spare_pool, NULL) ||
	    spa_has_spare(spa, device_guid)) {
		if (spare_guid)
			*spare_guid = device_guid;

		switch (reason) {
		case VDEV_LABEL_CREATE:
		case VDEV_LABEL_L2CACHE:
			return (B_TRUE);

		case VDEV_LABEL_REPLACE:
			return (!spa_has_spare(spa, device_guid) ||
			    spare_pool != 0ULL);

		case VDEV_LABEL_SPARE:
			return (spa_has_spare(spa, device_guid));
		}
	}

	/*
	 * Check to see if this is an l2cache device.
	 */
	if (spa_l2cache_exists(device_guid, NULL))
		return (B_TRUE);

	/*
	 * If the device is marked ACTIVE, then this device is in use by another
	 * pool on the system.
	 */
	return (state == POOL_STATE_ACTIVE);
}

/*
 * Initialize a vdev label.  We check to make sure each leaf device is not in
 * use, and writable.  We put down an initial label which we will later
 * overwrite with a complete label.  Note that it's important to do this
 * sequentially, not in parallel, so that we catch cases of multiple use of the
 * same leaf vdev in the vdev we're creating -- e.g. mirroring a disk with
 * itself.
 */
int
vdev_label_init(vdev_t *vd, uint64_t crtxg, vdev_labeltype_t reason)
{
	spa_t *spa = vd->vdev_spa;
	nvlist_t *label;
	vdev_phys_t *vp;
	vdev_boot_header_t *vb;
	uberblock_t *ub;
	zio_t *zio;
	int l, c, n;
	char *buf;
	size_t buflen;
	int error;
	uint64_t spare_guid, l2cache_guid;
	int flags = ZIO_FLAG_CONFIG_HELD | ZIO_FLAG_CANFAIL;

	ASSERT(spa_config_held(spa, RW_WRITER));

	for (c = 0; c < vd->vdev_children; c++)
		if ((error = vdev_label_init(vd->vdev_child[c],
		    crtxg, reason)) != 0)
			return (error);

	if (!vd->vdev_ops->vdev_op_leaf)
		return (0);

	/*
	 * Dead vdevs cannot be initialized.
	 */
	if (vdev_is_dead(vd))
		return (EIO);

	/*
	 * Determine if the vdev is in use.
	 */
	if (reason != VDEV_LABEL_REMOVE &&
	    vdev_inuse(vd, crtxg, reason, &spare_guid, &l2cache_guid))
		return (EBUSY);

	ASSERT(reason != VDEV_LABEL_REMOVE ||
	    vdev_inuse(vd, crtxg, reason, NULL, NULL));

	/*
	 * If this is a request to add or replace a spare or l2cache device
	 * that is in use elsewhere on the system, then we must update the
	 * guid (which was initialized to a random value) to reflect the
	 * actual GUID (which is shared between multiple pools).
	 */
	if (reason != VDEV_LABEL_REMOVE && reason != VDEV_LABEL_L2CACHE &&
	    spare_guid != 0ULL) {
		vdev_t *pvd = vd->vdev_parent;

		for (; pvd != NULL; pvd = pvd->vdev_parent) {
			pvd->vdev_guid_sum -= vd->vdev_guid;
			pvd->vdev_guid_sum += spare_guid;
		}

		vd->vdev_guid = vd->vdev_guid_sum = spare_guid;

		/*
		 * If this is a replacement, then we want to fallthrough to the
		 * rest of the code.  If we're adding a spare, then it's already
		 * labeled appropriately and we can just return.
		 */
		if (reason == VDEV_LABEL_SPARE)
			return (0);
		ASSERT(reason == VDEV_LABEL_REPLACE);
	}

	if (reason != VDEV_LABEL_REMOVE && reason != VDEV_LABEL_SPARE &&
	    l2cache_guid != 0ULL) {
		vdev_t *pvd = vd->vdev_parent;

		for (; pvd != NULL; pvd = pvd->vdev_parent) {
			pvd->vdev_guid_sum -= vd->vdev_guid;
			pvd->vdev_guid_sum += l2cache_guid;
		}

		vd->vdev_guid = vd->vdev_guid_sum = l2cache_guid;

		/*
		 * If this is a replacement, then we want to fallthrough to the
		 * rest of the code.  If we're adding an l2cache, then it's
		 * already labeled appropriately and we can just return.
		 */
		if (reason == VDEV_LABEL_L2CACHE)
			return (0);
		ASSERT(reason == VDEV_LABEL_REPLACE);
	}

	/*
	 * Initialize its label.
	 */
	vp = zio_buf_alloc(sizeof (vdev_phys_t));
	bzero(vp, sizeof (vdev_phys_t));

	/*
	 * Generate a label describing the pool and our top-level vdev.
	 * We mark it as being from txg 0 to indicate that it's not
	 * really part of an active pool just yet.  The labels will
	 * be written again with a meaningful txg by spa_sync().
	 */
	if (reason == VDEV_LABEL_SPARE ||
	    (reason == VDEV_LABEL_REMOVE && vd->vdev_isspare)) {
		/*
		 * For inactive hot spares, we generate a special label that
		 * identifies as a mutually shared hot spare.  We write the
		 * label if we are adding a hot spare, or if we are removing an
		 * active hot spare (in which case we want to revert the
		 * labels).
		 */
		VERIFY(nvlist_alloc(&label, NV_UNIQUE_NAME, KM_SLEEP) == 0);

		VERIFY(nvlist_add_uint64(label, ZPOOL_CONFIG_VERSION,
		    spa_version(spa)) == 0);
		VERIFY(nvlist_add_uint64(label, ZPOOL_CONFIG_POOL_STATE,
		    POOL_STATE_SPARE) == 0);
		VERIFY(nvlist_add_uint64(label, ZPOOL_CONFIG_GUID,
		    vd->vdev_guid) == 0);
	} else if (reason == VDEV_LABEL_L2CACHE ||
	    (reason == VDEV_LABEL_REMOVE && vd->vdev_isl2cache)) {
		/*
		 * For level 2 ARC devices, add a special label.
		 */
		VERIFY(nvlist_alloc(&label, NV_UNIQUE_NAME, KM_SLEEP) == 0);

		VERIFY(nvlist_add_uint64(label, ZPOOL_CONFIG_VERSION,
		    spa_version(spa)) == 0);
		VERIFY(nvlist_add_uint64(label, ZPOOL_CONFIG_POOL_STATE,
		    POOL_STATE_L2CACHE) == 0);
		VERIFY(nvlist_add_uint64(label, ZPOOL_CONFIG_GUID,
		    vd->vdev_guid) == 0);
	} else {
		label = spa_config_generate(spa, vd, 0ULL, B_FALSE);

		/*
		 * Add our creation time.  This allows us to detect multiple
		 * vdev uses as described above, and automatically expires if we
		 * fail.
		 */
		VERIFY(nvlist_add_uint64(label, ZPOOL_CONFIG_CREATE_TXG,
		    crtxg) == 0);
	}

	buf = vp->vp_nvlist;
	buflen = sizeof (vp->vp_nvlist);

	error = nvlist_pack(label, &buf, &buflen, NV_ENCODE_XDR, KM_SLEEP);
	if (error != 0) {
		nvlist_free(label);
		zio_buf_free(vp, sizeof (vdev_phys_t));
		/* EFAULT means nvlist_pack ran out of room */
		return (error == EFAULT ? ENAMETOOLONG : EINVAL);
	}

	/*
	 * Initialize boot block header.
	 */
	vb = zio_buf_alloc(sizeof (vdev_boot_header_t));
	bzero(vb, sizeof (vdev_boot_header_t));
	vb->vb_magic = VDEV_BOOT_MAGIC;
	vb->vb_version = VDEV_BOOT_VERSION;
	vb->vb_offset = VDEV_BOOT_OFFSET;
	vb->vb_size = VDEV_BOOT_SIZE;

	/*
	 * Initialize uberblock template.
	 */
	ub = zio_buf_alloc(VDEV_UBERBLOCK_SIZE(vd));
	bzero(ub, VDEV_UBERBLOCK_SIZE(vd));
	*ub = spa->spa_uberblock;
	ub->ub_txg = 0;

	/*
	 * Write everything in parallel.
	 */
	zio = zio_root(spa, NULL, NULL, flags);

	for (l = 0; l < VDEV_LABELS; l++) {

		vdev_label_write(zio, vd, l, vp,
		    offsetof(vdev_label_t, vl_vdev_phys),
		    sizeof (vdev_phys_t), NULL, NULL, flags);

		vdev_label_write(zio, vd, l, vb,
		    offsetof(vdev_label_t, vl_boot_header),
		    sizeof (vdev_boot_header_t), NULL, NULL, flags);

		for (n = 0; n < VDEV_UBERBLOCK_COUNT(vd); n++) {
			vdev_label_write(zio, vd, l, ub,
			    VDEV_UBERBLOCK_OFFSET(vd, n),
			    VDEV_UBERBLOCK_SIZE(vd), NULL, NULL, flags);
		}
	}

	error = zio_wait(zio);

	nvlist_free(label);
	zio_buf_free(ub, VDEV_UBERBLOCK_SIZE(vd));
	zio_buf_free(vb, sizeof (vdev_boot_header_t));
	zio_buf_free(vp, sizeof (vdev_phys_t));

	/*
	 * If this vdev hasn't been previously identified as a spare, then we
	 * mark it as such only if a) we are labeling it as a spare, or b) it
	 * exists as a spare elsewhere in the system.  Do the same for
	 * level 2 ARC devices.
	 */
	if (error == 0 && !vd->vdev_isspare &&
	    (reason == VDEV_LABEL_SPARE ||
	    spa_spare_exists(vd->vdev_guid, NULL, NULL)))
		spa_spare_add(vd);

	if (error == 0 && !vd->vdev_isl2cache &&
	    (reason == VDEV_LABEL_L2CACHE ||
	    spa_l2cache_exists(vd->vdev_guid, NULL)))
		spa_l2cache_add(vd);

	return (error);
}

/*
 * ==========================================================================
 * uberblock load/sync
 * ==========================================================================
 */

/*
 * Consider the following situation: txg is safely synced to disk.  We've
 * written the first uberblock for txg + 1, and then we lose power.  When we
 * come back up, we fail to see the uberblock for txg + 1 because, say,
 * it was on a mirrored device and the replica to which we wrote txg + 1
 * is now offline.  If we then make some changes and sync txg + 1, and then
 * the missing replica comes back, then for a new seconds we'll have two
 * conflicting uberblocks on disk with the same txg.  The solution is simple:
 * among uberblocks with equal txg, choose the one with the latest timestamp.
 */
static int
vdev_uberblock_compare(uberblock_t *ub1, uberblock_t *ub2)
{
	if (ub1->ub_txg < ub2->ub_txg)
		return (-1);
	if (ub1->ub_txg > ub2->ub_txg)
		return (1);

	if (ub1->ub_timestamp < ub2->ub_timestamp)
		return (-1);
	if (ub1->ub_timestamp > ub2->ub_timestamp)
		return (1);

	return (0);
}

static void
vdev_uberblock_load_done(zio_t *zio)
{
	uberblock_t *ub = zio->io_data;
	uberblock_t *ubbest = zio->io_private;
	spa_t *spa = zio->io_spa;

	ASSERT3U(zio->io_size, ==, VDEV_UBERBLOCK_SIZE(zio->io_vd));

	if (zio->io_error == 0 && uberblock_verify(ub) == 0) {
		mutex_enter(&spa->spa_uberblock_lock);
		if (vdev_uberblock_compare(ub, ubbest) > 0)
			*ubbest = *ub;
		mutex_exit(&spa->spa_uberblock_lock);
	}

	zio_buf_free(zio->io_data, zio->io_size);
}

void
vdev_uberblock_load(zio_t *zio, vdev_t *vd, uberblock_t *ubbest)
{
	int l, c, n;

	for (c = 0; c < vd->vdev_children; c++)
		vdev_uberblock_load(zio, vd->vdev_child[c], ubbest);

	if (!vd->vdev_ops->vdev_op_leaf)
		return;

	if (vdev_is_dead(vd))
		return;

	for (l = 0; l < VDEV_LABELS; l++) {
		for (n = 0; n < VDEV_UBERBLOCK_COUNT(vd); n++) {
			vdev_label_read(zio, vd, l,
			    zio_buf_alloc(VDEV_UBERBLOCK_SIZE(vd)),
			    VDEV_UBERBLOCK_OFFSET(vd, n),
			    VDEV_UBERBLOCK_SIZE(vd),
			    vdev_uberblock_load_done, ubbest);
		}
	}
}

/*
 * On success, increment root zio's count of good writes.
 * We only get credit for writes to known-visible vdevs; see spa_vdev_add().
 */
static void
vdev_uberblock_sync_done(zio_t *zio)
{
	uint64_t *good_writes = zio->io_private;

	if (zio->io_error == 0 && zio->io_vd->vdev_top->vdev_ms_array != 0)
		atomic_add_64(good_writes, 1);
}

/*
 * Write the uberblock to all labels of all leaves of the specified vdev.
 */
static void
vdev_uberblock_sync(zio_t *zio, uberblock_t *ub, vdev_t *vd)
{
	int l, c, n;
	uberblock_t *ubbuf;

	for (c = 0; c < vd->vdev_children; c++)
		vdev_uberblock_sync(zio, ub, vd->vdev_child[c]);

	if (!vd->vdev_ops->vdev_op_leaf)
		return;

	if (vdev_is_dead(vd))
		return;

	n = ub->ub_txg & (VDEV_UBERBLOCK_COUNT(vd) - 1);

	ubbuf = zio_buf_alloc(VDEV_UBERBLOCK_SIZE(vd));
	bzero(ubbuf, VDEV_UBERBLOCK_SIZE(vd));
	*ubbuf = *ub;

	for (l = 0; l < VDEV_LABELS; l++)
		vdev_label_write(zio, vd, l, ubbuf,
		    VDEV_UBERBLOCK_OFFSET(vd, n),
		    VDEV_UBERBLOCK_SIZE(vd),
		    vdev_uberblock_sync_done, zio->io_private,
		    ZIO_FLAG_CANFAIL | ZIO_FLAG_DONT_PROPAGATE);

	zio_buf_free(ubbuf, VDEV_UBERBLOCK_SIZE(vd));
}

static void
vdev_uberblock_sync_list_done(zio_t *zio)
{
	uint64_t *good_writes = zio->io_private;

	if (*good_writes == 0)
		zio->io_error = EIO;
}

int
vdev_uberblock_sync_list(vdev_t **svd, int svdcount, uberblock_t *ub, int flags)
{
	spa_t *spa = svd[0]->vdev_spa;
	int v;
	zio_t *zio, *nio;
	uint64_t good_writes = 0;
	int io_flags = flags;

	/*
	 * If we've been asked to update all the vdevs then we change
	 * our flags to ZIO_FLAG_MUSTSUCCEED so that the pipeline can
	 * handle error should all update fail.
	 */
	if (svdcount == spa->spa_root_vdev->vdev_children)
		io_flags &= ~ZIO_FLAG_CANFAIL;

	/*
	 * We rely on the value of good_writes and the root I/O to determine
	 * how a complete failure is handled. In the event that the root is a
	 * ZIO_FLAG_MUSTSUCCED, then the pipeline will block this I/O if we
	 * were unable to update any uberblock. Once the I/O is blocked the
	 * pipeline will retry it when the error is cleared. Unfortunately,
	 * the pipeline does not have the complete I/O tree so it will be
	 * unable to retry the actual uberblock update. Instead we rely on
	 * the value of good_writes to return the failed status to the caller
	 * which will retry on error and thus resubmit the complete I/O
	 * tree.
	 */
	zio = zio_root(spa, NULL, NULL, io_flags);
	nio = zio_null(zio, spa, vdev_uberblock_sync_list_done, &good_writes,
	    flags);
	for (v = 0; v < svdcount; v++)
		vdev_uberblock_sync(nio, ub, svd[v]);
	zio_nowait(nio);
	(void) zio_wait(zio);

	/*
	 * Flush the uberblocks to disk.  This ensures that the odd labels
	 * are no longer needed (because the new uberblocks and the even
	 * labels are safely on disk), so it is safe to overwrite them.
	 */
	zio = zio_root(spa, NULL, NULL, flags);

	for (v = 0; v < svdcount; v++)
		zio_flush(zio, svd[v]);

	(void) zio_wait(zio);

	return (good_writes >= 1 ? 0 : EIO);
}

/*
 * On success, increment the count of good writes for our top-level vdev.
 */
static void
vdev_label_sync_done(zio_t *zio)
{
	uint64_t *good_writes = zio->io_private;

	if (zio->io_error == 0)
		atomic_add_64(good_writes, 1);
}

/*
 * If there weren't enough good writes, indicate failure to the parent.
 */
static void
vdev_label_sync_top_done(zio_t *zio)
{
	uint64_t *good_writes = zio->io_private;

	if (*good_writes == 0)
		zio->io_error = EIO;

	kmem_free(good_writes, sizeof (uint64_t));
}

/*
 * We ignore errors for log and cache devices, simply free the private data.
 */
static void
vdev_label_sync_ignore_done(zio_t *zio)
{
	kmem_free(zio->io_private, sizeof (uint64_t));
}

/*
 * Write all even or odd labels to all leaves of the specified vdev.
 */
static void
vdev_label_sync(zio_t *zio, vdev_t *vd, int l, uint64_t txg)
{
	nvlist_t *label;
	vdev_phys_t *vp;
	char *buf;
	size_t buflen;
	int c;

	for (c = 0; c < vd->vdev_children; c++)
		vdev_label_sync(zio, vd->vdev_child[c], l, txg);

	if (!vd->vdev_ops->vdev_op_leaf)
		return;

	if (vdev_is_dead(vd))
		return;

	/*
	 * Generate a label describing the top-level config to which we belong.
	 */
	label = spa_config_generate(vd->vdev_spa, vd, txg, B_FALSE);

	vp = zio_buf_alloc(sizeof (vdev_phys_t));
	bzero(vp, sizeof (vdev_phys_t));

	buf = vp->vp_nvlist;
	buflen = sizeof (vp->vp_nvlist);

	if (nvlist_pack(label, &buf, &buflen, NV_ENCODE_XDR, KM_SLEEP) == 0) {
		for (; l < VDEV_LABELS; l += 2) {
			vdev_label_write(zio, vd, l, vp,
			    offsetof(vdev_label_t, vl_vdev_phys),
			    sizeof (vdev_phys_t),
			    vdev_label_sync_done, zio->io_private,
			    ZIO_FLAG_CANFAIL | ZIO_FLAG_DONT_PROPAGATE);
		}
	}

	zio_buf_free(vp, sizeof (vdev_phys_t));
	nvlist_free(label);
}

int
vdev_label_sync_list(spa_t *spa, int l, int flags, uint64_t txg)
{
	list_t *dl = &spa->spa_dirty_list;
	vdev_t *vd;
	zio_t *zio, *nio;
	int error;
	int io_flags = flags & ~ZIO_FLAG_CANFAIL;

	/*
	 * The root I/O for all label updates must succeed and we track
	 * the error returned back from the null I/O to determine if we
	 * need to reissue the I/O tree from scratch. If we are unable
	 * to update any leaf vdev associated with a dirty top-level vdev,
	 * then the pipeline will either suspend or panic when the root I/O
	 * is issued. If the error is cleared, then the pipleine will retry
	 * the root I/O. Unfortunately we've lost the entire I/O tree so we
	 * return back the original error to the caller and allow the caller
	 * to call use again so that we can build the I/O tree from scratch.
	 */
	zio = zio_root(spa, NULL, NULL, io_flags);
	nio = zio_null(zio, spa, NULL, NULL, flags);

	for (vd = list_head(dl); vd != NULL; vd = list_next(dl, vd)) {
		uint64_t *good_writes = kmem_zalloc(sizeof (uint64_t),
		    KM_SLEEP);
		zio_t *vio = zio_null(nio, spa,
		    (vd->vdev_islog || vd->vdev_aux != NULL) ?
		    vdev_label_sync_ignore_done : vdev_label_sync_top_done,
		    good_writes, flags);
		vdev_label_sync(vio, vd, l, txg);
		zio_nowait(vio);
	}
	error = zio_wait(nio);
	(void) zio_wait(zio);

	/*
	 * Flush the new labels to disk.
	 */
	zio = zio_root(spa, NULL, NULL, flags);

	for (vd = list_head(dl); vd != NULL; vd = list_next(dl, vd))
		zio_flush(zio, vd);

	(void) zio_wait(zio);

	return (error);
}

/*
 * Sync the uberblock and any changes to the vdev configuration.
 *
 * The order of operations is carefully crafted to ensure that
 * if the system panics or loses power at any time, the state on disk
 * is still transactionally consistent.  The in-line comments below
 * describe the failure semantics at each stage.
 *
 * Moreover, vdev_config_sync() is designed to be idempotent: if it fails
 * at any time, you can just call it again, and it will resume its work.
 */
void
vdev_config_sync(vdev_t **svd, int svdcount, uint64_t txg)
{
	spa_t *spa = svd[0]->vdev_spa;
	uberblock_t *ub = &spa->spa_uberblock;
	vdev_t *vd;
	zio_t *zio;
	int flags = ZIO_FLAG_CONFIG_HELD | ZIO_FLAG_CANFAIL;

	ASSERT(ub->ub_txg <= txg);

	/*
	 * If this isn't a resync due to I/O errors,
	 * and nothing changed in this transaction group,
	 * and the vdev configuration hasn't changed,
	 * then there's nothing to do.
	 */
	if (ub->ub_txg < txg &&
	    uberblock_update(ub, spa->spa_root_vdev, txg) == B_FALSE &&
	    list_is_empty(&spa->spa_dirty_list))
		return;

	if (txg > spa_freeze_txg(spa))
		return;

	ASSERT(txg <= spa->spa_final_txg);

	/*
	 * Flush the write cache of every disk that's been written to
	 * in this transaction group.  This ensures that all blocks
	 * written in this txg will be committed to stable storage
	 * before any uberblock that references them.
	 */
	zio = zio_root(spa, NULL, NULL, flags);

	for (vd = txg_list_head(&spa->spa_vdev_txg_list, TXG_CLEAN(txg)); vd;
	    vd = txg_list_next(&spa->spa_vdev_txg_list, vd, TXG_CLEAN(txg)))
		zio_flush(zio, vd);

	(void) zio_wait(zio);

	/*
	 * Sync out the even labels (L0, L2) for every dirty vdev.  If the
	 * system dies in the middle of this process, that's OK: all of the
	 * even labels that made it to disk will be newer than any uberblock,
	 * and will therefore be considered invalid.  The odd labels (L1, L3),
	 * which have not yet been touched, will still be valid.  We flush
	 * the new labels to disk to ensure that all even-label updates
	 * are committed to stable storage before the uberblock update.
	 * Failure to update any of the labels will invoke the 'failmode'
	 * code path. Thus we must retry the entire I/O tree once the error
	 * is cleared and we ar resumed.
	 */
	while (vdev_label_sync_list(spa, 0, flags, txg) != 0)
		;

	/*
	 * Sync the uberblocks to all vdevs in svd[]. If we are unable
	 * to do so, then we attempt to sync out to all top-level vdevs.
	 * If the system dies in the middle of this step, there are two cases
	 * to consider, and the on-disk state is consistent either way:
	 *
	 * (1)	If none of the new uberblocks made it to disk, then the
	 *	previous uberblock will be the newest, and the odd labels
	 *	(which had not yet been touched) will be valid with respect
	 *	to that uberblock.
	 *
	 * (2)	If one or more new uberblocks made it to disk, then they
	 *	will be the newest, and the even labels (which had all
	 *	been successfully committed) will be valid with respect
	 *	to the new uberblocks.
	 *
	 * In addition, if we have failed to update all the uberblocks then
	 * we will follow the 'failmode' code path. We must retry the entire
	 * I/O tree if we are resumed.
	 */
	if (vdev_uberblock_sync_list(svd, svdcount, ub, flags) != 0) {
		vdev_t *rvd = spa->spa_root_vdev;

		while (vdev_uberblock_sync_list(rvd->vdev_child,
		    rvd->vdev_children, ub, flags))
			;
	}

	/*
	 * Sync out odd labels for every dirty vdev.  If the system dies
	 * in the middle of this process, the even labels and the new
	 * uberblocks will suffice to open the pool.  The next time
	 * the pool is opened, the first thing we'll do -- before any
	 * user data is modified -- is mark every vdev dirty so that
	 * all labels will be brought up to date.  We flush the new labels
	 * to disk to ensure that all odd-label updates are committed to
	 * stable storage before the next transaction group begins.
	 * Failure to update any of the labels will invoke the 'failmode'
	 * code path. Thus we must retry the entire I/O tree once the error
	 * is cleared and we are resumed.
	 */
	while (vdev_label_sync_list(spa, 1, flags, txg) != 0)
		;
}
