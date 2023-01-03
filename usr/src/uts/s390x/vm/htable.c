/*------------------------------------------------------------------*/
/* 								    */
/* Name        - htable.c   					    */
/* 								    */
/* Function    - htable creation, allocation, stealing, and freeing.*/
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

#define	HTABLE_RESERVE_AMOUNT	(200)
#define	NUM_HTABLE_MUTEX 	128

#define	HTABLE_MUTEX_HASH(h) ((h) & (NUM_HTABLE_MUTEX - 1))

#define	HTABLE_ENTER(h)	mutex_enter(&htable_mutex[HTABLE_MUTEX_HASH(h)]);
#define	HTABLE_EXIT(h)	mutex_exit(&htable_mutex[HTABLE_MUTEX_HASH(h)]);

#define TRUE	1
#define	FALSE	0

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/bitmap.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/machsystm.h>
#include <sys/mman.h>
#include <sys/systm.h>
#include <sys/cpuvar.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/disp.h>
#include <sys/vmem.h>
#include <sys/vmsystm.h>
#include <sys/promif.h>
#include <sys/var.h>
#include <sys/dumphdr.h>
#include <sys/machs390x.h>
#include <vm/seg_kmem.h>
#include <vm/seg_kpm.h>
#include <vm/hat.h>
#include <vm/hat_pte.h>
#include <vm/htable.h>
#include <vm/hat_s390x.h>
#include <vm/hment.h>
#include <vm/vm_dep.h>
#include <sys/cmn_err.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern cpuset_t  khat_cpuset;
extern kmutex_t  hat_list_lock;
extern kthread_t *hat_reserves_thread;
extern RSPdescr_t RSP[5];

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void link_ptp(htable_t *higher, htable_t *new, uintptr_t vaddr);
static void unlink_ptp(htable_t *higher, htable_t *old, uintptr_t vaddr);
static void htable_free(htable_t *ht);
static __inline__ s390xpte_t *s390xpte_access_pagetable(htable_t *ht);
static __inline__ void s390xpte_release_pagetable(htable_t *ht);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

kmem_cache_t *htable_cache;

/*
 * The variable htable_reserve_amount, rather than HTABLE_RESERVE_AMOUNT,
 * is used in order to facilitate testing of the htable_steal() code.
 * By resetting htable_reserve_amount to a lower value, we can force
 * stealing to occur.  The reserve amount is a guess to get us through boot.
 */
uint_t htable_reserve_amount = HTABLE_RESERVE_AMOUNT;
kmutex_t htable_reserve_mutex;
uint_t htable_reserve_cnt;
htable_t *htable_reserve_pool;

/*
 * Used to hand test htable_steal().
 */
#ifdef DEBUG
ulong_t force_steal = 0;
ulong_t ptable_cnt = 0;
#endif

/*
 * This variable is so that we can tune this via /etc/system
 * Any value works, but a power of two <= mmu.ptes_per_table[0] is best.
 */
uint_t htable_steal_passes = 8;

/*
 * mutex stuff for access to htable hash
 */
kmutex_t htable_mutex[NUM_HTABLE_MUTEX];

/*
 * Address used for kernel page tables. See ptable_alloc() below.
 */
uintptr_t ptable_va = 0;
size_t	ptable_sz = 2 * MMU_PAGESIZE;

/*
 * A counter to track if we are stealing or reaping htables. When non-zero
 * htable_free() will directly free htables (either to the reserve or kmem)
 * instead of putting them in a hat's htable cache.
 */
uint32_t htable_dont_cache = 0;

/*
 * Track the number of active pagetables, so we can know how many to reap
 */
static uint32_t active_ptables = 0;

/*
 * The combination of using kpreempt_disable()/_enable() and the hci_mutex
 * are used to ensure that an interrupt won't overwrite a temporary mapping
 * while it's in use. If an interrupt thread tries to access a PTE, it will
 * yield briefly back to the pinned thread which holds the cpu's hci_mutex.
 */

static struct hat_cpu_info init_hci;	/* used for cpu 0 */

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- makePTP.                                          */
/*                                                                  */
/* Function	- Create a region or segment table for the given    */
/*		  htable.                          		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static __inline__ s390xpte_t
makePTP(htable_t *ht)
{
	s390xpte_t pte;
	rte	   *rto = (rte *) &pte;

	pte = (s390xpte_t) ht->ht_pfn << MMU_PAGESHIFT;

	if (ht->ht_level > 0) {		// if this is rte (region 1..3)
		rto->type = mmu.level_type[ht->ht_level+1];
		rto->len  = (ht->ht_len / MMU_PAGESIZE) - 1;
	}

	return (pte);
}

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ptable_alloc.                                     */
/*                                                                  */
/* Function	- Allocate a memory page for a hardware page table. */
/*		                               		 	    */
/*		  The pages allocated for page tables are currently */
/*		  obtained in a hacked up way. It works but needs   */
/*		  fixing.                      		 	    */
/*		                               		 	    */
/*		  We currently use page_create_va() on the kvp with */
/*		  fake offsets, segments and virtual address. This  */
/*		  is pretty bogus and has been copied from the i86  */
/*		  platform. A better approach would be to have a    */
/*		  custom page_get_physical() interface that can     */
/*		  specify either mnode random or mnode local and    */
/*		  take a page from whatever color has the MOST      */
/*		  available - this would have a minimal impact on   */
/*		  page coloring.               		 	    */
/*		                               		 	    */
/*		  For now, the htable pointer in ht isonly used to  */
/*		  compute a unique vnode offset for the page.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ptable_alloc(htable_t *ht)
{
	pfn_t 	pfn;
	page_t 	*pp,
		*npp;
	int	nPages;
	u_offset_t offset;
	static struct seg tmpseg;

	ht->ht_pfn = PFN_INVALID;
	__sync_fetch_and_add(&active_ptables, 1);

	/*
	 * Post boot get a page for the table.
	 *
	 * The first check is to see if there is memory in
	 * the system. If we drop to throttlefree, then fail
	 * the ptable_alloc() and let the stealing code kick in.
	 * Note that we have to do this test here, since the test in
	 * page_create_throttle() would let the NOSLEEP allocation
	 * go through and deplete the page reserves.
	 *
	 * The !NOMEMWAIT() lets pageout, fsflush, etc. skip this check.
	 */
	if (!NOMEMWAIT() && freemem <= throttlefree + 1)
		return;

#ifdef DEBUG
	/*
	 * This code makes htable_ steal() easier to test. By setting
	 * force_steal we force pagetable allocations to fall
	 * into the stealing code. Roughly 1 in ever "force_steal"
	 * page table allocations will fail.
	 */
	if (ht->ht_hat != kas.a_hat && force_steal > 1 &&
	    ++ptable_cnt > force_steal) {
		ptable_cnt = 0;
		return;
	}
