/*------------------------------------------------------------------*/
/* 								    */
/* Name        - kipl_mem.c 					    */
/* 								    */
/* Function    - Provide very basic alloc() and free() support      */
/* 		 during the boot stage.				    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - January, 2007					    */
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

#define	dprintf	if (memDebug & D_ALLOC) prom_printf

#define D_ALLOC	1			// Debug allocations

#define LOMEM		0x100000	// Keep 1st 1MB protected
#define MAX_VIRTMEM	-1		// Max. virtual memory size
#define MAX_PRIMALPP	PAGESIZE/sizeof(earlyAlloc) // Max per page

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/bootconf.h>
#include <sys/machs390x.h>
#include <sys/machparam.h>
#include <sys/machsystm.h>
#include <sys/memlist_plat.h>
#include <sys/mutex.h>
#include <sys/cpuvar.h>
#include <vm/mm_s390x.h>
#include <vm/hat_pte.h>
#include <vm/htable.h>
#include <vm/vm_dep.h>
#include "kipl_mem.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern	void	boot_die();

extern 	void   	*bootScratch,	// Scratch area for bkern_alloc()
		*bootScratchEnd,// Amount of available memory
		*_boot,      	// Start of the boot loader
		*_elfsz,	// End of ELF object
		*_end,  	// End of BSS
		*_ramdk,	// Start of RAM disk
		*_eramd;	// End of RAM disk

extern struct 	bootops *bop;	// The bootops structure 

extern int	physMemInit;	// pfn has has been initialized

extern struct hat_mmu_info mmu; // DAT related parameters

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void invalidateRegion(void *, size_t);
static void invalidateSegment(void *, size_t);
static void invalidatePageTbl(void *, size_t);
static htable_t *locateRSP(htable_t *, uint64_t, uint64_t, size_t,
			   size_t, void (initFn(void *, size_t)));
static caddr_t kernelRemap(caddr_t, size_t, caddr_t);
htable_t *createDATTables(memoryChunk *, int);
static size_t getRSPsize(memoryChunk *, int);
static void memlist_dump(struct memlist *);
static struct memlist * memlist_alloc(void);
static void memlist_free(struct memlist *);
static void memlist_insert(struct memlist **, uint64_t, uint64_t);
static int memlist_remove(struct memlist **, uint64_t, uint64_t);
static uint64_t memlist_find(struct memlist **, uint_t, int, void *);
static void memlists_print(void);
caddr_t bkern_alloc(caddr_t, size_t, int);
void bkern_free(caddr_t, size_t);
static void updateHtable(void *pRSP, htable_t *ht, int level, 
			 void *vAddr, htable_t *parent);
static void kipl_kpm_init(void);
static void kipl_mmu_init(void);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

//
// Region/Segment/Page Table Management
//
static htable_t   *RSPTable[5];		// RSP htables
RSPdescr_t RSP[5];			// RSP table descriptors
ssize_t	RSPSize;			// Size of RSP tables

//
// Memory lists 
//
struct memlist	*pinstalledp = NULL, 	// Installed memory 
		*pfreelistp  = NULL, 	// Free to use physical memory
		*pavailistp  = NULL, 	// Available physical memory
		*vfreelistp  = NULL, 	// Free to use virtual memory
		*pbooterp    = NULL,	// Memory in use by boot
		*pramdiskp   = NULL;	// Memory in use by ramdisk

void	*scratchStart  = NULL,
	*scratchEnd    = NULL,
	*scratchLimit;

int	memDebug = -1;

static  int memlist_init = 0;

//
// Track allocations made before page hash has been created
//
earlyAlloc *primalPages = NULL;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- init_DAT.                                         */
/*                                                                  */
/* Function	- Allocate space for nucleus Region, Segment and    */
/*		  Page Tables.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

htable_t *
init_DAT(memoryChunk *sysMem, int nChunk)
{
	int iRegion,			// Index through region tables
	    nRegion;			// Number of region tables

	htable_t *pRsp;			// RSP tables created

	char mask;			// System mask from STOSM
	
	ctlr13 datTable;		// Address space control

	kipl_mmu_init();

	if ((pRsp = createDATTables(sysMem, nChunk)) != NULL) {

		bzero(&datTable, sizeof(datTable));
		datTable.to     = (uint64_t) RSPTable[4]->ht_org >> 12;
		datTable.dsgCtl	= RTT1;
		datTable.tblLen = RSPTable[4]->ht_len / MMU_PAGESIZE - 1;
		
		__asm__ ("	lay	2,%0\n"
			 "	lay	1,%1\n"
			 "	lctlg	1,1,0(1)\n"
			 "#	lctlg	7,7,0(1)\n"
			 "#	lctlg	13,13,0(1)\n"
			 "	stosm	0(2),0x04\n"
			 "	sacf	0\n"
			 : "=m" (mask)
			 : "m" (datTable)
			 : "1", "2", "memory", "cc");

		prom_printf("DAT Enabled using RTO %p\n",
			    (void *) (datTable.to << 12));

		kipl_kpm_init();

	} else  
		prom_printf("Unable to build DAT tables - no storage available\n");

	return (pRsp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- createDATTables.                                  */
