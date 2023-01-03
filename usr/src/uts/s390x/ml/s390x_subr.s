/*------------------------------------------------------------------*/
/* 								    */
/* Name        - s390x_subr.c  					    */
/* 								    */
/* Function    - General assembly language routines.		    */
/* 								    */
/* 		 It is the intent of this file to contain routines  */
/*		 that are independent of the specific kernel arch-  */
/*		 itecture, and those that are common across kernel  */
/*		 architectures.					    */
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
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#if defined(lint)
#include <sys/types.h>
#include <sys/scb.h>
#include <sys/systm.h>
#include <sys/regset.h>
#include <sys/sunddi.h>
#include <sys/lockstat.h>
#include <sys/dtrace.h>
#endif	/* lint */

#include "assym.h"
#include <sys/asm_linkage.h>
#include <sys/machparam.h>	/* To get SYSBASE and PAGESIZE */
#include <sys/isa_defs.h>
#include <sys/dditypes.h>
#include <sys/panic.h>
#include <sys/machlock.h>
#include <sys/ontrap.h>
#include <sys/machs390x.h>
#include <sys/privregs.h>
#include <sys/clock.h>


/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

#define membar	br	%r0

#define _REGSIZE 16*8

#define NANO2TICK	10000000

/*
 * Macro to set the interrupt mask according to a PIL
 * 
 * - PIL is in %r2
 * - OLDPIL is in %r4
 * - Assumes %r3 has pointer to CPU structure
 */
	.macro  INTMASK
	sllg	%r2,%r2,3		/* Form index from PIL	      */
	larl	%r1,PILTable		/* Get address of PIL table   */
	agr	%r1,%r2			/* Locate entry pointer	      */
	lg	%r1,0(%r1)		/* Find PIL table entry	      */
	stctg	%c0,%c0,48(%r15)	/* Save CR0		      */
	nc	48(8,%r15),0(%r1)	/* Turn off unwanted bits     */
	oc	48(8,%r15),8(%r1)	/* Turn on wanted bits	      */
	lctlg	%c0,%c0,48(%r15)	/* Reload CR0		      */
	stctg	%c6,%c6,48(%r15)	/* Save CR6		      */
	nc	48(8,%r15),16(%r1)	/* Turn off unwanted bits     */
	oc	48(8,%r15),24(%r1)	/* Turn on wanted bits	      */
	lctlg	%c6,%c6,48(%r15)	/* Reload CR6		      */
	stctg	%c14,%c14,48(%r15)	/* Save CR14		      */
	nc	48(8,%r15),32(%r1)	/* Turn off unwanted bits     */
	oc	48(8,%r15),40(%r1)	/* Turn on wanted bits	      */
	lctlg	%c14,%c14,48(%r15)	/* Reload CR14		      */
									
5:									
	.endm

/*
 * Macro to raise processor priority level.
 * Avoid dropping processor priority if already at high level.
 * Also avoid going below CPU->cpu_base_spl, which could've just been set by
 * a higher-level interrupt thread that just blocked.
 *
 * level is in %r2
 */
	.macro	RAISE
	stnsm	56(%r15),0x04		/* Disable I/O & Externals    */
	lg	%r3,__LC_CPU		/* Get CPU		      */
	lgf	%r4,MCPU_PRI(%r3)	/* Get current priority	      */
	cgr	%r2,%r4			/* New < current?	      */
	jgl	1f			/* Yes... return	      */
	lgf	%r1,CPU_BASE_SPL(%r3)	/* Get base priority	      */
	cgr	%r2,%r1			/* New > base?		      */ 
	jgh	2f			/* Yes...use new priority     */
	lgr	%r2,%r1			/* Use base priority	      */
2:									
	st  	%r2,MCPU_PRI(%r3)	/* Set new priority	      */

	INTMASK				/* Enable/Disable interrupts  */
1:									
	ssm	56(%r15)		/* Restore mask               */
	lgr	%r2,%r4			/* Return old PIL	      */
	br	%r14			/* Return                     */
	.endm

/*
 * Macro to set the priority to a specified level.
 * Avoid dropping the priority below CPU->cpu_base_spl.
 *
 * newpil is in %r2
 */
	.macro 	SETPRI
	stnsm	56(%r15),0x04		/* Disable I/O & Externals    */
	lg	%r3,__LC_CPU		/* Get CPU		      */
	lgf	%r4,MCPU_PRI(%r3)	/* Get current PIL	      */
	lgf	%r1,CPU_BASE_SPL(%r3)	/* Get base priority	      */
	cgr	%r2,%r1			/* New > base?		      */ 
	jgh	1f			/* Yes...use new priority     */
	lgr	%r2,%r1			/* Use base priority	      */
1:									
	st  	%r2,MCPU_PRI(%r3)	/* Set new priority	      */
									
	INTMASK				/* Enable/Disable interrupts  */
									
	ssm	56(%r15)		/* Restore mask               */
	lgr	%r2,%r4			/* Get old PIL		      */
	br	%r14			/* Return to caller	      */
	.endm

/*
 * Since all of the fuword() variants are so similar, we have a macro to spit
 * them out.
 */

#define	FUWORD(NAME, LOAD, STORE, COPYOP)	\
	ENTRY(NAME);				\
	GET_THR(1);				\
	lg	%r4,T_LOFAULT(%r1);		\
	larl	%r5,1f;				\
	stg	%r5,T_LOFAULT(%r1);		\
	lghi	%r0,1;				\
	sar	%a2,%r0;			\
	sacf	AC_ACCESS;			\
	LOAD	%r0,0(%r2);			\
	sacf	AC_PRIMARY;			\
	stg	%r4,T_LOFAULT(%r1);		\
	STORE	%r0,0(%r3);			\
	lghi	%r2,0;				\
	br	%r14;				\
1:						\
	sacf	AC_PRIMARY;			\
	stg	%r4,T_LOFAULT(%r1);		\
	lg	%r1,T_COPYOPS(%r1);		\
	ltgr	%r1,%r1;			\
	jz	2f;				\
						\
	lg	%r1,COPYOP(%r1);		\
	br	%r1;				\
2:						\
	lghi	%r2,-1;				\
	br	%r14;				\
	SET_SIZE(NAME)

/*
 * Since all of the suword() variants are so similar, we have a macro to spit
 * them out.
 */

#define	SUWORD(NAME, STORE, COPYOP)		\
	ENTRY(NAME)				\
	GET_THR(1);				\
	lg	%r4,T_LOFAULT(%r1);		\
	larl	%r5,1f;				\
	stg	%r5,T_LOFAULT(%r1);		\
	lghi	%r0,1;				\
	sar	%a2,%r0;			\
	sacf	AC_ACCESS;			\
	STORE	%r3,0(%r2);			\
	sacf	AC_PRIMARY;			\
	stg	%r4,T_LOFAULT(%r1);		\
	lghi	%r2,0;				\
	br	%r14;				\
1:						\
	sacf	AC_PRIMARY;			\
	stg	%r4,T_LOFAULT(%r1);		\
	lg	%r1,T_COPYOPS(%r1);		\
	ltgr	%r1,%r1;			\
	jz	2f;				\
						\
	lg	%r1,COPYOP(%r1);		\
	br	%r1;				\
						\
2:						\
	lghi	%r2,-1;				\
	br	%r14;				\
	SET_SIZE(NAME)

