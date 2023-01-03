/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mach_vm_dep.c					    */
/* 								    */
/* Function    - Machine specific VM support routines.              */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
/* 								    */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/*                   L I C E N S E                                  */
/*------------------------------------------------------------------*/

/*==================================================================*/
/* 								    */
/* CDDL HEADER START						    */
/* 								    */
/* The contents of this file are subject to the terms of the	    */
/* Common Development and Distribution License                      */
/* (the "License").  You may not use this file except in compliance */
/* with the License.						    */
/* 								    */
/* You can obtain a copy of the license at: 			    */
/* - usr/src/OPENSOLARIS.LICENSE, or,				    */
/* - http://www.opensolaris.org/os/licensing.			    */
/* See the License for the specific language governing permissions  */
/* and limitations under the License.				    */
/* 								    */
/* When distributing Covered Code, include this CDDL HEADER in each */
/* file and include the License file at usr/src/OPENSOLARIS.LICENSE.*/
/* If applicable, add the following below this CDDL HEADER, with    */
/* the fields enclosed by brackets "[]" replaced with your own      */
/* identifying information: 					    */
/* Portions Copyright [yyyy] [name of copyright owner]		    */
/* 								    */
/* CDDL HEADER END						    */
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
/* 								    */
/* 								    */
/* Portions of this source code were derived from Berkeley 4.3 BSD  */
/* under license from the Regents of the University of California.  */
/* 								    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

#define	CONTIG_LOCK()	mutex_enter(&contig_lock);
#define	CONTIG_UNLOCK()	mutex_exit(&contig_lock);

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

/*
 * UNIX machine dependent virtual memory support.
 */

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/vm.h>
#include <sys/exec.h>
#include <sys/cmn_err.h>
#include <sys/cpu.h>
#include <sys/elf_s390.h>
#include <sys/archsystm.h>
#include <sys/mman.h>
#include <sys/memnode.h>
#include <sys/mem_cage.h>
#include <sys/vtrace.h>
#include <vm/as.h>
#include <vm/hat_s390x.h>
#include <vm/vm_dep.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern uint_t page_create_new;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static page_t * is_contigpage_free(pfn_t *, pgcnt_t *, pgcnt_t, int);
static page_t * page_get_contigpage(pgcnt_t *, int, pfn_t, pfn_t);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

uint_t page_colors = 0;
uint_t page_colors_mask = 0;
uint_t page_coloring_shift = 0;

static kmutex_t	contig_lock;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/


/*========================= End of Function ========================*/


