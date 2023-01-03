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
 * Server side RPC handler.
 */

#include <sys/byteorder.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>

#include <smbsrv/libsmb.h>
#include <smbsrv/libmlrpc.h>
#include <smbsrv/mlsvc.h>
#include <smbsrv/ndr.h>
#include <smbsrv/mlrpc.h>
#include <smbsrv/mlsvc_util.h>


#define	SMB_CTXT_BUFSZ		65536

/*
 * Fragment size (5680: NT style).
 */
#define	MLRPC_FRAG_SZ		5680
static unsigned long mlrpc_frag_size = MLRPC_FRAG_SZ;

/*
 * Service context table.
 */
#define	CTXT_TABLE_ENTRIES	128
static struct mlsvc_rpc_context context_table[CTXT_TABLE_ENTRIES];
static mutex_t mlrpc_context_lock;

static int ndr_s_transact(struct mlsvc_rpc_context *);
static struct mlsvc_rpc_context *ndr_s_lookup(int);
static void ndr_s_release(struct mlsvc_rpc_context *);
static struct mlsvc_rpc_context *ndr_s_allocate(int);
static void ndr_s_deallocate(struct mlsvc_rpc_context *);
static void ndr_s_rewind(struct mlsvc_rpc_context *);
static void ndr_s_flush(struct mlsvc_rpc_context *);

static int mlrpc_s_process(struct mlrpc_xaction *);
static int mlrpc_s_bind(struct mlrpc_xaction *);
static int mlrpc_s_request(struct mlrpc_xaction *);
static void mlrpc_reply_prepare_hdr(struct mlrpc_xaction *);
static int mlrpc_s_alter_context(struct mlrpc_xaction *);
static void mlrpc_reply_bind_ack(struct mlrpc_xaction *);
static void mlrpc_reply_fault(struct mlrpc_xaction *, unsigned long);
static int mlrpc_build_reply(struct mlrpc_xaction *);
static void mlrpc_build_frag(struct mlndr_stream *, uint8_t *, uint32_t);

/*
 * Allocate and associate a service context with a fid.
 */
int
ndr_s_open(int fid, uint8_t *data, uint32_t datalen)
{
	struct mlsvc_rpc_context *svc;

	(void) mutex_lock(&mlrpc_context_lock);

	if ((svc = ndr_s_lookup(fid)) != NULL) {
		ndr_s_release(svc);
		(void) mutex_unlock(&mlrpc_context_lock);
		return (EEXIST);
	}

	if ((svc = ndr_s_allocate(fid)) == NULL) {
		(void) mutex_unlock(&mlrpc_context_lock);
		return (ENOMEM);
	}

	if (smb_opipe_context_decode(&svc->svc_ctx, data, datalen) == -1) {
		ndr_s_release(svc);
		(void) mutex_unlock(&mlrpc_context_lock);
		return (EINVAL);
	}

	mlrpc_binding_pool_initialize(&svc->binding, svc->binding_pool,
	    CTXT_N_BINDING_POOL);

	(void) mutex_unlock(&mlrpc_context_lock);
	return (0);
}

/*
 * Release the context associated with a fid when an opipe is closed.
 */
int
ndr_s_close(int fid)
{
	struct mlsvc_rpc_context *svc;

	(void) mutex_lock(&mlrpc_context_lock);

	if ((svc = ndr_s_lookup(fid)) == NULL) {
		(void) mutex_unlock(&mlrpc_context_lock);
		return (ENOENT);
	}

	/*
	 * Release twice: once for the lookup above
	 * and again to close the fid.
	 */
	ndr_s_release(svc);
	ndr_s_release(svc);
	(void) mutex_unlock(&mlrpc_context_lock);
	return (0);
}

/*
 * Write RPC request data to the input stream.  Input data is buffered
 * until the response is requested.
 */
int
ndr_s_write(int fid, uint8_t *buf, uint32_t len)
{
	struct mlsvc_rpc_context *svc;
	ssize_t nbytes;

	if (len == 0)
		return (0);

	(void) mutex_lock(&mlrpc_context_lock);

	if ((svc = ndr_s_lookup(fid)) == NULL) {
		(void) mutex_unlock(&mlrpc_context_lock);
		return (ENOENT);
	}

	nbytes = ndr_uiomove((caddr_t)buf, len, UIO_READ, &svc->in_uio);

	ndr_s_release(svc);
	(void) mutex_unlock(&mlrpc_context_lock);
	return ((nbytes == len) ? 0 : EIO);
}

