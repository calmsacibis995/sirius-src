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

#include <sys/zfs_context.h>
#include <sys/txg_impl.h>
#include <sys/dmu_impl.h>
#include <sys/dsl_pool.h>
#include <sys/callb.h>

/*
 * Pool-wide transaction groups.
 */

static void txg_sync_thread(dsl_pool_t *dp);
static void txg_quiesce_thread(dsl_pool_t *dp);

int zfs_txg_timeout = 30;	/* max seconds worth of delta per txg */
int zfs_txg_synctime = 5;	/* target seconds to sync a txg */

int zfs_write_limit_shift = 3;	/* 1/8th of physical memory */

uint64_t zfs_write_limit_min = 32 << 20; /* min write limit is 32MB */
uint64_t zfs_write_limit_max = 0; /* max data payload per txg */
uint64_t zfs_write_limit_inflated = 0;

/*
 * Prepare the txg subsystem.
 */
void
txg_init(dsl_pool_t *dp, uint64_t txg)
{
	tx_state_t *tx = &dp->dp_tx;
	int c;
	bzero(tx, sizeof (tx_state_t));

	tx->tx_cpu = kmem_zalloc(max_ncpus * sizeof (tx_cpu_t), KM_SLEEP);

	for (c = 0; c < max_ncpus; c++) {
		int i;

		mutex_init(&tx->tx_cpu[c].tc_lock, NULL, MUTEX_DEFAULT, NULL);
		for (i = 0; i < TXG_SIZE; i++) {
			cv_init(&tx->tx_cpu[c].tc_cv[i], NULL, CV_DEFAULT,
			    NULL);
		}
	}

	rw_init(&tx->tx_suspend, NULL, RW_DEFAULT, NULL);
	mutex_init(&tx->tx_sync_lock, NULL, MUTEX_DEFAULT, NULL);

	tx->tx_open_txg = txg;
}

/*
 * Close down the txg subsystem.
 */
void
txg_fini(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;
	int c;

	ASSERT(tx->tx_threads == 0);

	rw_destroy(&tx->tx_suspend);
	mutex_destroy(&tx->tx_sync_lock);

	for (c = 0; c < max_ncpus; c++) {
		int i;

		mutex_destroy(&tx->tx_cpu[c].tc_lock);
		for (i = 0; i < TXG_SIZE; i++)
			cv_destroy(&tx->tx_cpu[c].tc_cv[i]);
	}

	kmem_free(tx->tx_cpu, max_ncpus * sizeof (tx_cpu_t));

	bzero(tx, sizeof (tx_state_t));
}

/*
 * Start syncing transaction groups.
 */
void
txg_sync_start(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;

	mutex_enter(&tx->tx_sync_lock);

	dprintf("pool %p\n", dp);

	ASSERT(tx->tx_threads == 0);

	tx->tx_threads = 2;

	tx->tx_quiesce_thread = thread_create(NULL, 0, txg_quiesce_thread,
	    dp, 0, &p0, TS_RUN, minclsyspri);

	/*
	 * The sync thread can need a larger-than-default stack size on
	 * 32-bit x86.  This is due in part to nested pools and
	 * scrub_visitbp() recursion.
	 */
	tx->tx_sync_thread = thread_create(NULL, 12<<10, txg_sync_thread,
	    dp, 0, &p0, TS_RUN, minclsyspri);

	mutex_exit(&tx->tx_sync_lock);
}

static void
txg_thread_enter(tx_state_t *tx, callb_cpr_t *cpr)
{
	CALLB_CPR_INIT(cpr, &tx->tx_sync_lock, callb_generic_cpr, FTAG);
	mutex_enter(&tx->tx_sync_lock);
}

static void
txg_thread_exit(tx_state_t *tx, callb_cpr_t *cpr, kthread_t **tpp)
{
	ASSERT(*tpp != NULL);
	*tpp = NULL;
	tx->tx_threads--;
	cv_broadcast(&tx->tx_exit_cv);
	CALLB_CPR_EXIT(cpr);		/* drops &tx->tx_sync_lock */
	thread_exit();
}