/*
 * Macro to get current time in nanoseconds
 */
	.macro 	TOD2NANO
	larl	%r1,c_tod2epoch		// Address our constants
	slg	%r2,0(%r1)		// Adjust to our epoch

	lgfi	%r1,US_MASK		// Get clock units mask
	ngr	%r1,%r2			// Mask off full micros
	lgfi	%r5,CVT2PICO		// Get clock units to femtos factor
	msgr	%r5,%r1			// Calc femtos
	lgfi	%r1,TODFACTOR		// Get femtos to nanos factor
	lghi	%r4,0			// Clear for divide
	dlgr	%r4,%r1			// Convert to nanos
	srag	%r1,%r1,1		// Get half of factor
	sgr	%r4,%r1			// Do we need to round up?
	jm	1f			// Nope... Skip

	aghi	%r5,1			// Round up
1:
	srlg	%r2,%r2,US_SHIFT	// Remove fractional micros
	mghi	%r2,US_TO_NS		// Convert micros to nanos
	agr	%r2,%r5			// Sum
	.endm

/*
 * Macro to get convert nanoseconds to tod clock value
 */
	.macro 	NANO2TOD
	lgr	%r5,%r2			// Get nanos
	lghi	%r4,0			// Clear for divide
	lghi	%r3,US_TO_NS		// Get nanos to micros factor
	dlgr	%r4,%r3			// Convert to micros
	sllg	%r2,%r5,US_SHIFT	// Convert to clock units and save

	lgr	%r5,%r4			// Get remaining nanos
	lghi	%r4,0			// Clear for multiply
	lgfi	%r3,TODFACTOR		// Get nanos to femtos factor
	mlgr	%r4,%r3			// Convert remaining nanos to femtos
	lgfi	%r3,CVT2PICO		// Get femtos to clock units factor
	dlgr	%r4,%r3			// Convert to clock units
	srlg	%r3,%r3,1		// Get half of factor
	sgr	%r4,%r3			// Do we need to round up?
	jm	1f			// Nope... Skip

	aghi	%r5,1			// Round up
1:
	algr	%r2,%r5			// Sum
	larl	%r1,c_tod2epoch		// Address our constants
	alg	%r2,0(%r1)		// Adjust to tod epoch
	.endm

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#if defined(lint)
#include <sys/types.h>
#include <sys/scb.h>
#include <sys/systm.h>
#include <sys/regset.h>
#include <sys/sunddi.h>
#include <sys/lockstat.h>
#include <sys/dtrace.h>
#endif	/* lint */

#include <sys/asm_linkage.h>
#include <sys/machparam.h>	/* To get SYSBASE and PAGESIZE */
#include <sys/isa_defs.h>
#include <sys/dditypes.h>
#include <sys/panic.h>
#include <sys/machlock.h>
#include <sys/ontrap.h>
#include <sys/machs390x.h>
#include <sys/privregs.h>
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


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

	.section	".rodata"
	.align	8

	.global		waitPSW
waitPSW: 
	.quad	0x0702000180000000,wakePoint
	.size	waitPSW, .-waitPSW

	.global		stosm
stosm:  stosm	48(%r15),0

	.global	c_tod2tick
c_tod2tick:
c_tod2epoch:	.quad	TOD_EPOCH

	/*
	 * Table defining the PSW and control register masks 
	 * for each Program Interrupt Level
	 */
	.global	PILTable
	.align	8
PILTable:
	.quad	.lcPL0
	.quad	.lcPL1
	.quad	.lcPL2
	.quad	.lcPL3
	.quad	.lcPL4
	.quad	.lcPL5
	.quad	.lcPL6
	.quad	.lcPL7
	.quad	.lcPL8
	.quad	.lcPL9
	.quad	.lcPL10
	.quad	.lcPL11
	.quad	.lcPL12
	.quad	.lcPL13
	.quad	.lcPL14
	.quad	.lcPL15

	/*----------------------------------------------------------*/
	/* Table Format:					    */
	/* ------------						    */
	/* CR0	- Masks to turn off bits			    */
	/* 	  Masks to turn on bits				    */
	/*							    */
	/* CR6	- Masks to turn off bits			    */
	/* 	  Masks to turn on bits				    */
	/*							    */
	/* CR14	- Masks to turn off bits			    */
	/* 	  Masks to turn on bits				    */
	/*							    */
	/* PSW  - System mask setting				    */
	/*							    */
	/*----------------------------------------------------------*/

	/*
	 * PL0 - All I/O interrupts; 
	 *	 All Externals; 
	 *	 Machine Check I/O
	 */
	.align	8
.lcPL0:	
	.long	0xffffffff					// +0
	.byte	0b11111111,0b11111111,0b00000000,0b00000000	// +4
	.long	0						// +8
	.byte	0b00000000,0b00000000,0b11101110,0b00010010	// +c

	.long	0xffffffff					// +10
	.byte	0x00000000,0,0,0				// +14
	.long	0						// +18
	.byte	0b11111111,0,0,0				// +1c

	.long	0xffffffff					// +20
	.byte	0b11101111,0xff,0xff,0xff			// +24
	.long	0						// +28
	.byte	0b00010000,0,0,0				// +2c

	.byte	0x07,0,0,0					// +30

	.long	0						// +34

	/*
	 * PL1 - Pri 2-7 I/O interrupts; 
	 *	 All Externals; 
	 *	 Machine Check I/O
	 */
	.align	8
.lcPL1:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101110,0b00010010

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00111111,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00010000,0,0,0

	.byte	0x07,0,0,0

	.long	0

	/*
	 * PL2 - Pri 4-7 I/O interrupts; 
	 *	 All Externals; 
	 *	 Machine Check I/O
	 */
	.align	8
.lcPL2:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101110,0b00010010

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00001111,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00010000,0,0,0

	.byte	0x07,0,0,0

	.long	0

	/*
	 * PL3 - Pri 6-7 I/O interrupts; 
	 *	 All Externals; 
	 *	 Machine Check I/O
	 */
	.align	8
.lcPL3:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101110,0b00010010

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000011,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00010000,0,0,0

	.byte	0x07,0,0,0

	.long	0

	/*
	 * PL4 - Pri 7 I/O interrupts; 
	 *	 All Externals; 
	 *	 Machine Check I/O
	 */
	.align	8
.lcPL4:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101110,0b00010010

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000001,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00010000,0,0,0

	.byte	0x07,0,0,0

	.long	0

	/*
	 * PL5 - No I/O interrupts; 
	 *	 All Externals; 
	 *	 Machine Check I/O
	 */
	.align	8
.lcPL5:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101110,0b00010010

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00010000,0,0,0

	.byte	0x05,0,0,0

	.long	0

	/*
	 * PL6 - No I/O interrupts; 
	 *	 All Externals; 
	 *	 No Machine Check I/O
	 */
	.align	8
.lcPL6:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101110,0b00010010

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0

	.long	0

	/*
	 * PL7 - No I/O interrupts; 
	 *	 No IUCV Externals; 
	 *	 No Machine Check I/O
	 */
	.align	8
.lcPL7:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101110,0b00010000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0

	.long	0

	/*
	 * PL8 - No I/O interrupts; 
	 *	 No IUCV Externals;
	 *       No Machine Check I/O
	 */
	.align	8
.lcPL8:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101110,0b00010000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0

	.long	0

	/*
	 * PL9 - No I/O interrupts; 
	 *	 No IUCV, No Signal Service Externals; 
	 *       No Machine Check I/O
	 */
	.align	8
.lcPL9:	
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11101100,0b00010000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0

	.long	0

	/*
	 * PL10 - No I/O interrupts; 
	 *	  No IUCV, No Signal Service Externals; No clocks; No ExtCall
	 *        No Machine Check I/O
	 */
	.align	8
