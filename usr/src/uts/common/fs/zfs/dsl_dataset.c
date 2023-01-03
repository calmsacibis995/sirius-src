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

#include <sys/dmu_objset.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_dir.h>
#include <sys/dsl_prop.h>
#include <sys/dsl_synctask.h>
#include <sys/dmu_traverse.h>
#include <sys/dmu_tx.h>
#include <sys/arc.h>
#include <sys/zio.h>
#include <sys/zap.h>
#include <sys/unique.h>
#include <sys/zfs_context.h>
#include <sys/zfs_ioctl.h>
#include <sys/spa.h>
#include <sys/zfs_znode.h>
#include <sys/sunddi.h>

static char *dsl_reaper = "the grim reaper";

static dsl_checkfunc_t dsl_dataset_destroy_begin_check;
static dsl_syncfunc_t dsl_dataset_destroy_begin_sync;
static dsl_checkfunc_t dsl_dataset_rollback_check;
static dsl_syncfunc_t dsl_dataset_rollback_sync;
static dsl_syncfunc_t dsl_dataset_set_reservation_sync;

#define	DS_REF_MAX	(1ULL << 62)

#define	DSL_DEADLIST_BLOCKSIZE	SPA_MAXBLOCKSIZE

#define	DSL_DATASET_IS_DESTROYED(ds)	((ds)->ds_owner == dsl_reaper)


/*
 * Figure out how much of this delta should be propogated to the dsl_dir
 * layer.  If there's a refreservation, that space has already been
 * partially accounted for in our ancestors.
 */
static int64_t
parent_delta(dsl_dataset_t *ds, int64_t delta)
{
	uint64_t old_bytes, new_bytes;

	if (ds->ds_reserved == 0)
		return (delta);

	old_bytes = MAX(ds->ds_phys->ds_unique_bytes, ds->ds_reserved);
	new_bytes = MAX(ds->ds_phys->ds_unique_bytes + delta, ds->ds_reserved);

	ASSERT3U(ABS((int64_t)(new_bytes - old_bytes)), <=, ABS(delta));
	return (new_bytes - old_bytes);
}

void
dsl_dataset_block_born(dsl_dataset_t *ds, blkptr_t *bp, dmu_tx_t *tx)
{
	int used = bp_get_dasize(tx->tx_pool->dp_spa, bp);
	int compressed = BP_GET_PSIZE(bp);
	int uncompressed = BP_GET_UCSIZE(bp);
	int64_t delta;

	dprintf_bp(bp, "born, ds=%p\n", ds);

	ASSERT(dmu_tx_is_syncing(tx));
	/* It could have been compressed away to nothing */
	if (BP_IS_HOLE(bp))
		return;
	ASSERT(BP_GET_TYPE(bp) != DMU_OT_NONE);
	ASSERT3U(BP_GET_TYPE(bp), <, DMU_OT_NUMTYPES);
	if (ds == NULL) {
		/*
		 * Account for the meta-objset space in its placeholder
		 * dsl_dir.
		 */
		ASSERT3U(compressed, ==, uncompressed); /* it's all metadata */
		dsl_dir_diduse_space(tx->tx_pool->dp_mos_dir,
		    used, compressed, uncompressed, tx);
		dsl_dir_dirty(tx->tx_pool->dp_mos_dir, tx);
		return;
	}
	dmu_buf_will_dirty(ds->ds_dbuf, tx);
	mutex_enter(&ds->ds_lock);
	delta = parent_delta(ds, used);
	ds->ds_phys->ds_used_bytes += used;
	ds->ds_phys->ds_compressed_bytes += compressed;
	ds->ds_phys->ds_uncompressed_bytes += uncompressed;
	ds->ds_phys->ds_unique_bytes += used;
	mutex_exit(&ds->ds_lock);
	dsl_dir_diduse_space(ds->ds_dir, delta, compressed, uncompressed, tx);
}

int
dsl_dataset_block_kill(dsl_dataset_t *ds, blkptr_t *bp, zio_t *pio,
    dmu_tx_t *tx)
{
	int used = bp_get_dasize(tx->tx_pool->dp_spa, bp);
	int compressed = BP_GET_PSIZE(bp);
	int uncompressed = BP_GET_UCSIZE(bp);

	ASSERT(dmu_tx_is_syncing(tx));
	/* No block pointer => nothing to free */
	if (BP_IS_HOLE(bp))
		return (0);

	ASSERT(used > 0);
	if (ds == NULL) {
		int err;
		/*
		 * Account for the meta-objset space in its placeholder
		 * dataset.
		 */
		err = dsl_free(pio, tx->tx_pool,
		    tx->tx_txg, bp, NULL, NULL, pio ? ARC_NOWAIT: ARC_WAIT);
		ASSERT(err == 0);

		dsl_dir_diduse_space(tx->tx_pool->dp_mos_dir,
		    -used, -compressed, -uncompressed, tx);
		dsl_dir_dirty(tx->tx_pool->dp_mos_dir, tx);
		return (used);
	}
	ASSERT3P(tx->tx_pool, ==, ds->ds_dir->dd_pool);

	dmu_buf_will_dirty(ds->ds_dbuf, tx);

	if (bp->blk_birth > ds->ds_phys->ds_prev_snap_txg) {
		int err;
		int64_t delta;

		dprintf_bp(bp, "freeing: %s", "");
		err = dsl_free(pio, tx->tx_pool,
		    tx->tx_txg, bp, NULL, NULL, pio ? ARC_NOWAIT: ARC_WAIT);
		ASSERT(err == 0);

		mutex_enter(&ds->ds_lock);
		ASSERT(ds->ds_phys->ds_unique_bytes >= used ||
		    !DS_UNIQUE_IS_ACCURATE(ds));
		delta = parent_delta(ds, -used);
		ds->ds_phys->ds_unique_bytes -= used;
		mutex_exit(&ds->ds_lock);
		dsl_dir_diduse_space(ds->ds_dir,
		    delta, -compressed, -uncompressed, tx);
	} else {
		dprintf_bp(bp, "putting on dead list: %s", "");
		VERIFY(0 == bplist_enqueue(&ds->ds_deadlist, bp, tx));
		ASSERT3U(ds->ds_prev->ds_object, ==,
		    ds->ds_phys->ds_prev_snap_obj);
		ASSERT(ds->ds_prev->ds_phys->ds_num_children > 0);
		/* if (bp->blk_birth > prev prev snap txg) prev unique += bs */
		if (ds->ds_prev->ds_phys->ds_next_snap_obj ==
		    ds->ds_object && bp->blk_birth >
		    ds->ds_prev->ds_phys->ds_prev_snap_txg) {
			dmu_buf_will_dirty(ds->ds_prev->ds_dbuf, tx);
			mutex_enter(&ds->ds_prev->ds_lock);
			ds->ds_prev->ds_phys->ds_unique_bytes += used;
			mutex_exit(&ds->ds_prev->ds_lock);
		}
	}
	mutex_enter(&ds->ds_lock);
	ASSERT3U(ds->ds_phys->ds_used_bytes, >=, used);
	ds->ds_phys->ds_used_bytes -= used;
	ASSERT3U(ds->ds_phys->ds_compressed_bytes, >=, compressed);
	ds->ds_phys->ds_compressed_bytes -= compressed;
	ASSERT3U(ds->ds_phys->ds_uncompressed_bytes, >=, uncompressed);
	ds->ds_phys->ds_uncompressed_bytes -= uncompressed;
	mutex_exit(&ds->ds_lock);

	return (used);
}

uint64_t
dsl_dataset_prev_snap_txg(dsl_dataset_t *ds)
{
	uint64_t trysnap = 0;

	if (ds == NULL)
		return (0);
	/*
	 * The snapshot creation could fail, but that would cause an
	 * incorrect FALSE return, which would only result in an
	 * overestimation of the amount of space that an operation would
	 * consume, which is OK.
	 *
	 * There's also a small window where we could miss a pending
	 * snapshot, because we could set the sync task in the quiescing
	 * phase.  So this should only be used as a guess.
	 */
	if (ds->ds_trysnap_txg >
	    spa_last_synced_txg(ds->ds_dir->dd_pool->dp_spa))
		trysnap = ds->ds_trysnap_txg;
	return (MAX(ds->ds_phys->ds_prev_snap_txg, trysnap));
}

int
dsl_dataset_block_freeable(dsl_dataset_t *ds, uint64_t blk_birth)
{
	return (blk_birth > dsl_dataset_prev_snap_txg(ds));
}

/* ARGSUSED */
static void
dsl_dataset_evict(dmu_buf_t *db, void *dsv)
{
	dsl_dataset_t *ds = dsv;

	ASSERT(ds->ds_owner == NULL || DSL_DATASET_IS_DESTROYED(ds));

	dprintf_ds(ds, "evicting %s\n", "");

	unique_remove(ds->ds_fsid_guid);

	if (ds->ds_user_ptr != NULL)
		ds->ds_user_evict_func(ds, ds->ds_user_ptr);

	if (ds->ds_prev) {
		dsl_dataset_drop_ref(ds->ds_prev, ds);
		ds->ds_prev = NULL;
	}

	bplist_close(&ds->ds_deadlist);
	if (ds->ds_dir)
		dsl_dir_close(ds->ds_dir, ds);

	ASSERT(!list_link_active(&ds->ds_synced_link));

	mutex_destroy(&ds->ds_lock);
	mutex_destroy(&ds->ds_opening_lock);
	mutex_destroy(&ds->ds_deadlist.bpl_lock);
	rw_destroy(&ds->ds_rwlock);
	cv_destroy(&ds->ds_exclusive_cv);

	kmem_free(ds, sizeof (dsl_dataset_t));
}

static int
dsl_dataset_get_snapname(dsl_dataset_t *ds)
{
	dsl_dataset_phys_t *headphys;
	int err;
	dmu_buf_t *headdbuf;
	dsl_pool_t *dp = ds->ds_dir->dd_pool;
	objset_t *mos = dp->dp_meta_objset;

	if (ds->ds_snapname[0])
		return (0);
	if (ds->ds_phys->ds_next_snap_obj == 0)
		return (0);

	err = dmu_bonus_hold(mos, ds->ds_dir->dd_phys->dd_head_dataset_obj,
	    FTAG, &headdbuf);
	if (err)
		return (err);
	headphys = headdbuf->db_data;
	err = zap_value_search(dp->dp_meta_objset,
	    headphys->ds_snapnames_zapobj, ds->ds_object, 0, ds->ds_snapname);
	dmu_buf_rele(headdbuf, FTAG);
	return (err);
}

static int
dsl_dataset_snap_lookup(dsl_dataset_t *ds, const char *name, uint64_t *value)
{
	objset_t *mos = ds->ds_dir->dd_pool->dp_meta_objset;
	uint64_t snapobj = ds->ds_phys->ds_snapnames_zapobj;
	matchtype_t mt;
	int err;

	if (ds->ds_phys->ds_flags & DS_FLAG_CI_DATASET)
		mt = MT_FIRST;
	else
		mt = MT_EXACT;

	err = zap_lookup_norm(mos, snapobj, name, 8, 1,
	    value, mt, NULL, 0, NULL);
	if (err == ENOTSUP && mt == MT_FIRST)
		err = zap_lookup(mos, snapobj, name, 8, 1, value);
	return (err);
}

static int
dsl_dataset_snap_remove(dsl_dataset_t *ds, char *name, dmu_tx_t *tx)
{
	objset_t *mos = ds->ds_dir->dd_pool->dp_meta_objset;
	uint64_t snapobj = ds->ds_phys->ds_snapnames_zapobj;
	matchtype_t mt;
	int err;

	if (ds->ds_phys->ds_flags & DS_FLAG_CI_DATASET)
		mt = MT_FIRST;
	else
		mt = MT_EXACT;

	err = zap_remove_norm(mos, snapobj, name, mt, tx);
	if (err == ENOTSUP && mt == MT_FIRST)
		err = zap_remove(mos, snapobj, name, tx);
	return (err);
}