static void
txg_thread_wait(tx_state_t *tx, callb_cpr_t *cpr, kcondvar_t *cv, uint64_t time)
{
	CALLB_CPR_SAFE_BEGIN(cpr);

	if (time)
		(void) cv_timedwait(cv, &tx->tx_sync_lock, lbolt + time);
	else
		cv_wait(cv, &tx->tx_sync_lock);

	CALLB_CPR_SAFE_END(cpr, &tx->tx_sync_lock);
}

/*
 * Stop syncing transaction groups.
 */
void
txg_sync_stop(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;

	dprintf("pool %p\n", dp);
	/*
	 * Finish off any work in progress.
	 */
	ASSERT(tx->tx_threads == 2);
	txg_wait_synced(dp, 0);

	/*
	 * Wake all sync threads and wait for them to die.
	 */
	mutex_enter(&tx->tx_sync_lock);

	ASSERT(tx->tx_threads == 2);

	tx->tx_exiting = 1;

	cv_broadcast(&tx->tx_quiesce_more_cv);
	cv_broadcast(&tx->tx_quiesce_done_cv);
	cv_broadcast(&tx->tx_sync_more_cv);

	while (tx->tx_threads != 0)
		cv_wait(&tx->tx_exit_cv, &tx->tx_sync_lock);

	tx->tx_exiting = 0;

	mutex_exit(&tx->tx_sync_lock);
}

uint64_t
txg_hold_open(dsl_pool_t *dp, txg_handle_t *th)
{
	tx_state_t *tx = &dp->dp_tx;
	tx_cpu_t *tc = &tx->tx_cpu[CPU_SEQID];
	uint64_t txg;

	mutex_enter(&tc->tc_lock);

	txg = tx->tx_open_txg;
	tc->tc_count[txg & TXG_MASK]++;

	th->th_cpu = tc;
	th->th_txg = txg;

	return (txg);
}

void
txg_rele_to_quiesce(txg_handle_t *th)
{
	tx_cpu_t *tc = th->th_cpu;

	mutex_exit(&tc->tc_lock);
}

void
txg_rele_to_sync(txg_handle_t *th)
{
	tx_cpu_t *tc = th->th_cpu;
	int g = th->th_txg & TXG_MASK;

	mutex_enter(&tc->tc_lock);
	ASSERT(tc->tc_count[g] != 0);
	if (--tc->tc_count[g] == 0)
		cv_broadcast(&tc->tc_cv[g]);
	mutex_exit(&tc->tc_lock);

	th->th_cpu = NULL;	/* defensive */
}

static void
txg_quiesce(dsl_pool_t *dp, uint64_t txg)
{
	tx_state_t *tx = &dp->dp_tx;
	int g = txg & TXG_MASK;
	int c;

	/*
	 * Grab all tx_cpu locks so nobody else can get into this txg.
	 */
	for (c = 0; c < max_ncpus; c++)
		mutex_enter(&tx->tx_cpu[c].tc_lock);

	ASSERT(txg == tx->tx_open_txg);
	tx->tx_open_txg++;

	/*
	 * Now that we've incremented tx_open_txg, we can let threads
	 * enter the next transaction group.
	 */
	for (c = 0; c < max_ncpus; c++)
		mutex_exit(&tx->tx_cpu[c].tc_lock);

	/*
	 * Quiesce the transaction group by waiting for everyone to txg_exit().
	 */
	for (c = 0; c < max_ncpus; c++) {
		tx_cpu_t *tc = &tx->tx_cpu[c];
		mutex_enter(&tc->tc_lock);
		while (tc->tc_count[g] != 0)
			cv_wait(&tc->tc_cv[g], &tc->tc_lock);
		mutex_exit(&tc->tc_lock);
	}
}