.lcPL10:
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b11000000,0b00000000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0,0

	.long	0

	/*
	 * PL11 - No I/O interrupts; 
	 *	  No Externals;
	 *        No Machine Check I/O
	 */
	.align	8
.lcPL11:
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b00000000,0b00000000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0

	.long	0

	/*
	 * PL12 - No I/O interrupts; 
	 *	  No Externals;
	 *        No Machine Check I/O
	 */
	.align	8
.lcPL12:
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b00000000,0b00000000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0

	.long	0

	/*
	 * PL13 - No I/O interrupts; 
	 *	  No Externals;
	 *        No Machine Check I/O
	 */
	.align	8
.lcPL13:
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b00000000,0b00000000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0,0

	.long	0

	/*
	 * PL14 - No I/O interrupts; 
	 *	  No Externals;
	 *        No Machine Check I/O
	 */
	.align	8
.lcPL14:
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b00000000,0b00000000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0

	.long	0

	/*
	 * PL15 - No I/O interrupts; 
	 *	  No Externals;
	 *        No Machine Check I/O
	 */
	.align	8
.lcPL15:
	.long	0xffffffff
	.byte	0b11111111,0b11111111,0b00000000,0b00000000
	.long	0
	.byte	0b00000000,0b00000000,0b00000000,0b00000000

	.long	0xffffffff
	.byte	0x00000000,0,0,0
	.long	0
	.byte	0b00000000,0,0,0

	.long	0xffffffff
	.byte	0b11101111,0xff,0xff,0xff
	.long	0
	.byte	0b00000000,0,0,0

	.byte	0x05,0,0,0

	.long	0
	.section	".text"

/*====================== End of Global Variables ===================*/

#if !defined(lint)

	.section ".text"
	.align	4

#endif

	/*
	 * Berkley 4.3 introduced symbolically named interrupt levels
	 * as a way deal with priority in a machine independent fashion.
	 * Numbered priorities are machine specific, and should be
	 * discouraged where possible.
	 *
	 * Note, for the machine specific priorities there are
	 * examples listed for devices that use a particular priority.
	 * It should not be construed that all devices of that
	 * type should be at that priority.  It is currently were
	 * the current devices fit into the priority scheme based
	 * upon time criticalness.
	 *
	 * The underlying assumption of these assignments is that
	 * IPL 10 is the highest level from which a device
	 * routine can call wakeup.  Devices that interrupt from higher
	 * levels are restricted in what they can do.  If they need
	 * kernels services they should schedule a routine at a lower
	 * level (via software interrupt) to do the required
	 * processing.
	 *
	 * Also, almost all splN routines (where N is a number or a
	 * mnemonic) will do a RAISE(), on the assumption that they are
	 * never used to lower our priority.
	 * The exceptions are:
	 *	spl8()		Because you can't be above 15 to begin with!
	 *	splzs()		Because this is used at boot time to lower our
	 *			priority, to allow the PROM to poll the uart.
	 *	spl0()		Used to lower priority to 0.
	 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- spl0-spl8 / splhi / splhigh / splzs               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

int spl0(void)		{ return (0); }
int spl6(void)		{ return (0); }
int spl7(void)		{ return (0); }
int spl8(void)		{ return (0); }
int splhi(void)		{ return (0); }
int splhigh(void)	{ return (0); }
int splzs(void)		{ return (0); }
int splsclp(void)	{ return (0); }

#else	/* lint */

	/* locks out all interrupts, including memory errors */
	ENTRY(spl8)
	lghi	%r2,15
	SETPRI
	SET_SIZE(spl8)

	/* just below the level that profiling runs */
	ENTRY(spl7)
	lghi	%r2,13
	RAISE
	SET_SIZE(spl7)

	/* sun specific - highest priority onboard serial i/o zs ports */
	ENTRY(splzs)
	lghi	%r2,12
	SETPRI			/* Can't be a RAISE, as it's used to lower us */
	SET_SIZE(splzs)

	/*
	 * should lock out clocks and all interrupts,
	 * as you can see, there are exceptions
	 */
	ENTRY(splhi)
	ALTENTRY(splhigh)
	ALTENTRY(spl6)
	ALTENTRY(i_ddi_splhigh)
	lghi	%r2,DISP_LEVEL
	RAISE
	SET_SIZE(i_ddi_splhigh)
	SET_SIZE(spl6)
	SET_SIZE(splhigh)
	SET_SIZE(splhi)

	/*
	 * Allow clocks and sclp so we can write messages to HMC
	 */
	ENTRY(splsclp)
	lghi	%r2,LOCK_LEVEL-1
	RAISE
	SET_SIZE(splsclp)

	/* allow all interrupts */
	ENTRY(spl0)
	lghi	%r2,0
	SETPRI
	SET_SIZE(spl0)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- splx.                                             */
/*                                                                  */
/* Function	- Set PIL back to that indicated by the old PIL     */
/*		  passed as an argument or to the CPU's base pri-   */
/*		  ority, whichever is higher.  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
void
splx(int level)
{}

#else	/* lint */

	ENTRY(splx)
	ALTENTRY(i_ddi_splx)
	SETPRI
	SET_SIZE(i_ddi_splx)
	SET_SIZE(splx)

#endif	/* level */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- splr.                                             */
/*                                                                  */
/* Function	- splr is like splx but will only raise the prior-  */
/*		  ity and never drop it. Be careful not to set 	    */
/*		  priority lower than CPU->cpu_base_pri, even 	    */
/*		  though it seems we're raising the priority, it    */
/*		  could be set higher at any time by an interrupt   */
/*		  routine, so we must block interrupts and look at  */
/*		  CPU->cpu_base_pri.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * splr()
 *
 */

#if defined(lint)

/* ARGSUSED */
int
splr(int level)
{ return (0); }

#else	/* lint */

	ENTRY(splr)
	RAISE
	SET_SIZE(splr)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setspl.                                           */
/*                                                                  */
/* Function	- Set the interrupt mask based on a PIL.            */
/*		                               		 	    */
/* Note		- It is assumed that the caller will maintain       */
/*                the CPUs mcpu_pri value.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
int
setspl(int level)
{ return (0); }

#else	/* lint */

	ENTRY(setspl)
	stnsm	56(%r15),0x04		/* Disable I/O & Externals    */
	lg	%r3,__LC_CPU		/* Address CPU structure      */
	lgf	%r4,MCPU_PRI(%r3)	/* Get current PIL	      */
	st	%r2,MCPU_PRI(%r3)	/* Set new PIL		      */
	INTMASK				/* Enable/Disable interrupts  */
	ssm	56(%r15)		/* Restore mask               */
	lgr	%r2,%r4			/* Return old PIL	      */
	br	%r14			/* Return		      */
	SET_SIZE(setspl)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- on_fault.                                         */
/*                                                                  */
/* Function	- Catch lofault faults. Like setjmp except it 	    */
/*		  returns one if code following causes uncorrectable*/ 
/*		  fault. Turned off by calling no_fault().	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
int
on_fault(label_t *ljb)
{ return (0); }

#else	/* lint */

	ENTRY(on_fault)
	GET_THR(1)			/* Get current thread	      */
	stg	%r2,T_ONFAULT(%r1)
	larl	%r3,catch_fault
	stg	%r3,T_LOFAULT(%r1)
	j	setjmp
	
catch_fault:
	stmg	%r6,%r15,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(1)			/* Get current thread	      */

	lg	%r2,T_ONFAULT(%r1)
	lghi	%r0,0
	stg	%r0,T_ONFAULT(%r1)
	stg	%r0,T_LOFAULT(%r1)
	j	longjmp
	SET_SIZE(on_fault)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- no_fault.                                         */