#endif /* DEBUG */

	/*
	 * This code is temporary, so don't review too critically.
	 * I'm awaiting a new phys page allocator from Kit -- Joe
	 *
	 * We need assign an offset for the page to call
	 */
	offset   = (uintptr_t) ht;
	offset <<= MMU_PAGESHIFT;

	nPages = MAX((mmu.pte_size * mmu.ptes_per_table[ht->ht_level] / MMU_PAGESIZE),1);

	if (page_resv(nPages, KM_NOSLEEP) == 0)
		return;

#ifdef DEBUG
	pp = page_exists(&kvp, offset);
	if (pp != NULL)
		panic("ptable already exists %p", pp);
#endif
	if (ht->ht_level == 0)
		pp = page_create_va(&kvp, offset, MMU_PAGESIZE,
				    PG_EXCL | PG_NORELOC, &tmpseg, (void *) offset);
	else 
		pp = page_create_contig(&kvp, offset, (nPages * MMU_PAGESIZE),
					PG_EXCL | PG_NORELOC, &tmpseg, 0, 0);

	if (pp == NULL) {
msgnoh("Unable to get a level %d ptable",ht->ht_level);
		return;
}

	npp = pp;
	do {
		page_io_unlock(npp);
		page_hashout(npp, NULL);

		page_downgrade(npp);
		ASSERT(PAGE_SHARED(npp));

		npp = npp->p_next;
	} while (npp != pp);

	pfn = pp->p_pagenum;
	if (pfn == PFN_INVALID)
		panic("ptable_alloc(): Invalid PFN!!");

	ht->ht_pfn = pfn;
	ht->ht_org = (void *) (pfn << MMU_PAGESHIFT);
	RSP[ht->ht_level].init(ht->ht_org, mmu.ptes_per_table[ht->ht_level] * mmu.pte_size);
//msgnoh("ptable_alloc - level: %d pages: %x-%x",ht->ht_level,ht->ht_pfn,ht->ht_pfn+nPages-1);
	HATSTAT_INC(hs_ptable_allocs);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ptable_free                                       */
/*                                                                  */
/* Function	- Free an htable's associated page table page. See  */
/*		  the comments for ptable_alloc().		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ptable_free(htable_t *ht)
{
	pfn_t pfn = ht->ht_pfn;
	page_t *pp, *npp, *fpp;
	int nPages = 0;

	/*
	 * need to destroy the page used for the pagetable
	 */
	ASSERT(pfn != PFN_INVALID);
	HATSTAT_INC(hs_ptable_frees);
	__sync_fetch_and_sub(&active_ptables, 1);
	pp = page_numtopp_nolock(pfn);
	if (pp == NULL)
		panic("ptable_free(): no page for pfn!");
	ASSERT(PAGE_SHARED(pp));
	ASSERT(pfn == pp->p_pagenum);

	npp = pp;
	do {
		/*
		 * Get an exclusive lock, might have to wait for a kmem reader.
		 */
		if (!page_tryupgrade(npp)) {
			page_unlock(npp);
			/*
			 * RFE: we could change this to not loop forever
			 * George Cameron had some idea on how to do that.
			 * For now looping works - it's just like sfmmu.
			 */
			while (!page_lock(npp, SE_EXCL, (kmutex_t *)NULL, P_RECLAIM))
				continue;
		}
		fpp = npp;
		npp = npp->p_next;
		page_free(fpp, 1);
		nPages++;
	} while (npp != pp);

	page_unresv(nPages);
//msgnoh("ptable_free - level: %d pages: %x-%x",ht->ht_level,ht->ht_pfn,ht->ht_pfn+nPages-1);
	ht->ht_pfn = PFN_INVALID;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_put_reserve                                */
/*                                                                  */
/* Function	- Put one htable on the reserve list.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
htable_put_reserve(htable_t *ht)
{
	ht->ht_hat = NULL;		/* no longer tied to a hat */
	ASSERT(ht->ht_pfn == PFN_INVALID);
	HATSTAT_INC(hs_htable_rputs);
	mutex_enter(&htable_reserve_mutex);
	ht->ht_next = htable_reserve_pool;
	htable_reserve_pool = ht;
	++htable_reserve_cnt;
	mutex_exit(&htable_reserve_mutex);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_get_reserve                                */
/*                                                                  */
/* Function	- Take one htable from the reserve.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static htable_t *
htable_get_reserve(void)
{
	htable_t *ht = NULL;

	mutex_enter(&htable_reserve_mutex);
	if (htable_reserve_cnt != 0) {
		ht = htable_reserve_pool;
		ASSERT(ht != NULL);
		ASSERT(ht->ht_pfn == PFN_INVALID);
		htable_reserve_pool = ht->ht_next;
		--htable_reserve_cnt;
		HATSTAT_INC(hs_htable_rgets);
	}
	mutex_exit(&htable_reserve_mutex);
	return (ht);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_initial_reserve                            */
