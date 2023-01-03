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

#ifndef	_VM_HTABLE_H
#define	_VM_HTABLE_H

#ifdef	__cplusplus
extern "C" {
#endif

extern void mmu_tlbflush_entry(caddr_t addr);

/*
 * Each hardware page table has an htable_t describing it.
 *
 * We use a reference counter mechanism to detect when we can free an htable.
 * In the implmentation the reference count is split into 2 separate counters:
 *
 *	ht_busy is a traditional reference count of uses of the htable pointer
 *
 *	ht_valid_cnt is a count of how references are implied by valid PTE/PTP
 *	         entries in the pagetable
 *
 * ht_busy is only incremented by htable_lookup() or htable_create()
 * while holding the appropriate hash_table mutex. While installing a new
 * valid PTE or PTP, in order to increment ht_valid_cnt a thread must have
 * done an htable_lookup() or htable_create() but not the htable_release yet.
 *
 * htable_release(), while holding the mutex, can know that if
 * busy == 1 and valid_cnt == 0, the htable can be free'd.
 *
 * The fields have been ordered to make htable_lookup() fast. Hence,
 * ht_hat, ht_vaddr, ht_level and ht_next need to be clustered together.
 */
struct htable {
	struct htable	*ht_next;	// forward link for hash table 
	struct hat	*ht_hat;	// hat this mapping comes from 
	uintptr_t	ht_vaddr;	// virt addr at start of this table 
	level_t		ht_level;	// table level: 0=PTE, 1=STE, 2=RTE3 ... 
	uint32_t	ht_flags;	// see below 
	uint32_t	ht_lock; 	// Lock word for IDTE/IPTE operations
	int32_t		ht_busy;	// implements locking protocol 
	uint16_t	ht_num_ptes;	// # of RTE/STE/PTEs in table 
	int16_t		ht_valid_cnt;	// # of valid entries in this table 
	uint32_t	ht_lock_cnt;	// # of locked entries in this table 
	uint32_t	ht_len;      	// length of table 
					// never used for kernel hat 
	void		*ht_org;	// address of the R/S/P table 
	pfn_t		ht_pfn;		// pfn of page of the execution space region table 
	pfn_t		ht_spfn;	// pfn of page of the data space region table 
	struct htable	*ht_prev;	// backward link for hash table 
	struct htable	*ht_parent;	// htable that points to this htable 
	struct htable	*ht_child; 	// htable for lower level table
	struct htable	*ht_shares;	// for HTABLE_SHARED_PFN only 
};
typedef struct htable htable_t;

/*
 * Flags values for htable ht_flags field:
 *
 * HTABLE_SHARED_PFN - this htable had it's PFN assigned from sharing another
 * 	htable. Used by hat_share() for ISM.
 */
#define	HTABLE_SHARED_PFN	(0x00000001)
#define	HTABLE_PRIMAL		(0x00000002)

/*
 * The htable hash table hashing function.  The 28 is so that high
 * order bits are include in the hash index to skew the wrap
 * around of addresses.
 */
#define	HTABLE_HASH(hat, va, lvl)					\
	((((va) >> LEVEL_SHIFT(1)) + ((va) >> 28) + (lvl)) &		\
	((hat)->hat_num_hash - 1))

/*
 * 64 bit kernels will use seg_kpm style mappings and avoid any overhead.
 *
 * Each CPU gets a unique hat_cpu_info structure in cpu_hat_info.
 */
struct hat_cpu_info {
	pfn_t hci_mapped_pfn;		/* pfn of currently mapped page table */
	s390xpte_t *hci_pagetable_va;	/* VA to use for mappings */
	s390xpte_t *hci_kernel_pte;	/* kernel PTE for cpu_pagetable_va */
	kmutex_t hci_mutex;		/* mutex to ensure sequential usage */
};

/*
 * Structure used to keep track of the htables created during boot
 */
typedef struct _RSPdescr {
	size_t	lTables;	// Length of tables
	size_t	nTables;	// Number of tables
	size_t  tLen;		// Length of a single RSP table
	size_t  nEnt;		// Number of r/s/p/te entries in 1 table
	htable_t *ht;		// Pointer to first htable entry
	void   (*init)(void *, size_t); // Initializing function
} RSPdescr_t;

/*
 * Compute the last page aligned VA mapped by an htable.
 *
 * Given a va and a level, compute the virtual address of the start of the
 * next page at that level.
 *
 */
#define	HTABLE_LAST_PAGE(ht)	((ht)->ht_vaddr - MMU_PAGESIZE + \
	((uintptr_t)((ht)->ht_num_ptes) << LEVEL_SHIFT((ht)->ht_level)))

#define	NEXT_ENTRY_VA(va, l) ((va & LEVEL_MASK(l)) + LEVEL_SIZE(l))

#if defined(_KERNEL)

/*
 * initialization function called from hat_init()
 */
extern void htable_init(void);