/*                                                                  */
/* Function	- Turn off fault catching.                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
no_fault(void)
{}

#else	/* lint */

	ENTRY(no_fault)

	GET_THR(1)			/* Get current thread	      */

	lghi	%r0,0
	stg	%r0,T_ONFAULT(%r1)
	stg	%r0,T_LOFAULT(%r1)
	br	%r14
	SET_SIZE(no_fault)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- on_trap_trampoline.                               */
/*                                                                  */
/* Function	- Default trampoline code for on_trap() (see 	    */
/*		  <sys/ontrap.h>).  On s390x, the trap code will    */
/*		  complete trap processing but reset the return %pc */
/*		  to ot_trampoline, which will by default be set to */ 
/*		  the address of this code. 			    */
/*								    */
/*		  We longjmp(&curthread->t_ontrap->ot_jmpbuf) to    */
/*		  return back to on_trap().			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void 
on_trap_trampoline(void)
{}

#else	/* lint */

	ENTRY(on_trap_trampoline)

	GET_THR(1)			/* Get current thread	      */

	lg	%r2,T_ONTRAP(%r1)
	aghi	%r2,OT_JMPBUF
	j	longjmp
	SET_SIZE(on_trap_trampoline)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- on_trap.                                          */
/*                                                                  */
/* Function	- Push a new element on to the t_ontrap stack.      */
/*		  Refer to <sys/ontrap.h> for more information 	    */
/* 		  about the on_trap() mechanism.  If the 	    */
/*		  on_trap_data is the same as the topmost stack     */
/*		  element, we just modify that element.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
int
on_trap(on_trap_data_t *otp, uint_t prot)
{ return (0); }

#else	/* lint */

	ENTRY(on_trap)
	sthy	%r3,OT_PROT(%r2)		// ot_prot = prot
	lghi	%r0,0
	sthy	%r0,OT_TRAP(%r2)		// ot_trap = 0
	larl	%r4,on_trap_trampoline		
	stg	%r4,OT_TRAMPOLINE(%r2)		// ot_trampoline = on_trap_trampoline
	stg	%r0,OT_HANDLE(%r2)		// ot_handle = NULL
	stg	%r0,OT_PAD1(%r2)		// ot_pad1 = NULL

	GET_THR(1)				// Get current thread	      

	lg	%r5,T_ONTRAP(%r1)		// %r5 = curthread->t_ontrap
	cgr	%r5,%r4				// if (otp == on_trap_trampoline)
	je	0f				//   don't modify t_ontrap
	
	stg	%r5,OT_PREV(%r2)		// ot_prev = t_ontrap
	stg	%r2,T_ONTRAP(%r1)		// t_ontrap = otp
0:
	aghi	%r2,OT_JMPBUF
	j	setjmp
	SET_SIZE(on_trap)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setjmp.                                           */
/*                                                                  */
/* Function	- Non-local goto using state vectors type label_t.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
int
setjmp(label_t *lp)
{ return (0); }

#else	/* lint */

	ENTRY(setjmp)
	stg	%r14,L_PC(%r2)		// Save return address
	stg	%r15,L_SP(%r2)		// Save stack pointer
	lghi	%r2,0
	br	%r14
	SET_SIZE(setjmp)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- longjmp.                                          */
/*                                                                  */
/* Function	- Non-local goto using state vectors type label_t.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
void
longjmp(label_t *lp)
{}

#else	/* lint */

	ENTRY(longjmp)
	lg	%r14,L_PC(%r2)		// Restore return address
	lg	%r15,L_SP(%r2)		// Restore stack pointer
	lghi	%r2,1			// Set return value
	br	%r14			// Return
	SET_SIZE(longjmp)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getfp.                                            */
/*                                                                  */
/* Function	- Return the current frame pointer.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

greg_t
getfp(void)
{ return (0); }

#else	/* lint */

	ENTRY(getfp)
	lgr	%r2,%r15
	br	%r14
	SET_SIZE(getfp)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getpil.                                           */
/*                                                                  */
/* Function	- Get current processor interrupt level.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

u_int
getpil(void)
{ return (0); }

#else	/* lint */

	ENTRY(getpil)
	lg	%r1,__LC_CPU
	lgf	%r2,MCPU_PRI(%r1)
	br	%r14
	SET_SIZE(getpil)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setpil.                                           */
/*                                                                  */
/* Function	- Set the PIL.                                      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
void
setpil(u_int pil)
{}

#else	/* lint */

	ENTRY(setpil)
	lg	%r1,__LC_CPU
	st	%r2,MCPU_PRI(%r1)
	br	%r14
	SET_SIZE(setpil)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _insque.                                          */
/*                                                                  */
/* Function	- Insert entryp after predp in a doubly linked list.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
void
_insque(caddr_t entryp, caddr_t predp)
{}

#else	/* lint */

	ENTRY(_insque)
	lg	%r1,0(%r3)		// predp->forw
	stg	%r3,CPTRSIZE(%r2)	// entry->back = predp
	stg	%r1,0(%r2)		// entry->forw = predp->forw
	stg	%r2,0(%r3)		// predp->forw = entryp
	stg	%r2,CPTRSIZE(%r1)	// predp->forw->back = entryp
	br      %r14
	SET_SIZE(_insque)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _remque.                                          */
/*                                                                  */
/* Function	- Remove entryp from a doubly linked list.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
void
_remque(caddr_t entryp)
{}

#else	/* lint */

	ENTRY(_remque)
	lg	%r1,0(%r2)		// entryp->forw
	lg	%r3,CPTRSIZE(%r2)	// entryp->back
	stg	%r1,0(%r3)		// entryp->back->forw = entryp->forw
	stg	%r3,CPTRSIZE(%r1)	// entryp->forw->back = entryp->back
	br	%r14
	SET_SIZE(_remque)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_wait.                                         */
/*                                                                  */
/* Function	- Load an enabled wait state PSW.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
void
cpu_wait()
{}

#else	/* lint */

	ENTRY(cpu_wait)
	.global	wakePoint

	lg	%r3,__LC_CPU		// Address this CPU
	lgf	%r4,CPU_STPENDING(%r3)	// Get ST pending flag
	lgf	%r2,MCPU_PRI(%r3)	// Get PIL
	cghi	%r2,DISP_LEVEL		// Is higher level lock held
	jnl	0f			// Yes... Just wait

	ltgr	%r4,%r4			// Anything pending
	jz	0f			// No... Just wait

	lghi	%r0,T_SOFTINT		// Get soft interrupt code
	svc	255			// Issue the soft interrupt
	j	wakePoint		// Continue without waiting

0:
	lghi	%r1,0			// Value to check for
	lghi	%r2,1			// Value to set
	cs	%r1,%r2,CPU_IDLING(%r3) // Try and set "we're waiting"
	jnz	wakePoint		// If not set then we've been woken
	
	larl	%r2,waitPSW		// Point at the wait PSW
	lpswe	0(%r2)			// Enter the enabled wait state

wakePoint:
	lghi	%r1,0			// Value to check for
	st	%r1,CPU_IDLING(%r3)	// Clear "we're waiting" indicator
	br	%r14			// Return
	SET_SIZE(cpu_wait)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- caller.                                           */
/*                                                                  */
/* Function	- Return the address of who called this routine.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
void
caller()
{}

#else	/* lint */

	ENTRY(caller)
	lgr	%r2,%r14
	br	%r14
	SET_SIZE(caller)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- scanc.                                            */
