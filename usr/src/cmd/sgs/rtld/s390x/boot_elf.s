/*------------------------------------------------------------------*/
/* 								    */
/* Name        - boot_elf.s 					    */
/* 								    */
/* Function    - Set up ld for execution of elf object.             */
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
/*	Copyright (c) 1988 AT&T					    */
/*	  All Rights Reserved					    */
/* 								    */
/* 								    */
/*	Copyright 2004 Sun Microsystems, Inc.  All rights reserved. */
/*	Use is subject to license terms.			    */
/* 								    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include	"machdep.h"
#include	"_audit.h"
#if	defined(lint)
# include	<sys/types.h>
# include	"_rtld.h"
#else
# include	<sys/stack.h>
# include	<sys/asm_linkage.h>

	.file	"boot_elf.s"
	.section ".text"

#endif

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

#if	defined(lint)
extern unsigned long	elf_bndr(Rt_map *, unsigned long, caddr_t);
#endif

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * We got here because the initial call to a function resolved to a procedure
 * linkage table entry.  That entry did a branch to the first PLT entry, which
 * in turn did a call to elf_rtbndr (refer elf_plt_init()).
 *
 */

#if	defined(lint)

/*
 * We're called here from .PLTn in a new frame, with %r2 containing
 * the result of a larl (. - .PLT0), and %r3 containing the pc of
 * the brasl instruction we're got here with inside .PLT1
 */
void
elf_rtbndr(Rt_map *lmp, unsigned long pltoff, caddr_t from)
{
	(void) elf_bndr(lmp, pltoff, from);
}

#else
	.weak	_elf_rtbndr		// keep dbx happy as it likes to
	_elf_rtbndr = elf_rtbndr	// rummage around for our symbols

	ENTRY(elf_rtbndr)
	lg	%r0,48(%r15)
	lg	%r1,56(%r15)
	stmg	%r2,%r14,16(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)	// Next frame
	stg	%r14,0(%r15)		// Save backchain
	lgr	%r2,%r0			// Rt_Map
	lgr	%r3,%r1			// PLT Offset
	lgr	%r4,%r14		// From address
	brasl	%r14,elf_bndr		// Load and resolve
	lgr	%r1,%r2			// Copy resolved address
	aghi	%r15,SA(MINFRAME)
	lmg	%r2,%r14,16(%r15)	// Restore registers
	br	%r1			// Go to actual routine
	SET_SIZE(elf_rtbndr)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_plt_trace.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if	defined(lint)

ulong_t
elf_plt_trace()
{
	return (0);
}
#else
	.global	elf_plt_trace
	.type   elf_plt_trace, @function

/*
 * The dyn_plt that called us has already created a stack-frame for
 * us and placed the following entries in it:
 *
 *	[%fp + STACK_BIAS + -0x8]	* dyndata
 *	[%fp + STACK_BIAS + -0x10]	* prev stack size
 *
 * dyndata currently contains:
 *
 *	dyndata:
 *	0x0	Addr		*reflmp
 *	0x8	Addr		*deflmp
 *	0x10	Word		symndx
 *	0x14	Word		sb_flags
 *	0x18	Sym		symdef.st_name
 *	0x1c			symdef.st_info
 *	0x1d			symdef.st_other
 *	0x1e			symdef.st_shndx
 *	0x20			symdef.st_value
 *	0x28			symdef.st_size
 */
#define	REFLMP_OFF		0x0	
#define	DEFLMP_OFF		0x8	
#define	SYMNDX_OFF		0x10
#define	SBFLAGS_OFF		0x14
#define	SYMDEF_OFF		0x18
#define	SYMDEF_VALUE_OFF	0x20

elf_plt_trace:
	stmg	%r2,%r14,16(%r15)
	lgr	%r11,%r15			// Save sp
	aghi	%r15,-SA(MINFRAME)
	larl	%r12,_GLOBAL_OFFSET_TABLE_
	lg	%r8,-(2*CLONGSIZE)(%r11)	// r8 = *dyndata
	lg	%r3,SBFLAGS_OFF(%r2)
	tmll	%r3,LA_SYMB_NOPLTENTER
	jz	.start_pltenter

	lg	%r9,SYMDEF_VALUE_OFF(%r2)
	j	.end_pltenter

.start_pltenter:
	lg	%r2,REFLMP_OFF(%r8)			// r2 = reflmp
	lg	%r3,DEFLMP_OFF(%r8)			// r3 = deflmp
	la	%r4,SYMDEF_OFF(%r8)			// r4 = symp
	lgb	%r5,SYMNDX_OFF(%r8)			// r5 = symndx
	brasl	%r14,audit_pltenter
	la	%r5,SBFLAGS_OFF(%r8)			// r5 = * sb_flags
	lgr	%r9,%r2					// Save calling address