/*
 * Functions to lookup, or "lookup and create", the htable corresponding
 * to the virtual address "vaddr"  in the "hat" at the given "level" of
 * page tables. htable_lookup() may return NULL if no such entry exists.
 *
 * On return the given htable is marked busy (a shared lock) - this prevents
 * the htable from being stolen or freed) until htable_release() is called.
 *
 * If kalloc_flag is set on an htable_create() we can't call kmem allocation
 * routines for this htable, since it's for the kernel hat itself.
 *
 * htable_acquire() is used when an htable pointer has been extracted from
 * an hment and we need to get a reference to the htable.
 */
extern htable_t *htable_lookup(struct hat *hat, uintptr_t vaddr, level_t level);
extern htable_t *htable_create(struct hat *hat, uintptr_t vaddr, level_t level,
	htable_t *shared);
extern void htable_acquire(htable_t *);

extern void htable_release(htable_t *ht);

/*
 * Code to free all remaining htables for a hat. Called after the hat is no
 * longer in use by any thread.
 */
extern void htable_purge_hat(struct hat *hat);

/*
 * Find the htable, page table entry index, and PTE of the given virtual
 * address.  If not found returns NULL. When found, returns the htable_t *,
 * sets entry, and has a hold on the htable.
 */
extern htable_t *htable_getpte(struct hat *, uintptr_t, uint_t *, s390xpte_t *,
	level_t);

/*
 * Similar to hat_getpte(), except that this only succeeds if a valid
 * page mapping is present.
 */
extern htable_t *htable_getpage(struct hat *hat, uintptr_t va, uint_t *entry);

/*
 * Called to allocate initial/additional htables for reserve.
 */
extern void htable_initial_reserve(uint_t);
extern void htable_reserve(uint_t);

/*
 * Used to readjust the htable reserve after the reserve list has been used.
 * Also called after boot to release left over boot reserves.
 */
extern void htable_adjust_reserve(void);

/*
 * Routine to find the next populated htable at or above a given virtual
 * address. Can specify an upper limit, or HTABLE_WALK_TO_END to indicate
 * that it should search the entire address space.  Similar to
 * hat_getpte(), but used for walking through address ranges. It can be
 * used like this:
 *
 *	va = ...
 *	ht = NULL;
 *	while (va < end_va) {
 *		pte = htable_walk(hat, &ht, &va, end_va);
 *		if (!pte)
 *			break;
 *
 *		... code to operate on page at va ...
 *
 *		va += LEVEL_SIZE(ht->ht_level);
 *	}
 *	if (ht)
 *		htable_release(ht);
 *
 */
extern s390xpte_t htable_walk(struct hat *hat, htable_t **ht, uintptr_t *va,
	uintptr_t eaddr);

extern void htable_attach(void);

#define	HTABLE_WALK_TO_END ((uintptr_t)-1)

/*
 * Utilities convert between virtual addresses and page table entry indeces.
 */
extern uint_t htable_va2entry(uintptr_t va, htable_t *ht);
extern uintptr_t htable_e2va(htable_t *ht, uint_t entry);

/*
 * Interfaces that provide access to page table entries via the htable.
 *
 * Note that all accesses except s390xpte_copy() and s390xpte_zero() are atomic.
 */
extern void	  s390xpte_cpu_init(cpu_t *, void *);

extern s390xpte_t s390xpte_get(htable_t *, uint_t entry);

extern s390xpte_t s390xpte_set(htable_t *, uint_t entry, s390xpte_t new, void *);

extern s390xpte_t s390xpte_invalidate_pfn(htable_t *ht, uint_t entry);

extern s390xpte_t s390xpte_update(htable_t *ht, uint_t entry,
	s390xpte_t old, s390xpte_t new);

extern void	  s390xpte_copy(htable_t *src, htable_t *dest, uint_t entry,
	uint_t cnt);

extern void	  s390xpte_zero(htable_t *ht, uint_t entry, uint_t cnt);

extern facList_t  facilities;

/*
 * these are actually inlines for "lock; incw", "lock; decw", etc. instructions.
 */
#define	HTABLE_INC(x)		__sync_fetch_and_add((uint16_t *)&x, 1)
#define	HTABLE_DEC(x)		__sync_fetch_and_sub((uint16_t *)&x, 1)
#define	HTABLE_LOCK_INC(ht)	__sync_fetch_and_add(&(ht)->ht_lock_cnt, 1)
#define	HTABLE_LOCK_DEC(ht)	__sync_fetch_and_sub(&(ht)->ht_lock_cnt, 1)

#define LOCK_HT(ht)	__sync_lock_test_and_set(&(ht)->ht_lock, 1)
#define UNLOCK_HT(ht)	__sync_lock_release(&(ht)->ht_lock)

#endif	/* _KERNEL */


#ifdef	__cplusplus
}
#endif

#endif	/* _VM_HTABLE_H */