/*
 * Read RPC response data.  If the input stream contains an RPC request,
 * we need to process the RPC transaction, which will place the RPC
 * response in the output (frags) stream.  Otherwise, read data from
 * the output stream.
 */
int
ndr_s_read(int fid, uint8_t *buf, uint32_t *len, uint32_t *resid)
{
	struct mlsvc_rpc_context *svc;
	ssize_t nbytes = *len;
	int rc;

	if (nbytes == 0) {
		*resid = 0;
		return (0);
	}

	(void) mutex_lock(&mlrpc_context_lock);
	if ((svc = ndr_s_lookup(fid)) == NULL) {
		(void) mutex_unlock(&mlrpc_context_lock);
		return (ENOENT);
	}
	(void) mutex_unlock(&mlrpc_context_lock);

	if (svc->in_uio.uio_offset) {
		if ((rc = ndr_s_transact(svc)) != 0) {
			ndr_s_flush(svc);
			(void) mutex_lock(&mlrpc_context_lock);
			ndr_s_release(svc);
			(void) mutex_unlock(&mlrpc_context_lock);
			return (rc);
		}

	}

	*len = ndr_uiomove((caddr_t)buf, nbytes, UIO_WRITE, &svc->frags.uio);
	*resid = svc->frags.uio.uio_resid;

	if (*resid == 0) {
		/*
		 * Nothing left, cleanup the output stream.
		 */
		ndr_s_flush(svc);
	}

	(void) mutex_lock(&mlrpc_context_lock);
	ndr_s_release(svc);
	(void) mutex_unlock(&mlrpc_context_lock);
	return (0);
}

/*
 * Process a server-side RPC request.
 */
static int
ndr_s_transact(struct mlsvc_rpc_context *svc)
{
	ndr_xa_t			*mxa;
	struct mlndr_stream		*recv_mlnds;
	struct mlndr_stream		*send_mlnds;
	char				*data;
	int				datalen;

	data = svc->in_buf;
	datalen = svc->in_uio.uio_offset;

	if ((mxa = (ndr_xa_t *)malloc(sizeof (ndr_xa_t))) == NULL)
		return (ENOMEM);

	bzero(mxa, sizeof (struct mlrpc_xaction));
	mxa->fid = svc->fid;
	mxa->context = svc;
	mxa->binding_list = svc->binding;

	if ((mxa->heap = mlrpc_heap_create()) == NULL) {
		free(mxa);
		return (ENOMEM);
	}

	recv_mlnds = &mxa->recv_mlnds;
	mlnds_initialize(recv_mlnds, datalen, NDR_MODE_CALL_RECV, mxa->heap);

	/*
	 * Copy the input data and reset the input stream.
	 */
	bcopy(data, recv_mlnds->pdu_base_addr, datalen);
	ndr_s_rewind(svc);

	send_mlnds = &mxa->send_mlnds;
	mlnds_initialize(send_mlnds, 0, NDR_MODE_RETURN_SEND, mxa->heap);

	(void) mlrpc_s_process(mxa);

	mlnds_finalize(send_mlnds, &svc->frags);
	mlnds_destruct(&mxa->recv_mlnds);
	mlnds_destruct(&mxa->send_mlnds);
	mlrpc_heap_destroy(mxa->heap);
	free(mxa);
	return (0);
}

/*
 * Must be called with mlrpc_context_lock held.
 */
static struct mlsvc_rpc_context *
ndr_s_lookup(int fid)
{
	struct mlsvc_rpc_context *svc;
	int i;

	for (i = 0; i < CTXT_TABLE_ENTRIES; ++i) {
		svc = &context_table[i];

		if (svc->fid == fid) {
			if (svc->refcnt == 0)
				return (NULL);

			svc->refcnt++;
			return (svc);
		}
	}

	return (NULL);
}

/*
 * Must be called with mlrpc_context_lock held.
 */
static void
ndr_s_release(struct mlsvc_rpc_context *svc)
{
	svc->refcnt--;
	ndr_s_deallocate(svc);
}

/*
 * Must be called with mlrpc_context_lock held.
 */
static struct mlsvc_rpc_context *
ndr_s_allocate(int fid)
{
	struct mlsvc_rpc_context *svc = NULL;
	int i;

	for (i = 0; i < CTXT_TABLE_ENTRIES; ++i) {
		svc = &context_table[i];

		if (svc->fid == 0) {
			bzero(svc, sizeof (struct mlsvc_rpc_context));

			if ((svc->in_buf = malloc(SMB_CTXT_BUFSZ)) == NULL)
				return (NULL);

			ndr_s_rewind(svc);
			svc->fid = fid;
			svc->refcnt = 1;
			return (svc);
		}
	}

	return (NULL);
}

