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
 * SMB Node State Machine
 * ----------------------
 *
 *    +----------------------------+	 T0
 *    |  SMB_NODE_STATE_AVAILABLE  |<----------- Creation/Allocation
 *    +----------------------------+
 *		    |
 *		    | T1
 *		    |
 *		    v
 *    +-----------------------------+    T2
 *    |  SMB_NODE_STATE_DESTROYING  |----------> Deletion/Free
 *    +-----------------------------+
 *
 * Transition T0
 *
 *    This transition occurs in smb_node_lookup(). If the node looked for is
 *    not found in the has table a new node is created. The reference count is
 *    initialized to 1 and the state initialized to SMB_NODE_STATE_AVAILABLE.
 *
 * Transition T1
 *
 *    This transition occurs in smb_node_release(). If the reference count
 *    drops to zero the state is moved to SMB_NODE_STATE_DESTROYING and no more
 *    reference count will be given out for that node.
 *
 * Transition T2
 *
 *    This transition occurs in smb_node_release(). The structure is deleted.
 *
 * Comments
 * --------
 *
 *    The reason the smb node has 2 states is the following synchronization
 *    rule:
 *
 *    There's a mutex embedded in the node used to protect its fields and
 *    there's a lock embedded in the bucket of the hash table the node belongs
 *    to. To increment or to decrement the reference count the mutex must be
 *    entered. To insert the node into the bucket and to remove it from the
 *    bucket the lock must be entered in RW_WRITER mode. When both (mutex and
 *    lock) have to be entered, the lock has always to be entered first then
 *    the mutex. This prevents a deadlock between smb_node_lookup() and
 *    smb_node_release() from occurring. However, in smb_node_release() when the
 *    reference count drops to zero and triggers the deletion of the node, the
 *    mutex has to be released before entering the lock of the bucket (to
 *    remove the node). This creates a window during which the node that is
 *    about to be freed could be given out by smb_node_lookup(). To close that
 *    window the node is moved to the state SMB_NODE_STATE_DESTROYING before
 *    releasing the mutex. That way, even if smb_node_lookup() finds it, the
 *    state will indicate that the node should be treated as non existent (of
 *    course the state of the node should be tested/updated under the
 *    protection of the mutex).
 */
#include <smbsrv/smb_incl.h>
#include <smbsrv/smb_fsops.h>
#include <sys/pathname.h>
#include <sys/sdt.h>
#include <sys/nbmlock.h>

uint32_t smb_is_executable(char *path);
static void smb_node_delete_on_close(smb_node_t *node);

#define	VALIDATE_DIR_NODE(_dir_, _node_) \
    ASSERT((_dir_)->n_magic == SMB_NODE_MAGIC); \
    ASSERT(((_dir_)->vp->v_xattrdir) || ((_dir_)->vp->v_type == VDIR)); \
    ASSERT((_dir_)->dir_snode != (_node_));

static boolean_t	smb_node_initialized = B_FALSE;
static smb_llist_t	smb_node_hash_table[SMBND_HASH_MASK+1];

/*
 * smb_node_init
 *
 * Initialization of the SMB node layer.
 *
 * This function is not multi-thread safe. The caller must make sure only one
 * thread makes the call.
 */
int
smb_node_init(void)
{
	int	i;

	if (smb_node_initialized)
		return (0);

	for (i = 0; i <= SMBND_HASH_MASK; i++) {
		smb_llist_constructor(&smb_node_hash_table[i],
		    sizeof (smb_node_t), offsetof(smb_node_t, n_lnd));
	}
	smb_node_initialized = B_TRUE;
	return (0);
}

/*
 * smb_node_fini
 *
 * This function is not multi-thread safe. The caller must make sure only one
 * thread makes the call.
 */
void
smb_node_fini(void)
{
	int	i;

	if (!smb_node_initialized)
		return;

#ifdef DEBUG
	for (i = 0; i <= SMBND_HASH_MASK; i++) {
		smb_node_t	*node;

		/*
		 * The following sequence is just intended for sanity check.
		 * This will have to be modified when the code goes into
		 * production.
		 *
		 * The SMB node hash table should be emtpy at this point. If the
		 * hash table is not empty a panic will be triggered.
		 *
		 * The reason why SMB nodes are still remaining in the hash
		 * table is problably due to a mismatch between calls to
		 * smb_node_lookup() and smb_node_release(). You must track that
		 * down.
		 */
		node = smb_llist_head(&smb_node_hash_table[i]);
		ASSERT(node == NULL);
	}
#endif

	for (i = 0; i <= SMBND_HASH_MASK; i++) {
		smb_llist_destructor(&smb_node_hash_table[i]);
	}
	smb_node_initialized = B_FALSE;
}

