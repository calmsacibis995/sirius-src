/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/systm.h>
#include <sys/kstat.h>
#include <sys/modctl.h>
#include <rpc/rpc_rdma.h>

#include <sys/ib/ibtl/ibti.h>

/*
 * RDMA chunk size
 */
#define	RDMA_MINCHUNK	1024
uint_t rdma_minchunk = RDMA_MINCHUNK;

/*
 * Globals
 */
int rdma_modloaded = 0;		/* flag to load RDMA plugin modules */
int rdma_dev_available = 0;	/* if any RDMA device is loaded */
kmutex_t rdma_modload_lock;	/* protects rdma_modloaded flag */
rdma_registry_t	*rdma_mod_head = NULL;	/* head for RDMA modules */
krwlock_t	rdma_lock;		/* protects rdma_mod_head list */
ldi_ident_t rpcmod_li = NULL;	/* identifies us with ldi_ framework */

/*
 * Statics
 */
static ldi_handle_t rpcib_handle = NULL;

/*
 * Externs
 */
extern	kstat_named_t	*rdmarcstat_ptr;
extern	uint_t		rdmarcstat_ndata;
extern	kstat_named_t	*rdmarsstat_ptr;
extern	uint_t		rdmarsstat_ndata;

void rdma_kstat_init();

/*
 * RDMATF module registration routine.
 * This routine is expected to be called by the init routine in
 * the plugin modules.
 */
rdma_stat
rdma_register_mod(rdma_mod_t *mod)
{
	rdma_registry_t **mp, *m;

	if (mod->rdma_version != RDMATF_VERS) {
		return (RDMA_BADVERS);
	}

	rw_enter(&rdma_lock, RW_WRITER);
	/*
	 * Ensure not already registered
	 */
	mp = &rdma_mod_head;
	while (*mp != NULL) {
		if (strncmp((*mp)->r_mod->rdma_api, mod->rdma_api,
		    KNC_STRSIZE) == 0) {
			rw_exit(&rdma_lock);
			return (RDMA_REG_EXIST);
		}
		mp = &((*mp)->r_next);
	}

	/*
	 * New one, create and add to registry
	 */
	m = kmem_alloc(sizeof (rdma_registry_t), KM_SLEEP);
	m->r_mod = kmem_alloc(sizeof (rdma_mod_t), KM_SLEEP);
	*m->r_mod = *mod;
	m->r_next = NULL;
	m->r_mod->rdma_api = kmem_zalloc(KNC_STRSIZE, KM_SLEEP);
	(void) strncpy(m->r_mod->rdma_api, mod->rdma_api, KNC_STRSIZE);
	m->r_mod->rdma_api[KNC_STRSIZE - 1] = '\0';
	*mp = m;
	rw_exit(&rdma_lock);

	return (RDMA_SUCCESS);
}

/*
 * RDMATF module unregistration routine.
 * This routine is expected to be called by the fini routine in
 * the plugin modules.
 */
rdma_stat
rdma_unregister_mod(rdma_mod_t *mod)
{
	rdma_registry_t **m, *mmod = NULL;

	rw_enter(&rdma_lock, RW_WRITER);

	m = &rdma_mod_head;
	while (*m != NULL) {
		if (strncmp((*m)->r_mod->rdma_api, mod->rdma_api,
		    KNC_STRSIZE) != 0) {
			m = &((*m)->r_next);
			continue;
		}
		/*
		 * Check if any device attached, if so return error
		 */
		if ((*m)->r_mod->rdma_count != 0) {
			rw_exit(&rdma_lock);
			return (RDMA_FAILED);
		}
		/*
		 * Found entry. Now remove it.
		 */
		mmod = *m;
		*m = (*m)->r_next;
		kmem_free(mmod->r_mod->rdma_api, KNC_STRSIZE);
		kmem_free(mmod->r_mod, sizeof (rdma_mod_t));
		kmem_free(mmod, sizeof (rdma_registry_t));
		rw_exit(&rdma_lock);
		return (RDMA_SUCCESS);
	}

	/*
	 * Not found.
	 */
	rw_exit(&rdma_lock);
	return (RDMA_FAILED);
}