/*
 * Must be called with mlrpc_context_lock held.
 */
static void
ndr_s_deallocate(struct mlsvc_rpc_context *svc)
{
	if (svc->refcnt == 0) {
		/*
		 * Ensure that there are no RPC service policy handles
		 * (associated with this fid) left around.
		 */
		ndr_hdclose(svc->fid);

		ndr_s_rewind(svc);
		ndr_s_flush(svc);
		free(svc->in_buf);
		free(svc->svc_ctx.oc_domain);
		free(svc->svc_ctx.oc_account);
		free(svc->svc_ctx.oc_workstation);
		bzero(svc, sizeof (struct mlsvc_rpc_context));
	}
}

/*
 * Rewind the input data stream, ready for the next write.
 */
static void
ndr_s_rewind(struct mlsvc_rpc_context *svc)
{
	svc->in_uio.uio_iov = &svc->in_iov;
	svc->in_uio.uio_iovcnt = 1;
	svc->in_uio.uio_offset = 0;
	svc->in_uio.uio_segflg = UIO_USERSPACE;
	svc->in_uio.uio_resid = SMB_CTXT_BUFSZ;
	svc->in_iov.iov_base = svc->in_buf;
	svc->in_iov.iov_len = SMB_CTXT_BUFSZ;
}

/*
 * Flush the output data stream.
 */
static void
ndr_s_flush(struct mlsvc_rpc_context *svc)
{
	ndr_frag_t *frag;

	while ((frag = svc->frags.head) != NULL) {
		svc->frags.head = frag->next;
		free(frag);
	}

	free(svc->frags.iov);
	bzero(&svc->frags, sizeof (ndr_fraglist_t));
}

/*
 * Check whether or not the specified user has administrator privileges,
 * i.e. is a member of Domain Admins or Administrators.
 * Returns true if the user is an administrator, otherwise returns false.
 */
boolean_t
ndr_is_admin(ndr_xa_t *xa)
{
	smb_opipe_context_t *svc = &xa->context->svc_ctx;

	return (svc->oc_flags & SMB_ATF_ADMIN);
}

/*
 * Check whether or not the specified user has power-user privileges,
 * i.e. is a member of Domain Admins, Administrators or Power Users.
 * This is typically required for operations such as managing shares.
 * Returns true if the user is a power user, otherwise returns false.
 */
boolean_t
ndr_is_poweruser(ndr_xa_t *xa)
{
	smb_opipe_context_t *svc = &xa->context->svc_ctx;

	return ((svc->oc_flags & SMB_ATF_ADMIN) ||
	    (svc->oc_flags & SMB_ATF_POWERUSER));
}

int32_t
ndr_native_os(ndr_xa_t *xa)
{
	smb_opipe_context_t *svc = &xa->context->svc_ctx;

	return (svc->oc_native_os);
}

/*
 * This is the entry point for all server-side RPC processing.
 * It is assumed that the PDU has already been received.
 */
static int
mlrpc_s_process(struct mlrpc_xaction *mxa)
{
	int rc;

	rc = mlrpc_decode_pdu_hdr(mxa);
	if (!MLRPC_DRC_IS_OK(rc))
		return (-1);

	(void) mlrpc_reply_prepare_hdr(mxa);

	switch (mxa->ptype) {
	case MLRPC_PTYPE_BIND:
		rc = mlrpc_s_bind(mxa);
		break;

	case MLRPC_PTYPE_REQUEST:
		rc = mlrpc_s_request(mxa);
		break;

	case MLRPC_PTYPE_ALTER_CONTEXT:
		rc = mlrpc_s_alter_context(mxa);
		break;

	default:
		rc = MLRPC_DRC_FAULT_RPCHDR_PTYPE_INVALID;
		break;
	}

	if (MLRPC_DRC_IS_FAULT(rc))
		mlrpc_reply_fault(mxa, rc);

	(void) mlrpc_build_reply(mxa);
	return (rc);
}

/*
 * Multiple p_cont_elem[]s, multiple transfer_syntaxes[] and multiple
 * p_results[] not supported.
 */
