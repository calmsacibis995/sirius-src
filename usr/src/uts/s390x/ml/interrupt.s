/*------------------------------------------------------------------*/
/* 								    */
/* Name        - interrupt.s					    */
/* 								    */
/* Function    - FLIH for all interrupts.                           */
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

#define	INTRCNT_LIMIT	16
#define EXTRC		0x01	// Explicit trace control value for cr12

#define CR12MASK_H	0x3fffffff
#define CR12MASK_L	0xffffff00

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#if defined(lint)
# include <sys/types.h>
# include <sys/thread.h>
#else	/* lint */
# include "assym.h"
#endif	/* lint */

#include <sys/cmn_err.h>
#include <sys/ftrace.h>
#include <sys/asm_linkage.h>
#include <sys/machcpuvar.h>
#include <sys/machthread.h>
#include <sys/machparam.h>
#include <sys/exts390x.h>
#include <sys/machlock.h>
#include <sys/trap.h>
#include <sys/intr.h>

#ifdef TRAPTRACE
// #include <sys/traptrace.h>
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

	.section ".rodata"
	.align	8
.svcTAB:
	.quad	.syscall,  .nosvcall, .nosvcall, .nosvcall	//  0- 3
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	//  4- 7
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	//  8- B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	//  C- F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 10-13
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 14-17
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 18-1B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 1C-1F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 20-23
	.quad	.gethrtm,  .gethrvtm, .nosvcall, .gethrstm	// 24-27
	.quad	.nosvcall, .getlgrp,  .nosvcall, .nosvcall	// 28-2B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 2C-2F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 30-33
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 34-37
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 38-3B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 3C-3F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 40-43
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 44-47
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 48-4B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 4C-4F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 50-53
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 54-57
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 58-5B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 5C-5F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 60-63
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 64-67
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 68-6B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 6C-6F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 70-73
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 74-77
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 78-7B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 7C-7F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 80-83
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 84-87
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 88-8B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 8C-8F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 90-93
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 94-97
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 98-9B
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// 9C-9F
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// A0-A3
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// A4-A7
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// A8-AB
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// AC-AF
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// B0-B3
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// B4-B7
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// B8-BB
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// BC-BF
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// C0-C3
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// C4-C7
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// C8-CB
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// CC-CF
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// D0-D3
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// D4-D7
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// D8-DB
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// DC-DF
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// E0-E3
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// E4-E7
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// E8-EB
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// EC-EF
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// F0-F3
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// F4-F7
	.quad	.nosvcall, .nosvcall, .nosvcall, .nosvcall	// F8-FB
	.quad	.nosvcall, .nosvcall, .nosvcall, .softcall	// FC-FF
	SET_SIZE(.svcTAB)
	.section ".text"

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- svc_flih.                                         */
/*                                                                  */
/* Function	- Syscall interrupt handler. Obtain the SVC code    */
/*		  from low-core, determine if it's a syscall        */
/*		  and pass control to syscall handler otherwise     */
/*		  check against the internal call table and pass    */
/*		  control to any notifiable routine.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
svc_flih()
{}

#else	/* lint */

	ENTRY_NP(svc_flih)
	KENTER  __LC_SVC_OLD_PSW,__LC_SVC_ILC,1
	lgr	%r11,%r0		// Copy syscall no.
#ifdef DEBUG
	lmg	%r0,%r1,__LC_SVC_OLD_PSW
	tracg	%r0,%r15,__LC_SVC_TRACE	// Trace this syscall
#endif
	brasl	%r14,tod2ticks		// Update lbolt

	GET_THR(9)

	llgh	%r7,__LC_SVC_INTCODE	// Get SVC code
	larl	%r3,.svcTAB		// Point at SVC table
	sllg	%r4,%r7,3		// Form index
	lg	%r1,0(%r4,%r3)		// Get A(SVC Routine)
	basr	%r14,%r1		// Go to SVC routine

	KLEAVE
	SET_SIZE(svc_flih)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- .syscall.                                         */