/*
 * smb_node_lookup()
 *
 * NOTE: This routine should only be called by the file system interface layer,
 * and not by SMB.
 *
 * smb_node_lookup() is called upon successful lookup, mkdir, and create
 * (for both non-streams and streams).  In each of these cases, a held vnode is
 * passed into this routine.  If an smb_node already exists for this vnode,
 * the vp is released.  Otherwise, a new smb_node will be created and the
 * reference will be held until the refcnt on the node goes to 0 (see
 * smb_node_release()).
 *
 * A reference is taken on the smb_node whether found in the hash table
 * or newly created.
 *
 * If an smb_node needs to be created, a reference is also taken on the
 * dir_snode (if passed in).
 *
 * See smb_node_release() for details on the release of these references.
 */

/*ARGSUSED*/
smb_node_t *
smb_node_lookup(
    struct smb_request	*sr,
    struct open_param	*op,
    cred_t		*cred,
    vnode_t		*vp,
    char		*od_name,
    smb_node_t		*dir_snode,
    smb_node_t		*unnamed_node,
    smb_attr_t		*attr)
{
	smb_llist_t		*node_hdr;
	smb_node_t		*node;
	uint32_t		hashkey = 0;
	fs_desc_t		fsd;
	int			error;
	krw_t			lock_mode;
	vnode_t			*unnamed_vp = NULL;

	/*
	 * smb_vop_getattr() is called here instead of smb_fsop_getattr(),
	 * because the node may not yet exist.  We also do not want to call
	 * it with the list lock held.
	 */

	if (unnamed_node)
		unnamed_vp = unnamed_node->vp;

	/*
	 * This getattr is performed on behalf of the server
	 * that's why kcred is used not the user's cred
	 */
	attr->sa_mask = SMB_AT_ALL;
	error = smb_vop_getattr(vp, unnamed_vp, attr, 0, kcred);
	if (error)
		return (NULL);

	if (sr) {
		if (sr->tid_tree) {
			/*
			 * The fsd for a file is that of the tree, even
			 * if the file resides in a different mountpoint
			 * under the share.
			 */
			fsd = sr->tid_tree->t_fsd;
		} else {
			/*
			 * This should be getting executed only for the
			 * tree's root smb_node.
			 */
			fsd = vp->v_vfsp->vfs_fsid;
		}
	} else {
		fsd = vp->v_vfsp->vfs_fsid;
	}

	hashkey = fsd.val[0] + attr->sa_vattr.va_nodeid;
	hashkey += (hashkey >> 24) + (hashkey >> 16) + (hashkey >> 8);
	node_hdr = &smb_node_hash_table[(hashkey & SMBND_HASH_MASK)];
	lock_mode = RW_READER;

	smb_llist_enter(node_hdr, lock_mode);
	for (;;) {
		node = list_head(&node_hdr->ll_list);
		while (node) {
			ASSERT(node->n_magic == SMB_NODE_MAGIC);
			ASSERT(node->n_hash_bucket == node_hdr);
			if ((node->n_hashkey == hashkey) && (node->vp == vp)) {
				smb_rwx_xenter(&node->n_lock);
				DTRACE_PROBE1(smb_node_lookup_hit,
				    smb_node_t *, node);
				switch (node->n_state) {
				case SMB_NODE_STATE_AVAILABLE:
					/* The node was found. */
					node->n_refcnt++;
					if ((node->dir_snode == NULL) &&
					    (dir_snode != NULL) &&
					    (strcmp(od_name, "..") != 0) &&
					    (strcmp(od_name, ".") != 0)) {
						VALIDATE_DIR_NODE(dir_snode,
						    node);
						node->dir_snode = dir_snode;
						smb_node_ref(dir_snode);
					}
					node->attr = *attr;
					node->n_size = attr->sa_vattr.va_size;

					smb_audit_node(node);
					smb_rwx_xexit(&node->n_lock);
					smb_llist_exit(node_hdr);
					VN_RELE(vp);
					return (node);

				case SMB_NODE_STATE_DESTROYING:
					/*
					 * Although the node exists it is about
					 * to be destroyed. We act as it hasn't
					 * been found.
					 */
					smb_rwx_xexit(&node->n_lock);
					break;
				default:
					/*
					 * Although the node exists it is in an
					 * unknown state. We act as it hasn't
					 * been found.
					 */
					ASSERT(0);
					smb_rwx_xexit(&node->n_lock);
					break;
				}
			}
			node = smb_llist_next(node_hdr, node);
		}
		if ((lock_mode == RW_READER) && smb_llist_upgrade(node_hdr)) {
			lock_mode = RW_WRITER;
			continue;
		}
		break;
	}
	node = kmem_cache_alloc(sr->sr_server->si_cache_node, KM_SLEEP);
	bzero(node, sizeof (smb_node_t));

	node->n_state = SMB_NODE_STATE_AVAILABLE;
	node->n_hash_bucket = node_hdr;
	node->n_sr = sr;
	node->vp = vp;
	node->n_hashkey = hashkey;
	node->n_refcnt = 1;
	node->tree_fsd = vp->v_vfsp->vfs_fsid;
	node->attr = *attr;
	node->flags |= NODE_FLAGS_ATTR_VALID;
	node->n_size = node->attr.sa_vattr.va_size;
	node->n_orig_session_id = sr->session->s_kid;
	node->n_orig_uid = crgetuid(sr->user_cr);
	node->n_cache = sr->sr_server->si_cache_node;

	ASSERT(od_name);
	(void) strlcpy(node->od_name, od_name, sizeof (node->od_name));

	if (fsd_chkcap(&vp->v_vfsp->vfs_fsid, FSOLF_READONLY) > 0)
		node->flags |= NODE_READ_ONLY;

	smb_llist_constructor(&node->n_ofile_list, sizeof (smb_ofile_t),
	    offsetof(smb_ofile_t, f_nnd));
	smb_llist_constructor(&node->n_lock_list, sizeof (smb_lock_t),
	    offsetof(smb_lock_t, l_lnd));


	if (strcmp(od_name, XATTR_DIR) == 0)
		node->flags |= NODE_XATTR_DIR;
	if (op)
		node->flags |= smb_is_executable(op->fqi.last_comp);

	if (dir_snode) {
		smb_node_ref(dir_snode);
		node->dir_snode = dir_snode;
		ASSERT(dir_snode->dir_snode != node);
		ASSERT((dir_snode->vp->v_xattrdir) ||
		    (dir_snode->vp->v_type == VDIR));
	}

	if (unnamed_node) {
		smb_node_ref(unnamed_node);
		node->unnamed_stream_node = unnamed_node;
	}

	smb_rwx_init(&node->n_lock);
	node->n_magic = SMB_NODE_MAGIC;
	smb_audit_buf_node_create(node);
	DTRACE_PROBE1(smb_node_lookup_miss, smb_node_t *, node);
	smb_audit_node(node);
	smb_llist_insert_head(node_hdr, node);

	smb_llist_exit(node_hdr);
	return (node);
}

