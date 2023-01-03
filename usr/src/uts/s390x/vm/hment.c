/*------------------------------------------------------------------*/
/* 								    */
/* Name        - hment.c    					    */
/* 								    */
/* Function    - Manipulate and manage hment entries.               */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - October, 2006.					    */
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

#define	HMENT_EMBEDDED ((hment_t *)(uintptr_t)1)
#define	HMENT_RESERVE_AMOUNT	(512)	/* currently a guess at right value. */
#define	HMENT_HASH_SIZE (64 * 1024)

/*
 * Lots of highly shared pages will have the same value for "entry" (consider
 * the starting address of "xterm" or "sh"). So we'll distinguish them by
 * adding the pfn of the page table into both the high bits.
 * The shift by 9 corresponds to the range of values for entry (0..511).
 */
#define	HMENT_HASH(pfn, entry) (uint32_t) 	\
	((((pfn) << 9) + entry + pfn) & (hment_hash_entries - 1))

#define	MLIST_NUM_LOCK	256		/* must be power of two */

/*
 * the shift by 9 is so that all large pages don't use the same hash bucket
 */
#define	MLIST_MUTEX(pp) \
	&mlist_lock[((pp)->p_pagenum + ((pp)->p_pagenum >> 9)) & \
	(MLIST_NUM_LOCK - 1)]

#define	HASH_NUM_LOCK	256		/* must be power of two */

#define	HASH_MUTEX(idx) &hash_lock[(idx) & (HASH_NUM_LOCK-1)]

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/bitmap.h>
#include <sys/systm.h>
#include <vm/seg_kmem.h>
#include <vm/hat.h>
#include <vm/vm_dep.h>
#include <vm/hat_s390x.h>
#include <vm/hment.h>
#include <sys/cmn_err.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*
 * When pages are shared by more than one mapping, a list of these
 * structs hangs off of the page_t connected by the hm_next and hm_prev
 * fields.  Every hment is also indexed by a system-wide hash table, using
 * hm_hashnext to connect it to the chain of hments in a single hash
 * bucket.
 */
struct hment {
	struct hment	*hm_hashnext;	/* next mapping on hash chain */
	struct hment	*hm_next;	/* next mapping of same page */
	struct hment	*hm_prev;	/* previous mapping of same page */
	htable_t	*hm_htable;	/* corresponding htable_t */
	pfn_t		hm_pfn;		/* mapping page frame number */
	uint16_t	hm_entry;	/* index of pte in htable */
	uint16_t	hm_pad;		/* explicitly expose compiler padding */
	uint32_t	hm_pad2;	/* explicitly expose compiler padding */
};

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern  kthread_t *hat_reserves_thread;
extern	hment_t	  *hati_page_unmap(page_t *, htable_t *, uint_t);

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

kmem_cache_t *hment_cache;

/*
 * The hment reserve is similar to the htable reserve, with the following
 * exception. Hment's are never needed for HAT kmem allocs.
 *
 * The hment_reserve_amount variable is used, so that you can change it's
 * value to zero via a kernel debugger to force stealing to get tested.
 */
uint_t hment_reserve_amount = HMENT_RESERVE_AMOUNT;
kmutex_t hment_reserve_mutex;
uint_t	hment_reserve_count = 0;
hment_t	*hment_reserve_pool;

/*
 * Possible performance RFE: we might need to make this dynamic, perhaps
 * based on the number of pages in the system.
 */
static uint_t hment_hash_entries = HMENT_HASH_SIZE;
static hment_t **hment_hash;

/*
 * "mlist_lock" is a hashed mutex lock for protecting per-page mapping
 * lists and "hash_lock" is a similar lock protecting the hment hash
 * table.  The hashed approach is taken to avoid the spatial overhead of
 * maintaining a separate lock for each page, while still achieving better
 * scalability than a single lock would allow.
 */
static kmutex_t mlist_lock[MLIST_NUM_LOCK];

static kmutex_t hash_lock[HASH_NUM_LOCK];

static hment_t *hment_steal(void);

static page_t *last_page = NULL;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_put_reserve                                 */
/*                                                                  */
/* Function	- Put one hment onto the reserves list.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hment_put_reserve(hment_t *hm)
{
	HATSTAT_INC(hs_hm_put_reserve);
	mutex_enter(&hment_reserve_mutex);
	hm->hm_next = hment_reserve_pool;
	hment_reserve_pool = hm;
	++hment_reserve_count;
	mutex_exit(&hment_reserve_mutex);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_get_reserve                                 */