/*                                                                  */
/* Function	- VAX scanc instruction.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
int
scanc(size_t length, u_char *string, u_char table[], u_char mask)
{ return (0); }

#else	/* lint */

	ENTRY(scanc)
	algr	%r2,%r3			// end = &string[length]
0:
	clgr	%r3,%r2			// ptr < end?
	jnl	1f			// No...not found
	llgc	%r1,0(%r3)		// ndx = *string;
	llgc	%r0,0(%r1,%r4)		// byte = table[ndx];
	ngr	%r0,%r5			// byte &= mask != 0?
	jnz	1f			// Yes...found
	aghi	%r3,1			// ptr++
	j	0b			// Iterate
1:
	slgr	%r2,%r3			// offset = end - ptr;
	br	%r14			// Return
	SET_SIZE(scanc)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ftrace_interrupt_disable.                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/* FIXME - What do these routines really need to do? */

ftrace_icookie_t
ftrace_interrupt_disable(void)
{ return (0); }

#else   /* __lint */

	ENTRY(ftrace_interrupt_disable)
	stnsm	48(%r15),0x04
	llgc	%r2,48(%r15)
	br	%r14
	SET_SIZE(ftrace_interrupt_disable)

#endif	/* __lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ftrace_interrupt_enable.                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
void
ftrace_interrupt_enable(ftrace_icookie_t cookie)
{}

#else	/* __lint */

	ENTRY(ftrace_interrupt_enable)
	larl	%r1,stosm
	ex	%r2,0(%r1)
	br	%r14
	SET_SIZE(ftrace_interrupt_enable)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- strlen.                                           */
/*                                                                  */
/* Function	- Returns the number of non-NULL bytes in string    */
/*		  argument.                    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
size_t
strlen(const char *str)
{ return (0); }

#else	/* lint */

	ENTRY(strlen)
	.align 	8
	lgr	%r3,%r2
	larl	%r1,strDelTab
0:	trt	0(256,%r2),0(%r1)
	jnz	1f

	aghi	%r2,256
	j	0b
	
1:
	sgr	%r1,%r3
	lgr	%r2,%r1
	br	%r14
	SET_SIZE(strlen)

	.section ".data"
	.global strDelTab
	.align	8
strDelTab:
	.quad	0xff00000000000000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.quad	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.size	strDelTab, .-strDelTab

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	- Provide a C callable interface to the membar      */
/*		  instruction.                 		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * Provide a C callable interface to the membar instruction.
 */

#if defined(lint)

void
membar_ldld(void)
{}

void
membar_stld(void)
{}

void
membar_ldst(void)
{}

void
membar_stst(void)
{}

void
membar_ldld_ldst(void)
{}

void
membar_ldld_stld(void)
{}

void
membar_ldld_stst(void)
{}

void
membar_stld_ldld(void)
{}

void
membar_stld_ldst(void)
{}

void
membar_stld_stst(void)
{}

void
membar_ldst_ldld(void)
{}

void
membar_ldst_stld(void)
{}

void
membar_ldst_stst(void)
{}

void
membar_stst_ldld(void)
{}

void
membar_stst_stld(void)
{}

void
membar_stst_ldst(void)
{}

void
membar_lookaside(void)
{}

void
membar_memissue(void)
{}

void
membar_sync(void)
{}

#else
	ENTRY(membar_ldld)
	ALTENTRY(membar_stld)
	ALTENTRY(membar_ldst)
	ALTENTRY(membar_stst)
	ALTENTRY(membar_ldld_stld)
	ALTENTRY(membar_stld_ldld)
	ALTENTRY(membar_ldld_ldst)
	ALTENTRY(membar_ldst_ldld)
	ALTENTRY(membar_ldld_stst)
	ALTENTRY(membar_stst_ldld)
	ALTENTRY(membar_ldst_stld)
	ALTENTRY(membar_stld_ldst)
	ALTENTRY(membar_stld_stst)
	ALTENTRY(membar_stst_stld)
	ALTENTRY(membar_ldst_stst)
	ALTENTRY(membar_stst_ldst)
	ALTENTRY(membar_lookaside)
	ALTENTRY(membar_memissue)
	ALTENTRY(membar_sync)
	membar	
	br	%r14
	SET_SIZE(membar_ldld)
	SET_SIZE(membar_stld)
	SET_SIZE(membar_ldst)
	SET_SIZE(membar_stst)
	SET_SIZE(membar_ldld_stld)
	SET_SIZE(membar_stld_ldld)
	SET_SIZE(membar_ldld_ldst)
	SET_SIZE(membar_ldst_ldld)
	SET_SIZE(membar_ldld_stst)
	SET_SIZE(membar_stst_ldld)
	SET_SIZE(membar_ldst_stld)
	SET_SIZE(membar_stld_ldst)
	SET_SIZE(membar_stld_stst)
	SET_SIZE(membar_stst_stld)
	SET_SIZE(membar_ldst_stst)
	SET_SIZE(membar_stst_ldst)
	SET_SIZE(membar_lookaside)
	SET_SIZE(membar_memissue)
	SET_SIZE(membar_sync)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dtrace_interrupt_disable.                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)

dtrace_icookie_t
dtrace_interrupt_disable(void)
{ return (0); }

#else	/* lint */

	ENTRY_NP(dtrace_interrupt_disable)
	stnsm	48(%r15),0x04
	llgc	%r2,48(%r15)
	br	%r14
	SET_SIZE(dtrace_interrupt_disable)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)

/*ARGSUSED*/
void
dtrace_interrupt_enable(dtrace_icookie_t cookie)
{}

#else

	ENTRY_NP(dtrace_interrupt_enable)
	larl	%r1,stosm
	ex	%r2,0(%r1)
	br	%r14
	SET_SIZE(dtrace_interrupt_enable)
	
#endif /* lint*/

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-  dtrace_membar_xxxxx                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
dtrace_membar_return(void)
{}

void
dtrace_membar_producer(void)
{}

void
dtrace_membar_consumer(void)
{}

#else	/* lint */

	ENTRY(dtrace_membar_return)
	br	%r14
	SET_SIZE(dtrace_membar_return)

	ENTRY(dtrace_membar_producer)
	membar
	br	%r14
	SET_SIZE(dtrace_membar_producer)

	ENTRY(dtrace_membar_consumer)
	membar	
	br	%r14
	SET_SIZE(dtrace_membar_consumer)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_trigger / dtrace_panic_trigger.             */
/*                                                                  */
/* Function	- A panic trigger is a word which is updated 	    */
/*		  atomically and can only be set once.  We atomic-  */
/*		  ally store -1 and load the old value.		    */
/*								    */
/*		  If the word was -1, the trigger has already been  */
/*		  activated and we fail.			    */
/*								    */
/* 		  If the previous value was 0, we succeed.  This    */
/*		  allows a partially corrupt trigger to still 	    */
/*		  trigger correctly.  DTrace has its own version of */
/*		  this function to allow it to panic correctly from */
/*		  probe context.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
int panic_trigger(int *tp) { return (0); }

/*ARGSUSED*/
int dtrace_panic_trigger(int *tp) { return (0); }

#else	/* lint */

	ENTRY_NP(panic_trigger)
	lghi	%r1,-1
	lghi	%r0,0
	lgr	%r3,%r0
	cs	%r0,%r1,0(%r2)
	jnz	1f	
	lghi	%r3,1
1:
	lgr	%r2,%r3
	br	%r14
	SET_SIZE(panic_trigger)

	ENTRY_NP(dtrace_panic_trigger)
	lghi	%r1,-1
	lghi	%r0,0
	lgr 	%r3,%r0
	cs	%r0,%r1,0(%r2)
	jnz	1f
	lghi	%r3,1
