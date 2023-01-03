/*------------------------------------------------------------------*/
/* 								    */
/* Name        - memlist.c  					    */
/* 								    */
/* Function    -                                                    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
/* 								    */
/* Notes       - Derived/plaigarized from sun/os/memlist.c	    */
/* 								    */
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
/* Copyright 2006 Sun Micro Systems.                                */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
/* 								    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/memlist.h>
#include <sys/memlist_plat.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- get_max_phys_size.                                */
/*                                                                  */
/* Function	- Returns the max contiguous physical memory        */
/*		  present in the memlist "physavail".		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint64_t
get_max_phys_size(
	struct memlist	*physavail)
{
	uint64_t	max_size = 0;

	for (; physavail; physavail = physavail->next) {
		if (physavail->size > max_size)
			max_size = physavail->size;
	}

	return (max_size);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- installed_top_size_memlist_array.                 */
/*                                                                  */
/* Function	- Find the page number of the highest installed     */
/*		  physical page and the number of pages installed   */
/*		  (one cannot be calculated from the other because  */
/*		  memory isn't necessarily contiguous).		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
installed_top_size_memlist_array(
	u_longlong_t *list,	/* base of array */
	size_t	nelems,		/* number of elements */
	pfn_t *topp,		/* return ptr for top value */
	pgcnt_t *sumpagesp)	/* return prt for sum of installed pages */
{
	pfn_t top = 0;
	pgcnt_t sumpages = 0;
	pfn_t highp;		/* high page in a chunk */
	size_t i;

	for (i = 0; i < nelems; i += 2) {
		highp = (list[i] + list[i+1] - 1) >> PAGESHIFT;
		if (top < highp)
			top = highp;
		sumpages += (list[i+1] >> PAGESHIFT);
	}

	*topp = top;
	*sumpagesp = sumpages;
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- installed_top_size.                               */
/*                                                                  */
/* Function	- Find the page number of the highest installed     */
/*		  physical page and the number of pages installed   */
/*		  (one cannot be calculated from the other because  */
/*		  memory isn't necessarily contiguous).		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
installed_top_size(
	struct memlist *list,	/* pointer to start of installed list */
	pfn_t *topp,		/* return ptr for top value */
	pgcnt_t *sumpagesp)	/* return prt for sum of installed pages */
{
	pfn_t top = 0;
	pfn_t highp;		/* high page in a chunk */
	pgcnt_t sumpages = 0;

	for (; list; list = list->next) {
		highp = (list->address + list->size - 1) >> PAGESHIFT;
		if (top < highp)
			top = highp;
		sumpages += (uint_t)(list->size >> PAGESHIFT);
	}

	*topp = top;
	*sumpagesp = sumpages;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- num_phys_pages.                                   */
/*                                                                  */
/* Function	- Return the number of physical pages.              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pgcnt_t
num_phys_pages()
{
	pgcnt_t npages = 0;
	struct memlist *mp;

	for (mp = phys_install; mp != NULL; mp = mp->next)
		npages += mp->size >> PAGESHIFT;

	return (npages);
}

/*========================= End of Function ========================*/
