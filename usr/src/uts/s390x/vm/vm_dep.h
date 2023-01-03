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
 *                                                              
 * Copyright 2008 Sine Nomine Associates.                        
 * All rights reserved.                                           
 * Use is subject to license terms.                                
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * UNIX machine dependent virtual memory support.
 */

#ifndef	_VM_DEP_H
#define	_VM_DEP_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <vm/hat_s390x.h>
#include <sys/archsystm.h>
#include <sys/memnode.h>
#include <sys/clock.h>

#define CACHE_ALIGNSIZE	4096
#define CACHE_SETSIZE	0x40000
#define MMU_PAGE_SIZES	1

#define	GETTICK()	tod2ticks()

/*
 * Do not use this function for obtaining clock tick.  This
 * is called by callers who do not need to have a guarenteed
 * correct tick value.  The proper routine to use is tsc_read().
 */
#define	randtick()	tod2ticks()
/*
 * Per page size free lists. Allocated dynamically.
 */
#define	MAX_MEM_TYPES	2	/* 0 = reloc, 1 = noreloc */
#define	MTYPE_RELOC	0
#define	MTYPE_NORELOC	1

#define	PP_2_MTYPE(pp)	(PP_ISNORELOC(pp) ? MTYPE_NORELOC : MTYPE_RELOC)

#define	MTYPE_INIT(mtype, vp, vaddr, flags, pgsz)			\
	mtype = (flags & PG_NORELOC) ? MTYPE_NORELOC : MTYPE_RELOC;

/* mtype init for page_get_replacement_page */
#define	MTYPE_PGR_INIT(mtype, flags, pp, mnode, pgcnt)			\
	mtype = (flags & PG_NORELOC) ? MTYPE_NORELOC : MTYPE_RELOC;

#define	MNODETYPE_2_PFN(mnode, mtype, pfnlo, pfnhi)			\
	ASSERT(mtype != MTYPE_NORELOC);					\
	pfnlo = mem_node_config[mnode].physbase;			\
	pfnhi = mem_node_config[mnode].physmax;

/*
 * candidate counters in vm_pagelist.c are indexed by color and range
 */
#define	MAX_MNODE_MRANGES		MAX_MEM_TYPES
#define	MNODE_RANGE_CNT(mnode)		MAX_MNODE_MRANGES
#define	MNODE_MAX_MRANGE(mnode)		(MAX_MEM_TYPES - 1)
#define	MTYPE_2_MRANGE(mnode, mtype)	(mtype)

/*
 * Internal PG_ flags.
 */
#define	PGI_RELOCONLY	0x10000	/* acts in the opposite sense to PG_NORELOC */
#define	PGI_NOCAGE	0x20000	/* indicates Cage is disabled */
#define	PGI_PGCPHIPRI	0x40000	/* page_get_contig_page priority allocation */
#define	PGI_PGCPSZC0	0x80000	/* relocate base pagesize page */

/*
 * PGI mtype flags - should not overlap PGI flags
 */
#define	PGI_MT_RANGE	0x1000000	/* mtype range */
#define	PGI_MT_NEXT	0x2000000	/* get next mtype */

extern page_t ***page_freelists[MMU_PAGE_SIZES][MAX_MEM_TYPES];
extern page_t ***page_cachelists[MAX_MEM_TYPES];

#define	PAGE_FREELISTS(mnode, szc, color, mtype) \
	(*(page_freelists[szc][mtype][mnode] + (color)))

#define	PAGE_CACHELISTS(mnode, color, mtype) \
	(*(page_cachelists[mtype][mnode] + (color)))

/*
 * There are 'page_colors' colors/bins.  Spread them out under a
 * couple of locks.  There are mutexes for both the page freelist
 * and the page cachelist.  We want enough locks to make contention
 * reasonable, but not too many -- otherwise page_freelist_lock() gets
 * so expensive that it becomes the bottleneck!
 */
#define	NPC_MUTEX	16

extern kmutex_t	*fpc_mutex[NPC_MUTEX];
extern kmutex_t	*cpc_mutex[NPC_MUTEX];

/* mem node iterator is not used on s390x */
#define	MEM_NODE_ITERATOR_DECL(it)
#define	MEM_NODE_ITERATOR_INIT(pfn, mnode, szc, it)

