/*------------------------------------------------------------------*/
/* 								    */
/* Name        - syscall_trap.c					    */
/* 								    */
/* Function    - Field the syscall request and send it to the right */
/* 		 request servicer.				    */
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

#if !defined(lint) && !defined(__lint)
# include "assym.h"
#endif

#include <sys/asm_linkage.h>
#include <sys/machpcb.h>
#include <sys/machthread.h>
#include <sys/syscall.h>
#include <sys/trap.h>
#include <sys/pcb.h>
#include <sys/machparam.h>

#ifdef TRAPTRACE
#include <sys/traptrace.h>
#endif /* TRAPTRACE */

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
/* Name		- syscall_trap.                                     */
/*                                                                  */
/* Function	- Handle syscall requests from 64-bit clients.      */
/*		                               		 	    */
/* Entry	- %r2 = Register save area from caller		    */
/*		                               		 	    */
/* Usage	- %r7 = lwp 					    */
/*		  %r9 = cpu					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)

/*ARGSUSED*/
void
syscall_trap(struct regs *rp)	/* for tags only; not called from C */
{}

#else /* lint */

#if (1 << SYSENT_SHIFT) != SYSENT_SIZE
# error	"SYSENT_SHIFT does not correspond to size of sysent structure"
#endif
	
	ENTRY_NP(syscall_trap)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME+32)		// Room for 4 stack parameters
	stg	%r14,0(%r15)
	lgr	%r12,%r2			// Save reg pointer

	GET_THR(7)

	lg	%r9,__LC_CPU			// Get CPU pointer
	llgh	%r8,T_SYSNUM(%r7)		// Get syscall code
	lg	%r11,CPU_STATS_SYS_SYSCALL(%r9)	// Get cpu_stats
	aghi	%r11,1				// Increment syscalls
	stg	%r11,CPU_STATS_SYS_SYSCALL(%r9) // Save
	lg	%r10,T_LWP(%r7)
	mvi	LWP_STATE(%r10),LWP_SYS		// Set new state for LWP
	mvi	LWP_UNUSED(%r10),0		// Clear "setcontext" flag
	lg	%r11,LWP_RU_SYSC(%r10)
	aghi	%r11,1
	stg	%r11,LWP_RU_SYSC(%r10)

#ifdef TRAPTRACE
#
# FIXME S390x - Use the HW trace instructions to log this event
#
#endif
	llgc	%r11,T_PRE_SYS(%r7)
#ifdef SYSCALLTRACE
	larl	%r10,syscalltrace
	ogr	%r11,%r10
#else
	ltgr	%r11,%r11
#endif
	jz	0f
	
	bras	%r14,__syscall_pre

0:
	lghi	%r2,LMS_USER
	lghi	%r3,LMS_SYSTEM
	brasl	%r14,syscall_mstate
	llgh	%r8,T_SYSNUM(%r7)		// Get syscall code
	cghi	%r8,NSYSCALL			// In range?
	jle	1f				// Yes... Go process

	brasl	%r14,nosys			// Illegal syscall
	j	__syscall_post			// Go post process

1:
	larl	%r10,sysent			// Address syscall table
	sllg	%r8,%r8,SYSENT_SHIFT		// Form index
	agr	%r10,%r8			// Point at sysent entry
	lg	%r1,SY_CALLC(%r10)		// Get A(Handler)
	lghi	%r0,1				// Get secondary space id
	sar	%a2,%r0				// Set access register
	lg	%r2,KSTK_R15(%r12)		// Get caller's SP
	sacf	AC_ACCESS			// Get into access register mode
	lg	%r3,SA(MINFRAME)(%r2)		// Get parameter 6
	stg	%r3,SA(MINFRAME)(%r15)		// Set in this stack
	lg	%r3,SA(MINFRAME+8)(%r2)		// Get parameter 7
	stg	%r3,SA(MINFRAME+8)(%r15)	// Set in this stack
	lg	%r3,SA(MINFRAME+16)(%r2)	// Get parameter 8
	stg	%r3,SA(MINFRAME+16)(%r15)	// Set in this stack
	lg	%r3,SA(MINFRAME+24)(%r2)	// Get parameter 9
	stg	%r3,SA(MINFRAME+24)(%r15)	// Set in this stack
	sacf	AC_PRIMARY			// Return to primary space mode
	lgr	%r8,%r12			// Get A(Saved Registers)
	lmg	%r2,%r6,32(%r8)			// Restore register based parameters
	basr	%r14,%r1			// Go handle the call
