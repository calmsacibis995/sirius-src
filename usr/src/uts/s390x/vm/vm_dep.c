/*------------------------------------------------------------------*/
/* 								    */
/* Name        - vm_dep.c   					    */
/* 								    */
/* Function    - UNIX machine dependent virtual memory support.	    */
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
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/vm.h>
#include <sys/exec.h>
#include <sys/exechdr.h>
#include <vm/seg_kmem.h>
#include <sys/atomic.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/kdi.h>
#include <sys/cpu.h>
#include <sys/machs390x.h>
#include <vm/hat_s390x.h>
#include <sys/memnode.h>
#include <sys/mem_config.h>
#include <sys/mem_cage.h>
#include <sys/mman.h>
#include <vm/vm_dep.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern uint_t disable_auto_data_large_pages;
extern uint_t disable_auto_text_large_pages;
extern uint_t disable_ism_large_pages;

extern uint_t page_colors;
extern uint_t page_colors_mask;
extern uint_t page_coloring_shift;

extern void page_relocate_hash(page_t *, page_t *);

/*
 * These must be defined in platform specific areas
 */
extern void map_addr_proc(caddr_t *, size_t, offset_t, int, caddr_t,
	struct proc *, uint_t);
extern page_t *page_get_freelist(struct vnode *, u_offset_t, struct seg *,
	caddr_t, size_t, uint_t, struct lgrp *);

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

hw_pagesize_t hw_page_array[] = {
	{MMU_PAGESIZE, MMU_PAGESHIFT, MMU_PAGESIZE >> MMU_PAGESHIFT},
	{0, 0, 0}
};

long	align;
plcnt_t plcnt;

uint_t vac_colors = 0;
uint_t vac_colors_mask = 0;
int cpu_page_colors;

uint_t mmu_page_sizes = 1;

/* How many page sizes the users can see */
uint_t mmu_exported_page_sizes;

/* page sizes that legacy applications can see */
uint_t mmu_legacy_page_sizes;


//
// kpm mapping window
// 
caddr_t kpm_vbase;
size_t  kpm_size;
uchar_t kpm_size_shift;

/*
 * Anchored in the table below are counters used to keep track
 * of free contiguous physical memory. Each element of the table contains
 * the array of counters, the size of array which is allocated during
 * startup based on physmax and a shift value used to convert a pagenum
 * into a counter array index or vice versa. The table has page size
 * for rows and region size for columns:
 *
 *	page_counters[page_size][region_size]
 *
 *	page_size: 	TTE size code of pages on page_size freelist.
 *
 *	region_size:	TTE size code of a candidate larger page made up
 *			made up of contiguous free page_size pages.
 *
 * As you go across a page_size row increasing region_size each
 * element keeps track of how many (region_size - 1) size groups
 * made up of page_size free pages can be coalesced into a
 * regsion_size page. Yuck! Lets try an example:
 *
 * 	page_counters[1][3] is the table element used for identifying
 *	candidate 4M pages from contiguous pages off the 64K free list.
 *	Each index in the page_counters[1][3].array spans 4M. Its the
 *	number of free 512K size (regsion_size - 1) groups of contiguous
 *	64K free pages.	So when page_counters[1][3].counters[n] == 8
 *	we know we have a candidate 4M page made up of 512K size groups
 *	of 64K free pages.
 */

/*
 * Per page size free lists. 3rd (max_mem_nodes) 
 * dimensions is allocated dynamically.
 */
page_t ***page_freelists[MMU_PAGE_SIZES][MAX_MEM_TYPES];

/*
 * For now there is only a single size cache list.
 * Allocated dynamically.
 */
page_t ***page_cachelists[MAX_MEM_TYPES];

kmutex_t *fpc_mutex[NPC_MUTEX];
kmutex_t *cpc_mutex[NPC_MUTEX];

/*
 * Maximum and default segment size tunables for user private
 * and shared anon memory, and user text and initialized data.
 * These can be patched via /etc/system to allow large pages
 * to be used for mapping application private and shared anon memory.
 */
