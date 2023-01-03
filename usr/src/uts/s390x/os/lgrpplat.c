/*------------------------------------------------------------------*/
/* 								    */
/* Name        - lgrpplat.c 					    */
/* 								    */
/* Function    - Platform-specific support for lgroups common to    */
/*		 s390x based platforms.				    */
/* 								    */
/* 		 Those s390x platforms wanting default lgroup 	    */
/*		 behavior build with MAX_MEM_NODES = 1.  Those 	    */
/*		 s390x platforms wanting other than default	    */
/* 		 lgroup behavior build with MAX_MEM_NODES > 1.      */
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

/*
 * For now, there are 0 or 1 memnodes per lgroup on s390x based platforms,
 * plus the root lgroup.
 */
#define	NLGRP	1

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/cpuvar.h>
#include <sys/lgrp.h>
#include <sys/memnode.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <vm/seg_spt.h>
#include <vm/seg_vn.h>
#include <vm/vm_dep.h>

#include <sys/errno.h>
#include <sys/kstat.h>
#include <sys/cmn_err.h>
#include <sys/memlist.h>
#include <sys/sysmacros.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern struct memlist *phys_avail;
extern struct memlist *phys_install;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static pgcnt_t lgrp_plat_mem_size_default(lgrp_handle_t, lgrp_mem_query_t);
int plat_lgrphand_to_mem_node(lgrp_handle_t);
lgrp_handle_t plat_mem_node_to_lgrphand(int);
void plat_assign_lgrphand_to_mem_node(lgrp_handle_t, int);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*
 * Allocate lgrp and lgrp stat arrays statically.
 */
struct lgrp_stats lgrp_stats[NLGRP];

static int nlgrps_alloc;
static lgrp_t lgrp_space[NLGRP];

/*
 * Arrays mapping lgroup handles to memnodes and vice versa.  This helps
 * manage a copy-rename operation during DR, which moves memory from one
 * board to another without changing addresses/pfns or memnodes.
 */
int lgrphand_to_memnode[MAX_MEM_NODES];
int memnode_to_lgrphand[MAX_MEM_NODES];


