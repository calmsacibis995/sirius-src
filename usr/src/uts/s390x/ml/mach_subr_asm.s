/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mach_subr_asm.					    */
/* 								    */
/* Function    - Various LPW/RTT subroutines.                       */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - June, 2007  					    */
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

#include "assym.h"
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
/* Name		- getprocessorid.                                   */
/*                                                                  */
/* Function	- Return CPU address.                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)
/* ARGSUSED */
int
getprocessorid(void)
{ return (0); }

#else	/* lint */

/*
 * Get the processor ID.
 */

	ENTRY(getprocessorid)
	stap	48(%r15)
	llgh	%r2,48(%r15)
	br	%r14
	SET_SIZE(getprocessorid)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bcmp.                                             */
/*                                                                  */
/* Function	- Compare two datastreams.                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)
int
bcmp(const void *s1_arg, const void *s2_arg, size_t len) {
return (0);
}

#else	/* lint */

	ENTRY(bcmp)
	ltgr	%r4,%r4			// Zero length?
	jz	1f			// Yes... Set as equal

	cghi	%r4,256			// Can we use simple compare
	jh	0f			// No... Must use clcle

	larl	%r1,.Lcmp		// Address the CLC instruction
	aghi	%r4,-1			// Adjust for execute
	ex	%r4,0(%r1)		// Perform the comparison
	je	1f			// Jump if equal
	jl	2f			// Jump if less than
4:	
	lghi	%r2,1			// Set as greater than
	br	%r14
1:
	lghi	%r2,0			// Set the equal indicator
	br	%r14
2:
	lghi	%r2,-1			// Set as less than
	br	%r14
0:
	lgr	%r0,%r3			// Copy comparhend
	lgr	%r1,%r4			// Copy length
	lgr	%r3,%r4	
	lghi	%r5,0			// Set pad character
3:
	clcle	%r0,%r2,0(%r5)		// Perform comparison
	jo	3b			// Keep going if more to do
	je	1b
	jl	2b
	j	4b

.Lcmp:	clc	0(1,%r2),0(%r3)

	SET_SIZE(bcmp)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_rtt_initial/lwp_rtt.                          */
/*                                                                  */
/* Function	- Return from _sys_trap routine.                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

void
lwp_rtt_initial(void)
{}

void
lwp_rtt(void)
{}

#else	/* __lint */

/*
 * lwp_rtt - start execution in newly created LWP.
 *	Here with t_post_sys set by lwp_create, and lwp_eosys == JUSTRETURN,
 *	so that post_syscall() will run and the registers will
 *	simply be restored.
 *	This must go out through sys_rtt instead of syscall_rtt.
 */

	ENTRY_NP(lwp_rtt_initial)

	stnsm	__LC_RUN_PSW,0x04

	LOAD_THR(1)

	lg	%r15,T_STACK(%r1)
	stmg	%r2,%r14,16(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)
	lgr	%r6,%r1
	brasl	%r14,__dtrace_probe___proc_start
	j	_lwp_rtt

	ENTRY_NP(lwp_rtt)

	/*
	 * r13	lwp
	 * r12	lwp->lwp_procp
	 * r6	curthread
	 */

	stnsm	__LC_RUN_PSW,0x04

	LOAD_THR(1)

	lg	%r15,T_STACK(%r1)
	stmg	%r2,%r14,16(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)
	lgr	%r6,%r1

_lwp_rtt:
	brasl	%r14,__dtrace_probe___proc_lwp__start
	lg	%r13,T_LWP(%r6)

	brasl	%r14,dtrace_systrace_rtt
	lg	%r12,LWP_PROCP(%r13)
	lg	%r2,MINFRAME+16(%r15)	// rval1
	lg	%r3,MINFRAME+24(%r15)	// rval2
	brasl	%r14,post_syscall

	lg	%r1,LWP_REGS(%r13)
	mvc	__LC_RUN_PSW(16,0),KSTK_PSW(%r1)
	lg	%r0,KSTK_R1(%r1)
	stg	%r0,__LC_SYN_SAVE_AREA
	lg	%r0,KSTK_R0(%r1)
	lmg	%r2,%r15,KSTK_R2(%r1)
	lam	%a0,%a15,KSTK_AREGS(%r1)
	lg	%r1,__LC_SYN_SAVE_AREA
	lpswe	__LC_RUN_PSW

	SET_SIZE(lwp_rtt)
	SET_SIZE(lwp_rtt_initial)

#endif 


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _sys_rtt.                                         */
/*                                                                  */
/* Function	- Resume interrupt thread execution.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

void
_sys_rtt(void)
{}

#else	/* __lint */

	ENTRY(_sys_rtt)
	stnsm	__LC_SCRATCH,0x04	/* Ensure interrupts are disabled */
	lgr	%r6,%r15		/* Save Kstack pointer */
	aghi	%r15,-SA(MINFRAME)	/* Give us some working room */
	lgr	%r2,%r6			/* pass rp to sys_rtt_common */
	brasl	%r14,sys_rtt_common
	lgr	%r15,%r6
	mvc	__LC_RUN_PSW(16,0),KSTK_PSW(%r15)
	lam	%a0,%a15,KSTK_AREGS(%r15)
	lmg	%r0,%r15,KSTK_REGS(%r15)
	stpt	__LC_TIMER_START
	lpswe	__LC_RUN_PSW

#endif

/*========================= End of Function ========================*/