size_t mcntl0_lpsize 		= MMU_PAGESIZE;
size_t max_uheap_lpsize 	= MMU_PAGESIZE;
size_t default_uheap_lpsize 	= MMU_PAGESIZE;
size_t max_ustack_lpsize 	= MMU_PAGESIZE;
size_t default_ustack_lpsize 	= MMU_PAGESIZE;
size_t max_privmap_lpsize 	= MMU_PAGESIZE;
size_t max_uidata_lpsize 	= MMU_PAGESIZE;
size_t max_utext_lpsize 	= MMU_PAGESIZE;
size_t max_shm_lpsize 		= MMU_PAGESIZE;

/*
 * Sanity control. Don't use large pages regardless of user
 * settings if there's less than priv or shm_lpg_min_physmem memory installed.
 * The units for this variable is 4K pages.
 */
pgcnt_t shm_lpg_min_physmem 	= 262144;		/* 1GB */
pgcnt_t privm_lpg_min_physmem 	= 262144;		/* 1GB */

int valid_va_range_aligned_wraparound;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_obmem_pfnum.                                 */
/*                                                                  */
/* Function	- Convert page frame number to an OBMEM page frame  */
/*		  number (i.e. put in the type bits -- zero for     */
/*		  this implementation).				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pfn_t
impl_obmem_pfnum(pfn_t pf)
{
	return (pf);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pf_is_memory.                                     */
/*                                                                  */
/* Function	- Use physmax to determine the highest physical     */
/* 		  Return 1 if the page frame is onboard memory.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
pf_is_memory(pfn_t pf)
{
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pagefault.                                        */
/*                                                                  */
/* Function	- Handle a pagefault.                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

faultcode_t
pagefault(caddr_t addr, enum fault_type type, enum seg_rw rw, int iskernel)
{
	struct as *as;
	struct proc *p;
	faultcode_t res;
	caddr_t base;
	size_t len;
	int err;

#if 0
	if (iskernel) {
		as = &kas;
	} else {
		p  = curproc;
		as = p->p_as;
	}
#endif
		p  = curproc;
		as = p->p_as;

	/*
	 * Dispatch pagefault.
	 */
	res = as_fault(as->a_hat, as, addr, 1, type, rw);

	/*
	 * If this isn't a potential unmapped hole in the user's
	 * UNIX data or stack segments, just return status info.
	 */
	if (!(res == FC_NOMAP && iskernel == 0))
		return(res);

	/*
	 * Check to see if we happened to faulted on a currently unmapped
	 * part of the UNIX data or stack segments.  If so, create a zfod
	 * mapping there and then try calling the fault routine again.
	 */
	base = p->p_brkbase;
	len  = p->p_brksize;

	if (addr < base || addr >= base + len) {		/* data seg? */
		base = (caddr_t)(p->p_usrstack - p->p_stksize);
		len = p->p_stksize;
		if (addr < base || addr >= p->p_usrstack) {	/* stack seg? */
			/* not in either UNIX data or stack segments */
			res = FC_NOMAP;
			return (res);
		}
	}

	/* the rest of this function implements a 3.X 4.X 5.X compatibility */
	/* This code is probably not needed anymore */

	/* expand the gap to the page boundaries on each side */
	len  = (((uintptr_t)base + len + PAGEOFFSET) & PAGEMASK) -
	       ((uintptr_t)base & PAGEMASK);
	base = (caddr_t)((uintptr_t)base & PAGEMASK);

	as_rangelock(as);
	as_purge(as);
	if (as_gap(as, PAGESIZE, &base, &len, AH_CONTAIN, addr) == 0) {
		err = as_map(as, base, len, segvn_create, zfod_argsp);
		as_rangeunlock(as);
		if (err) {
			res = FC_MAKE_ERR(err);
			return (res);
		}
	} else {
		/*
		 * This page is already mapped by another thread after we
		 * returned from as_fault() above.  We just fallthrough
		 * as_fault() below.
		 */
		as_rangeunlock(as);
	}

	res = as_fault(as->a_hat, as, addr, 1, F_INVAL, rw);

	return (res);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_addr.                                         */
/*                                                                  */
/* Function	- Define the address limit.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
map_addr(caddr_t *addrp, size_t len, offset_t off, int vacalign, uint_t flags)
{
	struct proc *p    = curproc;

	map_addr_proc(addrp, len, off, vacalign, p->p_as->a_userlimit, p, flags);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- valid_va_range.                                   */