static int
dsl_dataset_get_ref(dsl_pool_t *dp, uint64_t dsobj, void *tag,
    dsl_dataset_t **dsp)
{
	objset_t *mos = dp->dp_meta_objset;
	dmu_buf_t *dbuf;
	dsl_dataset_t *ds;
	int err;

	ASSERT(RW_LOCK_HELD(&dp->dp_config_rwlock) ||
	    dsl_pool_sync_context(dp));

	err = dmu_bonus_hold(mos, dsobj, tag, &dbuf);
	if (err)
		return (err);
	ds = dmu_buf_get_user(dbuf);
	if (ds == NULL) {
		dsl_dataset_t *winner;

		ds = kmem_zalloc(sizeof (dsl_dataset_t), KM_SLEEP);
		ds->ds_dbuf = dbuf;
		ds->ds_object = dsobj;
		ds->ds_phys = dbuf->db_data;

		mutex_init(&ds->ds_lock, NULL, MUTEX_DEFAULT, NULL);
		mutex_init(&ds->ds_opening_lock, NULL, MUTEX_DEFAULT, NULL);
		mutex_init(&ds->ds_deadlist.bpl_lock, NULL, MUTEX_DEFAULT,
		    NULL);
		rw_init(&ds->ds_rwlock, 0, 0, 0);
		cv_init(&ds->ds_exclusive_cv, NULL, CV_DEFAULT, NULL);

		err = bplist_open(&ds->ds_deadlist,
		    mos, ds->ds_phys->ds_deadlist_obj);
		if (err == 0) {
			err = dsl_dir_open_obj(dp,
			    ds->ds_phys->ds_dir_obj, NULL, ds, &ds->ds_dir);
		}
		if (err) {
			/*
			 * we don't really need to close the blist if we
			 * just opened it.
			 */
			mutex_destroy(&ds->ds_lock);
			mutex_destroy(&ds->ds_opening_lock);
			mutex_destroy(&ds->ds_deadlist.bpl_lock);
			rw_destroy(&ds->ds_rwlock);
			cv_destroy(&ds->ds_exclusive_cv);
			kmem_free(ds, sizeof (dsl_dataset_t));
			dmu_buf_rele(dbuf, tag);
			return (err);
		}

		if (ds->ds_dir->dd_phys->dd_head_dataset_obj == dsobj) {
			ds->ds_snapname[0] = '\0';
			if (ds->ds_phys->ds_prev_snap_obj) {
				err = dsl_dataset_get_ref(dp,
				    ds->ds_phys->ds_prev_snap_obj,
				    ds, &ds->ds_prev);
			}
		} else if (zfs_flags & ZFS_DEBUG_SNAPNAMES) {
			err = dsl_dataset_get_snapname(ds);
		}

		if (!dsl_dataset_is_snapshot(ds)) {
			/*
			 * In sync context, we're called with either no lock
			 * or with the write lock.  If we're not syncing,
			 * we're always called with the read lock held.
			 */
			boolean_t need_lock =
			    !RW_WRITE_HELD(&dp->dp_config_rwlock) &&
			    dsl_pool_sync_context(dp);

			if (need_lock)
				rw_enter(&dp->dp_config_rwlock, RW_READER);

			err = dsl_prop_get_ds(ds,
			    "refreservation", sizeof (uint64_t), 1,
			    &ds->ds_reserved, NULL);
			if (err == 0) {
				err = dsl_prop_get_ds(ds,
				    "refquota", sizeof (uint64_t), 1,
				    &ds->ds_quota, NULL);
			}

			if (need_lock)
				rw_exit(&dp->dp_config_rwlock);
		} else {
			ds->ds_reserved = ds->ds_quota = 0;
		}

		if (err == 0) {
			winner = dmu_buf_set_user_ie(dbuf, ds, &ds->ds_phys,
			    dsl_dataset_evict);
		}
		if (err || winner) {
			bplist_close(&ds->ds_deadlist);
			if (ds->ds_prev)
				dsl_dataset_drop_ref(ds->ds_prev, ds);
			dsl_dir_close(ds->ds_dir, ds);
			mutex_destroy(&ds->ds_lock);
			mutex_destroy(&ds->ds_opening_lock);
			mutex_destroy(&ds->ds_deadlist.bpl_lock);
			rw_destroy(&ds->ds_rwlock);
			cv_destroy(&ds->ds_exclusive_cv);
			kmem_free(ds, sizeof (dsl_dataset_t));
			if (err) {
				dmu_buf_rele(dbuf, tag);
				return (err);
			}
			ds = winner;
		} else {
			ds->ds_fsid_guid =
			    unique_insert(ds->ds_phys->ds_fsid_guid);
		}
	}
	ASSERT3P(ds->ds_dbuf, ==, dbuf);
	ASSERT3P(ds->ds_phys, ==, dbuf->db_data);
	ASSERT(ds->ds_phys->ds_prev_snap_obj != 0 ||
	    spa_version(dp->dp_spa) < SPA_VERSION_ORIGIN ||
	    dp->dp_origin_snap == NULL || ds == dp->dp_origin_snap);
	mutex_enter(&ds->ds_lock);
	if (!dsl_pool_sync_context(dp) && DSL_DATASET_IS_DESTROYED(ds)) {
		mutex_exit(&ds->ds_lock);
		dmu_buf_rele(ds->ds_dbuf, tag);
		return (ENOENT);
	}
	mutex_exit(&ds->ds_lock);
	*dsp = ds;
	return (0);
}

static int
dsl_dataset_hold_ref(dsl_dataset_t *ds, void *tag)
{
	dsl_pool_t *dp = ds->ds_dir->dd_pool;

	/*
	 * In syncing context we don't want the rwlock lock: there
	 * may be an existing writer waiting for sync phase to
	 * finish.  We don't need to worry about such writers, since
	 * sync phase is single-threaded, so the writer can't be
	 * doing anything while we are active.
	 */
	if (dsl_pool_sync_context(dp)) {
		ASSERT(!DSL_DATASET_IS_DESTROYED(ds));
		return (0);
	}

	/*
	 * Normal users will hold the ds_rwlock as a READER until they
	 * are finished (i.e., call dsl_dataset_rele()).  "Owners" will
	 * drop their READER lock after they set the ds_owner field.
	 *
	 * If the dataset is being destroyed, the destroy thread will
	 * obtain a WRITER lock for exclusive access after it's done its
	 * open-context work and then change the ds_owner to
	 * dsl_reaper once destruction is assured.  So threads
	 * may block here temporarily, until the "destructability" of
	 * the dataset is determined.
	 */
	ASSERT(!RW_WRITE_HELD(&dp->dp_config_rwlock));
	mutex_enter(&ds->ds_lock);
	while (!rw_tryenter(&ds->ds_rwlock, RW_READER)) {
		rw_exit(&dp->dp_config_rwlock);
		cv_wait(&ds->ds_exclusive_cv, &ds->ds_lock);
		if (DSL_DATASET_IS_DESTROYED(ds)) {
			mutex_exit(&ds->ds_lock);
			dsl_dataset_drop_ref(ds, tag);
			rw_enter(&dp->dp_config_rwlock, RW_READER);
			return (ENOENT);
		}
		rw_enter(&dp->dp_config_rwlock, RW_READER);
	}
	mutex_exit(&ds->ds_lock);
	return (0);
}

int
dsl_dataset_hold_obj(dsl_pool_t *dp, uint64_t dsobj, void *tag,
    dsl_dataset_t **dsp)
{
	int err = dsl_dataset_get_ref(dp, dsobj, tag, dsp);

	if (err)
		return (err);
	return (dsl_dataset_hold_ref(*dsp, tag));
}

int
dsl_dataset_own_obj(dsl_pool_t *dp, uint64_t dsobj, int flags, void *owner,
    dsl_dataset_t **dsp)
{
	int err = dsl_dataset_hold_obj(dp, dsobj, owner, dsp);

	ASSERT(DS_MODE_TYPE(flags) != DS_MODE_USER);

	if (err)
		return (err);
	if (!dsl_dataset_tryown(*dsp, DS_MODE_IS_INCONSISTENT(flags), owner)) {
		dsl_dataset_rele(*dsp, owner);
		return (EBUSY);
	}
	return (0);
}

int
dsl_dataset_hold(const char *name, void *tag, dsl_dataset_t **dsp)
{
	dsl_dir_t *dd;
	dsl_pool_t *dp;
	const char *snapname;
	uint64_t obj;
	int err = 0;

	err = dsl_dir_open_spa(NULL, name, FTAG, &dd, &snapname);
	if (err)
		return (err);

	dp = dd->dd_pool;
	obj = dd->dd_phys->dd_head_dataset_obj;
	rw_enter(&dp->dp_config_rwlock, RW_READER);
	if (obj)
		err = dsl_dataset_get_ref(dp, obj, tag, dsp);
	else
		err = ENOENT;
	if (err)
		goto out;

	err = dsl_dataset_hold_ref(*dsp, tag);

	/* we may be looking for a snapshot */
	if (err == 0 && snapname != NULL) {
		dsl_dataset_t *ds = NULL;

		if (*snapname++ != '@') {
			dsl_dataset_rele(*dsp, tag);
			err = ENOENT;
			goto out;
		}

		dprintf("looking for snapshot '%s'\n", snapname);
		err = dsl_dataset_snap_lookup(*dsp, snapname, &obj);
		if (err == 0)
			err = dsl_dataset_get_ref(dp, obj, tag, &ds);
		dsl_dataset_rele(*dsp, tag);

		ASSERT3U((err == 0), ==, (ds != NULL));

		if (ds) {
			mutex_enter(&ds->ds_lock);
			if (ds->ds_snapname[0] == 0)
				(void) strlcpy(ds->ds_snapname, snapname,
				    sizeof (ds->ds_snapname));
			mutex_exit(&ds->ds_lock);
			err = dsl_dataset_hold_ref(ds, tag);
			*dsp = err ? NULL : ds;
		}
	}
out:
	rw_exit(&dp->dp_config_rwlock);
	dsl_dir_close(dd, FTAG);
	return (err);
}

int
dsl_dataset_own(const char *name, int flags, void *owner, dsl_dataset_t **dsp)
{
	int err = dsl_dataset_hold(name, owner, dsp);
	if (err)
		return (err);
	if ((*dsp)->ds_phys->ds_num_children > 0 &&
	    !DS_MODE_IS_READONLY(flags)) {
		dsl_dataset_rele(*dsp, owner);
		return (EROFS);
	}
	if (!dsl_dataset_tryown(*dsp, DS_MODE_IS_INCONSISTENT(flags), owner)) {
		dsl_dataset_rele(*dsp, owner);
		return (EBUSY);
	}
	return (0);
}

void
dsl_dataset_name(dsl_dataset_t *ds, char *name)
{
	if (ds == NULL) {
		(void) strcpy(name, "mos");
	} else {
		dsl_dir_name(ds->ds_dir, name);
		VERIFY(0 == dsl_dataset_get_snapname(ds));
		if (ds->ds_snapname[0]) {
			(void) strcat(name, "@");
			/*
			 * We use a "recursive" mutex so that we
			 * can call dprintf_ds() with ds_lock held.
			 */
			if (!MUTEX_HELD(&ds->ds_lock)) {
				mutex_enter(&ds->ds_lock);
				(void) strcat(name, ds->ds_snapname);
				mutex_exit(&ds->ds_lock);
			} else {
				(void) strcat(name, ds->ds_snapname);
			}
		}
	}
}

static int
dsl_dataset_namelen(dsl_dataset_t *ds)
{
	int result;

	if (ds == NULL) {
		result = 3;	/* "mos" */
	} else {
		result = dsl_dir_namelen(ds->ds_dir);
		VERIFY(0 == dsl_dataset_get_snapname(ds));
		if (ds->ds_snapname[0]) {
			++result;	/* adding one for the @-sign */
			if (!MUTEX_HELD(&ds->ds_lock)) {
				mutex_enter(&ds->ds_lock);
				result += strlen(ds->ds_snapname);
				mutex_exit(&ds->ds_lock);
			} else {
				result += strlen(ds->ds_snapname);
			}
		}
	}

	return (result);
}

void
dsl_dataset_drop_ref(dsl_dataset_t *ds, void *tag)
{
	dmu_buf_rele(ds->ds_dbuf, tag);
}

void
dsl_dataset_rele(dsl_dataset_t *ds, void *tag)
{
	if (!dsl_pool_sync_context(ds->ds_dir->dd_pool)) {
		rw_exit(&ds->ds_rwlock);
	}
	dsl_dataset_drop_ref(ds, tag);
}

void
dsl_dataset_disown(dsl_dataset_t *ds, void *owner)
{
	ASSERT((ds->ds_owner == owner && ds->ds_dbuf) ||
	    (DSL_DATASET_IS_DESTROYED(ds) && ds->ds_dbuf == NULL));

	mutex_enter(&ds->ds_lock);
	ds->ds_owner = NULL;
	if (RW_WRITE_HELD(&ds->ds_rwlock)) {
		rw_exit(&ds->ds_rwlock);
		cv_broadcast(&ds->ds_exclusive_cv);
	}
	mutex_exit(&ds->ds_lock);
	if (ds->ds_dbuf)
		dsl_dataset_drop_ref(ds, owner);
	else
		dsl_dataset_evict(ds->ds_dbuf, ds);
}

boolean_t
dsl_dataset_tryown(dsl_dataset_t *ds, boolean_t inconsistentok, void *owner)
{
	boolean_t gotit = FALSE;

	mutex_enter(&ds->ds_lock);
	if (ds->ds_owner == NULL &&
	    (!DS_IS_INCONSISTENT(ds) || inconsistentok)) {
		ds->ds_owner = owner;
		if (!dsl_pool_sync_context(ds->ds_dir->dd_pool))
			rw_exit(&ds->ds_rwlock);
		gotit = TRUE;
	}
	mutex_exit(&ds->ds_lock);
	return (gotit);
}

void
dsl_dataset_make_exclusive(dsl_dataset_t *ds, void *owner)
{
	ASSERT3P(owner, ==, ds->ds_owner);
	if (!RW_WRITE_HELD(&ds->ds_rwlock))
		rw_enter(&ds->ds_rwlock, RW_WRITER);
}

