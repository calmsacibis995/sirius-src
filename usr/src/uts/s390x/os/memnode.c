/*------------------------------------------------------------------*/
/* 								    */
/* Name        - memnode.c     					    */
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
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/atomic.h>
#include <sys/memlist.h>
#include <sys/memlist_plat.h>
#include <sys/memnode.h>
#include <vm/vm_dep.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

#define cas32		__sync_val_compare_and_swap
#define cas64		__sync_val_compare_and_swap
#define atomic_add_16	__sync_add_and_fetch

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

int max_mem_nodes = 1;		/* max memory nodes on this system */

struct mem_node_conf mem_node_config[MAX_MEM_NODES];
int mem_node_pfn_shift;

/*
 * num_memnodes should be updated atomically and always >=
 * the number of bits in memnodes_mask or the algorithm may fail.
 */
uint32_t num_memnodes;
mnodeset_t memnodes_mask; /* assumes 8*(sizeof(mnodeset_t)) >= MAX_MEM_NODES */

/*
 * If set, mem_node_physalign should be a power of two, and
 * should reflect the minimum address alignment of each node.
 */
uint64_t mem_node_physalign;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mem_node_add_slice.                               */
/*                                                                  */
/* Function	- Adjust the memnode config after a DR operation.   */
/*                                                                  */
/* 		  It is rather tricky to do these updates since we  */
/* 		  can't protect the memnode structures with locks,  */
/* 		  so we must be mindful of the order in which 	    */
/* 		  updates and reads to these values can occur.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mem_node_add_slice(pfn_t start, pfn_t end)
{
	int mnode;
	mnodeset_t newmask, oldmask;

	/*
	 * DR will pass us the first pfn that is allocatable.
	 * We need to round down to get the real start of
	 * the slice.
	 */
	if (mem_node_physalign) {
		start &= ~(btop(mem_node_physalign) - 1);
		end = roundup(end, btop(mem_node_physalign)) - 1;
	}

	mnode = PFN_2_MEM_NODE(start);
	ASSERT(mnode < max_mem_nodes);

	if (cas32((uint32_t *)&mem_node_config[mnode].exists, 0, 1)) {
		/*
		 * Add slice to existing node.
		 */
		if (start < mem_node_config[mnode].physbase)
			mem_node_config[mnode].physbase = start;
		if (end > mem_node_config[mnode].physmax)
			mem_node_config[mnode].physmax = end;
	} else {
		mem_node_config[mnode].physbase = start;
		mem_node_config[mnode].physmax = end;
		atomic_add_32(&num_memnodes, 1);
		do {
			oldmask = memnodes_mask;
			newmask = memnodes_mask | (1ull << mnode);
		} while (cas64(&memnodes_mask, oldmask, newmask) != oldmask);
	}
	/*
	 * Let the common lgrp framework know about the new memory
	 */
	lgrp_config(LGRP_CONFIG_MEM_ADD, mnode, MEM_NODE_2_LGRPHAND(mnode));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mem_node_pre_del_slice.                           */
/*                                                                  */
/* Function	- Preliminary checks before deleting a slice.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mem_node_pre_del_slice(pfn_t start, pfn_t end)
{
	int mnode = PFN_2_MEM_NODE(start);

	ASSERT(mnode < max_mem_nodes);
	ASSERT(mem_node_config[mnode].exists == 1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mem_node_post_del_slice.                          */
