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

#ifndef	_VM_HAT_PTE_H
#define	_VM_HAT_PTE_H

#ifdef	__cplusplus
extern "C" {
#endif


#include <sys/types.h>

/*
 * Defines for the bits in S390X Page Tables
 *
 * Notes:
 *
 * In Solaris the PAT/PWT/PCD values are set up so that:
 *
 * PAT & PWT -> Write Protected
 * PAT & PCD -> Write Combining
 * PAT by itself (PWT == 0 && PCD == 0) yields uncacheable (same as PCD == 1)
 *
 *
 * Permission bits:
 *
 * - PT_USER must be set in all levels for user pages
 * - PT_WRITE must be set in all levels for user writable pages
 * - PT_NX applies if set at any level
 *
 * For these, we use the "allow" settings in all tables above level 0 and only
 * ever disable things in PTEs.
 *
 */
#define	RS_INVALID	(0x20)	/* a valid translation is not present */
#define	PT_INVALID	(0x400)	/* a valid translation is not present */

#define PG_PROTECT	(0x200) /* Page protection is enabled	      */

/*
 * Bits 56-63 of a PTE are unassigned and ignored by the hardware. We use these
 * for our own purposes
 *
 * The software bits are used by the HAT to track attributes.
 *
 * PT_WRITABLE - The page may be written to
 *
 *
 * PT_NOSYNC - The PT_REF/PT_MOD bits are not sync'd to page_t.
 *             The hat will install them as always set.
 *
 * PT_NOCONSIST - There is no entry for this hment for this mapping.
 */
#define	PT_PTPBITS	(PT_VALID | PT_USER | PT_WRITABLE | PT_REF)
#define	PT_FLAGBITS	(0xdf)	/* for masking off flag bits 		*/
#define PT_WRITABLE	(0x01)  /* Page is writable			*/
#define PT_NX		(0x02)	/* Not executable			*/
#define PT_NOSYNC	(0x08)  /* The Ref and mod bits are not syncd 	*/
#define PT_NOCONSIST	(0x10)	/* There is no entry for this hment   	*/
#define PT_SOFTWARE	(0xd8)	/* Psuedo-software bits (x86 compat)	*/

/*
 * macros to get/set/clear the PTE fields
 */
#define	PTE_SET(p, f)	((p) |= (f))
#define	PTE_CLR(p, f)	((p) &= ~(s390xpte_t)(f))
#define	PTE_GET(p, f)	((p) & (f))

/*
 * Handy macro to check if a pagetable entry or pointer is valid
 */
#define	PTE_ISVALID(p, l)		((l) == 0 ? !((p) & PT_INVALID) : !((p) & RS_INVALID))

/*
 * does this PTE represent a page (not a pointer to another page table)?
 */
#define	PTE_ISPAGE(p, l)	\
	((l) == 0 && PTE_ISVALID(p, l))

/*
 * Handy macro to check if 2 PTE's map the same page 
 *	- ignores REF/MOD bits
 */
#define	PTE_EQUIV(a, b, l)	 (l == 0 ? (((a) & ~0x1ff) == ((b) & ~0x1ff)) \
					 : (((a) & ~0x1df) == ((b) & ~0x1df))) 

/*
 * Handy macro to check if 2 PTE's map the same page 
 *	- ignores REF/MOD/PROT bits
 */
#define	PTE_SAME(a, b, l)	 (l == 0 ? (((a) & ~0xbff) == ((b) & ~0xbff)) \
					 : (((a) & ~0x7df) == ((b) & ~0x7df))) 

/*
 * Shorthand for converting a PTE to its pfn.
 */
#define	PTE2PFN(p, l)	\
	((p) >> MMU_PAGESHIFT)

typedef uint64_t s390xpte_t;

/*
 * Macro to create a PTE from the pfn and level
 */
#define	MAKEPTE(pfn, l)	\
	(((s390xpte_t)(pfn) << MMU_PAGESHIFT))

/*
 * The idea of "level" refers to the level where the page table is used in the
 * the hardware address translation steps. The level values correspond to the
 * following names of tables used in s390x architecture documents:
 *
 *	S390x         		Level #
 *	----------------------	-------
 *	Region Table 0  	   4
 *	Region Table 1  	   3
 *	Region Table 2  	   2
 *	Segment Table		   1
 *	Page Table		   0
 *
 * The type of "level_t" is signed so that it can be used like:
 *	level_t	l;
 *	...
 *	while (--l >= 0)
 *		...
 */
#define	MAX_NUM_LEVEL		5
#define	MAX_PAGE_LEVEL		1			
typedef	int16_t level_t;
#define	LEVEL_SHIFT(l)	(mmu.level_shift[l])
#define	LEVEL_SIZE(l)	(mmu.level_size[l])
#define	LEVEL_OFFSET(l)	(mmu.level_offset[l])
#define	LEVEL_MASK(l)	(mmu.level_mask[l])

/*
 * The CR registers 1, 7, 13 hold the physical address of the 
 * top level page table for home, primary, and secondary address 
 * spaces.
 */
#define	MAKECR(pfn)    mmu_ptob(pfn)

/*
 * HAT/MMU parameters that depend on kernel mode and/or processor type
 */
struct htable;

struct hat_mmu_info {
	pfn_t highest_pfn;

	uint_t num_level;		// number of page table levels in use 
	uint_t max_level;		// just num_level - 1 
	uint_t max_page_level;		// maximum level at which we can map a page 
	uint_t ptes_per_table[MAX_NUM_LEVEL];	// # of entries in lower level page tables 
	uint_t top_level_count;		// # of entries in top most level page table 

	uint_t	hash_cnt;		// cnt of entries in htable_hash_cache 

	struct htable **kmap_htables; 	// htables for segmap 
	s390xpte_t *kmap_ptes;		// mapping of pagetables that map kmap 

	uint_t pte_size;		// either 4 or 8 
	uint_t pte_size_shift;		// either 2 or 3 

	//
	// The following tables are equivalent to PAGEXXXXX at different levels
	// in the page table hierarchy.
	//
	uint_t level_shift[MAX_NUM_LEVEL];	// PAGESHIFT for given level 
	uintptr_t level_size[MAX_NUM_LEVEL];	// PAGESIZE for given level 
	uintptr_t level_offset[MAX_NUM_LEVEL];	// PAGEOFFSET for given level 
	uintptr_t level_mask[MAX_NUM_LEVEL];	// PAGEMASK for given level 
	uintptr_t level_type[MAX_NUM_LEVEL];	// R/S/P table type

	uint_t tlb_entries[MAX_NUM_LEVEL];	// tlb entries per pagesize 
};


#if defined(_KERNEL)

# define FMT_PTE "%lx"

extern struct hat_mmu_info mmu;

#endif	/* _KERNEL */

#ifdef	__cplusplus
}
#endif

#endif	/* _VM_HAT_PTE_H */