int mpo_disabled = 0;
lgrp_handle_t lgrp_default_handle = LGRP_DEFAULT_HANDLE;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_init.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lgrp_plat_init(void)
{
	int i;

	/*
	 * Initialize lookup tables to invalid values so we catch
	 * any illegal use of them.
	 */
	for (i = 0; i < MAX_MEM_NODES; i++) {
		memnode_to_lgrphand[i] = -1;
		lgrphand_to_memnode[i] = -1;
	}

	if (lgrp_topo_ht_limit() == 1) {
		max_mem_nodes = 1;
		return;
	}

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_main_init.                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lgrp_plat_main_init(void)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_config.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lgrp_plat_config(lgrp_config_flag_t flag, uintptr_t arg)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_cpu_to_hand.                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

lgrp_handle_t
lgrp_plat_cpu_to_hand(processorid_t id)
{
	return (LGRP_DEFAULT_HANDLE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_pfn_to_hand.                            */
/*                                                                  */
/* Function	- Return the platform handle of the lgroup that     */
/*		  contains the physical memory corresponding to the */
/*		  given page frame number.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

lgrp_handle_t
lgrp_plat_pfn_to_hand(pfn_t pfn)
{
	int	mnode;

	if (lgrp_topo_ht_limit() == 1 || max_mem_nodes == 1)
		return (LGRP_DEFAULT_HANDLE);

	if (pfn > physmax)
		return (LGRP_NULL_HANDLE);

	mnode = PFN_2_MEM_NODE(pfn);
	if (mnode < 0)
		return (LGRP_NULL_HANDLE);

	return (MEM_NODE_2_LGRPHAND(mnode));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_max_lgrps.                              */
/*                                                                  */
/* Function	- Return the maximum number of supported lgroups.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
lgrp_plat_max_lgrps(void)
{
	return (NLGRP);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_mem_size.                               */
/*                                                                  */
/* Function	- For query of LGRP_MEM_SIZE_FREE, return the 	    */
/* 		  number of base pagesize pages on freelists.  	    */
/*		  For query of LGRP_MEM_SIZE_AVAIL, return the	    */
/* 		  number of allocatable base pagesize pages 	    */
/*		  corresponding to the lgroup.			    */
/* 		  For query of LGRP_MEM_SIZE_INSTALL, return the    */
/*		  amount of physical memory installed, regardless   */
/*		  of whether or not it's usable.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pgcnt_t
lgrp_plat_mem_size(lgrp_handle_t plathand, lgrp_mem_query_t query)
{
	int	mnode;
	pgcnt_t	npgs = (pgcnt_t)0;


	if (lgrp_topo_ht_limit() == 1 || max_mem_nodes == 1 || mpo_disabled ||
	    plathand == LGRP_DEFAULT_HANDLE)
		return (lgrp_plat_mem_size_default(plathand, query));

	if (plathand != LGRP_NULL_HANDLE) {
		mnode = plat_lgrphand_to_mem_node(plathand);
		if (mnode >= 0 && mem_node_config[mnode].exists) {
			switch (query) {
			case LGRP_MEM_SIZE_FREE:
				npgs = MNODE_PGCNT(mnode);
				break;
			case LGRP_MEM_SIZE_AVAIL:
				npgs = mem_node_memlist_pages(mnode,
				    phys_avail);
				break;
			case LGRP_MEM_SIZE_INSTALL:
				npgs = mem_node_memlist_pages(mnode,
				    phys_install);
				break;
			default:
				break;
			}
		}
	}
	return (npgs);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_latency.                                */
/*                                                                  */
/* Function	- Return latency between "from" and "to" lgroups    */
/* 		  If "from" or "to" is LGRP_NONE, then just return  */
/*		  latency within other lgroup.  This latency number */
/*		  can only be used for relative comparison between  */
/*		  lgroups on the running system, cannot be used     */
/*		  across platforms, and may not reflect the actual  */
/*		  latency.  It is platform and implementation 	    */
/*		  specific, so platform gets to decide its value.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
lgrp_plat_latency(lgrp_handle_t from, lgrp_handle_t to)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_root_hand.                              */
/*                                                                  */
/* Function	- Return platform handle for root lgroup.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

lgrp_handle_t
lgrp_plat_root_hand(void)
{
	return (LGRP_DEFAULT_HANDLE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_mem_size_default.                       */
/*                                                                  */
/* Function	- Return the number of free, allocatable, or 	    */
/*		  installed pages in an lgroup.			    */
/* 		  This is a copy of the MAX_MEM_NODES == 1 version  */
/*		  of the routine used when MPO is disabled (i.e.    */
/*		  single lgroup).				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static pgcnt_t
lgrp_plat_mem_size_default(lgrp_handle_t lgrphand, lgrp_mem_query_t query)
{
	struct memlist *mlist;
	pgcnt_t npgs = 0;

	switch (query) {
	case LGRP_MEM_SIZE_FREE:
		return ((pgcnt_t)freemem);
	case LGRP_MEM_SIZE_AVAIL:
		memlist_read_lock();
		for (mlist = phys_avail; mlist; mlist = mlist->next)
			npgs += btop(mlist->size);
		memlist_read_unlock();
		return (npgs);
	case LGRP_MEM_SIZE_INSTALL:
		memlist_read_lock();
		for (mlist = phys_install; mlist; mlist = mlist->next)
			npgs += btop(mlist->size);
		memlist_read_unlock();
		return (npgs);
	default:
		return ((pgcnt_t)0);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_lgrphand_to_mem_node.                        */
/*                                                                  */
/* Function	- Return the memnode associated with the specified  */
/*		  lgroup handle.               		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
plat_lgrphand_to_mem_node(lgrp_handle_t plathand)
{
	int mnode;

	if (lgrp_topo_ht_limit() == 1 || mpo_disabled || max_mem_nodes == 1)
		return (-1);

	/*
	 * We should always receive a valid pointer to a platform
	 * handle, as we can not choose the allocation policy in
	 * this layer.
	 */
	ASSERT((int)plathand >= 0 && (int)plathand < max_mem_nodes);

	mnode = lgrphand_to_memnode[(int)plathand];
	return (mnode);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_mem_node_to_lgrphand.                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

lgrp_handle_t
plat_mem_node_to_lgrphand(int mnode)
{
	if (lgrp_topo_ht_limit() == 1 || mpo_disabled || max_mem_nodes == 1)
		return (lgrp_default_handle);

	ASSERT(mnode >= 0 && mnode < max_mem_nodes);
	return (memnode_to_lgrphand[mnode]);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_assigned_lgrphand_to_mem_node.               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
plat_assign_lgrphand_to_mem_node(lgrp_handle_t plathand, int mnode)
{
	if (lgrp_topo_ht_limit() == 1 || mpo_disabled || max_mem_nodes == 1)
		return;

	ASSERT(plathand < max_mem_nodes);
	ASSERT(mnode >= 0 && mnode < max_mem_nodes);

	lgrphand_to_memnode[plathand] = mnode;
	memnode_to_lgrphand[mnode] = plathand;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_alloc.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

lgrp_t *
lgrp_plat_alloc(lgrp_id_t lgrpid)
{
	lgrp_t *lgrp;

	lgrp = &lgrp_space[nlgrps_alloc++];
	if (lgrpid >= NLGRP || nlgrps_alloc > NLGRP)
		return (NULL);
	return (lgrp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lgrp_plat_probe.                                  */
/*                                                                  */
/* Function	- Probe memory in each node from current CPU to     */
/*		  determine latency topology.  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lgrp_plat_probe(void)
{
}

/*========================= End of Function ========================*/