/*
 * Creates a new chunk list entry, and
 * adds it to the end of a chunk list.
 */
void
clist_add(struct clist **clp, uint32_t xdroff, int len,
	struct mrc *shandle, caddr_t saddr,
	struct mrc *dhandle, caddr_t daddr)
{
	struct clist *cl;

	/* Find the end of the list */

	while (*clp != NULL)
		clp = &((*clp)->c_next);

	cl = kmem_zalloc(sizeof (*cl), KM_SLEEP);
	cl->c_xdroff = xdroff;
	cl->c_len = len;
	cl->c_saddr = (uint64_t)(uintptr_t)saddr;
	if (shandle)
		cl->c_smemhandle = *shandle;
	cl->c_daddr = (uint64_t)(uintptr_t)daddr;
	if (dhandle)
		cl->c_dmemhandle = *dhandle;
	cl->c_next = NULL;

	*clp = cl;
}

int
clist_register(CONN *conn, struct clist *cl, bool_t src)
{
	struct clist *c;
	int status;

	for (c = cl; c; c = c->c_next) {
		if (src) {
			status = RDMA_REGMEMSYNC(conn,
			    (caddr_t)(uintptr_t)c->c_saddr, c->c_len,
			    &c->c_smemhandle, (void **)&c->c_ssynchandle);
		} else {
			status = RDMA_REGMEMSYNC(conn,
			    (caddr_t)(uintptr_t)c->c_daddr, c->c_len,
			    &c->c_dmemhandle, (void **)&c->c_dsynchandle);
		}
		if (status != RDMA_SUCCESS) {
			(void) clist_deregister(conn, cl, src);
			return (status);
		}
	}

	return (RDMA_SUCCESS);
}

int
clist_deregister(CONN *conn, struct clist *cl, bool_t src)
{
	struct clist *c;

	for (c = cl; c; c = c->c_next) {
		if (src) {
			if (c->c_smemhandle.mrc_rmr != 0) {
				(void) RDMA_DEREGMEMSYNC(conn,
				    (caddr_t)(uintptr_t)c->c_saddr,
				    c->c_smemhandle,
				    (void *)(uintptr_t)c->c_ssynchandle);
				c->c_smemhandle.mrc_rmr = 0;
				c->c_ssynchandle = NULL;
			}
		} else {
			if (c->c_dmemhandle.mrc_rmr != 0) {
				(void) RDMA_DEREGMEMSYNC(conn,
				    (caddr_t)(uintptr_t)c->c_daddr,
				    c->c_dmemhandle,
				    (void *)(uintptr_t)c->c_dsynchandle);
				c->c_dmemhandle.mrc_rmr = 0;
				c->c_dsynchandle = NULL;
			}
		}
	}

	return (RDMA_SUCCESS);
}

/*
 * Frees up entries in chunk list
 */
void
clist_free(struct clist *cl)
{
	struct clist *c = cl;

	while (c != NULL) {
		cl = cl->c_next;
		kmem_free(c, sizeof (struct clist));
		c = cl;
	}
}

rdma_stat
rdma_clnt_postrecv(CONN *conn, uint32_t xid)
{
	struct clist *cl = NULL;
	rdma_stat retval;
	rdma_buf_t rbuf;

	rbuf.type = RECV_BUFFER;
	if (RDMA_BUF_ALLOC(conn, &rbuf)) {
		retval = RDMA_NORESOURCE;
	} else {
		clist_add(&cl, 0, rbuf.len, &rbuf.handle, rbuf.addr,
			NULL, NULL);
		retval = RDMA_CLNT_RECVBUF(conn, cl, xid);
		clist_free(cl);
	}
	return (retval);
}

