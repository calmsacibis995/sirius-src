/*------------------------------------------------------------------*/
/* 								    */
/* Name        - door_support.c					    */
/* 								    */
/* Function    - Platform specific doorfs support routines.         */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - September, 2007 				    */
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
#include <sys/systm.h>
#include <sys/door.h>
#include <sys/stack.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

void lwp_setsp(klwp_t *, caddr_t);

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- door_finish_dispatch.                             */
/*                                                                  */
/* Function	- Finish up dispatch of door server function.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
door_finish_dispatch(caddr_t newsp)
{
	char bytes[MAX(MINFRAME, MINFRAME32)];
	size_t count;
	caddr_t biased_sp;

	if (get_udatamodel() == DATAMODEL_NATIVE) {
		count     = MINFRAME;
	} else {
		count     = MINFRAME32;
	}
	biased_sp = newsp;

	/*
	 * We carefully zero out the stack frame we're pointing %sp at.
	 * This means that, upon returning to userland, the locals
	 * will be zeroed, instead of acquiring whatever garbage was on the
	 * stack previously.  In particular, this makes sure %fp is NULL,
	 * so that stack traces are properly terminated.
	 */
	bzero(bytes, count);
	if (copyout(bytes, newsp, count) != 0)
		return (E2BIG);

	lwp_setsp(ttolwp(curthread), biased_sp);
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- door_final_sp.                                    */
/*                                                                  */
/* Function	- Set SP based on frame size.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uintptr_t
door_final_sp(uintptr_t resultsp, size_t align, int datamodel)
{
	size_t minframe = (datamodel == DATAMODEL_NATIVE) ? MINFRAME : MINFRAME32;
	
	return (P2ALIGN(resultsp - minframe, align));
}

/*========================= End of Function ========================*/
