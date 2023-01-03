/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mach_locore.s					    */
/* 								    */
/* Function    - Post-bootstrap low-core defintions and control     */
/* 		 register setup.				    */
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

	.macro DIAG r1,r2,code
	.byte	0x83
	.byte	\r1<<4+\r2
	.short	\code
	.endm

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#if defined(lint)
# include <sys/types.h>
# include <sys/t_lock.h>
#endif

#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/clock.h>
#include <sys/intreg.h>
#include <sys/panic.h>
#include <sys/privregs.h>
#include <sys/asm_linkage.h>

#include "assym.h"

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

#if defined(lint)

void
_start(void)
{}

#endif

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

#if !defined(lint)

/*
 * On the s390x we put the panic buffer in the fourth page.
 * We set things up so that the first 2 pages of KERNELBASE is illegal
 * to act as a redzone during copyin/copyout type operations. One of
 * the reasons the panic buffer is allocated in low memory to
 * prevent being overwritten during booting operations (besides
 * the fact that it is small enough to share pages with others).
 */

	.section	".data"
	.global	panicbuf

panicbuf = SYSBASE32 + PAGESIZE

	.type	panicbuf, @object
	.size	panicbuf, PANICBUFSIZE

/*
 * The thread 0 stack. This must be the first thing in the data
 * segment (other than an sccs string) so that we don't stomp
 * on anything important if the stack overflows. We get a
 * red zone below this stack for free when the kernel text is
 * write protected.
 */

	.global	t0stack
	.global	t0stacktop
	.align	16
	.type	t0stack, @object
	.type	t0stacktop, @object
t0stack:
	.skip	T0STKSZ-MINFRAME	// thread 0 stack
t0stacktop:
	.skip	MINFRAME
	.size	t0stack, T0STKSZ

/*
 * cpu array
 */
	.global	cpu
	.global	e_cpu
	.align	8
cpu:
	.type	cpu, @object
	.skip	512
	.size	cpu, 512
e_cpu:
	.type	e_cpu, @object

/*
 * cpu0 and its panic stack.  
 */
	.global	cpu0
cpu0:
	.type	cpu0, @object
	.skip	CPU_ALLOC_SIZE
	.size	cpu0, CPU_ALLOC_SIZE

/*
 * thread0 
 */
	.global t0
	.align	8
	.type	t0, @object
t0:
	.skip	THREAD_SIZE
	.size	t0, THREAD_SIZE

/*
 * lwp0's FP save area
 */
	.global l0fpu
	.align	8
	.type	l0fpu, @object
l0fpu:
	.skip	132
	.size	l0fpu, .-l0fpu

	.global memfirst
memfirst:
	.quad	0

	.align	8
	.global facilities
facilities:
	.quad	0
	.size	facilities, 8

	.align	8
	.global hw_serial
hw_serial:
	.quad	0
	.quad	0
	.size	hw_serial, 8

	.global	boottime
boottime:
	.quad	0
	.size	boottime, 8

	.align	4096
	.global	sccb_page
sccb_page:
	.skip	4096
	.size	sccb_page, 4096

	.align	4096
	.global	traceTbl
traceTbl:
	.skip	TRACETBL_SIZE * 4096
	.size	traceTbl, .-traceTbl

	.section ".rodata"
	.align	16
	.global	bootConst
bootConst: 
.Lccons: 
.Lct0p:	.quad	t0

.LpswP: .long	0x04000001,0x80000000
	.quad	pgm_flih
.LpswS: .long	0x04000001,0x80000000
	.quad	svc_flih
.LpswM: .long	0x04000001,0x80000000
	.quad	mch_flih
.LpswI: .long	0x04000001,0x80000000
	.quad	io_flih
.LpswE: .long	0x04000001,0x80000000
	.quad	ext_flih
.LpswR: .long	0x00000001,0x80000000
	.quad	rst_flih
	.global qscPSW