#ifdef TRAPTRACE
#
# FIXME S390x - Use the HW trace instructions to log this event
#
#endif
	lg	%r11,T_LWP(%r7)
	cli	LWP_UNUSED(%r11),0		// Check "setcontext" flag
	jnz	3f				// Jump if set

	//
	// If handler returns two ints, then we need to split the 64-bit
	// return value in %r2 into %r2 and %r3
	//
	llgh	%r0,SY_FLAGS(%r10)		// Get flag
	tmll	%r0,SE_32RVAL2			// Is it expecting 2 x 32-bit?
	jz	2f				// No... Skip

	lgr	%r3,%r2				// Copy
	sllg	%r3,%r3,32			// Bottom 32 bits
	srlg	%r3,%r3,32			// ... in R3
	srlg	%r2,%r2,32			// Top 32 bits in R2
	stg	%r3,KSTK_R3(%r12)
	
2:
	stg	%r2,KSTK_R2(%r12)

	lghi	%r0,0
	stg	%r0,KSTK_R0(%r12)		// Set as "no error"

3:
	lgf	%r10,T_POST_SYS_AST(%r7)	// Get post process flag
#ifdef SYSCALLTRACE
	larl	%r11,syscalltrace
	ogr	%r11,%r10
#else
	ltgr	%r10,%r10
#endif	
	jnz	__syscall_post

	lghi	%r2,LMS_SYSTEM
	lghi	%r3,LMS_USER
	brasl	%r14,syscall_mstate

	aghi	%r15,SA(MINFRAME+32)
	lmg	%r6,%r14,48(%r15)
	br	%r14				// Return

__syscall_pre:
	lgr	%r9,%r14			// Save link
	brasl	%r14,pre_syscall		// Do pre-call
	ltgr	%r2,%r2				// Did it abort
	jnz	__syscall_post			// Yes... Do post call 
	br	%r9				// Return to main processing

__syscall_post:
	brasl	%r14,post_syscall		// Post call

	lghi	%r2,LMS_SYSTEM			// Restore state info
	lghi	%r3,LMS_USER
	brasl	%r14,syscall_mstate

	aghi	%r15,SA(MINFRAME+32)
	lmg	%r6,%r14,48(%r15)
	br	%r14				// Return
	SET_SIZE(syscall_trap)
#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- syscall_trap32.                                   */
/*                                                                  */
/* Function	- Handle syscall requests from 32-bit clients.      */
/*		                               		 	    */
/* Entry	- %r2 = Register save area from caller		    */
/*		                               		 	    */
/* Usage	- %r7 = lwp 					    */
/*		  %r9 = cpu					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)

/*ARGSUSED*/
void
syscall_trap(struct regs *rp)	/* for tags only; not called from C */
{}

#else

	ENTRY_NP(syscall_trap32)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME+32)		// Room for 4 stack parameters
	stg	%r14,0(%r15)
	lgr	%r12,%r2			// Save reg pointer

	GET_THR(7)

	lg	%r9,__LC_CPU			// Get CPU pointer
	llgh	%r8,T_SYSNUM(%r7)		// Get syscall code
	lg	%r11,CPU_STATS_SYS_SYSCALL(%r9)	// Get cpu_stats
	aghi	%r11,1				// Increment syscalls
	stg	%r11,CPU_STATS_SYS_SYSCALL(%r9) // Save
	lg	%r10,T_LWP(%r7)
	mvi	LWP_STATE(%r10),LWP_SYS		// Set new state for LWP
	mvi	LWP_UNUSED(%r10),0		// Clear "setcontext" flag
	lg	%r11,LWP_RU_SYSC(%r10)
	aghi	%r11,1
	stg	%r11,LWP_RU_SYSC(%r10)

#ifdef TRAPTRACE
#
# FIXME S390x - Use the HW trace instructions to log this event
#
#endif
	llgc	%r11,T_PRE_SYS(%r7)
#ifdef SYSCALLTRACE
	larl	%r10,syscalltrace
	ogr	%r11,%r10
#else
	ltgr	%r11,%r11
#endif
	jz	0f
	
	bras	%r14,__syscall_pre32

