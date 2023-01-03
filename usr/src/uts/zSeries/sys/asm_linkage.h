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

#ifndef _SYS_ASM_LINKAGE_H
#define	_SYS_ASM_LINKAGE_H

#include <sys/stack.h>
#include <sys/trap.h>
#include <sys/machs390x.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef _ASM	/* The remainder of this file is only for assembly files */

/*
 * These constants can be used to compute offsets into pointer arrays.
 */
#ifdef __s390x
# define	CPTRSHIFT	3
# define	CLONGSHIFT	3
#else
# define	CPTRSHIFT	2
# define	CLONGSHIFT	2
#endif
#define	CPTRSIZE	(1<<CPTRSHIFT)
#define	CLONGSIZE	(1<<CLONGSHIFT)
#define	CPTRMASK	(CPTRSIZE - 1)
#define	CLONGMASK	(CLONGSIZE - 1)

/*
 * Symbolic section definitions.
 */
#define	RODATA	".rodata"

/*
 * First Level Interrupt Handler macros
 */

/*
 * Offsets need to match layout of mcontext_t
 */

#define KSTK_PSW	 0x0
#define KSTK_REGS	 0x10
#define KSTK_R0 	 0x10
#define KSTK_R1		 0x18
#define KSTK_R2		 0x20
#define KSTK_R3		 0x28
#define KSTK_R4		 0x30
#define KSTK_R5		 0x38
#define KSTK_R6		 0x40
#define KSTK_R7		 0x48
#define KSTK_R8		 0x50
#define KSTK_R9		 0x58
#define KSTK_R10	 0x60
#define KSTK_R11	 0x68
#define KSTK_R12	 0x70
#define KSTK_R13	 0x78
#define KSTK_R14	 0x80
#define KSTK_R15	 0x88
#define KSTK_PIL	 0x90
#define KSTK_ILC	 0x94
#define KSTK_AREGS	 0x98
#define KSTK_FREGS	 0xd8
#define KSTK_FPC	 0x158
#define KSTK_CREGS	 0x160
#define KSTK_FRAME	 0x1e0 
#define KSTKSIZE	0x2000

#define F_PSW_PROB	0x01		
	
	.macro KENTER oldpsw,ilc,sync
	.if	\sync
	stpt	__LC_TIMER_SYN_END		// Save CPU timer
	stg	%r15,__LC_SYN_SAVE_AREA		// Save work register
	.else
	stpt	__LC_TIMER_ASY_END		// Save CPU timer
	stg	%r15,__LC_ASY_SAVE_AREA		// Save work register
	.endif
	tm	\oldpsw+1,F_PSW_PROB		// Problem state?
	jz	1f				// No... We have a stack ptr 

	.if	\sync
	lg	%r15,__LC_TIMER_SYN_END		// Get finish time
	.else
	lg	%r15,__LC_TIMER_ASY_END		// Get finish time
	.endif
	slg	%r15,__LC_TIMER_START		// Determine interval
	alg	%r15,__LC_TIMER_USER 		// Update total time
	stg	%r15,__LC_TIMER_USER 		// Save
	
	lg	%r15,__LC_TIMER_LAST 		// Get system time
	slg	%r15,__LC_TIMER_START   	// Get finish time
	alg	%r15,__LC_TIMER_SYS  		// Update total time
	stg	%r15,__LC_TIMER_SYS  		// Save

	lg	%r15,__LC_CPU      		// Get CPU pointer
	lg	%r15,CPU_THREAD(%r15)		// Get thread pointer
	lg	%r15,T_STACK(%r15)		// Get stack pointer

1:
	aghi    %r15,-SA(KSTK_FRAME)		// Ensure room for context data
	stmg	%r0,%r14,KSTK_REGS(%r15)	// Save General registers
	stam	%a0,%a15,KSTK_AREGS(%r15)	// Save Access Registers
	std	%f0,KSTK_FREGS(%r15)		// Save FP regs
	std	%f1,KSTK_FREGS+0x08(%r15)
	std	%f2,KSTK_FREGS+0x10(%r15)	
	std	%f3,KSTK_FREGS+0x18(%r15)	
	std	%f4,KSTK_FREGS+0x20(%r15)	
	std	%f5,KSTK_FREGS+0x28(%r15)	
	std	%f6,KSTK_FREGS+0x30(%r15)	
	std	%f7,KSTK_FREGS+0x38(%r15)	
	std	%f8,KSTK_FREGS+0x40(%r15)	
	std	%f9,KSTK_FREGS+0x48(%r15)	
	std	%f10,KSTK_FREGS+0x50(%r15)	
	std	%f11,KSTK_FREGS+0x58(%r15)	
	std	%f12,KSTK_FREGS+0x60(%r15)	
	std	%f13,KSTK_FREGS+0x68(%r15)	
	std	%f14,KSTK_FREGS+0x70(%r15)	
	std	%f15,KSTK_FREGS+0x78(%r15)	
	stfpc	KSTK_FPC(%r15)
	stctg	%c0,%c15,KSTK_CREGS(%r15)	// Save Control Registers
	lg	%r2,__LC_CPU			// Get CPU 
	mvc	KSTK_PSW(16,%r15),\oldpsw 	// Put old psw on stack
	.if	\sync
	mvc	KSTK_R15(8,%r15),__LC_SYN_SAVE_AREA 	// Retrieve original R15
	.else
	mvc	KSTK_R15(8,%r15),__LC_ASY_SAVE_AREA 	// Retrieve original R15
	.endif
	.if	\ilc
	lgb	%r1,\ilc			// Get ILC
	st	%r1,KSTK_ILC(%r15)
	.endif
	lgf	%r1,MCPU_PRI(%r2)		// Get current PIL
	st	%r1,KSTK_PIL(%r15)		// Save PIL
	lgr	%r14,%r15			// Save sp
	aghi    %r15,-SA(MINFRAME)		// Set new stack frame
	stg	%r14,0(%r15)			// Set backchain
	.endm