/*
 * smb_stream_node_lookup()
 *
 * Note: stream_name (the name that will be stored in the "od_name" field
 * of a stream's smb_node) is the same as the on-disk name for the stream
 * except that it does not have SMB_STREAM_PREFIX prepended.
 */

smb_node_t *
smb_stream_node_lookup(struct smb_request *sr, cred_t *cr, smb_node_t *fnode,
    vnode_t *xattrdirvp, vnode_t *vp, char *stream_name, smb_attr_t *ret_attr)
{
	smb_node_t	*xattrdir_node;
	smb_node_t	*snode;
	smb_attr_t	tmp_attr;

	xattrdir_node = smb_node_lookup(sr, NULL, cr, xattrdirvp, XATTR_DIR,
	    fnode, NULL, &tmp_attr);

	if (xattrdir_node == NULL)
		return (NULL);

	snode = smb_node_lookup(sr, NULL, cr, vp, stream_name, xattrdir_node,
	    fnode, ret_attr);

	/*
	 * The following VN_HOLD is necessary because the caller will VN_RELE
	 * xattrdirvp in the case of an error.  (xattrdir_node has the original
	 * hold on the vnode, which the smb_node_release() call below will
	 * release.)
	 */
	if (snode == NULL) {
		VN_HOLD(xattrdirvp);
	}
	(void) smb_node_release(xattrdir_node);
	return (snode);
}