/*                                                                  */
/* Function	- Create enough of the Region/Segment/Page tables   */
/*		  to provide a 64-bit address space that maps	    */
/*		  all of real memory (i.e. 1-to-1).		    */
/*		                               		 	    */
/* Note		- RSPTLens has already been calculated in startup.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

htable_t *
createDATTables(memoryChunk *sysMemory, int nChunk)
{
	size_t	lHtable,	// Length of htables
		offset,		// Offset into R/S/P table
		lRSP;		// Length of R/S/P tables
		
	rte	*regionTbl;	// Current region table
	ste	*segmentTbl;	// Current segment table
	pte	*pageTbl;	// Current page table

	void	*prev,		// Previous RSP table
		*vAddr,		// Current vAddr
		*pRSP;		// Base of Region/Segment/Page Tables

	htable_t *pHtable,	// Base of htables
		 *ht,		// Current htable pointer
		 *cht;		// Current child htable pointer

	int	iPfra;		// Index through page frames

	uintptr_t pfra;		// Page frame address

	lRSP 	= getRSPsize(sysMemory, nChunk);
	lHtable	= (RSP[0].nTables + RSP[1].nTables + RSP[2].nTables +
		  RSP[3].nTables + RSP[4].nTables) * sizeof(htable_t);
	pRSP    = (void *) resalloc(RES_MAINMEM, lRSP, NULL, MMU_PAGESIZE);
	pHtable	= (void *) resalloc(RES_MAINMEM, lHtable, NULL, sizeof(long));
	RSPSize	= lRSP + lHtable;

	if (pRSP == NULL)
		return (NULL);

	updateHtable(pRSP, pHtable, 4, NULL, NULL);

	/*----------------------------------------------------------*/
	/* Create enough Region/Segment/Page tables to map real     */
	/* memory but also enough to cover the 64-bit address space */
	/*----------------------------------------------------------*/
		
	/*----------------------------------------------------------*/
	/* Create the Region 1 tables            	            */
	/*----------------------------------------------------------*/
	for (ht = RSP[4].ht; ht != NULL; ht = ht->ht_next) {
		for (cht = ht->ht_child; cht != NULL; cht = cht->ht_next) {
			offset		    = (cht->ht_vaddr - ht->ht_vaddr) >> 53;
			regionTbl	    = ((rte *) ht->ht_org + offset);
			regionTbl->rsto     = ((uint64_t) cht->ht_org >> 12);
			regionTbl->type     = RTT1;
			regionTbl->invalid  = 0;
			regionTbl->len      = cht->ht_len / MMU_PAGESIZE - 1;
			++ht->ht_valid_cnt;
		}
	}
	
	/*----------------------------------------------------------*/
	/* Create the Region 2 tables            	            */
	/*----------------------------------------------------------*/
	for (ht = RSP[3].ht; ht != NULL; ht = ht->ht_next) {
		for (cht = ht->ht_child; cht != NULL; cht = cht->ht_next) {
			offset		    = (cht->ht_vaddr - ht->ht_vaddr) >> 42;
			regionTbl	    = ((rte *) ht->ht_org + offset);
			regionTbl->rsto     = ((uint64_t) cht->ht_org >> 12);
			regionTbl->type     = RTT2;
			regionTbl->invalid  = 0;
			regionTbl->len      = cht->ht_len / MMU_PAGESIZE - 1;
			++ht->ht_valid_cnt;
		}
	}
	
	/*----------------------------------------------------------*/
	/* Create the Region 3 tables            	            */
	/*----------------------------------------------------------*/
	for (ht = RSP[2].ht; ht != NULL; ht = ht->ht_next) {
		for (cht = ht->ht_child; cht != NULL; cht = cht->ht_next) {
			offset		    = (cht->ht_vaddr - ht->ht_vaddr) >> 31;
			regionTbl	    = ((rte *) ht->ht_org + offset);
			regionTbl->rsto     = ((uint64_t) cht->ht_org >> 12);
			regionTbl->type     = RTT3;
			regionTbl->invalid  = 0;
			regionTbl->len      = cht->ht_len / MMU_PAGESIZE - 1;
			++ht->ht_valid_cnt;
		}
	}
	
	/*----------------------------------------------------------*/
	/* Create the Segment tables             	            */
	/*----------------------------------------------------------*/
	for (ht = RSP[1].ht; ht != NULL; ht = ht->ht_next) {
		for (cht = ht->ht_child; cht != NULL; cht = cht->ht_next) {
			offset		    = (cht->ht_vaddr - ht->ht_vaddr) >> 20;
			segmentTbl	    = ((ste *) ht->ht_org) + offset;
			segmentTbl->pto	    = ((uint64_t) cht->ht_org >> 11);
			segmentTbl->invalid = 0;
			++ht->ht_valid_cnt;
		}
	}
	
	/*----------------------------------------------------------*/
	/* Create the Page tables                	            */
	/*----------------------------------------------------------*/
	for (ht = RSP[0].ht; ht != NULL; ht = ht->ht_next) {
		pageTbl = (pte *) ht->ht_org;
		pfra    = (uintptr_t) ht->ht_vaddr;
		for (iPfra = 0; iPfra < PT_SINGLE; iPfra++) {
			if (pfra >= COREHEAP_BASE) 
				break;
			pageTbl->pfra    = pfra >> 12;
			pageTbl->invalid = 0;
			pfra		 = pfra + MMU_PAGESIZE;
			pageTbl++;
			++ht->ht_valid_cnt;
		}
	}

	return (RSPTable[4]);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- updateHTable.                                     */