/*------------------------------------------------------------------*/
/* We leave an interrupt handler by - 				    */
/* - Disabling ALL interrupts so we can restore registers okay	    */
/* - Set the PIL as it was before the interrupt			    */
/* - Put the old PSW in the RUN PSW field			    */
/* - Reload Control, Access, and General registers		    */
/* - Store the processor timer					    */
/* - Reload the PSW we came in on (with wait bit turned off)	    */
/*------------------------------------------------------------------*/
	.macro KLEAVE
	stnsm	__LC_SCRATCH,0x04			// Disable interrupts
	aghi    %r15,SA(MINFRAME)			// Restore stack pointer
	mvc	__LC_RUN_PSW(16,0),KSTK_PSW(%r15)	// Restore PSW
	ni	__LC_RUN_PSW+1,0xfd			// Unset wait bit
	lam	%a0,%a15,KSTK_AREGS(%r15)		// Restore Access Registers
	lmg	%r0,%r15,KSTK_REGS(%r15)		// Restore General Registers
	stpt	__LC_TIMER_START
	lpswe	__LC_RUN_PSW
	.endm
	
/*------------------------------------------------------------------*/
/* Acquire the current thread pointer by getting the value from the */
/* current CPU structure. To be MP-safe we use the PLO instruction  */
/* which uses a few work registers. As this function is used in many*/
/* different places we use a macro that will save the work registers*/
/* and reload them at completion.				    */
/*------------------------------------------------------------------*/
	.macro GETTHREAD r
	stmg	%r0,%r3,16(%r15)	// Save work registers
	lghi	%r0,2			// Get function code
0:							
	la	%r1,__LC_CPU		// Use the A(CPU) as our token
	lg	%r2,__LC_CPU		// Get current CPU 
	plo	%r2,0(%r1),%r3,CPU_THREAD(%r2)  // Ensure we get the correct ptr
	jnz	0b			// Try again
	.ifc	\r,0		
		lgr	%r0,%r3		
		lmg	%r1,%r3,24(%r15)
	.else
		.ifc	\r,1
			lgr	%r1,%r3		
			lg	%r0,16(%r15)
			lmg	%r2,%r3,32(%r15)
		.else
			.ifc	\r,2
				lgr	%r2,%r3		
				lmg	%r0,%r1,16(%r15)
				lg	%r3,40(%r15)
			.else
				.ifc	\r,3
					lmg	%r0,%r2,16(%r15)
				.else
					lgr	%r\r,%r3
					lmg	%r0,%r3,16(%r15)
				.endif
			.endif
		.endif
	.endif
	.endm
	
/*
 * profiling causes defintions of the MCOUNT and RTMCOUNT
 * particular to the type
 */
#ifdef GPROF

#define	MCOUNT_SIZE	(32)
#define	MCOUNT(x)			\
	stmg	%r6,%r11,48(%r15);	\
	aghi	%r15, -SA(MINFRAME);	\
	larl	%r2,x;			\
	brasl	%r14,_mcount;		\
	aghi	%r15,SA(MINFRAME);	\
	lmg	%r6,%r11,48(%r15)

#endif /* GPROF */

#ifdef PROF

#define	MCOUNT_SIZE	(32)		/* Not sure if all this needs to be different */
#define	MCOUNT(x) 			\
	stmg	%r6,%r11,48(%r15);	\
	aghi	%r15, -SA(MINFRAME);	\
	larl	%r2,x;			\
	brasl	%r14,_mcount;		\
	aghi	%r15,SA(MINFRAME);	\
	lmg	%r6,%r11,48(%r15)

#endif /* PROF */

/*
 * if we are not profiling, MCOUNT should be defined to nothing
 */
#if !defined(PROF) && !defined(GPROF)
#define	MCOUNT_SIZE	0	/* no instructions inserted */
#define	MCOUNT(x)
#endif /* !defined(PROF) && !defined(GPROF) */

#define	RTMCOUNT(x)	MCOUNT(x)

