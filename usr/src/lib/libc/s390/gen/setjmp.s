/*------------------------------------------------------------------*/
/* 								    */
/* Name        - setjmp.c   					    */
/* 								    */
/* Function    - Support routines for setjmp.                       */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - November, 2007 				    */
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

#define JMPG6		0
#define JMPG7		4
#define JMPG8		8
#define JMPG9		12
#define JMPG10		16
#define JMPG11		20
#define JMPG12		24
#define JMPG13		28
#define JMPG14		32
#define JMPG15		36
#define JMPF4		40
#define JMPF6		48
#define JMPFPC 		56

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

	.ident	"@(#)setjmp.s	1.00	07/11/30 NAF"

	.file	"setjmp.s"

#include <sys/asm_linkage.h>
#include <sys/trap.h>

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

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setjmp.                                           */
/*                                                                  */
/* Function	- Save context for a subsequent longjmp.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(setjmp)

	stm	%r6,%r15,JMPG6(%r2)
	std	%f4,JMPF4(%r2)
	std	%f6,JMPF6(%r2)
	stfpc	JMPFPC(%r2)
	lhi	%r2,0
	br	%r14

	SET_SIZE(setjmp)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- longjmp.                                          */
/*                                                                  */
/* Function	- buf_ptr points to a jmpbuf which has been 	    */
/*		  initialized by setjmp.			    */
/*		  val is the value we wish to return to setjmp's    */
/*		  caller					    */
/*                                                                  */
/* 		  Registers are restored from the jmpbuf structure  */
/*		  and control is returned to the point specified    */
/*		  in the jmpbuf.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(longjmp)

	lm	%r6,%r15,JMPG6(%r2)
	ld	%f4,JMPF4(%r2)
	ld	%f6,JMPF6(%r2)
	stfpc	JMPFPC(%r2)
	ltr	%r2,%r3
	jnz	0f

	lhi	%r2,1
0:
	br	%r14

	SET_SIZE(longjmp)

	ANSI_PRAGMA_WEAK2(_setjmp,setjmp,function)
	ANSI_PRAGMA_WEAK2(_longjmp,longjmp,function)

/*========================= End of Function ========================*/