/*                                                                  */
/* Function	- Create htable entries for the given level of table*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
updateHtable(void *pRSP, htable_t *ht, int level, 
	     void *vAddr, htable_t *parent) 
{
	htable_t *prev = NULL;	// Previous htable_t

	size_t iRSP;		// Index through RSP table

	for (iRSP = 0; iRSP < RSP[level].nTables; iRSP++) {
		if (iRSP == 0) {
			RSP[level].ht  	= ht;
			RSPTable[level]	= ht;
		}
		ht->ht_org       = pRSP;
		ht->ht_pfn       = (uintptr_t) pRSP >> 12;
		ht->ht_level     = level;
		ht->ht_parent    = parent;
		ht->ht_prev      = prev;
		ht->ht_valid_cnt = 0;
		if (prev != NULL) 
			prev->ht_next = ht;
		prev	         = ht;
		ht->ht_next      = NULL;
		ht->ht_vaddr     = (uintptr_t) vAddr;
		ht->ht_len       = RSP[level].tLen;
		ht->ht_num_ptes  = RSP[level].nEnt;
		ht->ht_flags	 = HTABLE_PRIMAL;
		RSP[level].init(pRSP, ht->ht_len);
		pRSP	        += ht->ht_len;
		if (level > 0) {
			ht++;
			prev->ht_child = ht;
			updateHtable(pRSP, ht, level-1, vAddr, ht);
		} else {
			ht->ht_child = NULL;
			vAddr       += SEGMSIZE;
			ht++;
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- invalidateRegion.                                 */
/*                                                                  */
/* Function	- Invalidate all entries in the region table.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void 
invalidateRegion(void *tAddr, size_t tblLen)
{
	size_t	iRSP;
	rte	*regionTbl = tAddr;

	bzero(tAddr, tblLen);
	for (iRSP = 0; iRSP < (tblLen / sizeof(rte)); iRSP++)
		regionTbl[iRSP].invalid = 1;
	
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- invalidateSegment.                                */
/*                                                                  */
/* Function	- Invalidate all entries in the segment table.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void 
invalidateSegment(void *tAddr, size_t tblLen)
{
	size_t	iRSP;
	ste	*segmentTbl = tAddr;

	bzero(tAddr, tblLen);
	for (iRSP = 0; iRSP < (tblLen / sizeof(ste)); iRSP++)
		segmentTbl[iRSP].invalid = 1;
	
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- invalidatePageTbl.                                */
/*                                                                  */
/* Function	- Invalidate all entries in a page table.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void 
invalidatePageTbl(void *tAddr, size_t tblLen)
{
	size_t iRSP;
	pte	*pageTbl = tAddr;

	bzero(tAddr, tblLen);
	for (iRSP = 0; iRSP < (tblLen / sizeof(pte)); iRSP++)
		pageTbl[iRSP].invalid = 1;
	
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kernelRemap.                                      */
/*                                                                  */
/* Function	- Remap an area within the kernel starting at rAddr */
/*		  for size bytes and mapping it to mapAddr.  	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static caddr_t
kernelRemap(caddr_t rAddr, size_t size, caddr_t mapAddr)
{
	rte *r1Entry,
	    *r2Entry,
	    *r3Entry,
	    *r1Table,
	    *r2Table,
	    *r3Table;

	ste *stEntry,
	    *stTable;

	pte *ptEntry,
	    *ptTable;

	htable_t *r1,
		 *r2,
		 *r3,
		 *st,
		 *pt;
	
	uint64_t vAddr = (uint64_t) mapAddr;

	size_t	nPages,
		iPages;

	char	mask;

	if (vAddr == (uint64_t) rAddr) 
		return(mapAddr);

	__asm__ __volatile__
		("	lay	1,%0\n"
		 "	stnsm	0(1),0xfb\n"
		 : "=m" (mask) : : "1", "memory", "cc");

	nPages  = (size / MMU_PAGESIZE) + ((size % MMU_PAGESIZE) != 0);

	for (iPages = 0; iPages < nPages; iPages++) {

		r1      = RSPTable[4];
		r2      = locateRSP(RSPTable[3], 
				    (vAddr & R12_MASK), 
				    R1_MASK,
				    R2_SINGLE, sizeof(rte),
				    invalidateRegion);
		r3      = locateRSP(RSPTable[2], 
				    (vAddr & R123_MASK), 
				    R12_MASK,
				    R3_SINGLE, sizeof(rte),
				    invalidateRegion);
		st      = locateRSP(RSPTable[1], 
				    (vAddr & R123S_MASK),
				    R123_MASK,
				    ST_SINGLE, sizeof(ste),
				    invalidateSegment);
		pt      = locateRSP(RSPTable[0], 
				    (vAddr & R123SP_MASK), 
				    R123S_MASK,
				    PT_SINGLE, sizeof(pte),
				    invalidatePageTbl);

		r1Table = r1->ht_org;
		r1Entry = (rte *) (((vAddr & R1_MASK) >> 53) * sizeof(rte) +
			  (uint64_t) r1Table); 
		if (r1Entry->invalid == 1) {
			r1Entry->invalid = 0;
			r1Entry->rsto    = ((uint64_t) r2->ht_org >> 12);
			r1Entry->type    = RTT1;
			r1Entry->invalid = 0;
			r1Entry->len     = r2->ht_len / MMU_PAGESIZE - 1;
			HTABLE_INC(r1->ht_valid_cnt);
		}
		r2Table = (rte *) r2->ht_org;

		r2Entry = (rte *) ((((vAddr & R12_MASK) - r2->ht_vaddr) >> 42) * sizeof(rte) + 
			  (uint64_t) r2Table);
		if (r2Entry->invalid == 1) {
			r2Entry->invalid = 0;
			r2Entry->rsto	 = ((uint64_t) r3->ht_org >> 12);
			r2Entry->type    = RTT2;
			r2Entry->invalid = 0;
			r2Entry->len     = r3->ht_len / MMU_PAGESIZE - 1;
			HTABLE_INC(r2->ht_valid_cnt);
		}
		r3Table	= (rte *) r3->ht_org;
		
		r3Entry = (rte *) ((((vAddr & R123_MASK) - r3->ht_vaddr) >> 31) * sizeof(rte) + 
			  (uint64_t) r3Table);
		if (r3Entry->invalid == 1) {
			r3Entry->invalid = 0;
			r3Entry->rsto	 = ((uint64_t) st->ht_org >> 12);
			r3Entry->type    = RTT3;
			r3Entry->invalid = 0;
			r3Entry->len     = st->ht_len / MMU_PAGESIZE - 1;
			HTABLE_INC(r3->ht_valid_cnt);
		}
		stTable	= (ste *) st->ht_org;

		stEntry = (ste *) ((((vAddr & R123S_MASK) - st->ht_vaddr) >> 20) * sizeof(ste) + 
			  (uint64_t) stTable);
		if (stEntry->invalid != 0) {
			stEntry->invalid = 0;
			stEntry->pto	 = ((uint64_t) pt->ht_org >> 11);
			HTABLE_INC(st->ht_valid_cnt);
		}
		ptTable	= (pte *) pt->ht_org;

		ptEntry = (pte *) ((((vAddr & R123SP_MASK) - pt->ht_vaddr) >> 12) * sizeof(pte) + 
			  (uint64_t) ptTable);
		ptEntry->invalid = 0;
		ptEntry->pfra	 = ((uint64_t) rAddr >> 12);
		HTABLE_INC(pt->ht_valid_cnt);
	
		vAddr += MMU_PAGESIZE;
		rAddr += MMU_PAGESIZE;
	}

	__asm__ __volatile__
		("	lay	1,%0\n"
		 "	stosm	0(1),0x04\n"
		 : "=m" (mask) : : "1", "memory", "cc");

	return (mapAddr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- locateRSP.                                        */