static int
mlrpc_s_bind(struct mlrpc_xaction *mxa)
{
	mlrpc_p_cont_list_t	*cont_list;
	mlrpc_p_result_list_t	*result_list;
	mlrpc_p_result_t	*result;
	unsigned		p_cont_id;
	struct mlrpc_binding	*mbind;
	ndr_uuid_t		*as_uuid;
	ndr_uuid_t		*ts_uuid;
	char			as_buf[64];
	char			ts_buf[64];
	int			as_vers;
	int			ts_vers;
	struct mlndr_stream	*send_mlnds;
	struct mlrpc_service	*msvc;
	int			rc;
	mlrpc_port_any_t	*sec_addr;

	/* acquire targets */
	cont_list = &mxa->recv_hdr.bind_hdr.p_context_elem;
	result_list = &mxa->send_hdr.bind_ack_hdr.p_result_list;
	result = &result_list->p_results[0];

	/*
	 * Set up temporary secondary address port.
	 * We will correct this later (below).
	 */
	send_mlnds = &mxa->send_mlnds;
	sec_addr = &mxa->send_hdr.bind_ack_hdr.sec_addr;
	sec_addr->length = 13;
	(void) strcpy((char *)sec_addr->port_spec, "\\PIPE\\ntsvcs");

	result_list->n_results = 1;
	result_list->reserved = 0;
	result_list->reserved2 = 0;
	result->result = MLRPC_PCDR_ACCEPTANCE;
	result->reason = 0;
	bzero(&result->transfer_syntax, sizeof (result->transfer_syntax));

	/* sanity check */
	if (cont_list->n_context_elem != 1 ||
	    cont_list->p_cont_elem[0].n_transfer_syn != 1) {
		mlndo_trace("mlrpc_s_bind: warning: multiple p_cont_elem");
	}

	p_cont_id = cont_list->p_cont_elem[0].p_cont_id;

	if ((mbind = mlrpc_find_binding(mxa, p_cont_id)) != NULL) {
		/*
		 * Duplicate p_cont_id.
		 * Send a bind_ack with a better error.
		 */
		mlndo_trace("mlrpc_s_bind: duplicate binding");
		return (MLRPC_DRC_FAULT_BIND_PCONT_BUSY);
	}

	if ((mbind = mlrpc_new_binding(mxa)) == NULL) {
		/*
		 * No free binding slot
		 */
		result->result = MLRPC_PCDR_PROVIDER_REJECTION;
		result->reason = MLRPC_PPR_LOCAL_LIMIT_EXCEEDED;
		mlndo_trace("mlrpc_s_bind: no resources");
		return (MLRPC_DRC_OK);
	}

	as_uuid = &cont_list->p_cont_elem[0].abstract_syntax.if_uuid;
	as_vers = cont_list->p_cont_elem[0].abstract_syntax.if_version;

	ts_uuid = &cont_list->p_cont_elem[0].transfer_syntaxes[0].if_uuid;
	ts_vers = cont_list->p_cont_elem[0].transfer_syntaxes[0].if_version;

	msvc = mlrpc_find_service_by_uuids(as_uuid, as_vers, ts_uuid, ts_vers);
	if (!msvc) {
		mlrpc_uuid_to_str(as_uuid, as_buf);
		mlrpc_uuid_to_str(ts_uuid, ts_buf);

		mlndo_printf(send_mlnds, 0, "mlrpc_s_bind: unknown service");
		mlndo_printf(send_mlnds, 0, "abs=%s v%d, xfer=%s v%d",
		    as_buf, as_vers, ts_buf, ts_vers);

		result->result = MLRPC_PCDR_PROVIDER_REJECTION;
		result->reason = MLRPC_PPR_ABSTRACT_SYNTAX_NOT_SUPPORTED;
		return (MLRPC_DRC_OK);
	}

	/*
	 * We can now use the correct secondary address port.
	 */
	sec_addr = &mxa->send_hdr.bind_ack_hdr.sec_addr;
	sec_addr->length = strlen(msvc->sec_addr_port) + 1;
	(void) strlcpy((char *)sec_addr->port_spec, msvc->sec_addr_port,
	    MLRPC_PORT_ANY_MAX_PORT_SPEC);

	mbind->p_cont_id = p_cont_id;
	mbind->which_side = MLRPC_BIND_SIDE_SERVER;
	/* mbind->context set by app */
	mbind->service = msvc;
	mbind->instance_specific = 0;

	mxa->binding = mbind;

	if (msvc->bind_req) {
		/*
		 * Call the service-specific bind() handler.  If
		 * this fails, we shouild send a specific error
		 * on the bind ack.
		 */
		rc = (msvc->bind_req)(mxa);
		if (MLRPC_DRC_IS_FAULT(rc)) {
			mbind->service = 0;	/* free binding slot */
			mbind->which_side = 0;
			mbind->p_cont_id = 0;
			mbind->instance_specific = 0;
			return (rc);
		}
	}

	result->transfer_syntax =
	    cont_list->p_cont_elem[0].transfer_syntaxes[0];

	/*
	 * Special rejection of Windows 2000 DSSETUP interface.
	 * This interface was introduced in Windows 2000 but has
	 * been subsequently deprecated due to problems.
	 */
	if (strcmp(msvc->name, "DSSETUP") == 0) {
		result->result = MLRPC_PCDR_PROVIDER_REJECTION;
		result->reason = MLRPC_PPR_ABSTRACT_SYNTAX_NOT_SUPPORTED;
	}

	return (MLRPC_DRC_BINDING_MADE);
}