rdma_stat
rdma_svc_postrecv(CONN *conn)
{
	struct clist *cl = NULL;
	rdma_stat retval;
	rdma_buf_t rbuf;

	rbuf.type = RECV_BUFFER;
	if (RDMA_BUF_ALLOC(conn, &rbuf)) {
		retval = RDMA_NORESOURCE;
	} else {
		clist_add(&cl, 0, rbuf.len, &rbuf.handle, rbuf.addr,
			NULL, NULL);
		retval = RDMA_SVC_RECVBUF(conn, cl);
		clist_free(cl);
	}
	return (retval);
}

rdma_stat
clist_syncmem(CONN *conn, struct clist *cl, bool_t src)
{
	struct clist *c;
	rdma_stat status;

	c = cl;
	if (src) {
		while (c != NULL) {
			status = RDMA_SYNCMEM(conn,
			    (void *)(uintptr_t)c->c_ssynchandle,
			    (caddr_t)(uintptr_t)c->c_saddr, c->c_len, 0);
			if (status != RDMA_SUCCESS)
				return (status);
			c = c->c_next;
		}
	} else {
		while (c != NULL) {
			status = RDMA_SYNCMEM(conn,
			    (void *)(uintptr_t)c->c_dsynchandle,
			    (caddr_t)(uintptr_t)c->c_daddr, c->c_len, 1);
			if (status != RDMA_SUCCESS)
				return (status);
			c = c->c_next;
		}
	}
	return (RDMA_SUCCESS);
}

void
rdma_buf_free(CONN *conn, rdma_buf_t *rbuf)
{
	if (!rbuf || rbuf->addr == NULL) {
		return;
	}
	if (rbuf->type != CHUNK_BUFFER) {
		/* pool buffer */
		RDMA_BUF_FREE(conn, rbuf);
	} else {
		kmem_free(rbuf->addr, rbuf->len);
	}
	rbuf->addr = NULL;
	rbuf->len = 0;
}

/*
 * Caller is holding rdma_modload_lock mutex
 */
int
rdma_modload()
{
	int status;
	ASSERT(MUTEX_HELD(&rdma_modload_lock));
	/*
	 * Load all available RDMA plugins which right now is only IB plugin.
	 * If no IB hardware is present, then quit right away.
	 * ENODEV -- For no device on the system
	 * EPROTONOSUPPORT -- For module not avilable either due to failure to
	 * load or some other reason.
	 */
	rdma_modloaded = 1;
	if (ibt_hw_is_present() == 0) {
		rdma_dev_available = 0;
		return (ENODEV);
	}

	rdma_dev_available = 1;
	if (rpcmod_li == NULL)
		return (EPROTONOSUPPORT);

	status = ldi_open_by_name("/devices/ib/rpcib@0:rpcib",
	    FREAD | FWRITE, kcred,
	    &rpcib_handle, rpcmod_li);
	if (status != 0)
		return (EPROTONOSUPPORT);

	/* success */
	rdma_kstat_init();
	return (0);
}

void
rdma_kstat_init(void)
{
	kstat_t *ksp;

	/*
	 * The RDMA framework doesn't know how to deal with Zones, and is
	 * only available in the global zone.
	 */
	ASSERT(INGLOBALZONE(curproc));
	ksp = kstat_create_zone("unix", 0, "rpc_rdma_client", "rpc",
	    KSTAT_TYPE_NAMED, rdmarcstat_ndata,
	    KSTAT_FLAG_VIRTUAL | KSTAT_FLAG_WRITABLE, GLOBAL_ZONEID);
	if (ksp) {
		ksp->ks_data = (void *) rdmarcstat_ptr;
		kstat_install(ksp);
	}

	ksp = kstat_create_zone("unix", 0, "rpc_rdma_server", "rpc",
	    KSTAT_TYPE_NAMED, rdmarsstat_ndata,
	    KSTAT_FLAG_VIRTUAL | KSTAT_FLAG_WRITABLE, GLOBAL_ZONEID);
	if (ksp) {
		ksp->ks_data = (void *) rdmarsstat_ptr;
		kstat_install(ksp);
	}
}