/*                                                                  */
/* Function	- Allocate initial htables with page tables and put */
/*		  them on the kernel hat's cache list.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
htable_initial_reserve(uint_t count)
{
	htable_t *ht;
	hat_t *hat = kas.a_hat;

	count += HTABLE_RESERVE_AMOUNT;
	while (count > 0) {
		ht = kmem_cache_alloc(htable_cache, KM_NOSLEEP);
		ASSERT(ht != NULL);

		ht->ht_pfn = PFN_INVALID;
		htable_put_reserve(ht);
		--count;
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_adjust_reserve                             */
/*                                                                  */
/* Function	- Readjust the reserves after a thread finishes     */
/*		  using them.                  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
htable_adjust_reserve()
{
	htable_t *ht;

	ASSERT(curthread != hat_reserves_thread);

	/*
	 * Free any excess htables in the reserve list
	 */
	while (htable_reserve_cnt > htable_reserve_amount) {
		ht = htable_get_reserve();
		if (ht == NULL)
			return;
		ASSERT(ht->ht_pfn == PFN_INVALID);
		kmem_cache_free(htable_cache, ht);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_steal                                      */
/*                                                                  */
/* Function	- This routine steals htables from user processes   */
/*		  for htable_alloc() or htable_reap().		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static htable_t *
htable_steal(uint_t cnt)
{
	hat_t		*hat = kas.a_hat;	/* list starts with khat */
	htable_t	*list = NULL;
	htable_t	*ht;
	htable_t	*higher;
	uint_t		h;
	uint_t		h_start;
	static uint_t	h_seed = 0;
	uint_t		e;
	uintptr_t	va;
	s390xpte_t	pte;
	uint_t		stolen = 0;
	uint_t		pass;
	uint_t		threshold;

	/*
	 * Limit htable_steal_passes to something reasonable
	 */
	if (htable_steal_passes == 0)
		htable_steal_passes = 1;
	if (htable_steal_passes > mmu.ptes_per_table[0])
		htable_steal_passes = mmu.ptes_per_table[0];

	/*
	 * Loop through all user hats. The 1st pass takes cached htables that
	 * aren't in use. The later passes steal by removing mappings, too.
	 */
	__sync_fetch_and_add(&htable_dont_cache, 1);
	for (pass = 0; pass <= htable_steal_passes && stolen < cnt; ++pass) {
		threshold = pass * mmu.ptes_per_table[0] / htable_steal_passes;
		hat = kas.a_hat;
		for (;;) {

			/*
			 * Clear the victim flag and move to next hat
			 */
			mutex_enter(&hat_list_lock);
			if (hat != kas.a_hat) {
				hat->hat_flags &= ~HAT_VICTIM;
				cv_broadcast(&hat_list_cv);
			}
			hat = hat->hat_next;

			/*
			 * Skip any hat that is already being stolen from.
			 *
			 * We skip SHARED hats, as these are dummy
			 * hats that host ISM shared page tables.
			 *
			 * We also skip if HAT_FREEING because hat_pte_unmap()
			 * won't zero out the PTE's. That would lead to hitting
			 * stale PTEs either here or under hat_unload() when we
			 * steal and unload the same page table in competing
			 * threads.
			 */
			while (hat != NULL &&
			    (hat->hat_flags &
			    (HAT_VICTIM | HAT_SHARED | HAT_FREEING)) != 0)
				hat = hat->hat_next;

			if (hat == NULL) {
				mutex_exit(&hat_list_lock);
				break;
			}

			/*
			 * Are we finished?
			 */
			if (stolen == cnt) {
				/*
				 * Try to spread the pain of stealing,
				 * move victim HAT to the end of the HAT list.
				 */
				if (pass >= 1 && cnt == 1 &&
				    kas.a_hat->hat_prev != hat) {

					/* unlink victim hat */
					if (hat->hat_prev)
						hat->hat_prev->hat_next =
						    hat->hat_next;
					else
						kas.a_hat->hat_next =
						    hat->hat_next;
					if (hat->hat_next)
						hat->hat_next->hat_prev =
						    hat->hat_prev;
					else
						kas.a_hat->hat_prev =
						    hat->hat_prev;


					/* relink at end of hat list */
					hat->hat_next = NULL;
					hat->hat_prev = kas.a_hat->hat_prev;
					if (hat->hat_prev)
						hat->hat_prev->hat_next = hat;
					else
						kas.a_hat->hat_next = hat;
					kas.a_hat->hat_prev = hat;

				}

				mutex_exit(&hat_list_lock);
				break;
			}

			/*
			 * Mark the HAT as a stealing victim.
			 */
			hat->hat_flags |= HAT_VICTIM;
			mutex_exit(&hat_list_lock);

			/*
			 * Take any htables from the hat's cached "free" list.
			 */
			hat_enter(hat);
			while ((ht = hat->hat_ht_cached) != NULL &&
			    stolen < cnt) {
				hat->hat_ht_cached = ht->ht_next;
				ht->ht_next = list;
				list = ht;
				++stolen;
			}
			hat_exit(hat);

			/*
			 * Don't steal on first pass.
			 */
			if (pass == 0 || stolen == cnt)
				continue;

			/*
			 * Search the active htables for one to steal.
			 * Start at a different hash bucket every time to
			 * help spread the pain of stealing.
			 */
			h = h_start = h_seed++ % hat->hat_num_hash;
			do {
				higher = NULL;
				HTABLE_ENTER(h);
				for (ht = hat->hat_ht_hash[h]; ht;
				     ht = (htable_t *) ht->ht_next) {

					/*
					 * Can we rule out reaping?
					 */
					if (ht->ht_busy != 0 ||
					    (ht->ht_flags & HTABLE_SHARED_PFN)||
					    ht->ht_level > 0 ||
					    ht->ht_valid_cnt > threshold ||
					    ht->ht_lock_cnt != 0)
						continue;

					/*
					 * Increment busy so the htable can't
					 * disappear. We drop the htable mutex
					 * to avoid deadlocks with
					 * hat_pageunload() and the hment mutex
					 * while we call hat_pte_unmap()
					 */
					++ht->ht_busy;
					HTABLE_EXIT(h);

					/*
					 * Try stealing.
					 * - unload and invalidate all PTEs
					 */
					for (e = 0, va = ht->ht_vaddr;
					    e < ht->ht_num_ptes &&
					    ht->ht_valid_cnt > 0 &&
					    ht->ht_busy == 1 &&
					    ht->ht_lock_cnt == 0;
					    ++e, va += MMU_PAGESIZE) {
						pte = s390xpte_get(ht, e);
						if (!PTE_ISVALID(pte, ht->ht_level))
							continue;
						hat_pte_unmap(ht, e,
						    HAT_UNLOAD, pte, NULL);
					}

					/*
					 * Reacquire htable lock. If we didn't
					 * remove all mappings in the table,
					 * or another thread added a new mapping
					 * behind us, give up on this table.
					 */
					HTABLE_ENTER(h);
					if (ht->ht_busy != 1 ||
					    ht->ht_valid_cnt != 0 ||
					    ht->ht_lock_cnt != 0) {
						--ht->ht_busy;
						continue;
					}

					/*
					 * Steal it and unlink the page table.
					 */
					higher = ht->ht_parent;
					unlink_ptp(higher, ht, ht->ht_vaddr);

					/*
					 * remove from the hash list
					 */
					if (ht->ht_next)
						ht->ht_next->ht_prev =
						    ht->ht_prev;

					if (ht->ht_prev) {
						ht->ht_prev->ht_next =
						    ht->ht_next;
					} else {
						ASSERT(hat->hat_ht_hash[h] ==
						    ht);
						hat->hat_ht_hash[h] =
						    ht->ht_next;
					}

					/*
					 * Break to outer loop to release the
					 * higher (ht_parent) pagtable. This
					 * spreads out the pain caused by
					 * pagefaults.
					 */
					ht->ht_next = list;
					list = ht;
					++stolen;
					break;
				}
				HTABLE_EXIT(h);
				if (higher != NULL)
					htable_release(higher);
				if (++h == hat->hat_num_hash)
					h = 0;
			} while (stolen < cnt && h != h_start);
		}
	}
	__sync_fetch_and_sub(&htable_dont_cache, 1);
	return (list);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_reap                                       */
/*                                                                  */
/* Function	- This is invoked from kmem when the system is low  */
/*		  on memory. We try to free hments, htables, and    */
/*		  ptables to improve the memory situation.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
htable_reap(void *handle)
{
	uint_t		reap_cnt;
	htable_t	*list;
	htable_t	*ht;

	HATSTAT_INC(hs_reap_attempts);

	/*
	 * Try to reap 5% of the page tables bounded by a maximum of
	 * 5% of physmem and a minimum of 10.
	 */
	reap_cnt = MIN(MAX(physmem / 20, active_ptables / 20), 10);

	/*
	 * Let htable_steal() do the work, we just call htable_free()
	 */
	list = htable_steal(reap_cnt);
	while ((ht = list) != NULL) {
		list = ht->ht_next;
		HATSTAT_INC(hs_reaped);
		htable_free(ht);
	}

	/*
	 * Free up excess reserves
	 */
	htable_adjust_reserve();
	hment_adjust_reserve();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_alloc                                      */
/*                                                                  */
/* Function	- Allocate an htable, stealing one or using the     */
/*		  reserve if necessary.        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static htable_t *
htable_alloc(
	hat_t		*hat,
	uintptr_t	vaddr,
	level_t		level,
	htable_t	*shared)
{
	htable_t	*ht = NULL;
	uint_t		is_bare = 0;
	int		kmflags = KM_NOSLEEP;

	if (level < 0 || level > mmu.max_level)
		panic("htable_alloc(): level %d out of range\n", level);

	if (shared != NULL)
		is_bare = 1;

	/*
	 * First reuse a cached htable from the hat_ht_cached field, this
	 * avoids unnecessary trips through kmem/page allocators. 
	 */
	if (hat->hat_ht_cached != NULL && !is_bare) {
		hat_enter(hat);
		ht = hat->hat_ht_cached;
		if (ht != NULL) {
			hat->hat_ht_cached = ht->ht_next;
			if (((ht->ht_level != 0) && (level == 0)) ||
			    ((ht->ht_level == 0) && (level != 0))) {
				ptable_free(ht);
				ht->ht_level = level;
				ptable_alloc(ht);
			} else {
				ht->ht_level = level;
				RSP[level].init(ht->ht_org, 
					        mmu.ptes_per_table[level] * mmu.pte_size);
			}
			if (ht->ht_pfn == PFN_INVALID) {
				kmem_cache_free(htable_cache, ht);
				ht = NULL;
			}
			ASSERT(ht->ht_pfn != PFN_INVALID);
		}
		hat_exit(hat);
	}

	if (ht == NULL) {
		/*
		 * When allocating for hat_memload_arena, we use the reserve.
		 * Also use reserves if we are in a panic().
		 */
		if (curthread == hat_reserves_thread || panicstr != NULL) {
			ASSERT(panicstr != NULL || !is_bare);
			ASSERT(panicstr != NULL ||
			    curthread == hat_reserves_thread);
			ht = htable_get_reserve();
		} else {
			/*
			 * Donate successful htable allocations to the reserve.
			 */
			for (;;) {
				ASSERT(curthread != hat_reserves_thread);
				ht = kmem_cache_alloc(htable_cache, kmflags);
				if (ht == NULL)
					break;
				ht->ht_pfn = PFN_INVALID;
				if (curthread == hat_reserves_thread ||
				    panicstr != NULL ||
				    htable_reserve_cnt >= htable_reserve_amount)
					break;
				htable_put_reserve(ht);
			}
		}

		/*
		 * allocate a page for the hardware page table if needed
		 */
		if (ht != NULL && !is_bare) {
			ht->ht_hat   = hat;
			ht->ht_level = level;
			ptable_alloc(ht);
			if (ht->ht_pfn == PFN_INVALID) {
				kmem_cache_free(htable_cache, ht);
				ht = NULL;
			}
		}
	}

	/*
	 * If allocations failed, kick off a kmem_reap() and resort to
	 * htable steal(). We may spin here if the system is very low on
	 * memory. If the kernel itself has consumed all memory and kmem_reap()
	 * can't free up anything, then we'll really get stuck here.
	 * That should only happen in a system where the administrator has
	 * misconfigured VM parameters via /etc/system.
	 */
	while (ht == NULL) {
		kmem_reap();
		ht = htable_steal(1);
		HATSTAT_INC(hs_steals);

		/*
		 * If we stole for a bare htable, release the pagetable page.
		 */
		if (ht != NULL && is_bare) {
			ptable_free(ht);
			ht->ht_pfn = PFN_INVALID;
		}
	}

	/*
	 * All attempts to allocate or steal failed. This should only happen
	 * if we run out of memory during boot, due perhaps to a huge
	 * boot_archive. At this point there's no way to continue.
	 */
	if (ht == NULL)
		panic("htable_alloc(): couldn't steal\n");

	/*
	 * Shared page tables have all entries locked and entries may not
	 * be added or deleted.
	 */
	ht->ht_flags = 0;
	if (shared != NULL) {
		ASSERT(level == 0);
		ASSERT(shared->ht_valid_cnt > 0);
		ht->ht_flags    |= HTABLE_SHARED_PFN;
		ht->ht_pfn       = shared->ht_pfn;
		ht->ht_lock_cnt  = 0;
		ht->ht_valid_cnt = 0;		/* updated in hat_share() */
		ht->ht_shares    = shared;
	} else {
		ht->ht_shares    = NULL;
		ht->ht_lock_cnt  = 0;
		ht->ht_valid_cnt = 0;
	}

	/*
	 * fill in the htable
	 */
	ht->ht_num_ptes = mmu.ptes_per_table[level];
	ht->ht_hat  	= hat;
	ht->ht_parent 	= NULL;
	ht->ht_vaddr 	= vaddr;
	ht->ht_level 	= level;
	ht->ht_busy 	= 1;
	ht->ht_lock 	= 0;
	ht->ht_next 	= NULL;
	ht->ht_prev 	= NULL;

	return (ht);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_free                                       */
/*                                                                  */
/* Function	- Free up an htable, either to a hat's cached list, */
/*		  the reserves or back to kmem.		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
htable_free(htable_t *ht)
{
	hat_t *hat = ht->ht_hat;

	/*
	 * If the process isn't exiting, cache the free htable in the hat
	 * structure. We always do this for the boot reserve. We don't
	 * do this if the hat is exiting or we are stealing/reaping htables.
	 */
	if (hat != NULL &&
	    !(ht->ht_flags & HTABLE_SHARED_PFN) &&
	    (!(hat->hat_flags & HAT_FREEING) && !htable_dont_cache)) {
		ASSERT(ht->ht_pfn != PFN_INVALID);
		hat_enter(hat);
		ht->ht_next = hat->hat_ht_cached;
		hat->hat_ht_cached = ht;
		hat_exit(hat);
		return;
	}

	/*
	 * If we have a hardware page table, free it.
	 * We don't free page tables that are accessed by sharing someone else.
	 */
	if (ht->ht_flags & HTABLE_SHARED_PFN) {
		ASSERT(ht->ht_pfn != PFN_INVALID);
		ht->ht_pfn = PFN_INVALID;
	} else {
		ptable_free(ht);
	}

	/*
	 * If we are the thread using the reserves, put free htables
	 * into reserves.
	 */
	if (curthread == hat_reserves_thread ||
	    htable_reserve_cnt < htable_reserve_amount)
		htable_put_reserve(ht);
	else
		kmem_cache_free(htable_cache, ht);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_purge_hat                                  */
/*                                                                  */
/* Function	- This is called when a hat is being destroyed or   */
/*		  swapped out. We read all the remaining htables    */
/*		  in the hat cache. If destroying all left over     */
/*		  htables are also destroyed.  		 	    */
/*		                               		 	    */
/*		  We also don't need to invalidate any of the PTPs  */
/*		  nor do any demapping.        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
htable_purge_hat(hat_t *hat)
{
	htable_t *ht;
	int h;

	idte(hat->hat_htable->ht_org, 0, hat->hat_htable);

	/*
	 * Purge the htable cache if just reaping.
	 */
	if (!(hat->hat_flags & HAT_FREEING)) {
		__sync_fetch_and_add(&htable_dont_cache, 1);
		for (;;) {
			hat_enter(hat);
			ht = hat->hat_ht_cached;
			if (ht == NULL) {
				hat_exit(hat);
				break;
			}
			hat->hat_ht_cached = ht->ht_next;
			hat_exit(hat);
			htable_free(ht);
		}
		__sync_fetch_and_sub(&htable_dont_cache, 1);
		return;
	}

	/*
	 * if freeing, no locking is needed
	 */
	while ((ht = hat->hat_ht_cached) != NULL) {
		hat->hat_ht_cached = ht->ht_next;
		htable_free(ht);
	}

	/*
	 * walk thru the htable hash table and free all the htables in it.
	 */
	for (h = 0; h < hat->hat_num_hash; ++h) {
		while ((ht = hat->hat_ht_hash[h]) != NULL) {
			if (ht->ht_next)
				ht->ht_next->ht_prev = ht->ht_prev;

			if (ht->ht_prev) {
				ht->ht_prev->ht_next = ht->ht_next;
			} else {
				ASSERT(hat->hat_ht_hash[h] == ht);
				hat->hat_ht_hash[h] = ht->ht_next;
			}
			htable_free(ht);
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- unlink_ptp                                        */
/*                                                                  */
/* Function	- Unlink an entry for a table at vaddr and level    */
/*		  out of the existing table on level higher. We     */
/*		  are always holding the HASH_ENTER() when doing    */
/*		  this.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
unlink_ptp(htable_t *higher, htable_t *old, uintptr_t vaddr)
{
	uint_t		entry = htable_va2entry(vaddr, higher);
	s390xpte_t	expect = makePTP(old);

	ASSERT(higher->ht_busy > 0);
	ASSERT(higher->ht_valid_cnt > 0);
	ASSERT(old->ht_valid_cnt == 0);
	s390xpte_invalidate_pfn(higher, entry);
	HTABLE_DEC(higher->ht_valid_cnt);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- link_ptp                                          */
/*                                                                  */
/* Function	- Link an entry for a new table at vaddr and level  */
/*		  into the existing table on level higher. We are   */
/*		  always holding the HASH_ENTER() when doing this.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
link_ptp(htable_t *higher, htable_t *new, uintptr_t vaddr)
{
	uint_t		entry = htable_va2entry(vaddr, higher);
	s390xpte_t	newptp = makePTP(new);

	ASSERT(higher->ht_busy > 0);
	ASSERT(new->ht_level != mmu.max_level);

	HTABLE_INC(higher->ht_valid_cnt);
	s390xpte_set(higher, entry, newptp, NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_release                                    */
/*                                                                  */
/* Function	- During process exit, some empty page tables are   */
/*		  not unlinked - hat_free_end() cleans them up.     */
/*		  Upper level pagetable (mmu.max_page_level and     */
/*		  higher) are only released during hat_free_end()   */
/*		  or by htable_steal(). We always release SHARED    */
/*		  page tables.                 		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
htable_release(htable_t *ht)
{
	uint_t		hashval;
	htable_t	*shared;
	htable_t	*higher;
	hat_t		*hat;
	uintptr_t	va;
	level_t		level;

	while (ht != NULL) {
		shared = NULL;
		for (;;) {
			hat = ht->ht_hat;
			va = ht->ht_vaddr;
			level = ht->ht_level;
			hashval = HTABLE_HASH(hat, va, level);

			/*
			 * The common case is that this isn't the last use of
			 * an htable so we don't want to free the htable.
			 */
			HTABLE_ENTER(hashval);
			ASSERT(ht->ht_lock_cnt == 0 || ht->ht_valid_cnt > 0);
			ASSERT(ht->ht_valid_cnt >= 0);
			ASSERT(ht->ht_busy > 0);
			if (ht->ht_valid_cnt > 0)
				break;
			if (ht->ht_busy > 1)
				break;

			/*
			 * we always release empty shared htables
			 */
			if (!(ht->ht_flags & HTABLE_SHARED_PFN)) {

				/*
				 * don't release if in address space tear down
				 */
				if (hat->hat_flags & HAT_FREEING)
					break;

				/*
				 * At and above max_page_level, free if it's for
				 * a boot-time kernel mapping
				 */
				if ((level >= mmu.max_page_level) &&
				    (hat != kas.a_hat))
					break;
			}

			/*
			 * remember if we destroy an htable that shares its PFN
			 * from elsewhere
			 */
			if (ht->ht_flags & HTABLE_SHARED_PFN) {
				ASSERT(ht->ht_level == 0);
				ASSERT(shared == NULL);
				shared = ht->ht_shares;
				HATSTAT_INC(hs_htable_unshared);
			}

			/*
			 * Handle release of a table and freeing the htable_t.
			 * Unlink it from the table higher (ie. ht_parent).
			 */
			ASSERT(ht->ht_lock_cnt == 0);
			higher = ht->ht_parent;
			ASSERT(higher != NULL);

			/*
			 * Unlink the pagetable.
			 */
			unlink_ptp(higher, ht, va);

			/*
			 * remove this htable from its hash list
			 */
			if (ht->ht_next)
				ht->ht_next->ht_prev = ht->ht_prev;

			if (ht->ht_prev) {
				ht->ht_prev->ht_next = ht->ht_next;
			} else {
				ASSERT(hat->hat_ht_hash[hashval] == ht);
				hat->hat_ht_hash[hashval] = ht->ht_next;
			}
			HTABLE_EXIT(hashval);
			htable_free(ht);
			ht = higher;
		}

		ASSERT(ht->ht_busy >= 1);
		--ht->ht_busy;
		HTABLE_EXIT(hashval);

		/*
		 * If we released a shared htable, do a release on the htable
		 * from which it shared
		 */
		ht = shared;
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_lookup                                     */
/*                                                                  */
/* Function	- Find the htable for the page table at the given   */
/*		  given address. If found acquires a hold that      */
/*		  eventually needs to be htable_released()'d.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

htable_t *
htable_lookup(hat_t *hat, uintptr_t vaddr, level_t level)
{
	uintptr_t	base;
	uint_t		hashval;
	htable_t	*ht = NULL;

	ASSERT(level >= 0);
	ASSERT(level <= mmu.max_level);

	if (level == mmu.max_level)
		base = 0;
	else
		base = vaddr & LEVEL_MASK(level + 1);

	hashval = HTABLE_HASH(hat, base, level);
	HTABLE_ENTER(hashval);
	for (ht = hat->hat_ht_hash[hashval]; ht; ht = ht->ht_next) {
		if (ht->ht_hat == hat &&
		    ht->ht_vaddr == base &&
		    ht->ht_level == level)
			break;
	}
	if (ht)
		++ht->ht_busy;

	HTABLE_EXIT(hashval);
	return (ht);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_acquire                                    */
/*                                                                  */
/* Function	- Acquires a hold on a known htable (from a locked  */
/*		  hment entry).                		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
htable_acquire(htable_t *ht)
{
	hat_t		*hat = ht->ht_hat;
	level_t		level = ht->ht_level;
	uintptr_t	base = ht->ht_vaddr;
	uint_t		hashval = HTABLE_HASH(hat, base, level);

	HTABLE_ENTER(hashval);
#ifdef DEBUG
	/*
	 * make sure the htable is there
	 */
	{
		htable_t	*h;

		for (h = hat->hat_ht_hash[hashval];
		    h && h != ht;
		    h = h->ht_next)
			;
if (h != ht)
prom_printf("h: %p ht: %p base: %lx level: %d hat: %p\n",h,ht,base,level,hat);
		ASSERT(h == ht);
	}
#endif /* DEBUG */
	++ht->ht_busy;
	HTABLE_EXIT(hashval);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_create                                     */
/*                                                                  */
/* Function	- Find the htable for the page table at the given   */
/*		  address. If found acquires a hold that eventually */
/*		  needs to be htable_released()'d. If not found the */
/*		  table is created.            		 	    */
/*		                               		 	    */
/*		  Since we can't hold a hash table mutex during     */
/*		  allocation, we have to drop it and redo the       */
/*		  search on a create. Then we may have to free the  */
/*		  newly allocated htable if another thread raced    */
/*		  in and created it ahead of us.	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

htable_t *
htable_create(
	hat_t		*hat,
	uintptr_t	vaddr,
	level_t		level,
	htable_t	*shared)
{
	uint_t		h;
	level_t		l;
	uintptr_t	base;
	htable_t	*ht;
	htable_t	*higher = NULL;
	htable_t	*new = NULL;

	if (level < 0 || level > mmu.max_level)
		panic("htable_create(): level %d out of range\n", level);

	/*
	 * Create the page tables in top down order.
	 */
	for (l = mmu.max_level; l >= level; --l) {
		new = NULL;
		if (l == mmu.max_level)
			base = 0;
		else
			base = vaddr & LEVEL_MASK(l + 1);

		h = HTABLE_HASH(hat, base, l);
try_again:
		/*
		 * look up the htable at this level
		 */
		HTABLE_ENTER(h);
		if (l == mmu.max_level) {
			ht = hat->hat_htable;
		} else {
			for (ht = hat->hat_ht_hash[h]; ht; ht = ht->ht_next) {
				ASSERT(ht->ht_hat == hat);
				if (ht->ht_vaddr == base &&
				    ht->ht_level == l)
					break;
			}
		}

		/*
		 * if we found the htable, increment its busy cnt
		 * and if we had allocated a new htable, free it.
		 */
		if (ht != NULL) {
			/*
			 * If we find a pre-existing shared table, it must
			 * share from the same place.
			 */
			if (l == level && shared && ht->ht_shares &&
			    ht->ht_shares != shared) {
				panic("htable shared from wrong place "
				    "found htable=%p shared=%p", ht, shared);
			}
			++ht->ht_busy;
			HTABLE_EXIT(h);
			if (new)
				htable_free(new);
			if (higher != NULL)
				htable_release(higher);
			higher = ht;

		/*
		 * if we didn't find it on the first search
		 * allocate a new one and search again
		 */
		} else if (new == NULL) {
			HTABLE_EXIT(h);
			new = htable_alloc(hat, base, l,
			    l == level ? shared : NULL);
			goto try_again;

		/*
		 * 2nd search and still not there, use "new" table
		 * Link new table into higher, when not at top level.
		 */
		} else {
			ht = new;
			if (higher != NULL) {
				link_ptp(higher, ht, base);
				ht->ht_parent = higher;

			}
			ht->ht_next = hat->hat_ht_hash[h];
			ASSERT(ht->ht_prev == NULL);
			if (hat->hat_ht_hash[h])
				hat->hat_ht_hash[h]->ht_prev = ht;
			hat->hat_ht_hash[h] = ht;
			HTABLE_EXIT(h);

			/*
			 * Note we don't do htable_release(higher).
			 * That happens recursively when "new" is removed by
			 * htable_release() or htable_steal().
			 */
			higher = ht;

			/*
			 * If we just created a new shared page table we
			 * increment the shared htable's busy count, so that
			 * it can't be the victim of a steal even if it's empty.
			 */
			if (l == level && shared) {
				(void) htable_lookup(shared->ht_hat,
				    shared->ht_vaddr, shared->ht_level);
				HATSTAT_INC(hs_htable_shared);
			}
		}
	}

	return (ht);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_scan                                       */
/*                                                                  */
/* Function	- Walk through a given htable looking for the first */
/*		  valid entry. This routine takes both a starting   */
/*		  and ending address. The starting address is re-   */
/*		  quired to be within the htable provided by the    */
/*		  caller, but there is no such restriction on the   */
/*		  ending address.              		 	    */
/*		                               		 	    */
/*		  If the routine finds a valid entry in the htable  */
/*		  (at or beyond the starting address), the PTE (and */
/*		  its address) will be returned. This PTE may       */
/*		  correspond to either a page or a page table - it  */
/*		  is the caller's responsibility to determine which.*/
/*		  If no valid entry is found, 0 (an invalid PTE)    */
/*		  and the next unexamined address will be returned. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static s390xpte_t
htable_scan(htable_t *ht, uintptr_t *vap, uintptr_t eaddr)
{
	uint_t 	e;
	s390xpte_t found_pte = (s390xpte_t) PT_INVALID;
	s390xpte_t *pte_ptr,
		   *end_pte_ptr;
	int 	l = ht->ht_level;
	uintptr_t va = *vap & LEVEL_MASK(l);
	size_t 	pgsize = LEVEL_SIZE(l);

	ASSERT(va >= ht->ht_vaddr);
	ASSERT(va <= HTABLE_LAST_PAGE(ht));

	/*
	 * Compute the starting index and ending virtual address
	 */
	e = htable_va2entry(va, ht);

	pte_ptr     = s390xpte_access_pagetable(ht);
	end_pte_ptr = pte_ptr + ht->ht_num_ptes;
	pte_ptr    += e;

	while (!PTE_ISVALID(*pte_ptr, l)) {
		va += pgsize;
		if (va >= eaddr)
			break;
		pte_ptr++;
		ASSERT(pte_ptr <= end_pte_ptr);
		if (pte_ptr == end_pte_ptr)
			break;
	}

	/*
	 * if we found a valid PTE, load the entire PTE
	 */
	if (va < eaddr && pte_ptr != end_pte_ptr) {
		found_pte = *pte_ptr;
	}
	s390xpte_release_pagetable(ht);

	*vap = va;
	return (found_pte);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_walk                                       */
/*                                                                  */
/* Function	- Find the address and htable for the first pop-    */
/*		  ulated translation at or above the given virtual  */
/*		  address. The caller may also specify an upper     */
/*		  limit to the address range to search. Uses level  */
/*		  information to quickly skip unpopulated sections  */
/*		  of virtual address spaces.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

s390xpte_t
htable_walk(
	struct hat *hat,
	htable_t **htp,
	uintptr_t *vaddr,
	uintptr_t eaddr)
{
	uintptr_t va = *vaddr;
	htable_t *ht;
	htable_t *prev = *htp;
	level_t l;
	level_t max_mapped_level;
	s390xpte_t pte;

	ASSERT(eaddr > va);

	/*
	 * If we're coming in with a previous page table, search it first
	 * without doing an htable_lookup(), this should be frequent.
	 */
	if (prev) {
		ASSERT(prev->ht_busy > 0);
		ASSERT(prev->ht_vaddr <= va);
		l = prev->ht_level;
		if (va <= HTABLE_LAST_PAGE(prev)) {
			pte = htable_scan(prev, &va, eaddr);

			if (PTE_ISPAGE(pte, l)) {
				*vaddr = va;
				*htp = prev;
				return (pte);
			}
		}

		/*
		 * We found nothing in the htable provided by the caller,
		 * so fall through and do the full search
		 */
		htable_release(prev);
	}

	/*
	 * Find the level of the largest pagesize used by this HAT.
	 */
	max_mapped_level = 0;
	for (l = 1; l <= mmu.max_page_level; ++l)
		if (hat->hat_pages_mapped[l] != 0)
			max_mapped_level = l;

	while (va < eaddr && va >= *vaddr) {

		/*
		 *  Find lowest table with any entry for given address.
		 */
		for (l = 0; l <= mmu.max_level; ++l) {
			ht = htable_lookup(hat, va, l);
			if (ht != NULL) {
				pte = htable_scan(ht, &va, eaddr);
				if (PTE_ISPAGE(pte, l)) {
					*vaddr = va;
					*htp = ht;
					return (pte);
				}
				htable_release(ht);
				break;
			}

			/*
			 * The ht is never NULL at the top level since
			 * the top level htable is created in hat_alloc().
			 */
			ASSERT(l < mmu.max_level);

			/*
			 * No htable covers the address. If there is no
			 * larger page size that could cover it, we
			 * skip to the start of the next page table.
			 */
			if (l >= max_mapped_level) {
				va = NEXT_ENTRY_VA(va, l + 1);
			}
		}
	}

	*vaddr = 0;
	*htp = NULL;
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_getpte                                     */
/*                                                                  */
/* Function	- Find the htable and page table entry index of the */
/*		  given virtual address with page size at or below  */
/*		  at or below given level.     		 	    */
/*		                               		 	    */
/*		  If not found returns NULL. When found, returns    */
/*		  the htable, sets entry and has a hold on the      */
/*		  table.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

htable_t *
htable_getpte(
	struct hat *hat,
	uintptr_t vaddr,
	uint_t *entry,
	s390xpte_t *pte,
	level_t level)
{
	htable_t	*ht;
	level_t		l;
	uint_t		e;

	ASSERT(level <= mmu.max_page_level);

	for (l = 0; l <= level; ++l) {
		ht = htable_lookup(hat, vaddr, l);
		if (ht == NULL)
			continue;
		e = htable_va2entry(vaddr, ht);
		if (entry != NULL)
			*entry = e;
		if (pte != NULL)
			*pte = s390xpte_get(ht, e);
		return (ht);
	}
	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_getpage                                    */
/*                                                                  */
/* Function	- Find the htable and page table entry index of the */
/*		  given virtual address. There must be a valid page */
/*		  mapped at the given address. If not found returns */
/*		  NULL. When found, returns the htable, sets entry, */
/*		  and has a hold on the htable.		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

htable_t *
htable_getpage(struct hat *hat, uintptr_t vaddr, uint_t *entry)
{
	htable_t	*ht;
	uint_t		e;
	s390xpte_t	pte;

	ht = htable_getpte(hat, vaddr, &e, &pte, mmu.max_page_level);
	if (ht == NULL)
		return (NULL);

	if (entry)
		*entry = e;

	if (PTE_ISPAGE(pte, ht->ht_level))
		return (ht);
	htable_release(ht);
	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_init                                       */
/*                                                                  */
/* Function	- Initialize htable structures and cache.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
htable_init()
{
	int	kmem_flags = KMC_NOHASH;

	/*
	 * initialize kmem caches
	 */
	htable_cache = kmem_cache_create("htable_t",
	    sizeof (htable_t), 0, NULL, NULL,
	    htable_reap, NULL, hat_memload_arena, kmem_flags);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_va2entry                                   */
/*                                                                  */
/* Function	- Get the PTE index for the virtual address in the  */
/*		  given htable's page table.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint_t
htable_va2entry(uintptr_t va, htable_t *ht)
{
	level_t	l = ht->ht_level;

	ASSERT(va >= ht->ht_vaddr);
	ASSERT(va <= HTABLE_LAST_PAGE(ht));
	return ((va >> LEVEL_SHIFT(l)) & (ht->ht_num_ptes - 1));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_e2va                                       */
/*                                                                  */
/* Function	- Given an htable and the index of a pte in it,     */
/*		  return the virtual address of the page.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uintptr_t
htable_e2va(htable_t *ht, uint_t entry)
{
	level_t	l = ht->ht_level;
	uintptr_t va;

	ASSERT(entry < ht->ht_num_ptes);
	va = ht->ht_vaddr + ((uintptr_t)entry << LEVEL_SHIFT(l));

	return (va);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_cpu_init                                 */
/*                                                                  */
/* Function	- Initialize a CPU private window for mapping page  */
/*		  tables. There will be 3 total pages of address-   */
/*		  ing needed:                  		 	    */
/*		                               		 	    */
/*		  1 for r/w access to page tables	 	    */
/*		  1 for r/o access when copying page tables 	    */
/*		  1 that will map the PTEs for the 1st 2, so we can */
/*		    access them quickly.       		 	    */
/*		                               		 	    */
/*		  We use vmem_xalloc() to get a correct alignment   */
/*		  so that only one hat_mempte_setup() is needed.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
s390xpte_cpu_init(cpu_t *cpu, void *pages)
{
	struct hat_cpu_info *hci;
	caddr_t va;

	/*
	 * We can't use kmem_alloc/vmem_alloc for the 1st CPU, as this is
	 * called before we've activated our own HAT
	 */
	if (pages != NULL) {
		hci = &init_hci;
		va = pages;
	} else {
		hci = kmem_alloc(sizeof (struct hat_cpu_info), KM_SLEEP);
		va = vmem_xalloc(heap_arena, 3 * MMU_PAGESIZE, MMU_PAGESIZE, 0,
		    LEVEL_SIZE(1), NULL, NULL, VM_SLEEP);
	}
	mutex_init(&hci->hci_mutex, NULL, MUTEX_DEFAULT, NULL);

	cpu->cpu_hat_info = hci;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_access_pagetable                         */
/*                                                                  */
/* Function	- Disable preemption and establish a mapping to the */
/*		  page table with the given pfn. This is optimized  */
/*		  for the case where it's the same pfn as we last   */
/*		  used referenced from this CPU.	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static __inline__ s390xpte_t *
s390xpte_access_pagetable(htable_t *ht)
{
	pfn_t pfn;

	pfn = ht->ht_pfn;
	ASSERT(pfn != PFN_INVALID);
	return ((s390xpte_t *) ht->ht_org);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_release_pagetable                        */
/*                                                                  */
/* Function	- Release access to a page table.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static __inline__ void
s390xpte_release_pagetable(htable_t *ht)
{
	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_get                                      */
/*                                                                  */
/* Function	- Atomic retrieval of a page table entry.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

s390xpte_t
s390xpte_get(htable_t *ht, uint_t entry)
{
	s390xpte_t	pte;
	s390xpte_t	*ptep;

	ptep = s390xpte_access_pagetable(ht);
	pte  = *(ptep + entry);
	s390xpte_release_pagetable(ht);
	return (pte);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_set                                      */
/*                                                                  */
/* Function	- Atomic unconditional set of a page table entry,   */
/*		  it returns the previous value.	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

s390xpte_t
s390xpte_set(htable_t *ht, uint_t entry, s390xpte_t new, void *ptr)
{
	s390xpte_t	old,
			*ptep, 
			*base;

	ASSERT(!(ht->ht_flags & HTABLE_SHARED_PFN));
	base = s390xpte_access_pagetable(ht);
	ptep = (void *)((caddr_t)base + (entry << mmu.pte_size_shift));
	if (!(PTE_EQUIV(*ptep, new, ht->ht_level))) {
		__sync_lock_test_and_set(&ht->ht_lock, 1);
		LOCK_HT(ht);
		old   = invalidateRSP(base, entry, ptep, ht);
		*ptep = new;
		UNLOCK_HT(ht);
	}
	else
		old = new;

	s390xpte_release_pagetable(ht);

	return (old);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_invalidate_pfn                           */
/*                                                                  */
/* Function	- Invalidate a page table entry if it currently     */
/*		  maps the given pfn. This returns the previous     */
/*		  value of the PTE.            		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

s390xpte_t
s390xpte_invalidate_pfn(htable_t *ht, uint_t entry)
{
	s390xpte_t	*ptep,
			*base,
			old_pte;

	ASSERT(!(ht->ht_flags & HTABLE_SHARED_PFN));
	base    = s390xpte_access_pagetable(ht);
	ptep    = base + entry; 
	LOCK_HT(ht);
	old_pte = invalidateRSP(base, entry, ptep, ht);
	UNLOCK_HT(ht);
	s390xpte_release_pagetable(ht);

	return (old_pte);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_update                                   */
/*                                                                  */
/* Function	- Update a PTE and invalidate any stale TLB entries.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

s390xpte_t
s390xpte_update(htable_t *ht, uint_t entry, s390xpte_t expected, s390xpte_t new)
{
	s390xpte_t	*ptep,
			*base;
	hat_t		*hat;
	uintptr_t	addr;

	ASSERT(!(ht->ht_flags & HTABLE_SHARED_PFN));
	base = s390xpte_access_pagetable(ht);
	ptep = (void *)((caddr_t) base + (entry << mmu.pte_size_shift));

	if (!(PTE_EQUIV(*ptep, new, ht->ht_level))) {
		LOCK_HT(ht);
		invalidateRSP(base, entry, ptep, ht);
		*ptep = new;
		UNLOCK_HT(ht);
	}
	s390xpte_release_pagetable(ht);

	return (new);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_copy                                     */
/*                                                                  */
/* Function	- Copy page tables - this is just a little more     */
/*		  complicated than the previous routines. Note that */
/*		  it's also not atomic! 			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
s390xpte_copy(htable_t *src, htable_t *dest, uint_t entry, uint_t count)
{
	struct hat_cpu_info *hci;
	caddr_t	src_va;
	caddr_t dst_va;
	size_t size;

	ASSERT(!(src->ht_flags & HTABLE_SHARED_PFN));
	ASSERT(!(dest->ht_flags & HTABLE_SHARED_PFN));

	/*
	 * Acquire access to the CPU pagetable window for the destination.
	 */
	dst_va = (caddr_t)s390xpte_access_pagetable(dest);
	src_va = (caddr_t)s390xpte_access_pagetable(src);

	/*
	 * now do the copy
	 */

	dst_va += entry << mmu.pte_size_shift;
	src_va += entry << mmu.pte_size_shift;
	size = count << mmu.pte_size_shift;
	bcopy(src_va, dst_va, size);

	s390xpte_release_pagetable(dest);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390xpte_zero                                     */
/*                                                                  */
/* Function	- Zero page table entries - Note this doesn't use   */
/*		  atomic stores!               		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
s390xpte_zero(htable_t *dest, uint_t entry, uint_t count)
{
	caddr_t dst_va;
	s390xpte_t *p;
	size_t size;

prom_printf("pte_zero\n");
	/*
	 * Map in the page table to be zeroed.
	 */
	ASSERT(!(dest->ht_flags & HTABLE_SHARED_PFN));
	dst_va = (caddr_t)s390xpte_access_pagetable(dest);
	dst_va += entry << mmu.pte_size_shift;
	size = count << mmu.pte_size_shift;
	bzero(dst_va, size);

	s390xpte_release_pagetable(dest);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_dump                                          */
/*                                                                  */
/* Function	- Called to ensure that all page tables are in the  */
/*		  system dump.                 		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_dump(void)
{
	hat_t *hat;
	uint_t h;
	htable_t *ht;

	/*
	 * Dump all page tables
	 */
	for (hat = kas.a_hat; hat != NULL; hat = hat->hat_next) {
		for (h = 0; h < hat->hat_num_hash; ++h) {
			for (ht = hat->hat_ht_hash[h]; ht; ht = ht->ht_next) {
				dump_page(ht->ht_pfn);
			}
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- htable_attach.                                    */
/*                                                                  */
/* Function	- Attach all the htable entities created during boot*/
/*		  too the kernel address space hat.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
htable_attach()
{
	hat_t 	*hat;
	uint_t 	h,
		entry;
	int    	level;
	pfn_t	pfn;
	htable_t *ht,
		 *next,
		 *prev;
	hment_t  *hm;
	page_t	 *pp;

	hat		= kas.a_hat;
	hat->hat_htable = RSP[4].ht;
	hat->hat_rto	= RSP[4].ht->ht_org;
	for (level = mmu.max_level; level >= 0; level--) {
		for (ht = RSP[level].ht; ht != NULL; ht = next) {
			next 	    = ht->ht_next;
			ht->ht_next = NULL;
			ht->ht_prev = NULL;
			ht->ht_hat  = hat;
			ht->ht_busy = 1;
			h = HTABLE_HASH(hat, ht->ht_vaddr, ht->ht_level);
			HTABLE_ENTER(h);
			ht->ht_next = hat->hat_ht_hash[h];
			ASSERT(ht->ht_prev == NULL);
			if (hat->hat_ht_hash[h])
				hat->hat_ht_hash[h]->ht_prev = ht;
			hat->hat_ht_hash[h] = ht;
			HTABLE_EXIT(h);
		}
	
	}
}

/*========================= End of Function ========================*/