/*
 * This function should be called whenever a reference is needed on an
 * smb_node pointer.  The copy of an smb_node pointer from one non-local
 * data structure to another requires a reference to be taken on the smb_node
 * (unless the usage is localized).  Each data structure deallocation routine
 * will call smb_node_release() on its smb_node pointers.
 *
 * In general, an smb_node pointer residing in a structure should never be
 * stale.  A node pointer may be NULL, however, and care should be taken
 * prior to calling smb_node_ref(), which ASSERTs that the pointer is valid.
 * Care also needs to be taken with respect to racing deallocations of a
 * structure.
 */

void
smb_node_ref(smb_node_t *node)
{
	ASSERT(node);
	ASSERT(node->n_magic == SMB_NODE_MAGIC);
	ASSERT(node->n_state == SMB_NODE_STATE_AVAILABLE);

	smb_rwx_xenter(&node->n_lock);
	node->n_refcnt++;
	ASSERT(node->n_refcnt);
	DTRACE_PROBE1(smb_node_ref_exit, smb_node_t *, node);
	smb_audit_node(node);
	smb_rwx_xexit(&node->n_lock);
}

/*
 * smb_node_lookup() takes a hold on an smb_node, whether found in the
 * hash table or newly created.  This hold is expected to be released
 * in the following manner.
 *
 * smb_node_lookup() takes an address of an smb_node pointer.  This should
 * be getting passed down via a lookup (whether path name or component), mkdir,
 * create.  If the original smb_node pointer resides in a data structure, then
 * the deallocation routine for the data structure is responsible for calling
 * smb_node_release() on the smb_node pointer.  Alternatively,
 * smb_node_release() can be called as soon as the smb_node pointer is no longer
 * needed.  In this case, callers are responsible for setting an embedded
 * pointer to NULL if it is known that the last reference is being released.
 *
 * If the passed-in address of the smb_node pointer belongs to a local variable,
 * then the caller with the local variable should call smb_node_release()
 * directly.
 *
 * smb_node_release() itself will call smb_node_release() on a node's dir_snode,
 * as smb_node_lookup() takes a hold on dir_snode.
 */
void
smb_node_release(smb_node_t *node)
{
	ASSERT(node);
	ASSERT(node->n_magic == SMB_NODE_MAGIC);

	smb_rwx_xenter(&node->n_lock);
	ASSERT(node->n_refcnt);
	DTRACE_PROBE1(smb_node_release, smb_node_t *, node);
	if (--node->n_refcnt == 0) {
		switch (node->n_state) {

		case SMB_NODE_STATE_AVAILABLE:
			node->n_state = SMB_NODE_STATE_DESTROYING;
			smb_rwx_xexit(&node->n_lock);

			smb_llist_enter(node->n_hash_bucket, RW_WRITER);
			smb_llist_remove(node->n_hash_bucket, node);
			smb_llist_exit(node->n_hash_bucket);

			/*
			 * Check if the file was deleted
			 */
			smb_node_delete_on_close(node);
			node->n_magic = (uint32_t)~SMB_NODE_MAGIC;

			/* These lists should be empty. */
			smb_llist_destructor(&node->n_ofile_list);
			smb_llist_destructor(&node->n_lock_list);

			if (node->dir_snode) {
				ASSERT(node->dir_snode->n_magic ==
				    SMB_NODE_MAGIC);
				smb_node_release(node->dir_snode);
			}

			if (node->unnamed_stream_node) {
				ASSERT(node->unnamed_stream_node->n_magic ==
				    SMB_NODE_MAGIC);
				smb_node_release(node->unnamed_stream_node);
			}

			ASSERT(node->vp);
			VN_RELE(node->vp);

			smb_audit_buf_node_destroy(node);
			smb_rwx_destroy(&node->n_lock);
			kmem_cache_free(node->n_cache, node);
			return;

		default:
			ASSERT(0);
			break;
		}
	}
	smb_audit_node(node);
	smb_rwx_xexit(&node->n_lock);
}