/*
 * interleaved_mnodes mode is never set on x86, therefore,
 * simply return the limits of the given mnode, which then
 * determines the length of hpm_counters array for the mnode.
 */
#define	HPM_COUNTERS_LIMITS(mnode, physbase, physmax, first) 	\
	{							\
		(physbase) = mem_node_config[(mnode)].physbase;	\
		(physmax) = mem_node_config[(mnode)].physmax;	\
		(first) = (mnode);				\
	}

#define	PAGE_CTRS_WRITE_LOCK(mnode)				\
	{							\
		rw_enter(&page_ctrs_rwlock[(mnode)], RW_WRITER);\
		page_freelist_lock(mnode);			\
	}

#define	PAGE_CTRS_WRITE_UNLOCK(mnode)				\
	{							\
		page_freelist_unlock(mnode);			\
		rw_exit(&page_ctrs_rwlock[(mnode)]);		\
	}

/*
 * cpu specific color conversion functions
 */
#define	PAGE_GET_COLOR_SHIFT(szc, nszc)                          \
	    (hw_page_array[(nszc)].hp_shift - hw_page_array[(szc)].hp_shift)

#define	PAGE_CONVERT_COLOR(ncolor, szc, nszc)			\
	    ((ncolor) << PAGE_GET_COLOR_SHIFT((szc), (nszc)))

#define	PFN_2_COLOR(pfn, szc, it)					\
	(((pfn) & page_colors_mask) >>			                \
	(hw_page_array[szc].hp_shift - hw_page_array[0].hp_shift))

#define	PNUM_SIZE(szc)							\
	(hw_page_array[(szc)].hp_pgcnt)
#define	PNUM_SHIFT(szc)							\
	(hw_page_array[(szc)].hp_shift - hw_page_array[0].hp_shift)
#define	PAGE_GET_SHIFT(szc)						\
	(hw_page_array[(szc)].hp_shift)
#define	PAGE_GET_PAGECOLORS(szc)					\
	(hw_page_array[(szc)].hp_colors)

/*
 * This macro calculates the next sequential pfn with the specified
 * color using color equivalency mask
 */
#define	PAGE_NEXT_PFN_FOR_COLOR(pfn, szc, color, ceq_mask, color_mask, it)    \
	ASSERT(((color) & ~(ceq_mask)) == 0);                                 \
	{								      \
		uint_t	pfn_shift = PAGE_BSZS_SHIFT(szc);                     \
		pfn_t	spfn = pfn >> pfn_shift;                              \
		pfn_t	stride = (ceq_mask) + 1;                              \
		ASSERT((((ceq_mask) + 1) & (ceq_mask)) == 0);                 \
		if (((spfn ^ (color)) & (ceq_mask)) == 0) {                   \
			pfn += stride << pfn_shift;                           \
		} else {                                                      \
			pfn = (spfn & ~(pfn_t)(ceq_mask)) | (color);          \
			pfn = (pfn > spfn ? pfn : pfn + stride) << pfn_shift; \
		}                                                             \
	}

/* get the color equivalency mask for the next szc */
#define	PAGE_GET_NSZ_MASK(szc, mask)                                         \
	((mask) >> (PAGE_GET_SHIFT((szc) + 1) - PAGE_GET_SHIFT(szc)))

/* get the color of the next szc */
#define	PAGE_GET_NSZ_COLOR(szc, color)                                       \
	((color) >> (PAGE_GET_SHIFT((szc) + 1) - PAGE_GET_SHIFT(szc)))

/* Find the bin for the given page if it was of size szc */
#define	PP_2_BIN_SZC(pp, szc)	(PFN_2_COLOR(pp->p_pagenum, szc, NULL))

#define	PP_2_BIN(pp)		(PP_2_BIN_SZC(pp, pp->p_szc))

#define	PP_2_MEM_NODE(pp)	(PFN_2_MEM_NODE(pp->p_pagenum))

#define	PC_BIN_MUTEX(mnode, bin, flags) ((flags & PG_FREE_LIST) ?	\
	&fpc_mutex[(bin) & (NPC_MUTEX - 1)][mnode] :			\
	&cpc_mutex[(bin) & (NPC_MUTEX - 1)][mnode])