/*                                                                  */
/* Function	- Get one hment onto the reserves list.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static hment_t *
hment_get_reserve(void)
{
	hment_t *hm = NULL;

	/*
	 * We rely on a "donation system" to refill the hment reserve
	 * list, which only takes place when we are allocating hments for
	 * user mappings.  It is theoretically possible that an incredibly
	 * long string of kernel hment_alloc()s with no intervening user
	 * hment_alloc()s could exhaust that pool.
	 */
	HATSTAT_INC(hs_hm_get_reserve);
	mutex_enter(&hment_reserve_mutex);
	if (hment_reserve_count != 0) {
		hm = hment_reserve_pool;
		hment_reserve_pool = hm->hm_next;
		--hment_reserve_count;
	}
	mutex_exit(&hment_reserve_mutex);
	return (hm);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_alloc                                       */
/*                                                                  */
/* Function	- Allocate an hment.                                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static hment_t *
hment_alloc()
{
	int km_flag = KM_NOSLEEP;
	hment_t	*hm = NULL;

	/*
	 * If we aren't using the reserves, try using kmem to get an hment.
	 * Donate any successful allocations to reserves if low.
	 *
	 * If we're in panic, resort to using the reserves.
	 */
	HATSTAT_INC(hs_hm_alloc);
	if (!USE_HAT_RESERVES()) {
		for (;;) {
			hm = kmem_cache_alloc(hment_cache, km_flag);
			if (hment_reserve_count >= hment_reserve_amount ||
			    hm == NULL || panicstr != NULL ||
			    curthread == hat_reserves_thread)
				break;
			hment_put_reserve(hm);
		}
	}

	/*
	 * If allocation failed, we need to tap the reserves or steal
	 */
	if (hm == NULL) {
		if (USE_HAT_RESERVES())
			hm = hment_get_reserve();

		/*
		 * If we still haven't gotten an hment, attempt to steal one by
		 * victimizing a mapping in a user htable.
		 */
		if (hm == NULL)
			hm = hment_steal();

		/*
		 * we're in dire straights, try the reserve
		 */
		if (hm == NULL)
			hm = hment_get_reserve();

		/*
		 * still no hment is a serious problem.
		 */
		if (hm == NULL) 
			panic("hment_alloc(): no reserve, couldn't steal");
	}


	hm->hm_entry	= 0;
	hm->hm_htable	= NULL;
	hm->hm_hashnext = NULL;
	hm->hm_next	= NULL;
	hm->hm_prev	= NULL;
	hm->hm_pfn	= PFN_INVALID;
	return (hm);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_free                                        */
/*                                                                  */
/* Function	- Free an hment, possibly to the reserves list when */
/*		  called from the thread using the reserve. For     */
/*		  example, when freeing an hment during an htable_  */
/*		  steal(), we can't recurse into the kmen allocator */
/*		  so we just push the hment onto the reserve list.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hment_free(hment_t *hm)
{
#ifdef DEBUG
	/*
	 * zero out all fields to try and force any race conditions to segfault
	 */
	bzero(hm, sizeof (*hm));
#endif
	HATSTAT_INC(hs_hm_free);
	if (curthread == hat_reserves_thread ||
	    hment_reserve_count < hment_reserve_amount)
		hment_put_reserve(hm);
	else
		kmem_cache_free(hment_cache, hm);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390x_hm_held.                                    */
/*                                                                  */
/* Function	- Check if a mutex is held.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
s390x_hm_held(page_t *pp)
{
	ASSERT(pp != NULL);
	return (MUTEX_HELD(MLIST_MUTEX(pp)));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390x_hm_enter                                    */
/*                                                                  */
/* Function	- Acquire a mutex.                                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
s390x_hm_enter(page_t *pp)
{
	ASSERT(pp != NULL);
	mutex_enter(MLIST_MUTEX(pp));
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- s390x_hm_exit                                     */
/*                                                                  */
/* Function	- Release a mutex.                                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
s390x_hm_exit(page_t *pp)
{
	ASSERT(pp != NULL);
	mutex_exit(MLIST_MUTEX(pp));
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_insert.                                     */
/*                                                                  */
/* Function	- Internal routine to add a full hment to a page_t  */
/*		  mapping list.                		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
hment_insert(hment_t *hm, page_t *pp)
{
	uint_t		idx;

	ASSERT(s390x_hm_held(pp));
	ASSERT(!pp->p_embed);

	/*
	 * Add the hment to the page's mapping list.
	 */
	++pp->p_share;
	hm->hm_next = pp->p_mapping;
	if (pp->p_mapping != NULL)
		((hment_t *)pp->p_mapping)->hm_prev = hm;
	pp->p_mapping = hm;

	/*
	 * Add the hment to the system-wide hash table.
	 */
	idx = HMENT_HASH(hm->hm_htable->ht_pfn, hm->hm_entry);

	mutex_enter(HASH_MUTEX(idx));
	hm->hm_hashnext = hment_hash[idx];
	hment_hash[idx] = hm;
	mutex_exit(HASH_MUTEX(idx));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_prepare                                     */