1:
	lgr	%r2,%r3
	br	%r14
	SET_SIZE(dtrace_panic_trigger)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- vpanic / dtrace_vpanic.                           */
/*                                                                  */
/* Function	- The panic() and cmn_err() functions invoke 	    */
/*		  vpanic() as a common entry point into the panic   */
/*		  code implemented in panicsys(). vpanic() is 	    */
/*		  responsible for passing through the format string */
/*		  and arguments, and constructing a regs structure  */
/* 		  on the stack into which it saves the current 	    */
/*		  register values.  If we are not dying due to a    */
/* 		  fatal trap, these registers will then be preserved*/ 
/*		  in panicbuf as the current processor state.  	    */
/*		  Before invoking panicsys(), vpanic() activates the*/ 
/*		  first panic trigger (see common/os/panic.c) and   */
/*		  switches to the panic_stack if successful.  Note  */
/*		  that DTrace takes a slightly different panic path */
/*		  if it must panic from probe context.  Instead of  */
/*		  calling panic, it calls into dtrace_vpanic(),     */
/*		  which sets up the initial stack as vpanic does,   */
/*		  calls dtrace_panic_trigger(), and branches back   */
/*		  into vpanic().				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
void vpanic(const char *format, va_list alist) {}

/*ARGSUSED*/
void dtrace_vpanic(const char *format, va_list alist) {}

#else	/* lint */

	ENTRY_NP(vpanic)
	stmg	%r6,%r15,48(%r15)
	aghi	%r15,-SA(MINFRAME + _REGSIZE)
	stmg	%r0,%r14,MINFRAME(%r15)
	lgr	%r11,%r15
	stg	%r11,SA(MINFRAME + _REGSIZE)(%r15)
	lgr	%r7,%r2
	lgr	%r6,%r3
	aghi	%r11,SA(MINFRAME + _REGSIZE)
	stg	%r11,MINFRAME+_REGSIZE-8(%r15)
	lgr	%r11,%r15
	brasl	%r14,dumpCPU
	larl	%r2,panic_quiesce
	bras	%r14,panic_trigger
	ltgr	%r2,%r2
	jz	0f

	larl	%r2,panic_stack
	lghi	%r0,PANICSTKSIZE
	agr	%r2,%r0
	lgr	%r15,%r2
	aghi	%r15,-SA(MINFRAME + _REGSIZE)

0:
	la	%r0,MINFRAME(%r11)		// Copy REG struct
	la	%r2,MINFRAME(%r15)
	lghi	%r1,_REGSIZE
	lgr	%r3,%r1
	mvcl	%r2,%r0

	larl	%r5,panic_stack
	la	%r4,MINFRAME(%r15)
	lgr	%r3,%r6
	lgr	%r2,%r7
	brasl	%r14,panicsys
	lgr	%r15,%r11
	aghi	%r15,SA(MINFRAME + _REGSIZE)
	lmg	%r6,%r15,48(%r15)
	br	%r14
	SET_SIZE(vpanic)

	ENTRY_NP(dtrace_vpanic)
	stmg	%r6,%r15,48(%r15)
	aghi	%r15,-SA(MINFRAME + _REGSIZE)
	stmg	%r0,%r14,MINFRAME(%r15)
	lgr	%r11,%r15
	stg	%r11,SA(MINFRAME + _REGSIZE)(%r15)
	lgr	%r7,%r2
	lgr	%r6,%r3
	aghi	%r11,SA(MINFRAME + _REGSIZE)
	stg	%r11,MINFRAME+_REGSIZE-8(%r15)
	lgr	%r11,%r15
	brasl	%r14,dumpCPU
	larl	%r2,panic_quiesce
	bras	%r14,dtrace_panic_trigger
	ltgr	%r2,%r2
	jz	0f

	larl	%r2,panic_stack
	lghi	%r0,PANICSTKSIZE
	agr	%r2,%r0
	lgr	%r15,%r2
	aghi	%r15,-SA(MINFRAME + _REGSIZE)

0:
	la	%r0,MINFRAME(%r11)		// Copy REG struct
	la	%r2,MINFRAME(%r15)
	lghi	%r1,_REGSIZE
	lgr	%r3,%r1
	mvcl	%r2,%r1

	larl	%r5,panic_stack
	la	%r4,MINFRAME(%r15)
	lgr	%r3,%r6
	lgr	%r2,%r7
	brasl	%r14,panicsys
	lgr	%r15,%r11
	aghi	%r15,SA(MINFRAME + _REGSIZE)
	lmg	%r6,%r15,48(%r15)
	br	%r14
	SET_SIZE(dtrace_vpanic)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- fuwordxx.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
int
fuword64(const void *addr, uint64_t *dst)
{ return (0); }

/*ARGSUSED*/
int
fuword32(const void *addr, uint32_t *dst)
{ return (0); }

/*ARGSUSED*/
int
fuword16(const void *addr, uint16_t *dst)
{ return (0); }

/*ARGSUSED*/
int
fuword8(const void *addr, uint8_t *dst)
{ return (0); }

/*ARGSUSED*/
int
dtrace_ft_fuword64(const void *addr, uint64_t *dst)
{ return (0); }

/*ARGSUSED*/
int
dtrace_ft_fuword32(const void *addr, uint32_t *dst)
{ return (0); }

#else	/* lint */

	FUWORD(fuword64, lg, stg, CP_FUWORD64)
	FUWORD(fuword32, lgf, st, CP_FUWORD32)
	FUWORD(fuword16, llgh, sth, CP_FUWORD16)
	FUWORD(fuword8, lb, stc, CP_FUWORD8)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- suwordxx.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
int
suword64(void *addr, uint64_t value)
{ return (0); }

/*ARGSUSED*/
int
suword32(void *addr, uint32_t value)
{ return (0); }

/*ARGSUSED*/
int
suword16(void *addr, uint16_t value)
{ return (0); }

/*ARGSUSED*/
int
suword8(void *addr, uint8_t value)
{ return (0); }

#else	/* lint */

	SUWORD(suword64, stg, CP_SUWORD64)
	SUWORD(suword32, st, CP_SUWORD32)
	SUWORD(suword16, sth, CP_SUWORD16)
	SUWORD(suword8, stc, CP_SUWORD8)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- fuwordxx_noerr.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
void
fuword8_noerr(const void *addr, uint8_t *dst)
{}

/*ARGSUSED*/
void
fuword16_noerr(const void *addr, uint16_t *dst)
{}

/*ARGSUSED*/
void
fuword32_noerr(const void *addr, uint32_t *dst)
{}

/*ARGSUSED*/
void
fuword64_noerr(const void *addr, uint64_t *dst)
{}

#else	/* lint */

	ENTRY(fuword8_noerr)
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS
	ic	%r2,0(%r2)
	sacf	AC_PRIMARY
	stc	%r2,0(%r3)
	br	%r14
	SET_SIZE(fuword8_noerr)

	ENTRY(fuword16_noerr)
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS
	llgh	%r2,0(%r2)
	sacf	AC_PRIMARY
	sth	%r2,0(%r3)
	br	%r14
	SET_SIZE(fuword16_noerr)

	ENTRY(fuword32_noerr)
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS
	lgf	%r2,0(%r2)
	sacf	AC_PRIMARY
	st	%r2,0(%r3)
	br	%r14
	SET_SIZE(fuword32_noerr)

	ENTRY(fuword64_noerr)
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS
	lg	%r2,0(%r2)
	sacf	AC_PRIMARY
	stg	%r2,0(%r3)
	br	%r14
	SET_SIZE(fuword64_noerr)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- suwordxx_noerr.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/*ARGSUSED*/