#define	FPC_MUTEX(mnode, i)	(&fpc_mutex[i][mnode])
#define	CPC_MUTEX(mnode, i)	(&cpc_mutex[i][mnode])

#define	SZCPAGES(szc)		(1 << PAGE_BSZS_SHIFT(szc))
#define	PFN_BASE(pfnum, szc)	(pfnum & ~((1 << PAGE_BSZS_SHIFT(szc)) - 1))

/*
 * this structure is used for walking free page lists
 * controls when to split large pages into smaller pages,
 * and when to coalesce smaller pages into larger pages
 */
typedef struct page_list_walker {
	uint_t	plw_colors;		/* num of colors for szc */
	uint_t  plw_color_mask;		/* colors-1 */
	uint_t	plw_bin_step;		/* next bin: 1 or 2 */
	uint_t  plw_count;		/* loop count */
	uint_t	plw_bin0;		/* starting bin */
	uint_t  plw_bin_marker;		/* bin after initial jump */
	uint_t  plw_bin_split_prev;	/* last bin we tried to split */
	uint_t  plw_do_split;		/* set if OK to split */
	uint_t  plw_split_next;		/* next bin to split */
	uint_t	plw_ceq_dif;		/* number of different color groups */
					/* to check */
	uint_t	plw_ceq_mask[MMU_PAGE_SIZES + 1]; /* color equiv mask */
	uint_t	plw_bins[MMU_PAGE_SIZES + 1];	/* num of bins */
} page_list_walker_t;

void	page_list_walk_init(uchar_t szc, uint_t flags, uint_t bin,
    int can_split, int use_ceq, page_list_walker_t *plw);

typedef	char	hpmctr_t;

#ifdef DEBUG
#define	CHK_LPG(pp, szc)	chk_lpg(pp, szc)
extern void	chk_lpg(page_t *, uchar_t);
#else
#define	CHK_LPG(pp, szc)
#endif

/*
 * page list count per mnode and type.
 */
typedef	struct {
	pgcnt_t	plc_mt_pgmax;		/* max page cnt */
	pgcnt_t plc_mt_clpgcnt;		/* cache list cnt */
	pgcnt_t plc_mt_flpgcnt;		/* free list cnt - small pages */
	pgcnt_t plc_mt_lgpgcnt;		/* free list cnt - large pages */
#ifdef DEBUG
	struct {
		pgcnt_t plc_mts_pgcnt;	/* per page size count */
		int	plc_mts_colors;
		pgcnt_t	*plc_mtsc_pgcnt; /* per color bin count */
	} plc_mts[MMU_PAGE_SIZES];
#endif
} plcnt_t[MAX_MEM_NODES][MAX_MEM_TYPES];

#ifdef DEBUG

#define	PLCNT_SZ(ctrs_sz) {						\
	int	szc;							\
	for (szc = 0; szc < mmu_page_sizes; szc++) {			\
		int	colors = page_get_pagecolors(szc);		\
		ctrs_sz += (max_mem_nodes * MAX_MEM_TYPES *		\
		    colors * sizeof (pgcnt_t));				\
	}								\
}

#define	PLCNT_INIT(base) {						\
	int	mn, mt, szc, colors;					\
	for (szc = 0; szc < mmu_page_sizes; szc++) {			\
		colors = page_get_pagecolors(szc);			\
		for (mn = 0; mn < max_mem_nodes; mn++) {		\
			for (mt = 0; mt < MAX_MEM_TYPES; mt++) {	\
				plcnt[mn][mt].plc_mts[szc].		\
				    plc_mts_colors = colors;		\
				plcnt[mn][mt].plc_mts[szc].		\
				    plc_mtsc_pgcnt = (pgcnt_t *)base;	\
				base += (colors * sizeof (pgcnt_t));	\
			}						\
		}							\
	}								\
}

#define	PLCNT_DO(pp, mn, mtype, szc, cnt, flags) {			\
	int	bin = PP_2_BIN(pp);					\
	if (flags & PG_CACHE_LIST)					\
		atomic_add_long(&plcnt[mn][mtype].plc_mt_clpgcnt, cnt);	\
	else if (szc)							\
		atomic_add_long(&plcnt[mn][mtype].plc_mt_lgpgcnt, cnt);	\
	else								\
		atomic_add_long(&plcnt[mn][mtype].plc_mt_flpgcnt, cnt);	\
	atomic_add_long(&plcnt[mn][mtype].plc_mts[szc].plc_mts_pgcnt,	\
	    cnt);							\
	atomic_add_long(&plcnt[mn][mtype].plc_mts[szc].			\
	    plc_mtsc_pgcnt[bin], cnt);					\
}