/*                                                                  */
/* Function	- Locate an RSP descriptor table in the list, if it */
/*		  cannot be found then create a new one and init-   */
/*	 	  ialize the underlying table.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static htable_t *
locateRSP(htable_t *base, 
	  uint64_t origin, 
	  uint64_t mask,   
	  size_t numEnts,
	  size_t entSize,
	  void (initFn(void *, size_t)))
{
	htable_t *next,
		 *last,
		 *ht,
		 *new;

	size_t	tblSize;

	for (ht = base; ht != NULL; ht = ht->ht_next) {
		if ((origin >= ht->ht_vaddr) && 
		    (origin <= HTABLE_LAST_PAGE(ht))) {
			return (ht);
		}
		last = ht;
	}

	tblSize	          = entSize * numEnts;
	new = last->ht_next = (void *) bop_allreal(bop, sizeof(htable_t), sizeof(long));
	new->ht_vaddr     = origin & mask;
	new->ht_org       = (void *) bop_allreal(bop, tblSize, MMU_PAGESIZE);
	new->ht_pfn       = (uintptr_t) new->ht_org >> MMU_PAGESHIFT;
	new->ht_len       = roundup(tblSize, MMU_PAGESIZE);
	new->ht_level	  = base->ht_level;
	new->ht_flags	  = HTABLE_PRIMAL;
	new->ht_num_ptes  = numEnts;
	new->ht_busy      = 1;
	new->ht_lock      = 0;
	new->ht_next      = NULL;
	new->ht_prev      = last;
	new->ht_parent    = base->ht_parent;
	new->ht_valid_cnt = 0;
	if ((new->ht_parent != NULL) && 
	    (new->ht_parent->ht_child == NULL))
		new->ht_parent->ht_child = new;
	initFn(new->ht_org, new->ht_len);

	return (new);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getRSPSize.                                       */