/*                                                                  */
/* Function	- Prepare a mapping list entry to the given page.   */
/*		                               		 	    */
/*		  There are 4 different situations to deal with:    */
/*		                               		 	    */
/*		  1. Adding the 1st mapping to a page_t as an       */
/*		     imbedded hment            		 	    */
/*		  2. Refault on an existing imbedded mapping	    */
/*		  3. Upgrading an imbedded mapping when adding a    */
/*		     2nd mapping               		 	    */
/*		  4. Adding another mapping to a page_t that        */
/*		     already has multiple mappings. Note we don't   */
/*		     optimize for the refaulting case here.         */
/*		                               		 	    */
/*		  Due to competition with other threads that may be */
/*		  mapping/unmapping the same page and the need to   */
/*		  drop all locks while allocating hments, any or    */
/*		  all of the 3 situations can occur (and in almost  */
/*		  any order) in any given call. Isn't this fun!     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hment_t *
hment_prepare(htable_t *htable, uint_t entry, page_t *pp)
{
	hment_t		*hm = NULL;

	ASSERT(s390x_hm_held(pp));

	for (;;) {

		/*--------------------------------------------------*/
		/* The most common case is establishing the first   */
		/* mapping to a page, so check that first. This	    */
		/* doesn't need any allocated hment.		    */
		/*--------------------------------------------------*/
		if (pp->p_mapping == NULL) {
			ASSERT(!pp->p_embed);
			ASSERT(pp->p_share == 0);
			if (hm == NULL)
				break;

			/*------------------------------------------*/
			/* we had an hment already: free it & retry */
			/*------------------------------------------*/
			goto free_and_continue;
		}

		/*--------------------------------------------------*/
		/* If there is an embedded mapping, we may need to  */
		/* convert it to an hment.			    */
		/*--------------------------------------------------*/
		if (pp->p_embed) {

			/* should point to htable */
			ASSERT(pp->p_mapping != NULL);

			/*
			 * If we are faulting on a pre-existing mapping
			 * there is no need to promote/allocate a new hment.
			 * This happens a lot due to segmap.
			 */
			if (pp->p_mapping == htable && pp->p_mlentry == entry) {
				if (hm == NULL)
					break;
				goto free_and_continue;
			}

			/*
			 * If we have an hment allocated, use it to promote the
			 * existing embedded mapping.
			 */
			if (hm != NULL) {
				hm->hm_htable	= pp->p_mapping;
				hm->hm_entry	= pp->p_mlentry;
				hm->hm_pfn	= pp->p_pagenum;
				pp->p_mapping	= NULL;
				pp->p_share	= 0;
				pp->p_embed	= 0;
				hment_insert(hm, pp);
			}

			/*
			 * We either didn't have an hment allocated or we just
			 * used it for the embedded mapping. In either case,
			 * allocate another hment and restart.
			 */
			goto allocate_and_continue;
		}

		/*
		 * Last possibility is that we're adding an hment to a list
		 * of hments.
		 */
		if (hm != NULL)
			break;
allocate_and_continue:
		s390x_hm_exit(pp);
		hm = hment_alloc();
		s390x_hm_enter(pp);
		continue;

free_and_continue:
		/*
		 * we allocated an hment already, free it and retry
		 */
		s390x_hm_exit(pp);
		hment_free(hm);
		hm = NULL;
		s390x_hm_enter(pp);
	}
	ASSERT(s390x_hm_held(pp));
	return (hm);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_assign                                      */