#else

#define	PLCNT_SZ(ctrs_sz)

#define	PLCNT_INIT(base)

/* PG_FREE_LIST may not be explicitly set in flags for large pages */

#define	PLCNT_DO(pp, mn, mtype, szc, cnt, flags) {			\
	if (flags & PG_CACHE_LIST)					\
		atomic_add_long(&plcnt[mn][mtype].plc_mt_clpgcnt, cnt);	\
	else if (szc)							\
		atomic_add_long(&plcnt[mn][mtype].plc_mt_lgpgcnt, cnt);	\
	else								\
		atomic_add_long(&plcnt[mn][mtype].plc_mt_flpgcnt, cnt);	\
}

#endif

#define	PLCNT_INCR(pp, mn, mtype, szc, flags) {				\
	long	cnt = (1 << PAGE_BSZS_SHIFT(szc));			\
	PLCNT_DO(pp, mn, mtype, szc, cnt, flags);			\
}

#define	PLCNT_DECR(pp, mn, mtype, szc, flags) {				\
	long	cnt = ((-1) << PAGE_BSZS_SHIFT(szc));			\
	PLCNT_DO(pp, mn, mtype, szc, cnt, flags);			\
}

/*
 * macros to update page list max counts - done when pages transferred
 * from RELOC to NORELOC mtype (kcage_init or kcage_assimilate_page).
 */

#define	PLCNT_XFER_NORELOC(pp) {					\
	long	cnt = (1 << PAGE_BSZS_SHIFT((pp)->p_szc));		\
	int	mn = PP_2_MEM_NODE(pp);					\
	atomic_add_long(&plcnt[mn][MTYPE_NORELOC].plc_mt_pgmax, cnt);	\
	atomic_add_long(&plcnt[mn][MTYPE_RELOC].plc_mt_pgmax, -cnt);	\
}

/*
 * macro to modify the page list max counts when memory is added to
 * the page lists during startup (add_physmem) or during a DR operation
 * when memory is added (kphysm_add_memory_dynamic) or deleted
 * (kphysm_del_cleanup).
 */
#define	PLCNT_MODIFY_MAX(pfn, cnt) {					\
	int	mn = PFN_2_MEM_NODE(pfn);				\
	atomic_add_long(&plcnt[mn][MTYPE_RELOC].plc_mt_pgmax, (cnt));	\
}

extern plcnt_t	plcnt;

#define	MNODE_PGCNT(mn)							\
	(plcnt[mn][MTYPE_RELOC].plc_mt_clpgcnt +			\
	    plcnt[mn][MTYPE_NORELOC].plc_mt_clpgcnt +			\
	    plcnt[mn][MTYPE_RELOC].plc_mt_flpgcnt +			\
	    plcnt[mn][MTYPE_NORELOC].plc_mt_flpgcnt +			\
	    plcnt[mn][MTYPE_RELOC].plc_mt_lgpgcnt +			\
	    plcnt[mn][MTYPE_NORELOC].plc_mt_lgpgcnt)

#define	MNODETYPE_PGCNT(mn, mtype)					\
	(plcnt[mn][mtype].plc_mt_clpgcnt +				\
	    plcnt[mn][mtype].plc_mt_flpgcnt +				\
	    plcnt[mn][mtype].plc_mt_lgpgcnt)

/*
 * macros to loop through the mtype range - MTYPE_START returns -1 in
 * mtype if no pages in mnode/mtype and possibly NEXT mtype.
 */
#define	MTYPE_START(mnode, mtype, flags) {				\
	if (plcnt[mnode][mtype].plc_mt_pgmax == 0) {			\
		ASSERT(mtype == MTYPE_RELOC ||				\
		    MNODETYPE_PGCNT(mnode, mtype) == 0 ||		\
		    plcnt[mnode][mtype].plc_mt_pgmax != 0);		\
		MTYPE_NEXT(mnode, mtype, flags);			\
	}								\
}

