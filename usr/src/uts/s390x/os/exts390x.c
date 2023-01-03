/*------------------------------------------------------------------*/
/* 								    */
/* Name        - exts390x.c 					    */
/* 								    */
/* Function    - Handle external interrupts for system.             */
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

#define NUM_EXT	16
#define NUM_SSG	2

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/regset.h>
#include <sys/exts390x.h>
#include <sys/errno.h>

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

static void ssg_slih(mcontext_t, short, long, void *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

//
// We define a static array of external interrupt handlers
// Architecturally there could be 64K of them but in reality
// we only need to handle a few:
// - 0x1201: emergency signal
// - 0x1202: external call 
// - 0x1004: Clock comparator
// - 0x1005: CPU timer
// - 0x1202: Malfunction alert
// - 0x1406: Timing alert
// - 0x2401: Hardware console
// - 0x2603: Service Signal - PFAULT and DIAG 250
// - 0x4000: IUCV
//

static hndext_t handlers[NUM_EXT] = {EXT_SSIG, 0, 0, 0, ssg_slih, 0};

static hndext_t ssgproc[NUM_SSG];

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hndext_set.                                       */
/*                                                                  */
/* Function	- Establish an interrupt handler for a given sub-   */
/*		  code.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int 
hndext_set(short code, hndext_proc_t handler, void *uword) 
{
	int i_hnd;

	if (code > 0) {
		for (i_hnd = 0; 
		     ((i_hnd < NUM_EXT) && (handlers[i_hnd].code != 0)); 
		     i_hnd++);
		if (i_hnd < NUM_EXT) {
			handlers[i_hnd].code    = code;
			handlers[i_hnd].handler = handler;
			handlers[i_hnd].uword   = uword;
		} else
			return -ENOMEM;
	} else
		return -EINVAL;

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hndext_clr.                                       */
/*                                                                  */
/* Function	- Clear an interrupt handler for a given subcode.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hndext_clr(short code, hndext_proc_t handler)
{
	int i_hnd;

	if (code > 0) {
		for (i_hnd = 0; 
		     ((i_hnd < NUM_EXT) && (handlers[i_hnd].code != code)); 
		     i_hnd++);
		if (i_hnd < NUM_EXT) {
			handlers[i_hnd].code    = 0;
			handlers[i_hnd].handler = 0;
			handlers[i_hnd].uword   = 0;
		} else
			return -ENOENT;
	} else
		return -EINVAL;

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ext_slih.                                         */
/*                                                                  */
/* Function	- Locate the handler for the given subcode and call */
/*		  it passing the context, code, interrupt parameter */
/*		  and the user word.           		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ext_slih(mcontext_t ctx, short code, long intparm) 
{
	int i_hnd;

	for (i_hnd = 0; i_hnd < NUM_EXT; i_hnd++) {
		if (handlers[i_hnd].code == code) {
			handlers[i_hnd].handler(ctx, code, intparm, handlers[i_hnd].uword);
			break;
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hndssg_set.                                       */
/*                                                                  */
/* Function	- Establish an interrupt handler for a SSG subcode. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int 
hndssg_set(short code, hndext_proc_t handler, void *uword) 
{
	int i_hnd;

	if (code > 0) {
		for (i_hnd = 0; 
		     ((i_hnd < NUM_SSG) && (ssgproc[i_hnd].code != 0)); 
		     i_hnd++);
		if (i_hnd < NUM_SSG) {
			ssgproc[i_hnd].code    = code;
			ssgproc[i_hnd].handler = handler;
			ssgproc[i_hnd].uword   = uword;
		} else
			return -ENOMEM;
	} else
		return -EINVAL;

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hndssg_clr.                                       */
/*                                                                  */
/* Function	- Clear a handler for a given SSG subcode.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hndssg_clr(short code, hndext_proc_t handler)
{
	int i_hnd;

	if (code > 0) {
		for (i_hnd = 0; 
		     ((i_hnd < NUM_SSG) && (ssgproc[i_hnd].code != code)); 
		     i_hnd++);
		if (i_hnd < NUM_SSG) {
			ssgproc[i_hnd].code    = 0;
			ssgproc[i_hnd].handler = 0;
			ssgproc[i_hnd].uword   = 0;
		} else
			return -ENOENT;
	} else
		return -EINVAL;

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ssg_slih.                                         */
/*                                                                  */
/* Function	- Locate the handler for the given service signal   */
/*		  subcode (e.g. PFAULT or DIAG 250) and pass it     */
/*		  the context, code, interrupt parameter and user   */
/*		  word.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ssg_slih(mcontext_t ctx, short code, long intparm, void *uword)
{
	int i_hnd;

	for (i_hnd = 0; i_hnd < NUM_SSG; i_hnd++) {
		if (ssgproc[i_hnd].code == code) {
			ssgproc[i_hnd].handler(ctx, code, intparm, ssgproc[i_hnd].uword);
			break;
		}
	}
}

/*========================= End of Function ========================*/