static void
smb_node_delete_on_close(smb_node_t *node)
{
	smb_node_t	*d_snode;
	int		rc = 0;

	d_snode = node->dir_snode;
	if (node->flags & NODE_FLAGS_DELETE_ON_CLOSE) {

		node->flags &= ~NODE_FLAGS_DELETE_ON_CLOSE;
		ASSERT(node->od_name != NULL);
		if (node->attr.sa_vattr.va_type == VDIR)
			rc = smb_fsop_rmdir(0, node->delete_on_close_cred,
			    d_snode, node->od_name, 1);
		else
			rc = smb_fsop_remove(0, node->delete_on_close_cred,
			    d_snode, node->od_name, 1);
		smb_cred_rele(node->delete_on_close_cred);
	}
	if (rc != 0)
		cmn_err(CE_WARN, "File %s could not be removed, rc=%d\n",
		    node->od_name, rc);
	DTRACE_PROBE2(smb_node_delete_on_close, int, rc, smb_node_t *, node);
}

/*
 * smb_node_rename()
 *
 */
int
smb_node_rename(
    smb_node_t	*from_dir_snode,
    smb_node_t	*ret_snode,
    smb_node_t	*to_dir_snode,
    char	*to_name)
{
	ASSERT(from_dir_snode);
	ASSERT(to_dir_snode);
	ASSERT(ret_snode);
	ASSERT(from_dir_snode->n_magic == SMB_NODE_MAGIC);
	ASSERT(to_dir_snode->n_magic == SMB_NODE_MAGIC);
	ASSERT(ret_snode->n_magic == SMB_NODE_MAGIC);
	ASSERT(from_dir_snode->n_state == SMB_NODE_STATE_AVAILABLE);
	ASSERT(to_dir_snode->n_state == SMB_NODE_STATE_AVAILABLE);
	ASSERT(ret_snode->n_state == SMB_NODE_STATE_AVAILABLE);

	smb_node_ref(to_dir_snode);
	smb_rwx_xenter(&ret_snode->n_lock);
	ret_snode->dir_snode = to_dir_snode;
	smb_rwx_xexit(&ret_snode->n_lock);
	ASSERT(to_dir_snode->dir_snode != ret_snode);
	ASSERT((to_dir_snode->vp->v_xattrdir) ||
	    (to_dir_snode->vp->v_type == VDIR));
	smb_node_release(from_dir_snode);

	(void) strcpy(ret_snode->od_name, to_name);

	/*
	 * XXX Need to update attributes?
	 */

	return (0);
}

int
smb_node_root_init(vnode_t *vp, smb_server_t *sv, smb_node_t **root)
{
	smb_attr_t	va;
	int		error;
	uint32_t	hashkey;
	smb_llist_t	*node_hdr;
	smb_node_t	*node;

	/*
	 * Take an explicit hold on rootdir.  This goes with the
	 * corresponding release in smb_node_root_fini()/smb_node_release().
	 */
	VN_HOLD(vp);

	va.sa_mask = SMB_AT_ALL;
	error = smb_vop_getattr(vp, NULL, &va, 0, kcred);
	if (error) {
		VN_RELE(vp);
		return (error);
	}

	hashkey = vp->v_vfsp->vfs_fsid.val[0] + va.sa_vattr.va_nodeid;
	hashkey += (hashkey >> 24) + (hashkey >> 16) + (hashkey >> 8);
	node_hdr = &smb_node_hash_table[(hashkey & SMBND_HASH_MASK)];

	node = kmem_cache_alloc(sv->si_cache_node, KM_SLEEP);
	bzero(node, sizeof (smb_node_t));

	node->n_state = SMB_NODE_STATE_AVAILABLE;
	node->n_hash_bucket = node_hdr;
	node->vp = vp;
	node->n_hashkey = hashkey;
	node->n_refcnt = 1;
	node->tree_fsd = vp->v_vfsp->vfs_fsid;
	node->attr = va;
	node->flags |= NODE_FLAGS_ATTR_VALID;
	node->n_size = node->attr.sa_vattr.va_size;
	node->n_cache = sv->si_cache_node;
	(void) strlcpy(node->od_name, ROOTVOL, sizeof (node->od_name));

	if (fsd_chkcap(&vp->v_vfsp->vfs_fsid, FSOLF_READONLY) > 0)
		node->flags |= NODE_READ_ONLY;

	smb_llist_constructor(&node->n_ofile_list, sizeof (smb_ofile_t),
	    offsetof(smb_ofile_t, f_nnd));
	smb_llist_constructor(&node->n_lock_list, sizeof (smb_lock_t),
	    offsetof(smb_lock_t, l_lnd));

	smb_rwx_init(&node->n_lock);
	node->n_magic = SMB_NODE_MAGIC;
	smb_audit_buf_node_create(node);

	sv->si_root_smb_node = node;

	smb_audit_node(node);
	smb_llist_enter(node_hdr, RW_WRITER);
	smb_llist_insert_head(node_hdr, node);
	smb_llist_exit(node_hdr);

	*root = node;

	return (0);
}

