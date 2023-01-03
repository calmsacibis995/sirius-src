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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "nscd_db.h"
#include "nscd_log.h"
#include "nscd_switch.h"
#include "nscd_door.h"

extern int		_whoami;
static mutex_t		getent_monitor_mutex = DEFAULTMUTEX;
static int		getent_monitor_started = 0;

static rwlock_t		getent_ctxDB_rwlock = DEFAULTRWLOCK;
static nscd_db_t	*getent_ctxDB = NULL;

/*
 * internal structure representing a nscd getent context
 */
typedef struct nscd_getent_ctx {
	int			to_delete; /* this ctx no longer valid */
	nscd_getent_context_t	*ptr;
	nscd_cookie_num_t	cookie_num;
} nscd_getent_ctx_t;

/*
 * nscd_getent_context_t list for each nss database. Protected
 * by the readers/writer lock nscd_getent_ctx_lock.
 */
nscd_getent_ctx_base_t **nscd_getent_ctx_base;
static rwlock_t nscd_getent_ctx_base_lock = DEFAULTRWLOCK;

extern nscd_db_entry_t *_nscd_walk_db(nscd_db_t *db, void **cookie);

static nscd_rc_t _nscd_init_getent_ctx_monitor();

/*
 * FUNCTION: _nscd_create_getent_ctxDB
 *
 * Create the internal getent context database to keep track of the
 * getent contexts currently being used.
 */
nscd_db_t *
_nscd_create_getent_ctxDB()
{

	nscd_db_t	*ret;

	(void) rw_wrlock(&getent_ctxDB_rwlock);

	if (getent_ctxDB != NULL) {
		(void) rw_unlock(&getent_ctxDB_rwlock);
		return (getent_ctxDB);
	}

	ret = _nscd_alloc_db(NSCD_DB_SIZE_LARGE);

	if (ret != NULL)
		getent_ctxDB = ret;

	(void) rw_unlock(&getent_ctxDB_rwlock);

	return (ret);
}

/*
 * FUNCTION: _nscd_add_getent_ctx
 *
 * Add a getent context to the internal context database.
 */
static nscd_rc_t
_nscd_add_getent_ctx(
	nscd_getent_context_t	*ptr,
	nscd_cookie_num_t	cookie_num)
{
	int			size;
	char			buf[2 * sizeof (cookie_num) + 1];
	nscd_db_entry_t		*db_entry;
	nscd_getent_ctx_t	*gnctx;

	if (ptr == NULL)
		return (NSCD_INVALID_ARGUMENT);

	(void) snprintf(buf, sizeof (buf), "%lld", cookie_num);

	size = sizeof (*gnctx);

	db_entry = _nscd_alloc_db_entry(NSCD_DATA_CTX_ADDR,
			(const char *)buf, size, 1, 1);
	if (db_entry == NULL)
		return (NSCD_NO_MEMORY);

	gnctx = (nscd_getent_ctx_t *)*(db_entry->data_array);
	gnctx->ptr = ptr;
	gnctx->cookie_num = cookie_num;

	(void) rw_wrlock(&getent_ctxDB_rwlock);
	(void) _nscd_add_db_entry(getent_ctxDB, buf, db_entry,
		NSCD_ADD_DB_ENTRY_FIRST);
	(void) rw_unlock(&getent_ctxDB_rwlock);

	return (NSCD_SUCCESS);
}

/*
 * FUNCTION: _nscd_is_getent_ctx
 *
 * Check to see if a getent context can be found in the internal
 * getent context database.
 */