/*                                                                  */
/* Function	- Record a mapping list entry for the htable/entry  */
/*		  to the given page.           		 	    */
/*		                               		 	    */
/*		  hment_prepare() should have properly set up the   */
/*		  situation.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hment_assign(htable_t *htable, uint_t entry, page_t *pp, hment_t *hm)
{
	ASSERT(s390x_hm_held(pp));

	/*
	 * The most common case is establishing the first mapping to a
	 * page, so check that first. This doesn't need any allocated
	 * hment.
	 */
	if (pp->p_mapping == NULL) {
		ASSERT(hm == NULL);
		ASSERT(!pp->p_embed);
		ASSERT(pp->p_share == 0);
		pp->p_embed = 1;
		pp->p_mapping = htable;
		pp->p_mlentry = entry;
		return;
	}

	/*
	 * We should never get here with a pre-existing embedded maping
	 */
	ASSERT(!pp->p_embed);

	/*
	 * add the new hment to the mapping list
	 */
	ASSERT(hm != NULL);
	hm->hm_htable 	= htable;
	hm->hm_entry 	= entry;
	hm->hm_pfn	= pp->p_pagenum;
	hment_insert(hm, pp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_walk                                        */
/*                                                                  */
/* Function	- Walk through the mappings for a page. Note:       */
/*		  s390x_hm_enter() must have already been called.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hment_t *
hment_walk(page_t *pp, htable_t **ht, uint_t *entry, hment_t *prev)
{
	hment_t		*hm;

	ASSERT(s390x_hm_held(pp));

	if (pp->p_embed) {
		if (prev == NULL) {
			*ht = (htable_t *)pp->p_mapping;
			*entry = pp->p_mlentry;
			hm = HMENT_EMBEDDED;
		} else {
			ASSERT(prev == HMENT_EMBEDDED);
			hm = NULL;
		}
	} else {
		if (prev == NULL) {
			ASSERT(prev != HMENT_EMBEDDED);
			hm = (hment_t *)pp->p_mapping;
		} else {
			hm = prev->hm_next;
		}

		if (hm != NULL) {
			*ht = hm->hm_htable;
			*entry = hm->hm_entry;
		}
	}
	return (hm);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_remove                                      */
/*                                                                  */
/* Function	- Remove a mapping to a page from its mapping list. */
/*		  The corresponding mapping list must be locked.    */
/*		  Finds the mapping list entry with the give pte_t  */
/*		  and unlinks it from the mapping list.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hment_t *
hment_remove(page_t *pp, htable_t *ht, uint_t entry)
{
	hment_t		*prev = NULL;
	hment_t		*hm;
	uint_t		idx;
	pfn_t		pfn;

	ASSERT(s390x_hm_held(pp));

	/*
	 * Check if we have only one mapping embedded in the page_t.
	 */
	if (pp->p_embed) {
		ASSERT(ht == (htable_t *)pp->p_mapping);
		ASSERT(entry == pp->p_mlentry);
		ASSERT(pp->p_share == 0);
		pp->p_mapping = NULL;
		pp->p_mlentry = 0;
		pp->p_embed = 0;
		return (NULL);
	}

	/* 
	 * Check to see if this page was created at boot time and has no hment
	 */
	if ((pp->p_share == 0) &&
	    (ht->ht_flags & HTABLE_PRIMAL) == HTABLE_PRIMAL) {
		pp->p_mapping = NULL;
		pp->p_mlentry = 0;
		pp->p_embed = 0;
		return (NULL);
	}
		
	/*
	 * Otherwise it must be in the list of hments.
	 * Find the hment in the system-wide hash table and remove it.
	 */

	ASSERT(pp->p_share != 0);
	pfn = pp->p_pagenum;
	idx = HMENT_HASH(ht->ht_pfn, entry);
	mutex_enter(HASH_MUTEX(idx));
	hm = hment_hash[idx];
	while (hm && (hm->hm_htable != ht || hm->hm_entry != entry) || 
		hm->hm_pfn != pfn) {
		prev = hm;
		hm = hm->hm_hashnext;
	}
	if (hm == NULL) {
		panic("hment_remove() missing in hash table pp=%lx, ht=%lx,"
		    "entry=0x%x hash index=0x%x", (uintptr_t)pp, (uintptr_t)ht,
		    entry, idx);
	}

	if (prev)
		prev->hm_hashnext = hm->hm_hashnext;
	else
		hment_hash[idx] = hm->hm_hashnext;
	mutex_exit(HASH_MUTEX(idx));

	/*
	 * Remove the hment from the page's mapping list
	 */
	if (hm->hm_next)
		hm->hm_next->hm_prev = hm->hm_prev;
	if (hm->hm_prev)
		hm->hm_prev->hm_next = hm->hm_next;
	else
		pp->p_mapping = hm->hm_next;

	--pp->p_share;
	hm->hm_hashnext = NULL;
	hm->hm_next = NULL;
	hm->hm_prev = NULL;

	return (hm);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_reserve                                     */
/*                                                                  */
/* Function	- Put initial hments in the reserve pool.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hment_reserve(uint_t count)
{
	hment_t	*hm;

	count += hment_reserve_amount;

	while (hment_reserve_count < count) {
		hm = kmem_cache_alloc(hment_cache, KM_NOSLEEP);
		if (hm == NULL)
			return;
		hment_put_reserve(hm);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_adjust_reserve                              */
/*                                                                  */
/* Function	- Readjust the hment reserves after they have been  */
/*		  used.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hment_adjust_reserve()
{
	hment_t	*hm;

	/*
	 * Free up any excess reserves
	 */
	while (hment_reserve_count > hment_reserve_amount) {
		ASSERT(curthread != hat_reserves_thread);
		hm = hment_get_reserve();
		if (hm == NULL)
			return;
		hment_free(hm);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_init                                        */
/*                                                                  */
/* Function	- Initialize hment data structures.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hment_init(void)
{
	int i;
	int flags = KMC_NOHASH | KMC_NODEBUG;

	/*
	 * Initialize kmem caches. 
	 */
	hment_cache = kmem_cache_create("hment_t",
	    sizeof (hment_t), 0, NULL, NULL, NULL,
	    NULL, hat_memload_arena, flags);

	hment_hash = kmem_zalloc(hment_hash_entries * sizeof (hment_t *),
	    KM_SLEEP);

	for (i = 0; i < MLIST_NUM_LOCK; i++)
		mutex_init(&mlist_lock[i], NULL, MUTEX_DEFAULT, NULL);

	for (i = 0; i < HASH_NUM_LOCK; i++)
		mutex_init(&hash_lock[i], NULL, MUTEX_DEFAULT, NULL);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_mapcnt                                      */
/*                                                                  */
/* Function	- Return the number of mappings to a page. Note     */
/*		  there is no ASSER() that the MUTEX is held for    */
/*		  this. Hence the return value might be inaccurate  */
/*		  if this is called without doing an s390x_hm_enter.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint_t
hment_mapcnt(page_t *pp)
{
	uint_t cnt;
	uint_t szc;
	page_t *larger;
	hment_t	*hm;

	s390x_hm_enter(pp);

	if (pp->p_mapping == NULL)
		cnt = 0;
	else if (pp->p_embed)
		cnt = 1;
	else
		cnt = pp->p_share;

	s390x_hm_exit(pp);

	return (cnt);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hment_steal                                       */
/*                                                                  */
/* Function	- We need to steal an hment. Walk through all the   */
/*		  page_t entries until we find one that has mult-   */
/*		  iple mappings. Unload one of the mappings and     */
/*		  reclaim that hment. Note that we'll save/restart  */
/*		  the starting page to try and spread the pain.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static hment_t *
hment_steal(void)
{
	page_t *last = last_page;
	page_t *pp = last;
	hment_t *hm = NULL;
	hment_t *hm2;
	htable_t *ht;
	uint_t found_one = 0;

	HATSTAT_INC(hs_hm_steals);
	if (pp == NULL)
		last = pp = page_first();

	while (!found_one) {
		HATSTAT_INC(hs_hm_steal_exam);
		pp = page_next(pp);
		if (pp == NULL)
			pp = page_first();

		/*
		 * The loop and function exit here if nothing found to steal.
		 */
		if (pp == last)
			return (NULL);

		/*
		 * Only lock the page_t if it has hments.
		 */
		if (pp->p_mapping == NULL || pp->p_embed)
			continue;

		/*
		 * Search the mapping list for a usable mapping.
		 */
		s390x_hm_enter(pp);
		if (!pp->p_embed) {
			for (hm = pp->p_mapping; hm; hm = hm->hm_next) {
				ht = hm->hm_htable;
				if (ht->ht_hat != kas.a_hat &&
				    ht->ht_busy == 0 &&
				    ht->ht_lock_cnt == 0) {
					found_one = 1;
					break;
				}
			}
		}
		if (!found_one)
			s390x_hm_exit(pp);
	}

	/*
	 * Steal the mapping we found.  Note that hati_page_unmap() will
	 * do the s390x_hm_exit().
	 */
	hm2 = hati_page_unmap(pp, ht, hm->hm_entry);
	ASSERT(hm2 == hm);
	last_page = pp;
	return (hm);
}

/*========================= End of Function ========================*/
