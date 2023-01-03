/*------------------------------------------------------------------*/
/* 								    */
/* Name        - s390xmmu.c 					    */
/* 								    */
/* Function    -                                                    */
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
#include <sys/memlist_plat.h>
#include <sys/machs390x.h>
#include <sys/machsystm.h>
#include <sys/prom_debug.h>
#include <sys/errno.h>
#include <vm/as.h>
#include <vm/mm_s390x.h>
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

extern int page_relocate_ready;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static struct memlist * ndata_select_chunk(struct memlist *, size_t, size_t);
static int createDATTables(struct memlist *);
static void invalidateRegion(void *, size_t);
static void invalidateSegment(void *, size_t);
static void invalidatePageTbl(void *, size_t);
static rspList * locateRSP(rspList *, uint64_t, uint64_t, size_t,  
			   size_t, size_t, void (initFn(void *, size_t)));

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

int	sfmmu_kern_mapped = 0;

uint64_t memsegspa = (uintptr_t) MSEG_NULLPTR_PA;

//
// Region/Segment/Page Table Management
//
size_t	RSPTLens;			// Length of all RSP tables
rspList RSPTable[5];                    // RSP tables descriptors

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- va_to_pfn.                                        */
/*                                                                  */
/* Function	- Convert a virtual address to a real PFN.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pfn_t
va_to_pfn(void *vaddr)
{
	pfn_t pfn;
	int valid;

	__asm__ ("	lgr	0,%2\n"
		 "	lrag	%0,0(%2)\n"
		 "	ipm	%1\n"
		 "	srl	%1,28\n"
		 "	srlg	%0,%0,12"
		 : "=r" (pfn), "=r" (valid) : "a" (vaddr) : "cc", "0");

	if (valid == 0)
		return (pfn);

	return (PFN_INVALID);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- va_to_pa.                                         */
/*                                                                  */
/* Function	- Convert a virtual address to a real address.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint64_t
va_to_pa(void *vaddr)
{
	uint64_t pa;
	int valid;

	__asm__ ("	lgr	0,%2\n"
		 "	lrag	%0,0(%2)\n"
		 "	ipm	%1\n"
		 "	srl	%1,28\n"
		 : "=r" (pa), "=r" (valid) : "a" (vaddr) : "cc", "0");

	if (valid == 0)
		return (pa);
	else
		return (-1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- as_va_to_pa.                                      */
/*                                                                  */
/* Function	- Convert a virtual address of a given address space*/
/*		  to a real address.           		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint64_t
as_va_to_pa(void *vaddr, hat_t *hat)
{
	uint64_t pa;
	int valid;
	char mask;

	if (hat != kas.a_hat) {
		__asm__ ("	lctlg	13,13,%4\n"
			 "	lgr	0,%3\n"
			 "	stnsm	0xfb,%3\n"
			 "	sacf	256\n"
			 "	lrag	%0,0(%3)\n"
			 "	ipm	%1\n"
			 "	srl	%1,28\n"
			 "	sacf	768\n"
			 "	stosm	%3,%3\n"
			 : "=r" (pa), "=r" (valid), "=m" (mask)
			 : "a" (vaddr), "a" (hat->hat_rto)
			 : "cc", "0");
	} else { 
		__asm__ ("	lgr	0,%2\n"
			 "	lrag	%0,0(%2)\n"
			 "	ipm	%1\n"
			 "	srl	%1,28\n"
			 : "=r" (pa), "=r" (valid) : "a" (vaddr) : "cc", "0");
	}

	if (valid == 0)
		return (pa);
	else
		return (-1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hat_kern_setup.                                   */