nscd_getent_context_t *
_nscd_is_getent_ctx(
	nscd_cookie_num_t	cookie_num)
{
	char			ptrstr[1 + 2 * sizeof (cookie_num)];
	const nscd_db_entry_t	*db_entry;
	nscd_getent_context_t	*ret = NULL;

	(void) snprintf(ptrstr, sizeof (ptrstr), "%lld", cookie_num);

	(void) rw_rdlock(&getent_ctxDB_rwlock);

	db_entry = _nscd_get_db_entry(getent_ctxDB, NSCD_DATA_CTX_ADDR,
		(const char *)ptrstr, NSCD_GET_FIRST_DB_ENTRY, 0);

	if (db_entry != NULL) {
		nscd_getent_ctx_t *gnctx;

		gnctx = (nscd_getent_ctx_t *)*(db_entry->data_array);

		/*
		 * If the ctx is not to be deleted and
		 * the cookie numbers match, return the ctx.
		 * Otherwise return NULL.
		 */
		if (gnctx->to_delete == 0 && gnctx->cookie_num == cookie_num)
			ret = gnctx->ptr;
	}

	(void) rw_unlock(&getent_ctxDB_rwlock);

	return (ret);
}

/*
 * FUNCTION: _nscd_del_getent_ctx
 *
 * Delete a getent context from the internal getent context database.
 */
static void
_nscd_del_getent_ctx(
	nscd_getent_context_t	*ptr,
	nscd_cookie_num_t	cookie_num)
{
	char			ptrstr[1 + 2 * sizeof (cookie_num)];
	nscd_getent_ctx_t	*gnctx;
	const nscd_db_entry_t	*db_entry;

	if (ptr == NULL)
		return;

	(void) snprintf(ptrstr, sizeof (ptrstr), "%lld", cookie_num);

	(void) rw_rdlock(&getent_ctxDB_rwlock);
	/*
	 * first find the db entry and make sure the
	 * sequence number matched, then delete it from
	 * the database.
	 */
	db_entry = _nscd_get_db_entry(getent_ctxDB,
		NSCD_DATA_CTX_ADDR,
		(const char *)ptrstr,
		NSCD_GET_FIRST_DB_ENTRY, 0);
	if (db_entry != NULL) {
		gnctx = (nscd_getent_ctx_t *)*(db_entry->data_array);
		if (gnctx->ptr == ptr && gnctx->cookie_num  == cookie_num) {

			(void) rw_unlock(&getent_ctxDB_rwlock);
			(void) rw_wrlock(&getent_ctxDB_rwlock);

			(void) _nscd_delete_db_entry(getent_ctxDB,
				NSCD_DATA_CTX_ADDR,
				(const char *)ptrstr,
				NSCD_DEL_FIRST_DB_ENTRY, 0);
		}
	}
	(void) rw_unlock(&getent_ctxDB_rwlock);
}

static void
_nscd_free_getent_ctx(
	nscd_getent_context_t	*gnctx)
{

	char			*me = "_nscd_free_getent_ctx";

	_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
	(me, "getent context %p\n", gnctx);

	_nscd_put_nsw_state(gnctx->nsw_state);
	_nscd_del_getent_ctx(gnctx, gnctx->cookie_num);
	free(gnctx);
}


static void
_nscd_free_getent_ctx_base(
	nscd_acc_data_t		*data)
{
	nscd_getent_ctx_base_t	*base = (nscd_getent_ctx_base_t *)data;
	nscd_getent_context_t	*c, *tc;
	char			*me = "_nscd_free_getent_ctx_base";

	_NSCD_LOG(NSCD_LOG_GETENT_CTX | NSCD_LOG_CONFIG, NSCD_LOG_LEVEL_DEBUG)
	(me, "getent context base %p\n", base);

	if (base == NULL)
		return;

	c = base->first;
	while (c != NULL) {
		tc = c->next;
		_nscd_free_getent_ctx(c);
		c = tc;
	}
}

void
_nscd_free_all_getent_ctx_base()
{
	nscd_getent_ctx_base_t	*base;
	int			i;
	char			*me = "_nscd_free_all_getent_ctx_base";

	_NSCD_LOG(NSCD_LOG_GETENT_CTX | NSCD_LOG_CONFIG, NSCD_LOG_LEVEL_DEBUG)
	(me, "entering ..\n");

	(void) rw_wrlock(&nscd_getent_ctx_base_lock);

	for (i = 0; i < NSCD_NUM_DB; i++) {

		base = nscd_getent_ctx_base[i];
		if (base == NULL)
			continue;

		nscd_getent_ctx_base[i] = (nscd_getent_ctx_base_t *)
			_nscd_set((nscd_acc_data_t *)base, NULL);
	}
	(void) rw_unlock(&nscd_getent_ctx_base_lock);
}