uint64_t
dsl_dataset_create_sync_dd(dsl_dir_t *dd, dsl_dataset_t *origin,
    uint64_t flags, dmu_tx_t *tx)
{
	dsl_pool_t *dp = dd->dd_pool;
	dmu_buf_t *dbuf;
	dsl_dataset_phys_t *dsphys;
	uint64_t dsobj;
	objset_t *mos = dp->dp_meta_objset;

	if (origin == NULL)
		origin = dp->dp_origin_snap;

	ASSERT(origin == NULL || origin->ds_dir->dd_pool == dp);
	ASSERT(origin == NULL || origin->ds_phys->ds_num_children > 0);
	ASSERT(dmu_tx_is_syncing(tx));
	ASSERT(dd->dd_phys->dd_head_dataset_obj == 0);

	dsobj = dmu_object_alloc(mos, DMU_OT_DSL_DATASET, 0,
	    DMU_OT_DSL_DATASET, sizeof (dsl_dataset_phys_t), tx);
	VERIFY(0 == dmu_bonus_hold(mos, dsobj, FTAG, &dbuf));
	dmu_buf_will_dirty(dbuf, tx);
	dsphys = dbuf->db_data;
	bzero(dsphys, sizeof (dsl_dataset_phys_t));
	dsphys->ds_dir_obj = dd->dd_object;
	dsphys->ds_flags = flags;
	dsphys->ds_fsid_guid = unique_create();
	(void) random_get_pseudo_bytes((void*)&dsphys->ds_guid,
	    sizeof (dsphys->ds_guid));
	dsphys->ds_snapnames_zapobj =
	    zap_create_norm(mos, U8_TEXTPREP_TOUPPER, DMU_OT_DSL_DS_SNAP_MAP,
	    DMU_OT_NONE, 0, tx);
	dsphys->ds_creation_time = gethrestime_sec();
	dsphys->ds_creation_txg = tx->tx_txg == TXG_INITIAL ? 1 : tx->tx_txg;
	dsphys->ds_deadlist_obj =
	    bplist_create(mos, DSL_DEADLIST_BLOCKSIZE, tx);

	if (origin) {
		dsphys->ds_prev_snap_obj = origin->ds_object;
		dsphys->ds_prev_snap_txg =
		    origin->ds_phys->ds_creation_txg;
		dsphys->ds_used_bytes =
		    origin->ds_phys->ds_used_bytes;
		dsphys->ds_compressed_bytes =
		    origin->ds_phys->ds_compressed_bytes;
		dsphys->ds_uncompressed_bytes =
		    origin->ds_phys->ds_uncompressed_bytes;
		dsphys->ds_bp = origin->ds_phys->ds_bp;
		dsphys->ds_flags |= origin->ds_phys->ds_flags;

		dmu_buf_will_dirty(origin->ds_dbuf, tx);
		origin->ds_phys->ds_num_children++;

		if (spa_version(dp->dp_spa) >= SPA_VERSION_NEXT_CLONES) {
			if (origin->ds_phys->ds_next_clones_obj == 0) {
				origin->ds_phys->ds_next_clones_obj =
				    zap_create(mos,
				    DMU_OT_NEXT_CLONES, DMU_OT_NONE, 0, tx);
			}
			VERIFY(0 == zap_add_int(mos,
			    origin->ds_phys->ds_next_clones_obj,
			    dsobj, tx));
		}

		dmu_buf_will_dirty(dd->dd_dbuf, tx);
		dd->dd_phys->dd_origin_obj = origin->ds_object;
	}

	if (spa_version(dp->dp_spa) >= SPA_VERSION_UNIQUE_ACCURATE)
		dsphys->ds_flags |= DS_FLAG_UNIQUE_ACCURATE;

	dmu_buf_rele(dbuf, FTAG);

	dmu_buf_will_dirty(dd->dd_dbuf, tx);
	dd->dd_phys->dd_head_dataset_obj = dsobj;

	return (dsobj);
}

uint64_t
dsl_dataset_create_sync(dsl_dir_t *pdd, const char *lastname,
    dsl_dataset_t *origin, uint64_t flags, cred_t *cr, dmu_tx_t *tx)
{
	dsl_pool_t *dp = pdd->dd_pool;
	uint64_t dsobj, ddobj;
	dsl_dir_t *dd;

	ASSERT(lastname[0] != '@');

	ddobj = dsl_dir_create_sync(dp, pdd, lastname, tx);
	VERIFY(0 == dsl_dir_open_obj(dp, ddobj, lastname, FTAG, &dd));

	dsobj = dsl_dataset_create_sync_dd(dd, origin, flags, tx);

	dsl_deleg_set_create_perms(dd, tx, cr);

	dsl_dir_close(dd, FTAG);

	return (dsobj);
}

struct destroyarg {
	dsl_sync_task_group_t *dstg;
	char *snapname;
	char *failed;
};

static int
dsl_snapshot_destroy_one(char *name, void *arg)
{
	struct destroyarg *da = arg;
	dsl_dataset_t *ds;
	char *cp;
	int err;

	(void) strcat(name, "@");
	(void) strcat(name, da->snapname);
	err = dsl_dataset_own(name, DS_MODE_READONLY | DS_MODE_INCONSISTENT,
	    da->dstg, &ds);
	cp = strchr(name, '@');
	*cp = '\0';
	if (err == 0) {
		dsl_dataset_make_exclusive(ds, da->dstg);
		if (ds->ds_user_ptr) {
			ds->ds_user_evict_func(ds, ds->ds_user_ptr);
			ds->ds_user_ptr = NULL;
		}
		dsl_sync_task_create(da->dstg, dsl_dataset_destroy_check,
		    dsl_dataset_destroy_sync, ds, da->dstg, 0);
	} else if (err == ENOENT) {
		err = 0;
	} else {
		(void) strcpy(da->failed, name);
	}
	return (err);
}

/*
 * Destroy 'snapname' in all descendants of 'fsname'.
 */
#pragma weak dmu_snapshots_destroy = dsl_snapshots_destroy
int
dsl_snapshots_destroy(char *fsname, char *snapname)
{
	int err;
	struct destroyarg da;
	dsl_sync_task_t *dst;
	spa_t *spa;

	err = spa_open(fsname, &spa, FTAG);
	if (err)
		return (err);
	da.dstg = dsl_sync_task_group_create(spa_get_dsl(spa));
	da.snapname = snapname;
	da.failed = fsname;

	err = dmu_objset_find(fsname,
	    dsl_snapshot_destroy_one, &da, DS_FIND_CHILDREN);

	if (err == 0)
		err = dsl_sync_task_group_wait(da.dstg);

	for (dst = list_head(&da.dstg->dstg_tasks); dst;
	    dst = list_next(&da.dstg->dstg_tasks, dst)) {
		dsl_dataset_t *ds = dst->dst_arg1;
		/*
		 * Return the file system name that triggered the error
		 */
		if (dst->dst_err) {
			dsl_dataset_name(ds, fsname);
			*strchr(fsname, '@') = '\0';
		}
		dsl_dataset_disown(ds, da.dstg);
	}

	dsl_sync_task_group_destroy(da.dstg);
	spa_close(spa, FTAG);
	return (err);
}

/*
 * ds must be opened as OWNER.  On return (whether successful or not),
 * ds will be closed and caller can no longer dereference it.
 */
int
dsl_dataset_destroy(dsl_dataset_t *ds, void *tag)
{
	int err;
	dsl_sync_task_group_t *dstg;
	objset_t *os;
	dsl_dir_t *dd;
	uint64_t obj;

	if (dsl_dataset_is_snapshot(ds)) {
		/* Destroying a snapshot is simpler */
		dsl_dataset_make_exclusive(ds, tag);

		if (ds->ds_user_ptr) {
			ds->ds_user_evict_func(ds, ds->ds_user_ptr);
			ds->ds_user_ptr = NULL;
		}
		err = dsl_sync_task_do(ds->ds_dir->dd_pool,
		    dsl_dataset_destroy_check, dsl_dataset_destroy_sync,
		    ds, tag, 0);
		goto out;
	}

	dd = ds->ds_dir;

	/*
	 * Check for errors and mark this ds as inconsistent, in
	 * case we crash while freeing the objects.
	 */
	err = dsl_sync_task_do(dd->dd_pool, dsl_dataset_destroy_begin_check,
	    dsl_dataset_destroy_begin_sync, ds, NULL, 0);
	if (err)
		goto out;

	err = dmu_objset_open_ds(ds, DMU_OST_ANY, &os);
	if (err)
		goto out;

	/*
	 * remove the objects in open context, so that we won't
	 * have too much to do in syncing context.
	 */
	for (obj = 0; err == 0; err = dmu_object_next(os, &obj, FALSE,
	    ds->ds_phys->ds_prev_snap_txg)) {
		/*
		 * Ignore errors, if there is not enough disk space
		 * we will deal with it in dsl_dataset_destroy_sync().
		 */
		(void) dmu_free_object(os, obj);
	}

	dmu_objset_close(os);
	if (err != ESRCH)
		goto out;

	rw_enter(&dd->dd_pool->dp_config_rwlock, RW_READER);
	err = dsl_dir_open_obj(dd->dd_pool, dd->dd_object, NULL, FTAG, &dd);
	rw_exit(&dd->dd_pool->dp_config_rwlock);

	if (err)
		goto out;

	if (ds->ds_user_ptr) {
		/*
		 * We need to sync out all in-flight IO before we try
		 * to evict (the dataset evict func is trying to clear
		 * the cached entries for this dataset in the ARC).
		 */
		txg_wait_synced(dd->dd_pool, 0);
	}

	/*
	 * Blow away the dsl_dir + head dataset.
	 */
	dsl_dataset_make_exclusive(ds, tag);
	if (ds->ds_user_ptr) {
		ds->ds_user_evict_func(ds, ds->ds_user_ptr);
		ds->ds_user_ptr = NULL;
	}
	dstg = dsl_sync_task_group_create(ds->ds_dir->dd_pool);
	dsl_sync_task_create(dstg, dsl_dataset_destroy_check,
	    dsl_dataset_destroy_sync, ds, tag, 0);
	dsl_sync_task_create(dstg, dsl_dir_destroy_check,
	    dsl_dir_destroy_sync, dd, FTAG, 0);
	err = dsl_sync_task_group_wait(dstg);
	dsl_sync_task_group_destroy(dstg);
	/* if it is successful, dsl_dir_destroy_sync will close the dd */
	if (err)
		dsl_dir_close(dd, FTAG);
out:
	dsl_dataset_disown(ds, tag);
	return (err);
}

int
dsl_dataset_rollback(dsl_dataset_t *ds, dmu_objset_type_t ost)
{
	ASSERT(ds->ds_owner);

	return (dsl_sync_task_do(ds->ds_dir->dd_pool,
	    dsl_dataset_rollback_check, dsl_dataset_rollback_sync,
	    ds, &ost, 0));
}

void *
dsl_dataset_set_user_ptr(dsl_dataset_t *ds,
    void *p, dsl_dataset_evict_func_t func)
{
	void *old;

	mutex_enter(&ds->ds_lock);
	old = ds->ds_user_ptr;
	if (old == NULL) {
		ds->ds_user_ptr = p;
		ds->ds_user_evict_func = func;
	}
	mutex_exit(&ds->ds_lock);
	return (old);
}

void *
dsl_dataset_get_user_ptr(dsl_dataset_t *ds)
{
	return (ds->ds_user_ptr);
}


blkptr_t *
dsl_dataset_get_blkptr(dsl_dataset_t *ds)
{
	return (&ds->ds_phys->ds_bp);
}

void
dsl_dataset_set_blkptr(dsl_dataset_t *ds, blkptr_t *bp, dmu_tx_t *tx)
{
	ASSERT(dmu_tx_is_syncing(tx));
	/* If it's the meta-objset, set dp_meta_rootbp */
	if (ds == NULL) {
		tx->tx_pool->dp_meta_rootbp = *bp;
	} else {
		dmu_buf_will_dirty(ds->ds_dbuf, tx);
		ds->ds_phys->ds_bp = *bp;
	}
}

spa_t *
dsl_dataset_get_spa(dsl_dataset_t *ds)
{
	return (ds->ds_dir->dd_pool->dp_spa);
}

void
dsl_dataset_dirty(dsl_dataset_t *ds, dmu_tx_t *tx)
{
	dsl_pool_t *dp;

	if (ds == NULL) /* this is the meta-objset */
		return;

	ASSERT(ds->ds_user_ptr != NULL);

	if (ds->ds_phys->ds_next_snap_obj != 0)
		panic("dirtying snapshot!");

	dp = ds->ds_dir->dd_pool;

	if (txg_list_add(&dp->dp_dirty_datasets, ds, tx->tx_txg) == 0) {
		/* up the hold count until we can be written out */
		dmu_buf_add_ref(ds->ds_dbuf, ds);
	}
}

/*
 * The unique space in the head dataset can be calculated by subtracting
 * the space used in the most recent snapshot, that is still being used
 * in this file system, from the space currently in use.  To figure out
 * the space in the most recent snapshot still in use, we need to take
 * the total space used in the snapshot and subtract out the space that
 * has been freed up since the snapshot was taken.
 */
static void
dsl_dataset_recalc_head_uniq(dsl_dataset_t *ds)
{
	uint64_t mrs_used;
	uint64_t dlused, dlcomp, dluncomp;

	ASSERT(ds->ds_object == ds->ds_dir->dd_phys->dd_head_dataset_obj);

	if (ds->ds_phys->ds_prev_snap_obj != 0)
		mrs_used = ds->ds_prev->ds_phys->ds_used_bytes;
	else
		mrs_used = 0;

	VERIFY(0 == bplist_space(&ds->ds_deadlist, &dlused, &dlcomp,
	    &dluncomp));

	ASSERT3U(dlused, <=, mrs_used);
	ds->ds_phys->ds_unique_bytes =
	    ds->ds_phys->ds_used_bytes - (mrs_used - dlused);

	if (!DS_UNIQUE_IS_ACCURATE(ds) &&
	    spa_version(ds->ds_dir->dd_pool->dp_spa) >=
	    SPA_VERSION_UNIQUE_ACCURATE)
		ds->ds_phys->ds_flags |= DS_FLAG_UNIQUE_ACCURATE;
}

static uint64_t
dsl_dataset_unique(dsl_dataset_t *ds)
{
	if (!DS_UNIQUE_IS_ACCURATE(ds) && !dsl_dataset_is_snapshot(ds))
		dsl_dataset_recalc_head_uniq(ds);

	return (ds->ds_phys->ds_unique_bytes);
}

struct killarg {
	int64_t *usedp;
	int64_t *compressedp;
	int64_t *uncompressedp;
	zio_t *zio;
	dmu_tx_t *tx;
};

static int
kill_blkptr(traverse_blk_cache_t *bc, spa_t *spa, void *arg)
{
	struct killarg *ka = arg;
	blkptr_t *bp = &bc->bc_blkptr;

	ASSERT3U(bc->bc_errno, ==, 0);

	/*
	 * Since this callback is not called concurrently, no lock is
	 * needed on the accounting values.
	 */
	*ka->usedp += bp_get_dasize(spa, bp);
	*ka->compressedp += BP_GET_PSIZE(bp);
	*ka->uncompressedp += BP_GET_UCSIZE(bp);
	/* XXX check for EIO? */
	(void) dsl_free(ka->zio, spa_get_dsl(spa), ka->tx->tx_txg,
	    bp, NULL, NULL, ARC_NOWAIT);
	return (0);
}