/*
 * Macro to define weak symbol aliases. These are similar to the ANSI-C
 *	#pragma weak name = _name
 * except a compiler can determine type. The assembler must be told. Hence,
 * the second parameter must be the type of the symbol (i.e.: function,...)
 */
#define	ANSI_PRAGMA_WEAK(sym, stype)	\
	.equ	_##sym, sym;		\
	.weak	sym; 			\
	.global _##sym;			\
	.type	_##sym, @stype

/*
 * Like ANSI_PRAGMA_WEAK(), but for unrelated names, as in:
 *	#pragma weak sym1 = sym2
 */
#define	ANSI_PRAGMA_WEAK2(sym1, sym2, stype)	\
	.weak	sym1; 				\
	.type	sym1, @stype; 			\
	.equ	sym1, sym2

/*
 * ENTRY provides the standard procedure entry code and an easy way to
 * insert the calls to mcount for profiling. ENTRY_NP is identical, but
 * never calls mcount.
 */
#define	ENTRY(x)		 	\
	.section	".text"; 	\
	.align	8;			\
	.global	x;			\
	.type	x, @function;		\
x:	MCOUNT(x)

#define	ENTRY_SIZE	MCOUNT_SIZE

#define	ENTRY_NP(x)			\
	.section	".text";	\
	.align	8;			\
	.global	x;			\
	.type	x, @function;		\
x:

#define	RTENTRY(x)			\
	.section	".text";	\
	.align	8;			\
	.global	x;			\
	.type	x, @function;		\
x:	RTMCOUNT(x)

/*
 * ENTRY2 is identical to ENTRY but provides two labels for the entry point.
 */
#define	ENTRY2(x, y)			\
	.section	".text";	\
	.align	8;			\
	.global	x, y;			\
	.type	x, @function;		\
	.type	y, @function;		\
/* CSTYLED */				\
x:	;				\
y:	MCOUNT(x)

#define	ENTRY_NP2(x, y)			\
	.section	".text";	\
	.align	8;			\
	.global	x, y;			\
	.type	x, @function;		\
	.type	y, @function;		\
/* CSTYLED */				\
x:	;				\
y:


/*
 * ALTENTRY provides for additional entry points.
 */
#define	ALTENTRY(x)			\
	.global x;			\
	.type	x, @function;		\
x:

/*
 * DGDEF and DGDEF2 provide global data declarations.
 *
 * DGDEF provides a word aligned word of storage.
 *
 * DGDEF2 allocates "sz" bytes of storage with **NO** alignment.  This
 * implies this macro is best used for byte arrays.
 *
 * DGDEF3 allocates "sz" bytes of storage with "algn" alignment.
 */
#define	DGDEF2(name, sz)		\
	.section	".data";	\
	.global name;			\
	.type	name, @object;		\
	.size	name, sz;		\
name:

#define	DGDEF3(name, sz, algn)		\
	.section	".data";	\
	.align	algn;			\
	.global name;			\
	.type	name, @object;		\
	.size	name, sz;		\
name:

#define	DGDEF(name)	DGDEF3(name, 4, 4)

/*
 * SET_SIZE trails a function and set the size for the ELF symbol table.
 */
#define	SET_SIZE(x)			\
	.size	x, (.-x)

# ifdef _KERNEL
/*
 * Extract the current thread pointer - only when interrupts are disabled
 *
 */
#define LOAD_THR(r)						\
	lg	r,__LC_CPU;					\
	lg	r,CPU_THREAD(r);

/*
 * Extract the current thread pointer - MP safe
 *
 * We use the PLO instruction to ensure we aren't moved from
 * CPU to CPU during the load of the thread pointer from the
 * CPU structure.
 */
#define GET_THR(r)						\
	GETTHREAD r

/*
 * Store the current thread pointer into __LC_CPU->CPU_THREAD 
 *
 * - Interrupts must be disabled
 *
 */
#define PUT_THR(r)						\
	lg	1,__LC_CPU;					\
	stg	r,CPU_THREAD(1);

/*
 * Set the current thread pointer - MP safe
 *
 * We use the PLO instruction to ensure we aren't moved from
 * CPU to CPU during the set of the thread pointer from the
 * CPU structure.
 */
#define SET_THR(r)						\
	lgr	2,r;						\
	brasl	14,set_threadp;

# else

#  ifdef __s390x
/*
 * Extract the current thread pointer from AR0-1
 *
 */
#define GET_THR(r)						\
	ear	r,%a0;						\
	sllg	r,r,32;						\
	ear	r,%a1;						

/*
 * Store the current thread pointer into AR0-1
 *
 */
#define SET_THR(r)						\
	lgr	0,r;						\
	sar	%a1,0;						\
	srlg	0,0,32;						\
	sar	%a0,0;						

#  else
/*
 * Extract the current thread pointer from AR0
 *
 */
#define GET_THR(r)						\
	ear	r,%a0;						\

/*
 * Store the current thread pointer into AR0
 *
 */
#define SET_THR(r)						\
	sar	%a0,r;

#  endif /* __s390x */

# endif /* KERNEL */

#endif /* _ASM */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ASM_LINKAGE_H */
