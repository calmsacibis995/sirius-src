/*------------------------------------------------------------------*/
/* 								    */
/* Name        - kobj_ipl.s   	 				    */
/* 								    */
/* Function    - Low-core definitions and boot entry point.         */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - December, 2006					    */
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
_kipl(void)
{}

#endif

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

#if !defined(lint)

	.section 	".boot_stack"
	.global	bootStack
	.global	stackTop
	.global	kStack
	.global	kStackTop
bootStack:
	.skip	16384
stackTop:
	.size	bootStack, 16384

kStack:
	.skip	4096
	.size	kStack, 4096
kStackTop:
	.section	".data"

	.global availmem
availmem:
	 .quad	0
	.size	availmem, 8

	.global highmem
highmem:
	 .quad	0
	.size	highmem, 8

	.align	8
	.global	sysMemory
sysMemory:
	.quad	memoryChunks
	.size	sysMemory, .-sysMemory
#define NUM_CHUNKS 10
	.global memoryChunks
memoryChunks:
	.skip	NUM_CHUNKS*3*8
	.size	memoryChunks, .-memoryChunks

	.global	nMemChunk
nMemChunk:
	.word	0
	.size	nMemChunk, .-nMemChunk

	.align	8
.segName:	
	.byte	0xe2,0xc9,0xd9,0xc9,0xe4,0xe2,0x40,0x40
	.size	.segName, .-.segName

	.align	8
	.global zvmData, zvmUsr
zvmData:
	.skip	8
zvmEnv:	.short	0		// z/VM Execution environment
zvmVId: .byte	0		// Version information
zvmVCd:	.byte	0		// Version code
	.skip  	2		// Reserved
zvmPrc:	.short 	0		// Processor address
zvmUsr: .quad	0		// User id
zvmMap:	.quad	0		// Licensed program bitmap
zvmTdf:	.long	0		// Time zone differential
zvmRel:	.long	0		// Release information
	.size	zvmData, .-zvmData
#define SZVMDT	44

	.align	8
	.global	bootScratch
bootScratch:
	.quad	0
	.size	bootScratch, .-bootScratch

	.global	bootScratchEnd
bootScratchEnd:
	.quad	0
	.size	bootScratch, .-bootScratchEnd

	.align	8
.clkInit:
	.quad	-1
	.quad	0x7fffffffffffffff
	.size	.clkInit, .-.clkInit

	.align	4096
	.global stsiInfo
stsiInfo:
	.skip	4096
	.size	stsiInfo, .-stsiInfo

#endif	/* lint */

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _kipl.                                            */
/*                                                                  */
/* Function	- Entry point for boot initialization. The IPL      */
/*		  mechanism will pass control to this point.        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if !defined(lint)

	.section	".text"
	.align	8
	.text
/*
 * Boot initialization
 */
	.global _boot,_unix,_elfsz,_ramdk,_eramd
_boot:
_unix: 	.quad	0
_elfsz: .quad   0
_ramdk:	.quad	0
_eramd:	.quad	0
	.global	_kipl
	.type	_kipl, @function
_kipl:

	//
	// Stash away our arguments in memory.
	//
	stm	%r0,%r15,__LC_PARMREG

	//
	// Get straight into zArchitecture mode
	//
	lhi	%r1,1
	slr	%r0,%r0
	sigp	%r1,%r0,0x12
	sam64	

	//
	// Address the constants
	//
	basr	%r13,0
.Lccons:
	j	.Lcskip
	.align	8
	//
.Lcr:	// Control Register Settings
	//
	.quad	0x0000000004b54202	// CR0:  All sorts of flags
	.quad	0			// CR1:  Primary space specifications
	.quad	.Lductape		// CR2:  Dispatchable Unit Control Table
	.quad	0			// CR3:  Instruction authorization/Address spaces
	.quad	0			// CR4:  Instruction authorization/Address spaces
	.quad	0x0			// CR5:  Primary-aste origin
	.quad	0			// CR6:  I/O interrupts
	.quad	0			// CR7:  Secondary space specifications
	.quad	0			// CR8:  Access register translation/Monitor
	.quad	0			// CR9:  Tracing - event related
	.quad	0			// CR10: Tracing - start address
	.quad	0			// CR11: Tracing - end address
	.quad	0			// CR12: Tracing related
	.quad	0			// CR13: Home space specifications
	.quad	0x00000000DD000000	// CR14: Machine check related/ASN data
	.quad	0			// CR15: Linkage stack

.Lductape:
	.long	0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0

.Lqsc: 	.long	0x00020000,0x80000000
	.quad	0x0fff