/*                                                                  */
/* Function	- Take control over HAT functions for kernel.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hat_kern_setup(void)
{
	htable_attach();
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ndata_alloc_init.                                 */
/*                                                                  */
/* Function	- Init routine of the nucleus data memory allocator.*/
/*                                                                  */
/* 		  The nucleus data memory allocator is organized in */
/*		  CACHE_ALIGNSIZE'd memory chunks. Memory 	    */
/*		  allocated by ndata_alloc() will never be freed.   */
/*                                                                  */
/* 		  The ndata argument is used as header of the ndata */
/*		  freelist. Other freelist nodes are placed in the  */
/*		  nucleus memory itself at the beginning of a free  */
/*		  memory chunk. Therefore a freelist node (struct   */
/*		  memlist) must fit into the smallest allocatable   */
/* 		  memory chunk (CACHE_ALIGNSIZE bytes).	    	    */
/*                                                                  */
/* 		  The memory interval [base, end] passed to 	    */
/*		  ndata_alloc_init() must be bzero'd to allow the   */
/*		  allocator to return bzero'd memory easily.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ndata_alloc_init(struct memlist *ndata, uintptr_t base, uintptr_t end)
{
	int iChunk;

	void *endChunk;

	struct memlist *memChunk;

	uintptr_t ndSize;
	
	ASSERT(sizeof (struct memlist) <= CACHE_ALIGNSIZE);

	base   = roundup(base, CACHE_ALIGNSIZE);
	end    = end - end % CACHE_ALIGNSIZE;
	ndSize = end - base;

	ASSERT(base < end);

	ndata->address = base;
	ndata->size    = ndSize;
	ndata->next    = NULL;
	ndata->prev    = NULL;
	
	/*----------------------------------------------------------*/
	/* As there may be holes in physical memory we need to skip */
	/* these in the ndata memlist.				    */
	/*----------------------------------------------------------*/
	memChunk = ndata;
	endChunk = sysMemory[0].start + sysMemory[0].len;
	for (iChunk = 1; 
	     ((iChunk < nMemChunk) && (ndSize > 0)); 
             iChunk++) {
		if (endChunk < sysMemory[iChunk].start) {
			memChunk->size 	     = MIN(sysMemory[iChunk-1].len,
						   ndSize);
			memChunk->next       = sysMemory[iChunk].start;
			memChunk->next->prev = memChunk;
			memChunk             = memChunk->next;
			memChunk->next       = NULL;
			memChunk->address    = (uintptr_t) memChunk + 
						sizeof(struct memlist);
			ndSize -= MIN(sysMemory[iChunk-1].len, ndSize);
		}
		endChunk = sysMemory[iChunk].start + sysMemory[iChunk].len;
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ndata_maxsize.                                    */
/*                                                                  */
/* Function	- Deliver the size of the largest free memory chunk.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

size_t
ndata_maxsize(struct memlist *ndata)
{
	size_t chunksize = ndata->size;

	while ((ndata = ndata->next) != NULL) {
		if (chunksize < ndata->size)
			chunksize = ndata->size;
	}

	return (chunksize);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ndata_spare.                                      */
/*                                                                  */
/* Function	- This is a special function to figure out if the   */
/*		  memory chunk needed for the page structs can fit  */
/*		  in the nucleus or not. If it fits the function    */
/*		  calculates and returns the possible remaining     */
/*		  ndata size in the last element if the size needed */
/*		  for page structs would be allocated from the 	    */
/*		  nucleus.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

size_t
ndata_spare(struct memlist *ndata, size_t wanted, size_t alignment)
{
	struct memlist *frlist;
	uintptr_t base;
	uintptr_t end;

	for (frlist = ndata; frlist != NULL; frlist = frlist->next) {
		base = roundup(frlist->address, alignment);
		end = roundup(base + wanted, CACHE_ALIGNSIZE);

		if (end <= frlist->address + frlist->size) {
			if (frlist->next == NULL)
				return (frlist->address + frlist->size - end);

			while (frlist->next != NULL)
				frlist = frlist->next;

			return (frlist->size);
		}
	}

	return (0);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ndata_extra_base.                                 */
/*                                                                  */
/* Function	- Allocate the last properly aligned memory chunk.  */
/* 		  This function is called when no more large 	    */
/*		  nucleus memory chunks will be allocated.  The     */
/*		  remaining free nucleus memory at the end of the   */
/*		  nucleus can be added to the phys_avail list.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
ndata_extra_base(struct memlist *ndata, size_t alignment)
{
	uintptr_t base;
	size_t wasteage = 0;

	/*
	 * The alignment needs to be a multiple of CACHE_ALIGNSIZE.
	 */
	ASSERT((alignment % CACHE_ALIGNSIZE) ==  0);

	while (ndata->next != NULL) {
		wasteage += ndata->size;
		ndata = ndata->next;
	}

	base = roundup(ndata->address, alignment);

	if (base >= ndata->address + ndata->size)
		return (NULL);

	if (base == ndata->address) {
		if (ndata->prev != NULL)
			ndata->prev->next = NULL;
		else
			ndata->size = 0;

		bzero((void *)base, sizeof (struct memlist));

	} else {
		ndata->size = base - ndata->address;
		wasteage += ndata->size;
	}
	PRM_DEBUG(wasteage);

	return ((void *)base);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ndata_select_chunk.                               */
/*                                                                  */
/* Function	- Select the best matching buffer and avoid memory  */
/*		  fragmentation.               		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static struct memlist *
ndata_select_chunk(struct memlist *ndata, size_t wanted, size_t alignment)
{
	struct memlist *fnd_below = NULL;
	struct memlist *fnd_above = NULL;
	struct memlist *fnd_unused = NULL;
	struct memlist *frlist;
	uintptr_t base;
	uintptr_t end;
	size_t below;
	size_t above;
	size_t unused;
	size_t best_below = ULONG_MAX;
	size_t best_above = ULONG_MAX;
	size_t best_unused = ULONG_MAX;

	ASSERT(ndata != NULL);

	/*
	 * Look for the best matching buffer, avoid memory fragmentation.
	 * The following strategy is used, try to find
	 *   1. an exact fitting buffer
	 *   2. avoid wasting any space below the buffer, take first
	 *	fitting buffer
	 *   3. avoid wasting any space above the buffer, take first
	 *	fitting buffer
	 *   4. avoid wasting space, take first fitting buffer
	 *   5. take the last buffer in chain
	 */
	for (frlist = ndata; frlist != NULL; frlist = frlist->next) {
		base = roundup(frlist->address, alignment);
		end  = roundup(base + wanted, CACHE_ALIGNSIZE);

		if (end > frlist->address + frlist->size)
			continue;

		below = (base - frlist->address) / CACHE_ALIGNSIZE;
		above = (frlist->address + frlist->size - end) /
		        CACHE_ALIGNSIZE;
		unused = below + above;

		if (unused == 0) 
			return (frlist);

		if (frlist->next == NULL)
			break;

		if (below < best_below) {
			best_below = below;
			fnd_below = frlist;
		}

		if (above < best_above) {
			best_above = above;
			fnd_above = frlist;
		}

		if (unused < best_unused) {
			best_unused = unused;
			fnd_unused = frlist;
		}
	}

	if (best_below == 0)
		return (fnd_below);
	if (best_above == 0)
		return (fnd_above);
	if (best_unused < ULONG_MAX)
		return (fnd_unused);

	return (frlist);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ndata_alloc.                                      */
/*                                                                  */
/* Function	- Nucleus data memory allocator. The granularity of */
/*		  the allocator is CACHE_ALIGNSIZE. See also 	    */
/*		  comment for ndata_alloc_init().		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
ndata_alloc(struct memlist *ndata, size_t wanted, size_t alignment)
{
	struct memlist *found;
	struct memlist *fnd_above;
	uintptr_t base;
	uintptr_t end;
	size_t below;
	size_t above;

	/*
	 * Look for the best matching buffer, avoid memory fragmentation.
	 */
	if ((found = ndata_select_chunk(ndata, wanted, alignment)) == NULL)
		return (NULL);

	/*
	 * Allocate the nucleus data buffer.
	 */
	base = roundup(found->address, alignment);
	end = roundup(base + wanted, CACHE_ALIGNSIZE);
	ASSERT(end <= found->address + found->size);

	below = base - found->address;
	above = found->address + found->size - end;
	ASSERT(above == 0 || (above % CACHE_ALIGNSIZE) == 0);

	if (below >= CACHE_ALIGNSIZE) {
		/*
		 * There is free memory below the allocated memory chunk.
		 */
		found->size = below - below % CACHE_ALIGNSIZE;

		if (above) {
			fnd_above = (struct memlist *)end;
			fnd_above->address = end;
			fnd_above->size = above;

			if ((fnd_above->next = found->next) != NULL)
				found->next->prev = fnd_above;
			fnd_above->prev = found;
			found->next = fnd_above;
		}

		bzero ((void *) base, wanted);
		return ((void *) base);
	}

	if (found->prev == NULL) {
		/*
		 * The first chunk (ndata) is selected.
		 */
		ASSERT(found == ndata);
		if (above) {
			found->address = end;
			found->size = above;
		} else if (found->next != NULL) {
			found->address = found->next->address;
			found->size = found->next->size;
			if ((found->next = found->next->next) != NULL)
				found->next->prev = found;

			bzero((void *)found->address, sizeof (struct memlist));
		} else {
			found->address = end;
			found->size = 0;
		}

		bzero ((void *) base, wanted);
		return ((void *) base);
	}

	/*
	 * Not the first chunk.
	 */
	if (above) {
		fnd_above = (struct memlist *)end;
		fnd_above->address = end;
		fnd_above->size = above;

		if ((fnd_above->next = found->next) != NULL)
			fnd_above->next->prev = fnd_above;
		fnd_above->prev = found->prev;
		found->prev->next = fnd_above;

	} else {
		if ((found->prev->next = found->next) != NULL)
			found->next->prev = found->prev;
	}

	bzero((void *) found->address, sizeof (struct memlist));
	bzero ((void *) base, wanted);

	return ((void *) base);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dumpRSP.                                          */
/*                                                                  */
/* Function	- Nucleus data memory allocator. The granularity of */
/*		  the allocator is CACHE_ALIGNSIZE. See also 	    */
/*		  comment for ndata_alloc_init().		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
dumpRSP() 
{
	uint64_t *r1,*r2,*r3,*st,*pt;
	void	 *va;
	int	 iR1,iR2,iR3,iSt,iPt;

	__asm__ ("	stctg	1,1,%0\n"
		 "	la	1,%0\n"
		 "	ni	7(1),0\n"
		 : "=m" (r1) : : "0", "cc");

	va = NULL;
	prom_printf("r1 - %lx\n",r1);
	for (iR1 = 0; iR1 < R1_SINGLE; iR1++) {
		if ((*r1 & 0x20) == 0) {
			*r1 &= ~(0xfff);
			r2 = (uint64_t *) *r1;
			prom_printf("r2 - %lx\n",r2);
			for (iR2 = 0; iR2 < R2_SINGLE; iR2++) { 
				if ((*r2 & 0x20) == 0) {
					*r2 &= ~(0xfff);
					r3 = (uint64_t *) *r2;
					prom_printf("r3 - %lx\n",r3);
					for (iR3 = 0; iR3 < R3_SINGLE; iR3++) { 
						if ((*r3 & 0x20) == 0) {
							*r3 &= ~(0xfff);
							st = (uint64_t *) *r3;
							prom_printf("st - %lx\n",st);
							for (iSt = 0; iSt < ST_SINGLE; iSt++) { 
								if ((*st & 0x20) == 0) {
									*st &= ~(0x7ff);
									pt = (uint64_t *) *st;
									prom_printf("pt - %lx\n",pt);
									for (iPt = 0; iPt < PT_SINGLE; iPt++) {
										if ((*pt & 0x400) == 0) {
											*pt &= ~(0xfff);
if ((uint64_t) va != *pt)
												prom_printf("%016lx - %016lx\n",va,*pt);
										}
										va += MMU_PAGESIZE;
										pt++;	
									}
								} else { 
									va += ASPACE_CHUNK20;
								}
								st++;
							}
						} else { 
							va += ASPACE_CHUNK31;
						}
						r3++;
					}
				} else { 
					va += ASPACE_CHUNK42;
				}
				r2++;
			}
		} else { 
			va += ASPACE_CHUNK53;
		}
		r1++;
	}
}

/*========================= End of Function ========================*/
