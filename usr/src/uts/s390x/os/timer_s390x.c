/*------------------------------------------------------------------*/
/* 								    */
/* Name        - timer_s390x.c.					    */
/* 								    */
/* Function    - Handle basic timer functions of initializing       */
/* 		 timers and setting interrupt vectors for them.	    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
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
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/asm_linkage.h>
#include <sys/kmem.h>
#include <sys/exts390x.h>
#include <sys/machs390x.h>
#include <sys/avintr.h>
#include <sys/intr.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static uint_t cpu_timer_handler(caddr_t arg1, caddr_t arg2);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- timers_init.                                       */
/*                                                                  */
/* Function	- Set up the clocks on s390x.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
timers_init()
{
	long *pClock;
	long ckcInit = -1;
	long cptInit = 0x7ffffffffffffff;

	pClock = &ckcInit;
	__asm__ ("	sckc	0(%0)"
		 : : "a" (pClock));

	pClock = &cptInit;
	__asm__ ("	spt 	0(%0)"
		 : : "a" (pClock));

	add_avintr(NULL, 1, cpu_timer_handler, "cpu timer",
		   S390_INTR_EXT, 0, 0, NULL, NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- timers_term.                                       */
/*                                                                  */
/* Function	- Terminate timer processing.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
timers_term()
{
	rem_avintr(NULL, 1, cpu_timer_handler, S390_INTR_EXT);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_timer_handler.                                */
/*                                                                  */
/* Function	- Handle the CPU timer interrupt.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint_t
cpu_timer_handler(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;
	long *pClock,
	     cptInit = 0x7ffffffffffffff;

	if (ip->u.ext.intcode != EXT_CPUT) {
		return (DDI_INTR_UNCLAIMED);
	}

	pClock = &cptInit;
	__asm__ ("	spt 	0(%0)"
		 : : "a" (pClock));

	/*FIXME*/

	return (DDI_INTR_CLAIMED);
}

/*========================= End of Function ========================*/