/*                                                                  */
/* Function	- System call handler.                              */
/*		                               		 	    */
/* On Entry	- %r14 - return address        		 	    */
/*          	- %r9  - Current thread pointer		 	    */
/*          	- %r11 - Syscall number 			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
syscall()
{}

#else	/* lint */

.syscall:
	lgr	%r10,%r14		// Save link

	sth	%r11,T_SYSNUM(%r9)	// Save syscall no.
	lg	%r7,T_LWP(%r9)		// Get LWP
	la	%r2,MINFRAME(%r15)	// Point at registers on entry
	stg	%r2,LWP_REGS(%r7)	// Save in LWP
	tm	KSTK_PSW+3(%r2),0x01	// This from a 64-bit app?
	jno	.syscall32		// No... It's a 32-bit app

	brasl	%r14,syscall_trap	// Go process syscall
	j	1f

.syscall32:
	brasl	%r14,syscall_trap32	// Go process syscall

1:	
	lgr	%r14,%r10
	br	%r14

#endif

/*========================= End of Function ========================*/
	
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- .softcall.					    */
/*                                                                  */
/* Function	- Handle a softcall request.			    */
/*		                               		 	    */
/* On Entry	- %r14 - return address        		 	    */
/*          	- %r9  - Current thread pointer		 	    */
/*          	- %r11 - Syscall number (not applicable here)	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
softcall()
{}

#else	/* lint */

.softcall:
	lgr	%r10,%r14		// Save link
	cghi	%r11,T_SOFTINT		// Soft interrupt?
	jne	1f			// No... Ignore

	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,dosoftint		// Perform soft interrupt
	j	2f			// Exit time

1:	
	cghi	%r11,ST_KMDB_TRAP	// Invoke KMDB?
	jne	2f			// No... Ignore

	lg	%r3,__LC_CPU		// Address CPU
	lgf	%r4,CPU_STPENDING(%r3)	// Get pending flag
	ltgr	%r4,%r4			// Any softints pending?
	jz	2f			// No... Just return

	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,dosoftint		// Handle soft interrupt

2:
	lgr	%r14,%r10
	br	%r14

#endif

/*========================= End of Function ========================*/
	
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- .gethrtm.					    */
/*                                                                  */
/* Function	- Process a gethrtime() request (fast call).	    */
/*		                               		 	    */
/* On Entry	- %r14 - return address        		 	    */
/*          	- %r9  - Current thread pointer		 	    */
/*          	- %r11 - Syscall number (not applicable here)	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

hrtime_t
gethrtm()
{
        hrtime_t ts;

        gethrtime((&ts);
        return (ts);
}

#else	/* lint */

.gethrtm:
	lgr	%r10,%r14		// Save link
	la	%r6,MINFRAME(%r15)	// Get A(Save Area)
	aghi	%r15,-SA(MINFRAME+8)
	brasl	%r14,gethrtime		// Get time struct
	tm	KSTK_PSW+3(%r6),0x01	// This from a 64-bit app?
	jo	.exgethrtm		// Yes... Just use r2

.gethrtm32:
	lgfr	%r3,%r2			// Copy lower half of result
	nihf	%r3,0			// Clear top 
	srlg	%r2,%r2,32		// Isolate upper half
	stg	%r3,KSTK_R3(%r6)	// Save lower half

.exgethrtm:
	stg	%r2,KSTK_R2(%r6)	// Save in caller's R2	
	aghi	%r15,SA(MINFRAME+8)
	lgr	%r14,%r10		// Restore link
	br	%r14

#endif

/*========================= End of Function ========================*/
	
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- .getlgrp.					    */
/*                                                                  */
/* Function	- Handle the fast call getlgrp request.		    */
/*		                               		 	    */
/* On Entry	- %r14 - return address        		 	    */
/*          	- %r9  - Current thread pointer		 	    */
/*          	- %r11 - Syscall number (not applicable here)	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
()
{}

#else	/* lint */