0:
	lghi	%r2,LMS_USER
	lghi	%r3,LMS_SYSTEM
	brasl	%r14,syscall_mstate
	llgh	%r8,T_SYSNUM(%r7)		// Get syscall code
	cghi	%r8,NSYSCALL			// In range?
	jle	1f				// Yes... Go process

	brasl	%r14,nosys			// Illegal syscall
	j	__syscall_post32		// Go post process

1:
	larl	%r10,sysent32			// Address syscall table
	sllg	%r8,%r8,SYSENT_SHIFT		// Form index
	agr	%r10,%r8			// Point at sysent32 entry
	lg	%r1,SY_CALLC(%r10)		// Get A(Handler)
	lghi	%r0,1				// Get secondary space id
	sar	%a2,%r0				// Set access register
	lg	%r2,KSTK_R15(%r12)		// Get caller's SP
	sacf	AC_ACCESS			// Get into access register mode
	lgf	%r3,SA(MINFRAME32)(%r2)		// Get parameter 6
	stg	%r3,SA(MINFRAME)(%r15)		// Set in this stack
	lgf	%r3,SA(MINFRAME32+4)(%r2)	// Get parameter 7
	stg	%r3,SA(MINFRAME+8)(%r15)	// Set in this stack
	lgf	%r3,SA(MINFRAME32+8)(%r2)	// Get parameter 8
	stg	%r3,SA(MINFRAME+16)(%r15)	// Set in this stack
	lgf	%r3,SA(MINFRAME32+12)(%r2)	// Get parameter 9
	stg	%r3,SA(MINFRAME+24)(%r15)	// Set in this stack
	sacf	AC_PRIMARY			// Return to primary space mode
	lgr	%r8,%r12			// Get A(Saved Registers)
	lmg	%r2,%r6,32(%r8)			// Restore register based parameters
	basr	%r14,%r1			// Go handle the call
#ifdef TRAPTRACE
#
# FIXME S390x - Use the HW trace instructions to log this event
#
#endif
	lg	%r11,T_LWP(%r7)
	cli	LWP_UNUSED(%r11),0		// Check "setcontext" flag
	jnz	3f				// Jump if set

	//
	// If handler returns two ints, then we need to split the 64-bit
	// return value in %r2 into %r2 and %r3
	//
	llgh	%r0,SY_FLAGS(%r10)		// Get flag
	tmll	%r0,SE_32RVAL2 | SE_64RVAL	// Is it expecting 2 x 32-bit?
	jz	2f				// No... Skip

	lgr	%r3,%r2				// Copy
	sllg	%r3,%r3,32			// Bottom 32 bits
	srlg	%r3,%r3,32			// ... in R3
	srlg	%r2,%r2,32			// Top 32 bits in R2
	stg	%r3,KSTK_R3(%r12)
	
2:
	stg	%r2,KSTK_R2(%r12)

	lghi	%r0,0
	stg	%r0,KSTK_R0(%r12)		// Set as "no error"

3:
	lgf	%r10,T_POST_SYS_AST(%r7)	// Get post process flag
#ifdef SYSCALLTRACE
	larl	%r11,syscalltrace
	ogr	%r11,%r10
#else
	ltgr	%r10,%r10
#endif	
	jnz	__syscall_post32

	lghi	%r2,LMS_SYSTEM
	lghi	%r3,LMS_USER
	brasl	%r14,syscall_mstate

	aghi	%r15,SA(MINFRAME+32)
	lmg	%r6,%r14,48(%r15)
	br	%r14				// Return

__syscall_pre32:
	lgr	%r9,%r14			// Save link
	brasl	%r14,pre_syscall		// Do pre-call
	ltgr	%r2,%r2				// Did it abort
	jnz	__syscall_post32		// Yes... Do post call 
	br	%r9				// Return to main processing

__syscall_post32:
	brasl	%r14,post_syscall		// Post call

	lghi	%r2,LMS_SYSTEM			// Restore state info
	lghi	%r3,LMS_USER
	brasl	%r14,syscall_mstate

	aghi	%r15,SA(MINFRAME+32)
	lmg	%r6,%r14,48(%r15)
	br	%r14				// Return
	SET_SIZE(syscall_trap32)
#endif	/* lint */

/*========================= End of Function ========================*/