/*                                                                  */
/* Function	- Determine the amount of storage required to       */
/*		  hold DAT tables for the size of the system at     */
/*		  this time.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static size_t 
getRSPsize(memoryChunk *sysMem, int nChunk)
{			
	size_t 	nRegion1  = 2048,
		nRegion2  = 0,
		nRegion3  = 0,
		nSegTbls  = 0,
		nPageTbls = 0,
		rspLength;

	int	iChunk;

	for (iChunk = 0; iChunk < nChunk; iChunk++) {
		size_t sysSize;

		sysSize     = sysMem[iChunk].len;

		nRegion2   += MAX(COUNT_RSP(sysSize, ASPACE_CHUNK42), R2_SINGLE);
		nRegion3   += MAX(COUNT_RSP(sysSize, ASPACE_CHUNK31), R3_SINGLE);
		nSegTbls   += MAX(COUNT_RSP(sysSize, ASPACE_CHUNK20), ST_SINGLE);
		nPageTbls  += COUNT_RSP(sysSize, MMU_PAGESIZE);  
	}

	RSP[4].nTables = nRegion1 / R1_SINGLE + (nRegion1 % R1_SINGLE != 0);
	RSP[3].nTables = nRegion2 / R2_SINGLE + (nRegion2 % R2_SINGLE != 0);
	RSP[2].nTables = nRegion3 / R3_SINGLE + (nRegion3 % R3_SINGLE != 0);
	RSP[1].nTables = nSegTbls / ST_SINGLE + (nSegTbls % ST_SINGLE != 0);
	RSP[0].nTables = nPageTbls / PT_SINGLE;

	RSP[4].lTables = roundup(nRegion1  * sizeof(rte), MMU_PAGESIZE);
	RSP[3].lTables = roundup(nRegion2  * sizeof(rte), MMU_PAGESIZE);
	RSP[2].lTables = roundup(nRegion3  * sizeof(rte), MMU_PAGESIZE);
	RSP[1].lTables = roundup(nSegTbls  * sizeof(ste), MMU_PAGESIZE);
	RSP[0].lTables = roundup(nPageTbls * sizeof(pte), MMU_PAGESIZE);

	RSP[4].tLen    = R1_SINGLE * sizeof(rte);
	RSP[3].tLen    = R2_SINGLE * sizeof(rte);
	RSP[2].tLen    = R3_SINGLE * sizeof(rte);
	RSP[1].tLen    = ST_SINGLE * sizeof(ste);
	RSP[0].tLen    = PT_SINGLE * sizeof(pte);

	RSP[4].nEnt    = R1_SINGLE;
	RSP[3].nEnt    = R2_SINGLE;
	RSP[2].nEnt    = R3_SINGLE;
	RSP[1].nEnt    = ST_SINGLE;
	RSP[0].nEnt    = PT_SINGLE;

	RSP[4].init    = invalidateRegion;
	RSP[3].init    = invalidateRegion;
	RSP[2].init    = invalidateRegion;
	RSP[1].init    = invalidateSegment;
	RSP[0].init    = invalidatePageTbl;

	rspLength   = RSP[0].lTables + RSP[1].lTables + RSP[2].lTables +
		      RSP[3].lTables + RSP[4].lTables;
	
	return (rspLength);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlist_dump.                                     */
/*                                                                  */
/* Function	- Dump a specific memory list.                      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
memlist_dump(struct memlist *listp)
{
	while (listp) {
		dprintf("(0x%x%x, 0x%x%x)",
		    (int)(listp->address >> 32), (int)listp->address,
		    (int)(listp->size >> 32), (int)listp->size);
		listp = listp->next;
	}
	dprintf("\n");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlist_alloc.                                    */
/*                                                                  */
/* Function	- Allocate storage for a memory list.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static struct memlist *
memlist_alloc()
{
	return ((struct memlist *)kipl_alloc(sizeof (struct memlist)));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlist_free.                                     */
/*                                                                  */
/* Function	- Free storage occupied by a memory list.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
memlist_free(struct memlist *buf)
{
	kipl_free(buf, sizeof (struct memlist));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlist_insert.                                   */
