/*------------------------------------------------------------------*/
/* 								    */
/* Name        - asm_subr.s 					    */
/* 								    */
/* Function    -                                                    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - August, 2007					    */
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

#define	GREGSIZE	8

#define JMPG6		0
#define JMPG7		8
#define JMPG8		16
#define JMPG9		24
#define JMPG10		32
#define JMPG11		40
#define JMPG12		48
#define JMPG13		56
#define JMPG14		64
#define JMPG15		72
#define JMPF1		80
#define JMPF3		88
#define JMPF5		96
#define JMPF7		104
#define JMPF4		112
#define JMPF6		120
#define JMPMSVF		128
#define JMPMASK		132

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

	.file	"asm_subr.s"

#include <sys/asm_linkage.h>
#include <sys/trap.h>
#include <../assym.h>
#include "SYS.h"

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
/* Name		- _lwp_start.                                       */
/*                                                                  */
/* Function	- This is where execution resumes when a thread     */
/*		  created with thr_create() or pthread_create()     */
/*		  returns (see setup_context()). We pass the 	    */
/*		  (void *) return value to _thrp_terminate().	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(_lwp_start)
	jg	_thrp_terminate	// %r2 contains the return value
	SET_SIZE(_lwp_start)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _lwp_terminate.                                   */
/*                                                                  */
/* Function	- Complete thread termination.                      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(_lwp_terminate)
	lghi	%r0,SYS_lwp_exit	
	svc	0
	br	%r14		// We'd better not return - bad stuff!
	SET_SIZE(_lwp_terminate)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- set_curthread.                                    */
/*                                                                  */
/* Function	- Set the current thread pointer in ar0/1.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(set_curthread)
	stg	%r2,48(%r15)
	lam	%a0,%a1,48(%r15)
	br	%r14
	SET_SIZE(set_curthread)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- __lwp_park.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(__lwp_park)
	lgr	%r4,%r3
	lgr	%r3,%r2
	lghi	%r2,0
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_park)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- __lwp_unpark.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(__lwp_unpark)
	lgr	%r3,%r2
	lghi	%r2,1
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_unpark)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- __lwp_unpark_all.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(__lwp_unpark_all)
	lgr	%r4,%r3
	lgr	%r3,%r2
	lghi	%r2,2
	SYSTRAP_RVAL1(lwp_park)
	SYSLWPERR
	RET
	SET_SIZE(__lwp_unpark_all)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_yield.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

	ENTRY(lwp_yield)
	SYSTRAP_RVAL1(yield)
	RET
	SET_SIZE(lwp_yield)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- __sighndlr.                                       */
/*                                                                  */
/* Function	- This is called from sigacthandler() for the       */
/*		  entire purpise of communicating the ucontext to   */
/*		  java's stack tracing functions.	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * __sighndlr(int sig, siginfo_t *si, ucontex_t *uc, void (*hndlr)())
 *
 */
	ENTRY(__sighndlr)
	.globl	__sighndlrend
	stg	%r14,112(%r15)
	aghi	%r15,-SA(MINFRAME)
	basr	%r14,%r5
	aghi	%r15,SA(MINFRAME)
	lg	%r14,112(%r15)
	br	%r14
__sighndlrend:
	SET_SIZE(__sighndlr)

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _sigsetjmp.                                       */
/*                                                                  */
/* Function	- This version is faster than the old non-threaded  */
/*		  version because we don't normally have to call    */
/*		  __getcontext() to get the singal mask. (We have a */
/*		  copy of it in the ulwp_t structure.)		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * int _sigsetjmp(sigjmp_buf env, int savemask)
 *
 */

#undef	sigsetjmp

	ENTRY2(sigsetjmp,_sigsetjmp)
	stmg	%r6,%r15,JMPG6(%r2)
	std	%f1,JMPF1(%r2)
	std	%f3,JMPF3(%r2)
	std	%f5,JMPF5(%r2)
	std	%f7,JMPF7(%r2)
	std	%f4,JMPF4(%r2)
	std	%f6,JMPF6(%r2)
	jg	__csigsetjmp
	SET_SIZE(sigsetjmp)
	SET_SIZE(_sigsetjmp)

/*========================= End of Function ========================*/