/* ARGSUSED */
static int
dsl_dataset_rollback_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	dmu_objset_type_t *ost = arg2;

	/*
	 * We can only roll back to emptyness if it is a ZPL objset.
	 */
	if (*ost != DMU_OST_ZFS && ds->ds_phys->ds_prev_snap_txg == 0)
		return (EINVAL);

	/*
	 * This must not be a snapshot.
	 */
	if (ds->ds_phys->ds_next_snap_obj != 0)
		return (EINVAL);

	/*
	 * If we made changes this txg, traverse_dsl_dataset won't find
	 * them.  Try again.
	 */
	if (ds->ds_phys->ds_bp.blk_birth >= tx->tx_txg)
		return (EAGAIN);

	return (0);
}

/* ARGSUSED */
static void
dsl_dataset_rollback_sync(void *arg1, void *arg2, cred_t *cr, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	dmu_objset_type_t *ost = arg2;
	objset_t *mos = ds->ds_dir->dd_pool->dp_meta_objset;

	dmu_buf_will_dirty(ds->ds_dbuf, tx);

	/*
	 * Before the roll back destroy the zil.
	 */
	if (ds->ds_user_ptr != NULL) {
		zil_rollback_destroy(
		    ((objset_impl_t *)ds->ds_user_ptr)->os_zil, tx);

		/*
		 * We need to make sure that the objset_impl_t is reopened after
		 * we do the rollback, otherwise it will have the wrong
		 * objset_phys_t.  Normally this would happen when this
		 * dataset-open is closed, thus causing the
		 * dataset to be immediately evicted.  But when doing "zfs recv
		 * -F", we reopen the objset before that, so that there is no
		 * window where the dataset is closed and inconsistent.
		 */
		ds->ds_user_evict_func(ds, ds->ds_user_ptr);
		ds->ds_user_ptr = NULL;
	}

	/* Zero out the deadlist. */
	bplist_close(&ds->ds_deadlist);
	bplist_destroy(mos, ds->ds_phys->ds_deadlist_obj, tx);
	ds->ds_phys->ds_deadlist_obj =
	    bplist_create(mos, DSL_DEADLIST_BLOCKSIZE, tx);
	VERIFY(0 == bplist_open(&ds->ds_deadlist, mos,
	    ds->ds_phys->ds_deadlist_obj));

	{
		/* Free blkptrs that we gave birth to */
		zio_t *zio;
		int64_t used = 0, compressed = 0, uncompressed = 0;
		struct killarg ka;
		int64_t delta;

		zio = zio_root(tx->tx_pool->dp_spa, NULL, NULL,
		    ZIO_FLAG_MUSTSUCCEED);
		ka.usedp = &used;
		ka.compressedp = &compressed;
		ka.uncompressedp = &uncompressed;
		ka.zio = zio;
		ka.tx = tx;
		(void) traverse_dsl_dataset(ds, ds->ds_phys->ds_prev_snap_txg,
		    ADVANCE_POST, kill_blkptr, &ka);
		(void) zio_wait(zio);

		/* only deduct space beyond any refreservation */
		delta = parent_delta(ds, -used);
		dsl_dir_diduse_space(ds->ds_dir,
		    delta, -compressed, -uncompressed, tx);
	}

	if (ds->ds_prev && ds->ds_prev != ds->ds_dir->dd_pool->dp_origin_snap) {
		/* Change our contents to that of the prev snapshot */
		ASSERT3U(ds->ds_prev->ds_object, ==,
		    ds->ds_phys->ds_prev_snap_obj);
		ds->ds_phys->ds_bp = ds->ds_prev->ds_phys->ds_bp;
		ds->ds_phys->ds_used_bytes =
		    ds->ds_prev->ds_phys->ds_used_bytes;
		ds->ds_phys->ds_compressed_bytes =
		    ds->ds_prev->ds_phys->ds_compressed_bytes;
		ds->ds_phys->ds_uncompressed_bytes =
		    ds->ds_prev->ds_phys->ds_uncompressed_bytes;
		ds->ds_phys->ds_flags = ds->ds_prev->ds_phys->ds_flags;
		ds->ds_phys->ds_unique_bytes = 0;

		if (ds->ds_prev->ds_phys->ds_next_snap_obj == ds->ds_object) {
			dmu_buf_will_dirty(ds->ds_prev->ds_dbuf, tx);
			ds->ds_prev->ds_phys->ds_unique_bytes = 0;
		}
	} else {
		objset_impl_t *osi;

		/* Zero out our contents, recreate objset */
		bzero(&ds->ds_phys->ds_bp, sizeof (blkptr_t));
		ds->ds_phys->ds_used_bytes = 0;
		ds->ds_phys->ds_compressed_bytes = 0;
		ds->ds_phys->ds_uncompressed_bytes = 0;
		ds->ds_phys->ds_flags = 0;
		ds->ds_phys->ds_unique_bytes = 0;
		osi = dmu_objset_create_impl(ds->ds_dir->dd_pool->dp_spa, ds,
		    &ds->ds_phys->ds_bp, *ost, tx);
#ifdef _KERNEL
		zfs_create_fs(&osi->os, kcred, NULL, tx);
#endif
	}

	spa_history_internal_log(LOG_DS_ROLLBACK, ds->ds_dir->dd_pool->dp_spa,
	    tx, cr, "dataset = %llu", ds->ds_object);
}

/* ARGSUSED */
static int
dsl_dataset_destroy_begin_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	objset_t *mos = ds->ds_dir->dd_pool->dp_meta_objset;
	uint64_t count;
	int err;

	/*
	 * Can't delete a head dataset if there are snapshots of it.
	 * (Except if the only snapshots are from the branch we cloned
	 * from.)
	 */
	if (ds->ds_prev != NULL &&
	    ds->ds_prev->ds_phys->ds_next_snap_obj == ds->ds_object)
		return (EINVAL);

	/*
	 * This is really a dsl_dir thing, but check it here so that
	 * we'll be less likely to leave this dataset inconsistent &
	 * nearly destroyed.
	 */
	err = zap_count(mos, ds->ds_dir->dd_phys->dd_child_dir_zapobj, &count);
	if (err)
		return (err);
	if (count != 0)
		return (EEXIST);

	return (0);
}

/* ARGSUSED */
static void
dsl_dataset_destroy_begin_sync(void *arg1, void *arg2, cred_t *cr, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	dsl_pool_t *dp = ds->ds_dir->dd_pool;

	/* Mark it as inconsistent on-disk, in case we crash */
	dmu_buf_will_dirty(ds->ds_dbuf, tx);
	ds->ds_phys->ds_flags |= DS_FLAG_INCONSISTENT;

	spa_history_internal_log(LOG_DS_DESTROY_BEGIN, dp->dp_spa, tx,
	    cr, "dataset = %llu", ds->ds_object);
}

/* ARGSUSED */
int
dsl_dataset_destroy_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;

	/* we have an owner hold, so noone else can destroy us */
	ASSERT(!DSL_DATASET_IS_DESTROYED(ds));

	/* Can't delete a branch point. */
	if (ds->ds_phys->ds_num_children > 1)
		return (EEXIST);

	/*
	 * Can't delete a head dataset if there are snapshots of it.
	 * (Except if the only snapshots are from the branch we cloned
	 * from.)
	 */
	if (ds->ds_prev != NULL &&
	    ds->ds_prev->ds_phys->ds_next_snap_obj == ds->ds_object)
		return (EINVAL);

	/*
	 * If we made changes this txg, traverse_dsl_dataset won't find
	 * them.  Try again.
	 */
	if (ds->ds_phys->ds_bp.blk_birth >= tx->tx_txg)
		return (EAGAIN);

	/* XXX we should do some i/o error checking... */
	return (0);
}

struct refsarg {
	kmutex_t lock;
	boolean_t gone;
	kcondvar_t cv;
};

/* ARGSUSED */
static void
dsl_dataset_refs_gone(dmu_buf_t *db, void *argv)
{
	struct refsarg *arg = argv;

	mutex_enter(&arg->lock);
	arg->gone = TRUE;
	cv_signal(&arg->cv);
	mutex_exit(&arg->lock);
}

static void
dsl_dataset_drain_refs(dsl_dataset_t *ds, void *tag)
{
	struct refsarg arg;

	mutex_init(&arg.lock, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&arg.cv, NULL, CV_DEFAULT, NULL);
	arg.gone = FALSE;
	(void) dmu_buf_update_user(ds->ds_dbuf, ds, &arg, &ds->ds_phys,
	    dsl_dataset_refs_gone);
	dmu_buf_rele(ds->ds_dbuf, tag);
	mutex_enter(&arg.lock);
	while (!arg.gone)
		cv_wait(&arg.cv, &arg.lock);
	ASSERT(arg.gone);
	mutex_exit(&arg.lock);
	ds->ds_dbuf = NULL;
	ds->ds_phys = NULL;
	mutex_destroy(&arg.lock);
	cv_destroy(&arg.cv);
}