/*
 * smb_node_get_size
 */
u_offset_t
smb_node_get_size(smb_node_t *node, smb_attr_t *attr)
{
	u_offset_t size;

	if (attr->sa_vattr.va_type == VDIR)
		return (0);

	smb_rwx_xenter(&node->n_lock);
	if (node && (node->flags & NODE_FLAGS_SET_SIZE))
		size = node->n_size;
	else
		size = attr->sa_vattr.va_size;
	smb_rwx_xexit(&node->n_lock);
	return (size);
}

static int
timeval_cmp(timestruc_t *a, timestruc_t *b)
{
	if (a->tv_sec < b->tv_sec)
		return (-1);
	if (a->tv_sec > b->tv_sec)
		return (1);
	/* Seconds are equal compare tv_nsec */
	if (a->tv_nsec < b->tv_nsec)
		return (-1);
	return (a->tv_nsec > b->tv_nsec);
}

/*
 * smb_node_set_time
 *
 * This function will update the time stored in the node and
 * set the appropriate flags. If there is nothing to update
 * or the node is readonly, the function would return without
 * any updates. The update is only in the node level and the
 * attribute in the file system will be updated when client
 * close the file.
 */
void
smb_node_set_time(struct smb_node *node, struct timestruc *crtime,
    struct timestruc *mtime, struct timestruc *atime,
    struct timestruc *ctime, unsigned int what)
{
	smb_rwx_xenter(&node->n_lock);
	if (node->flags & NODE_READ_ONLY || what == 0) {
		smb_rwx_xexit(&node->n_lock);
		return;
	}

	if ((what & SMB_AT_CRTIME && crtime == 0) ||
	    (what & SMB_AT_MTIME && mtime == 0) ||
	    (what & SMB_AT_ATIME && atime == 0) ||
	    (what & SMB_AT_CTIME && ctime == 0)) {
		smb_rwx_xexit(&node->n_lock);
		return;
	}

	if ((what & SMB_AT_CRTIME) &&
	    timeval_cmp((timestruc_t *)&node->attr.sa_crtime,
	    crtime) != 0) {
		node->what |= SMB_AT_CRTIME;
		node->attr.sa_crtime = *((timestruc_t *)crtime);
	}

	if ((what & SMB_AT_MTIME) &&
	    timeval_cmp((timestruc_t *)&node->attr.sa_vattr.va_mtime,
	    mtime) != 0) {
		node->what |= SMB_AT_MTIME;
		node->attr.sa_vattr.va_mtime = *((timestruc_t *)mtime);
	}

	if ((what & SMB_AT_ATIME) &&
	    timeval_cmp((timestruc_t *)&node->attr.sa_vattr.va_atime,
	    atime) != 0) {
			node->what |= SMB_AT_ATIME;
			node->attr.sa_vattr.va_atime = *((timestruc_t *)atime);
	}