static void
txg_sync_thread(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;
	callb_cpr_t cpr;
	uint64_t timeout, start, delta, timer;
	int target;

	txg_thread_enter(tx, &cpr);

	start = delta = 0;
	timeout = zfs_txg_timeout * hz;
	for (;;) {
		uint64_t txg, written;

		/*
		 * We sync when there's someone waiting on us, or the
		 * quiesce thread has handed off a txg to us, or we have
		 * reached our timeout.
		 */
		timer = (delta >= timeout ? 0 : timeout - delta);
		while (!tx->tx_exiting && timer > 0 &&
		    tx->tx_synced_txg >= tx->tx_sync_txg_waiting &&
		    tx->tx_quiesced_txg == 0) {
			dprintf("waiting; tx_synced=%llu waiting=%llu dp=%p\n",
			    tx->tx_synced_txg, tx->tx_sync_txg_waiting, dp);
			txg_thread_wait(tx, &cpr, &tx->tx_sync_more_cv, timer);
			delta = lbolt - start;
			timer = (delta > timeout ? 0 : timeout - delta);
		}

		/*
		 * Wait until the quiesce thread hands off a txg to us,
		 * prompting it to do so if necessary.
		 */
		while (!tx->tx_exiting && tx->tx_quiesced_txg == 0) {
			if (tx->tx_quiesce_txg_waiting < tx->tx_open_txg+1)
				tx->tx_quiesce_txg_waiting = tx->tx_open_txg+1;
			cv_broadcast(&tx->tx_quiesce_more_cv);
			txg_thread_wait(tx, &cpr, &tx->tx_quiesce_done_cv, 0);
		}

		if (tx->tx_exiting)
			txg_thread_exit(tx, &cpr, &tx->tx_sync_thread);

		rw_enter(&tx->tx_suspend, RW_WRITER);

		/*
		 * Consume the quiesced txg which has been handed off to
		 * us.  This may cause the quiescing thread to now be
		 * able to quiesce another txg, so we must signal it.
		 */
		txg = tx->tx_quiesced_txg;
		tx->tx_quiesced_txg = 0;
		tx->tx_syncing_txg = txg;
		cv_broadcast(&tx->tx_quiesce_more_cv);
		rw_exit(&tx->tx_suspend);

		dprintf("txg=%llu quiesce_txg=%llu sync_txg=%llu\n",
		    txg, tx->tx_quiesce_txg_waiting, tx->tx_sync_txg_waiting);
		mutex_exit(&tx->tx_sync_lock);
		start = lbolt;
		spa_sync(dp->dp_spa, txg);
		delta = (lbolt - start) + 1;

		written = dp->dp_space_towrite[txg & TXG_MASK];
		dp->dp_space_towrite[txg & TXG_MASK] = 0;
		ASSERT(dp->dp_tempreserved[txg & TXG_MASK] == 0);

		/*
		 * If the write limit max has not been explicitly set, set it
		 * to a fraction of available phisical memory (default 1/8th).
		 * Note that we must inflate the limit because the spa
		 * inflates write sizes to account for data replication.
		 * Check this each sync phase to catch changing memory size.
		 */
		if (zfs_write_limit_inflated == 0 ||
		    (zfs_write_limit_shift && zfs_write_limit_max !=
		    physmem * PAGESIZE >> zfs_write_limit_shift)) {
			zfs_write_limit_max =
			    physmem * PAGESIZE >> zfs_write_limit_shift;
			zfs_write_limit_inflated =
			    spa_get_asize(dp->dp_spa, zfs_write_limit_max);
			if (zfs_write_limit_min > zfs_write_limit_inflated)
				zfs_write_limit_inflated = zfs_write_limit_min;
		}

		/*
		 * Attempt to keep the sync time consistant by adjusting the
		 * amount of write traffic allowed into each transaction group.
		 */
		target = zfs_txg_synctime * hz;
		if (delta > target) {
			uint64_t old = MIN(dp->dp_write_limit, written);

			dp->dp_write_limit = MAX(zfs_write_limit_min,
			    old * target / delta);
		} else if (written >= dp->dp_write_limit &&
		    delta >> 3 < target >> 3) {
			uint64_t rescale =
			    MIN((100 * target) / delta, 200);

			dp->dp_write_limit = MIN(zfs_write_limit_inflated,
			    written * rescale / 100);
		}

		mutex_enter(&tx->tx_sync_lock);
		rw_enter(&tx->tx_suspend, RW_WRITER);
		tx->tx_synced_txg = txg;
		tx->tx_syncing_txg = 0;
		rw_exit(&tx->tx_suspend);
		cv_broadcast(&tx->tx_sync_done_cv);
	}
}