void
dsl_dataset_destroy_sync(void *arg1, void *tag, cred_t *cr, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	int64_t used = 0, compressed = 0, uncompressed = 0;
	zio_t *zio;
	int err;
	int after_branch_point = FALSE;
	dsl_pool_t *dp = ds->ds_dir->dd_pool;
	objset_t *mos = dp->dp_meta_objset;
	dsl_dataset_t *ds_prev = NULL;
	uint64_t obj;

	ASSERT(ds->ds_owner);
	ASSERT3U(ds->ds_phys->ds_num_children, <=, 1);
	ASSERT(ds->ds_prev == NULL ||
	    ds->ds_prev->ds_phys->ds_next_snap_obj != ds->ds_object);
	ASSERT3U(ds->ds_phys->ds_bp.blk_birth, <=, tx->tx_txg);

	/* signal any waiters that this dataset is going away */
	mutex_enter(&ds->ds_lock);
	ds->ds_owner = dsl_reaper;
	cv_broadcast(&ds->ds_exclusive_cv);
	mutex_exit(&ds->ds_lock);

	/* Remove our reservation */
	if (ds->ds_reserved != 0) {
		uint64_t val = 0;
		dsl_dataset_set_reservation_sync(ds, &val, cr, tx);
		ASSERT3U(ds->ds_reserved, ==, 0);
	}

	ASSERT(RW_WRITE_HELD(&dp->dp_config_rwlock));

	dsl_pool_ds_destroyed(ds, tx);

	obj = ds->ds_object;

	if (ds->ds_phys->ds_prev_snap_obj != 0) {
		if (ds->ds_prev) {
			ds_prev = ds->ds_prev;
		} else {
			VERIFY(0 == dsl_dataset_hold_obj(dp,
			    ds->ds_phys->ds_prev_snap_obj, FTAG, &ds_prev));
		}
		after_branch_point =
		    (ds_prev->ds_phys->ds_next_snap_obj != obj);

		dmu_buf_will_dirty(ds_prev->ds_dbuf, tx);
		if (after_branch_point &&
		    ds_prev->ds_phys->ds_next_clones_obj != 0) {
			VERIFY(0 == zap_remove_int(mos,
			    ds_prev->ds_phys->ds_next_clones_obj, obj, tx));
			if (ds->ds_phys->ds_next_snap_obj != 0) {
				VERIFY(0 == zap_add_int(mos,
				    ds_prev->ds_phys->ds_next_clones_obj,
				    ds->ds_phys->ds_next_snap_obj, tx));
			}
		}
		if (after_branch_point &&
		    ds->ds_phys->ds_next_snap_obj == 0) {
			/* This clone is toast. */
			ASSERT(ds_prev->ds_phys->ds_num_children > 1);
			ds_prev->ds_phys->ds_num_children--;
		} else if (!after_branch_point) {
			ds_prev->ds_phys->ds_next_snap_obj =
			    ds->ds_phys->ds_next_snap_obj;
		}
	}

	zio = zio_root(dp->dp_spa, NULL, NULL, ZIO_FLAG_MUSTSUCCEED);

	if (ds->ds_phys->ds_next_snap_obj != 0) {
		blkptr_t bp;
		dsl_dataset_t *ds_next;
		uint64_t itor = 0;
		uint64_t old_unique;

		VERIFY(0 == dsl_dataset_hold_obj(dp,
		    ds->ds_phys->ds_next_snap_obj, FTAG, &ds_next));
		ASSERT3U(ds_next->ds_phys->ds_prev_snap_obj, ==, obj);

		old_unique = dsl_dataset_unique(ds_next);

		dmu_buf_will_dirty(ds_next->ds_dbuf, tx);
		ds_next->ds_phys->ds_prev_snap_obj =
		    ds->ds_phys->ds_prev_snap_obj;
		ds_next->ds_phys->ds_prev_snap_txg =
		    ds->ds_phys->ds_prev_snap_txg;
		ASSERT3U(ds->ds_phys->ds_prev_snap_txg, ==,
		    ds_prev ? ds_prev->ds_phys->ds_creation_txg : 0);

		/*
		 * Transfer to our deadlist (which will become next's
		 * new deadlist) any entries from next's current
		 * deadlist which were born before prev, and free the
		 * other entries.
		 *
		 * XXX we're doing this long task with the config lock held
		 */
		while (bplist_iterate(&ds_next->ds_deadlist, &itor, &bp) == 0) {
			if (bp.blk_birth <= ds->ds_phys->ds_prev_snap_txg) {
				VERIFY(0 == bplist_enqueue(&ds->ds_deadlist,
				    &bp, tx));
				if (ds_prev && !after_branch_point &&
				    bp.blk_birth >
				    ds_prev->ds_phys->ds_prev_snap_txg) {
					ds_prev->ds_phys->ds_unique_bytes +=
					    bp_get_dasize(dp->dp_spa, &bp);
				}
			} else {
				used += bp_get_dasize(dp->dp_spa, &bp);
				compressed += BP_GET_PSIZE(&bp);
				uncompressed += BP_GET_UCSIZE(&bp);
				/* XXX check return value? */
				(void) dsl_free(zio, dp, tx->tx_txg,
				    &bp, NULL, NULL, ARC_NOWAIT);
			}
		}

		/* free next's deadlist */
		bplist_close(&ds_next->ds_deadlist);
		bplist_destroy(mos, ds_next->ds_phys->ds_deadlist_obj, tx);

		/* set next's deadlist to our deadlist */
		bplist_close(&ds->ds_deadlist);
		ds_next->ds_phys->ds_deadlist_obj =
		    ds->ds_phys->ds_deadlist_obj;
		VERIFY(0 == bplist_open(&ds_next->ds_deadlist, mos,
		    ds_next->ds_phys->ds_deadlist_obj));
		ds->ds_phys->ds_deadlist_obj = 0;

		if (ds_next->ds_phys->ds_next_snap_obj != 0) {
			/*
			 * Update next's unique to include blocks which
			 * were previously shared by only this snapshot
			 * and it.  Those blocks will be born after the
			 * prev snap and before this snap, and will have
			 * died after the next snap and before the one
			 * after that (ie. be on the snap after next's
			 * deadlist).
			 *
			 * XXX we're doing this long task with the
			 * config lock held
			 */
			dsl_dataset_t *ds_after_next;

			VERIFY(0 == dsl_dataset_hold_obj(dp,
			    ds_next->ds_phys->ds_next_snap_obj,
			    FTAG, &ds_after_next));
			itor = 0;
			while (bplist_iterate(&ds_after_next->ds_deadlist,
			    &itor, &bp) == 0) {
				if (bp.blk_birth >
				    ds->ds_phys->ds_prev_snap_txg &&
				    bp.blk_birth <=
				    ds->ds_phys->ds_creation_txg) {
					ds_next->ds_phys->ds_unique_bytes +=
					    bp_get_dasize(dp->dp_spa, &bp);
				}
			}

			dsl_dataset_rele(ds_after_next, FTAG);
			ASSERT3P(ds_next->ds_prev, ==, NULL);
		} else {
			ASSERT3P(ds_next->ds_prev, ==, ds);
			dsl_dataset_drop_ref(ds_next->ds_prev, ds_next);
			ds_next->ds_prev = NULL;
			if (ds_prev) {
				VERIFY(0 == dsl_dataset_get_ref(dp,
				    ds->ds_phys->ds_prev_snap_obj,
				    ds_next, &ds_next->ds_prev));
			}

			dsl_dataset_recalc_head_uniq(ds_next);

			/*
			 * Reduce the amount of our unconsmed refreservation
			 * being charged to our parent by the amount of
			 * new unique data we have gained.
			 */
			if (old_unique < ds_next->ds_reserved) {
				int64_t mrsdelta;
				uint64_t new_unique =
				    ds_next->ds_phys->ds_unique_bytes;

				ASSERT(old_unique <= new_unique);
				mrsdelta = MIN(new_unique - old_unique,
				    ds_next->ds_reserved - old_unique);
				dsl_dir_diduse_space(ds->ds_dir, -mrsdelta,
				    0, 0, tx);
			}
		}
		dsl_dataset_rele(ds_next, FTAG);

		/*
		 * NB: unique_bytes might not be accurate for the head objset.
		 * Before SPA_VERSION 9, we didn't update its value when we
		 * deleted the most recent snapshot.
		 */
		ASSERT3U(used, ==, ds->ds_phys->ds_unique_bytes);
	} else {
		/*
		 * There's no next snapshot, so this is a head dataset.
		 * Destroy the deadlist.  Unless it's a clone, the
		 * deadlist should be empty.  (If it's a clone, it's
		 * safe to ignore the deadlist contents.)
		 */
		struct killarg ka;

		ASSERT(after_branch_point || bplist_empty(&ds->ds_deadlist));
		bplist_close(&ds->ds_deadlist);
		bplist_destroy(mos, ds->ds_phys->ds_deadlist_obj, tx);
		ds->ds_phys->ds_deadlist_obj = 0;

		/*
		 * Free everything that we point to (that's born after
		 * the previous snapshot, if we are a clone)
		 *
		 * XXX we're doing this long task with the config lock held
		 */
		ka.usedp = &used;
		ka.compressedp = &compressed;
		ka.uncompressedp = &uncompressed;
		ka.zio = zio;
		ka.tx = tx;
		err = traverse_dsl_dataset(ds, ds->ds_phys->ds_prev_snap_txg,
		    ADVANCE_POST, kill_blkptr, &ka);
		ASSERT3U(err, ==, 0);
		ASSERT(spa_version(dp->dp_spa) <
		    SPA_VERSION_UNIQUE_ACCURATE ||
		    used == ds->ds_phys->ds_unique_bytes);
	}

	err = zio_wait(zio);
	ASSERT3U(err, ==, 0);

	dsl_dir_diduse_space(ds->ds_dir, -used, -compressed, -uncompressed, tx);

	if (ds->ds_dir->dd_phys->dd_head_dataset_obj == ds->ds_object) {
		/* Erase the link in the dir */
		dmu_buf_will_dirty(ds->ds_dir->dd_dbuf, tx);
		ds->ds_dir->dd_phys->dd_head_dataset_obj = 0;
		ASSERT(ds->ds_phys->ds_snapnames_zapobj != 0);
		err = zap_destroy(mos, ds->ds_phys->ds_snapnames_zapobj, tx);
		ASSERT(err == 0);
	} else {
		/* remove from snapshot namespace */
		dsl_dataset_t *ds_head;
		ASSERT(ds->ds_phys->ds_snapnames_zapobj == 0);
		VERIFY(0 == dsl_dataset_hold_obj(dp,
		    ds->ds_dir->dd_phys->dd_head_dataset_obj, FTAG, &ds_head));
		VERIFY(0 == dsl_dataset_get_snapname(ds));
#ifdef ZFS_DEBUG
		{
			uint64_t val;

			err = dsl_dataset_snap_lookup(ds_head,
			    ds->ds_snapname, &val);
			ASSERT3U(err, ==, 0);
			ASSERT3U(val, ==, obj);
		}
#endif
		err = dsl_dataset_snap_remove(ds_head, ds->ds_snapname, tx);
		ASSERT(err == 0);
		dsl_dataset_rele(ds_head, FTAG);
	}

	if (ds_prev && ds->ds_prev != ds_prev)
		dsl_dataset_rele(ds_prev, FTAG);

	spa_prop_clear_bootfs(dp->dp_spa, ds->ds_object, tx);
	spa_history_internal_log(LOG_DS_DESTROY, dp->dp_spa, tx,
	    cr, "dataset = %llu", ds->ds_object);

	if (ds->ds_phys->ds_next_clones_obj != 0) {
		uint64_t count;
		ASSERT(0 == zap_count(mos,
		    ds->ds_phys->ds_next_clones_obj, &count) && count == 0);
		VERIFY(0 == dmu_object_free(mos,
		    ds->ds_phys->ds_next_clones_obj, tx));
	}
	if (ds->ds_phys->ds_props_obj != 0) {
		VERIFY(0 == zap_destroy(mos,
		    ds->ds_phys->ds_props_obj, tx));
	}
	dsl_dir_close(ds->ds_dir, ds);
	ds->ds_dir = NULL;
	dsl_dataset_drain_refs(ds, tag);
	VERIFY(0 == dmu_object_free(mos, obj, tx));
}

static int
dsl_dataset_snapshot_reserve_space(dsl_dataset_t *ds, dmu_tx_t *tx)
{
	uint64_t asize;

	if (!dmu_tx_is_syncing(tx))
		return (0);

	/*
	 * If there's an fs-only reservation, any blocks that might become
	 * owned by the snapshot dataset must be accommodated by space
	 * outside of the reservation.
	 */
	asize = MIN(dsl_dataset_unique(ds), ds->ds_reserved);
	if (asize > dsl_dir_space_available(ds->ds_dir, NULL, 0, FALSE))
		return (ENOSPC);

	/*
	 * Propogate any reserved space for this snapshot to other
	 * snapshot checks in this sync group.
	 */
	if (asize > 0)
		dsl_dir_willuse_space(ds->ds_dir, asize, tx);

	return (0);
}

/* ARGSUSED */
int
dsl_dataset_snapshot_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	const char *snapname = arg2;
	int err;
	uint64_t value;

	/*
	 * We don't allow multiple snapshots of the same txg.  If there
	 * is already one, try again.
	 */
	if (ds->ds_phys->ds_prev_snap_txg >= tx->tx_txg)
		return (EAGAIN);

	/*
	 * Check for conflicting name snapshot name.
	 */
	err = dsl_dataset_snap_lookup(ds, snapname, &value);
	if (err == 0)
		return (EEXIST);
	if (err != ENOENT)
		return (err);

	/*
	 * Check that the dataset's name is not too long.  Name consists
	 * of the dataset's length + 1 for the @-sign + snapshot name's length
	 */
	if (dsl_dataset_namelen(ds) + 1 + strlen(snapname) >= MAXNAMELEN)
		return (ENAMETOOLONG);

	err = dsl_dataset_snapshot_reserve_space(ds, tx);
	if (err)
		return (err);

	ds->ds_trysnap_txg = tx->tx_txg;
	return (0);
}

void
dsl_dataset_snapshot_sync(void *arg1, void *arg2, cred_t *cr, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	const char *snapname = arg2;
	dsl_pool_t *dp = ds->ds_dir->dd_pool;
	dmu_buf_t *dbuf;
	dsl_dataset_phys_t *dsphys;
	uint64_t dsobj, crtxg;
	objset_t *mos = dp->dp_meta_objset;
	int err;

	ASSERT(RW_WRITE_HELD(&dp->dp_config_rwlock));

	/*
	 * The origin's ds_creation_txg has to be < TXG_INITIAL
	 */
	if (strcmp(snapname, ORIGIN_DIR_NAME) == 0)
		crtxg = 1;
	else
		crtxg = tx->tx_txg;

	dsobj = dmu_object_alloc(mos, DMU_OT_DSL_DATASET, 0,
	    DMU_OT_DSL_DATASET, sizeof (dsl_dataset_phys_t), tx);
	VERIFY(0 == dmu_bonus_hold(mos, dsobj, FTAG, &dbuf));
	dmu_buf_will_dirty(dbuf, tx);
	dsphys = dbuf->db_data;
	bzero(dsphys, sizeof (dsl_dataset_phys_t));
	dsphys->ds_dir_obj = ds->ds_dir->dd_object;
	dsphys->ds_fsid_guid = unique_create();
	(void) random_get_pseudo_bytes((void*)&dsphys->ds_guid,
	    sizeof (dsphys->ds_guid));
	dsphys->ds_prev_snap_obj = ds->ds_phys->ds_prev_snap_obj;
	dsphys->ds_prev_snap_txg = ds->ds_phys->ds_prev_snap_txg;
	dsphys->ds_next_snap_obj = ds->ds_object;
	dsphys->ds_num_children = 1;
	dsphys->ds_creation_time = gethrestime_sec();
	dsphys->ds_creation_txg = crtxg;
	dsphys->ds_deadlist_obj = ds->ds_phys->ds_deadlist_obj;
	dsphys->ds_used_bytes = ds->ds_phys->ds_used_bytes;
	dsphys->ds_compressed_bytes = ds->ds_phys->ds_compressed_bytes;
	dsphys->ds_uncompressed_bytes = ds->ds_phys->ds_uncompressed_bytes;
	dsphys->ds_flags = ds->ds_phys->ds_flags;
	dsphys->ds_bp = ds->ds_phys->ds_bp;
	dmu_buf_rele(dbuf, FTAG);

	ASSERT3U(ds->ds_prev != 0, ==, ds->ds_phys->ds_prev_snap_obj != 0);
	if (ds->ds_prev) {
		uint64_t next_clones_obj =
		    ds->ds_prev->ds_phys->ds_next_clones_obj;
		ASSERT(ds->ds_prev->ds_phys->ds_next_snap_obj ==
		    ds->ds_object ||
		    ds->ds_prev->ds_phys->ds_num_children > 1);
		if (ds->ds_prev->ds_phys->ds_next_snap_obj == ds->ds_object) {
			dmu_buf_will_dirty(ds->ds_prev->ds_dbuf, tx);
			ASSERT3U(ds->ds_phys->ds_prev_snap_txg, ==,
			    ds->ds_prev->ds_phys->ds_creation_txg);
			ds->ds_prev->ds_phys->ds_next_snap_obj = dsobj;
		} else if (next_clones_obj != 0) {
			VERIFY3U(0, ==, zap_remove_int(mos,
			    next_clones_obj, dsphys->ds_next_snap_obj, tx));
			VERIFY3U(0, ==, zap_add_int(mos,
			    next_clones_obj, dsobj, tx));
		}
	}

	/*
	 * If we have a reference-reservation on this dataset, we will
	 * need to increase the amount of refreservation being charged
	 * since our unique space is going to zero.
	 */
	if (ds->ds_reserved) {
		int64_t add = MIN(dsl_dataset_unique(ds), ds->ds_reserved);
		dsl_dir_diduse_space(ds->ds_dir, add, 0, 0, tx);
	}

	bplist_close(&ds->ds_deadlist);
	dmu_buf_will_dirty(ds->ds_dbuf, tx);
	ASSERT3U(ds->ds_phys->ds_prev_snap_txg, <, tx->tx_txg);
	ds->ds_phys->ds_prev_snap_obj = dsobj;
	ds->ds_phys->ds_prev_snap_txg = crtxg;
	ds->ds_phys->ds_unique_bytes = 0;
	if (spa_version(dp->dp_spa) >= SPA_VERSION_UNIQUE_ACCURATE)
		ds->ds_phys->ds_flags |= DS_FLAG_UNIQUE_ACCURATE;
	ds->ds_phys->ds_deadlist_obj =
	    bplist_create(mos, DSL_DEADLIST_BLOCKSIZE, tx);
	VERIFY(0 == bplist_open(&ds->ds_deadlist, mos,
	    ds->ds_phys->ds_deadlist_obj));

	dprintf("snap '%s' -> obj %llu\n", snapname, dsobj);
	err = zap_add(mos, ds->ds_phys->ds_snapnames_zapobj,
	    snapname, 8, 1, &dsobj, tx);
	ASSERT(err == 0);

	if (ds->ds_prev)
		dsl_dataset_drop_ref(ds->ds_prev, ds);
	VERIFY(0 == dsl_dataset_get_ref(dp,
	    ds->ds_phys->ds_prev_snap_obj, ds, &ds->ds_prev));

	dsl_pool_ds_snapshotted(ds, tx);

	spa_history_internal_log(LOG_DS_SNAPSHOT, dp->dp_spa, tx, cr,
	    "dataset = %llu", dsobj);
}