.getlgrp:

	la	%r6,MINFRAME(%r15)	// Get A(Save Area)
	lg	%r3,__LC_CPU		// Get CPU structure
	lgf	%r2,CPU_ID(%r3)		// Get CPU id
	lg	%r4,CPU_THREAD(%r3)	// Get thread
	lg	%r5,T_LPL(%r4)		// Get LPL pointer
	lgf	%r2,LPL_LGRPID(%r5)	// Get lgl_lgrpid
	stg	%r2,KSTK_R2(%r6)	// Save in caller's R2	
	
	br	%r14

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- .gethrstm.					    */
/*                                                                  */
/* Function	- Process a gethrestime() request (fast call).	    */
/*		                               		 	    */
/* On Entry	- %r14 - return address        		 	    */
/*          	- %r9  - Current thread pointer		 	    */
/*          	- %r11 - Syscall number (not applicable here)	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
gethrstm()
{}

#else	/* lint */

.gethrstm:
	lgr	%r10,%r14		// Save link
	la	%r6,MINFRAME(%r15)	// Get A(Save Area)
	aghi	%r15,-SA(MINFRAME+CLONGSIZE*2)
	lay	%r2,-CLONGSIZE*2(%r15)	// Point at *tm struct
	lgr	%r7,%r2			// Copy
	brasl	%r14,gethrestime	// Get time in secs/nanosecs
	lmg	%r2,%r3,0(%r7)		// Get tm
	tm	KSTK_PSW+3(%r6),0x01	// This from a 64-bit app?
	jo	.gethrstm64		// Yes... It's a 64-bit app

	nihf	%r2,0			// Clear top half
	nihf	%r3,0			// Clear top half

.gethrstm64:
	stg	%r2,KSTK_R2(%r6)	// Save in caller's R2	
	stg	%r3,KSTK_R3(%r6)	// Save in caller's R3	
	aghi	%r15,SA(MINFRAME+CLONGSIZE*2)
	lgr	%r14,%r10		// Restore link
	br	%r14

#endif

/*========================= End of Function ========================*/
	
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- .gethrvtm.					    */
/*                                                                  */
/* Function	- Process a gethrvtime() request (fast call).	    */
/*		                               		 	    */
/* On Entry	- %r14 - return address        		 	    */
/*          	- %r9  - Current thread pointer		 	    */
/*          	- %r11 - Syscall number (not applicable here)	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
gethrvtm()
{}

#else	/* lint */

.gethrvtm:
	lgr	%r10,%r14
	la	%r6,MINFRAME(%r15)	// Get A(Save Area)
	aghi	%r15,-SA(MINFRAME+8)
	la	%r2,MINFRAME(%r15)
	brasl	%r14,gethrtime
	lg	%r2,MINFRAME(%r15)
	lg	%r3,T_LWP(%r9)		// Get A(LWP)
	lg	%r4,LWP_STATE_START(%r3)
	lg	%r5,LWP_ACCT_USER(%r3)
	sgr	%r2,%r4
	agr	%r2,%r5
	tm	KSTK_PSW+3(%r6),0x01	// This from a 64-bit app?
	jno	.gethrvtm32		// No... It's a 32-bit app

	stg	%r2,KSTK_R2(%r6)	// Save in caller's R2	
	j	.exgethrvtm

.gethrvtm32:
	lgfr	%r3,%r2			// Copy lower half of result
	nihf	%r3,0			// Clear top
	srlg	%r2,%r2,32		// Isolate upper half
	stg	%r2,KSTK_R2(%r6)	// Save top half
	stg	%r3,KSTK_R3(%r6)	// Save lower half

.exgethrvtm:
	aghi	%r15,SA(MINFRAME+8)
	lgr	%r14,%r10		// Restore link
	br	%r14

#endif

