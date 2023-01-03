/*------------------------------------------------------------------*/
/* 								    */
/* Name        - zero.s     					    */
/* 								    */
/* Function    - Zero out storage between data section and the next */
/* 		 page boundary.					    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - August, 2007  					    */
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
/* 								    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#pragma ident   "%Z%%M% %I%     %E% NAF"


/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- zero.                                             */
/*                                                                  */
/* Function	- Use MVCL to clear the storage.                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if	defined(lint)

#include	<sys/types.h>

void
zero(caddr_t addr, size_t len)
{
	while (len-- > 0)
		*addr++ = 0;
}

#else

#include	<sys/asm_linkage.h>

	.file	"zero.s"

	ENTRY(zero)
	ltr	%r3,%r3		// Count > 0?
	jnp	1f		// No... Get out of here

	lr	%r0,%r2		// Copy
	lhi	%r1,0		// Set zero src length
	lr 	%r4,%r1		// Set pad byte
0:
	mvcle	%r2,%r0,0(%r4)	// Clear it
	jo	0b
1:
	br	%r14		// Return
	SET_SIZE(zero)
#endif

/*========================= End of Function ========================*/