void
suword8_noerr(void *addr, uint8_t value)
{}

/*ARGSUSED*/
void
suword16_noerr(void *addr, uint16_t value)
{}

/*ARGSUSED*/
void
suword32_noerr(void *addr, uint32_t value)
{}

/*ARGSUSED*/
void
suword64_noerr(void *addr, uint64_t value)
{}

#else	/* lint */

	ENTRY(suword8_noerr)
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS
	stc	%r3,0(%r2)
	sacf	AC_PRIMARY
	br	%r14
	SET_SIZE(suword8_noerr)

	ENTRY(suword16_noerr)
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS
	sth	%r3,0(%r2)
	sacf	AC_PRIMARY
	br	%r14
	SET_SIZE(suword16_noerr)

	ENTRY(suword32_noerr)
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS
	st	%r3,0(%r2)
	sacf	AC_PRIMARY
	br	%r14
	SET_SIZE(suword32_noerr)

	ENTRY(suword64_noerr)
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS
	stg	%r3,0(%r2)
	sacf	AC_PRIMARY
	br	%r14
	SET_SIZE(suword64_noerr)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- subytexx / fulwordxx / sulwordxx.                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
int
subyte(void *addr, uchar_t value)
{ return (0); }

/*ARGSUSED*/
void
subyte_noerr(void *addr, uchar_t value)
{}

/*ARGSUSED*/
int
fulword(const void *addr, ulong_t *valuep)
{ return (0); }

/*ARGSUSED*/
void
fulword_noerr(const void *addr, ulong_t *valuep)
{}

/*ARGSUSED*/
int
sulword(void *addr, ulong_t valuep)
{ return (0); }

/*ARGSUSED*/
void
sulword_noerr(void *addr, ulong_t valuep)
{}

#else

	.weak	subyte
	subyte=suword8
	.weak	subyte_noerr
	subyte_noerr=suword8_noerr
	.weak	fulword
	fulword=fuword64
	.weak	fulword_noerr
	fulword_noerr=fuword64_noerr
	.weak	sulword
	sulword=suword64
	.weak	sulword_noerr
	sulword_noerr=suword64_noerr

#endif	/* lint */


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prefetch_smap_w.                                  */
/*                                                                  */
/* Function	- Prefetch ahead within a linear list of smap 	    */
/*		  structures.					    */
/*                                                                  */
/* 		  Not implemented for s390x.  Compatibility stub.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
void prefetch_smap_w(void *smp)
{}

#else	/* __lint */

	ENTRY(prefetch_smap_w)
	br	%r14
	SET_SIZE(prefetch_smap_w)

#endif	/* __lint */


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prefetch_page_r.                                  */
/*                                                                  */
/* Function	- Issue prefetch instructions for a page_t.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
void
prefetch_page_r(void *pp)
{}

#else	/* __lint */

	ENTRY(prefetch_page_r)
	br	%r14
	SET_SIZE(prefetch_page_r)

#endif	/* __lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kmdb_enter.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)
void
kmdb_enter(void)
{
}

#else	/* lint */

	ENTRY_NP(kmdb_enter)
	lghi	%r0,ST_KMDB_TRAP
	svc	255
	br	%r14
	SET_SIZE(kmdb_enter)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sigsoftint.                                       */
/*                                                                  */
/* Function	- Signal a soft interrupt.                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)
void
sigsoftint(void)
{
}

#else	/* lint */

	ENTRY_NP(sigsoftint)
	lghi	%r0,T_SOFTINT
	svc	255
	br	%r14
	SET_SIZE(sigsoftint)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sti.                                              */
/*                                                                  */
/* Function	- Enable interrupts.                                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)
void
sti(void)
{
}

#else	/* lint */

	ENTRY_NP(sti)
	stosm	48(%r15),0x03
	br	%r14
	SET_SIZE(sti)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cli.                                              */
/*                                                                  */
/* Function	- Disable interrupts.                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)
void
cli(void)
{
}

#else	/* lint */

	ENTRY_NP(cli)
	stnsm	48(%r15),0xfc
	br	%r14
	SET_SIZE(cli)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_restore.                                     */
/*                                                                  */
/* Function	- Restore interrupt mask.                           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)
int
intr_restore(int)
{
}

#else	/* lint */

	ENTRY_NP(intr_restore)
	stnsm	48(%r15),0xfc		/* Clear current mask		*/
	llgc	%r3,48(%r15)		/* Remember original mask	*/
	larl	%r1,stosm		/* Point at STOSM instruction	*/
	ex	%r2,0(%r1)		/* Set the system mask		*/
	lgr	%r2,%r3			/* Return original mask		*/
	br	%r14			/* Return			*/
	SET_SIZE(intr_restore)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_clear.                                       */
/*                                                                  */
/* Function	- Clear interrupt mask.                             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)
int
intr_clear(void)
{
}

#else	/* lint */

	ENTRY_NP(intr_clear)
	stnsm	48(%r15),0xfc
	llgc	%r2,48(%r15)
	br	%r14
	SET_SIZE(intr_clear)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_enable.                                      */
/*                                                                  */
/* Function	- Enable maskable interrupts.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint) || defined(__lint)
int
intr_enable(void)
{
}

#else	/* lint */

	ENTRY_NP(intr_enable)
	stosm	48(%r15),0x03
	llgc	%r2,48(%r15)
	br	%r14
	SET_SIZE(intr_enable)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- switch_sp_and_call.                               */
/*                                                                  */
/* Function	- Switch the stack pointer and call a function.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
void
switch_sp_and_call(void *newsp, void (*func)(uint_t, uint_t), uint_t arg1, uint_t arg2)
{}

#else	/* __lint */

	ENTRY_NP(switch_sp_and_call)
	stmg	%r13,%r15,104(%r15)	// Save important registers
	lgr	%r13,%r15		// Save stack pointer
	stg	%r15,0(%r13)		// Save backchain pointer
	lgr	%r15,%r2		// Set new stack pointer
	aghi	%r15,-SA(MINFRAME)	// New stack pointer
	stg	%r13,0(%r15)		// Save backchain pointer
	lgr	%r1,%r3			// Copy function address
	lgr	%r2,%r4			// Set arg1 pointer
	lgr	%r3,%r5			// Set arg2 pointer
	basr	%r14,%r1		// Call function
	lmg	%r13,%r15,104(%r13)	// Restore registers
	br	%r14			// Return
	SET_SIZE(switch_sp_and_call)

#endif	/* __lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- frogr.                                            */
/*                                                                  */
/* Function	- Find the rightmost one bit in a 16-bit field.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
int
frogr(uint16_t word)
{}

#else	/* __lint */

	ENTRY_NP(frogr)
	lghi	%r0,16			// Initialize index
0:
	tmll	%r2,1			// Found our first 'on' bit?
	jo	1f			// Yes... Go return result

	aghi	%r0,-1			// Decrement the index count
	jnp	1f			// Yep... Exit

	srlg	%r2,%r2,1		// Shuffle down one bit
	j	0b			// Test again

1:
	lghi	%r2,16			// Initialize index
	sgr	%r2,%r0			// Determine value
	br	%r14			// Return
	SET_SIZE(frogr)
	
#endif	/* __lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- flogr.                                            */
/*                                                                  */
/* Function	- Find the leftmost one bit in a 16-bit field.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
int
flogr(uint16_t word)
{}

#else	/* __lint */