/*========================= End of Function ========================*/
	
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- .nosvcall.  					    */
/*                                                                  */
/* Function	- Unsupported call.                                 */
/*		                               		 	    */
/* On Entry	- %r14 - return address        		 	    */
/*          	- %r9  - Current thread pointer		 	    */
/*          	- %r11 - Syscall number (not applicable here)	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
nosvcall()
{}

#else	/* lint */

.nosvcall:

	la	%r6,MINFRAME(%r15)	// Get A(Save Area)
	lg	%r3,__LC_CPU		// Get CPU structure
	lghi	%r2,-1
	tm	KSTK_PSW+3(%r6),0x01	// This from a 64-bit app?
	jo	.nosvc64		// Yes... Skip

	srlg	%r2,%r2,32		// Fullword sized result

.nosvc64:
	stg	%r2,KSTK_R2(%r6)	// Save in caller's R2	
	stg	%r2,KSTK_R0(%r6)	// Save in caller's R0
	br	%r14

#endif

/*========================= End of Function ========================*/
	
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pgm_flih.                                         */
/*                                                                  */
/* Function	- Program interrupt first level handler.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
pgm_flih()
{}

#else	/* lint */

	ENTRY_NP(pgm_flih)
	KENTER  __LC_PGM_OLD_PSW,__LC_PGM_ILC,1
	mvc	__LC_RST_OLD_PSW(16,0),__LC_PGM_OLD_PSW
	stg	%r15,__LC_PARMREG+16

	/*
	 * We need to handle the trace table overflow here so we
	 * don't invoke anything that may execute another TRACG 
	 * intruction which causes us to be called recursively (ad 
	 * infinitum).
	 */
	llgh	%r0,__LC_PGM_INTCODE		// Get interrupt code
	nill	%r0,0x7f			// Strip off any PER
	cghi	%r0,PXC_TRT			// Is this a trace overflow?
	jne	0f				// No... Skip
	
	/*
	 * For this interrupt we simply reset the pointer in CR12
	 * to the start of the trace table or the next page of
	 * the trace table and leave.
	 */
	lg	%r9,__LC_CPU			// Get A(CPU Structure)
	stctg	%c12,%c12,48(%r15)		// Get CR12
	lg	%r0,48(%r15)			// Get contents of CR12
	nihf	%r0,CR12MASK_H			// Strip to leave table address
	nilf	%r0,CR12MASK_L			// ....
	lg	%r1,CPU_TRACETBL(%r9)		// Get A(Start of Trace Table)
	lgr	%r2,%r1				// Copy
	ag	%r1,CPU_LTRACETBL(%r9)		// Point at end
	aghi	%r1,-MMU_PAGESIZE		// Point at start of last page
	cgr	%r0,%r1				// Are we at end of table?
	jnl	1f				// No... Just go to next page

	aghi	%r0,MMU_PAGESIZE		// Bump to next page
	nill	%r0,0xf000			// Reset to start of page
	lgr	%r2,%r0				// Copy
1:
	oill	%r2,EXTRC			// Enable for tracing
	stg	%r2,48(%r15)			// Save for reload
	lctlg	%c12,%c12,48(%r15)		// Reload trace register
	j	2f

0:
	brasl	%r14,tod2ticks			// Update lbolt
#ifdef DEBUG
	lm	%r0,%r1,__LC_PGM_OLD_PSW	// Get old PSW
	llgh	%r3,__LC_PGM_INTCODE		// Get interrupt code
	lg	%r4,__LC_XLT_EXCID		// Get translate id
	tracg	%r0,%r15,__LC_PGM_TRACE		// Add trace table entry
#endif
	la	%r2,MINFRAME(%r15)		// Point at registers on entry
	tm	KSTK_PSW+1(%r2),F_PSW_PROB	// Is this a user process
	jz	.pgm_sys			// No... No need to play with LWP

	GET_THR(9)
	
	lg	%r7,T_LWP(%r9)			// Get LWP
	stg	%r2,LWP_REGS(%r7)		// Save in LWP

.pgm_sys:
	brasl	%r14,pgm_slih

