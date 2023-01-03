/*------------------------------------------------------------------*/
/* 								    */
/* Name        - thunk.s    					    */
/* 								    */
/* Function    - Provide parameter translation for various 32-bit   */
/* 		 system calls.					    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - June, 2008      				    */
/* 								    */
/* Notes       - We don't need to worry about parameters that are   */
/*               on the stack as syscall_trap32() has taken care    */
/*               of the 32->64-bit ABI conversion. Only parameters  */
/*               that are signed integers need to be considered.    */
/*               64-bit values that are in two registers for 32-bit */
/*               syscalls are taken care of in the syscall handler  */
/*               which defines the parameter as two 32-bit parms.   */
/*                                                                  */
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

#include <sys/asm_linkage.h>

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
/* Name		- _lseek32.                                         */
/*                                                                  */
/* Function	- Translate the offset parameter to 64-bit.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(_lseek32)
	lgfr	%r3,%r3
	jg	lseek32
	SET_SIZE(_lseek32)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _pread32.                                         */
/*                                                                  */
/* Function	- Translate the offset parameter to 64-bit.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(_pread32)
	lgfr	%r5,%r5
	jg	pread32
	SET_SIZE(_pread32)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _pwrite32.                                        */
/*                                                                  */
/* Function	- Translate the offset parameter to 64-bit.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(_pwrite32)
	lgfr	%r5,%r5
	jg	pread32
	SET_SIZE(_pwrite32)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _cpcmd32.                                         */
/*                                                                  */
/* Function	- Translate the size parameter to 64-bit.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(_cpcmd32)
	lgfr	%r4,%r4
	jg	cpcmd
	SET_SIZE(_cpcmd32)

/*========================= End of Function ========================*/