static nscd_getent_context_t *
_nscd_create_getent_ctx(
	nscd_nsw_params_t	*params)
{
	nscd_getent_context_t	*gnctx;
	nss_db_root_t		db_root;
	char			*me = "_nscd_create_getent_ctx";

	gnctx = calloc(1, sizeof (nscd_getent_context_t));
	if (gnctx == NULL)
		return (NULL);
	else {
		_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
		(me, "getent context allocated %p\n", gnctx);
	}

	gnctx->dbi = params->dbi;
	gnctx->cookie_num = _nscd_get_cookie_num();
	gnctx->pid = -1;

	if (_nscd_get_nsw_state(&db_root, params) != NSCD_SUCCESS) {
		free(gnctx);
		return (NULL);
	}
	gnctx->nsw_state = (nscd_nsw_state_t *)db_root.s;
	/* this is a nsw_state used for getent processing */
	gnctx->nsw_state->getent = 1;

	_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
	(me, "got nsw_state %p\n", gnctx->nsw_state);

	return (gnctx);
}


nscd_rc_t
_nscd_get_getent_ctx(
	nss_getent_t		*contextpp,
	nscd_nsw_params_t	*params)
{

	nscd_getent_context_t	*c;
	nscd_getent_ctx_base_t	*base, *tmp;
	nscd_rc_t		rc;
	char			*me = "_nscd_get_getent_ctx";

	_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
	(me, "entering ...\n");

	(void) rw_rdlock(&nscd_getent_ctx_base_lock);
	base = nscd_getent_ctx_base[params->dbi];
	(void) rw_unlock(&nscd_getent_ctx_base_lock);
	assert(base != NULL);

	/*
	 * If the context list is not empty, return the first one
	 * on the list. Otherwise, create and return a new one if
	 * limit is not reached. if reacehed, wait for the 'one is
	 * available' signal.
	 */
	tmp = (nscd_getent_ctx_base_t *)_nscd_mutex_lock(
		(nscd_acc_data_t *)base);
	assert(base == tmp);
	if (base->first == NULL) {
		if (base->num_getent_ctx == base->max_getent_ctx) {
			base->num_waiter++;
			while (base->first == NULL) {

				_NSCD_LOG(NSCD_LOG_GETENT_CTX,
					NSCD_LOG_LEVEL_DEBUG)
				(me, "waiting for signal\n");

				_nscd_cond_wait((nscd_acc_data_t *)base, NULL);

				_NSCD_LOG(NSCD_LOG_GETENT_CTX,
					NSCD_LOG_LEVEL_DEBUG)
				(me, "woke up\n");
			}
			base->num_waiter--;
		} else {
			base->first = _nscd_create_getent_ctx(params);
			if (base->first != NULL) {
				base->first->base = base;
				base->num_getent_ctx++;
			} else {
				/* not able to create an getent ctx */

				_NSCD_LOG(NSCD_LOG_GETENT_CTX,
					NSCD_LOG_LEVEL_ERROR)
				(me, "create getent ctx failed\n");

				_nscd_mutex_unlock((nscd_acc_data_t *)base);
				return (NSCD_CREATE_GETENT_CTX_FAILED);
			}

			_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
			(me, "got a new getent ctx %p\n", base->first);
		}
	}

	assert(base->first != NULL);

	c = base->first;
	base->first = c->next;
	c->next = NULL;
	c->seq_num = 1;

	_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
	(me, "got a getent ctx %p\n", c);

	_nscd_mutex_unlock((nscd_acc_data_t *)base);

	_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
	(me, "adding new ctx %p, cookie # = %lld\n", c, c->cookie_num);

	if ((rc = _nscd_add_getent_ctx(c, c->cookie_num)) != NSCD_SUCCESS) {
		_nscd_put_getent_ctx(c);
		return (rc);
	}
	contextpp->ctx = (struct nss_getent_context *)c;

	/* start monitor and reclaim orphan getent context */
	if (getent_monitor_started == 0) {
		(void) mutex_lock(&getent_monitor_mutex);
		if (getent_monitor_started == 0) {
			getent_monitor_started = 1;
			(void) _nscd_init_getent_ctx_monitor();
		}
		(void) mutex_unlock(&getent_monitor_mutex);
	}

	return (NSCD_SUCCESS);
}