static void
txg_quiesce_thread(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;
	callb_cpr_t cpr;

	txg_thread_enter(tx, &cpr);

	for (;;) {
		uint64_t txg;

		/*
		 * We quiesce when there's someone waiting on us.
		 * However, we can only have one txg in "quiescing" or
		 * "quiesced, waiting to sync" state.  So we wait until
		 * the "quiesced, waiting to sync" txg has been consumed
		 * by the sync thread.
		 */
		while (!tx->tx_exiting &&
		    (tx->tx_open_txg >= tx->tx_quiesce_txg_waiting ||
		    tx->tx_quiesced_txg != 0))
			txg_thread_wait(tx, &cpr, &tx->tx_quiesce_more_cv, 0);

		if (tx->tx_exiting)
			txg_thread_exit(tx, &cpr, &tx->tx_quiesce_thread);

		txg = tx->tx_open_txg;
		dprintf("txg=%llu quiesce_txg=%llu sync_txg=%llu\n",
		    txg, tx->tx_quiesce_txg_waiting,
		    tx->tx_sync_txg_waiting);
		mutex_exit(&tx->tx_sync_lock);
		txg_quiesce(dp, txg);
		mutex_enter(&tx->tx_sync_lock);

		/*
		 * Hand this txg off to the sync thread.
		 */
		dprintf("quiesce done, handing off txg %llu\n", txg);
		tx->tx_quiesced_txg = txg;
		cv_broadcast(&tx->tx_sync_more_cv);
		cv_broadcast(&tx->tx_quiesce_done_cv);
	}
}

/*
 * Delay this thread by 'ticks' if we are still in the open transaction
 * group and there is already a waiting txg quiesing or quiesced.  Abort
 * the delay if this txg stalls or enters the quiesing state.
 */
void
txg_delay(dsl_pool_t *dp, uint64_t txg, int ticks)
{
	tx_state_t *tx = &dp->dp_tx;
	int timeout = lbolt + ticks;

	/* don't delay if this txg could transition to quiesing immediately */
	if (tx->tx_open_txg > txg ||
	    tx->tx_syncing_txg == txg-1 || tx->tx_synced_txg == txg-1)
		return;

	mutex_enter(&tx->tx_sync_lock);
	if (tx->tx_open_txg > txg || tx->tx_synced_txg == txg-1) {
		mutex_exit(&tx->tx_sync_lock);
		return;
	}

	while (lbolt < timeout &&
	    tx->tx_syncing_txg < txg-1 && !txg_stalled(dp))
		(void) cv_timedwait(&tx->tx_quiesce_more_cv, &tx->tx_sync_lock,
		    timeout);

	mutex_exit(&tx->tx_sync_lock);
}

void
txg_wait_synced(dsl_pool_t *dp, uint64_t txg)
{
	tx_state_t *tx = &dp->dp_tx;

	mutex_enter(&tx->tx_sync_lock);
	ASSERT(tx->tx_threads == 2);
	if (txg == 0)
		txg = tx->tx_open_txg;
	if (tx->tx_sync_txg_waiting < txg)
		tx->tx_sync_txg_waiting = txg;
	dprintf("txg=%llu quiesce_txg=%llu sync_txg=%llu\n",
	    txg, tx->tx_quiesce_txg_waiting, tx->tx_sync_txg_waiting);
	while (tx->tx_synced_txg < txg) {
		dprintf("broadcasting sync more "
		    "tx_synced=%llu waiting=%llu dp=%p\n",
		    tx->tx_synced_txg, tx->tx_sync_txg_waiting, dp);
		cv_broadcast(&tx->tx_sync_more_cv);
		cv_wait(&tx->tx_sync_done_cv, &tx->tx_sync_lock);
	}
	mutex_exit(&tx->tx_sync_lock);
}

void
txg_wait_open(dsl_pool_t *dp, uint64_t txg)
{
	tx_state_t *tx = &dp->dp_tx;

	mutex_enter(&tx->tx_sync_lock);
	ASSERT(tx->tx_threads == 2);
	if (txg == 0)
		txg = tx->tx_open_txg + 1;
	if (tx->tx_quiesce_txg_waiting < txg)
		tx->tx_quiesce_txg_waiting = txg;
	dprintf("txg=%llu quiesce_txg=%llu sync_txg=%llu\n",
	    txg, tx->tx_quiesce_txg_waiting, tx->tx_sync_txg_waiting);
	while (tx->tx_open_txg < txg) {
		cv_broadcast(&tx->tx_quiesce_more_cv);
		cv_wait(&tx->tx_quiesce_done_cv, &tx->tx_sync_lock);
	}
	mutex_exit(&tx->tx_sync_lock);
}

boolean_t
txg_stalled(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;
	return (tx->tx_quiesce_txg_waiting > tx->tx_open_txg);
}

