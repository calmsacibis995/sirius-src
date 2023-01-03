/*------------------------------------------------------------------*/
/* 								    */
/* Name        - machdep.c  					    */
/* 								    */
/* Function    - Various machine dependent routines for libc thread */
/* 		 support.					    */
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

#include "lint.h"
#include "thr_uberdata.h"
#include <procfs.h>
#include <setjmp.h>
#include <sys/fsr.h>
#include "sigjmp_struct.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern int getlwpstatus(thread_t, lwpstatus_t *);
extern int putlwpregs(thread_t, prgregset_t);

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
/* Name		- setup_context.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
setup_context(ucontext_t *ucp, void *(*func)(ulwp_t *),
	ulwp_t *ulwp, caddr_t stk, size_t stksize)
{
	/*
	 * Top-of-stack must be rounded down to STACK_ALIGN and
	 * there must be a minimum frame for the register window.
	 */
	uintptr_t stack = (((uintptr_t)stk + stksize) & ~(STACK_ALIGN - 1)) -
	    SA(MINFRAME32);

	/* clear the context and the top stack frame */
	(void) _memset(ucp, 0, sizeof (*ucp));
	(void) _memset((void *)stack, 0, SA(MINFRAME32));

	/* fill in registers of interest */
	ucp->uc_flags |= UC_CPU;
	ucp->uc_mcontext.gregs[REG_G13] = (greg_t) func;
	ucp->uc_mcontext.gregs[REG_G2]  = (greg_t) ulwp;
	ucp->uc_mcontext.gregs[REG_SP]  = (greg_t) (stack - STACK_BIAS);
	ucp->uc_mcontext.gregs[REG_G14] = (greg_t) _lwp_start;
	ucp->uc_mcontext.aregs[REG_A0]  = (uint32_t) ulwp;

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _thr_setup.                                       */
/*                                                                  */
/* Function	- Machine-dependent startup code for a newly created*/
/*		  thread.                      		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
_thrp_setup(ulwp_t *self)
{
	extern void _setfsr(greg_t *);

	self->ul_ustack.ss_sp = (void *)(self->ul_stktop - self->ul_stksiz);
	self->ul_ustack.ss_size = self->ul_stksiz;
	self->ul_ustack.ss_flags = 0;
	(void) setustack(&self->ul_ustack);
	tls_setup();

	/* signals have been deferred until now */
	sigon(self);

	return (self->ul_startpc(self->ul_startarg));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _fpinherit.                                       */
/*                                                                  */
/* Function	- No op.                                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
_fpinherit(ulwp_t *ulwp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getgregs.                                         */
/*                                                                  */
/* Function	- Get general registers.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
getgregs(ulwp_t *ulwp, gregset_t rs)
{
	lwpstatus_t status;

	if (getlwpstatus(ulwp->ul_lwpid, &status) == 0) {
		rs[REG_G0]  = status.pr_reg[REG_G0];
		rs[REG_G1]  = status.pr_reg[REG_G1];
		rs[REG_G2]  = status.pr_reg[REG_G2];
		rs[REG_G3]  = status.pr_reg[REG_G3];
		rs[REG_G4]  = status.pr_reg[REG_G4];
		rs[REG_G5]  = status.pr_reg[REG_G5];
		rs[REG_G6]  = status.pr_reg[REG_G6];
		rs[REG_G7]  = status.pr_reg[REG_G7];
		rs[REG_G8]  = status.pr_reg[REG_G8];
		rs[REG_G9]  = status.pr_reg[REG_G9];
		rs[REG_G10] = status.pr_reg[REG_G10];
		rs[REG_G11] = status.pr_reg[REG_G11];
		rs[REG_G12] = status.pr_reg[REG_G12];
		rs[REG_G13] = status.pr_reg[REG_G13];
		rs[REG_G14] = status.pr_reg[REG_G14];
		rs[REG_G15] = status.pr_reg[REG_G15];
	} else {
		rs[REG_G0]  = 0;
		rs[REG_G1]  = 0;
		rs[REG_G2]  = 0;
		rs[REG_G3]  = 0;
		rs[REG_G4]  = 0;
		rs[REG_G5]  = 0;
		rs[REG_G6]  = 0;
		rs[REG_G7]  = 0;
		rs[REG_G8]  = 0;
		rs[REG_G9]  = 0;
		rs[REG_G10] = 0;
		rs[REG_G11] = 0;
		rs[REG_G12] = 0;
		rs[REG_G13] = 0;
		rs[REG_G14] = 0;
		rs[REG_G15] = 0;
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setgregs.                                         */
/*                                                                  */
/* Function	- Set the general registers.                        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
setgregs(ulwp_t *ulwp, gregset_t rs)
{
	lwpstatus_t status;

	if (getlwpstatus(ulwp->ul_lwpid, &status) == 0) {
		status.pr_reg[REG_G0]  = rs[REG_G0];
		status.pr_reg[REG_G1]  = rs[REG_G1];
		status.pr_reg[REG_G2]  = rs[REG_G2];
		status.pr_reg[REG_G3]  = rs[REG_G3];
		status.pr_reg[REG_G4]  = rs[REG_G4];
		status.pr_reg[REG_G5]  = rs[REG_G5];
		status.pr_reg[REG_G6]  = rs[REG_G6];
		status.pr_reg[REG_G7]  = rs[REG_G7];
		status.pr_reg[REG_G8]  = rs[REG_G8];
		status.pr_reg[REG_G9]  = rs[REG_G9];
		status.pr_reg[REG_G10] = rs[REG_G10];
		status.pr_reg[REG_G11] = rs[REG_G11];
		status.pr_reg[REG_G12] = rs[REG_G12];
		status.pr_reg[REG_G13] = rs[REG_G13];
		status.pr_reg[REG_G14] = rs[REG_G14];
		status.pr_reg[REG_G15] = rs[REG_G15];
		(void) putlwpregs(ulwp->ul_lwpid, status.pr_reg);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- __csigsetjmp.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
__csigsetjmp(sigjmp_buf env, int savemask)
{
	sigjmp_struct_t *bp = (sigjmp_struct_t *)env;
	ulwp_t *self = curthread;

	/*
	 * bp->sjs_sp and bp->sjs_pc are already set.
	 */
	bp->sjs_flags  = JB_FRAMEPTR;
	bp->sjs_uclink = self->ul_siglink;
	if (self->ul_ustack.ss_flags & SS_ONSTACK)
		bp->sjs_stack = self->ul_ustack;
	else {
		bp->sjs_stack.ss_sp =
			(void *)(self->ul_stktop - self->ul_stksiz);
		bp->sjs_stack.ss_size = self->ul_stksiz;
		bp->sjs_stack.ss_flags = 0;
	}

	if (savemask) {
		bp->sjs_flags |= JB_SAVEMASK;
		enter_critical(self);
		bp->sjs_sigmask = self->ul_sigmask;
		exit_critical(self);
	}

	return (0);
}

/*========================= End of Function ========================*/