void
dsl_dataset_sync(dsl_dataset_t *ds, zio_t *zio, dmu_tx_t *tx)
{
	ASSERT(dmu_tx_is_syncing(tx));
	ASSERT(ds->ds_user_ptr != NULL);
	ASSERT(ds->ds_phys->ds_next_snap_obj == 0);

	/*
	 * in case we had to change ds_fsid_guid when we opened it,
	 * sync it out now.
	 */
	dmu_buf_will_dirty(ds->ds_dbuf, tx);
	ds->ds_phys->ds_fsid_guid = ds->ds_fsid_guid;

	dsl_dir_dirty(ds->ds_dir, tx);
	dmu_objset_sync(ds->ds_user_ptr, zio, tx);
}

void
dsl_dataset_stats(dsl_dataset_t *ds, nvlist_t *nv)
{
	uint64_t refd, avail, uobjs, aobjs;

	dsl_dir_stats(ds->ds_dir, nv);

	dsl_dataset_space(ds, &refd, &avail, &uobjs, &aobjs);
	dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_AVAILABLE, avail);
	dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_REFERENCED, refd);

	dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_CREATION,
	    ds->ds_phys->ds_creation_time);
	dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_CREATETXG,
	    ds->ds_phys->ds_creation_txg);
	dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_REFQUOTA,
	    ds->ds_quota);
	dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_REFRESERVATION,
	    ds->ds_reserved);
	dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_GUID,
	    ds->ds_phys->ds_guid);

	if (ds->ds_phys->ds_next_snap_obj) {
		/*
		 * This is a snapshot; override the dd's space used with
		 * our unique space and compression ratio.
		 */
		dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_USED,
		    ds->ds_phys->ds_unique_bytes);
		dsl_prop_nvlist_add_uint64(nv, ZFS_PROP_COMPRESSRATIO,
		    ds->ds_phys->ds_compressed_bytes == 0 ? 100 :
		    (ds->ds_phys->ds_uncompressed_bytes * 100 /
		    ds->ds_phys->ds_compressed_bytes));
	}
}

void
dsl_dataset_fast_stat(dsl_dataset_t *ds, dmu_objset_stats_t *stat)
{
	stat->dds_creation_txg = ds->ds_phys->ds_creation_txg;
	stat->dds_inconsistent = ds->ds_phys->ds_flags & DS_FLAG_INCONSISTENT;
	stat->dds_guid = ds->ds_phys->ds_guid;
	if (ds->ds_phys->ds_next_snap_obj) {
		stat->dds_is_snapshot = B_TRUE;
		stat->dds_num_clones = ds->ds_phys->ds_num_children - 1;
	}

	/* clone origin is really a dsl_dir thing... */
	rw_enter(&ds->ds_dir->dd_pool->dp_config_rwlock, RW_READER);
	if (dsl_dir_is_clone(ds->ds_dir)) {
		dsl_dataset_t *ods;

		VERIFY(0 == dsl_dataset_get_ref(ds->ds_dir->dd_pool,
		    ds->ds_dir->dd_phys->dd_origin_obj, FTAG, &ods));
		dsl_dataset_name(ods, stat->dds_origin);
		dsl_dataset_drop_ref(ods, FTAG);
	}
	rw_exit(&ds->ds_dir->dd_pool->dp_config_rwlock);
}

uint64_t
dsl_dataset_fsid_guid(dsl_dataset_t *ds)
{
	return (ds->ds_fsid_guid);
}

void
dsl_dataset_space(dsl_dataset_t *ds,
    uint64_t *refdbytesp, uint64_t *availbytesp,
    uint64_t *usedobjsp, uint64_t *availobjsp)
{
	*refdbytesp = ds->ds_phys->ds_used_bytes;
	*availbytesp = dsl_dir_space_available(ds->ds_dir, NULL, 0, TRUE);
	if (ds->ds_reserved > ds->ds_phys->ds_unique_bytes)
		*availbytesp += ds->ds_reserved - ds->ds_phys->ds_unique_bytes;
	if (ds->ds_quota != 0) {
		/*
		 * Adjust available bytes according to refquota
		 */
		if (*refdbytesp < ds->ds_quota)
			*availbytesp = MIN(*availbytesp,
			    ds->ds_quota - *refdbytesp);
		else
			*availbytesp = 0;
	}
	*usedobjsp = ds->ds_phys->ds_bp.blk_fill;
	*availobjsp = DN_MAX_OBJECT - *usedobjsp;
}

boolean_t
dsl_dataset_modified_since_lastsnap(dsl_dataset_t *ds)
{
	dsl_pool_t *dp = ds->ds_dir->dd_pool;

	ASSERT(RW_LOCK_HELD(&dp->dp_config_rwlock) ||
	    dsl_pool_sync_context(dp));
	if (ds->ds_prev == NULL)
		return (B_FALSE);
	if (ds->ds_phys->ds_bp.blk_birth >
	    ds->ds_prev->ds_phys->ds_creation_txg)
		return (B_TRUE);
	return (B_FALSE);
}

/* ARGSUSED */
static int
dsl_dataset_snapshot_rename_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	char *newsnapname = arg2;
	dsl_dir_t *dd = ds->ds_dir;
	dsl_dataset_t *hds;
	uint64_t val;
	int err;

	err = dsl_dataset_hold_obj(dd->dd_pool,
	    dd->dd_phys->dd_head_dataset_obj, FTAG, &hds);
	if (err)
		return (err);

	/* new name better not be in use */
	err = dsl_dataset_snap_lookup(hds, newsnapname, &val);
	dsl_dataset_rele(hds, FTAG);

	if (err == 0)
		err = EEXIST;
	else if (err == ENOENT)
		err = 0;

	/* dataset name + 1 for the "@" + the new snapshot name must fit */
	if (dsl_dir_namelen(ds->ds_dir) + 1 + strlen(newsnapname) >= MAXNAMELEN)
		err = ENAMETOOLONG;

	return (err);
}

static void
dsl_dataset_snapshot_rename_sync(void *arg1, void *arg2,
    cred_t *cr, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	const char *newsnapname = arg2;
	dsl_dir_t *dd = ds->ds_dir;
	objset_t *mos = dd->dd_pool->dp_meta_objset;
	dsl_dataset_t *hds;
	int err;

	ASSERT(ds->ds_phys->ds_next_snap_obj != 0);

	VERIFY(0 == dsl_dataset_hold_obj(dd->dd_pool,
	    dd->dd_phys->dd_head_dataset_obj, FTAG, &hds));

	VERIFY(0 == dsl_dataset_get_snapname(ds));
	err = dsl_dataset_snap_remove(hds, ds->ds_snapname, tx);
	ASSERT3U(err, ==, 0);
	mutex_enter(&ds->ds_lock);
	(void) strcpy(ds->ds_snapname, newsnapname);
	mutex_exit(&ds->ds_lock);
	err = zap_add(mos, hds->ds_phys->ds_snapnames_zapobj,
	    ds->ds_snapname, 8, 1, &ds->ds_object, tx);
	ASSERT3U(err, ==, 0);

	spa_history_internal_log(LOG_DS_RENAME, dd->dd_pool->dp_spa, tx,
	    cr, "dataset = %llu", ds->ds_object);
	dsl_dataset_rele(hds, FTAG);
}

struct renamesnaparg {
	dsl_sync_task_group_t *dstg;
	char failed[MAXPATHLEN];
	char *oldsnap;
	char *newsnap;
};

static int
dsl_snapshot_rename_one(char *name, void *arg)
{
	struct renamesnaparg *ra = arg;
	dsl_dataset_t *ds = NULL;
	char *cp;
	int err;

	cp = name + strlen(name);
	*cp = '@';
	(void) strcpy(cp + 1, ra->oldsnap);

	/*
	 * For recursive snapshot renames the parent won't be changing
	 * so we just pass name for both the to/from argument.
	 */
	if (err = zfs_secpolicy_rename_perms(name, name, CRED())) {
		(void) strcpy(ra->failed, name);
		return (err);
	}

#ifdef _KERNEL
	/*
	 * For all filesystems undergoing rename, we'll need to unmount it.
	 */
	(void) zfs_unmount_snap(name, NULL);
#endif
	err = dsl_dataset_hold(name, ra->dstg, &ds);
	*cp = '\0';
	if (err == ENOENT) {
		return (0);
	} else if (err) {
		(void) strcpy(ra->failed, name);
		return (err);
	}

	dsl_sync_task_create(ra->dstg, dsl_dataset_snapshot_rename_check,
	    dsl_dataset_snapshot_rename_sync, ds, ra->newsnap, 0);

	return (0);
}

static int
dsl_recursive_rename(char *oldname, const char *newname)
{
	int err;
	struct renamesnaparg *ra;
	dsl_sync_task_t *dst;
	spa_t *spa;
	char *cp, *fsname = spa_strdup(oldname);
	int len = strlen(oldname);

	/* truncate the snapshot name to get the fsname */
	cp = strchr(fsname, '@');
	*cp = '\0';

	err = spa_open(fsname, &spa, FTAG);
	if (err) {
		kmem_free(fsname, len + 1);
		return (err);
	}
	ra = kmem_alloc(sizeof (struct renamesnaparg), KM_SLEEP);
	ra->dstg = dsl_sync_task_group_create(spa_get_dsl(spa));

	ra->oldsnap = strchr(oldname, '@') + 1;
	ra->newsnap = strchr(newname, '@') + 1;
	*ra->failed = '\0';

	err = dmu_objset_find(fsname, dsl_snapshot_rename_one, ra,
	    DS_FIND_CHILDREN);
	kmem_free(fsname, len + 1);

	if (err == 0) {
		err = dsl_sync_task_group_wait(ra->dstg);
	}

	for (dst = list_head(&ra->dstg->dstg_tasks); dst;
	    dst = list_next(&ra->dstg->dstg_tasks, dst)) {
		dsl_dataset_t *ds = dst->dst_arg1;
		if (dst->dst_err) {
			dsl_dir_name(ds->ds_dir, ra->failed);
			(void) strcat(ra->failed, "@");
			(void) strcat(ra->failed, ra->newsnap);
		}
		dsl_dataset_rele(ds, ra->dstg);
	}

	if (err)
		(void) strcpy(oldname, ra->failed);

	dsl_sync_task_group_destroy(ra->dstg);
	kmem_free(ra, sizeof (struct renamesnaparg));
	spa_close(spa, FTAG);
	return (err);
}

static int
dsl_valid_rename(char *oldname, void *arg)
{
	int delta = *(int *)arg;

	if (strlen(oldname) + delta >= MAXNAMELEN)
		return (ENAMETOOLONG);

	return (0);
}

#pragma weak dmu_objset_rename = dsl_dataset_rename
int
dsl_dataset_rename(char *oldname, const char *newname, boolean_t recursive)
{
	dsl_dir_t *dd;
	dsl_dataset_t *ds;
	const char *tail;
	int err;

	err = dsl_dir_open(oldname, FTAG, &dd, &tail);
	if (err)
		return (err);
	if (tail == NULL) {
		int delta = strlen(newname) - strlen(oldname);

		/* if we're growing, validate child name lengths */
		if (delta > 0)
			err = dmu_objset_find(oldname, dsl_valid_rename,
			    &delta, DS_FIND_CHILDREN | DS_FIND_SNAPSHOTS);

		if (!err)
			err = dsl_dir_rename(dd, newname);
		dsl_dir_close(dd, FTAG);
		return (err);
	}
	if (tail[0] != '@') {
		/* the name ended in a nonexistant component */
		dsl_dir_close(dd, FTAG);
		return (ENOENT);
	}

	dsl_dir_close(dd, FTAG);

	/* new name must be snapshot in same filesystem */
	tail = strchr(newname, '@');
	if (tail == NULL)
		return (EINVAL);
	tail++;
	if (strncmp(oldname, newname, tail - newname) != 0)
		return (EXDEV);

	if (recursive) {
		err = dsl_recursive_rename(oldname, newname);
	} else {
		err = dsl_dataset_hold(oldname, FTAG, &ds);
		if (err)
			return (err);

		err = dsl_sync_task_do(ds->ds_dir->dd_pool,
		    dsl_dataset_snapshot_rename_check,
		    dsl_dataset_snapshot_rename_sync, ds, (char *)tail, 1);

		dsl_dataset_rele(ds, FTAG);
	}

	return (err);
}

struct promotenode {
	list_node_t link;
	dsl_dataset_t *ds;
};

