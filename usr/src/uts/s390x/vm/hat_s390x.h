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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the contents of the sun-reference-mmu(s390x)-
 * specific hat data structures and the s390x-specific hat procedures.
 * The machine-independent interface is described in <vm/hat.h>.
 */

#ifndef	_VM_HAT_S390X_H
#define	_VM_HAT_S390X_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _ASM

#include <sys/types.h>

#endif /* _ASM */

#ifdef	_KERNEL

#include <vm/mm_s390x.h>

#endif

#ifndef _ASM

#include <sys/cpuvar.h>
#include <sys/t_lock.h>
#include <vm/hat.h>
#include <vm/hat_pte.h>
#include <vm/htable.h>
#include <vm/seg.h>
#include <sys/systm.h>
#include <sys/ksynch.h>
#include <vm/page.h>
#include <vm/mm_s390x.h>

#define MSEG_NULLPTR_PA		0xffffffffffffffff

/* Return the leader for this mapping size */
#define	PP_GROUPLEADER(pp, szc) \
	(&(pp)[-(int)((pp)->p_pagenum & (SZCPAGES(szc)-1))])

/* Return the root page for this page based on p_szc */
#define	PP_PAGEROOT(pp) ((pp)->p_szc == 0 ? (pp) : \
	PP_GROUPLEADER((pp), (pp)->p_szc))

#define	PP_PAGENEXT_N(pp, n)	((pp) + (n))
#define	PP_PAGENEXT(pp)		PP_PAGENEXT_N((pp), 1)

#define	PP_PAGEPREV_N(pp, n)	((pp) - (n))
#define	PP_PAGEPREV(pp)		PP_PAGEPREV_N((pp), 1)

#define	PP_ISMAPPED_LARGE(pp)	(PP_MAPINDEX(pp) != 0)

/* Need function to test the page mappping which takes p_index into account */
#define	PP_ISMAPPED(pp)	((pp)->p_mapping || PP_ISMAPPED_LARGE(pp))

#define	PGCNT_INC(hat, level) \
	__sync_fetch_and_add(&(hat)->hat_pages_mapped[level], 1);
#define	PGCNT_DEC(hat, level) \
	__sync_fetch_and_sub(&(hat)->hat_pages_mapped[level], 1);

/*
 * Flags for the hat_flags field
 *
 * HAT_FREEING - set when HAT is being destroyed - mostly used to detect that
 *	demap()s can be avoided.
 *
 * HAT_VICTIM - This is set while a hat is being examined for page table
 *	stealing and prevents it from being freed.
 *
 * HAT_SHARED - The hat has exported it's page tables via hat_share()
 */
#define	HAT_FREEING	(0x0001)
#define	HAT_VICTIM	(0x0002)
#define	HAT_SHARED	(0x0004)

/*
 * Additional platform attribute for hat_devload() to force no caching.
 */
#define	HAT_PLAT_NOCACHE	(0x100000)

/*
 * Useful macro to align hat_XXX() address arguments to a page boundary
 */
#define	ALIGN2PAGE(a)		((uintptr_t)(a) & MMU_PAGEMASK)
#define	IS_PAGEALIGNED(a)	(((uintptr_t)(a) & MMU_PAGEOFFSET) == 0)

/*
 * The hat struct exists for each address space.
 */
struct hat {
	kmutex_t	hat_mutex;
	struct as	*hat_as;
	rte		*hat_rto;
	cpuset_t	hat_cpus;
	uint_t		hat_stats;
	pgcnt_t		hat_pages_mapped[MAX_PAGE_LEVEL + 1];
	uint16_t	hat_flags;
	htable_t	*hat_htable;		/* top level htable */
	struct hat	*hat_next;
	struct hat	*hat_prev;
	uint_t		hat_num_hash;		/* number of htable hash buckets */
	htable_t	**hat_ht_hash;		/* htable hash buckets */
	htable_t	*hat_ht_cached;		/* cached free htables */
	ushort_t	hat_clrstart;		/* start color bin for page coloring */
	ushort_t	hat_clrbin;		/* per as phys page coloring bin */
};
typedef struct hat hat_t;

#define	astohat(as)		((as)->a_hat)
#define	hblktohat(hmeblkp)	((hat_t *)(hmeblkp)->hblk_tag.htag_id)
#define	hattoas(hatp)		((hatp)->hat_as)
/*
 * We use the hat data structure to keep the per as page coloring info.
 */