/*                                                                  */
/* Function	- Determine whether [*basep, *basep + *lenp) 	    */
/*		  contains a mappable range of addresses at least   */
/*		  "minlen" long, where the base of the range is at  */
/*		  "off" phase from an "align" boundary and there is */
/*		  space for a "redzone"-sized redzone on either     */ 
/*		  side of the range.  On success, 1 is returned and */
/*		  *basep and *lenp are adjusted to describe the     */
/*		  acceptable range (including the redzone).  On     */
/*		  failure, 0 is returned.			    */
/*                                                                  */
/*------------------------------------------------------------------*/

/*ARGSUSED3*/
int
valid_va_range_aligned(caddr_t *basep, size_t *lenp, size_t minlen, int dir,
    size_t align, size_t redzone, size_t off)
{
	uintptr_t hi, lo;
	size_t tot_len;

	ASSERT(align == 0 ? off == 0 : off < align);
	ASSERT(ISP2(align));
	ASSERT(align == 0 || align >= PAGESIZE);

	lo = (uintptr_t)*basep;
	hi = lo + *lenp;
	tot_len = minlen + 2 * redzone; /* need at least this much space */

	/*
	 * If hi rolled over the top, try cutting back.
	 */
	if (hi < lo) {
		*lenp = 0UL - lo - 1UL;
		/* See if this really happens. If so, then we figure out why */
		valid_va_range_aligned_wraparound++;
		hi = lo + *lenp;
	}
	if (*lenp < tot_len) {
		return (0);
	}

	if (hi - lo < tot_len)
		return (0);

	if (align > 1) {
		uintptr_t tlo = lo + redzone;
		uintptr_t thi = hi - redzone;
		tlo = (uintptr_t)P2PHASEUP(tlo, align, off);
		if (tlo < lo + redzone) {
			return (0);
		}
		if (thi < tlo || thi - tlo < minlen) {
			return (0);
		}
	}

	*basep = (caddr_t)lo;
	*lenp = hi - lo;
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- valid_va_range.                                   */
/*                                                                  */
/* Function	- Determine whether [base, base+len] contains a     */
/*		  mapable range of addresses at least minlen long.  */
/*		  base and len are adjusted if required to provide  */
/*		  a mapable range.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
valid_va_range(caddr_t *basep, size_t *lenp, size_t minlen, int dir)
{
	return (valid_va_range_aligned(basep, lenp, minlen, dir, 0, 0, 0));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- valid_usr_range.                                  */
/*                                                                  */
/* Function	- Determine whether [addr, addr+len] with 	    */
/*		  protections `prot' are valid for a user address   */
/*		  space.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
valid_usr_range(caddr_t addr, size_t len, uint_t prot, struct as *as,
    caddr_t userlimit)
{
	caddr_t eaddr = addr + len;

	if (eaddr <= addr || addr >= userlimit || eaddr > userlimit)
		return (RANGE_BADADDR);

	return (RANGE_OKAY);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_pgszheap.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static size_t
map_pgszheap(struct proc *p, caddr_t addr, size_t len)
{
	size_t		pgsz = MMU_PAGESIZE;
	int		szc;

	/*
	 * If len is zero, retrieve from proc and don't demote the page size.
	 * Use atleast the default pagesize.
	 */
	if (len == 0) {
		len = p->p_brkbase + p->p_brksize - p->p_bssbase;
	}
	len = MAX(len, default_uheap_lpsize);

	for (szc = mmu_page_sizes - 1; szc >= 0; szc--) {
		pgsz = hw_page_array[szc].hp_size;
		if ((disable_auto_data_large_pages & (1 << szc)) ||
		    pgsz > max_uheap_lpsize)
			continue;
		if (len >= pgsz) {
			break;
		}
	}

	/*
	 * If addr == 0 we were called by memcntl() when the
	 * size code is 0.  Don't set pgsz less than current size.
	 */
	if (addr == 0 && (pgsz < hw_page_array[p->p_brkpageszc].hp_size)) {
		pgsz = hw_page_array[p->p_brkpageszc].hp_size;
	}

	return (pgsz);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_pgszism.                                      */
/*                                                                  */
/* Function	- Return the optimum page size for ISM.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static size_t
map_pgszism(caddr_t addr, size_t len)
{
	uint_t szc;
	size_t pgsz;

	for (szc = mmu_page_sizes - 1; szc >= MAX_PGSZC; szc--) {
		if (disable_ism_large_pages & (1 << szc))
			continue;

		pgsz = hw_page_array[szc].hp_size;
		if ((len >= pgsz) && IS_P2ALIGNED(addr, pgsz))
			return (pgsz);
	}

	return (DEFAULT_ISM_PAGESIZE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_szvec.                                        */
/*                                                                  */
/* Function	- Return a bit vector of large page size codes that */
/*		  can be used to map [addr, addr + len] region.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint_t
map_szcvec(caddr_t addr, size_t size, uintptr_t off, int disable_lpgs,
    size_t max_lpsize, size_t min_physmem)
{
	caddr_t eaddr = addr + size;
	uint_t szcvec = 0;
	caddr_t raddr;
	caddr_t readdr;
	size_t pgsz;
	int i;

	if (physmem < min_physmem || max_lpsize <= MMU_PAGESIZE) {
		return (0);
	}
	for (i = mmu_page_sizes - 1; i > 0; i--) {
		if (disable_lpgs & (1 << i)) {
			continue;
		}
		pgsz = page_get_pagesize(i);
		if (pgsz > max_lpsize) {
			continue;
		}
		raddr = (caddr_t)P2ROUNDUP((uintptr_t)addr, pgsz);
		readdr = (caddr_t)P2ALIGN((uintptr_t)eaddr, pgsz);
		if (raddr < addr || raddr >= readdr) {
			continue;
		}
		if (P2PHASE((uintptr_t)addr ^ off, pgsz)) {
			continue;
		}
		szcvec |= (1 << i);
		/*
		 * And or in the remaining enabled page sizes.
		 */
		szcvec |= P2PHASE(~disable_lpgs, (1 << i));
		szcvec &= ~1; /* no need to return 8K pagesize */
		break;
	}
	return (szcvec);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_szvec.                                        */
