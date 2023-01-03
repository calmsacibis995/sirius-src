/*------------------------------------------------------------------*/
/* 								    */
/* Name        - boot.s     					    */
/* 								    */
/* Function    - Bootstrapper for ld.so                             */
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
/*	Copyright (c) 1988 AT&T					    */
/*	  All Rights Reserved					    */
/* 								    */
/* 								    */
/*	Copyright (c) 1998 by Sun Microsystems, Inc.		    */
/*	All rights reserved.					    */
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

#define OFF_ARGC	0x00	// Displacement to argc

#define EBOFF_ARGVID	0x00	// Offset to EB_ARGV
#define EBOFF_ARGV	0x08	// Offset to **argv
#define EBOFF_ENVPID	0x10	// Offset to EB_ENVP
#define EBOFF_ENVP	0x18	// Offset to envp
#define EBOFF_AUXVID	0x20	// Offset to EB_AUXV
#define EBOFF_AUXV	0x28	// Offset to auxv
#define EBOFF_NULLID	0x30	// Offset to EB_NULL

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/asm_linkage.h>
#include <sys/param.h>
#include <link.h>

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

	.file	"boot.s"
	.section ".text"
	.global	_rt_boot
	.extern	_setup, atexit_fini
	.type	_rt_boot, @function
	.align	4

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _rt_boot.                                         */
/*                                                                  */
/* Function	- Bootstrap routine for ld.so.  Control arrives     */
/*		  here directly from exec() upon invocation of a    */
/*		  dynamically linked program specifying ld.so	    */
/* 		  as its interpreter. 				    */
/*		  On entry, the stack appears as:		    */
/*                                                                  */
/*			._______________________.  high addresses   */
/*			!	0 word		!		    */
/*			!_______________________!		    */
/*			!			!		    */
/*			!	Information	!		    */
/*			!	Block		!		    */
/*			!	(size varies)	!		    */
/*			!_______________________!		    */
/*			!	Auxiliary	!		    */
/*			!	vector		!		    */
/*			!	2 word entries	!		    */
/*			!			!		    */
/*			!_______________________!		    */
/*			!	0 word		!		    */
/*			!_______________________!		    */
/*			!	Environment	!		    */
/*			!	pointers	!		    */
/*			!	...		!		    */
/*			!	(one word each)	!		    */
/*			!_______________________!		    */
/*			!	0 word		!		    */
/*			!_______________________!		    */
/*			!	Argument	! low addresses	    */
/*			!	pointers	!		    */
/*			!	Argc words	!		    */
/*			!_______________________!		    */
/*			!			!		    */
/*			!	Argc		!		    */
/*			!_______________________! <- %sp	    */
/*                                                                  */
/*------------------------------------------------------------------*/

#if	defined(lint)

extern	unsigned long	_setup();
extern	void		atexit_fini();

void
main()
{
	(void) _setup();
	atexit_fini();
}

#else

// Entry vector
//	+0: normal start
//	+4: normal start
//	+8: alias start (frame exists)		XX64 what's this for?

_rt_boot:
	j	_elf_start
	j	_elf_start
	j	_alias_start

// Start up routines

_elf_start:

// Create a stack frame, perform PIC set up.  We have
// to determine a bunch of things from our "environment" and
// construct an Elf64_Boot attribute value vector.

	stmg	%r6,%r14,48(%r15)
	lgr	%r11,%r15
	lgr	%r2,%r15
	aghi	%r15,-SA(MINFRAME + (EB_MAX * 16))
	aghi	%r11,-SA(MINFRAME)
	aghi	%r2,SA(MINFRAME)

	//
	// Set null curthread pointer
	//
	lghi	%r6,0
	
	SET_THR(%r6)
	
_alias_start:

	larl	%r12,_GLOBAL_OFFSET_TABLE_

// %r11 points to the root of our ELF bootstrap vector, use it to construct
// the vector and send it to _setup.
// 
// %r2 points to the structure placed on the stack by the exec() processor
// 
// The resulting Elf64_Boot vector looks like this:
// 
//	Offset		Contents
//	+0x0		EB_ARGV
//	+0x8		argv[]
//	+0x10		EB_ENVP
// 	+0x18		envp[]
//	+0x20		EB_AUXV
//	+0x28		auxv[]
//	+0x30		EB_NULL


	/*-----------------------------------------------*/
	/* &eb[0] = %sp + frame_size			 */
	/* eb[0]  = EB_ARGV				 */
	/*-----------------------------------------------*/
	lghi	%r0,EB_ARGV
	stg	%r0,EBOFF_ARGVID(%r11)
	
	/*-----------------------------------------------*/
	/* *argv = %sp + frame_size + 8			 */
	/*-----------------------------------------------*/
	la	%r0,8(%r2)
	stg	%r0,EBOFF_ARGV(%r11)

	/*-----------------------------------------------*/
	/* *envp = (argc + 1) * 8 + %sp + frame_size	 */
	/*-----------------------------------------------*/
	lghi	%r0,EB_ENVP
	stg	%r0,EBOFF_ENVPID(%r11)
	lg	%r1,OFF_ARGC(%r2)
	aghi	%r1,2
	sllg	%r1,%r1,3
	agr	%r1,%r2
	stg	%r1,EBOFF_ENVP(%r11)
	lgr	%r0,%r1
	
	/*-----------------------------------------------*/
	/* *auxv = &end of env				 */
	/*-----------------------------------------------*/
	lghi	%r1,EB_AUXV
	stg	%r1,EBOFF_AUXVID(%r11)

	lgr	%r1,%r0
2:	lg	%r0,0(%r1)
	ltgr	%r0,%r0			// Search for end of env
	jz	3f

	aghi	%r1,8
	j	2b

3:
	aghi	%r1,8
	stg	%r1,EBOFF_AUXV(%r11)	// Store auxv

	lghi	%r1,EB_NULL		// Set up for the last pointer
	stg	%r1,EBOFF_NULLID(%r11)	// Save it
	
// Call _setup.  Two arguments, the ELF bootstrap vector and our (unrelocated)
// _DYNAMIC address.  The _DYNAMIC address is located in entry 0 of the GOT

	lgr	%r7,%r2			// Save eb ptr
	lgr	%r2,%r11		// Set parm 1
	lg	%r3,0(%r12)		// Get _DYNAMIC address
	brasl	%r14,_setup
	lgr	%r6,%r2			// Copy entry point address

// On return, give callee the exit function in %g1, and jump to the
// target program, clearing out the reserved globals as we go.
	
	lg	%r2,OFF_ARGC(%r7)	// Get argc
	lg	%r3,EBOFF_ARGV(%r11)	// Get **argv
	lg	%r4,EBOFF_ENVP(%r11)	// Get envp
	larl	%r5,atexit_fini
	br	%r6			// Call main program
	.size	_rt_boot, . - _rt_boot
#endif

/*========================= End of Function ========================*/