struct promotearg {
	list_t snap_list;
	dsl_dataset_t *clone_origin, *old_head;
	uint64_t used, comp, uncomp, unique;
	uint64_t newnext_obj;
};

/* ARGSUSED */
static int
dsl_dataset_promote_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	dsl_dataset_t *hds = arg1;
	struct promotearg *pa = arg2;
	struct promotenode *snap = list_head(&pa->snap_list);
	dsl_pool_t *dp = hds->ds_dir->dd_pool;
	dsl_dataset_t *origin_ds = snap->ds;
	dsl_dataset_t *newnext_ds;
	char *name;
	uint64_t itor = 0;
	blkptr_t bp;
	int err;

	/* Check that it is a real clone */
	if (!dsl_dir_is_clone(hds->ds_dir))
		return (EINVAL);

	/* Since this is so expensive, don't do the preliminary check */
	if (!dmu_tx_is_syncing(tx))
		return (0);

	if (hds->ds_phys->ds_flags & DS_FLAG_NOPROMOTE)
		return (EXDEV);

	/* find origin's new next ds */
	newnext_ds = hds;
	while (newnext_ds->ds_phys->ds_prev_snap_obj != origin_ds->ds_object) {
		dsl_dataset_t *prev;

		err = dsl_dataset_hold_obj(dp,
		    newnext_ds->ds_phys->ds_prev_snap_obj, FTAG, &prev);
		if (newnext_ds != hds)
			dsl_dataset_rele(newnext_ds, FTAG);
		if (err)
			return (err);
		newnext_ds = prev;
	}
	pa->newnext_obj = newnext_ds->ds_object;

	/* compute origin's new unique space */
	pa->unique = 0;
	while ((err = bplist_iterate(&newnext_ds->ds_deadlist,
	    &itor, &bp)) == 0) {
		if (bp.blk_birth > origin_ds->ds_phys->ds_prev_snap_txg)
			pa->unique += bp_get_dasize(dp->dp_spa, &bp);
	}
	if (newnext_ds != hds)
		dsl_dataset_rele(newnext_ds, FTAG);
	if (err != ENOENT)
		return (err);

	name = kmem_alloc(MAXPATHLEN, KM_SLEEP);

	/*
	 * Walk the snapshots that we are moving
	 *
	 * Compute space to transfer.  Each snapshot gave birth to:
	 * (my used) - (prev's used) + (deadlist's used)
	 * So a sequence would look like:
	 * uN - u(N-1) + dN + ... + u1 - u0 + d1 + u0 - 0 + d0
	 * Which simplifies to:
	 * uN + dN + ... + d1 + d0
	 * Note however, if we stop before we reach the ORIGIN we get:
	 * uN + dN + ... + dM - uM-1
	 */
	pa->used = origin_ds->ds_phys->ds_used_bytes;
	pa->comp = origin_ds->ds_phys->ds_compressed_bytes;
	pa->uncomp = origin_ds->ds_phys->ds_uncompressed_bytes;
	do {
		uint64_t val, dlused, dlcomp, dluncomp;
		dsl_dataset_t *ds = snap->ds;

		/* Check that the snapshot name does not conflict */
		dsl_dataset_name(ds, name);
		err = dsl_dataset_snap_lookup(hds, ds->ds_snapname, &val);
		if (err == 0)
			err = EEXIST;
		if (err != ENOENT)
			break;
		err = 0;

		/* The very first snapshot does not have a deadlist */
		if (ds->ds_phys->ds_prev_snap_obj != 0) {
			if (err = bplist_space(&ds->ds_deadlist,
			    &dlused, &dlcomp, &dluncomp))
				break;
			pa->used += dlused;
			pa->comp += dlcomp;
			pa->uncomp += dluncomp;
		}
	} while (snap = list_next(&pa->snap_list, snap));

	/*
	 * If we are a clone of a clone then we never reached ORIGIN,
	 * so we need to subtract out the clone origin's used space.
	 */
	if (pa->clone_origin) {
		pa->used -= pa->clone_origin->ds_phys->ds_used_bytes;
		pa->comp -= pa->clone_origin->ds_phys->ds_compressed_bytes;
		pa->uncomp -= pa->clone_origin->ds_phys->ds_uncompressed_bytes;
	}

	kmem_free(name, MAXPATHLEN);

	/* Check that there is enough space here */
	if (err == 0) {
		dsl_dir_t *odd = origin_ds->ds_dir;
		err = dsl_dir_transfer_possible(odd, hds->ds_dir, pa->used);
	}

	return (err);
}

static void
dsl_dataset_promote_sync(void *arg1, void *arg2, cred_t *cr, dmu_tx_t *tx)
{
	dsl_dataset_t *hds = arg1;
	struct promotearg *pa = arg2;
	struct promotenode *snap = list_head(&pa->snap_list);
	dsl_dataset_t *origin_ds = snap->ds;
	dsl_dir_t *dd = hds->ds_dir;
	dsl_pool_t *dp = hds->ds_dir->dd_pool;
	dsl_dir_t *odd = NULL;
	char *name;
	uint64_t oldnext_obj;

	ASSERT(0 == (hds->ds_phys->ds_flags & DS_FLAG_NOPROMOTE));

	/*
	 * We need to explicitly open odd, since origin_ds's dd will be
	 * changing.
	 */
	VERIFY(0 == dsl_dir_open_obj(dp, origin_ds->ds_dir->dd_object,
	    NULL, FTAG, &odd));

	/* change origin's next snap */
	dmu_buf_will_dirty(origin_ds->ds_dbuf, tx);
	oldnext_obj = origin_ds->ds_phys->ds_next_snap_obj;
	origin_ds->ds_phys->ds_next_snap_obj = pa->newnext_obj;

	/* change the origin's next clone */
	if (origin_ds->ds_phys->ds_next_clones_obj) {
		VERIFY3U(0, ==, zap_remove_int(dp->dp_meta_objset,
		    origin_ds->ds_phys->ds_next_clones_obj,
		    pa->newnext_obj, tx));
		VERIFY3U(0, ==, zap_add_int(dp->dp_meta_objset,
		    origin_ds->ds_phys->ds_next_clones_obj,
		    oldnext_obj, tx));
	}

	/* change origin */
	dmu_buf_will_dirty(dd->dd_dbuf, tx);
	ASSERT3U(dd->dd_phys->dd_origin_obj, ==, origin_ds->ds_object);
	dd->dd_phys->dd_origin_obj = odd->dd_phys->dd_origin_obj;
	dmu_buf_will_dirty(odd->dd_dbuf, tx);
	odd->dd_phys->dd_origin_obj = origin_ds->ds_object;

	/* move snapshots to this dir */
	name = kmem_alloc(MAXPATHLEN, KM_SLEEP);
	do {
		dsl_dataset_t *ds = snap->ds;

		/* unregister props as dsl_dir is changing */
		if (ds->ds_user_ptr) {
			ds->ds_user_evict_func(ds, ds->ds_user_ptr);
			ds->ds_user_ptr = NULL;
		}
		/* move snap name entry */
		dsl_dataset_name(ds, name);
		VERIFY(0 == dsl_dataset_snap_remove(pa->old_head,
		    ds->ds_snapname, tx));
		VERIFY(0 == zap_add(dp->dp_meta_objset,
		    hds->ds_phys->ds_snapnames_zapobj, ds->ds_snapname,
		    8, 1, &ds->ds_object, tx));
		/* change containing dsl_dir */
		dmu_buf_will_dirty(ds->ds_dbuf, tx);
		ASSERT3U(ds->ds_phys->ds_dir_obj, ==, odd->dd_object);
		ds->ds_phys->ds_dir_obj = dd->dd_object;
		ASSERT3P(ds->ds_dir, ==, odd);
		dsl_dir_close(ds->ds_dir, ds);
		VERIFY(0 == dsl_dir_open_obj(dp, dd->dd_object,
		    NULL, ds, &ds->ds_dir));

		ASSERT3U(dsl_prop_numcb(ds), ==, 0);
	} while (snap = list_next(&pa->snap_list, snap));

	/* change space accounting */
	dsl_dir_diduse_space(odd, -pa->used, -pa->comp, -pa->uncomp, tx);
	dsl_dir_diduse_space(dd, pa->used, pa->comp, pa->uncomp, tx);
	origin_ds->ds_phys->ds_unique_bytes = pa->unique;

	/* log history record */
	spa_history_internal_log(LOG_DS_PROMOTE, dd->dd_pool->dp_spa, tx,
	    cr, "dataset = %llu", hds->ds_object);

	dsl_dir_close(odd, FTAG);
	kmem_free(name, MAXPATHLEN);
}

int
dsl_dataset_promote(const char *name)
{
	dsl_dataset_t *ds;
	dsl_dir_t *dd;
	dsl_pool_t *dp;
	dmu_object_info_t doi;
	struct promotearg pa;
	struct promotenode *snap;
	uint64_t snap_obj;
	uint64_t last_snap = 0;
	int err;

	err = dsl_dataset_hold(name, FTAG, &ds);
	if (err)
		return (err);
	dd = ds->ds_dir;
	dp = dd->dd_pool;

	err = dmu_object_info(dp->dp_meta_objset,
	    ds->ds_phys->ds_snapnames_zapobj, &doi);
	if (err) {
		dsl_dataset_rele(ds, FTAG);
		return (err);
	}

	/*
	 * We are going to inherit all the snapshots taken before our
	 * origin (i.e., our new origin will be our parent's origin).
	 * Take ownership of them so that we can rename them into our
	 * namespace.
	 */
	pa.clone_origin = NULL;
	list_create(&pa.snap_list,
	    sizeof (struct promotenode), offsetof(struct promotenode, link));
	rw_enter(&dp->dp_config_rwlock, RW_READER);
	ASSERT(dd->dd_phys->dd_origin_obj != 0);
	snap_obj = dd->dd_phys->dd_origin_obj;
	while (snap_obj) {
		dsl_dataset_t *snapds;

		/*
		 * NB: this would be handled by the below check for
		 * clone of a clone, but then we'd always own_obj() the
		 * $ORIGIN, thus causing unnecessary EBUSYs.  We don't
		 * need to set pa.clone_origin because the $ORIGIN has
		 * no data to account for.
		 */
		if (dp->dp_origin_snap &&
		    snap_obj == dp->dp_origin_snap->ds_object)
			break;

		err = dsl_dataset_own_obj(dp, snap_obj, 0, FTAG, &snapds);
		if (err == ENOENT) {
			/* lost race with snapshot destroy */
			struct promotenode *last = list_tail(&pa.snap_list);
			ASSERT(snap_obj != last->ds->ds_phys->ds_prev_snap_obj);
			snap_obj = last->ds->ds_phys->ds_prev_snap_obj;
			continue;
		} else if (err) {
			rw_exit(&dp->dp_config_rwlock);
			goto out;
		}

		/*
		 * We could be a clone of a clone.  If we reach our
		 * parent's branch point, we're done.
		 */
		if (last_snap &&
		    snapds->ds_phys->ds_next_snap_obj != last_snap) {
			pa.clone_origin = snapds;
			break;
		}

		snap = kmem_alloc(sizeof (struct promotenode), KM_SLEEP);
		snap->ds = snapds;
		list_insert_tail(&pa.snap_list, snap);
		last_snap = snap_obj;
		snap_obj = snap->ds->ds_phys->ds_prev_snap_obj;
	}
	snap = list_head(&pa.snap_list);
	ASSERT(snap != NULL);
	err = dsl_dataset_hold_obj(dp,
	    snap->ds->ds_dir->dd_phys->dd_head_dataset_obj, FTAG, &pa.old_head);
	rw_exit(&dp->dp_config_rwlock);

	if (err)
		goto out;

	/*
	 * Add in 128x the snapnames zapobj size, since we will be moving
	 * a bunch of snapnames to the promoted ds, and dirtying their
	 * bonus buffers.
	 */
	err = dsl_sync_task_do(dp, dsl_dataset_promote_check,
	    dsl_dataset_promote_sync, ds, &pa, 2 + 2 * doi.doi_physical_blks);

	dsl_dataset_rele(pa.old_head, FTAG);
out:
	while ((snap = list_tail(&pa.snap_list)) != NULL) {
		list_remove(&pa.snap_list, snap);
		dsl_dataset_disown(snap->ds, FTAG);
		kmem_free(snap, sizeof (struct promotenode));
	}
	list_destroy(&pa.snap_list);
	if (pa.clone_origin)
		dsl_dataset_disown(pa.clone_origin, FTAG);
	dsl_dataset_rele(ds, FTAG);
	return (err);
}

struct cloneswaparg {
	dsl_dataset_t *cds; /* clone dataset */
	dsl_dataset_t *ohds; /* origin's head dataset */
	boolean_t force;
	int64_t unused_refres_delta; /* change in unconsumed refreservation */
};

/* ARGSUSED */
static int
dsl_dataset_clone_swap_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	struct cloneswaparg *csa = arg1;

	/* they should both be heads */
	if (dsl_dataset_is_snapshot(csa->cds) ||
	    dsl_dataset_is_snapshot(csa->ohds))
		return (EINVAL);

	/* the branch point should be just before them */
	if (csa->cds->ds_prev != csa->ohds->ds_prev)
		return (EINVAL);

	/* cds should be the clone */
	if (csa->cds->ds_prev->ds_phys->ds_next_snap_obj !=
	    csa->ohds->ds_object)
		return (EINVAL);

	/* the clone should be a child of the origin */
	if (csa->cds->ds_dir->dd_parent != csa->ohds->ds_dir)
		return (EINVAL);

	/* ohds shouldn't be modified unless 'force' */
	if (!csa->force && dsl_dataset_modified_since_lastsnap(csa->ohds))
		return (ETXTBSY);

	/* adjust amount of any unconsumed refreservation */
	csa->unused_refres_delta =
	    (int64_t)MIN(csa->ohds->ds_reserved,
	    csa->ohds->ds_phys->ds_unique_bytes) -
	    (int64_t)MIN(csa->ohds->ds_reserved,
	    csa->cds->ds_phys->ds_unique_bytes);

	if (csa->unused_refres_delta > 0 &&
	    csa->unused_refres_delta >
	    dsl_dir_space_available(csa->ohds->ds_dir, NULL, 0, TRUE))
		return (ENOSPC);

	return (0);
}