/*                                                                  */
/* Function	- Return a bit vector of large page size codes that */
/*		  can be used to map [addr, addr + len] region.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
uint_t
map_pgszcvec(caddr_t addr, size_t size, uintptr_t off, int flags, int type,
    int memcntl)
{
	if (flags & MAP_TEXT) {
	    return (map_szcvec(addr, size, off, disable_auto_text_large_pages,
		    max_utext_lpsize, shm_lpg_min_physmem));

	} else if (flags & MAP_INITDATA) {
	    return (map_szcvec(addr, size, off, disable_auto_data_large_pages,
		    max_uidata_lpsize, privm_lpg_min_physmem));

	} else if (type == MAPPGSZC_SHM) {
	    return (map_szcvec(addr, size, off, disable_auto_data_large_pages,
		    max_shm_lpsize, shm_lpg_min_physmem));

	} else if (type == MAPPGSZC_HEAP) {
	    return (map_szcvec(addr, size, off, disable_auto_data_large_pages,
		    max_uheap_lpsize, privm_lpg_min_physmem));

	} else if (type == MAPPGSZC_STACK) {
	    return (map_szcvec(addr, size, off, disable_auto_data_large_pages,
		    max_ustack_lpsize, privm_lpg_min_physmem));

	} else {
	    return (map_szcvec(addr, size, off, disable_auto_data_large_pages,
		    max_privmap_lpsize, privm_lpg_min_physmem));
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_pgszstk.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static size_t
map_pgszstk(struct proc *p, caddr_t addr, size_t len)
{
	size_t		pgsz = MMU_PAGESIZE;
	int		szc;

	/*
	 * If len is zero, retrieve from proc and don't demote the page size.
	 * Use atleast the default pagesize.
	 */
	if (len == 0) {
		len = p->p_stksize;
	}
	len = MAX(len, default_ustack_lpsize);

	for (szc = mmu_page_sizes - 1; szc >= 0; szc--) {
		pgsz = hw_page_array[szc].hp_size;
		if ((disable_auto_data_large_pages & (1 << szc)) ||
		    pgsz > max_ustack_lpsize)
			continue;
		if (len >= pgsz) {
			break;
		}
	}

	/*
	 * If addr == 0 we were called by memcntl() or exec_args() when the
	 * size code is 0.  Don't set pgsz less than current size.
	 */
	if (addr == 0 && (pgsz < hw_page_array[p->p_stkpageszc].hp_size)) {
		pgsz = hw_page_array[p->p_stkpageszc].hp_size;
	}

	return (pgsz);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_pgsz.                                         */