/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_addr_proc.                                    */
/*                                                                  */
/* Function	- This routine is called when the system is to      */
/*		  choose an address for the user. We will pick an   */
/*		  an address range which is just below the current  */
/*		  stack limit. The algorithm used for cache con-    */
/*		  sistency on machines with virtual address caches  */
/*		  is such that offset 0 in the vnode is always on   */
/*		  a shm_alingment'ed aligned address. Unfortunately */
/*		  this means that vnode which are demand paged will */
/*		  not be mapped cache consistently with the exec-   */
/*		  utable images. When the cache alignment for a     */
/*		  given object is inconsistent, the lower level     */
/*		  code must manage the translations so that this is */
/*		  not seen here (at the cost of efficiency of 	    */
/*		  course).                     		 	    */
/*		                               		 	    */
/*		  addrp is a value/result parameter.		    */
/*		     On input it is a hint from the user to be used */
/*		     in a completely machine dependent fashion. For */
/*		     MAP_ALIGN, addrp contains the minimal align-   */
/*		     ment.                     		 	    */
/*		                               		 	    */
/*		     On output it is NULL if no address can be      */
/*		     found in the current process address space or  */
/*		     else an address that is currently not mapped   */
/*		     for len bytes with a page of red zone on either*/
/*		     side. If vacalign is true, then the selected   */
/*		     address will obey the alignment constraints of */
/*		     a vac machine based on the given off value.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED4*/
void
map_addr_proc(caddr_t *addrp, size_t len, offset_t off, int vacalign,
    caddr_t userlimit, struct proc *p, uint_t flags)
{
	struct as *as = p->p_as;
	caddr_t addr;
	caddr_t base;
	size_t slen;
	uintptr_t align_amount;
	int allow_largepage_alignment = 1;

	base = p->p_brkbase;
	if (userlimit < as->a_userlimit) {
		/*
		 * This happens when a program wants to map something in
		 * a range that's accessible to a program in a smaller
		 * address space.  For example, a 64-bit program might
		 * be calling mmap32(2) to guarantee that the returned
		 * address is below 4Gbytes.
		 */
		ASSERT(userlimit > base);
		slen = userlimit - base;
	} else {
		slen = p->p_usrstack - base - (((size_t)rctl_enforced_value(
		    rctlproc_legacy[RLIMIT_STACK], p->p_rctls, p) + PAGEOFFSET)
		    & PAGEMASK);
	}

	len = (len + PAGEOFFSET) & PAGEMASK;

	/*
	 * Redzone for each side of the request. This is done to leave
	 * one page unmapped between segments. This is not required, but
	 * it's useful for the user because if their program strays across
	 * a segment boundary, it will catch a fault immediately making
	 * debugging a little easier.
	 */
	len += (2 * PAGESIZE);

	/*
	 * Align virtual addresses on a 4K boundary to ensure
	 * that ELF shared libraries are mapped with the appropriate
	 * alignment constraints by the run-time linker.
	 */
	align_amount = MMU_PAGESIZE;
	if ((flags & MAP_ALIGN) && ((uintptr_t)*addrp != 0) &&
		((uintptr_t)*addrp < align_amount))
		align_amount = (uintptr_t)*addrp;

	/*
	 * 64-bit processes require 1024K alignment of ELF shared libraries.
	 */
	align_amount = MAX(align_amount, MMU_SEGMSIZE);
#ifdef VAC
	if (vac && vacalign && (align_amount < shm_alignment))
		align_amount = shm_alignment;
#endif

	if ((flags & MAP_ALIGN) && ((uintptr_t)*addrp > align_amount)) {
		align_amount = (uintptr_t)*addrp;
	}
	len += align_amount;

	/*
	 * Look for a large enough hole starting below the stack limit.
	 * After finding it, use the upper part.  Addition of PAGESIZE is
	 * for the redzone as described above.
	 */
	as_purge(as);
	if (as_gap(as, len, &base, &slen, AH_HI, NULL) == 0) {
		caddr_t as_addr;

		addr = base + slen - len + PAGESIZE;
		as_addr = addr;
		/*
		 * Round address DOWN to the alignment amount,
		 * add the offset, and if this address is less
		 * than the original address, add alignment amount.
		 */
		addr = (caddr_t)((uintptr_t)addr & (~(align_amount - 1l)));
		addr += (long)(off & (align_amount - 1l));
		if (addr < as_addr) {
			addr += align_amount;
		}

		ASSERT(addr <= (as_addr + align_amount));
		ASSERT(((uintptr_t)addr & (align_amount - 1l)) ==
		    ((uintptr_t)(off & (align_amount - 1l))));
		*addrp = addr;

	} else {
		*addrp = NULL;	/* no more virtual space */
	}
}
/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pagescrub.                                        */
/*                                                                  */
/* Function	- Platform-dependent page scrub call.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
pagescrub(page_t *pp, uint_t off, uint_t len)
{
	/*
	 * For now, we rely on the fact that pagezero() will
	 * always clear UEs.
	 */
	pagezero(pp, off, len);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sync_data_memory.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
sync_data_memory(caddr_t va, size_t len)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- contig_mem_init.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
contig_mem_init(void)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- exec_get_spslew.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

size_t
exec_get_spslew(void)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- page_create_contig.                               */
/*                                                                  */
/* Function	- Acquire contiguous number of pages.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

page_t *
page_create_contig(
	struct vnode	*vp,
	u_offset_t	off,
	uint_t		bytes,
	uint_t		flags,
	struct as	*as,
	pfn_t		bndry,
	pfn_t		limit)
{
	page_t		*plist = NULL;
	uint_t		plist_len = 0;
	pgcnt_t		npages;
	page_t		*npp = NULL;
	uint_t		pages_req;
	page_t		*pp;

	TRACE_4(TR_FAC_VM, TR_PAGE_CREATE_START,
		"page_create_start:vp %p off %llx bytes %u flags %x",
		vp, off, bytes, flags);

	pages_req = npages = mmu_btopr(bytes);

	/*
	 * Do the freemem and pcf accounting.
	 */
	if (!page_create_wait(npages, (flags & ~PG_NORELOC))) {
		return (NULL);
	}

	TRACE_2(TR_FAC_VM, TR_PAGE_CREATE_SUCCESS,
		"page_create_success:vp %p off %llx",
		vp, off);

	/*
	 * If satisfying this request has left us with too little
	 * memory, start the wheels turning to get some back.  The
	 * first clause of the test prevents waking up the pageout
	 * daemon in situations where it would decide that there's
	 * nothing to do.
	 */
	if (nscan < desscan && freemem < minfree) {
		TRACE_1(TR_FAC_VM, TR_PAGEOUT_CV_SIGNAL,
			"pageout_cv_signal:freemem %ld", freemem);
		cv_signal(&proc_pageout->p_cv);
	}

	plist = page_get_contigpage(&npages, 1, bndry, limit);
	if (plist == NULL) {
		page_create_putback(npages);
		return (NULL);
	}

	pp = plist;

	do {
		if (!page_hashin(pp, vp, off, NULL)) {
			panic("page_create_contig: hashin failed %p %p %llx",
			    (void *)pp, (void *)vp, off);
		}
		VM_STAT_ADD(page_create_new);
		off += MMU_PAGESIZE;
		PP_CLRFREE(pp);
		PP_CLRAGED(pp);
		page_set_props(pp, P_REF);
		pp = pp->p_next;
	} while (pp != plist);

	if (!npages) 
		return (plist);

	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- page_get_contigpage.                              */