/*                                                                  */
/* Function	- Insert in the order of addresses.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
memlist_insert(struct memlist **listp, uint64_t addr, uint64_t size)
{
	struct memlist *entry;
	struct memlist *prev = 0, *next;

	/* find the location in list */
	next = *listp;
	while (next && next->address < addr) {
		prev = next;
		next = prev->next;
	}

	if (prev == 0) {
		entry = memlist_alloc();
		entry->address = addr;
		entry->size = size;
		entry->next = *listp;
		*listp = entry;
		return;
	}

	/* coalesce entries if possible */
	if (addr == prev->address + prev->size) {
		prev->size += size;
	} else {
		entry = memlist_alloc();
		entry->address = addr;
		entry->size = size;
		entry->next = next;
		prev->next = entry;
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlist_remove.                                   */
/*                                                                  */
/* Function	- Delete memory chunks, assuming list is sorted by  */
/*		  address.                     		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
memlist_remove(struct memlist **listp, uint64_t addr, uint64_t size)
{
	struct memlist *entry;
	struct memlist *prev = 0, *next;

	/* find the location in list */
	next = *listp;
	while (next && (next->address + next->size < addr)) {
		prev = next;
		next = prev->next;
	}

	if (next == 0 || (addr < next->address)) {
		if (memlist_init) {
			dprintf("memlist_remove: addr 0x%lx, size 0x%lx"
			    " not contained in list\n",
			    addr, size);
			memlist_dump(*listp);
			return (-1);
		} else
			return (0);
	}

	if (addr > next->address) {
		uint64_t oldsize = next->size;
		next->size = addr - next->address;
		if ((next->address + oldsize) > (addr + size)) {
			entry          = memlist_alloc();
			entry->address = addr + size;
			entry->size    = next->address + oldsize - addr - size;
			entry->next    = next->next;
			next->next     = entry;
		}
	} else if ((next->address + next->size) > (addr + size)) {
		/* addr == next->address */
		next->address = addr + size;
		next->size -= size;
	} else {
		/* the entire chunk is deleted */
		if (prev == 0) {
			*listp = next->next;
		} else {
			prev->next = next->next;
		}
		memlist_free(next);
	}

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlist_find.                                     */
/*                                                                  */
/* Function	- Find and claim a memory chunk of a given size,    */
/*		  bypassing scratch memory and room below 16MB.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint64_t
memlist_find(struct memlist **listp, uint_t size, int align, void *base)
{
	uint64_t delta;
	uint64_t paddr;
	struct memlist *prev = 0, *next;

	/* find the chunk with sufficient size */
	next = *listp;
	while (next && 
		(next->address < (uint64_t) base ||
	         next->size < size + align - 1)) {
		prev = next;
		next = prev->next;
	}

	if (next == NULL) {
		prom_printf("Could not find chunk with size of at least %d\n",size);
		return (-1);
	}

	paddr = next->address;
	delta = (uint64_t) paddr & (align - 1);
	if (delta)
		paddr += align - delta;
	(void) memlist_remove(listp, paddr, size);
	return (paddr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlists_print.                                   */
/*                                                                  */
/* Function	- Dump all the memlists to console.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
memlists_print()
{
	prom_printf("Installed physical memory @ %p:\n",pinstalledp);
	memlist_dump(pinstalledp);
	prom_printf("Booter occupied memory (including modules) @ %p:\n",pbooterp);
	memlist_dump(pbooterp);
	prom_printf("Ramdisk memory @ %p:\n",pramdiskp);
	memlist_dump(pramdiskp);
	prom_printf("Available physical memory @ %p:\n",pavailistp);
	memlist_dump(pavailistp);
	prom_printf("Free physical memory @ %p:\n",pfreelistp);
	memlist_dump(pfreelistp);
	prom_printf("Available virtual memory @ %p:\n",vfreelistp);
	memlist_dump(vfreelistp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setup_memlists.                                   */
/*                                                                  */
/* Function	- Initialize the memlists used to track memory.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
setup_memlists(void)
{
	int 		i_Chunk;
	uint64_t	address, 
			size;
	struct memlist 	*entry;

	/*
	 * Initialize scratch memory so we can call kipl_alloc
	 * to get memory for keeping track of memory lists
	 */
	reset_alloc();

	/*
	 * Initialize RAM list (pinstalledp) 
	 */
	for (i_Chunk = 0; i_Chunk < nMemChunk; i_Chunk++) {
		memlist_insert(&pinstalledp, (uint64_t) sysMemory[i_Chunk].start, 
					     sysMemory[i_Chunk].len);
		memlist_insert(&pfreelistp,  (uint64_t) sysMemory[i_Chunk].start, 
					     sysMemory[i_Chunk].len);
		memlist_insert(&pavailistp,  (uint64_t) sysMemory[i_Chunk].start, 
					     sysMemory[i_Chunk].len);
	}

	/*
	 * Initialize memory occupied by the booter
	 */
	address = (uint64_t) _boot;
	size    = (uint64_t) &_end - address;
	size    = roundup(size, MMU_PAGESIZE);
	memlist_insert(&pbooterp, address, size);

	/*
	 * We include scratch memory in the ramdisk memlist so 
	 * that we can free it in one fell swoop and not have to 
	 * maintain another list
	 */
	memlist_insert(&pramdiskp, (uint64_t) _ramdk, (_eramd - _ramdk));
	address = (uint64_t) bootScratch;
	size    = (uint64_t) bootScratchEnd - address;
	memlist_insert(&pbooterp, address, size);

	/*
	 * Initialize free virtual memory list
	 *	start with the entire range
	 */
	memlist_insert(&vfreelistp, 0, MAX_VIRTMEM);

	/* We leave the prefix area out of the free list as well as the free lists themselves */

	/* 
	 * Delete booter memory from p/vfreelistp 
	 */
	entry = pbooterp;
	while (entry) {
		address = entry->address;
		size    = entry->size;
		(void) memlist_remove(&pfreelistp, address, size);
		(void) memlist_remove(&pavailistp, address, size);
		(void) memlist_remove(&vfreelistp, address, size);
		entry   = entry->next;
	}

	/* 
	 * Delete ramdisk memory from the p/vfreelistp. We leave it in 
	 * the availist so we can return it for general use when the 
	 * ramdisk is freed
	 */
	entry = pramdiskp;
	while (entry) {
		address = entry->address;
		size    = entry->size;
		(void) memlist_remove(&pfreelistp, address, size);
		(void) memlist_remove(&vfreelistp, address, size);
		entry   = entry->next;
	}
	(void) memlist_remove(&pfreelistp, 0, LOMEM);
	(void) memlist_remove(&pavailistp, 0, LOMEM);

	memlist_init = 1;

	if (memDebug & D_ALLOC)
		memlists_print();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- reset_alloc.                                      */
/*                                                                  */
/* Function	- Initialize scratch memory.                        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
reset_alloc(void)
{
	dprintf("initialize scratch memory \n");

	scratchLimit = bootScratchEnd;

	/* Reclaim any existing scratch memory */
	if (scratchEnd > scratchStart) {
		memlist_insert(&pfreelistp, (uint64_t) scratchStart,
		    (uint64_t) (scratchLimit - scratchStart));
	}

	/* start allocating at end of boot */
	scratchEnd = scratchStart = bootScratch;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bkern_allreal.                                    */
/*                                                                  */
/* Function	- Allocate real memory from non-scratch area.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
bkern_allreal(size_t bytes, int align)
{
	return (resalloc(RES_MAINMEM, bytes, NULL, align));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- claimPages.                                       */
/*                                                                  */
/* Function	- Claim pages that were allocated as a result of    */
/*		  an allocation of real memory.		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
claimPages(uint64_t ra, size_t bytes)
{
	page_t 	*pp;
	pfn_t	pfn;
	void 	*nAddr,
		*eAddr;

	/*
	 * Claim pages for the kernel vnode
	 */
	nAddr = (void *) ra;
	eAddr = (void *)(nAddr + bytes);
	for ( ; nAddr < eAddr; nAddr += MMU_PAGESIZE) {
		pp = page_find(&kvp, (u_offset_t)(uintptr_t) nAddr);
		if (pp == NULL) {
			pfn = va_to_pfn(nAddr);
			pp = page_numtopp(pfn, SE_EXCL);
			if (pp == NULL) 
				panic("claimPages: pp is NULL!");
					
			PP_CLRFREE(pp);
			page_hashin(pp, &kvp, (u_offset_t)(uintptr_t) nAddr, NULL);
			page_unlock(pp);
		}
	}
	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- primaltrack.                                      */
/*                                                                  */
/* Function	- Track early allocations.                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
primalTrack(caddr_t vaddr, size_t bytes)
{
	size_t ndx;

	/* Allocate a new index page...first entry used for tracking */
	if (primalPages == NULL || primalPages[0].size == MAX_PRIMALPP) {
		earlyAlloc *iPage;

		/* Recursion...but safe */
		iPage = (earlyAlloc *) resalloc(RES_BOOTSCRATCH,
						MAX_PRIMALPP * sizeof(earlyAlloc),
						NULL, 0);
		iPage->addr = primalPages;
		iPage->size = 1;
		primalPages = iPage;
	}

	ndx = primalPages[0].size++;
	primalPages[ndx].addr = vaddr;
	primalPages[ndx].size = bytes;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- resalloc.                                         */
/*                                                                  */
/* Function	- Allocate memory.                                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
caddr_t
resalloc(enum RESOURCES type, size_t bytes, caddr_t virthint, int align)
{
	uint64_t delta;
	caddr_t vaddr;
	uint64_t paddr;

	/* sanity checks */
	if (bytes == 0)
		return ((caddr_t)0);

	if (scratchEnd == 0)
		prom_panic("scratch memory uninitialized\n");

	switch (type) {
	case RES_BOOTSCRATCH:

		/* Scratch memory */

		vaddr = (caddr_t)(uintptr_t)scratchEnd;
		bytes = roundup(bytes, PAGESIZE);
		scratchEnd += bytes;
		if (scratchEnd > scratchLimit)
			prom_panic("scratch memory overflow!");
		return (vaddr);
		/*NOTREACHED*/

	case RES_CHILDVIRT:

		/* Program memory */

		delta = (uint64_t) virthint & (PAGESIZE - 1);
		if (delta) {
			dprintf("resalloc: address not page aligned: 0x%p\n",
				virthint);
			return (0);
		}

		vaddr  = virthint - delta;
		bytes += delta;
		bytes  = roundup(bytes, PAGESIZE);

		if (memlist_remove(&vfreelistp,
		    (uint64_t)(uintptr_t)vaddr, (uint64_t)bytes)) {
			dprintf("resalloc: 0x%lx bytes of virtual memory not available at 0x%p\n",
				bytes,vaddr);
			return (0);
		}
		
		if (align == 0)
			align = 1;

		paddr = memlist_find(&pfreelistp, bytes, align, NULL);
		if (paddr == -1) {
			dprintf("resalloc: No physical memory left for 0x%lx bytes at 0x%p\n",
				bytes,vaddr);
			return (0);
		}
			
		vaddr = kernelRemap((void *) paddr, bytes, vaddr);

		if (physMemInit == 0) {
			primalTrack(vaddr, bytes);
		} 

		return (vaddr);
		break;

	case RES_MAINMEM:

		/* Fixed memory - page/segment/region tables */

		bytes  = roundup(bytes, PAGESIZE);

		if (align == 0)
			align = 1;

		paddr = memlist_find(&pfreelistp, bytes, align, bootScratchEnd);
		if (paddr == -1) {
			dprintf("resalloc: No physical memory left for 0x%lx bytes\n",
				bytes);
			return (0);
		}
		memlist_remove(&vfreelistp, paddr, bytes);
			
		if (physMemInit == 0) {
			primalTrack((caddr_t) paddr, bytes);
		} else
			claimPages(paddr, bytes);

		return ((caddr_t) paddr);
		break;
		/*NOTREACHED*/
	}
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- resfree.                                          */
/*                                                                  */
/* Function	- Free storage - scratch memory is freed in one     */
/*		  shot.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
resfree(caddr_t addr, size_t bytes)
{
	if ((uint64_t) addr < (uint64_t) scratchLimit)
		return;

	dprintf("resfree: 0x%p 0x%lx not implemented\n",
		(void *)addr, bytes);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bkern_alloc.                                      */
/*                                                                  */
/* Function	- Allocate storage from free lists or from scratch. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
caddr_t
bkern_alloc(caddr_t virt, size_t size, int align)
{
	caddr_t vaddr;

	if (size < PAGESIZE) {
		vaddr = (caddr_t) kipl_alloc(size);
	} else {
		vaddr = resalloc(((virt == 0) ? RES_BOOTSCRATCH : RES_CHILDVIRT),
				 size, virt, align);
	}
	return (vaddr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bkern_free.                                       */
/*                                                                  */
/* Function	- Free storage.                                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
bkern_free(caddr_t virt, size_t size)
{
	resfree(virt, size);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kipl_kpm_init.                                    */
/*                                                                  */
/* Function	- Create the mappings required for kpm.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void 
kipl_kpm_init()
{
	int iChunk;
	caddr_t base;

	kpm_vbase = (caddr_t)(uintptr_t) SEGKPMBASE;
	kpm_size  = 0;

	prom_printf("Creating mappings for KPM\n");
	for (iChunk = 0; iChunk < nMemChunk; iChunk++) {
		base = (caddr_t) ((uintptr_t) kpm_vbase + 
				  (uintptr_t) sysMemory[iChunk].start);
		prom_printf("\tMapping %p to %p for %dMB\n",
			    base, sysMemory[iChunk].start,
			    (sysMemory[iChunk].len / MMU_SEGMSIZE));
		kernelRemap((caddr_t) sysMemory[iChunk].start, 
			    sysMemory[iChunk].len, base);
		kpm_size += sysMemory[iChunk].len;
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kipl_mmu_init.                                    */
/*                                                                  */
/* Function	- Initialize most of the mmu structure. The rest    */
/*		  will be done as part of hat initialization.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
kipl_mmu_init()
{
	int i;

	mmu.pte_size       	= sizeof(pte);
	mmu.pte_size_shift 	= 3;

	mmu.max_page_level	= 0;
	mmu_page_sizes		= 1;
	mmu_exported_page_sizes = mmu_page_sizes;

	mmu.num_level 		= 5;
	mmu.max_level 		= 4;
	mmu.ptes_per_table[0] 	= PT_SINGLE;
	mmu.ptes_per_table[1] 	= ST_SINGLE;
	mmu.ptes_per_table[2] 	= R3_SINGLE;
	mmu.ptes_per_table[3] 	= R2_SINGLE;
	mmu.ptes_per_table[4] 	= R1_SINGLE;
	mmu.top_level_count 	= R1_SINGLE;

	mmu.level_shift[0] 	= 12;
	mmu.level_shift[1] 	= 20;
	mmu.level_shift[2] 	= 31;
	mmu.level_shift[3] 	= 42;
	mmu.level_shift[4] 	= 53;

	mmu.level_type[0] 	= 0;
	mmu.level_type[1] 	= 0;
	mmu.level_type[2] 	= RTT3;
	mmu.level_type[3] 	= RTT2;
	mmu.level_type[4] 	= RTT1;

	for (i = 0; i < mmu.num_level; ++i) {
		mmu.level_size[i]   = 1UL << mmu.level_shift[i];
		mmu.level_offset[i] = mmu.level_size[i] - 1;
		mmu.level_mask[i]   = ~mmu.level_offset[i];
	}
}

/*========================= End of Function ========================*/