/*
 * mlrpc_s_alter_context
 *
 * The alter context request is used to request additional presentation
 * context for another interface and/or version. It's very similar to a
 * bind request.
 *
 * We don't fully support multiple contexts so, for now, we reject this
 * request.  Windows 2000 clients attempt to use an alternate LSA context
 * when ACLs are modified.
 */
static int
mlrpc_s_alter_context(struct mlrpc_xaction *mxa)
{
	mlrpc_p_result_list_t *result_list;
	mlrpc_p_result_t *result;
	mlrpc_p_cont_list_t *cont_list;
	struct mlrpc_binding *mbind;
	struct mlrpc_service *msvc;
	unsigned p_cont_id;
	ndr_uuid_t *as_uuid;
	ndr_uuid_t *ts_uuid;
	int as_vers;
	int ts_vers;
	mlrpc_port_any_t *sec_addr;

	result_list = &mxa->send_hdr.bind_ack_hdr.p_result_list;
	result_list->n_results = 1;
	result_list->reserved = 0;
	result_list->reserved2 = 0;

	result = &result_list->p_results[0];
	result->result = MLRPC_PCDR_ACCEPTANCE;
	result->reason = 0;
	bzero(&result->transfer_syntax, sizeof (result->transfer_syntax));

	if (mxa != NULL) {
		result->result = MLRPC_PCDR_PROVIDER_REJECTION;
		result->reason = MLRPC_PPR_ABSTRACT_SYNTAX_NOT_SUPPORTED;
		return (MLRPC_DRC_OK);
	}

	cont_list = &mxa->recv_hdr.bind_hdr.p_context_elem;
	p_cont_id = cont_list->p_cont_elem[0].p_cont_id;

	if ((mbind = mlrpc_find_binding(mxa, p_cont_id)) != NULL)
		return (MLRPC_DRC_FAULT_BIND_PCONT_BUSY);

	if ((mbind = mlrpc_new_binding(mxa)) == NULL) {
		result->result = MLRPC_PCDR_PROVIDER_REJECTION;
		result->reason = MLRPC_PPR_LOCAL_LIMIT_EXCEEDED;
		return (MLRPC_DRC_OK);
	}

	as_uuid = &cont_list->p_cont_elem[0].abstract_syntax.if_uuid;
	as_vers = cont_list->p_cont_elem[0].abstract_syntax.if_version;

	ts_uuid = &cont_list->p_cont_elem[0].transfer_syntaxes[0].if_uuid;
	ts_vers = cont_list->p_cont_elem[0].transfer_syntaxes[0].if_version;

	msvc = mlrpc_find_service_by_uuids(as_uuid, as_vers, ts_uuid, ts_vers);
	if (msvc == 0) {
		result->result = MLRPC_PCDR_PROVIDER_REJECTION;
		result->reason = MLRPC_PPR_ABSTRACT_SYNTAX_NOT_SUPPORTED;
		return (MLRPC_DRC_OK);
	}

	mbind->p_cont_id = p_cont_id;
	mbind->which_side = MLRPC_BIND_SIDE_SERVER;
	/* mbind->context set by app */
	mbind->service = msvc;
	mbind->instance_specific = 0;
	mxa->binding = mbind;

	sec_addr = &mxa->send_hdr.bind_ack_hdr.sec_addr;
	sec_addr->length = 0;
	bzero(sec_addr->port_spec, MLRPC_PORT_ANY_MAX_PORT_SPEC);

	result->transfer_syntax =
	    cont_list->p_cont_elem[0].transfer_syntaxes[0];

	return (MLRPC_DRC_BINDING_MADE);
}