void
_nscd_put_getent_ctx(
	nscd_getent_context_t	*gnctx)
{

	nscd_getent_ctx_base_t	*base;
	char			*me = "_nscd_put_getent_ctx";

	base = gnctx->base;
	gnctx->seq_num = 0;

	/* if context base is gone, so should this context */
	if ((_nscd_mutex_lock((nscd_acc_data_t *)base)) == NULL) {
		_nscd_free_getent_ctx(gnctx);
		return;
	}

	if (base->first != NULL) {
		gnctx->next = base->first;
		base->first = gnctx;
	} else
		base->first = gnctx;

	/* put back the db state */
	_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
	(me, "putting back nsw state %p\n", gnctx->nsw_state);

	/* this nsw_state is no longer used for getent processing */
	if (gnctx->nsw_state != NULL)
		gnctx->nsw_state->getent = 0;
	_nscd_put_nsw_state(gnctx->nsw_state);
	gnctx->nsw_state = NULL;

	_nscd_del_getent_ctx(gnctx, gnctx->cookie_num);

	_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
	(me, "ctx (%p, cookie # = %lld) removed from getent ctx DB\n",
		gnctx, gnctx->cookie_num);

	if (base->num_waiter > 0) {
		_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
		(me, "signaling (waiter = %d)\n", base->num_waiter);

		_nscd_cond_signal((nscd_acc_data_t *)base);
	}

	_nscd_mutex_unlock((nscd_acc_data_t *)base);
}

nscd_rc_t
_nscd_init_getent_ctx_base(
	int			dbi,
	int			lock)
{
	nscd_getent_ctx_base_t	*base = NULL;
	char			*me = "_nscd_init_getent_ctx_base";

	if (lock)
		(void) rw_rdlock(&nscd_getent_ctx_base_lock);

	base = (nscd_getent_ctx_base_t *)_nscd_alloc(
		NSCD_DATA_GETENT_CTX_BASE,
		sizeof (nscd_getent_ctx_base_t),
		_nscd_free_getent_ctx_base,
		NSCD_ALLOC_MUTEX | NSCD_ALLOC_COND);

	if (base == NULL) {
		if (lock)
			(void) rw_unlock(&nscd_getent_ctx_base_lock);
		return (NSCD_NO_MEMORY);
	}
	_NSCD_LOG(NSCD_LOG_GETENT_CTX | NSCD_LOG_CONFIG, NSCD_LOG_LEVEL_DEBUG)
	(me, "base %p allocated\n", base);

	/*
	 * initialize and activate the new getent_ctx base
	 */
	base->dbi = dbi;
	base->max_getent_ctx = NSCD_SW_CFG(dbi).max_getent_ctx_per_db;
	nscd_getent_ctx_base[dbi] =
		(nscd_getent_ctx_base_t *)_nscd_set(
		(nscd_acc_data_t *)nscd_getent_ctx_base[dbi],
		(nscd_acc_data_t *)base);

	if (lock)
		(void) rw_unlock(&nscd_getent_ctx_base_lock);

	return (NSCD_SUCCESS);
}