.end_pltenter:
	/*
	 * If *no* la_pltexit() routines exist we do not need
	 * to keep the stack frame before we call the actual
	 * routine.  Instead we jump to it and remove ourself
	 * from the stack at the same time.
	 */
	lg	%r1,audit_flags(%r12)			// Get Audit Flags
	lb	%r1,0(%r1)
	lghi	%r0,AF_PLTEXIT
	ngr	%r0,%r1					// Do we need to record exit
	jz	.bypass_pltexit				// No... Bypass

	tm	SBFLAGS_OFF(%r8),LA_SYMB_NOPLTEXIT	// Need to do audit?
	jz	.bypass_pltexit				// No... Bypass
	j	.start_pltexit

.bypass_pltexit:
	basr	%r14,%r9				// Go audit
	aghi	%r15,SA(MINFRAME)			// Restore
	lmg	%r6,%r14,56(%r15)
	br	%r14

.start_pltexit:
	/*
	 * In order to call la_pltexit() we must duplicate the
	 * arguments from the 'callers' stack on our stack frame.
	 *
	 * First we check the size of the callers stack and grow
	 * our stack to hold any of the arguments that need
	 * duplicating (these are arguments 5->N), because the
	 * first 5 (0->4) are passed via registers on s390.
	 */

	/*
	 * The first calculation is to determine how large the
	 * argument passing area might be.  Since there is no
	 * way to distinquish between 'argument passing' and
	 * 'local storage' from the previous stack this amount must
	 * cover both.
	 */
	lg	%r1,-(2*CLONGSIZE)(%r11)	// Caller's stack size
	aghi	%r1,-MINFRAME

	/*
	 * Next we compare the prev. stack size against the audit_argcnt.  We
	 * copy at most 'audit_argcnt' arguments.  The default arg count is 64.
	 *
	 * NOTE: on s390 we always copy at least five args since these
	 *	 are in registers and not on the stack.
	 *
	 * NOTE: Also note that we multiply (shift really) the arg count
	 *	 by 4 which is the 'word size' to calculate the amount
	 *	 of stack space needed.
	 */
	lg	%r7,audit_argcnt(%r12)
	lg	%r7,0(%r7)			// r7 = audit_argcnt
	cghi	%r7,5				// Fit in registers?
	jle	.grow_stack			// Yes... Skip

.check_growth:
	aghi	%r7,-5				// No. parms not in registers
	slag	%r7,%r7,CLONGSHIFT		// Get space requirement
	cgr	%r1,%r7				// Enough space already?
	jle	.grow_stack			// Yep... Skip

	lgr	%r1,%r7				// Set stack requirement
.grow_stack:
	/*
	 * When duplicating the stack we skip the first SA(MINFRAME)
	 * bytes. This is the space on the stack reserved for preserving
	 * the register windows and such and do not need to be duplicated
	 * on this new stack frame.  We start duplicating at the portion
	 * of the stack reserved for argument's above 6.
	 */
	sgr	%r15,%r1			// Grow stack by required amount
	lg	%r10,-CLONGSIZE(%r11)		// r10 = index into stack
	agr	%r10,%r11			// Point at arg pos

	larl	%r1,.movearg			// Point at moev instruction
	aghi	%r7,1				// Get move count
	ex	%r7,0(%r1)

	lmg	%r2,%r6,16(%r11)		// Restore parameter registers
	basr	%r14,%r9			// Call original routine
	lgr	%r10,%r2			// Save 1st half of return value
	lgr	%r13,%r3			// Save 2nd half of return value
	lg	%r8,-(2*CLONGSIZE)(%r11)	// r8 = *dyndata
	lg	%r3,REFLMP_OFF(%r8)		// r3 = reflmp
	lg	%r4,DEFLMP_OFF(%r8)		// r4 = deflmp
	la	%r5,SYMDEF_OFF(%r8)		// r5 = symp
	lgb	%r6,SYMNDX_OFF(%r8)		// r6 = symndx
	brasl	%r14,audit_pltexit		// Go audit
	lgr	%r2,%r10			// Restore return vals
	lgr	%r3,%r13			
	lgr	%r15,%r11			// Restore sp
	lmg	%r6,%r14,48(%r15)
	br	%r14

.movearg:
	mvc	0(1,%r15),0(%r10)

	.size	elf_plt_trace, . - elf_plt_trace
#endif

/*========================= End of Function ========================*/
