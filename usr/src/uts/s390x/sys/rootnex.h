/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License                  
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
 *                                                                 
 * Copyright 2008 Sine Nomine Associates.                         
 * All rights reserved.                                          
 * Use is subject to license terms.                             
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_SYS_ROOTNEX_H
#define	_SYS_ROOTNEX_H

/*
 * s390x root nexus implementation specific state
 */

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/sunddi.h>

#ifdef	__cplusplus
extern "C" {
#endif


/* size of buffer used for ctlop reportdev */
#define	REPORTDEV_BUFSIZE	1024

/* min and max interrupt vectors */
#define	VEC_MIN			0
#define	VEC_MAX			65535

/* atomic increment/decrement to keep track of outstanding binds, etc */
#define	ROOTNEX_PROF_INC(addr)		atomic_inc_64(addr)
#define	ROOTNEX_PROF_DEC(addr)		atomic_add_64(addr, -1)

/*
 * integer or boolean property name and value. A few static rootnex properties
 * are created during rootnex attach from an array of rootnex_intprop_t..
 */
typedef struct rootnex_intprop_s {
	char	*prop_name;
	int	prop_value;
} rootnex_intprop_t;

/*
 * sgl related information which is visible to rootnex_get_sgl(). Trying to
 * isolate get_sgl() as much as possible so it can be easily replaced.
 */
typedef struct rootnex_sglinfo_s {
	/*
	 * These are passed into rootnex_get_sgl().
	 *
	 * si_min_addr - the minimum physical address
	 * si_max_addr - the maximum physical address
	 * si_max_cookie_size - the maximum size of a physically contiguous
	 *    piece of memory that we can handle in a sgl.
	 * si_segmask - segment mask to determine if we cross a segment boundary
	 * si_max_pages - max number of pages this sgl could occupy (which
	 *    is also the maximum number of cookies we might see.
	 */
	uint64_t	si_min_addr;
	uint64_t	si_max_addr;
	uint64_t	si_max_cookie_size;
	uint64_t	si_segmask;
	uint_t		si_max_pages;

	/*
	 * these are returned by rootnex_get_sgl()
	 *
	 * si_copybuf_req - amount of copy buffer needed by the buffer.
	 * si_buf_offset - The initial offset into the first page of the buffer.
	 *    It's set in get sgl and used in the bind slow path to help
	 *    calculate the current page index & offset from the current offset
	 *    which is relative to the start of the buffer.
	 * si_asp - address space of buffer passed in.
	 * si_sgl_size - The actual number of cookies in the sgl. This does
	 *    not reflect and sharing that we might do on window boundaries.
	 */
	size_t		si_copybuf_req;
	off_t		si_buf_offset;
	struct as	*si_asp;
	uint_t		si_sgl_size;
} rootnex_sglinfo_t;

/*
 * profile/performance counters. Most things will be dtrace probes, but there
 * are a couple of things we want to keep track all the time. We track the
 * total number of active handles and binds (i.e. an alloc without a free or
 * a bind without an unbind) since rootnex attach. We also track the total
 * number of binds which have failed since rootnex attach.
 */
typedef enum {
	ROOTNEX_CNT_ACTIVE_HDLS = 0,
	ROOTNEX_CNT_ACTIVE_BINDS = 1,
	ROOTNEX_CNT_ALLOC_FAIL = 2,
	ROOTNEX_CNT_BIND_FAIL = 3,
	ROOTNEX_CNT_SYNC_FAIL = 4,
	ROOTNEX_CNT_GETWIN_FAIL = 5,

	/* This one must be last */
	ROOTNEX_CNT_LAST
} rootnex_cnt_t;

/*
 * global driver state.
 *   r_dmahdl_cache - dma_handle kmem_cache
 *   r_dvma_call_list_id - ddi_set_callback() id
 *   r_peekpoke_mutex - serialize peeks and pokes.
 *   r_dip - rootnex dip
 *   r_reserved_msg_printed - ctlops reserve message threshold
 *   r_counters - profile/performance counters
 */
typedef struct rootnex_state_s {
	uint_t			r_prealloc_cookies;
	uint_t			r_prealloc_size;
	uintptr_t		r_dvma_call_list_id;
	kmutex_t		r_peekpoke_mutex;
	dev_info_t		*r_dip;
	ddi_iblock_cookie_t	r_err_ibc;
	boolean_t		r_reserved_msg_printed;
	uint64_t		r_counters[ROOTNEX_CNT_LAST];
} rootnex_state_t;


#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ROOTNEX_H */
