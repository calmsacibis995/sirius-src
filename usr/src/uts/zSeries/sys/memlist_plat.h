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
 * Copyright 1991-2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_SYS_MEMLIST_PLAT_H
#define	_SYS_MEMLIST_PLAT_H

/*
 * Boot time configuration information objects
 */

#include <sys/types.h>
#include <sys/memlist.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _memoryChunk {
	void	*start;
	long	len;
	long	type;
} memoryChunk;

extern long availmem;			// Amount of memory on system
extern int nMemChunk;			// Number of "chunks" of memory
extern memoryChunk *sysMemory;		// List of chunks and sizes

extern int check_boot_version(int);
extern int check_memexp(struct memlist *, uint_t);
extern pgcnt_t size_physavail(void);
extern void installed_top_size_memlist_array(u_longlong_t *, size_t, pfn_t *,
    pgcnt_t *);
extern void installed_top_size(struct memlist *, pfn_t *, pgcnt_t *);
extern void phys_install_has_changed(void);

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_MEMLIST_PLAT_H */