nscd_rc_t
_nscd_init_all_getent_ctx_base()
{
	int			i;
	nscd_rc_t		rc;
	char			*me = "_nscd_init_all_getent_ctx_base";

	(void) rw_wrlock(&nscd_getent_ctx_base_lock);

	for (i = 0; i < NSCD_NUM_DB; i++) {

		rc = _nscd_init_getent_ctx_base(i, 0);

		if (rc != NSCD_SUCCESS) {
			(void) rw_unlock(&nscd_getent_ctx_base_lock);
			return (rc);
		}
	}

	_NSCD_LOG(NSCD_LOG_GETENT_CTX | NSCD_LOG_CONFIG, NSCD_LOG_LEVEL_DEBUG)
	(me, "all getent context base initialized\n");

	(void) rw_unlock(&nscd_getent_ctx_base_lock);

	return (NSCD_SUCCESS);
}
nscd_rc_t
_nscd_alloc_getent_ctx_base()
{

	(void) rw_wrlock(&nscd_getent_ctx_base_lock);

	nscd_getent_ctx_base = calloc(NSCD_NUM_DB,
			sizeof (nscd_getent_ctx_base_t *));
	if (nscd_getent_ctx_base == NULL) {
		(void) rw_unlock(&nscd_getent_ctx_base_lock);
		return (NSCD_NO_MEMORY);
	}

	(void) rw_unlock(&nscd_getent_ctx_base_lock);

	return (NSCD_SUCCESS);
}

static int
process_exited(pid_t pid)
{
	char	pname[PATH_MAX];
	int	fd;

	(void) snprintf(pname, sizeof (pname), "/proc/%d/psinfo", pid);
	if ((fd = open(pname, O_RDONLY)) == -1)
		return (1);
	else {
		(void) close(fd);
		return (0);
	}
}

/*
 * FUNCTION: reclaim_getent_ctx
 */
/*ARGSUSED*/
static void *
reclaim_getent_ctx(void *arg)
{
	void			*cookie = NULL;
	nscd_db_entry_t		*ep;
	nscd_getent_ctx_t	*ctx;
	nscd_getent_context_t	*gctx, *c;
	nscd_getent_context_t	*first = NULL, *last = NULL;
	char			*me = "reclaim_getent_ctx";

	/*CONSTCOND*/
	while (1) {

		(void) sleep(60);

		(void) rw_rdlock(&getent_ctxDB_rwlock);

		for (ep = _nscd_walk_db(getent_ctxDB, &cookie); ep != NULL;
				ep = _nscd_walk_db(getent_ctxDB, &cookie)) {

			ctx = (nscd_getent_ctx_t *)*(ep->data_array);

			gctx = ctx->ptr;

			/*
			 * if the client process, which did the setent,
			 * exited, add the context to the orphan list
			 */
			if (gctx->pid != -1 && process_exited(gctx->pid)) {

				_NSCD_LOG(NSCD_LOG_GETENT_CTX,
					NSCD_LOG_LEVEL_DEBUG)
				(me, "process  %d exited, "
				"getent context = %p, "
				"db index = %d, cookie # = %lld, "
				"sequence # = %lld\n",
				gctx->pid, gctx, gctx->dbi,
				gctx->cookie_num, gctx->seq_num);

				if (first != NULL) {
					last->next = gctx;
					last = gctx;
				} else {
					first = gctx;
					last = gctx;
				}
			}
		}

		(void) rw_unlock(&getent_ctxDB_rwlock);


		/*
		 * return all the orphan getent contexts to the pool
		 */
		for (gctx = first; gctx; ) {
			c = gctx->next;
			gctx->next = NULL;
			_nscd_put_getent_ctx(gctx);
			gctx = c;
		}
		first = last = NULL;
	}
	/*NOTREACHED*/
	/*LINTED E_FUNC_HAS_NO_RETURN_STMT*/
}

static nscd_rc_t
_nscd_init_getent_ctx_monitor() {

	int	errnum;
	char	*me = "_nscd_init_getent_ctx_monitor";

	_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_DEBUG)
	(me, "initializing the getent context monitor\n");

	/*
	 * the forker nscd does not process getent requests
	 * so no need to monitor orphan getent contexts
	 */
	if (_whoami == NSCD_FORKER)
		return (NSCD_SUCCESS);

	/*
	 * start a thread to reclaim unused getent contexts
	 */
	if (thr_create(NULL, NULL, reclaim_getent_ctx,
		NULL, THR_DETACHED, NULL) != 0) {
		errnum = errno;
		_NSCD_LOG(NSCD_LOG_GETENT_CTX, NSCD_LOG_LEVEL_ERROR)
		(me, "thr_create: %s\n", strerror(errnum));
		return (NSCD_THREAD_CREATE_ERROR);
	}

	return (NSCD_SUCCESS);
}