.Lpsw: 	.long	0x00000000,0x80000000
	.quad	boot_die
.Lext:	.long	0x00000000,0x80000000
	.quad	ext_flih
.LpswT:	.long	0x00000001,0x80000000
	.quad	.LnxtChk
.Lcskip:
	//
	// Initialize kernel stack
	//
	larl	%r15,kStackTop
	stg	%r15,__LC_KSTACK

	//
	// Initialize boot stack
	//
	larl	%r15,stackTop
	aghi	%r15,-SA(MINFRAME)

	//
	// Initialize CPU and curthread pointers
	//
	larl	%r0,t0
	larl	%r1,cpu0
	stg	%r0,CPU_THREAD(%r1)
	stg	%r1,__LC_CPU

	//
	// Initialize the clock comparator and CPU timer
	//
	larl	%r1,.clkInit
	sckc	0(%r1)
	spt	8(%r1)

	// Get user information
	//
	larl	%r1,zvmData		// Point at area for DIAG 0x00
	lghi	%r0,SZVMDT		// Get length of area
	sam31
	DIAG	1,0,0x00		// Get the user data
	sam64

	//
	// Load the ramdisk
	//
	larl	%r3,_eramd		// Address end of ramdisk
	larl	%r2,_ramdk		// Get A(Start of ramdisk)
	lg	%r0,0(%r3)		// Get current value
	ltgr	%r0,%r0			// Has it been set?
	jnz	.gotRd			// Yes... Skip load

	larl	%r1,zvmUsr 		// Look for a segment id = username
	lghi	%r0,4			// Set "Load Non Shared" option
	sam31
	DIAG	1,0,0x64		// Load the segment
	sam64
	jnh	.gotNSS			// If loaded... Skip

	larl	%r1,.segName		// Get A(Segment Name)
	lghi	%r0,4			// Set "Load Non Shared" option
	sam31	
	DIAG	1,0,0x64		// Load a segment with id = 'SIRIUS'
	sam64
	jh	boot_die		// No RAMDISK? The die...

.gotNSS:
	stg	%r1,0(%r2)		// Save start of ramdisk
	aghi	%r0,1			// r0 is last usable byte, bump past
	stg	%r0,0(%r3)		// Save end of ramdisk

	//
	// Mark ramdisk pages as "modified/referenced"
	//
.gotRd:	
	lghi	%r4,6			// Modified + Referenced
.rdLoop:
	cgr	%r1,%r0			// End of ramdisk?
	jnl	.doneRd			// Yes... done

	sske	%r4,%r1			// Set the storage key
	aghi	%r1,4096        	// Next page
	j	.rdLoop			// Go do it
	
	//
	// Establish bootScratch/bootScratchEnd based on ramdisk
	//
.doneRd:
	larl	%r2,bootScratch		// Address bootScratch
	lg  	%r0,0(%r3)		// Get A(End of Ramdisk)
	stg	%r0,0(%r2)		// Set bootScratch
	lghi	%r1,8			//
	sllg	%r1,%r1,20		// Specify end as +8M
	agr	%r1,%r0			//
	stg	%r1,8(%r2)		// Set bootScratchEnd

	//
	// Get system information
	//
	larl	%r2,stsiInfo		// Address the SYSIB
	lghi	%r0,1			// Get function code
	sllg	%r0,%r0,28		// Shuffle up
	aghi	%r0,1			// Set selector 1
	lghi	%r1,1			// Set selector 2
	stsi	0(%r2)			// Store subsystem information

	//
	// Clear bss
	//
	larl	%r0,__bss_start		// Get A(Start of BSS)
	larl	%r1,_end		// Get A(End of BSS)
	sgr	%r1,%r0			// Determine length
	lghi	%r2,0			// Clear source ptr
	lgr	%r3,%r2			// Set length and pad
	mvcl	%r0,%r2			// Clear BSS
	
	// Determine available memory
	//
	//	If we have discontiguous memory configuration
	//	then after DIAG 260 r1 != r0. In this case 
	//	we'll need to use a TPROT loop between r1 and
	//	r2 to determine where the holes are
	//
	larl	%r2,availmem
	lghi	%r0,0x0c
	DIAG	1,0,0x260	
	stg	%r1,0(%r2)
	stg	%r0,8(%r2)
	cgr	%r1,%r0			// Contiguous?
	jne	.LchkChk		// No... Go check chunks

	larl	%r7,memoryChunks	// Point at chunk area
	larl	%r5,nMemChunk		// Point at chunk count
	lghi	%r2,0			// Starting address is 0
	stg	%r2,0(%r7)		// Save address
	aghi	%r1,1			// Get length
	stg	%r1,8(%r7)		// Save length
	lghi	%r1,0			// Get type
	stg	%r1,16(%r7)		// Set
	lghi	%r1,1			// Get chunk count
	st	%r1,0(%r5)		// Set chunk count
	j	.LskipChk

	//	We can now go through blocks of storage 
	//	and see what "chunks" of memory we have
	//