/*                                                                  */
/* Function	- Return the optimum page size for a given mapping. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
size_t
map_pgsz(int maptype, struct proc *p, caddr_t addr, size_t len, int memcntl)
{
	size_t	pgsz = MMU_PAGESIZE;

	ASSERT(maptype != MAPPGSZ_VA);

	if (maptype != MAPPGSZ_ISM && physmem < privm_lpg_min_physmem) {
		return (MMU_PAGESIZE);
	}

	switch (maptype) {
	case MAPPGSZ_ISM:
		pgsz = map_pgszism(addr, len);
		break;

	case MAPPGSZ_STK:
		if (max_ustack_lpsize > MMU_PAGESIZE) {
			pgsz = map_pgszstk(p, addr, len);
		}
		break;

	case MAPPGSZ_HEAP:
		if (max_uheap_lpsize > MMU_PAGESIZE) {
			pgsz = map_pgszheap(p, addr, len);
		}
		break;
	}
	return (pgsz);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- map_addr_vacalign_check.                          */
/*                                                                  */
/* Function	- Return non 0 value if the address may cause a VAC */
/*		  alias with KPM mappings. KPM selects an address   */
/*		  such that it's equal offset modulo shm_alignment  */
/*		  and * assumes it can't be in VAC conflict with    */
/*		  any larger than PAGESIZE mapping.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
map_addr_vacalign_check(caddr_t addr, u_offset_t off)
{
	if (vac) {
		return (((uintptr_t)addr ^ off) & shm_alignment - 1);
	} else {
		return (0);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- alloc_page_freelists.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
alloc_page_freelists(int mnode, caddr_t alloc_base, int alloc_align)
{
	int	mtype;
	uint_t	szc;

	alloc_base = (caddr_t)roundup((uintptr_t)alloc_base, alloc_align);

	/*
	 * We only support small pages in the cachelist.
	 */
	for (mtype = 0; mtype < MAX_MEM_TYPES; mtype++) {
		page_cachelists[mtype][mnode] = (page_t **)alloc_base;
		alloc_base += (sizeof (page_t *) * page_get_pagecolors(0));
		/*
		 * Allocate freelists bins for all
		 * supported page sizes.
		 */
		for (szc = 0; szc < mmu_page_sizes; szc++) {
			page_freelists[szc][mtype][mnode] =
			    (page_t **)alloc_base;
			alloc_base += ((sizeof (page_t *) *
			    page_get_pagecolors(szc)));
		}
	}

	alloc_base = (caddr_t)roundup((uintptr_t)alloc_base, alloc_align);

	return (alloc_base);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ndata_alloc_page_freelists.                       */
/*                                                                  */
/* Function	- Allocate page_freelists bin headers for a memnode */
/*		  from the nucleus data area.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
ndata_alloc_page_freelists(struct memlist *ndata, int mnode)
{
	size_t alloc_sz;
	caddr_t alloc_base;
	caddr_t end;
	int	mtype;
	uint_t	szc;
	int32_t allp = 0;

	/* first time called - allocate max_mem_nodes dimension */
	if (mnode == 0) {
		int	i;

		/* page_cachelists */
		alloc_sz = MAX_MEM_TYPES * max_mem_nodes *
		    sizeof (page_t **);

		/* page_freelists */
		alloc_sz += MAX_MEM_TYPES * mmu_page_sizes * max_mem_nodes *
		    sizeof (page_t **);

		/* cpc_mutex */
		alloc_sz +=  NPC_MUTEX * max_mem_nodes * sizeof (kmutex_t);

		alloc_base = ndata_alloc(ndata, alloc_sz, CACHE_ALIGNSIZE);
		if (alloc_base == NULL)
			return (-1);

		ASSERT(((uintptr_t)alloc_base & (CACHE_ALIGNSIZE - 1)) == 0);

		for (mtype = 0; mtype < MAX_MEM_TYPES; mtype++) {
			page_cachelists[mtype] = (page_t ***)alloc_base;
			alloc_base += (max_mem_nodes * sizeof (page_t **));
			for (szc = 0; szc < mmu_page_sizes; szc++) {
				page_freelists[szc][mtype] =
				    (page_t ***)alloc_base;
				alloc_base += (max_mem_nodes *
				    sizeof (page_t **));
			}
		}
		for (i = 0; i < NPC_MUTEX; i++) {
			fpc_mutex[i] = (kmutex_t *)alloc_base;
			alloc_base += (sizeof (kmutex_t) * max_mem_nodes);
			cpc_mutex[i] = (kmutex_t *)alloc_base;
			alloc_base += (sizeof (kmutex_t) * max_mem_nodes);
		}
		alloc_sz = 0;
	}

	/*
	 * Calculate the size needed by alloc_page_freelists().
	 */
	for (mtype = 0; mtype < MAX_MEM_TYPES; mtype++) {
		alloc_sz += sizeof (page_t *) * page_colors;
	}

	alloc_base = ndata_alloc(ndata, alloc_sz, CACHE_ALIGNSIZE);
	if (alloc_base == NULL)
		return (-1);

	end = alloc_page_freelists(mnode, alloc_base, CACHE_ALIGNSIZE);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pageout_init.                                     */