2:
	KLEAVE
	SET_SIZE(pgm_flih)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mch_flih.                                         */
/*                                                                  */
/* Function	- Machine check interrupt first level handler.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
mch_flih()
{}

#else	/* lint */

	ENTRY_NP(mch_flih)
	KENTER  __LC_MC_OLD_PSW,0,0
	brasl	%r14,tod2ticks		// Update lbolt

	lghi	%r3,S390_INTR_MCHK	// Set vector
	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,do_interrupt	// Handle hard interrupt

	lg	%r3,__LC_CPU		// Address CPU
	lgf	%r4,CPU_STPENDING(%r3)	// Get pending flag
	ltgr	%r4,%r4			// Any softints pending?
	jz	1f			// No... Just return

	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,dosoftint		// Handle soft interrupt
1:
	KLEAVE
	SET_SIZE(mch_flih)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ext_flih.                                         */
/*                                                                  */
/* Function	- External interrupt first level handler.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
ext_flih()
{}

#else	/* lint */
	
	ENTRY_NP(ext_flih)
	KENTER  __LC_EXT_OLD_PSW,0,0
	brasl	%r14,tod2ticks		// Update lbolt

	llgh	%r2,__LC_EXT_INTCODE	// Get interrupt code
	cghi	%r2,EXT_HWCN		// Was this the console?
	je      2f                      // Yes, bypass normal intr handling

	lghi	%r3,S390_INTR_EXT	// Set vector
	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,do_interrupt	// Handle hard interrupt

	lg	%r3,__LC_CPU		// Address CPU
	lgf	%r4,CPU_STPENDING(%r3)	// Get pending flag
	ltgr	%r4,%r4			// Any softints pending?
	jz	1f			// No... Just return

	stnsm	__LC_SCRATCH,0x04	// Ensure interrupts are disabled
	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,dosoftint		// Handle soft interrupt
1:
	stnsm	__LC_SCRATCH,0x04	// Ensure interrupts are disabled
	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,sys_rtt_common	// Check if thread needs preemption
	KLEAVE
2:
	llgh    %r4,__LC_EXT_SUBCODE    // Get subcode
	llgh    %r3,__LC_EXT_INTCODE    // Get interrupt code
	l       %r2,__LC_EXT_INTPARM    // Get interrupt parm
	brasl   %r14,sclp_intr          // Handle it
	KLEAVE
	SET_SIZE(ext_flih)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- io_flih.                                          */
/*                                                                  */
/* Function	- I/O interrupt first level handler.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
io_flih()
{}

#else	/* lint */

	ENTRY_NP(io_flih)
	KENTER  __LC_IO_OLD_PSW,0,0
	brasl	%r14,tod2ticks		// Update lbolt

	lghi	%r3,S390_INTR_IO	// Set vector
	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,do_interrupt	// Handle hard interrupt

	lg	%r3,__LC_CPU		// Address CPU
	lgf	%r4,CPU_STPENDING(%r3)	// Get pending flag
	ltgr	%r4,%r4			// Any softints pending?
	jz	1f			// No... Just return

	la	%r2,MINFRAME(%r15)	// Copy stack ptr
	brasl	%r14,dosoftint		// Handle soft interrupt
1:
	KLEAVE
	SET_SIZE(io_flih)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- rst_flih.                                         */
/*                                                                  */
/* Function	- Restart interrupt first level handler.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
rst_flih()
{}

#else	/* lint */

	ENTRY_NP(rst_flih)
	lamy	%a0,%a15,__LC_SSMC_AR_AREA
	lctlg	%c0,%c15,__LC_SSMC_CR_AREA
	stosm	__LC_SCRATCH,0x04
	lg	%r9,__LC_SSMC_GR_AREA
	lg	%r1,__LC_CPU
	lg	%r1,CPU_THREAD(%r1)
	lg	%r15,T_SP(%r1)
	br	%r9
	SET_SIZE(rst_flih)

#endif

/*========================= End of Function ========================*/
