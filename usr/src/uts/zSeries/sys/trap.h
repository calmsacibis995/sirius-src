/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License                  
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *                                                             
 * Copyright 2008 Sine Nomine Associates.                       
 * All rights reserved.                                          
 * Use is subject to license terms.                               
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_SYS_TRAP_H
#define	_SYS_TRAP_H

#ifdef	__cplusplus
extern "C" {
#endif


/*
 * This file is machine specific as is.
 */

/*
 * Program Exception Interruption Codes
 */
#define PXC_OPR		0x0001	// Operation
#define PXC_PRV		0x0002	// Privileged-operation
#define PXC_EXC		0x0003	// Execute
#define PXC_PRT		0x0004	// Protection
#define PXC_ADR		0x0005	// Addressing
#define PXC_SPC		0x0006	// Specification
#define PXC_DTA		0x0007	// Data
#define PXC_FOV		0x0008	// Fixed-point-overflow
#define PXC_FPD		0x0009	// Fixed-point-divide
#define PXC_DOV		0x000A	// Decimal-overflow
#define PXC_DDV		0x000B	// Decimal-divide
#define PXC_HEO		0x000C	// HFP-exponent-overflow
#define PXC_HEU		0x000D	// HFP-exponent-underflow
#define PXC_HES		0x000E	// HFP-significance
#define PXC_HFD		0x000F	// HFP-floating-point-divide
#define PXC_SGT		0x0010	// Segment-translation
#define PXC_PGT		0x0011	// Page-translation
#define PXC_TRS		0x0012	// Translation-specification
#define PXC_SOP		0x0013	// Special-operation
#define PXC_OPN		0x0015	// Operand
#define PXC_TRT		0x0016	// Trace-table
#define PXC_SSE		0x001C	// Space-switch event
#define PXC_HSR		0x001D	// HFP-square-root
#define PXC_PCT		0x001F	// PC-translation-specification
#define PXC_AFX		0x0020	// AFX-translation
#define PXC_ASX		0x0021	// ASX-translation
#define PXC_LXT		0x0022	// LX-translation
#define PXC_EXT		0x0023	// EX-translation
#define PXC_PRA		0x0024	// Primary-authority
#define PXC_SCA		0x0025	// Secondary-authority
#define PXC_ALT		0x0028	// ALET-specification
#define PXC_ALN		0x0029	// ALEN-translation
#define PXC_ALE		0x002A	// ALE-sequence
#define PXC_ASV		0x002B	// ASTE-validity
#define PXC_ASS		0x002C	// ASTE-sequence
#define PXC_EXA		0x002D	// Extended-authority
#define PXC_STF		0x0030	// Stack-full
#define PXC_STE		0x0031	// Stack-empty
#define PXC_STS		0x0032	// Stack-specification
#define PXC_STT		0x0033	// Stack-type
#define PXC_STO		0x0034	// Stack-operation
#define PXC_ASC		0x0038	// ASCE-type
#define PXC_RFT		0x0039	// Region-first-translation
#define PXC_RST		0x003A	// Region-second-translation
#define PXC_RTT		0x003B	// Region-third-translation
#define PXC_MON		0x0040	// Monitor event
#define PXC_PER		0x0080	// PER event
#define PXC_CRY		0x0119	// Crypto-operation


/*
 * Data exception subtypes
 */
#define DX_DEC		0x00	// Decimal data exception
#define DX_AFP		0x01	// AFP Register
#define DX_BFP		0x02	// BFP instruction
#define DX_DFP		0x03	// DFP instruction
#define DX_IAT		0x08	// IEEE inexact and truncated
#define DX_SIT		0x0b	// Simulated IEEE inexact and truncated
#define DX_IAI		0x0c	// IEEE inexact and incremented
#define DX_UEX		0x10	// IEEE underflow and exact
#define DX_SUX		0x13	// Simulated IEEE underflow and exact
#define DX_UIT		0x18	// IEEE underflow, inexact, and truncated
#define DX_SUT		0x1b	// Simulated IEEE underflow, inexact, and truncated
#define DX_UII		0x1c	// IEEE underflow, inexact, and incremented
#define DX_OVE		0x20	// IEEE overflow and exact
#define DX_SOV		0x23	// Simulated IEEE overflow and exact
#define DX_OIT		0x28	// IEEE overflow, inexact, and truncated
#define DX_SOT		0x2b	// Simulated IEEE overflow, inexact, and truncated
#define DX_OII		0x2c	// IEEE overflow, inexact, and incremented
#define DX_DBZ		0x40	// IEEE division by zero
#define DX_SDZ		0x43	// Simulated IEEE division by zero
#define DX_INV		0x80	// IEEE invalid operation
#define DX_SIN		0x83	// Simulated IEEE invalid operation

/*
 * Software traps
 */
#define	ST_OSYSCALL		0x00
#define	ST_BREAKPOINT		0x01
#define	ST_DIV0			0x02
#define	ST_FLUSH_WINDOWS	0x03
#define	ST_CLEAN_WINDOWS	0x04
#define	ST_RANGE_CHECK		0x05
#define	ST_FIX_ALIGN		0x06
#define	ST_INT_OVERFLOW		0x07
#define	ST_SYSCALL		0x08

/*
 * Software trap vectors 16 - 31 are reserved for use by the user
 * and will not be usurped by Sun.
 */

#define	ST_GETCC		0x20
#define	ST_SETCC		0x21
#define	ST_GETPSR		0x22
#define	ST_SETPSR		0x23
#define	ST_GETHRTIME		0x24
#define	ST_GETHRVTIME		0x25
#define	ST_SELFXCALL		0x26
#define	ST_GETHRESTIME		0x27
#define	ST_SETV9STACK		0x28
#define	ST_GETLGRP		0x29

/*
 * DTrace traps used for user-land tracing.
 */
#define	ST_DTRACE_PID		0x38
#define	ST_DTRACE_PROBE		0x39
#define	ST_DTRACE_RETURN	0x3a

#define	ST_KMDB_TRAP		0x7d
#define	ST_KMDB_BREAKPOINT	0x7e
#define	ST_MON_BREAKPOINT	0x7f

/*
 * Trap type values
 */

#define	T_ZERODIV	0x0	/* #de	divide by 0 error		*/
#define	T_SGLSTP	0x1	/* #db	single step			*/
#define	T_NMIFLT	0x2	/* 	NMI				*/
#define	T_BPTFLT	0x3	/* #bp	breakpoint fault, INT3 insn	*/
#define	T_OVFLW		0x4	/* #of	INTO overflow fault		*/
#define	T_BOUNDFLT	0x5	/* #br	BOUND insn fault		*/
#define	T_ILLINST	0x6	/* #ud	invalid opcode fault		*/
#define	T_NOEXTFLT	0x7	/* #nm	device not available: x87	*/
#define	T_DBLFLT	0x8	/* #df	double fault			*/
#define	T_EXTOVRFLT	0x9	/* 	[not generated: 386 only]	*/
#define	T_TSSFLT	0xa	/* #ts	invalid TSS fault		*/
#define	T_SEGFLT	0xb	/* #np	segment not present fault	*/
#define	T_STKFLT	0xc	/* #ss	stack fault			*/
#define	T_GPFLT		0xd	/* #gp	general protection fault	*/
#define	T_PGFLT		0xe	/* #pf	page fault			*/
#define	T_EXTERRFLT	0x10	/* #mf	x87 FPU error fault		*/
#define	T_ALIGNMENT	0x11	/* #ac	alignment check error		*/
#define	T_MCE		0x12	/* #mc	machine check exception		*/
#define	T_SIMDFPE	0x13	/* #xm	SSE/SSE exception		*/
#define	T_DBGENTR	0x14	/*	debugger entry 			*/
#define	T_ENDPERR	0x21	/*	emulated extension error flt	*/
#define	T_ENOEXTFLT	0x20	/*	emulated ext not present	*/
#define	T_FASTTRAP	0xd2	/*	fast system call		*/
#define	T_SYSCALLINT	0x91	/*	general system call		*/
#define	T_SOFTINT	0x50fd	/*	pseudo softint trap type	*/

/*
 * Pseudo traps.
 */
#define	T_INTERRUPT		0x100
#define	T_FAULT			0x200
#define	T_AST			0x400
#define	T_SYSCALL		0x180


#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_TRAP_H */