# if 0				// Z9 only - FIXME when we get one!

	ENTRY_NP(flogr)
	sllg	%r2,%r2,48		// We're only interested in last halfword
	flogr	%r2,%r3			// Find left most one
	jz	0f			// None found

	lghi	%r0,15			// Get range
	srg	%r0,%r3			// Make it lowest bit is 0
	lgr	%r2,%r0			// Copy to result register
	j	1f
0:
	lghi	%r2,0			// Set none found
1:
	br	%r14			// Return
	SET_SIZE(flogr)

# else

	ENTRY_NP(flogr)
	lghi	%r0,15			// Initialize index
	sllg	%r2,%r2,48		// We're only interested in last halfword
0:
	ltgr	%r2,%r2			// One bit found?
	jm	1f			// Yes... Go return result

	aghi	%r0,-1			// Decrement the index count
	jm	1f			// Yep... Exit

	sllg	%r2,%r2,1		// Shuffle up one bit
	j	0b			// Test again

1:
	lgr	%r2,%r0			// Set result register
	br	%r14			// Return
	SET_SIZE(flogr)

# endif
	
#endif	/* __lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- atomic_btr32.                                     */
/*                                                                  */
/* Function	- Perform a locked test.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
int
atomic_btr32(uint32_t *word, value)
{}

#else	/* __lint */

	ENTRY(atomic_btr32)
	lghi	%r0,1			// Form bitstring with ...
	sllg	%r0,%r0,0(%r3)		// ... the pil'th bit on 
	lgr	%r5,%r0			// Copy
	lghi	%r1,-1			// Form a bit string ...
	xgr	%r1,%r0			// ... that will turn ...
	lgr	%r3,%r1			// ... the pil'th bit off
0:
	lgf	%r0,0(%r2)		// Get current value
	lgf	%r1,0(%r2)		// ....
	lgr	%r4,%r1			// Copy
	ngr  	%r1,%r3			// Form string with bit off
	cs	%r0,%r1,0(%r2)		// Atomically set 
	jnz	0b			// Try again if changed

	ngr	%r4,%r5			// Test if that bit was on
	lgr	%r2,%r4			// Use that as return code
	br	%r14
	SET_SIZE(atomic_btr32)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- curthread.                                        */
/*                                                                  */
/* Function	- Return current thread pointer.                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
kthread_t *
threadp(void)
{}

#else	/* __lint */

	ENTRY(threadp)
	
	GET_THR(2)

	br	%r14
	SET_SIZE(threadp)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ticks2tod.					    */
/*                                                                  */
/* Function	- Convert ticks to TOD value by converting to TOD   */
/*		  units then adjusting from epoch and accounting    */
/*		  for time since boot. The result is a value of a   */
/*		  time in the future that can be used in the clock  */
/*		  comparator.                  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
uint64_t
ticks2tod(hrtime_t time)
{}

#else	/* __lint */

	ENTRY(ticks2tod)
	lghi	%r5,0			// Clear for multiply
	lgr	%r4,%r2			// Get ticks
	lgfi	%r3,NANO2TICK		// Nano to ticks conversion factor
	mlgr	%r4,%r3			// Convert from ticks to nanosecs
	lgr	%r2,%r5			// Get nanos

	NANO2TOD			// Convert to tod clock value

	larl	%r1,boottime		// Address boottime constant
	alg     %r2,0(%r1)		// Adjust from time since boot to tod
	br	%r14
	SET_SIZE(ticks2tod)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- nano2tod. 					    */
/*                                                                  */
/* Function	- Convert an interval in nanoseconds to a TOD value */
/*		  based on the current time. The result is a value  */
/*		  in the future that can be used in the clock  	    */
/*		  comparator.                  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
uint64_t
nano2tod(hrtime_t time)
{}

#else	/* __lint */

	ENTRY(nano2tod)

	NANO2TOD			// Convert nanos to tod clock value

	larl	%r1,boottime		// Address boottime constant
	alg     %r2,0(%r1)		// Adjust from time since boot to tod
	br	%r14
	SET_SIZE(nano2tod)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- tod2nano. 					    */
/*                                                                  */
/* Function	- Convert a time in TOD format to a value in	    */
/*		  nanoseconds.               			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
hrtime_t
tod2nano(void)
{}

#else	/* __lint */

	ENTRY(tod2nano)
	stck	48(%r15)		// Get current TOD
	lg	%r2,48(%r15)		// ....

	larl	%r1,todAdjust		// Get adjustment factor
	alg	%r2,0(%r1)		// Adjust to our base

	TOD2NANO			// Convert tod to nanoseconds

	br	%r14			// And return it
	SET_SIZE(tod2nano)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- tod2ticks. 					    */
/*                                                                  */
/* Function	- Convert TOD to ticks. Get the current time, adjust*/
/*		  for the epoch, factor in the time since boot, then*/
/*		  convert to nanoseconds before converting to the   */
/*		  tick value used by Solaris. Store this value in   */
/*		  lbolt to mark the current time.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
hrtime_t
tod2ticks(void)
{}

#else	/* __lint */

	ENTRY(tod2ticks)
	stck	48(%r15)		// Get current TOD
	lg	%r2,48(%r15)		// ....

	larl	%r1,boottime		// Get adjustment factor
	slg	%r2,0(%r1)		// Adjust to our base

	TOD2NANO			// Convert TOD to nanos

	lgfi	%r3,NANO2TICK		// Get conversion factor
	lgr	%r1,%r2			// Get nanos
	lghi	%r0,0			// Clear
	dlgr	%r0,%r3			// Convert to ticks
	srlg	%r3,%r3,1		// Get half of factor
	sgr	%r0,%r3			// Do we need to round up?
	jm	1f			// Nope... Skip

	aghi	%r1,1			// Round up
1:
	lgr	%r2,%r1			// Return current time in ticks

#if 0 	// S390X FIXME
	lg	%r5,c_lbolt64-c_tod2tick(%r3)	// Get *lbolt
	lg	%r4,c_lbolt-c_tod2tick(%r3)	// Get *lbolt
	ltgr	%r4,%r4			// Check that address has been resolved
	jz	2f			// No... Too early in boot process

	stg	%r2,0(%r4)		// Set new lbolt value
	stg	%r2,0(%r5)		// Set new lbolt64 value
2:
#endif
	lghi	%r5,100			// Get ticks to seconds factor
	dsgr	%r0,%r5			// Convert ticks to seconds

	larl	%r3,lastSec		// Get addr of lastSec
	cg	%r1,0(%r3)		// Reached a new second yet?
	je	3f			// No, skip

	stg	%r1,0(%r3)		// Save to lastSec
	lghi	%r1,1			// Get flag
	larl	%r3,one_sec		// Get addr of one_sec
	st	%r1,0(%r3)		// And set
3:
	br	%r14
	SET_SIZE(tod2ticks)

	.section ".data"
lastSec:
	.quad	0
	.section ".text"
#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- gettick_counter.				    */
/*                                                                  */
/* Function	- Convert TOD to nanoseconds only adjusting for boot*/
/*		  time as this will give us the value in nanoseconds*/
/*		  since we started the system up.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(__lint)

/*ARGSUSED*/
hrtime_t
gettick_counter(void)
{}

#else	/* __lint */

	ENTRY(gettick_counter)
	stck	48(%r15)		// Get current TOD
	lg	%r2,48(%r15)		// ....

	larl	%r1,boottime		// Get adjustment factor
	slg	%r2,0(%r1)		// Adjust to our base

	TOD2NANO			// Convert TOD to nanos (in %r2)

	br	%r14
	SET_SIZE(tod2ticks)

#endif

/*========================= End of Function ========================*/
