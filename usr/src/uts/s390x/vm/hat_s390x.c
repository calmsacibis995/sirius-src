/*------------------------------------------------------------------*/
/* 								    */
/* Name        - hat_s390x.c.					    */
/* 								    */
/* Function    - VM - Hardware Address Translation management.	    */
/* 								    */
/* 		 Implementation of the interfaces described in 	    */
/*		 <common/vm/hat.h>				    */
/* 								    */
/* 		 Nearly all the details of how the hardware is 	    */
/*		 managed should not be visible outside this layer   */
/*		 except for misc. machine specific functions that   */
/*		 work in conjunction with this code.		    */
/* 								    */
/* 		 Routines used only inside of s390x/vm start with   */
/*		 hati_ for HAT Internal.			    */
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

#define	MAX_UNLOAD_CNT (8)

#define	HAT_LOAD_ATTR	1
#define	HAT_SET_ATTR	2
#define	HAT_CLR_ATTR	3

#define	HASH_MAX_LENGTH 4

//
// Storage key modification and reference bits
//
#define SK_MOD	0x02
#define SK_REF	0x04

//
// Macros to atomically get, clear, and set bits in page_t
//
#define PP_GETRM(pp, rmmask)	(pp->p_nrm & rmmask)
#define PP_ISMOD(pp)		PP_GETRM(pp, P_MOD)
#define PP_ISREF(pp)		PP_GETRM(pp, P_REF)
#define PP_ISRO(pp)		PP_GETRM(pp, P_RO)

#define PP_SETRM(pp, rm)	__sync_or_and_fetch(&pp->p_nrm, rm)
#define PP_SETMOD(pp)		PP_SETRM(pp, P_MOD)
#define PP_SETREF(pp)		PP_SETRM(pp, P_REF)
#define PP_SETRO(pp)		PP_SETRM(pp, P_RO)

#define PP_CLRRM(pp, rm)	__sync_and_and_fetch(&pp->p_nrm, ~(rm))
#define PP_CLRMOD(pp)		PP_CLRRM(pp, P_MOD)
#define PP_CLRREF(pp)		PP_CLRRM(pp, P_REF)
#define PP_CLRRO(pp)		PP_CLRRM(pp, P_RO)
#define PP_CLRALL(pp)		PP_CLRRM(pp, P_MOD | P_REF | P_RO)

//
// Some useful tracing macros
//

#ifdef DEBUG