	/*
	 * The ctime handling is trickier. It has three scenarios.
	 * 1. Only ctime need to be set and it is the same as the ctime
	 *    stored in the node. (update not necessary)
	 * 2. The ctime is the same as the ctime stored in the node but
	 *    is not the only time need to be set. (update required)
	 * 3. The ctime need to be set and is not the same as the ctime
	 *    stored in the node. (update required)
	 * Unlike other time setting, the ctime needs to be set even when
	 * it is the same as the ctime in the node if there are other time
	 * needs to be set (#2). This will ensure the ctime not being
	 * updated when other times are being updated in the file system.
	 *
	 * Retained file rules:
	 *
	 * 1. Don't add SMB_AT_CTIME to node->what by default because the
	 *    request will be rejected by filesystem
	 * 2. 'what' SMB_AT_CTIME shouldn't be set for retained files, i.e.
	 *    any request for changing ctime on these files should have
	 *    been already rejected
	 */
	node->what |= SMB_AT_CTIME;
	if (what & SMB_AT_CTIME) {
		if ((what == SMB_AT_CTIME) &&
		    timeval_cmp((timestruc_t *)&node->attr.sa_vattr.va_ctime,
		    ctime) == 0) {
			node->what &= ~SMB_AT_CTIME;
		} else {
			gethrestime(&node->attr.sa_vattr.va_ctime);
		}
	} else {
		gethrestime(&node->attr.sa_vattr.va_ctime);
	}
	smb_rwx_xexit(&node->n_lock);
}


timestruc_t *
smb_node_get_crtime(smb_node_t *node)
{
	return ((timestruc_t *)&node->attr.sa_crtime);
}

timestruc_t *
smb_node_get_atime(smb_node_t *node)
{
	return ((timestruc_t *)&node->attr.sa_vattr.va_atime);
}

timestruc_t *
smb_node_get_ctime(smb_node_t *node)
{
	return ((timestruc_t *)&node->attr.sa_vattr.va_ctime);
}

timestruc_t *
smb_node_get_mtime(smb_node_t *node)
{
	return ((timestruc_t *)&node->attr.sa_vattr.va_mtime);
}

/*
 * smb_node_set_dosattr
 *
 * Parse the specified DOS attributes and, if they have been modified,
 * update the node cache. This call should be followed by a
 * smb_sync_fsattr() call to write the attribute changes to filesystem.
 */