/*
 * if allocation from the RELOC pool failed and there is sufficient cage
 * memory, attempt to allocate from the NORELOC pool.
 */
#define	MTYPE_NEXT(mnode, mtype, flags) { 				\
	if (!(flags & (PG_NORELOC | PGI_NOCAGE | PGI_RELOCONLY)) &&	\
	    (kcage_freemem >= kcage_lotsfree)) {			\
		if (plcnt[mnode][MTYPE_NORELOC].plc_mt_pgmax == 0) {	\
			ASSERT(MNODETYPE_PGCNT(mnode, MTYPE_NORELOC) == 0 || \
			    plcnt[mnode][MTYPE_NORELOC].plc_mt_pgmax != 0);  \
			mtype = -1;					\
		} else {						\
			mtype = MTYPE_NORELOC;				\
			flags |= PG_NORELOC;				\
		}							\
	} else {							\
		mtype = -1;						\
	}								\
}

/*
 * get the ecache setsize for the current cpu.
 */
#define	CPUSETSIZE()	CACHE_SETSIZE

extern struct cpu	cpu0;
#define	CPU0		&cpu0

#define	PAGE_BSZS_SHIFT(szc)	(LEVEL_SHIFT(szc) - MMU_PAGESHIFT)
/*
 * For sfmmu each larger page is 8 times the size of the previous
 * size page.
 */
#define	FULL_REGION_CNT(rg_szc)	(8)

/*
 * The counter base must be per page_counter element to prevent
 * races when re-indexing, and the base page size element should
 * be aligned on a boundary of the given region size.
 *
 * We also round up the number of pages spanned by the counters
 * for a given region to PC_BASE_ALIGN in certain situations to simplify
 * the coding for some non-performance critical routines.
 */
#define	PC_BASE_ALIGN		((pfn_t)1 << PAGE_BSZS_SHIFT(mmu_page_sizes-1))
#define	PC_BASE_ALIGN_MASK	(PC_BASE_ALIGN - 1)

#define	L2CACHE_ALIGN		256
#define	L2CACHE_ALIGN_MAX	512

extern uint_t vac_colors_mask;
extern int vac_size;
extern int vac_shift;

/*
 * Maximum and default values for user heap, stack, private and shared
 * anonymous memory, and user text and initialized data.
 *
 * Initial values are defined in architecture specific mach_vm_dep.c file.
 * Used by map_pgsz*() routines.
 */
extern size_t max_uheap_lpsize;
extern size_t default_uheap_lpsize;
extern size_t max_ustack_lpsize;
extern size_t default_ustack_lpsize;
extern size_t max_privmap_lpsize;
extern size_t max_uidata_lpsize;
extern size_t max_utext_lpsize;
extern size_t max_shm_lpsize;

/*
 * For adjusting the default lpsize, for DTLB-limited page sizes.
 */
extern void adjust_data_maxlpsize(size_t ismpagesize);

/*
 * Sanity control. Don't use large pages regardless of user
 * settings if there's less than priv or shm_lpg_min_physmem memory installed.
 * The units for this variable are 8K pages.
 */
extern pgcnt_t privm_lpg_min_physmem;
extern pgcnt_t shm_lpg_min_physmem;

/*
 * AS_2_BIN macro controls the page coloring policy.
 * 0 (default) uses various vaddr bits
 * 1 virtual=paddr
 * 2 bin hopping
 */
#define	AS_2_BIN(as, seg, vp, addr, bin, szc)				\
	bin = PFN_2_COLOR(((uintptr_t)addr >> MMU_PAGESHIFT), szc, NULL);    	

/*
 * cpu private vm data - accessed thru CPU->cpu_vm_data
 *	vc_pnum_memseg: tracks last memseg visited in page_numtopp_nolock()
 *	vc_pnext_memseg: tracks last memseg visited in page_nextn()
 *	vc_kmptr: unaligned kmem pointer for this vm_cpu_data_t
 *	vc_kmsize: orignal kmem size for this vm_cpu_data_t
 */

typedef struct {
	struct memseg	*vc_pnum_memseg;
	struct memseg	*vc_pnext_memseg;
	void		*vc_kmptr;
	size_t		vc_kmsize;
} vm_cpu_data_t;

