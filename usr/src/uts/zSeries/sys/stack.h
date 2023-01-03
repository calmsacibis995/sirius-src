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

#ifndef _SYS_STACK_H
#define	_SYS_STACK_H

#if !defined(_ASM)

#include <sys/types.h>

#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * The 64-bit C ABI uses a stack frame that looks like:
 *
 *	0		back chain (a 0 here signifies end of back chain) 
 *	8		eos (end of stack, not used on Solaris)
 *	16		glue used in other linkage formats
 *	24		glue used in other linkage formats
 *	32		scratch area 
 *	40		scratch area 
 *	48		saved r6 of caller function 
 *	56		saved r7 of caller function 
 *	64		saved r8 of caller function 
 *	72		saved r9 of caller function 
 *	80		saved r10 of caller function 
 *	88		saved r11 of caller function 
 *	96		saved r12 of caller function 
 *	104		saved r13 of caller function 
 *	112		saved r14 of caller function 
 *	120		saved r15 of caller function 
 *	128		saved f4 of caller function 
 *	136		saved f6 of caller function 
 *	160		outgoing args passed from caller to callee 
 *	160+x		possible stack alignment (8 bytes desirable) 
 *	160+x+y		alloca space of caller (if used) 
 *	160+x+y+z	automatics of caller (if used) 
 */

/*
 * The 32-bit C ABI uses a stack frame that looks like:
 *
 *	0		back chain (a 0 here signifies end of back chain) 
 *	4		eos (end of stack, not used on Solaris)
 *	8		glue used in other linkage formats
 *	12		glue used in other linkage formats
 *	16		scratch area 
 *	20		scratch area 
 *	24		saved r6 of caller function 
 *	28		saved r7 of caller function 
 *	32		saved r8 of caller function 
 *	36		saved r9 of caller function 
 *	40		saved r10 of caller function 
 *	44		saved r11 of caller function 
 *	48		saved r12 of caller function 
 *	52 		saved r13 of caller function 
 *	56 		saved r14 of caller function 
 *	60 		saved r15 of caller function 
 *	64 		saved f4 of caller function 
 *	68 		saved f6 of caller function 
 *	96 		outgoing args passed from caller to callee 
 *	96+x		possible stack alignment (8 bytes desirable) 
 *	96+x+y		alloca space of caller (if used) 
 *	96+x+y+z	automatics of caller (if used) 
 */
#ifndef _ASM

typedef struct _s390xstk {
	void	 *st_bc;	// Back chain
	void	 *st_eos;	// End-of-stack (not used)
	uint64_t st_regs[14];	// R2-R15 save area
	void	 *st_args;	// Start of stack arguments
} s390xstk;

typedef struct _s390xstk32 {
	uint32_t *st32_bc;	// Back chain
	uint32_t *st32_eos;	// End-of-stack (not used)
	uint32_t st32_regs[14];	// R2-R15 save area
	uint32_t *st32_args;	// Start of stack arguments
} s390xstk32;

#endif

/*
 * Constants defining a stack frame.
 */

#define STACK_ALIGN		8
#define	STACK_ENTRY_ALIGN	8
#define	STACK_BIAS		0
#define FPSAVESZ		136
#define FPFPC			128

#define	MINFRAME32		96
#define STACK_REGS32		24
#define	SA32(X)			(((X)+(STACK_ALIGN-1)) & ~(STACK_ALIGN-1))
#define	STACK_ALIGN32		STACK_ALIGN
#define	STACK_ENTRY_ALIGN32	STACK_ENTRY_ALIGN
#define	STACK_BIAS32		STACK_BIAS

#define	MINFRAME64		160
#define STACK_REGS64		48
#define	SA64(X)			(((X)+(STACK_ALIGN-1)) & ~(STACK_ALIGN-1))
#define	STACK_ALIGN64		STACK_ALIGN
#define	STACK_ENTRY_ALIGN64	STACK_ENTRY_ALIGN
#define	STACK_BIAS64		STACK_BIAS

#if defined(__s390x)
#define	MINFRAME		MINFRAME64
#define	SA(X)			SA64(X)
#define	STACK_REGS		STACK_REGS64
#else
#define	MINFRAME		MINFRAME32
#define	SA(X)			SA32(X)
#define	STACK_REGS		STACK_REGS32
#endif

#if defined(_KERNEL) && !defined(_ASM)

#if defined(DEBUG)
#if STACK_ALIGN == 8
#define	ASSERT_STACK_ALIGNED()						\
	{								\
		uint64_t __tmp;						\
		ASSERT((((uintptr_t)&__tmp) & (STACK_ALIGN - 1)) == 0);	\
	}
#elif (STACK_ALIGN == 16) && (_LONG_DOUBLE_ALIGNMENT == 16)
#define	ASSERT_STACK_ALIGNED()						\
	{								\
		long double __tmp;					\
		ASSERT((((uintptr_t)&__tmp) & (STACK_ALIGN - 1)) == 0);	\
	}
#endif
#else	/* DEBUG */
#define	ASSERT_STACK_ALIGNED()
#endif	/* DEBUG */

struct regs;

void flush_windows(void);
void flush_user_windows(void);
int  flush_user_windows_to_stack(caddr_t *);
void trash_user_windows(void);
void traceregs(struct regs *);
void traceback(caddr_t);

#endif	/* defined(_KERNEL) && !defined(_ASM) */

#define STACK_GROWTH_DOWN /* stacks grow from high to low addresses */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_STACK_H */
