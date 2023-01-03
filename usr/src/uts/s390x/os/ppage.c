/*------------------------------------------------------------------*/
/* 								    */
/* Name        - ppage.c       					    */
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

#ifdef DEBUG
# define	PP_STAT_ADD(stat)	(stat)++
#else
# define	PP_STAT_ADD(stat)
#endif

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/machs390x.h>
#include <sys/t_lock.h>
#include <sys/vmem.h>
#include <sys/mman.h>
#include <sys/vm.h>
#include <sys/cpu.h>
#include <sys/cmn_err.h>
#include <sys/cpuvar.h>
#include <sys/atomic.h>
#include <vm/as.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/seg_kmem.h>
#include <vm/hat_s390x.h>
#include <sys/debug.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

#ifdef DEBUG
uint_t pload, ploadfail;
uint_t ppzero, ppzero_short;
#endif

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- rcopy.                                            */
/*                                                                  */
/* Function	- Copy a page from real address "from" to real      */
/*		  address "to". Save the ref/mod bits on entry and  */
/*		  restore them after the copy.		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void __inline__
rcopy(caddr_t from, caddr_t to)
{
	long  	 key;
	_pfxPage *pfx = NULL;	
	
//	GET_KEY(to, key);

	__asm__ ("	lgr	0,%1\n"
		 "	lgr	1,%2\n"
		 "	lgr	3,1\n"
		 "	lgr	2,%3\n"
		 "	stnsm	%0,0\n"
		 "	mvcl	0,2\n"
		 "	ssm	%0\n"
		 : "=m" (pfx->__lc_scratch)
		 : "r" (to), "r" (MMU_PAGESIZE), "r" (from)
		 : "0", "1", "2", "3", "memory", "cc");

//	SET_KEY(to, key);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- rzero.                                            */
/*                                                                  */
/* Function	- Zero a page at real address "to".                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void __inline__
rzero(caddr_t to, uint_t len)
{
	long  	 key;
	_pfxPage *pfx = NULL;	
	
	GET_KEY(to, key);

	__asm__ ("	lgr	0,%1\n"
		 "	lgfr	1,%2\n"
		 "	lghi	3,0\n"
		 "	lgr	2,%1\n"
		 "	stnsm	%0,0\n"
		 "	mvcl	0,2\n"
		 "	ssm	%0\n"
		 : "=m" (pfx->__lc_scratch)
		 : "r" (to), "r" (len)
		 : "0", "1", "2", "3", "memory", "cc");

	SET_KEY(to, key);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ppmapin.                                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
ppmapin(page_t *pp, uint_t vprot, caddr_t avoid)
{
	caddr_t va;

	va = vmem_alloc(heap_arena, PAGESIZE, VM_SLEEP);
	hat_memload(kas.a_hat, va, pp, vprot | HAT_NOSYNC, HAT_LOAD_LOCK);
	return (va);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ppmapout.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ppmapout(caddr_t va)
{
	hat_unload(kas.a_hat, va, PAGESIZE, HAT_UNLOAD_UNLOCK);
	vmem_free(heap_arena, va, PAGESIZE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ppcopy.                                           */
/*                                                                  */
/* Function	- Copy the data from the physical page represented  */
/*		  by "frompp" to that represented by "topp".        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
ppcopy(page_t *fm_pp, page_t *to_pp)
{
	caddr_t from, to;
	int ret = 1;

	from = (fm_pp->p_pagenum << MMU_PAGESHIFT);
	to   = (to_pp->p_pagenum << MMU_PAGESHIFT);
	rcopy(from, to);
	return (ret);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pagezero.                                         */
/*                                                                  */
/* Function	- Zero the physical page from off to off + len      */
/*		  given by 'pp' without changing the reference &    */
/*		  modified bits of page.       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
pagezero(page_t *pp, uint_t off, uint_t len)
{
	caddr_t ra = (pp->p_pagenum << MMU_PAGESHIFT);

	ASSERT((int)len > 0 && (int)off >= 0 && off + len <= PAGESIZE);
	ASSERT(PAGE_LOCKED(pp));

	PP_STAT_ADD(ppzero);

	if (len != MMU_PAGESIZE) {
		PP_STAT_ADD(ppzero_short);
	}
	rzero(ra + off, len);
}

/*========================= End of Function ========================*/