void
smb_node_set_dosattr(smb_node_t *node, uint32_t dosattr)
{
	uint32_t mode = dosattr & (FILE_ATTRIBUTE_ARCHIVE |
	    FILE_ATTRIBUTE_READONLY |
	    FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

	smb_rwx_xenter(&node->n_lock);
	if (node->attr.sa_dosattr != mode) {
		node->attr.sa_dosattr = mode;
		node->what |= SMB_AT_DOSATTR;
	}
	smb_rwx_xexit(&node->n_lock);
}

/*
 * smb_node_get_dosattr
 *
 * This function will get dos attribute using the node.
 */
uint32_t
smb_node_get_dosattr(smb_node_t *node)
{
	return (smb_mode_to_dos_attributes(&node->attr));
}

int
smb_node_set_delete_on_close(smb_node_t *node, cred_t *cr)
{
	int	rc = -1;

	smb_rwx_xenter(&node->n_lock);
	if (!(node->attr.sa_dosattr & FILE_ATTRIBUTE_READONLY) &&
	    !(node->flags & NODE_FLAGS_DELETE_ON_CLOSE)) {
		crhold(cr);
		node->delete_on_close_cred = cr;
		node->flags |= NODE_FLAGS_DELETE_ON_CLOSE;
		rc = 0;
	}
	smb_rwx_xexit(&node->n_lock);
	return (rc);
}

void
smb_node_reset_delete_on_close(smb_node_t *node)
{
	smb_rwx_xenter(&node->n_lock);
	if (node->flags & NODE_FLAGS_DELETE_ON_CLOSE) {
		node->flags &= ~NODE_FLAGS_DELETE_ON_CLOSE;
		crfree(node->delete_on_close_cred);
		node->delete_on_close_cred = NULL;
	}
	smb_rwx_xexit(&node->n_lock);
}

/*
 * smb_node_open_check
 *
 * check file sharing rules for current open request
 * against all existing opens for a file.
 *
 * Returns NT_STATUS_SHARING_VIOLATION if there is any
 * sharing conflict, otherwise returns NT_STATUS_SUCCESS.
 */
uint32_t
smb_node_open_check(struct smb_node *node, cred_t *cr,
    uint32_t desired_access, uint32_t share_access)
{
	smb_ofile_t *of;
	uint32_t status;

	ASSERT(node);
	ASSERT(node->n_magic == SMB_NODE_MAGIC);
	ASSERT(node->n_state == SMB_NODE_STATE_AVAILABLE);

	smb_llist_enter(&node->n_ofile_list, RW_READER);
	of = smb_llist_head(&node->n_ofile_list);
	while (of) {
		status = smb_ofile_open_check(of, cr, desired_access,
		    share_access);

		switch (status) {
		case NT_STATUS_INVALID_HANDLE:
		case NT_STATUS_SUCCESS:
			of = smb_llist_next(&node->n_ofile_list, of);
			break;
		default:
			ASSERT(status == NT_STATUS_SHARING_VIOLATION);
			smb_llist_exit(&node->n_ofile_list);
			return (status);
		}
	}

	smb_llist_exit(&node->n_ofile_list);
	return (NT_STATUS_SUCCESS);
}

uint32_t
smb_node_rename_check(struct smb_node *node)
{
	struct smb_ofile *of;
	uint32_t status;

	ASSERT(node);
	ASSERT(node->n_magic == SMB_NODE_MAGIC);
	ASSERT(node->n_state == SMB_NODE_STATE_AVAILABLE);

	/*
	 * Intra-CIFS check
	 */

	smb_llist_enter(&node->n_ofile_list, RW_READER);
	of = smb_llist_head(&node->n_ofile_list);
	while (of) {
		status = smb_ofile_rename_check(of);

		switch (status) {
		case NT_STATUS_INVALID_HANDLE:
		case NT_STATUS_SUCCESS:
			of = smb_llist_next(&node->n_ofile_list, of);
			break;
		default:
			ASSERT(status == NT_STATUS_SHARING_VIOLATION);
			smb_llist_exit(&node->n_ofile_list);
			return (status);
		}
	}
	smb_llist_exit(&node->n_ofile_list);

	/*
	 * system-wide share check
	 */

	if (nbl_share_conflict(node->vp, NBL_RENAME, NULL))
		return (NT_STATUS_SHARING_VIOLATION);
	else
		return (NT_STATUS_SUCCESS);
}

uint32_t
smb_node_delete_check(smb_node_t *node)
{
	smb_ofile_t *of;
	uint32_t status;

	ASSERT(node);
	ASSERT(node->n_magic == SMB_NODE_MAGIC);
	ASSERT(node->n_state == SMB_NODE_STATE_AVAILABLE);

	if (node->attr.sa_vattr.va_type == VDIR)
		return (NT_STATUS_SUCCESS);

	/*
	 * intra-CIFS check
	 */

	smb_llist_enter(&node->n_ofile_list, RW_READER);
	of = smb_llist_head(&node->n_ofile_list);
	while (of) {
		status = smb_ofile_delete_check(of);

		switch (status) {
		case NT_STATUS_INVALID_HANDLE:
		case NT_STATUS_SUCCESS:
			of = smb_llist_next(&node->n_ofile_list, of);
			break;
		default:
			ASSERT(status == NT_STATUS_SHARING_VIOLATION);
			smb_llist_exit(&node->n_ofile_list);
			return (status);
		}
	}
	smb_llist_exit(&node->n_ofile_list);

	/*
	 * system-wide share check
	 */

	if (nbl_share_conflict(node->vp, NBL_REMOVE, NULL))
		return (NT_STATUS_SHARING_VIOLATION);
	else
		return (NT_STATUS_SUCCESS);
}

/*
 * smb_node_start_crit()
 *
 * Enter critical region for share reservations.
 * See comments above smb_fsop_shrlock().
 */

void
smb_node_start_crit(smb_node_t *node, krw_t mode)
{
	rw_enter(&node->n_share_lock, mode);
	nbl_start_crit(node->vp, mode);
}

/*
 * smb_node_end_crit()
 *
 * Exit critical region for share reservations.
 */

void
smb_node_end_crit(smb_node_t *node)
{
	nbl_end_crit(node->vp);
	rw_exit(&node->n_share_lock);
}

int
smb_node_in_crit(smb_node_t *node)
{
	return (nbl_in_crit(node->vp) && RW_LOCK_HELD(&node->n_share_lock));
}