/* allocation size to ensure vm_cpu_data_t resides in its own cache line */
#define	VM_CPU_DATA_PADSIZE						\
	(P2ROUNDUP(sizeof (vm_cpu_data_t), L2CACHE_ALIGN_MAX))

/* for boot cpu before kmem is initialized */
extern char	vm_cpu_data0[];

/*
 * Function to get an ecache color bin: F(as, cnt, vcolor).
 * the goal of this function is to:
 * - to spread a processes' physical pages across the entire ecache to
 *	maximize its use.
 * - to minimize vac flushes caused when we reuse a physical page on a
 *	different vac color than it was previously used.
 * - to prevent all processes to use the same exact colors and trash each
 *	other.
 *
 * cnt is a bin ptr kept on a per as basis.  As we page_create we increment
 * the ptr so we spread out the physical pages to cover the entire ecache.
 * The virtual color is made a subset of the physical color in order to
 * in minimize virtual cache flushing.
 * We add in the as to spread out different as.	 This happens when we
 * initialize the start count value.
 * sizeof(struct as) is 60 so we shift by 3 to get into the bit range
 * that will tend to change.  For example, on spitfire based machines
 * (vcshft == 1) contigous as are spread bu ~6 bins.
 * vcshft provides for proper virtual color alignment.
 * In theory cnt should be updated using cas only but if we are off by one
 * or 2 it is no big deal.
 * We also keep a start value which is used to randomize on what bin we
 * start counting when it is time to start another loop. This avoids
 * contigous allocations of ecache size to point to the same bin.
 * Why 3? Seems work ok. Better than 7 or anything larger.
 */
#define	PGCLR_LOOPFACTOR 3

/*
 * When a bin is empty, and we can't satisfy a color request correctly,
 * we scan.  If we assume that the programs have reasonable spatial
 * behavior, then it will not be a good idea to use the adjacent color.
 * Using the adjacent color would result in virtually adjacent addresses
 * mapping into the same spot in the cache.  So, if we stumble across
 * an empty bin, skip a bunch before looking.  After the first skip,
 * then just look one bin at a time so we don't miss our cache on
 * every look. Be sure to check every bin.  Page_create() will panic
 * if we miss a page.
 *
 * This also explains the `<=' in the for loops in both page_get_freelist()
 * and page_get_cachelist().  Since we checked the target bin, skipped
 * a bunch, then continued one a time, we wind up checking the target bin
 * twice to make sure we get all of them bins.
 */
#define	BIN_STEP	20