/*                                                                  */
/* Function	- Locate enough contiguous pages to satisfy request.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

static page_t *
page_get_contigpage(pgcnt_t *pgcnt, int iolock, pfn_t bndry, pfn_t limit)
{
	pfn_t		pfn;
	pgcnt_t		minctg;
	page_t		*pplist = NULL, *plist;
	uint64_t	lo, hi;
	static pfn_t	startpfn = 0;
	static pgcnt_t	lastctgcnt = 0;

	CONTIG_LOCK();

	if (limit == 0)
		hi = physmax - 1;
	else
		hi = MIN(limit, physmax);

	lo     = 0;
	minctg = *pgcnt;

	if (minctg < lastctgcnt)
		startpfn = lo;
	
	lastctgcnt = minctg;

	pfn = startpfn;
	if (bndry > 1)
		pfn += (pfn % bndry);

	while (pfn + minctg - 1 <= hi) {

		plist = is_contigpage_free(&pfn, pgcnt, minctg, iolock);
		if (plist) {
			page_list_concat(&pplist, &plist);
			/*
			 * return when contig pages no longer needed
			 */
			if (!*pgcnt) {
				startpfn = pfn;
				CONTIG_UNLOCK();
				return (pplist);
			}
		}
		if (bndry > 1)
			pfn += (pfn % bndry);
	}

	/* Cannot find contig pages in specified range */
	if (startpfn == lo) {
		CONTIG_UNLOCK();
		return (NULL);
	}

	/* did not start with lo previously */
	pfn = lo;
	if (bndry > 1)
		pfn += (pfn % bndry);

	/* allow search to go above startpfn */
	while (pfn < startpfn) {

		plist = is_contigpage_free(&pfn, pgcnt, minctg, iolock);
		if (plist != NULL) {

			page_list_concat(&pplist, &plist);

			/*
			 * return when contig pages no longer needed
			 */
			if (!*pgcnt) {
				startpfn = pfn;
				CONTIG_UNLOCK();
				return (pplist);
			}
		}
		if (bndry > 1)
			pfn += (pfn % bndry);
	}
	CONTIG_UNLOCK();
	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- is_contigpage_free.                               */
/*                                                                  */
/* Function	- Returns a page list of contiguous pages.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static page_t *
is_contigpage_free(
	pfn_t *pfnp,
	pgcnt_t *pgcnt,
	pgcnt_t minctg,
	int iolock)
{
	int	i = 0;
	pfn_t	pfn = *pfnp;
	page_t	*pp;
	page_t	*plist = NULL;

	do {
retry:
		pp = page_numtopp_nolock(pfn + i);
		if ((pp == NULL) ||
		    (page_trylock(pp, SE_EXCL) == 0)) {
			(*pfnp)++;
			break;
		}
		if (page_pptonum(pp) != pfn + i) {
			page_unlock(pp);
			goto retry;
		}

		if (!(PP_ISFREE(pp))) {
			page_unlock(pp);
			(*pfnp)++;
			break;
		}

		if (!PP_ISAGED(pp)) {
			page_list_sub(pp, PG_CACHE_LIST);
			page_hashout(pp, (kmutex_t *)NULL);
		} else {
			page_list_sub(pp, PG_FREE_LIST);
		}

		if (iolock)
			page_io_lock(pp);
		page_list_concat(&plist, &pp);

		/*
		 * exit loop when pgcnt satisfied
		 */

	} while (++i < *pgcnt);

	*pfnp += i;		/* set to next pfn to search */

	if (i >= minctg) {
		*pgcnt -= i;
		return (plist);
	}

	/*
	 * failure: minctg not satisfied.
	 */

	/* clean up any pages already allocated */

	while (plist) {
		pp = plist;
		page_sub(&plist, pp);
		page_list_add(pp, PG_FREE_LIST | PG_LIST_TAIL);
		if (iolock)
			page_io_unlock(pp);
		page_unlock(pp);
	}

	return (NULL);
}

/*========================= End of Function ========================*/