qscPSW:	.long	0x00020000,0x80000000
	.quad	0x0fff
	.size	bootConst, .-bootConst

bootepoch:
	.quad	TOD_EPOCH
	.size	bootepoch, 8

	.global	getcr
getcr:	stctg	0,0,0(%r3)
	
	.global	setcr
setcr:	lctlg	0,0,0(%r3)
	
#endif	/* lint */

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if !defined(lint)

	.section	".text"
.text

/*
 * System initialization
 *
 * Our tasks are:
 * 	save parameters
 * 	construct mappings for KERNELBASE
 * 	hop up into high memory         
 * 	initialize stack pointer
 * 	figure out all the module type stuff
 * 	tear down the 1-1 mappings
 * 	dive into main()
 */
	ENTRY_NP(_start)

	//
	// Address the constants
	//
	larl	%r13,bootConst

	//
	// Save boot time and adjust to our epoch
	//
	larl	%r1,boottime
	stck	0(%r1)
	lg	%r0,0(%r1)
	sg	%r0,bootepoch-.Lccons(%r13)
	stg	%r0,0(%r1)

	//
	// Save our CPU id
	//
	larl	%r1,hw_serial
	stidp	0(%r1)

	//
	// Initialize global thread register.
	//
	mvc	CPU_THREAD(8,%r1),.Lct0p-.Lccons(%r13)
	lam	%a0,%a1,.Lct0p-.Lccons(%r13)

	//
	// Initialize thread 0's stack.
	//
	larl	%r15,t0stacktop			// Setup kernel stack pointer
	aghi	%r15,-SA(MINFRAME)

	//
	// Plug in PSWs
	//
	mvc	__LC_EXT_NEW_PSW(16,%r0),.LpswE-.Lccons(%r13)
	mvc	__LC_IO_NEW_PSW(16,%r0),.LpswI-.Lccons(%r13)
	mvc	__LC_MC_NEW_PSW(16,%r0),.LpswM-.Lccons(%r13)
	mvc	__LC_PGM_NEW_PSW(16,%r0),.LpswP-.Lccons(%r13)
	mvc	__LC_RST_NEW_PSW(16,%r0),.LpswR-.Lccons(%r13)
	mvc	__LC_SVC_NEW_PSW(16,%r0),.LpswS-.Lccons(%r13)

	//
	// Fill in enough of the cpu structure so that
	// the wbuf management code works. Make sure the
	// boot cpu is inserted in cpu[] based on cpuid.
	//
	larl	%r2,cpu				// Get &cpu[]
	lgr	%r4,%r2				// Copy
	larl	%r3,e_cpu			// Get end of CPU list
	sgr	%r3,%r2				// Get table length
	lgr	%r0,%r2				// Copy
	lghi	%r1,0				// Destination length
	mvcl	%r2,%r0				// Clear
	larl	%r0,cpu0			// Get &cpu0
	stap	0(%r15)				// Get CPU address 
	lgh	%r1,0(%r15)			
	slag	%r1,%r1,3			// Form index
	stg	%r0,0(%r1,%r4)			// cpu[index] = &cpu0

	//
	// Get list of facilities
	//
	stfl	0				// Get facilities supported
	larl	%r1,facilities			// Get A(Facilities area)
	mvc	0(8,%r1),__LC_STFL_LIST		// Copy to global area

	//
	// Call mlsetup to prepare things before we all main
	//
	lgr	%r2,%r15
	aghi	%r2,-SA(MINFRAME)
	brasl	%r14,mlsetup

	//
	// Now call main.  We will return as process 1 (init).
	//
	brasl	%r14,main

	//
	// Main should never return.
	//
	lpswe	qscPSW-.Lccons(%r13)
	SET_SIZE(_start)

.Lcmainretmsg:
	.asciz	"main returned"
	.align	4

#endif	/* lint */

/*========================= End of Function ========================*/