static int
mlrpc_s_request(struct mlrpc_xaction *mxa)
{
	struct mlrpc_binding	*mbind;
	struct mlrpc_service	*msvc;
	unsigned		p_cont_id;
	int			rc;

	mxa->opnum = mxa->recv_hdr.request_hdr.opnum;
	p_cont_id = mxa->recv_hdr.request_hdr.p_cont_id;

	if ((mbind = mlrpc_find_binding(mxa, p_cont_id)) == NULL)
		return (MLRPC_DRC_FAULT_REQUEST_PCONT_INVALID);

	mxa->binding = mbind;
	msvc = mbind->service;

	/*
	 * Make room for the response hdr.
	 */
	mxa->send_mlnds.pdu_scan_offset = MLRPC_RSP_HDR_SIZE;

	if (msvc->call_stub)
		rc = (*msvc->call_stub)(mxa);
	else
		rc = mlrpc_generic_call_stub(mxa);

	if (MLRPC_DRC_IS_FAULT(rc)) {
		mlndo_printf(0, 0, "%s[0x%02x]: 0x%04x",
		    msvc->name, mxa->opnum, rc);
	}

	return (rc);
}

/*
 * The transaction and the two mlnds streams use the same heap, which
 * should already exist at this point.  The heap will also be available
 * to the stub.
 */
int
mlrpc_generic_call_stub(struct mlrpc_xaction *mxa)
{
	struct mlrpc_binding 	*mbind = mxa->binding;
	struct mlrpc_service 	*msvc = mbind->service;
	struct ndr_typeinfo 	*intf_ti = msvc->interface_ti;
	struct mlrpc_stub_table *ste;
	int			opnum = mxa->opnum;
	unsigned		p_len = intf_ti->c_size_fixed_part;
	char 			*param;
	int			rc;

	if (mxa->heap == NULL) {
		mlndo_printf(0, 0, "%s[0x%02x]: no heap", msvc->name, opnum);
		return (MLRPC_DRC_FAULT_OUT_OF_MEMORY);
	}

	if ((ste = mlrpc_find_stub_in_svc(msvc, opnum)) == NULL) {
		mlndo_printf(0, 0, "%s[0x%02x]: invalid opnum",
		    msvc->name, opnum);
		return (MLRPC_DRC_FAULT_REQUEST_OPNUM_INVALID);
	}

	if ((param = mlrpc_heap_malloc(mxa->heap, p_len)) == NULL)
		return (MLRPC_DRC_FAULT_OUT_OF_MEMORY);

	bzero(param, p_len);

	rc = mlrpc_decode_call(mxa, param);
	if (!MLRPC_DRC_IS_OK(rc))
		return (rc);

	rc = (*ste->func)(param, mxa);
	if (rc == MLRPC_DRC_OK)
		rc = mlrpc_encode_return(mxa, param);

	return (rc);
}

/*
 * We can perform some initial setup of the response header here.
 * We also need to cache some of the information from the bind
 * negotiation for use during subsequent RPC calls.
 */
static void
mlrpc_reply_prepare_hdr(struct mlrpc_xaction *mxa)
{
	mlrpcconn_common_header_t *rhdr = &mxa->recv_hdr.common_hdr;
	mlrpcconn_common_header_t *hdr = &mxa->send_hdr.common_hdr;

	hdr->rpc_vers = 5;
	hdr->rpc_vers_minor = 0;
	hdr->pfc_flags = MLRPC_PFC_FIRST_FRAG + MLRPC_PFC_LAST_FRAG;
	hdr->packed_drep = rhdr->packed_drep;
	hdr->frag_length = 0;
	hdr->auth_length = 0;
	hdr->call_id = rhdr->call_id;
#ifdef _BIG_ENDIAN
	hdr->packed_drep.intg_char_rep = MLRPC_REPLAB_CHAR_ASCII
	    | MLRPC_REPLAB_INTG_BIG_ENDIAN;
#else
	hdr->packed_drep.intg_char_rep = MLRPC_REPLAB_CHAR_ASCII
	    | MLRPC_REPLAB_INTG_LITTLE_ENDIAN;
#endif

	switch (mxa->ptype) {
	case MLRPC_PTYPE_BIND:
		hdr->ptype = MLRPC_PTYPE_BIND_ACK;
		mxa->send_hdr.bind_ack_hdr.max_xmit_frag =
		    mxa->recv_hdr.bind_hdr.max_xmit_frag;
		mxa->send_hdr.bind_ack_hdr.max_recv_frag =
		    mxa->recv_hdr.bind_hdr.max_recv_frag;
		mxa->send_hdr.bind_ack_hdr.assoc_group_id =
		    mxa->recv_hdr.bind_hdr.assoc_group_id;

		if (mxa->send_hdr.bind_ack_hdr.assoc_group_id == 0)
			mxa->send_hdr.bind_ack_hdr.assoc_group_id = time(0);

		/*
		 * Save the maximum fragment sizes
		 * for use with subsequent requests.
		 */
		mxa->context->max_xmit_frag =
		    mxa->recv_hdr.bind_hdr.max_xmit_frag;

		mxa->context->max_recv_frag =
		    mxa->recv_hdr.bind_hdr.max_recv_frag;

		break;

	case MLRPC_PTYPE_REQUEST:
		hdr->ptype = MLRPC_PTYPE_RESPONSE;
		/* mxa->send_hdr.response_hdr.alloc_hint */
		mxa->send_hdr.response_hdr.p_cont_id =
		    mxa->recv_hdr.request_hdr.p_cont_id;
		mxa->send_hdr.response_hdr.cancel_count = 0;
		mxa->send_hdr.response_hdr.reserved = 0;
		break;

	case MLRPC_PTYPE_ALTER_CONTEXT:
		hdr->ptype = MLRPC_PTYPE_ALTER_CONTEXT_RESP;
		/*
		 * The max_xmit_frag, max_recv_frag
		 * and assoc_group_id are ignored.
		 */
		break;

	default:
		hdr->ptype = 0xFF;
	}
}