/*                                                                  */
/* Function	- Remove a PFN range from a memnode.  On some 	    */
/*		  platforms, the memnode will be created with 	    */
/*		  physbase at the first allocatable PFN, but later  */
/*		  deleted with the MC slice base address converted  */
/*		  to a PFN, in which case we need to assume 	    */
/*		  physbase and up.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mem_node_post_del_slice(pfn_t start, pfn_t end, int cancelled)
{
	int mnode;
	pgcnt_t delta_pgcnt, node_size;
	mnodeset_t omask, nmask;

	if (mem_node_physalign) {
		start &= ~(btop(mem_node_physalign) - 1);
		end = roundup(end, btop(mem_node_physalign)) - 1;
	}
	mnode = PFN_2_MEM_NODE(start);

	ASSERT(mnode < max_mem_nodes);
	ASSERT(mem_node_config[mnode].exists == 1);

	if (!cancelled) {
		delta_pgcnt = end - start;
		node_size = mem_node_config[mnode].physmax -
				mem_node_config[mnode].physbase;

		if (node_size > delta_pgcnt) {
			/*
			 * Subtract the slice from the memnode.
			 */
			if (start <= mem_node_config[mnode].physbase)
				mem_node_config[mnode].physbase = end + 1;
			ASSERT(end <= mem_node_config[mnode].physmax);
			if (end == mem_node_config[mnode].physmax)
				mem_node_config[mnode].physmax = start - 1;
		} else {

			/*
			 * Let the common lgrp framework know the mnode is
			 * leaving
			 */
			lgrp_config(LGRP_CONFIG_MEM_DEL, mnode,
			    MEM_NODE_2_LGRPHAND(mnode));

			/*
			 * Delete the whole node.
			 */
			ASSERT(MNODE_PGCNT(mnode) == 0);
			do {
				omask = memnodes_mask;
				nmask = omask & ~(1ull << mnode);
			} while (cas64(&memnodes_mask, omask, nmask) != omask);
			atomic_add_32(&num_memnodes, -1);
			mem_node_config[mnode].exists = 0;
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- startup_build_mem_nodes.                          */
/*                                                                  */
/* Function	- Build mem_nodes from the memory configuration     */
/*		  discovered at startup.       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
startup_build_mem_nodes()
{
	size_t	elem;
	pfn_t	basepfn;
	pgcnt_t	npgs;

	/* LINTED: ASSERT will always true or false */
	ASSERT(NBBY * sizeof (mnodeset_t) >= max_mem_nodes);

	/*
	 * Lists are arranged <addr, len>, ...
	 */
	for (elem = 0; elem < nMemChunk; elem++) {
		basepfn = btop((uint64_t)(sysMemory[elem].start));
		npgs    = btop(sysMemory[elem].len);
		mem_node_add_slice(basepfn, basepfn + npgs - 1);
	}
	mem_node_physalign = 0;
	mem_node_pfn_shift = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mem_node_alloc.                                   */
/*                                                                  */
/* Function	- Allocate an unassigned memnode.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
mem_node_alloc()
{
	int mnode;
	mnodeset_t newmask, oldmask;

	/*
	 * Find an unused memnode.  Update it atomically to prevent
	 * a first time memnode creation race.
	 */
	for (mnode = 0; mnode < max_mem_nodes; mnode++)
		if (cas32((uint32_t *)&mem_node_config[mnode].exists,
			0, 1) == 0)
			break;

	if (mnode >= max_mem_nodes)
			panic("Out of free memnodes\n");

	mem_node_config[mnode].physbase = (uint64_t)-1;
	mem_node_config[mnode].physmax = 0;
	atomic_add_32(&num_memnodes, 1);
	do {
		oldmask = memnodes_mask;
		newmask = memnodes_mask | (1ull << mnode);
	} while (cas64(&memnodes_mask, oldmask, newmask) != oldmask);

	return (mnode);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mem_node_memlist_pages.                           */
/*                                                                  */
/* Function	- Find the interesection between a memnode and a    */
/*		  memlist and returns the number of pages that 	    */
/*		  overlap. This routine assumes the list is protect-*/
/*		  ed from DR operations by the memlist lock.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pgcnt_t
mem_node_memlist_pages(int mnode, struct memlist *mlist)
{
	pfn_t		base, end;
	pfn_t		cur_base, cur_end;
	pgcnt_t		npgs;
	struct memlist	*pmem;

	base = mem_node_config[mnode].physbase;
	end  = mem_node_config[mnode].physmax;
	npgs = 0;

	memlist_read_lock();

	for (pmem = mlist; pmem; pmem = pmem->next) {
		cur_base = btop(pmem->address);
		cur_end = cur_base + btop(pmem->size) - 1;
		if (end <= cur_base || base >= cur_end)
			continue;
		npgs = npgs + (MIN(cur_end, end) -
		    MAX(cur_base, base)) + 1;
	}

	memlist_read_unlock();

	return (npgs);
}

/*========================= End of Function ========================*/