#ifdef VM_STATS
struct vmm_vmstats_str {
	ulong_t pgf_alloc[MMU_PAGE_SIZES];	/* page_get_freelist */
	ulong_t pgf_allocok[MMU_PAGE_SIZES];
	ulong_t pgf_allocokrem[MMU_PAGE_SIZES];
	ulong_t pgf_allocfailed[MMU_PAGE_SIZES];
	ulong_t pgf_allocdeferred;
	ulong_t	pgf_allocretry[MMU_PAGE_SIZES];
	ulong_t pgc_alloc;			/* page_get_cachelist */
	ulong_t pgc_allocok;
	ulong_t pgc_allocokrem;
	ulong_t	pgc_allocokdeferred;
	ulong_t pgc_allocfailed;
	ulong_t	pgcp_alloc[MMU_PAGE_SIZES];	/* page_get_contig_pages */
	ulong_t	pgcp_allocfailed[MMU_PAGE_SIZES];
	ulong_t	pgcp_allocempty[MMU_PAGE_SIZES];
	ulong_t	pgcp_allocok[MMU_PAGE_SIZES];
	ulong_t	ptcp[MMU_PAGE_SIZES];		/* page_trylock_contig_pages */
	ulong_t	ptcpfreethresh[MMU_PAGE_SIZES];
	ulong_t	ptcpfailexcl[MMU_PAGE_SIZES];
	ulong_t	ptcpfailszc[MMU_PAGE_SIZES];
	ulong_t	ptcpfailcage[MMU_PAGE_SIZES];
	ulong_t	ptcpok[MMU_PAGE_SIZES];
	ulong_t	pgmf_alloc[MMU_PAGE_SIZES];	/* page_get_mnode_freelist */
	ulong_t	pgmf_allocfailed[MMU_PAGE_SIZES];
	ulong_t	pgmf_allocempty[MMU_PAGE_SIZES];
	ulong_t	pgmf_allocok[MMU_PAGE_SIZES];
	ulong_t	pgmc_alloc;			/* page_get_mnode_cachelist */
	ulong_t	pgmc_allocfailed;
	ulong_t	pgmc_allocempty;
	ulong_t	pgmc_allocok;
	ulong_t	pladd_free[MMU_PAGE_SIZES];	/* page_list_add/sub */
	ulong_t	plsub_free[MMU_PAGE_SIZES];
	ulong_t	pladd_cache;
	ulong_t	plsub_cache;
	ulong_t	plsubpages_szcbig;
	ulong_t	plsubpages_szc0;
	ulong_t	pfs_req[MMU_PAGE_SIZES];	/* page_freelist_split */
	ulong_t	pfs_demote[MMU_PAGE_SIZES];
	ulong_t	pfc_coalok[MMU_PAGE_SIZES][MAX_MNODE_MRANGES];
	ulong_t ppr_reloc[MMU_PAGE_SIZES];	/* page_relocate */
	ulong_t ppr_relocok[MMU_PAGE_SIZES];
	ulong_t ppr_relocnoroot[MMU_PAGE_SIZES];
	ulong_t ppr_reloc_replnoroot[MMU_PAGE_SIZES];
	ulong_t ppr_relocnolock[MMU_PAGE_SIZES];
	ulong_t ppr_relocnomem[MMU_PAGE_SIZES];
	ulong_t ppr_krelocfail[MMU_PAGE_SIZES];
	ulong_t ppr_copyfail;
	/* page coalesce counter */
	ulong_t	page_ctrs_coalesce[MMU_PAGE_SIZES][MAX_MNODE_MRANGES];
	/* candidates useful */
	ulong_t	page_ctrs_cands_skip[MMU_PAGE_SIZES][MAX_MNODE_MRANGES];
	/* ctrs changed after locking */
	ulong_t	page_ctrs_changed[MMU_PAGE_SIZES][MAX_MNODE_MRANGES];
	/* page_freelist_coalesce failed */
	ulong_t	page_ctrs_failed[MMU_PAGE_SIZES][MAX_MNODE_MRANGES];
	ulong_t	page_ctrs_coalesce_all;	/* page coalesce all counter */
	ulong_t	page_ctrs_cands_skip_all; /* candidates useful for all func */
};
extern struct vmm_vmstats_str vmm_vmstats;
#endif	/* VM_STATS */

/*
 * Used to hold off page relocations into the cage until OBP has completed
 * its boot-time handoff of its resources to the kernel.
 */
extern int page_relocate_ready;

/*
 * cpu/mmu-dependent vm variables may be reset at bootup.
 */
extern uint_t mmu_page_sizes;
extern uint_t max_mmu_page_sizes;
extern uint_t mmu_hashcnt;
extern uint_t max_mmu_hashcnt;
extern size_t mmu_ism_pagesize;
extern int mmu_exported_pagesize_mask;
extern uint_t mmu_exported_page_sizes;
/*
 * page sizes that legacy applications can see via getpagesizes(3c).
 * Used to prevent legacy applications from inadvertantly using the
 * 'new' large pagesizes (1g and above).
 */
extern uint_t mmu_legacy_page_sizes;

extern uint_t szc_2_userszc[];
extern uint_t userszc_2_szc[];

/* For s390x, userszc is the same as the kernel's szc */
#define	USERSZC_2_SZC(userszc)	(userszc)
#define	SZC_2_USERSZC(szc)	(szc)

/*
 * Platform specific page routines
 */
extern void mach_page_add(page_t **, page_t *);
extern void mach_page_sub(page_t **, page_t *);
extern uint_t page_get_pagecolors(uint_t);
extern void ppcopy_kernel__relocatable(page_t *, page_t *);
#define	ppcopy_kernel(p1, p2)	ppcopy_kernel__relocatable(p1, p2)

/*
 * platform specific page creation
 */
extern page_t *page_create_contig(vnode_t *, u_offset_t, uint_t, 
				  uint_t, struct as *, pfn_t, pfn_t);

#ifdef	__cplusplus
}
#endif

#endif	/* _VM_DEP_H */