/*                                                                  */
/* Function	- Create & Initialise pageout scanner thread. The   */
/*		  thread has to start at procedure with process pp  */
/*		  and priority pri.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
pageout_init(void (*procedure)(), proc_t *pp, pri_t pri)
{
	(void) thread_create(NULL, 0, procedure, NULL, 0, pp, TS_RUN, pri);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	- Function for flushing D-cache when performing     */
/* 		  module relocations to an alternate mapping.  	    */
/*		  Stubbed out on all platforms except sun4u, at	    */
/*		  least for now.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
dcache_flushall()
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kdi_range_is_nontoxic.                            */
/*                                                                  */
/* Function	- Return the number of bytes, relative to the 	    */
/*		  beginning of a given range, that are non-toxic    */
/*		  (can be read from and written to with relative    */
/*		  impunity).					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

size_t
kdi_range_is_nontoxic(uintptr_t va, size_t sz, int write)
{
	return (sz); /* no overlap */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- page_coloring_init.                               */
/*                                                                  */
/* Function	- Initializes page_colors and page_colors_mask 	    */
/*		  based on ecache_setsize.  Also initializes the    */
/*		  counter locks.				    */
/*                                                                  */
/*		  Called once at startup before memialloc()	    */
/* 		  is invoked to do the 1st page_free() /	    */
/*		  page_freelist_add().				    */
/*		                               		 	    */
/* FIXME S390X							    */
/*------------------------------------------------------------------*/

void
page_coloring_init()
{
	int a,i;

	/*
	 * Calculate page_colors from CACHE_SETSIZE, which contains
	 * the max cache setsize of all cpus configured in the system.
	 */
	page_colors      = CACHE_SETSIZE / MMU_PAGESIZE;
	page_colors_mask = page_colors - 1;

	/*
	 * Initialize cpu_page_colors if ecache setsizes are homogenous.
	 * cpu_page_colors set to -1 during DR operation or during startup
	 * if setsizes are heterogenous.
	 *
	 * The value of cpu_page_colors determines if additional color bins
	 * need to be checked for a particular color in the page_get routines.
	 */
	cpu_page_colors = page_colors;

	vac_colors      = page_colors;
	vac_colors_mask = vac_colors - 1;

	page_coloring_shift = 0;
	a = CACHE_SETSIZE;
	while (a >>= 1) {
		page_coloring_shift++;
	}

	/* initialize number of colors per page size */
	for (i = 0; i <= mmu.max_page_level; i++) {
		hw_page_array[i].hp_size   = LEVEL_SIZE(i);
		hw_page_array[i].hp_shift  = LEVEL_SHIFT(i);
		hw_page_array[i].hp_pgcnt  = LEVEL_SIZE(i) >> LEVEL_SHIFT(0);
		hw_page_array[i].hp_colors = (page_colors_mask >>
		    (hw_page_array[i].hp_shift - hw_page_array[0].hp_shift))
		    + 1;
	}

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bp_color.                                         */
/*                                                                  */
/* Function	- Return the color of page.                         */
/*                                                                  */
/*------------------------------------------------------------------*/
/*ARGSUSED*/

int
bp_color(struct buf *bp)
{
	return (0);
}

/*========================= End of Function ========================*/