/* ARGSUSED */
static void
dsl_dataset_clone_swap_sync(void *arg1, void *arg2, cred_t *cr, dmu_tx_t *tx)
{
	struct cloneswaparg *csa = arg1;
	dsl_pool_t *dp = csa->cds->ds_dir->dd_pool;
	uint64_t itor = 0;
	blkptr_t bp;
	uint64_t unique = 0;
	int err;

	ASSERT(csa->cds->ds_reserved == 0);
	ASSERT(csa->cds->ds_quota == csa->ohds->ds_quota);

	dmu_buf_will_dirty(csa->cds->ds_dbuf, tx);
	dmu_buf_will_dirty(csa->ohds->ds_dbuf, tx);
	dmu_buf_will_dirty(csa->cds->ds_prev->ds_dbuf, tx);

	if (csa->cds->ds_user_ptr != NULL) {
		csa->cds->ds_user_evict_func(csa->cds, csa->cds->ds_user_ptr);
		csa->cds->ds_user_ptr = NULL;
	}

	if (csa->ohds->ds_user_ptr != NULL) {
		csa->ohds->ds_user_evict_func(csa->ohds,
		    csa->ohds->ds_user_ptr);
		csa->ohds->ds_user_ptr = NULL;
	}

	/* compute unique space */
	while ((err = bplist_iterate(&csa->cds->ds_deadlist,
	    &itor, &bp)) == 0) {
		if (bp.blk_birth > csa->cds->ds_prev->ds_phys->ds_prev_snap_txg)
			unique += bp_get_dasize(dp->dp_spa, &bp);
	}
	VERIFY(err == ENOENT);

	/* reset origin's unique bytes */
	csa->cds->ds_prev->ds_phys->ds_unique_bytes = unique;

	/* swap blkptrs */
	{
		blkptr_t tmp;
		tmp = csa->ohds->ds_phys->ds_bp;
		csa->ohds->ds_phys->ds_bp = csa->cds->ds_phys->ds_bp;
		csa->cds->ds_phys->ds_bp = tmp;
	}

	/* set dd_*_bytes */
	{
		int64_t dused, dcomp, duncomp;
		uint64_t cdl_used, cdl_comp, cdl_uncomp;
		uint64_t odl_used, odl_comp, odl_uncomp;

		VERIFY(0 == bplist_space(&csa->cds->ds_deadlist, &cdl_used,
		    &cdl_comp, &cdl_uncomp));
		VERIFY(0 == bplist_space(&csa->ohds->ds_deadlist, &odl_used,
		    &odl_comp, &odl_uncomp));
		dused = csa->cds->ds_phys->ds_used_bytes + cdl_used -
		    (csa->ohds->ds_phys->ds_used_bytes + odl_used);
		dcomp = csa->cds->ds_phys->ds_compressed_bytes + cdl_comp -
		    (csa->ohds->ds_phys->ds_compressed_bytes + odl_comp);
		duncomp = csa->cds->ds_phys->ds_uncompressed_bytes +
		    cdl_uncomp -
		    (csa->ohds->ds_phys->ds_uncompressed_bytes + odl_uncomp);

		dsl_dir_diduse_space(csa->ohds->ds_dir,
		    dused, dcomp, duncomp, tx);
		dsl_dir_diduse_space(csa->cds->ds_dir,
		    -dused, -dcomp, -duncomp, tx);
	}

#define	SWITCH64(x, y) \
	{ \
		uint64_t __tmp = (x); \
		(x) = (y); \
		(y) = __tmp; \
	}

	/* swap ds_*_bytes */
	SWITCH64(csa->ohds->ds_phys->ds_used_bytes,
	    csa->cds->ds_phys->ds_used_bytes);
	SWITCH64(csa->ohds->ds_phys->ds_compressed_bytes,
	    csa->cds->ds_phys->ds_compressed_bytes);
	SWITCH64(csa->ohds->ds_phys->ds_uncompressed_bytes,
	    csa->cds->ds_phys->ds_uncompressed_bytes);
	SWITCH64(csa->ohds->ds_phys->ds_unique_bytes,
	    csa->cds->ds_phys->ds_unique_bytes);

	/* apply any parent delta for change in unconsumed refreservation */
	dsl_dir_diduse_space(csa->ohds->ds_dir, csa->unused_refres_delta,
	    0, 0, tx);

	/* swap deadlists */
	bplist_close(&csa->cds->ds_deadlist);
	bplist_close(&csa->ohds->ds_deadlist);
	SWITCH64(csa->ohds->ds_phys->ds_deadlist_obj,
	    csa->cds->ds_phys->ds_deadlist_obj);
	VERIFY(0 == bplist_open(&csa->cds->ds_deadlist, dp->dp_meta_objset,
	    csa->cds->ds_phys->ds_deadlist_obj));
	VERIFY(0 == bplist_open(&csa->ohds->ds_deadlist, dp->dp_meta_objset,
	    csa->ohds->ds_phys->ds_deadlist_obj));
}

/*
 * Swap 'clone' with its origin head file system.  Used at the end
 * of "online recv" to swizzle the file system to the new version.
 */
int
dsl_dataset_clone_swap(dsl_dataset_t *clone, dsl_dataset_t *origin_head,
    boolean_t force)
{
	struct cloneswaparg csa;
	int error;

	ASSERT(clone->ds_owner);
	ASSERT(origin_head->ds_owner);
retry:
	/* Need exclusive access for the swap */
	rw_enter(&clone->ds_rwlock, RW_WRITER);
	if (!rw_tryenter(&origin_head->ds_rwlock, RW_WRITER)) {
		rw_exit(&clone->ds_rwlock);
		rw_enter(&origin_head->ds_rwlock, RW_WRITER);
		if (!rw_tryenter(&clone->ds_rwlock, RW_WRITER)) {
			rw_exit(&origin_head->ds_rwlock);
			goto retry;
		}
	}
	csa.cds = clone;
	csa.ohds = origin_head;
	csa.force = force;
	error = dsl_sync_task_do(clone->ds_dir->dd_pool,
	    dsl_dataset_clone_swap_check,
	    dsl_dataset_clone_swap_sync, &csa, NULL, 9);
	return (error);
}

/*
 * Given a pool name and a dataset object number in that pool,
 * return the name of that dataset.
 */
int
dsl_dsobj_to_dsname(char *pname, uint64_t obj, char *buf)
{
	spa_t *spa;
	dsl_pool_t *dp;
	dsl_dataset_t *ds;
	int error;

	if ((error = spa_open(pname, &spa, FTAG)) != 0)
		return (error);
	dp = spa_get_dsl(spa);
	rw_enter(&dp->dp_config_rwlock, RW_READER);
	if ((error = dsl_dataset_hold_obj(dp, obj, FTAG, &ds)) == 0) {
		dsl_dataset_name(ds, buf);
		dsl_dataset_rele(ds, FTAG);
	}
	rw_exit(&dp->dp_config_rwlock);
	spa_close(spa, FTAG);

	return (error);
}

int
dsl_dataset_check_quota(dsl_dataset_t *ds, boolean_t check_quota,
    uint64_t asize, uint64_t inflight, uint64_t *used, uint64_t *ref_rsrv)
{
	int error = 0;

	ASSERT3S(asize, >, 0);

	/*
	 * *ref_rsrv is the portion of asize that will come from any
	 * unconsumed refreservation space.
	 */
	*ref_rsrv = 0;

	mutex_enter(&ds->ds_lock);
	/*
	 * Make a space adjustment for reserved bytes.
	 */
	if (ds->ds_reserved > ds->ds_phys->ds_unique_bytes) {
		ASSERT3U(*used, >=,
		    ds->ds_reserved - ds->ds_phys->ds_unique_bytes);
		*used -= (ds->ds_reserved - ds->ds_phys->ds_unique_bytes);
		*ref_rsrv =
		    asize - MIN(asize, parent_delta(ds, asize + inflight));
	}

	if (!check_quota || ds->ds_quota == 0) {
		mutex_exit(&ds->ds_lock);
		return (0);
	}
	/*
	 * If they are requesting more space, and our current estimate
	 * is over quota, they get to try again unless the actual
	 * on-disk is over quota and there are no pending changes (which
	 * may free up space for us).
	 */
	if (ds->ds_phys->ds_used_bytes + inflight >= ds->ds_quota) {
		if (inflight > 0 || ds->ds_phys->ds_used_bytes < ds->ds_quota)
			error = ERESTART;
		else
			error = EDQUOT;
	}
	mutex_exit(&ds->ds_lock);

	return (error);
}

/* ARGSUSED */
static int
dsl_dataset_set_quota_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	uint64_t *quotap = arg2;
	uint64_t new_quota = *quotap;

	if (spa_version(ds->ds_dir->dd_pool->dp_spa) < SPA_VERSION_REFQUOTA)
		return (ENOTSUP);

	if (new_quota == 0)
		return (0);

	if (new_quota < ds->ds_phys->ds_used_bytes ||
	    new_quota < ds->ds_reserved)
		return (ENOSPC);

	return (0);
}

/* ARGSUSED */
void
dsl_dataset_set_quota_sync(void *arg1, void *arg2, cred_t *cr, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	uint64_t *quotap = arg2;
	uint64_t new_quota = *quotap;

	dmu_buf_will_dirty(ds->ds_dbuf, tx);

	ds->ds_quota = new_quota;

	dsl_prop_set_uint64_sync(ds->ds_dir, "refquota", new_quota, cr, tx);

	spa_history_internal_log(LOG_DS_REFQUOTA, ds->ds_dir->dd_pool->dp_spa,
	    tx, cr, "%lld dataset = %llu ",
	    (longlong_t)new_quota, ds->ds_object);
}

int
dsl_dataset_set_quota(const char *dsname, uint64_t quota)
{
	dsl_dataset_t *ds;
	int err;

	err = dsl_dataset_hold(dsname, FTAG, &ds);
	if (err)
		return (err);

	if (quota != ds->ds_quota) {
		/*
		 * If someone removes a file, then tries to set the quota, we
		 * want to make sure the file freeing takes effect.
		 */
		txg_wait_open(ds->ds_dir->dd_pool, 0);

		err = dsl_sync_task_do(ds->ds_dir->dd_pool,
		    dsl_dataset_set_quota_check, dsl_dataset_set_quota_sync,
		    ds, &quota, 0);
	}
	dsl_dataset_rele(ds, FTAG);
	return (err);
}

static int
dsl_dataset_set_reservation_check(void *arg1, void *arg2, dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	uint64_t *reservationp = arg2;
	uint64_t new_reservation = *reservationp;
	int64_t delta;
	uint64_t unique;

	if (new_reservation > INT64_MAX)
		return (EOVERFLOW);

	if (spa_version(ds->ds_dir->dd_pool->dp_spa) <
	    SPA_VERSION_REFRESERVATION)
		return (ENOTSUP);

	if (dsl_dataset_is_snapshot(ds))
		return (EINVAL);

	/*
	 * If we are doing the preliminary check in open context, the
	 * space estimates may be inaccurate.
	 */
	if (!dmu_tx_is_syncing(tx))
		return (0);

	mutex_enter(&ds->ds_lock);
	unique = dsl_dataset_unique(ds);
	delta = MAX(unique, new_reservation) - MAX(unique, ds->ds_reserved);
	mutex_exit(&ds->ds_lock);

	if (delta > 0 &&
	    delta > dsl_dir_space_available(ds->ds_dir, NULL, 0, TRUE))
		return (ENOSPC);
	if (delta > 0 && ds->ds_quota > 0 &&
	    new_reservation > ds->ds_quota)
		return (ENOSPC);

	return (0);
}

/* ARGSUSED */
static void
dsl_dataset_set_reservation_sync(void *arg1, void *arg2, cred_t *cr,
    dmu_tx_t *tx)
{
	dsl_dataset_t *ds = arg1;
	uint64_t *reservationp = arg2;
	uint64_t new_reservation = *reservationp;
	uint64_t unique;
	int64_t delta;

	dmu_buf_will_dirty(ds->ds_dbuf, tx);

	mutex_enter(&ds->ds_lock);
	unique = dsl_dataset_unique(ds);
	delta = MAX(0, (int64_t)(new_reservation - unique)) -
	    MAX(0, (int64_t)(ds->ds_reserved - unique));
	ds->ds_reserved = new_reservation;
	mutex_exit(&ds->ds_lock);

	dsl_prop_set_uint64_sync(ds->ds_dir, "refreservation",
	    new_reservation, cr, tx);

	dsl_dir_diduse_space(ds->ds_dir, delta, 0, 0, tx);

	spa_history_internal_log(LOG_DS_REFRESERV,
	    ds->ds_dir->dd_pool->dp_spa, tx, cr, "%lld dataset = %llu",
	    (longlong_t)new_reservation,
	    ds->ds_dir->dd_phys->dd_head_dataset_obj);
}

int
dsl_dataset_set_reservation(const char *dsname, uint64_t reservation)
{
	dsl_dataset_t *ds;
	int err;

	err = dsl_dataset_hold(dsname, FTAG, &ds);
	if (err)
		return (err);

	err = dsl_sync_task_do(ds->ds_dir->dd_pool,
	    dsl_dataset_set_reservation_check,
	    dsl_dataset_set_reservation_sync, ds, &reservation, 0);
	dsl_dataset_rele(ds, FTAG);
	return (err);
}