boolean_t
txg_sync_waiting(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;

	return (tx->tx_syncing_txg <= tx->tx_sync_txg_waiting ||
	    tx->tx_quiesced_txg != 0);
}

void
txg_suspend(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;
	/* XXX some code paths suspend when they are already suspended! */
	rw_enter(&tx->tx_suspend, RW_READER);
}

void
txg_resume(dsl_pool_t *dp)
{
	tx_state_t *tx = &dp->dp_tx;
	rw_exit(&tx->tx_suspend);
}

/*
 * Per-txg object lists.
 */
void
txg_list_create(txg_list_t *tl, size_t offset)
{
	int t;

	mutex_init(&tl->tl_lock, NULL, MUTEX_DEFAULT, NULL);

	tl->tl_offset = offset;

	for (t = 0; t < TXG_SIZE; t++)
		tl->tl_head[t] = NULL;
}

void
txg_list_destroy(txg_list_t *tl)
{
	int t;

	for (t = 0; t < TXG_SIZE; t++)
		ASSERT(txg_list_empty(tl, t));

	mutex_destroy(&tl->tl_lock);
}

int
txg_list_empty(txg_list_t *tl, uint64_t txg)
{
	return (tl->tl_head[txg & TXG_MASK] == NULL);
}

/*
 * Add an entry to the list.
 * Returns 0 if it's a new entry, 1 if it's already there.
 */
int
txg_list_add(txg_list_t *tl, void *p, uint64_t txg)
{
	int t = txg & TXG_MASK;
	txg_node_t *tn = (txg_node_t *)((char *)p + tl->tl_offset);
	int already_on_list;

	mutex_enter(&tl->tl_lock);
	already_on_list = tn->tn_member[t];
	if (!already_on_list) {
		tn->tn_member[t] = 1;
		tn->tn_next[t] = tl->tl_head[t];
		tl->tl_head[t] = tn;
	}
	mutex_exit(&tl->tl_lock);

	return (already_on_list);
}

/*
 * Remove the head of the list and return it.
 */
void *
txg_list_remove(txg_list_t *tl, uint64_t txg)
{
	int t = txg & TXG_MASK;
	txg_node_t *tn;
	void *p = NULL;

	mutex_enter(&tl->tl_lock);
	if ((tn = tl->tl_head[t]) != NULL) {
		p = (char *)tn - tl->tl_offset;
		tl->tl_head[t] = tn->tn_next[t];
		tn->tn_next[t] = NULL;
		tn->tn_member[t] = 0;
	}
	mutex_exit(&tl->tl_lock);

	return (p);
}

/*
 * Remove a specific item from the list and return it.
 */
void *
txg_list_remove_this(txg_list_t *tl, void *p, uint64_t txg)
{
	int t = txg & TXG_MASK;
	txg_node_t *tn, **tp;

	mutex_enter(&tl->tl_lock);

	for (tp = &tl->tl_head[t]; (tn = *tp) != NULL; tp = &tn->tn_next[t]) {
		if ((char *)tn - tl->tl_offset == p) {
			*tp = tn->tn_next[t];
			tn->tn_next[t] = NULL;
			tn->tn_member[t] = 0;
			mutex_exit(&tl->tl_lock);
			return (p);
		}
	}

	mutex_exit(&tl->tl_lock);

	return (NULL);
}

int
txg_list_member(txg_list_t *tl, void *p, uint64_t txg)
{
	int t = txg & TXG_MASK;
	txg_node_t *tn = (txg_node_t *)((char *)p + tl->tl_offset);

	return (tn->tn_member[t]);
}

/*
 * Walk a txg list -- only safe if you know it's not changing.
 */
void *
txg_list_head(txg_list_t *tl, uint64_t txg)
{
	int t = txg & TXG_MASK;
	txg_node_t *tn = tl->tl_head[t];

	return (tn == NULL ? NULL : (char *)tn - tl->tl_offset);
}

void *
txg_list_next(txg_list_t *tl, void *p, uint64_t txg)
{
	int t = txg & TXG_MASK;
	txg_node_t *tn = (txg_node_t *)((char *)p + tl->tl_offset);

	tn = tn->tn_next[t];

	return (tn == NULL ? NULL : (char *)tn - tl->tl_offset);
}