# define HATIN(r, h, a, l) 	\
	if (hattrace) prom_printf("->%s hat=%p, adr=%p, len=%lx\n", #r, h, a, l)

# define HATOUT(r, h, a) 	\
	if (hattrace) prom_printf("<-%s hat=%p, adr=%p\n", #r, h, a)

#else

# define HATIN(r, h, a, l)
# define HATOUT(r, h, a)

#endif
	
#define	REMAPASSERT(EX)	if (!(EX)) panic("hati_pte_map: " #EX)

/*
 * The t_hatdepth field is an 8-bit counter.  We use the lower seven bits
 * to track exactly how deep we are in the memload->kmem_alloc recursion.
 * If the depth is greater than 1, that indicates that we are performing a
 * hat operation to satisfy another hat operation.  To prevent infinite
 * recursion, we switch over to using pre-allocated "reserves" of htables
 * and hments.
 *
 * The uppermost bit is used to indicate that we are transitioning away
 * from being the reserves thread.  See hati_reserves_exit() for the
 * details.
 */
#define	EXITING_FLAG		(1 << 7)
#define	DEPTH_MASK		(~EXITING_FLAG)
#define	HAT_DEPTH(t)		((t)->t_hatdepth & DEPTH_MASK)
#define	EXITING_RESERVES(t)	((t)->t_hatdepth & EXITING_FLAG)

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
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
#include <sys/shm.h>
#include <sys/sysmacros.h>
#include <sys/machparam.h>
#include <sys/vmem.h>
#include <sys/vmsystm.h>
#include <sys/promif.h>
#include <sys/var.h>
#include <sys/atomic.h>
#include <sys/bitmap.h>
#include <sys/machs390x.h>
#include <sys/spl.h>

#include <vm/seg_kmem.h>
#include <vm/hat_pte.h>
#include <vm/htable.h>
#include <vm/hat_s390x.h>
#include <vm/hment.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/page.h>
#include <vm/seg_kp.h>
#include <vm/seg_kpm.h>
#include <vm/vm_dep.h>

#include <sys/cmn_err.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*
 * Call back range information for unloads
 */
typedef struct range_info {
	uintptr_t	rng_va;
	ulong_t		rng_cnt;
	level_t		rng_level;
} range_info_t;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern ulong_t	po_share;
extern int	vpm_enable;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void hat_updateattr(hat_t *, caddr_t, size_t, uint_t, int);
static void hati_sync_key(page_t *, uintptr_t);
static void hati_memload_plist(hat_t *, caddr_t, size_t, page_t *,
			       uint_t, uint_t);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*
 * Flags for hat_memload/hat_devload/hat_*attr.
 *
 * 	HAT_LOAD	Default flags to load a translation to the page.
 *
 * 	HAT_LOAD_LOCK	Lock down mapping resources; hat_map(), hat_memload(),
 *			and hat_devload().
 *
 *	HAT_LOAD_NOCONSIST Do not add mapping to page_t mapping list.
 *
 *	HAT_LOAD_SHARE	A flag to hat_memload() to indicate h/w page tables
 *			that map some user pages (not kas) is shared by more
 *			than one process (eg. ISM).
 *
 *	HAT_LOAD_REMAP	Reload a valid pte with a different page frame.
 *
 *	HAT_NO_KALLOC	Do not kmem_alloc while creating the mapping; at this
 *			point, it's setting up mapping to allocate internal
 *			hat layer data structures.  This flag forces hat layer
 *			to tap its reserves in order to prevent infinite
 *			recursion.
 *
 * The following is a protection attribute (like PROT_READ, etc.)
 *
 *	HAT_NOSYNC	set PT_NOSYNC (soft bit) - this mapping's ref/mod bits
 *			are never cleared.
 *
 * Installing new valid PTE's and creation of the mapping list
 * entry are controlled under the same lock. It's derived from the
 * page_t being mapped.
 */
static uint_t supported_memload_flags =
	HAT_LOAD | HAT_LOAD_LOCK | HAT_LOAD_ADV | HAT_LOAD_NOCONSIST |
	HAT_LOAD_SHARE | HAT_NO_KALLOC | HAT_LOAD_REMAP | HAT_LOAD_TEXT;

/*
 * Advisory ordering attributes. Apply only to device mappings.
 *
 * HAT_STRICTORDER: the CPU must issue the references in order, as the
 *	programmer specified.  This is the default.
 * HAT_UNORDERED_OK: the CPU may reorder the references (this is all kinds
 *	of reordering; store or load with store or load).
 * HAT_MERGING_OK: merging and batching: the CPU may merge individual stores
 *	to consecutive locations (for example, turn two consecutive byte
 *	stores into one halfword store), and it may batch individual loads
 *	(for example, turn two consecutive byte loads into one halfword load).
 *	This also implies re-ordering.
 * HAT_LOADCACHING_OK: the CPU may cache the data it fetches and reuse it
 *	until another store occurs.  The default is to fetch new data
 *	on every load.  This also implies merging.
 * HAT_STORECACHING_OK: the CPU may keep the data in the cache and push it to
 *	the device (perhaps with other data) at a later time.  The default is
 *	to push the data right away.  This also implies load caching.
 *
 * Equivalent of hat_memload(), but can be used for device memory where
 * there are no page_t's and we support additional flags (write merging, etc).
 * Note that we can have large page mappings with this interface.
 */
static int supported_devload_flags = HAT_LOAD | HAT_LOAD_LOCK |
	HAT_LOAD_NOCONSIST | HAT_STRICTORDER | HAT_UNORDERED_OK |
	HAT_MERGING_OK | HAT_LOADCACHING_OK | HAT_STORECACHING_OK;

struct hat_mmu_info mmu;

static s390xpte_t hati_update_pte(htable_t *, uint_t, s390xpte_t, s390xpte_t);

//
// Locks etc., to control use of the hat reserves when recursively
// allocating page tables for the hat data structures
//
static kmutex_t hat_reserves_lock;
static kcondvar_t hat_reserves_cv;
kthread_t *hat_reserves_thread;

//
// A CPUSET for all CPUs. This is used for kernel address SIGP calls, 
// since kernel addresses apply to all CPUs. Much of the work can be 
// achieved by CSP instructions but others require cross-CPU calls.
//
cpuset_t khat_cpuset;

//
// Serialization resources for managing hat structures
//
kmutex_t hat_list_lock;
kcondvar_t hat_list_cv;
kmem_cache_t *hat_cache,
	     *hat_hash_cache;

//
// Simple statistics
//
struct hatstats hatstat;

//
// Control tracing for DEBUG code
//
int hattrace = 0;
      

//
// If in case one day s390x supports > 4K page sizes
//
uint_t disable_auto_data_large_pages 	= 1;
uint_t disable_auto_text_large_pages 	= 1;
uint_t disable_ism_large_pages		= 1;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_constructor.                                 */
/*                                                                  */
/* Function	- Cache constructor for struct hat.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
hati_constructor(void *buf, void *handle, int kmflags)
{
	hat_t	*hat = buf;

	mutex_init(&hat->hat_mutex, NULL, MUTEX_DEFAULT, NULL);
	bzero(hat->hat_pages_mapped, sizeof(pgcnt_t));
	hat->hat_stats = 0;
	hat->hat_flags = 0;
	hat->hat_htable  = NULL;
	hat->hat_ht_hash = NULL;
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_mkpte.                                       */
/*                                                                  */
/* Function	- Utility to return a valid s390xpte_t from         */
/*		  protections, pfn, and level number.		    */			
/*		                               		 	    */
/*------------------------------------------------------------------*/

static s390xpte_t
hati_mkpte(pfn_t pfn, uint_t attr, level_t level, uint_t flags)
{
	s390xpte_t	pte;
	uint_t		cache_attr = attr & HAT_ORDER_MASK;
	uint64_t	key;

	pte = MAKEPTE(pfn, level);

	if (level == 0) {

		key = 0x00;

		if (!(attr & PROT_EXEC))
			PTE_SET(pte, PT_NX);
//		else
//			FIXME - secondary address space stuff here?		

		if (flags & HAT_LOAD_NOCONSIST) {
			PTE_SET(pte, PT_NOCONSIST);
			key |= SK_REF | SK_MOD;
		} else if (attr & HAT_NOSYNC) {
			PTE_SET(pte, PT_NOSYNC);
			key |= SK_REF | SK_MOD;
		}

		if (attr & PROT_WRITE) {
			PTE_SET(pte, PT_WRITABLE);
			PTE_CLR(pte, PG_PROTECT);
		} else {
			PTE_SET(pte, PG_PROTECT);
		}

		SET_KEY((pfn << MMU_PAGESHIFT), key);

	}

	return (pte);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_pte_map.                                     */
/*                                                                  */
/* Function	- Do the low-level work to get a mapping entered    */
/*		  into a HAT's page tables and in the mapping list  */
/*		  of the associated page_t.    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hati_pte_map(
	htable_t	*ht,
	uint_t		entry,
	page_t		*pp,
	s390xpte_t	pte,
	int		flags,
	void		*pte_ptr)
{
	hat_t		*hat = ht->ht_hat;
	s390xpte_t	old_pte;
	level_t		l = ht->ht_level;
	hment_t		*hm;
	uint_t		is_consist;

	/*
	 * Is this a consistant (ie. need mapping list lock) mapping?
	 */
	is_consist = (pp != NULL && (flags & HAT_LOAD_NOCONSIST) == 0);

	/*
	 * Track locked mapping count in the htable.  Do this first,
	 * as we track locking even if there already is a mapping present.
	 */
	if ((flags & HAT_LOAD_LOCK) != 0 && hat != kas.a_hat)
		HTABLE_LOCK_INC(ht);

	/*
	 * Acquire the page's mapping list lock and get an hment to use.
	 * Note that hment_prepare() might return NULL.
	 */
	if (is_consist) {
		s390x_hm_enter(pp);
		hm = hment_prepare(ht, entry, pp);
	}

	/*
	 * Set the new pte, retrieving the old one at the same time.
	 */
	old_pte = s390xpte_set(ht, entry, pte, pte_ptr);

	/*
	 * If the mapping didn't change there is nothing more to do.
	 */
	if (PTE_SAME(pte, old_pte, l)) {
		if (is_consist) {
			s390x_hm_exit(pp);
			if (hm != NULL)
				hment_free(hm);
		}
		return;
	}

	/*
	 * Install a new mapping in the page's mapping list
	 */
	if (!PTE_ISVALID(old_pte, l)) {
		if (is_consist) {
			hment_assign(ht, entry, pp, hm);
			s390x_hm_exit(pp);
		} else {
			ASSERT(flags & HAT_LOAD_NOCONSIST);
		}
		HTABLE_INC(ht->ht_valid_cnt);
		PGCNT_INC(hat, l);
		return;
	}

	/*
	 * Remap's are more complicated:
	 *  - HAT_LOAD_REMAP must be specified if changing the pfn.
	 *    We also require that NOCONSIST be specified.
	 *  - Otherwise only permission or caching bits may change.
	 */
	if (!PTE_ISPAGE(old_pte, l))
		panic("non-null/page mapping pte=" FMT_PTE, old_pte);

	if (PTE2PFN(old_pte, l) != PTE2PFN(pte, l)) {
		REMAPASSERT(flags & HAT_LOAD_REMAP);
		REMAPASSERT(flags & HAT_LOAD_NOCONSIST);
		REMAPASSERT(pf_is_memory(PTE2PFN(old_pte, l)) ==
		    pf_is_memory(PTE2PFN(pte, l)));
		REMAPASSERT(!is_consist);
	}

	/*
	 * A remap requires invalidating the TLBs, since remapping the
	 * same PFN requires NOCONSIST, we don't have to sync R/M bits.
	 */
	if (!(hat->hat_flags & HAT_FREEING))
		ipte(ht->ht_org, entry);

	/*
	 * We don't create any mapping list entries on a remap, so release
	 * any allocated hment after we drop the mapping list lock.
	 */
	if (is_consist) {
		s390x_hm_exit(pp);
		if (hm != NULL)
			hment_free(hm);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_reserves_enter                               */
/*                                                                  */
/* Function	- Access to reserves for HAT_NO_KALLOC is single    */
/*		  threaded. If someone else is in the reserves,     */
/*		  we'll politely wait for them to finish. This      */
/*		  keeps normal hat_memload()'s from getting up the  */
/*		  mappings needed to replenish the reserve.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hati_reserves_enter(uint_t kmem_for_hat)
{
	/*
	 * 64 is an arbitrary number to catch serious problems.  I'm not
	 * sure what the absolute maximum depth is, but it should be
	 * substantially less than this.
	 */
	ASSERT(HAT_DEPTH(curthread) < 64);

	/*
	 * If we are doing a memload to satisfy a kmem operation, we enter
	 * the reserves immediately; we don't wait to recurse to a second
	 * level of memload.
	 */
	ASSERT(kmem_for_hat < 2);
	curthread->t_hatdepth += (1 + kmem_for_hat);

	if (hat_reserves_thread == curthread)
		return;

	if (HAT_DEPTH(curthread) > 1 || hat_reserves_thread != NULL) {
		mutex_enter(&hat_reserves_lock);
		while (hat_reserves_thread != NULL)
			cv_wait(&hat_reserves_cv, &hat_reserves_lock);

		if (HAT_DEPTH(curthread) > 1)
			hat_reserves_thread = curthread;

		mutex_exit(&hat_reserves_lock);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_reserves_exit                                */
/*                                                                  */
/* Function	- If we are the reserves_thread and we've finally   */
/*		  finished with all our memloads (i.e. no longer    */
/*		  doing hat slabs), we can release our use of the   */
/*		  reserve.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hati_reserves_exit(uint_t kmem_for_hat)
{
	ASSERT(kmem_for_hat < 2);
	curthread->t_hatdepth -= (1 + kmem_for_hat);

	/*
	 * Simple case: either we are not the reserves thread, or we are
	 * the reserves thread and we are nested deeply enough that we
	 * should still be the reserves thread.
	 *
	 * Note: we may not become the reserves thread after we recursively
	 * enter our second HAT routine, but we don't stop being the
	 * reserves thread until we exit the toplevel HAT routine.  This is
	 * to work around vmem's inability to determine when an allocation
	 * should be satisfied from the hat_memload arena, which can lead
	 * to an infinite loop of memload->vmem_populate->memload->.
	 */
	if (curthread != hat_reserves_thread || HAT_DEPTH(curthread) > 0)
		return;

	mutex_enter(&hat_reserves_lock);
	ASSERT(hat_reserves_thread == curthread);
	hat_reserves_thread = NULL;
	cv_broadcast(&hat_reserves_cv);
	mutex_exit(&hat_reserves_lock);

	/*
	 * As we leave the reserves, we want to be sure the reserve lists
	 * aren't overstocked.  Freeing excess reserves requires that we
	 * call kmem_free(), which may require additional allocations,
	 * causing us to re-enter the reserves.  To avoid infinite
	 * recursion, we only try to adjust reserves at the very top level.
	 */
	if (!kmem_for_hat && !EXITING_RESERVES(curthread)) {
		curthread->t_hatdepth |= EXITING_FLAG;
		htable_adjust_reserve();
		hment_adjust_reserve();
		curthread->t_hatdepth &= (~EXITING_FLAG);
	}

	/*
	 * just in case something went wrong in doing adjust reserves
	 */
	ASSERT(hat_reserves_thread != curthread);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_load_common.                                 */
/*                                                                  */
/* Function	- Internal routine to load a single page table      */
/*		  entry.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hati_load_common(
	hat_t		*hat,
	uintptr_t	va,
	page_t		*pp,
	uint_t		attr,
	uint_t		flags,
	level_t		level,
	pfn_t		pfn)
{
	htable_t	*ht;
	uint_t		entry;
	s390xpte_t	pte;
	uint_t		kmem_for_hat = (flags & HAT_NO_KALLOC) ? 1 : 0;

	ASSERT(hat == kas.a_hat ||
	    AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));

	if (flags & HAT_LOAD_SHARE)
		hat->hat_flags |= HAT_SHARED;
	/*
	 * Find the page table that maps this page if it already exists.
	 */
	ht = htable_lookup(hat, va, level);

	/*
	 * All threads go through hati_reserves_enter() to at least wait
	 * for any existing reserves user to finish. This helps reduce
	 * pressure on the reserves. In addition, if this thread needs
	 * to become the new reserve user it will.
	 */
	hati_reserves_enter(kmem_for_hat);

	/*
	 * Kernel memloads for HAT data should never use hments!
	 * If it did that would seriously complicate the reserves system, since
	 * hment_alloc() would need to know about HAT_NO_KALLOC.
	 *
	 * We also must have HAT_LOAD_NOCONSIST if page_t is NULL.
	 */
	if (HAT_DEPTH(curthread) > 1 || pp == NULL)
		flags |= HAT_LOAD_NOCONSIST;

	if (ht == NULL) {
		ht = htable_create(hat, va, level, NULL);
		ASSERT(ht != NULL);
	}
	entry = htable_va2entry(va, ht);

	/*
	 * a bunch of paranoid error checking
	 */
	ASSERT(ht->ht_busy > 0);
	if (ht->ht_vaddr > va || va > HTABLE_LAST_PAGE(ht))
		panic("hati_load_common: bad htable %p, va %p", ht, (void *)va);
	ASSERT(ht->ht_level == level);

	pte = hati_mkpte(pfn, attr, level, flags);

	/*
	 * establish the mapping
	 */
	hati_pte_map(ht, entry, pp, pte, flags, NULL);

	/*
	 * release the htable and any reserves
	 */
	htable_release(ht);
	hati_reserves_exit(kmem_for_hat);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_page_clrwrt.                                 */
/*                                                                  */
/* Function	- Remove write permission from all mappings to a    */
/*		  page.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hati_page_clrwrt(struct page *pp)
{
	hment_t		*hm = NULL;
	htable_t	*ht;
	uint_t		entry;
	s390xpte_t	pte,
			new;

	/*
	 * walk thru the mapping list clearing write permission
	 */
	s390x_hm_enter(pp);
	while ((hm = hment_walk(pp, &ht, &entry, hm)) != NULL) {
		pte = new = s390xpte_get(ht, entry);
		if ((PTE_ISVALID(pte, ht->ht_level)) &&
		    (pte & PT_WRITABLE)) {
			new &= ~PT_WRITABLE;
			new |= PG_PROTECT;
			s390xpte_update(ht, entry, pte, new);
		}
	}
	s390x_hm_exit(pp);
	CLR_KEY((pp->p_pagenum << MMU_PAGESHIFT));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_alloc.                                        */
/*                                                                  */
/* Function	- Allocate a hat structure for address space "as".  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hat_t *
hat_alloc(struct as *as)
{
	hat_t		*hat;
	htable_t	*ht;		// Top level htable 
	uint_t		use_vlp;
	page_t		*pp;

	ASSERT(AS_WRITE_HELD(as, &as->a_lock));
	hat = kmem_cache_alloc(hat_cache, KM_SLEEP);
	hat->hat_as = as;
	mutex_init(&hat->hat_mutex, NULL, MUTEX_DEFAULT, NULL);
	ASSERT(hat->hat_flags == 0);

	/*
	 * Allocate the htable hash
	 */
	hat->hat_num_hash  = mmu.hash_cnt;
	hat->hat_ht_hash   = kmem_cache_alloc(hat_hash_cache, KM_SLEEP);
	bzero(hat->hat_ht_hash, hat->hat_num_hash * sizeof (htable_t *));

	hat->hat_htable    = NULL;
	hat->hat_ht_cached = NULL;

	CPUSET_ZERO(hat->hat_cpus);

	/*
	 * Allocate a top level htable to kick start the DAT table creation
	 */
	ht                 = htable_create(hat, (uintptr_t)0, mmu.max_level, NULL);
	hat->hat_htable    = ht;
	hat->hat_rto	   = ht->ht_org;
	ht->ht_spfn	   = 0;
	memset(&hat->hat_pages_mapped, 0, sizeof(hat->hat_pages_mapped));

	/*
	 * Put it at the start of the global list of all hats (used by stealing)
	 *
	 * kas.a_hat is not in the list but is instead used to find the
	 * first and last items in the list.
	 *
	 * - kas.a_hat->hat_next points to the start of the user hats.
	 *   The list ends where hat->hat_next == NULL
	 *
	 * - kas.a_hat->hat_prev points to the last of the user hats.
	 *   The list begins where hat->hat_prev == NULL
	 */
	mutex_enter(&hat_list_lock);
	hat->hat_prev = NULL;
	hat->hat_next = kas.a_hat->hat_next;
	if (hat->hat_next)
		hat->hat_next->hat_prev = hat;
	else
		kas.a_hat->hat_prev = hat;
	kas.a_hat->hat_next = hat;

	mutex_exit(&hat_list_lock);

	return (hat);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_free_start.                                   */
/*                                                                  */
/* Function	- Process has finished executing but has not been   */
/*		  cleaned up yet.              		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_free_start(hat_t *hat)
{
	_pfxPage *pfx = NULL;

	ASSERT(AS_WRITE_HELD(hat->hat_as, &hat->hat_as->a_lock));

	/*
	 * If the hat is currently a stealing victim, wait for the stealing
	 * to finish.  Once we mark it as HAT_FREEING, htable_steal()
	 * won't look at its pagetables anymore.
	 */
	mutex_enter(&hat_list_lock);
	__asm__ ("	lgr	0,%0\n"
		 "	lghi	1,0\n"
		 "	tracg	0,1,%1\n"
		 : : "r" (hat), "m" (pfx->__lc_hat_trace) : "0", "1");
	while (hat->hat_flags & HAT_VICTIM)
		cv_wait(&hat_list_cv, &hat_list_lock);
	hat->hat_flags |= HAT_FREEING;
	mutex_exit(&hat_list_lock);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_free_end.                                     */
/*                                                                  */
/* Function	- An address space is being destroyed, so we de-    */
/*		  stroy the associated hat.    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_free_end(hat_t *hat)
{
	_pfxPage *pfx = NULL;

	ASSERT(hat->hat_flags & HAT_FREEING);

	/*
	 * must not be running on the given hat
	 */
	ASSERT(CPU->cpu_current_hat != hat);

	__asm__ ("	lgr	0,%0\n"
		 "	lghi	1,1\n"
		 "	tracg	0,1,%1\n"
		 : : "r" (hat), "m" (pfx->__lc_hat_trace) : "0", "1");
	/*
	 * Remove it from the list of HATs
	 */
	mutex_enter(&hat_list_lock);
	if (hat->hat_prev)
		hat->hat_prev->hat_next = hat->hat_next;
	else
		kas.a_hat->hat_next = hat->hat_next;
	if (hat->hat_next)
		hat->hat_next->hat_prev = hat->hat_prev;
	else
		kas.a_hat->hat_prev = hat->hat_prev;
	mutex_exit(&hat_list_lock);
	hat->hat_next = hat->hat_prev = NULL;

	/*
	 * Make a pass through the htables freeing them all up.
	 */
	htable_purge_hat(hat);
	kmem_cache_free(hat_hash_cache, hat->hat_ht_hash);
	hat->hat_ht_hash = NULL;

	hat->hat_flags = 0;
	kmem_cache_free(hat_cache, hat);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mmu_init.                                         */
/*                                                                  */
/* Function	- Initialize mmu data structures based on machine   */
/*		  characteristics.             		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mmu_init()
{
	uint_t max_htables;

	mmu.highest_pfn    = mmu_btop(-1);

	/*
	 * Compute how many hash table entries to have per process for htables.
	 * We start with 1 page's worth of entries.
	 *
	 * If physical memory is small, reduce the amount need to cover it.
	 */
	max_htables  = physmax / mmu.ptes_per_table[0];
	mmu.hash_cnt = MMU_PAGESIZE / sizeof (htable_t *);
	while (mmu.hash_cnt > 16 && mmu.hash_cnt >= max_htables)
		mmu.hash_cnt >>= 1;

	while (mmu.hash_cnt * HASH_MAX_LENGTH < max_htables)
		mmu.hash_cnt <<= 1;

	mmu_exported_page_sizes = mmu.max_page_level + 1;

	/* restrict legacy applications from using pagesizes 1g and above */
	mmu_legacy_page_sizes =
	    (mmu_exported_page_sizes > 2) ? 2 : mmu_exported_page_sizes;

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_init.                                         */
/*                                                                  */
/* Function	- Initialize hat data structures.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_init()
{
	cv_init(&hat_list_cv, NULL, CV_DEFAULT, NULL);

	/*
	 * initialize kmem caches
	 */
	htable_init();
	hment_init();

	hat_cache = kmem_cache_create("hat_t",
	    sizeof (hat_t), 0, hati_constructor, NULL, NULL,
	    NULL, 0, 0);

	hat_hash_cache = kmem_cache_create("HatHash",
	    mmu.hash_cnt * sizeof (htable_t *), 0, NULL, NULL, NULL,
	    NULL, 0, 0);

	/*
	 * Set up the kernel's hat
	 */
	AS_LOCK_ENTER(&kas, &kas.a_lock, RW_WRITER);
	kas.a_hat = kmem_cache_alloc(hat_cache, KM_NOSLEEP);
	mutex_init(&kas.a_hat->hat_mutex, NULL, MUTEX_DEFAULT, NULL);
	kas.a_hat->hat_as = &kas;
	kas.a_hat->hat_flags = 0;
	AS_LOCK_EXIT(&kas, &kas.a_lock);

	CPUSET_ZERO(khat_cpuset);
	CPUSET_ADD(khat_cpuset, CPU->cpu_id);

	/*
	 * The kernel hat's next pointer serves as the head of the hat list .
	 * The kernel hat's prev pointer tracks the last hat on the list for
	 * htable_steal() to use.
	 */
	kas.a_hat->hat_next = NULL;
	kas.a_hat->hat_prev = NULL;

	/*
	 * Allocate an htable hash bucket for the kernel
	 */
	kas.a_hat->hat_num_hash = mmu.hash_cnt;
	kas.a_hat->hat_ht_hash  = kmem_cache_alloc(hat_hash_cache, KM_NOSLEEP);
	bzero(kas.a_hat->hat_ht_hash, mmu.hash_cnt * sizeof (htable_t *));

	/*
	 * zero out the top level and cached htable pointers
	 */
	kas.a_hat->hat_ht_cached = NULL;
	kas.a_hat->hat_htable    = NULL;

	htable_initial_reserve(0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_switch.                                       */
/*                                                                  */
/* Function	- Switch to a new active hat, maintaining bit masks */
/*		  to track active CPUs.        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_switch(hat_t *hat)
{
	uintptr_t	newcr;
	cpu_t		*cpu = CPU;
	hat_t		*old = cpu->cpu_current_hat;

	/*
	 * set up this information first, so we don't miss any cross calls
	 */
	if (old != NULL) {
		if (old == hat)
			return;
		if (old != kas.a_hat) 
{
uint32_t *eye;
			CPUSET_ATOMIC_DEL(old->hat_cpus, cpu->cpu_id);
eye = (uint32_t *) &old->hat_cpus;
if ((*eye == 0xdeadbeee) || (*eye == 0xdeadbeed) || (*eye == 0xdeadbeef)) {
msgnoh("***** hat: %p cpu: %p thread: %p - hat_t has been freed *****",
old,cpu,curthread);
dumpCPU();
}
}
	}

	if (hat != kas.a_hat) 
		CPUSET_ATOMIC_ADD(hat->hat_cpus, cpu->cpu_id);

	cpu->cpu_current_hat = hat;

	/*
	 * If we are switching to kernel then CR7 already contains
	 * address of the highest level region table. We just need 
	 * to set our address space mode to 'home'
	 */
	if (hat == kas.a_hat) {
	} else {
		/*
		 * Otherwise we load CR7/13 with region table representing
		 * the address space
		 */
		newcr = MAKECR(hat->hat_htable->ht_pfn) | 0x0f | ASCE_SAE;
		set_cr(newcr, 7);
		set_cr(newcr, 13);
		ptlb();
#if 0
		/*
		 * Otherwise we load CR1 with region table representing
		 * the execution space and CR13 with the region table
		 * representing the stack space
		 */
		newcr1  = MAKECR(hat->hat_htable->ht_pfn) | 0x0f;
		set_cr(newcr1, 1);
		if (hat->hat_htable->ht_spfn != 0) {
			newcr13 = MAKECR(hat->hat_htable->ht_spfn) | 0x0f;
			set_cr(newcr13, 13);
		}
#endif
	}
	ASSERT(cpu == CPU);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_dup.                                          */
/*                                                                  */
/* Function	- Duplicate address translations of the parent to   */
/*		  the child. This function really isn't used any-   */
/*		  more.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hat_dup(hat_t *old, hat_t *new, caddr_t addr, size_t len, uint_t flag)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_swapin.                                       */
/*                                                                  */
/* Function	- Allocate any hat resources required for a process */
/*		  being swapped in.            		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_swapin(hat_t *hat)
{
	/*----------------------------------------------------------*/
	/* Do nothing: we let everything fault back in...	    */
	/*----------------------------------------------------------*/
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_swapout.                                      */
/*                                                                  */
/* Function	- Unload all translations associated with an        */
/*		  address space of a process that is being swapped  */
/*		  out.                         		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_swapout(hat_t *hat)
{
	uintptr_t	vaddr = (uintptr_t)0;
	uintptr_t	eaddr = _userlimit;
	htable_t	*ht = NULL;
	level_t		l;

	/*----------------------------------------------------------*/
	/* We can't just call hat_unload(hat, 0, _userlimit...)     */
	/* here, because seg_spt and shared pagetables can't be     */
	/* swapped out.						    */
	/* Take a look at segspt_shmswapout() - it's a big no-op.   */
	/*							    */
	/* Instead we'll walk through all the address space and     */
	/* unload any mappings which we are sure are not shared and */
	/* not locked.						    */
	/*----------------------------------------------------------*/
	
	ASSERT(IS_PAGEALIGNED(vaddr));
	ASSERT(IS_PAGEALIGNED(eaddr));
	ASSERT(AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));
	if ((uintptr_t)hat->hat_as->a_userlimit < eaddr)
		eaddr = (uintptr_t)hat->hat_as->a_userlimit;

	while (vaddr < eaddr) {
		(void) htable_walk(hat, &ht, &vaddr, eaddr);
		if (ht == NULL)
			break;

		/*
		 * If the page table is shared skip its entire range.
		 * This code knows that only segment tables are shared
		 */
		l = ht->ht_level;
		if (ht->ht_flags & HTABLE_SHARED_PFN) {
			ASSERT(l == 1);
			vaddr = ht->ht_vaddr + MMU_PAGESIZE;
			htable_release(ht);
			ht = NULL;
			continue;
		}

		/*
		 * If the page table has no locked entries, unload this one.
		 */
		if (ht->ht_lock_cnt == 0)
			hat_unload(hat, (caddr_t)vaddr, MMU_PAGESIZE,
			    HAT_UNLOAD_UNMAP);

		/*
		 * If we have a level 0 page table with locked entries,
		 * skip the entire page table, otherwise skip just one entry.
		 */
		if (ht->ht_lock_cnt > 0 && l == 0)
			vaddr = ht->ht_vaddr + MMU_PAGESIZE; 
		else
			vaddr += MMU_PAGESIZE;
	}
	if (ht)
		htable_release(ht);

	/*
	 * We're in swapout because the system is low on memory, so
	 * go back and flush all the htables off the cached list.
	 */
	htable_purge_hat(hat);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_get_mapped_size.                              */
/*                                                                  */
/* Function	- Returns the number of bytes that have valid map-  */
/*		  pings in hat.                		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

size_t
hat_get_mapped_size(hat_t *hat)
{
	size_t total = 0;
	return (total);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_stats_enable.                                 */
/*                                                                  */
/* Function	- Enable/disable collection of stats for hat.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hat_stats_enable(hat_t *hat)
{
	__sync_add_and_fetch(&hat->hat_stats, 1);
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_stats_disable                                 */
/*                                                                  */
/* Function	- Disable collection of stats for hat.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_stats_disable(hat_t *hat)
{
	__sync_sub_and_fetch(&hat->hat_stats, 1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_pte_unmap                                     */
/*                                                                  */
/* Function	- Interior routine for HAT_UNLOADs from hat_unload_ */
/*		  callback(), hat_kmap_unload(), or hat_steal()     */
/*		  code. This routine doesn't handle releasing of    */
/*		  the htables.                 		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_pte_unmap(
	htable_t	*ht,
	uint_t		entry,
	uint_t		flags,
	s390xpte_t	old_pte,
	void		*pte_ptr)
{
	hat_t		*hat = ht->ht_hat;
	hment_t		*hm = NULL;
	page_t		*pp = NULL;
	level_t		l = ht->ht_level;
	pfn_t		pfn;

	/*
	 * We always track the locking counts, even if nothing is unmapped
	 */
	if ((flags & HAT_UNLOAD_UNLOCK) != 0 && hat != kas.a_hat) {
		ASSERT(ht->ht_lock_cnt > 0);
		HTABLE_LOCK_DEC(ht);
	}

	/*
	 * Figure out which page's mapping list lock to acquire using the PFN
	 * passed in "old" PTE. We then attempt to invalidate the PTE.
	 * If another thread, probably a hat_pageunload, has asynchronously
	 * unmapped/remapped this address we'll loop here.
	 */
	ASSERT(ht->ht_busy > 0);
	while (PTE_ISVALID(old_pte, l)) {
		pfn = PTE2PFN(old_pte, l);
		if (PTE_GET(old_pte, PT_SOFTWARE) >= PT_NOCONSIST) {
			pp = NULL;
		} else {
			pp = page_numtopp_nolock(pfn);
			if (pp == NULL) {
				panic("no page_t, not NOCONSIST: old_pte="
				    FMT_PTE " ht=%lx entry=0x%x pte_ptr=%lx",
				    old_pte, (uintptr_t)ht, entry,
				    (uintptr_t)pte_ptr);
			}
			s390x_hm_enter(pp);
		}

		/*
		 * If freeing the address space, check that the PTE
		 * hasn't changed, as the mappings are no longer in use by
		 * any thread, invalidation is unnecessary.
		 * If not freeing, do a full invalidate.
		 */
		if (hat->hat_flags & HAT_FREEING)
			old_pte = s390xpte_get(ht, entry);
		else
			old_pte = s390xpte_invalidate_pfn(ht, entry);

		/*
		 * If the page hadn't changed we've unmapped it and can proceed
		 */
		if (PTE_ISVALID(old_pte, l) && PTE2PFN(old_pte, l) == pfn)
			break;

		/*
		 * Otherwise, we'll have to retry with the current old_pte.
		 * Drop the hment lock, since the pfn may have changed.
		 */
		if (pp != NULL) {
			s390x_hm_exit(pp);
			pp = NULL;
		}
	}

	/*
	 * If the old mapping wasn't valid, there's nothing more to do
	 */
	if (!PTE_ISVALID(old_pte, l)) {
		if (pp != NULL)
			s390x_hm_exit(pp);
		return;
	}

	/*
	 * Remove the hment
	 */
	if (pp != NULL) {
		if (!(flags & HAT_UNLOAD_NOSYNC))
			hati_sync_key(pp, old_pte);
		hm = hment_remove(pp, ht, entry);
		s390x_hm_exit(pp);
		if (hm != NULL)
			hment_free(hm);
	}

	/*
	 * Handle book keeping in the htable and hat
	 */
	ASSERT(ht->ht_valid_cnt > 0);
	HTABLE_DEC(ht->ht_valid_cnt);
	PGCNT_DEC(hat, l);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_memload.                                      */
/*                                                                  */
/* Function	- Load a translation to the given page struct.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_memload(
	hat_t		*hat,
	caddr_t		addr,
	page_t		*pp,
	uint_t		attr,
	uint_t		flags)
{
	uintptr_t	va = (uintptr_t)addr;
	level_t		level = 0;
	pfn_t		pfn = page_pptonum(pp);

	HATIN(hat_memload, hat, addr, (size_t)MMU_PAGESIZE);
	ASSERT(IS_PAGEALIGNED(va));
	ASSERT(hat == kas.a_hat ||
	    AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));
	ASSERT((flags & supported_memload_flags) == flags);

	ASSERT(!PP_ISFREE(pp));

	/*
	 * This is used for memory with normal caching enabled, so
	 * always set HAT_STORECACHING_OK.
	 */
	attr |= HAT_STORECACHING_OK;
	hati_load_common(hat, va, pp, attr, flags, level, pfn);
	HATOUT(hat_memload, hat, addr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_memload_region.                               */
/*                                                                  */
/* Function	- Load the given array of page structures.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
hat_memload_region(struct hat *hat, caddr_t addr, struct page *pp,
    uint_t attr, uint_t flags, hat_region_cookie_t rcookie)
{
	hat_memload(hat, addr, pp, attr, flags);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_memload_array.                                */
/*                                                                  */
/* Function	- Load the given array of page structures.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_memload_array(
	hat_t		*hat,
	caddr_t		addr,
	size_t		len,
	page_t		**pages,
	uint_t		attr,
	uint_t		flags)
{
	uintptr_t	va = (uintptr_t)addr;
	uintptr_t	eaddr = va + len;
	level_t		level = 0;
	pgcnt_t		pgindx = 0;
	pfn_t		pfn;
	pgcnt_t		i;

	HATIN(hat_memload_array, hat, addr, len);
	ASSERT(IS_PAGEALIGNED(va));
	ASSERT(hat == kas.a_hat ||
	    AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));
	ASSERT((flags & supported_memload_flags) == flags);

	/*
	 * memload is used for memory with full caching enabled, so
	 * set HAT_STORECACHING_OK.
	 */
	attr |= HAT_STORECACHING_OK;

	/*
	 * handle all pages using largest possible pagesize
	 */
	while (va < eaddr) {
		/*
		 * decide what level mapping to use (ie. pagesize)
		 */
		pfn = page_pptonum(pages[pgindx]);

		/*
		 * load this page mapping
		 */
		hati_load_common(hat, va, pages[pgindx], attr, flags,
		    level, pfn);

		/*
		 * move to next page
		 */
		va += MMU_PAGESIZE;
		pgindx += mmu_btop(MMU_PAGESIZE);
	}
	HATOUT(hat_memload_array, hat, addr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_memload_array_region.                         */
/*                                                                  */
/* Function	- Load the given array of page structures.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
hat_memload_array_region(struct hat *hat, caddr_t addr, size_t len,
    struct page **pps, uint_t attr, uint_t flags,
    hat_region_cookie_t rcookie)
{
	hat_memload_array(hat, addr, len, pps, attr, flags);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_devload.                                      */
/*                                                                  */
/* Function	- Load/lock the given page frame number.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_devload(
	hat_t		*hat,
	caddr_t		addr,
	size_t		len,
	pfn_t		pfn,
	uint_t		attr,
	int		flags)
{
	uintptr_t	va = ALIGN2PAGE(addr);
	uintptr_t	eva = va + len;
	level_t		level = 0;
	size_t		pgsize;
	page_t		*pp;
	int		f;	/* per PTE copy of flags  - maybe modified */
	uint_t		a;	/* per PTE copy of attr */

	HATIN(hat_devload, hat, addr, len);
	ASSERT(IS_PAGEALIGNED(va));
	ASSERT(hat == kas.a_hat ||
	    AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));
	ASSERT((flags & supported_devload_flags) == flags);

	/*
	 * handle all pages
	 */
	while (va < eva) {

		/*
		 * If it is memory get page_t and allow caching (this happens
		 * for the nucleus pages) - though HAT_PLAT_NOCACHE can be used
		 * to override that. If we don't have a page_t, make sure
		 * NOCONSIST is set.
		 */
		a = attr;
		f = flags;
		if (pf_is_memory(pfn)) {
			if (!(a & HAT_PLAT_NOCACHE))
				a |= HAT_STORECACHING_OK;

			if (f & HAT_LOAD_NOCONSIST)
				pp = NULL;
			else
				pp = page_numtopp_nolock(pfn);
		} else {
			pp = NULL;
			f |= HAT_LOAD_NOCONSIST;
		}

		/*
		 * load this page mapping
		 */
		hati_load_common(hat, va, pp, a, f, level, pfn);

		/*
		 * move to next page
		 */
		va  += MMU_PAGESIZE;
		pfn += mmu_btop(MMU_PAGESIZE);
	}
	HATOUT(hat_devload, hat, addr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_memload_plist.                               */
/*                                                                  */
/* Function	- Load the given list of page structures.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hati_memload_plist(
	hat_t		*hat,
	caddr_t		addr,
	size_t		len,
	page_t		*pages,
	uint_t		attr,
	uint_t		flags)
{
	uintptr_t	va = (uintptr_t)addr;
	uintptr_t	eaddr = va + len;
	level_t		level = 0;
	pfn_t		pfn;
	pgcnt_t		iPage;
	page_t		*pp;

	HATIN(hati_memload_plist, hat, addr, len);
	ASSERT(IS_PAGEALIGNED(va));
	ASSERT(hat == kas.a_hat ||
	    AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));
	ASSERT((flags & supported_memload_flags) == flags);

	/*
	 * memload is used for memory with full caching enabled, so
	 * set HAT_STORECACHING_OK.
	 */
	attr |= HAT_STORECACHING_OK;

	/*
	 * handle all pages using largest possible pagesize
	 */
	while (va < eaddr) {
		pp = pages;
		page_sub(&pages, pp);
		page_io_unlock(pp);
		page_hashout(pp, NULL);
		page_downgrade(pp);

		/*
		 * decide what level mapping to use (ie. pagesize)
		 */
		pfn = page_pptonum(pp);

		/*
		 * load this page mapping
		 */
		hati_load_common(hat, va, pp, attr, flags, level, pfn);

		/*
		 * move to next page
		 */
		va += MMU_PAGESIZE;
	}
	HATOUT(hati_memload_plist, hat, addr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_unlock.                                       */
/*                                                                  */
/* Function	- Unlock the mappings to a given range of addresses.*/
/*		  Locks are tracked by ht_lock_cnt in the htable.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_unlock(hat_t *hat, caddr_t addr, size_t len)
{
	uintptr_t	vaddr = (uintptr_t)addr;
	uintptr_t	eaddr = vaddr + len;
	htable_t	*ht = NULL;

	/*
	 * kernel entries are always locked, we don't track lock counts
	 */
	ASSERT(IS_PAGEALIGNED(vaddr));
	ASSERT(IS_PAGEALIGNED(eaddr));
	if (hat == kas.a_hat)
		return;

	if (eaddr > _userlimit)
		panic("hat_unlock() address out of range - above _userlimit");

	ASSERT(AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));
	while (vaddr < eaddr) {
		(void) htable_walk(hat, &ht, &vaddr, eaddr);
		if (ht == NULL)
			break;

		if (ht->ht_lock_cnt < 1)
			panic("hat_unlock(): lock_cnt < 1, "
			    "htable=%p, vaddr=%p\n", ht, (caddr_t)vaddr);
		HTABLE_LOCK_DEC(ht);

		vaddr += MMU_PAGESIZE;
	}
	if (ht)
		htable_release(ht);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_unload.                                       */
/*                                                                  */
/* Function	- Unload a range of virtual address space (no       */
/*		  callback).                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_unload(hat_t *hat, caddr_t addr, size_t len, uint_t flags)
{
	uintptr_t va = (uintptr_t)addr;

	hat_unload_callback(hat, addr, len, flags, NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- handle_ranges                                     */
/*                                                                  */
/* Function	- Do the callbacks for ranges being unloaded.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
handle_ranges(hat_callback_t *cb, uint_t cnt, range_info_t *range)
{
	/*
	 * do callbacks to upper level VM system
	 */
	while (cb != NULL && cnt > 0) {
		--cnt;
		cb->hcb_start_addr = (caddr_t)range[cnt].rng_va;
		cb->hcb_end_addr = cb->hcb_start_addr;
		cb->hcb_end_addr +=
		    range[cnt].rng_cnt << LEVEL_SIZE(range[cnt].rng_level);
		cb->hcb_function(cb);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_unload_callback.                              */
/*                                                                  */
/* Function	- Unload a given range of addresses (has optional   */
/*		  callback).                   		 	    */
/*		                               		 	    */
/* 		  Flags:					    */
/* 			HAT_UNLOAD		0x00		    */
/* 			HAT_UNLOAD_NOSYNC	0x02		    */
/* 			HAT_UNLOAD_UNLOCK	0x04		    */
/* 			HAT_UNLOAD_OTHER	0x08 - unused	    */
/* 			HAT_UNLOAD_UNMAP	0x10 - = HAT_UNLOAD */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_unload_callback(
	hat_t		*hat,
	caddr_t		addr,
	size_t		len,
	uint_t		flags,
	hat_callback_t	*cb)
{
	uintptr_t	vaddr = (uintptr_t)addr;
	uintptr_t	eaddr = vaddr + len;
	htable_t	*ht = NULL;
	uint_t		entry;
	uintptr_t	contig_va = (uintptr_t)-1L;
	range_info_t	r[MAX_UNLOAD_CNT];
	uint_t		r_cnt = 0;
	s390xpte_t	old_pte;

	HATIN(hat_unload_callback, hat, addr, len);
	ASSERT(IS_PAGEALIGNED(vaddr));
	ASSERT(IS_PAGEALIGNED(eaddr));

	/*
	 * Special case a single page being unloaded for speed. This happens
	 * quite frequently, COW faults after a fork() for example.
	 */
	if (cb == NULL && len == MMU_PAGESIZE) {
		ht = htable_getpte(hat, vaddr, &entry, &old_pte, 0);
		if (ht != NULL) {
			if (PTE_ISVALID(old_pte, ht->ht_level))
				hat_pte_unmap(ht, entry, flags, old_pte, NULL);
			htable_release(ht);
		}
		return;
	}

	while (vaddr < eaddr) {
		old_pte = htable_walk(hat, &ht, &vaddr, eaddr);
		if (ht == NULL)
			break;

		/*
		 * We'll do the call backs for contiguous ranges
		 */
		if (vaddr != contig_va ||
		    (r_cnt > 0 && r[r_cnt - 1].rng_level != ht->ht_level)) {
			if (r_cnt == MAX_UNLOAD_CNT) {
				handle_ranges(cb, r_cnt, r);
				r_cnt = 0;
			}
			r[r_cnt].rng_va = vaddr;
			r[r_cnt].rng_cnt = 0;
			r[r_cnt].rng_level = ht->ht_level;
			++r_cnt;
		}

		/*
		 * Unload one mapping from the page tables.
		 */
		entry     = htable_va2entry(vaddr, ht);
		hat_pte_unmap(ht, entry, flags, old_pte, NULL);
		ASSERT(ht->ht_level <= mmu.max_page_level);
		vaddr    += LEVEL_SIZE(ht->ht_level);
		contig_va = vaddr;
		++r[r_cnt - 1].rng_cnt;
	}
	if (ht)
		htable_release(ht);

	/*
	 * handle last range for callbacks
	 */
	if (r_cnt > 0)
		handle_ranges(cb, r_cnt, r);

	HATOUT(hat_unload_callback, hat, addr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_sync.                                         */
/*                                                                  */
/* Function	- Synchronize mapping with software data struct-    */
/*		  ures. Currently this interface is only used by    */
/*		  the working set monitor driver.	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_sync(hat_t *hat, caddr_t addr, size_t len, uint_t flags)
{
	uintptr_t	vaddr = (uintptr_t) addr,
			eaddr = vaddr + len,
			raddr;
	htable_t	*ht = NULL;
	uint_t		entry,
			key;
	page_t		*pp;
	s390xpte_t	pte,
			save_pte;

	ASSERT(IS_PAGEALIGNED(vaddr));
	ASSERT(IS_PAGEALIGNED(eaddr));

	for (; vaddr < eaddr; vaddr += MMU_PAGESIZE) {
retry:
		pte = htable_walk(hat, &ht, &vaddr, eaddr);
		if (ht == NULL)
			break;
		entry = htable_va2entry(vaddr, ht);

		GET_KEY(pte, key);

		if ((PTE_GET(pte, PT_SOFTWARE) >= PT_NOSYNC) ||
		    (key & (SK_MOD | SK_REF) == 0))
			continue;

		/*
		 * We need to acquire the mapping list lock to protect
		 * against hat_pageunload(), hat_unload(), etc.
		 */
		pp = page_numtopp_nolock(PTE2PFN(pte, ht->ht_level));
		if (pp == NULL)
			break;
		s390x_hm_enter(pp);
		save_pte = pte;
		pte = s390xpte_get(ht, entry);
		if (pte != save_pte) {
			s390x_hm_exit(pp);
			goto retry;
		}

		GET_KEY(pte, key);

		if ((PTE_GET(pte, PT_SOFTWARE) >= PT_NOSYNC) ||
		    (key & (SK_MOD | SK_REF) == 0)) {
			s390x_hm_exit(pp);
			continue;
		}

		/*
		 * Need to clear ref or mod bits. We may compete with
		 * hardware updating the R/M bits and have to try again.
		 */
		if (flags == HAT_SYNC_ZERORM) {
			CLR_KEY((uintptr_t) pte);
		} else {
			/*
			 * sync the storage key to the page_t
			 */
			hati_sync_key(pp, save_pte);
		}
		s390x_hm_exit(pp);
	}

	if (ht)
		htable_release(ht);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_map.                                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_map(hat_t *hat, caddr_t addr, size_t len, uint_t flags)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_getattr.                                      */
/*                                                                  */
/* Function	- Returns attr for <hat,addr> in *attr. Returns 0   */
/*		  if there was a mapping and *attr is valid, non-   */
/*		  zero if there was no mapping and *attr is not     */
/*		  valid.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint_t
hat_getattr(hat_t *hat, caddr_t addr, uint_t *attr)
{
	uintptr_t	vaddr = ALIGN2PAGE(addr);
	htable_t	*ht = NULL;
	s390xpte_t	pte;

	ht = htable_getpte(hat, vaddr, NULL, &pte, 0);
	if (ht == NULL)
		return ((uint_t)-1);

	if (!PTE_ISVALID(pte, 0)) {
		htable_release(ht);
		return ((uint_t)-1);
	}

	*attr = PROT_READ;
	if (PTE_GET(pte, PT_WRITABLE))
		*attr |= PROT_WRITE;
	if (!PTE_GET(pte, PT_NX))
		*attr |= PROT_EXEC;
	if (PTE_GET(pte, PT_SOFTWARE) >= PT_NOSYNC)
		*attr |= HAT_NOSYNC;
	htable_release(ht);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_updateattr.                                   */
/*                                                                  */
/* Function	- Applies the given attribute change to an exist-   */
/*		  ing mapping.                 		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hat_updateattr(hat_t *hat, caddr_t addr, size_t len, uint_t attr, int what)
{
	uintptr_t	vaddr = (uintptr_t) addr,
			eaddr = (uintptr_t) addr + len;
	htable_t	*ht = NULL;
	uint_t		entry,	
			key = 0;
	s390xpte_t	oldpte, 
			newpte;
	page_t		*pp;

	ASSERT(IS_PAGEALIGNED(vaddr));
	ASSERT(IS_PAGEALIGNED(eaddr));
	ASSERT(hat == kas.a_hat ||
	    AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));
	for (; vaddr < eaddr; vaddr += LEVEL_SIZE(ht->ht_level)) {
try_again:
		oldpte = htable_walk(hat, &ht, &vaddr, eaddr);
		if (ht == NULL)
			break;

		if (PTE_GET(oldpte, PT_SOFTWARE) >= PT_NOCONSIST)
			continue;

		pp = page_numtopp_nolock(PTE2PFN(oldpte, ht->ht_level));
		if (pp == NULL)
			continue;
		s390x_hm_enter(pp);

		newpte = oldpte;

		/*
		 * We found a page table entry in the desired range,
		 * figure out the new attributes.
		 */
		if (what == HAT_SET_ATTR || what == HAT_LOAD_ATTR) {
			if (attr & PROT_WRITE) {
				newpte |= PT_WRITABLE;
				newpte &= ~PG_PROTECT;
			}
			else
				newpte |= PG_PROTECT;

			if ((attr & HAT_NOSYNC) &&
			    PTE_GET(oldpte, PT_SOFTWARE) < PT_NOSYNC)
				newpte |= PT_NOSYNC;

			if ((attr & PROT_EXEC) && PTE_GET(oldpte, PT_NX))
				newpte &= ~PT_NX;

#if 0
			if (attr & (P_MOD | P_REF)) {
				long oldkey;
	
				GET_KEY(newpte, key);
	
				key = oldkey;
		
				if (attr & P_MOD)
					key |= SK_MOD;
				if (attr & P_REF)
					key |= SK_REF;

				if (key != oldkey)
					SET_KEY(newpte, key);
			}
#endif
		}

		if (what == HAT_CLR_ATTR) {
			if (attr & PROT_WRITE) {
				newpte &= ~PT_WRITABLE;
				newpte |= PG_PROTECT;
			}

			if ((attr & HAT_NOSYNC) &&
			    PTE_GET(oldpte, PT_SOFTWARE) >= PT_NOSYNC)
				newpte &= ~PT_SOFTWARE;

			if ((attr & PROT_EXEC) && !PTE_GET(oldpte, PT_NX))
				newpte |= PT_NX;
	
#if 0
			if (attr & (P_MOD | P_REF)) {
				long oldkey;
	
				GET_KEY(newpte, key);
	
				key = oldkey;
		
				if (attr & P_MOD)
					key &= ~SK_MOD;
				if (attr & P_REF)
					key &= ~SK_REF;

				if (key != oldkey)
					SET_KEY(newpte, key);
			}
#endif
		}

		/*
		 * Ensure NOSYNC/NOCONSIST mappings have REF and MOD set.
		 */
		if (PTE_GET(newpte, PT_SOFTWARE) >= PT_NOSYNC) {
			key = SK_REF | SK_MOD;
			SET_KEY(newpte, key);
		}

		/*
		 * what about PROT_READ or others? this code only handles:
		 * EXEC, WRITE, NOSYNC
		 */

		/*
		 * If new PTE really changed, update the table.
		 */
		if (newpte != oldpte) {
			entry  = htable_va2entry(vaddr, ht);
			oldpte = hati_update_pte(ht, entry, oldpte, newpte);
			if (oldpte != 0) {
				s390x_hm_exit(pp);
				goto try_again;
			}
		}
		s390x_hm_exit(pp);
	}
	if (ht)
		htable_release(ht);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_setattr.                                      */
/*                                                                  */
/* Function	- 'Set' wrapper for hat_updateattr.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_setattr(hat_t *hat, caddr_t addr, size_t len, uint_t attr)
{
	hat_updateattr(hat, addr, len, attr, HAT_SET_ATTR);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_clrattr.                                      */
/*                                                                  */
/* Function	- 'Clear' wrapper for hat_updateattr.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_clrattr(hat_t *hat, caddr_t addr, size_t len, uint_t attr)
{
	hat_updateattr(hat, addr, len, attr, HAT_CLR_ATTR);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_chgattr.                                      */
/*                                                                  */
/* Function	- 'Change' wrapper for hat_updateattr.              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_chgattr(hat_t *hat, caddr_t addr, size_t len, uint_t attr)
{
	hat_updateattr(hat, addr, len, attr, HAT_LOAD_ATTR);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_chgprot.                                      */
/*                                                                  */
/* Function	- 'Change Protection' wrapper for hat_updateattr.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_chgprot(hat_t *hat, caddr_t addr, size_t len, uint_t vprot)
{
	hat_updateattr(hat, addr, len, vprot & HAT_PROT_MASK, HAT_LOAD_ATTR);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_chgattr_pagedir.                              */
/*                                                                  */
/* Function	- 'Change Page Dir' wrapper for hat_updateattr.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_chgattr_pagedir(hat_t *hat, caddr_t addr, size_t len, uint_t attr)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_getpagesize.                                  */
/*                                                                  */
/* Function	- Returns pagesize in bytes for <hat, addr>. 	    */
/*		  returns -1 of there is no mapping. This is an     */
/*		  advisory call.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

ssize_t
hat_getpagesize(hat_t *hat, caddr_t addr)
{
	size_t		pagesize;
	htable_t	*ht;

	ht = htable_getpage(hat, (uintptr_t) addr, NULL);
	if (ht == NULL)
		return (-1);
	pagesize = LEVEL_SIZE(ht->ht_level);
	htable_release(ht);
	return (pagesize);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_getpfnum.                                     */
/*                                                                  */
/* Function	- Returns pfn for <hat, addr> or PFN_INVALID if     */
/*		  mapping is invalid.          		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pfn_t
hat_getpfnum(hat_t *hat, caddr_t addr)
{
	uintptr_t	vaddr = ALIGN2PAGE(addr);
	htable_t	*ht;
	uint_t		entry;
	pfn_t		pfn = PFN_INVALID;

	ht = htable_getpage(hat, vaddr, &entry);
	if (ht == NULL)
		return (PFN_INVALID);
	ASSERT(vaddr >= ht->ht_vaddr);
	ASSERT(vaddr <= HTABLE_LAST_PAGE(ht));
	pfn = PTE2PFN(s390xpte_get(ht, entry), ht->ht_level);
	if (ht->ht_level > 0)
		pfn += mmu_btop(vaddr & LEVEL_OFFSET(ht->ht_level));
	htable_release(ht);
	return (pfn);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_probe.                                        */
/*                                                                  */
/* Function	- Return 0 if no valid mapping is persent. Faster   */
/*		  version of hat_getattr in certain architectures.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hat_probe(hat_t *hat, caddr_t addr)
{
	uintptr_t	vaddr = ALIGN2PAGE(addr);
	uint_t		entry;
	htable_t	*ht;
	s390xpte_t	pte;

	ASSERT(hat == kas.a_hat ||
	    AS_LOCK_HELD(hat->hat_as, &hat->hat_as->a_lock));

	ht = htable_getpage(hat, vaddr, &entry);
	if (ht == NULL)
		return (0);
	htable_release(ht);
	pte = s390xpte_get(ht, entry);
	if (PTE_ISVALID(pte, 0))
		return (1);
	else
		return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_share.                                        */
/*                                                                  */
/* Function	- Simple implementation of ISM. hat_share() is just */
/*		  like hat_memload_array(), except that we use the  */
/*		  ism_hat's existing mappings to determine the pages*/
/* 		  and protections to use for this hat. In case we   */
/*		  find a properly aligned and sized pagetable of 4K */
/*		  mappings, we will attempt to share the pagetable  */
/* 		  itself.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hat_share(
	hat_t		*hat,
	caddr_t		addr,
	hat_t		*ism_hat,
	caddr_t		src_addr,
	size_t		len,	/* almost useless value, see below.. */
	uint_t		ismszc)
{
	uintptr_t	vaddr_start = (uintptr_t) addr,
			eaddr       = vaddr_start + len,
			vaddr,
			pt_vaddr,
			ism_addr_start = (uintptr_t)src_addr,
			ism_addr       = ism_addr_start,
			e_ism_addr     = ism_addr + len;
	htable_t	*ism_ht = NULL,
			*ht;
	s390xpte_t	pte;
	page_t		*pp;
	pfn_t		pfn;
	level_t		l;
	pgcnt_t		pgcnt;
	uint_t		prot,
			valid_cnt;

	/*
	 * We might be asked to share an empty DISM hat by as_dup()
	 */
	ASSERT(hat != kas.a_hat);
	ASSERT(eaddr <= _userlimit);
	if (!(ism_hat->hat_flags & HAT_SHARED)) {
		ASSERT(hat_get_mapped_size(ism_hat) == 0);
		return (0);
	}

	/*
	 * The SPT segment driver often passes us a size larger than there are
	 * valid mappings. That's because it rounds the segment size up to a
	 * large pagesize, even if the actual memory mapped by ism_hat is less.
	 */
	HATIN(hat_share, hat, addr, len);
	ASSERT(IS_PAGEALIGNED(vaddr_start));
	ASSERT(IS_PAGEALIGNED(ism_addr_start));
	ASSERT(ism_hat->hat_flags & HAT_SHARED);
	while (ism_addr < e_ism_addr) {
		/*
		 * use htable_walk to get the next valid ISM mapping
		 */
		pte = htable_walk(ism_hat, &ism_ht, &ism_addr, e_ism_addr);
		if (ism_ht == NULL)
			break;

		/*
		 * Find the largest page size we can use, based on the
		 * ISM mapping size, our address alignment and the remaining
		 * map length.
		 */
		vaddr = vaddr_start + (ism_addr - ism_addr_start);
		for (l = ism_ht->ht_level; l > 0; --l) {
			if (LEVEL_SIZE(l) <= eaddr - vaddr &&
			    (vaddr & LEVEL_OFFSET(l)) == 0)
				break;
		}

		/*
		 * attempt to share the pagetable
		 *
		 * - only 4K pagetables are shared (ie. level == 0)
		 * - the hat_share() length must cover the whole pagetable
		 * - the shared address must align at level 1
		 * - a shared PTE for this address already exists OR
		 * - no page table for this address exists yet
		 */
		pt_vaddr = vaddr_start + (ism_ht->ht_vaddr - ism_addr_start);
		if (ism_ht->ht_level == 0 &&
		    ism_ht->ht_vaddr + LEVEL_SIZE(1) <= e_ism_addr &&
		    (pt_vaddr & LEVEL_OFFSET(1)) == 0) {

			ht = htable_lookup(hat, pt_vaddr, 0);
			if (ht == NULL)
				ht = htable_create(hat, pt_vaddr, 0, ism_ht);

			if (ht->ht_level > 0 ||
			    !(ht->ht_flags & HTABLE_SHARED_PFN)) {

				htable_release(ht);

			} else {

				/*
				 * share the page table
				 */
				ASSERT(ht->ht_level == 0);
				ASSERT(ht->ht_shares == ism_ht);
				valid_cnt = ism_ht->ht_valid_cnt;
				atomic_add_long(&hat->hat_pages_mapped[0],
				    valid_cnt - ht->ht_valid_cnt);
				ht->ht_valid_cnt = valid_cnt;
				htable_release(ht);
				ism_addr = ism_ht->ht_vaddr + LEVEL_SIZE(1);
				htable_release(ism_ht);
				ism_ht = NULL;
				continue;
			}
		}

		/*
		 * Unable to share the page table. Instead we will
		 * create new mappings from the values in the ISM mappings.
		 *
		 * The ISM mapping might be larger than the share area,
		 * be careful to truncate it if needed.
		 */
		if (eaddr - vaddr >= LEVEL_SIZE(ism_ht->ht_level)) {
			pgcnt = mmu_btop(LEVEL_SIZE(ism_ht->ht_level));
		} else {
			pgcnt = mmu_btop(eaddr - vaddr);
			l = 0;
		}

		pfn = PTE2PFN(pte, ism_ht->ht_level);
		ASSERT(pfn != PFN_INVALID);
		while (pgcnt > 0) {
			/*
			 * Make a new pte for the PFN for this level.
			 * Copy protections for the pte from the ISM pte.
			 */
			pp = page_numtopp_nolock(pfn);
			ASSERT(pp != NULL);

			prot = PROT_READ | HAT_UNORDERED_OK;
			if (PTE_GET(pte, PT_WRITABLE))
				prot |= PROT_WRITE;
			if (!PTE_GET(pte, PT_NX))
				prot |= PROT_EXEC;

			hati_load_common(hat, vaddr, pp, prot, HAT_LOAD, l, pfn);

			vaddr    += LEVEL_SIZE(l);
			ism_addr += LEVEL_SIZE(l);
			pfn	 += mmu_btop(LEVEL_SIZE(l));
			pgcnt    -= mmu_btop(LEVEL_SIZE(l));
		}
	}

	if (ism_ht != NULL)
		htable_release(ism_ht);

	HATOUT(hat_share, hat, addr);
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_unshare.                                      */
/*                                                                  */
/* Function	- hat_unshare() is similar to hat_unload_callback(),*/
/*		  but we have to look for empty shared pagetables.  */
/*		  Note that hat_unshare() is always invoked against */
/*		  an entire segment.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_unshare(hat_t *hat, caddr_t addr, size_t len, uint_t ismszc)
{
	uintptr_t	vaddr = (uintptr_t) addr,
			eaddr = vaddr + len;
	htable_t	*ht = NULL;
	uint_t		need_demaps = 0;

	ASSERT(hat != kas.a_hat);
	HATIN(hat_unshare, hat, addr, len);
	ASSERT(IS_PAGEALIGNED(vaddr));
	ASSERT(IS_PAGEALIGNED(eaddr));

	/*
	 * First go through and remove any shared pagetables.
	 *
	 * Note that it's ok to delay the TLB shootdown till the entire range is
	 * finished, because if hat_pageunload() were to unload a shared
	 * pagetable page, its hat_tlb_inval() will do a global TLB invalidate.
	 */
	while (vaddr < eaddr) {
		/*
		 * find the pagetable that would map the current address
		 */
		ht = htable_lookup(hat, vaddr, 0);
		if (ht != NULL) {
			if (ht->ht_flags & HTABLE_SHARED_PFN) {
				/*
				 * clear mapped pages count, set valid_cnt to 0
				 * and let htable_release() finish the job
				 */
				atomic_add_long(&hat->hat_pages_mapped[0],
						-ht->ht_valid_cnt);
				ht->ht_valid_cnt = 0;
				need_demaps = 1;
			}
			htable_release(ht);
		}
		vaddr = (vaddr & LEVEL_MASK(1)) + LEVEL_SIZE(1);
	}

	/*
	 * flush the TLBs - since we're probably dealing with MANY mappings
	 */
	if (!(hat->hat_flags & HAT_FREEING) && need_demaps)
		ptlb();

	/*
	 * Now go back and clean up any unaligned mappings that
	 * couldn't share pagetables.
	 */
	hat_unload(hat, addr, len, HAT_UNLOAD_UNMAP);

	HATOUT(hat_unshare, hat, addr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_reserve.                                      */
/*                                                                  */
/* Function	- Does nothing.                                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_reserve(struct as *as, caddr_t addr, size_t len)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_page_set_attr.                                */
/*                                                                  */
/* Function	- Set reference/modified bits.                      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_page_setattr(struct page *pp, uint_t flag)
{
	vnode_t		*vp = pp->p_vnode;
	kmutex_t	*vphm = NULL;
	page_t		**listp;
	long		key;

	if (PP_GETRM(pp, flag) == flag)
		return;

	if ((flag & P_MOD) != 0 && vp != NULL && IS_VMODSORT(vp)) {
		vphm = page_vnode_mutex(vp);
		mutex_enter(vphm);
	}

	PP_SETRM(pp, flag);

	if (vphm != NULL) {

		/*
		 * Some File Systems examine v_pages for NULL w/o
		 * grabbing the vphm mutex. Must not let it become NULL when
		 * pp is the only page on the list.
		 */
		if (pp->p_vpnext != pp) {
			page_vpsub(&vp->v_pages, pp);
			if (vp->v_pages != NULL)
				listp = &vp->v_pages->p_vpprev->p_vpnext;
			else
				listp = &vp->v_pages;
			page_vpadd(listp, pp);
		}
		mutex_exit(vphm);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_page_clr_attr.                                */
/*                                                                  */
/* Function	- Clear reference/modified bits.                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_page_clrattr(struct page *pp, uint_t flag)
{
	vnode_t		*vp = pp->p_vnode;
	ASSERT(!(flag & ~(P_MOD | P_REF | P_RO)));

	/*
	 * Caller is expected to hold page's io lock for VMODSORT to work
	 * correctly with pvn_vplist_dirty() and pvn_getdirty() when mod
	 * bit is cleared.
	 * We don't have assert to avoid tripping some existing third party
	 * code. The dirty page is moved back to top of the v_page list
	 * after IO is done in pvn_write_done().
	 */
	PP_CLRRM(pp, flag);

	if ((flag & P_MOD) != 0 && vp != NULL && IS_VMODSORT(vp)) {

		/*
		 * VMODSORT works by removing write permissions and getting
		 * a fault when a page is made dirty. At this point
		 * we need to remove write permission from all mappings
		 * to this page.
		 */
		hati_page_clrwrt(pp);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_page_getattr.                                 */
/*                                                                  */
/* Function	- If flag is specified, returns 0 if attribute is   */
/*		  disabled and non-zero if enabled. If flag spec-   */
/*		  ifies multiple attributes then returns 0 if ALL   */
/*		  attributes are disabled. This is an advisory call.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint_t
hat_page_getattr(struct page *pp, uint_t flag)
{
	return (PP_GETRM(pp, flag));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_page_unmap                                   */
/*                                                                  */
/* Function	- Common code used by hat_pageunload() and          */
/*		  hment_steal().               		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hment_t *
hati_page_unmap(page_t *pp, htable_t *ht, uint_t entry)
{
	s390xpte_t old_pte;
	pfn_t pfn = pp->p_pagenum;
	hment_t *hm;

	/*
	 * We need to acquire a hold on the htable in order to
	 * do the invalidate. We know the htable must exist, since
	 * unmap's don't release the htable until after removing any
	 * hment. Having s390x_hm_enter() keeps that from proceeding.
	 */
	htable_acquire(ht);

	/*
	 * Invalidate the PTE and remove the hment.
	 */
	old_pte = s390xpte_invalidate_pfn(ht, entry);
	if (PTE2PFN(old_pte, ht->ht_level) != pfn) {
		panic("s390xpte_invalidate_pfn() failure found PTE = " FMT_PTE
		    " pfn being unmapped is %lx ht=0x%lx entry=0x%x",
		    old_pte, pfn, (uintptr_t)ht, entry);
	}

	/*
	 * Clean up all the htable information for this mapping
	 */
	ASSERT(ht->ht_valid_cnt > 0);
	HTABLE_DEC(ht->ht_valid_cnt);
	PGCNT_DEC(ht->ht_hat, ht->ht_level);

	/*
	 * sync ref/mod bits to the page_t
	 */
	hati_sync_key(pp, old_pte);

	/*
	 * Remove the mapping list entry for this page.
	 */
	hm = hment_remove(pp, ht, entry);

	/*
	 * drop the mapping list lock so that we might free the
	 * hment and htable.
	 */
	s390x_hm_exit(pp);
	htable_release(ht);
	return (hm);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_sync_key.                                    */
/*                                                                  */
/* Function	- Synchronize the storage key modified/referenced   */
/*		  bits with page_t representing this page.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hati_sync_key(page_t *pp, s390xpte_t pte)
{
	long	key,
		rm = 0;
	
	if (PTE_GET(pte, PT_SOFTWARE) >= PT_NOSYNC)
		return;

	GET_KEY(pte, key);

	if (key & SK_MOD)
		rm  = P_MOD;

	if (key & SK_REF)
		rm |= P_REF;

	if (rm == 0)
		return;

	if (pp != NULL) {
		hat_page_setattr(pp, rm);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_pageunload.                                  */
/*                                                                  */
/* Function	- Unload all translations to a page.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
hati_pageunload(struct page *pp, uint_t pg_szcd, uint_t forceflag)
{
	page_t		*cur_pp = pp;
	hment_t		*hm;
	hment_t		*prev;
	htable_t	*ht;
	uint_t		entry;
	level_t		level;

	/*
	 * clear the vpm ref.
	 */
	if (vpm_enable) {
		pp->p_vpmref = 0;
	}

	for (;;) {
		/*
		 * Get a mapping list entry
		 */
		s390x_hm_enter(cur_pp);
		for (prev = NULL; ; prev = hm) {
			hm = hment_walk(cur_pp, &ht, &entry, prev);
			if (hm == NULL) {
				s390x_hm_exit(cur_pp);
				return (0);
			}

			/*
			 * If this mapping size matches, remove it.
			 */
			if (ht->ht_level == pg_szcd)
				break;
		}

		/*
		 * Remove the mapping list entry for this page.
		 * Note this does the s390x_hm_exit() for us.
		 */
		hm = hati_page_unmap(cur_pp, ht, entry);
		if (hm != NULL)
			hment_free(hm);

	}
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_pageunload.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hat_pageunload(struct page *pp, uint_t forceflag)
{
	ASSERT(PAGE_EXCL(pp));
	return (hati_pageunload(pp, 0, forceflag));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_page_demote.                                  */
/*                                                                  */
/* Function	- Unload all large mappings to pp and reduce by 1   */
/*		  p_szc field of every large page level that 	    */
/*		  included pp.					    */
/*                                                                  */
/* 		  pp must be locked EXCL. Even though no other 	    */
/*		  constituent pages are locked it's legal to unload */
/*		  large mappings to pp because all constituent 	    */
/*		  pages of large locked mappings have to be locked  */
/*		  SHARED. Therefore if we have EXCL lock on one of  */
/*		  constituent pages none of the large mappings to   */
/*		  pp are locked.				    */
/*                                                                  */
/* 		  Change (always decrease) p_szc field starting     */
/*		  from the last constituent page and ending with    */
/*		  root constituent page so that root's pszc always  */
/*		  shows the area where hat_page_demote() may be     */
/*		  active.					    */
/*                                                                  */
/* 		  This mechanism is only used for file system pages */
/*		  where it's not always possible to get EXCL locks  */
/*		  on all constituent pages to demote the size code  */
/* 		  (as is done for anonymous or kernel large pages). */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_page_demote(page_t *pp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_pagesync.                                     */
/*                                                                  */
/* Function	- Get hw stats from hardware into page struct and   */
/*		  reset hw stats returns attributes of page.	    */
/*                                                                  */
/* 		  Flags for hat_pagesync, hat_getstat, hat_sync	    */
/*                                                                  */
/* 			HAT_SYNC_ZERORM		0x01		    */
/*                                                                  */
/* 		  Additional flags for hat_pagesync		    */
/*                                                                  */
/* 			HAT_SYNC_STOPON_REF	0x02		    */
/* 			HAT_SYNC_STOPON_MOD	0x04		    */
/* 			HAT_SYNC_STOPON_RM	0x06		    */
/* 			HAT_SYNC_STOPON_SHARED	0x08		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint_t
hat_pagesync(struct page *pp, uint_t flags)
{
	hment_t		*hm = NULL;
	htable_t	*ht;
	uint_t		entry;
	s390xpte_t	pte;
	uchar_t		nrmbits = P_REF | P_MOD | P_RO;
	page_t		*save_pp = pp;
	uint_t		pszc = 0;
	long		key;

	ASSERT(PAGE_LOCKED(pp) || panicstr);

	if (PP_ISRO(pp) && (flags & HAT_SYNC_STOPON_MOD))
		return (pp->p_nrm & nrmbits);

	if ((flags & HAT_SYNC_ZERORM) == 0) {

		if ((flags & HAT_SYNC_STOPON_REF) != 0 && PP_ISREF(pp))
			return (pp->p_nrm & nrmbits);

		if ((flags & HAT_SYNC_STOPON_MOD) != 0 && PP_ISMOD(pp))
			return (pp->p_nrm & nrmbits);

		if ((flags & HAT_SYNC_STOPON_SHARED) != 0 &&
		    hat_page_getshare(pp) > po_share) {
			if (PP_ISRO(pp))
				PP_SETREF(pp);
			return (pp->p_nrm & nrmbits);
		}
	}

	/*
	 * walk thru the mapping list syncing (and clearing) ref/mod bits.
	 */
	s390x_hm_enter(pp);
	while ((hm = hment_walk(pp, &ht, &entry, hm)) != NULL) {
		pte = s390xpte_get(ht, entry);

		GET_KEY((uintptr_t) pte, key);

		if (key & (SK_REF | SK_MOD) == 0)
			continue;

		if ((flags & HAT_SYNC_ZERORM) != 0) {
			CLR_KEY((uintptr_t) pte);
			hat_page_setattr(pp, 0);
		} else
			hati_sync_key(pp, pte);

		/*
		 * can stop short if we found a ref'd or mod'd page
		 */
		if ((flags & HAT_SYNC_STOPON_MOD) && PP_ISMOD(save_pp) ||
		    (flags & HAT_SYNC_STOPON_REF) && PP_ISREF(save_pp)) {
			break;
		}
	}
	s390x_hm_exit(pp);

	return (save_pp->p_nrm & nrmbits);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_page_getshare.                                */
/*                                                                  */
/* Function	- Returns the approximate number of mappings to     */
/*		  this pp. A return of 0 implies there are no 	    */
/*		  mappings to the page.        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

ulong_t
hat_page_getshare(page_t *pp)
{
	uint_t cnt;

	cnt = hment_mapcnt(pp);
	return (cnt);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_page_checkshare.                              */
/*                                                                  */
/* Function	- Return 1 if the number of mappings exceeeds       */
/*		  sh_thresh, else return 0.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hat_page_checkshare(page_t *pp, ulong_t sh_thresh)
{
	return (hat_page_getshare(pp) > sh_thresh);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_softlock.                                     */
/*                                                                  */
/* Function	- Not supported anymore.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

faultcode_t
hat_softlock(
	hat_t *hat,
	caddr_t addr,
	size_t *len,
	struct page **page_array,
	uint_t flags)
{
	return (FC_NOSUPPORT);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_supported.                                    */
/*                                                                  */
/* Function	- Routines to expose supported HAT features to      */
/*		  platform independent code.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hat_supported(enum hat_features feature, void *arg)
{
	switch (feature) {

	case HAT_SHARED_PT:	/* this is really ISM */
		return (1);

	case HAT_DYNAMIC_ISM_UNMAP:
		return (0);

	case HAT_VMODSORT:
		return (1);

	case HAT_SHARED_REGIONS:
		return (0);

	default:
		panic("hat_supported() - unknown feature");
	}
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_thread_exit.                                  */
/*                                                                  */
/* Function	- Called when a thread is exiting and has been      */
/*		  switched to the kernel address space.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_thread_exit(kthread_t *thd)
{
	hat_switch(thd->t_procp->p_as->a_hat);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_setup.                                        */
/*                                                                  */
/* Function	- Setup the given brand new hat structure as the    */
/*		  new HAT on this CPU's DAT unit.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_setup(hat_t *hat, int flags)
{
	page_t	*pl;
	proc_t	*p;
	int	ppIndex = 0;
	static struct seg tmpseg;

	cli();
	kpreempt_disable();

	/* Init needs some special treatment. */
	if (flags & HAT_INIT) {
		hat_enter(hat);
		
		/*
		 * If this is not the kernel address space then we 
		 * allocate a page at the stack top that will kick start
		 * the generation of region, segment and page tables
		 */
		if (hat->hat_as != &kas) {
			uintptr_t stkBase;

			p       = hat->hat_as->a_proc;
			stkBase = ((uintptr_t) p->p_usrstack) - p->p_stksize;
			pl      = page_create_va(&kvp, stkBase, p->p_stksize,
						 PG_EXCL, &tmpseg, 
						 (caddr_t) hat->hat_as);

			if (pl == NULL)
				panic("Cannot create address space\n");

			AS_LOCK_ENTER(hat->hat_as, &hat->hat_as->a_lock, RW_WRITER);
			hati_memload_plist(hat, (caddr_t) stkBase, p->p_stksize,
					   pl, PROT_READ|PROT_WRITE, HAT_LOAD);
			AS_LOCK_EXIT(hat->hat_as, &hat->hat_as->a_lock);
		}
		hat_exit(hat);
	} else {
		p = hat->hat_as->a_proc;
	}

	hat_switch(hat);

	kpreempt_enable();

	sti();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_mempte_kern_setup                             */
/*                                                                  */
/* Function	- Prepare for a CPU private mapping for the given   */
/*		  address. The address can only be used from a      */
/*		  single CPU and can be remapped using hat_mempte_  */
/*		  remap(). Return the address of the PTE.	    */
/*		                               		 	    */
/*		  We do the htable_create() if necessary and inc-   */
/*		  rement the valid count so the htable can't dis-   */
/*		  appear. We also hat_devload() the page table      */
/*		  into kernel so that the PTE is quickly accessed.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
hat_mempte_kern_setup(caddr_t addr, void *pt)
{
	uintptr_t	va = (uintptr_t)addr;
	htable_t	*ht;
	uint_t		entry;
	s390xpte_t	oldpte;
	caddr_t		p = (caddr_t)pt;

	ASSERT(IS_PAGEALIGNED(va));
	ht = htable_getpte(kas.a_hat, va, &entry, &oldpte, 0);
	if (ht == NULL) {
		/*
		 * Note that we don't need a hat_reserves_exit() check
		 * for this htable_create(), since that'll be done by the
		 * hat_devload() just below.
		 */
		ht = htable_create(kas.a_hat, va, 0, NULL);
		entry = htable_va2entry(va, ht);
		ASSERT(ht->ht_level == 0);
		oldpte = s390xpte_get(ht, entry);
	}
	if (PTE_ISVALID(oldpte, 0))
		panic("hat_mempte_setup(): address already mapped"
		    "ht=%p, entry=%d, pte=" FMT_PTE, ht, entry, oldpte);

	/*
	 * increment ht_valid_cnt so that the pagetable can't disappear
	 */
	HTABLE_INC(ht->ht_valid_cnt);

	/*
	 * now we need to map the page holding the pagetable for va into
	 * the kernel's address space.
	 */
	hat_devload(kas.a_hat, p, MMU_PAGESIZE, ht->ht_pfn,
	    PROT_READ | PROT_WRITE | HAT_NOSYNC | HAT_UNORDERED_OK,
	    HAT_LOAD | HAT_LOAD_NOCONSIST);

	/*
	 * return the PTE address to the caller.
	 */
	htable_release(ht);
	p += entry << mmu.pte_size_shift;
	return ((void *)p);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_mempte_setup                                  */
/*                                                                  */
/* Function	- Prepare for CPU private mapping for the given     */
/*		  adderss.                       		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
hat_mempte_setup(caddr_t addr)
{
	s390xpte_t	*p;

	p = vmem_alloc(heap_arena, MMU_PAGESIZE, VM_SLEEP);
	return (hat_mempte_kern_setup(addr, p));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_mempte_release                                */
/*                                                                  */
/* Function	- Release a CPU private mapping for the given add-  */
/*		  ress. We decrement the htable valid count so it   */
/*		  might be destroyed.          		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_mempte_release(caddr_t addr, void *pteptr)
{
	htable_t	*ht;
	uintptr_t	va = ALIGN2PAGE(pteptr);
	uint_t		entry;

	/*
	 * first invalidate any left over mapping and decrement the
	 * htable's mapping count
	 */
	*(s390xpte_t *)pteptr = 0;

	ht = htable_getpte(kas.a_hat, ALIGN2PAGE(addr), &entry, NULL, 0);
	if (ht == NULL)
		panic("hat_mempte_release(): invalid address");

	ASSERT(ht->ht_level == 0);
	ipte(ht->ht_org, entry);
	HTABLE_DEC(ht->ht_valid_cnt);
	htable_release(ht);

	/*
	 * now blow away the kernel mapping to the page table page
	 * XX64 -- see comment in hat_mempte_setup()
	 */
	hat_unload_callback(kas.a_hat, (caddr_t)va, MMU_PAGESIZE,
	    HAT_UNLOAD, NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_setup.                                        */
/*                                                                  */
/* Function	- Setup the given brand new hat structure as the    */
/*		  new HAT on this CPU's DAT unit.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * Apply a temporary CPU private mapping to a page. We flush the TLB only
 * on this CPU, so this ought to have been called with preemption disabled.
 */
void
hat_mempte_remap(
	pfn_t pfn,
	caddr_t addr,
	void *pteptr,
	uint_t attr,
	uint_t flags)
{
	uintptr_t	va = (uintptr_t)addr;
	s390xpte_t	pte;
	htable_t	*ht;
	uint_t		entry;

	/*
	 * Remap the given PTE to the new page's PFN. Invalidate only
	 * on this CPU.
	 */

	ht = htable_getpte(kas.a_hat, va, &entry, NULL, 0);

#ifdef DEBUG
	ASSERT(IS_PAGEALIGNED(va));
	ASSERT(ht != NULL);
	ASSERT(ht->ht_level == 0);
	ASSERT(ht->ht_valid_cnt > 0);
#endif

	htable_release(ht);
	pte = hati_mkpte(pfn, attr, 0, flags);
	ipte(ht->ht_org, entry);
	*(s390xpte_t *)pteptr = pte;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_enter.                                        */
/*                                                                  */
/* Function	- Lock enter function. This function is being used  */
/*		  used by hatstats: it can be removed by using a    */
/*		  per-AS mutex for hatstats.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_enter(hat_t *hat)
{
	mutex_enter(&hat->hat_mutex);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_exit.                                         */
/*                                                                  */
/* Function	- Lock exit function. This function is being used   */
/*		  used by hatstats: it can be removed by using a    */
/*		  per-AS mutex for hatstats.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_exit(hat_t *hat)
{
	mutex_exit(&hat->hat_mutex);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hati_update_pte                                   */
/*                                                                  */
/* Function	- Atomically update a new translation for a single  */
/*		  page. If the currently installed PTE doesn't      */
/*		  match the value we expect to find, it's not       */
/*		  updated and we return the PTE we found.           */
/*		                               		 	    */
/*		  If activiating nosync or NOWRITE and the page was */
/*		  modified we need to sync with the page_t. Also    */
/*		  sync with page_t if clearing ref/mod bits. 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static s390xpte_t
hati_update_pte(htable_t *ht, uint_t entry, s390xpte_t expected, s390xpte_t new)
{
	page_t		*pp;
	s390xpte_t	replaced;

	replaced = s390xpte_update(ht, entry, expected, new);
	if (replaced != new)
		return (replaced);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_join_srd.                                     */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
hat_join_srd(struct hat *sfmmup, vnode_t *evp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_join_region.                                  */
/*                                                                  */
/* Function	- Join a region - not supported on s390x.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
hat_region_cookie_t
hat_join_region(struct hat *sfmmup,
    caddr_t r_saddr,
    size_t r_size,
    void *r_obj,
    u_offset_t r_objoff,
    uchar_t r_perm,
    uchar_t r_pgszc,
    hat_rgn_cb_func_t r_cb_function,
    uint_t flags)
{
	panic("hat_join_region called - No shared region support on s390x");
	return (HAT_INVALID_REGION_COOKIE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_leave_region.                                 */
/*                                                                  */
/* Function	- Leave a region - not supported on s390x.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
hat_leave_region(struct hat *sfmmup, hat_region_cookie_t rcookie, uint_t flags)
{
	panic("hat_leave_region called - No shared region support on s390x");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_dup_region.                                   */
/*                                                                  */
/* Function	- Duplicate a region - not supported on s390x.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
hat_dup_region(struct hat *sfmmup, hat_region_cookie_t rcookie)
{
	panic("hat_dup_region called - No shared region support on s390x");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_unlock_region.                                */
/*                                                                  */
/* Function	- Unlock a region - not supported on s390x.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
hat_unlock_region(struct hat *sfmmup, caddr_t addr, size_t len,
    hat_region_cookie_t rcookie)
{
	panic("hat_unlock_region called - No shared region support on s390x");
}

/*========================= End of Function ========================*/

/*==================================================================*/
/*		                               		 	    */
/* Kernel Physical Mapping (kpm) facility			    */
/*		                               		 	    */
/* Most of the routines needed to support segkpm are almost no-ops  */
/* on the s390x platform.  We map in the entire segment when it is  */
/* created and leave it mapped in, so there is no additional work   */
/* required to set up and tear down individual mappings.  All of    */
/* these routines were created to support SPARC platforms that have */
/* to avoid aliasing in their virtually indexed caches.		    */
/*		                               		 	    */
/* Most of the routines have sanity checks in them (e.g. verifying  */
/* that the passed-in page is locked).  We don't actually care 	    */
/* about most of these checks on s390x, but we leave them in place  */
/* to identify problems in the upper levels.			    */
/*		                               		 	    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_mapin.                                    */
/*                                                                  */
/* Function	- Map in a locked page and return the vaddr.        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
hat_kpm_mapin(struct page *pp, struct kpme *kpme)
{
	caddr_t		vaddr;

#ifdef DEBUG
	if (kpm_enable == 0) {
		cmn_err(CE_WARN, "hat_kpm_mapin: kpm_enable not set\n");
		return ((caddr_t)NULL);
	}

	if (pp == NULL || PAGE_LOCKED(pp) == 0) {
		cmn_err(CE_WARN, "hat_kpm_mapin: pp zero or not locked\n");
		return ((caddr_t)NULL);
	}
#endif

	vaddr = hat_kpm_page2va(pp, 1);

	return (vaddr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_mapout.                                   */
/*                                                                  */
/* Function	- Map out a locked page.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kpm_mapout(struct page *pp, struct kpme *kpme, caddr_t vaddr)
{
#ifdef DEBUG
	if (kpm_enable == 0) {
		cmn_err(CE_WARN, "hat_kpm_mapout: kpm_enable not set\n");
		return;
	}

	if (IS_KPM_ADDR(vaddr) == 0) {
		cmn_err(CE_WARN, "hat_kpm_mapout: no kpm address\n");
		return;
	}

	if (pp == NULL || PAGE_LOCKED(pp) == 0) {
		cmn_err(CE_WARN, "hat_kpm_mapout: page zero or not locked\n");
		return;
	}
#endif

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_va2pfn.                                   */
/*                                                                  */
/* Function	- Return the page frame number for the kpm virtual  */
/*		  address 'vaddr'.             		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pfn_t
hat_kpm_va2pfn(caddr_t vaddr)
{
	pfn_t		pfn;

	ASSERT(IS_KPM_ADDR(vaddr));

	pfn = (pfn_t)btop(vaddr - kpm_vbase);

	return (pfn);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_pfn2va.                                   */
/*                                                                  */
/* Function	- Return the kpm virtual address for a specific pfn.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
hat_kpm_pfn2va(pfn_t pfn)
{
	uintptr_t vaddr = (uintptr_t)kpm_vbase + mmu_ptob(pfn);

	return ((caddr_t)vaddr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_vaddr2page.                               */
/*                                                                  */
/* Function	- Return the page for the kpm virtual address vaddr.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

page_t *
hat_kpm_vaddr2page(caddr_t vaddr)
{
	pfn_t		pfn;

	ASSERT(IS_KPM_ADDR(vaddr));

	pfn = hat_kpm_va2pfn(vaddr);

	return (page_numtopp_nolock(pfn));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_page2va.                                  */
/*                                                                  */
/* Function	- Return the kpm virtual address for the page at pp.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
hat_kpm_page2va(struct page *pp, int checkswap)
{
	return (hat_kpm_pfn2va(pp->p_pagenum));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_fault.                                    */
/*                                                                  */
/* Function	- hat_kpm_fault is called from segkpm_fault when    */
/* 		  we take a page fault on a KPM page.  This should  */
/*		  never happen on s390x.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hat_kpm_fault(hat_t *hat, caddr_t vaddr)
{
	panic("pagefault in seg_kpm.  hat: 0x%p  vaddr: 0x%p", hat, vaddr);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_mseghash_clear.                           */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kpm_mseghash_clear(int nentries)
{}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_memseghash_update.                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kpm_mseghash_update(pgcnt_t inx, struct memseg *msp)
{}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_addmem_mseg_update.                       */
/*                                                                  */
/* Function	- Update kpm memseg members from basic memseg info. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kpm_addmem_mseg_update(struct memseg *msp, pgcnt_t nkpmpgs,
	offset_t kpm_pages_off)
{
	if (kpm_enable == 0)
		return;

	msp->kpm_pages   = (kpm_page_t *)((caddr_t)msp->pages + kpm_pages_off);
	msp->kpm_nkpmpgs = nkpmpgs;
	msp->kpm_pbase   = kpmptop(ptokpmp(msp->pages_base));
	msp->pagespa 	 = va_to_pa(msp->pages);
	msp->epagespa 	 = va_to_pa(msp->epages);
	msp->kpm_pagespa = va_to_pa(msp->kpm_pages);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_addmem_mseg_insert.                       */
/*                                                                  */
/* Function	- Setup nextpa whan a memseg is inserted.           */
/*                                                                  */
/*         	  Assumes that the memsegslock is already held.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kpm_addmem_mseg_insert(struct memseg *msp)
{
	if (kpm_enable == 0)
		return;

	ASSERT(memsegs_lock_held());
	msp->nextpa = (memsegs) ? va_to_pa(memsegs) : MSEG_NULLPTR_PA;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_mseg_reuse.                               */
/*                                                                  */
/* Function	- Return end of metadata for an already setup 	    */
/*		  memseg.					    */
/*                                                                  */
/* 		  Note: kpm_pages and kpm_spages are aliases and    */
/*		  the underlying member of struct memseg is a union,*/
/*		  therefore they always have the same address 	    */
/*		  within a memseg. They must be differentiated when */
/* 		  pointer arithmetic is used with them.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
hat_kpm_mseg_reuse(struct memseg *msp)
{
	caddr_t end;

	if (kpm_smallpages == 0)
		end = (caddr_t)(msp->kpm_pages + msp->kpm_nkpmpgs);
	else
		end = (caddr_t)(msp->kpm_spages + msp->kpm_nkpmpgs);

	return (end);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_addmem_memsegs_update.                    */
/*                                                                  */
/* Function	- Setup memsegspa when a memseg is (head) inserted. */
/* 		  Called before memsegs is updated to complete a    */
/* 		  memseg insert operation.			    */
/*		                               		 	    */
/* 		  Assumes that the memsegslock is already held.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kpm_addmem_memsegs_update(struct memseg *msp)
{
	if (kpm_enable == 0)
		return;

	ASSERT(memsegs_lock_held());
	ASSERT(memsegs);
	memsegspa = va_to_pa(msp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_split_mseg_update.                        */
/*                                                                  */
/* Function	- Update kpm members for all memseg's involved in a */
/*		  split operation and do the atomic update of the   */
/*		  physical memseg chain.			    */
/*                                                                  */
/* 		  Note: kpm_pages and kpm_spages are aliases and    */
/*		  the underlying member of struct memseg is a union,*/
/*		  therefore they always have the same address 	    */
/*		  within a memseg. With that the direct assignments */
/*		  and va_to_pa conversions below don't have to be   */
/*		  distinguished wrt * kpm_smallpages. They must be  */
/*		  differentiated when pointer arithmetic is used    */
/*		  with them.					    */
/*                                                                  */
/* 		  Assumes that the memsegslock is already held.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kpm_split_mseg_update(struct memseg *msp, struct memseg **mspp,
	struct memseg *lo, struct memseg *mid, struct memseg *hi)
{
	pgcnt_t start, end, kbase, kstart, num;
	struct memseg *lmsp;

	if (kpm_enable == 0)
		return;

	ASSERT(memsegs_lock_held());
	ASSERT(msp && mid && msp->kpm_pages);

	kbase = ptokpmp(msp->kpm_pbase);

	if (lo) {
		num   = lo->pages_end - lo->pages_base;
		start = kpmptop(ptokpmp(lo->pages_base));

		/* align end to kpm page size granularity */

		end 		= kpmptop(ptokpmp(start + num - 1)) + kpmpnpgs;
		lo->kpm_pbase 	= start;
		lo->kpm_nkpmpgs = ptokpmp(end - start);
		lo->kpm_pages 	= msp->kpm_pages;
		lo->kpm_pagespa = va_to_pa(lo->kpm_pages);
		lo->pagespa 	= va_to_pa(lo->pages);
		lo->epagespa 	= va_to_pa(lo->epages);
		lo->nextpa 	= va_to_pa(lo->next);
	}

	/* mid */
	num 	= mid->pages_end - mid->pages_base;
	kstart	= ptokpmp(mid->pages_base);
	start 	= kpmptop(kstart);

	/* align end to kpm page size granularity */
	end 		 = kpmptop(ptokpmp(start + num - 1)) + kpmpnpgs;
	mid->kpm_pbase 	 = start;
	mid->kpm_nkpmpgs = ptokpmp(end - start);
	if (kpm_smallpages == 0) {
		mid->kpm_pages = msp->kpm_pages + (kstart - kbase);
	} else {
		mid->kpm_spages = msp->kpm_spages + (kstart - kbase);
	}
	mid->kpm_pagespa = va_to_pa(mid->kpm_pages);
	mid->pagespa     = va_to_pa(mid->pages);
	mid->epagespa    = va_to_pa(mid->epages);
	mid->nextpa      = (mid->next) ?  va_to_pa(mid->next) : MSEG_NULLPTR_PA;

	if (hi) {
		num    = hi->pages_end - hi->pages_base;
		kstart = ptokpmp(hi->pages_base);
		start  = kpmptop(kstart);

		/* align end to kpm page size granularity */
		end		= kpmptop(ptokpmp(start + num - 1)) + kpmpnpgs;
		hi->kpm_pbase   = start;
		hi->kpm_nkpmpgs = ptokpmp(end - start);
		if (kpm_smallpages == 0) {
			hi->kpm_pages = msp->kpm_pages + (kstart - kbase);
		} else {
			hi->kpm_spages = msp->kpm_spages + (kstart - kbase);
		}
		hi->kpm_pagespa = va_to_pa(hi->kpm_pages);
		hi->pagespa     = va_to_pa(hi->pages);
		hi->epagespa    = va_to_pa(hi->epages);
		hi->nextpa      = (hi->next) ? va_to_pa(hi->next) : MSEG_NULLPTR_PA;
	}

	/*
	 * Atomic update of the physical memseg chain
	 */
	if (mspp == &memsegs) {
		memsegspa = (lo) ? va_to_pa(lo) : va_to_pa(mid);
	} else {
		lmsp = (struct memseg *)
			((uint64_t)mspp - offsetof(struct memseg, next));
		lmsp->nextpa = (lo) ? va_to_pa(lo) : va_to_pa(mid);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kpm_delmem_mseg_update.                       */
/*                                                                  */
/* Function	- Update memsegspa (when first memseg in list is    */
/*		  deleted) or nextpa when a memseg deleted.         */
/*                                                                  */
/* 		  Assumes that the memsegslock is already held.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kpm_delmem_mseg_update(struct memseg *msp, struct memseg **mspp)
{
	struct memseg *lmsp;

	if (kpm_enable == 0)
		return;

	ASSERT(memsegs_lock_held());

	if (mspp == &memsegs) {
		memsegspa = (msp->next) ?
				va_to_pa(msp->next) : MSEG_NULLPTR_PA;
	} else {
		lmsp = (struct memseg *)
			((uint64_t)mspp - offsetof(struct memseg, next));
		lmsp->nextpa = (msp->next) ?
				va_to_pa(msp->next) : MSEG_NULLPTR_PA;
	}
}

/*========================= End of Function ========================*/