#define	as_color_bin(as)	(astohat(as)->hat_clrbin)
#define	as_color_start(as)	(astohat(as)->hat_clrstart)

/*
 * Simple statistics for the HAT. These are just counters that are
 * atomically incremented. They can be reset directly from the kernel
 * debugger.
 */
struct hatstats {
	uint64_t	hs_reap_attempts;
	uint64_t	hs_reaped;
	uint64_t	hs_steals;
	uint64_t	hs_ptable_allocs;
	uint64_t	hs_ptable_frees;
	uint64_t	hs_htable_rgets;	/* allocs from reserve */
	uint64_t	hs_htable_rputs;	/* putbacks to reserve */
	uint64_t	hs_htable_shared;	/* number of htables shared */
	uint64_t	hs_htable_unshared;	/* number of htables unshared */
	uint64_t	hs_hm_alloc;
	uint64_t	hs_hm_free;
	uint64_t	hs_hm_put_reserve;
	uint64_t	hs_hm_get_reserve;
	uint64_t	hs_hm_steals;
	uint64_t	hs_hm_steal_exam;
};
extern struct hatstats	hatstat;
#define	HATSTAT_INC(x)	(atomic_add_64(&hatstat.x, 1))

extern kcondvar_t	hat_list_cv;

uint64_t as_va_to_pa(void *, hat_t *);

static s390xpte_t __inline__
ipte(void *pto, uint_t entry)
{
	s390xpte_t	*pte,
			old_pte;

	pte      = (s390xpte_t *) pto + entry;
	old_pte  = *pte;

	__asm__ ("	srlg	%0,%0,11\n"
		 "	sllg	%0,%0,11\n"
		 "	sllg	%1,%1,12\n"
		 "	ipte	%0,%1"
		 :  : "r" (pto), "r" (entry) 
		 : "memory");

	return (old_pte);
}

static s390xpte_t __inline__
idte(void *to, uint_t entry, htable_t *ht)
{
	uint64_t	rsEntry,
			asce;
	s390xpte_t	*pte,
			old_pte;
	htable_t	*as_ht;

	pte     = (s390xpte_t *) to + entry;
	old_pte = *pte;

	if (facilities.facDATe) {
		rsEntry = ((uint64_t) entry) << LEVEL_SHIFT(ht->ht_level);
		as_ht   = ht->ht_hat->hat_htable;
		asce    = ((uint64_t) as_ht->ht_org) | ((as_ht->ht_level - 1) << 2);

		__asm__ ("	srlg	%0,%0,12\n"
			 "	sllg	%0,%0,12\n"
			 "	lgfr	%2,%2\n"
			 "	aghi	%2,-1\n"
			 "	sllg	%2,%2,2\n"
			 "	ogr	%0,%2\n"
			 "	idte	%0,%1,%3"
			 :  
			 : "r" (to), "r" (rsEntry), 
			   "r" (ht->ht_level) , "r" (asce)
			 : "cc", "memory");

	} else {
		uint64_t dummy = 0;

		*pte    |= RS_INVALID;

		__asm__ ("	la	2,%0\n"
			 "	lghi	0,0\n"
			 "	lghi	1,1\n"
			 "	oill	2,3\n"
			 "	csp	0,2\n"
			 : "=m" (dummy) 
			 : : "0", "1", "2", "cc", "memory");

		ptlb();
	}

	return (old_pte);
}

static s390xpte_t __inline__
invalidateRSP(void *pto, uint_t entry, s390xpte_t *pte, htable_t *ht) 
{
	s390xpte_t old_pte;

	if (PTE_ISVALID(*pte, ht->ht_level)) {
		if (ht->ht_level == 0) 
			old_pte = ipte(pto, entry);
		else
			old_pte = idte(pto, entry, ht);
	}
	else
		old_pte = *pte;

	PTE_CLR(*pte, PT_FLAGBITS);

	return (old_pte);
}

#endif /* !_ASM */

#define MAX_PGSZC	0	// Maximum page size code

#define DEFAULT_ISM_PAGESIZE	MMU_PAGESIZE	// Default ISM page size

#define KCONTEXT	0	// Kernel always runs in its own context

#ifdef	__cplusplus
}
#endif

#endif	/* _VM_HAT_S390X_H */