.LchkChk:
	mvc	__LC_PGM_NEW_PSW(16,%r0),.LpswT-.Lccons(%r13)
	lghi	%r6,0			// Set start address
	stg	%r6,0(%r2)		// Reset available mem
	lgr 	%r4,%r6			// Set current chunk size
	lgr 	%r8,%r6			// Set access code
	lgr 	%r9,%r6			// Set chunk count
	larl	%r7,memoryChunks	// Point at chunk area
	lghi	%r1,1024		// Scan in 1M
	msr	%r1,%r1			// Increments
	lghi	%r10,NUM_CHUNKS		// Set max. chunk count

.LmemChk:
	clg	%r6,8(%r2)		// Reached end of storage?
	jl	.LtstPrt		// No... Go test this block

	lghi	%r10,0			// Set for exit
	j	.LnxtChk

.LtstPrt:
	tprot	0(%r6),0		// Test storage
	ipm	%r0			// Get condition code
	srl	%r0,28			// Shuffle on down
	lgr	%r11,%r6		// Set last chunk checked
	clr	%r0,%r8			// Has access code changed?
	jne	.LnxtChk		// Yes... Register new chunk

	algr	%r6,%r1			// Update top of chunk
	j	.LmemChk				

.LnxtChk:
	clgr	%r4,%r6			// Zero sized chunk?
	je	.LmemLoop		// Yes... Skip

	stg	%r4,0(%r7)		// Save chunk start
	lgr	%r3,%r6			// Copy current address
	slgr	%r3,%r4			// Determine length
	stg	%r3,8(%r7)		// Save chunk length
	alg	%r3,0(%r2)		// Accumulate available
	stg	%r3,0(%r2)		// Save available mem
	stg	%r0,16(%r7)		// Save chunk type
	aghi	%r7,24			// Bump to next chunk area
	aghi	%r9,1			// Bump chunk count
	lgr	%r4,%r6			// Set start of new chunk

.LmemLoop:
	clgr	%r11,%r6		// Did trap on last check?
	jnl	.LmemNext		// No... Just go check next

	algr	%r6,%r1			// Yes... Update chunk pointer
	lgr	%r4,%r6			// Set start of new chunk

.LmemNext:
	lgr	%r8,%r0			// Set current access code
	ltgr	%r10,%r10		// Enough for more chunks?
	jnz	.LmemChk		// Yes... Go do it

	larl	%r8,nMemChunk		// Get A(Chunk Count)
	st	%r9,0(%r8)		// Save
	
.LskipChk:
	//
	// Plug in PSWs
	//
	mvc	__LC_EXT_NEW_PSW(16,%r0),.Lext-.Lccons(%r13)
	mvc	__LC_IO_NEW_PSW(16,%r0),.Lpsw-.Lccons(%r13)
	mvc	__LC_MC_NEW_PSW(16,%r0),.Lpsw-.Lccons(%r13)
	mvc	__LC_PGM_NEW_PSW(16,%r0),.Lpsw-.Lccons(%r13)
	mvc	__LC_RST_NEW_PSW(16,%r0),.Lpsw-.Lccons(%r13)
	mvc	__LC_SVC_NEW_PSW(16,%r0),.Lpsw-.Lccons(%r13)

	//
	// Initialize CPU state registers
	//
	lctlg	%c0,%c15,.Lcr-.Lccons(%r13)
                                                                                                                                   
	//
	// Enable external interrupts to allow Signal-service interrupts.
	// This will provide early use of the SCLP write/read functions.
	//
	stosm   48(%r15),0x01

	//
	// Call __kipl_setup to get things ready before calling _kobj_boot
	//
	larl	%r3,availmem
	larl	%r5,nMemChunk
	larl	%r2,_boot
	larl	%r4,memoryChunks
	lg	%r3,0(%r3)
	lgf	%r5,0(%r5)
	brasl	%r14,_kipl_setup

	//
	// It should never return
	//
	.global	boot_die
boot_die:
	larl	%r13,.Lqsc
	lpswe	0(%r13)

	SET_SIZE(_kipl)

/*========================= End of Function ========================*/

#endif	/* lint */