/*
 * Finish and encode the bind acknowledge (MLRPC_PTYPE_BIND_ACK) header.
 * The frag_length is different from a regular RPC response.
 */
static void
mlrpc_reply_bind_ack(struct mlrpc_xaction *mxa)
{
	mlrpcconn_common_header_t	*hdr;
	mlrpcconn_bind_ack_hdr_t	*bahdr;

	hdr = &mxa->send_hdr.common_hdr;
	bahdr = &mxa->send_hdr.bind_ack_hdr;
	hdr->frag_length = mlrpc_bind_ack_hdr_size(bahdr);
}

/*
 * Signal an RPC fault. The stream is reset and we overwrite whatever
 * was in the response header with the fault information.
 */
static void
mlrpc_reply_fault(struct mlrpc_xaction *mxa, unsigned long drc)
{
	mlrpcconn_common_header_t *rhdr = &mxa->recv_hdr.common_hdr;
	mlrpcconn_common_header_t *hdr = &mxa->send_hdr.common_hdr;
	struct mlndr_stream *mlnds = &mxa->send_mlnds;
	unsigned long fault_status;

	MLNDS_RESET(mlnds);

	hdr->rpc_vers = 5;
	hdr->rpc_vers_minor = 0;
	hdr->pfc_flags = MLRPC_PFC_FIRST_FRAG + MLRPC_PFC_LAST_FRAG;
	hdr->packed_drep = rhdr->packed_drep;
	hdr->frag_length = sizeof (mxa->send_hdr.fault_hdr);
	hdr->auth_length = 0;
	hdr->call_id = rhdr->call_id;
#ifdef _BIG_ENDIAN
	hdr->packed_drep.intg_char_rep = MLRPC_REPLAB_CHAR_ASCII
	    | MLRPC_REPLAB_INTG_BIG_ENDIAN;
#else
	hdr->packed_drep.intg_char_rep = MLRPC_REPLAB_CHAR_ASCII
	    | MLRPC_REPLAB_INTG_LITTLE_ENDIAN;
#endif

	switch (drc & MLRPC_DRC_MASK_SPECIFIER) {
	case MLRPC_DRC_FAULT_OUT_OF_MEMORY:
	case MLRPC_DRC_FAULT_ENCODE_TOO_BIG:
		fault_status = MLRPC_FAULT_NCA_OUT_ARGS_TOO_BIG;
		break;

	case MLRPC_DRC_FAULT_REQUEST_PCONT_INVALID:
		fault_status = MLRPC_FAULT_NCA_INVALID_PRES_CONTEXT_ID;
		break;

	case MLRPC_DRC_FAULT_REQUEST_OPNUM_INVALID:
		fault_status = MLRPC_FAULT_NCA_OP_RNG_ERROR;
		break;

	case MLRPC_DRC_FAULT_DECODE_FAILED:
	case MLRPC_DRC_FAULT_ENCODE_FAILED:
		fault_status = MLRPC_FAULT_NCA_PROTO_ERROR;
		break;

	default:
		fault_status = MLRPC_FAULT_NCA_UNSPEC_REJECT;
		break;
	}

	mxa->send_hdr.fault_hdr.common_hdr.ptype = MLRPC_PTYPE_FAULT;
	mxa->send_hdr.fault_hdr.status = fault_status;
	mxa->send_hdr.response_hdr.alloc_hint = hdr->frag_length;
}

static int
mlrpc_build_reply(struct mlrpc_xaction *mxa)
{
	mlrpcconn_common_header_t *hdr = &mxa->send_hdr.common_hdr;
	struct mlndr_stream *mlnds = &mxa->send_mlnds;
	uint8_t *pdu_buf;
	unsigned long pdu_size;
	unsigned long frag_size;
	unsigned long pdu_data_size;
	unsigned long frag_data_size;

	frag_size = mlrpc_frag_size;
	pdu_size = mlnds->pdu_size;
	pdu_buf = mlnds->pdu_base_addr;

	if (pdu_size <= frag_size) {
		/*
		 * Single fragment response. The PDU size may be zero
		 * here (i.e. bind or fault response). So don't make
		 * any assumptions about it until after the header is
		 * encoded.
		 */
		switch (hdr->ptype) {
		case MLRPC_PTYPE_BIND_ACK:
			mlrpc_reply_bind_ack(mxa);
			break;

		case MLRPC_PTYPE_FAULT:
			/* already setup */
			break;

		case MLRPC_PTYPE_RESPONSE:
			hdr->frag_length = pdu_size;
			mxa->send_hdr.response_hdr.alloc_hint =
			    hdr->frag_length;
			break;

		default:
			hdr->frag_length = pdu_size;
			break;
		}

		mlnds->pdu_scan_offset = 0;
		(void) mlrpc_encode_pdu_hdr(mxa);
		pdu_size = mlnds->pdu_size;
		mlrpc_build_frag(mlnds, pdu_buf,  pdu_size);
		return (0);
	}

	/*
	 * Multiple fragment response.
	 */
	hdr->pfc_flags = MLRPC_PFC_FIRST_FRAG;
	hdr->frag_length = frag_size;
	mxa->send_hdr.response_hdr.alloc_hint = pdu_size - MLRPC_RSP_HDR_SIZE;
	mlnds->pdu_scan_offset = 0;
	(void) mlrpc_encode_pdu_hdr(mxa);
	mlrpc_build_frag(mlnds, pdu_buf,  frag_size);

	/*
	 * We need to update the 24-byte header in subsequent fragments.
	 *
	 * pdu_data_size:	total data remaining to be handled
	 * frag_size:		total fragment size including header
	 * frag_data_size:	data in fragment
	 *			(i.e. frag_size - MLRPC_RSP_HDR_SIZE)
	 */
	pdu_data_size = pdu_size - MLRPC_RSP_HDR_SIZE;
	frag_data_size = frag_size - MLRPC_RSP_HDR_SIZE;

	while (pdu_data_size) {
		mxa->send_hdr.response_hdr.alloc_hint -= frag_data_size;
		pdu_data_size -= frag_data_size;
		pdu_buf += frag_data_size;

		if (pdu_data_size <= frag_data_size) {
			frag_data_size = pdu_data_size;
			frag_size = frag_data_size + MLRPC_RSP_HDR_SIZE;
			hdr->pfc_flags = MLRPC_PFC_LAST_FRAG;
		} else {
			hdr->pfc_flags = 0;
		}

		hdr->frag_length = frag_size;
		mlnds->pdu_scan_offset = 0;
		(void) mlrpc_encode_pdu_hdr(mxa);
		bcopy(mlnds->pdu_base_addr, pdu_buf, MLRPC_RSP_HDR_SIZE);

		mlrpc_build_frag(mlnds, pdu_buf, frag_size);

		if (hdr->pfc_flags & MLRPC_PFC_LAST_FRAG)
			break;
	}

	return (0);
}

/*
 * mlrpc_build_frag
 *
 * Build an RPC PDU fragment from the specified buffer.
 * If malloc fails, the client will see a header/pdu inconsistency
 * and report an error.
 */
static void
mlrpc_build_frag(struct mlndr_stream *mlnds, uint8_t *buf, uint32_t len)
{
	ndr_frag_t *frag;
	int size = sizeof (ndr_frag_t) + len;

	if ((frag = (ndr_frag_t *)malloc(size)) == NULL)
		return;

	frag->next = NULL;
	frag->buf = (uint8_t *)frag + sizeof (ndr_frag_t);
	frag->len = len;
	bcopy(buf, frag->buf, len);

	if (mlnds->frags.head == NULL) {
		mlnds->frags.head = frag;
		mlnds->frags.tail = frag;
		mlnds->frags.nfrag = 1;
	} else {
		mlnds->frags.tail->next = frag;
		mlnds->frags.tail = frag;
		++mlnds->frags.nfrag;
	}
}
