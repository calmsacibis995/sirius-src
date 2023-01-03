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
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	All Rights Reserved	*/


/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_SYS_REGSET_H
#define	_SYS_REGSET_H

#include <sys/feature_tests.h>

#if !defined(_ASM)
# include <sys/types.h>
# include <sys/int_types.h>
# include <sys/privregs.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Location of the users' stored registers relative to R0.
 * Usage is as an index into a gregset_t array or as u.u_ar0[XX].
 */
#if !defined(_XPG4_2) || defined(__EXTENSIONS__)

#define	REG_G0	(0)
#define	REG_G1	(1)
#define	REG_G2	(2)
#define	REG_G3	(3)
#define	REG_G4	(4)
#define	REG_G5	(5)
#define	REG_G6	(6)
#define	REG_G7	(7)
#define	REG_G8	(8)
#define	REG_G9	(9)
#define	REG_G10	(10)
#define	REG_G11	(11)
#define	REG_G12	(12)
#define	REG_G13	(13)
#define	REG_G14	(14)
#define	REG_G15	(15)
#define	REG_ILC (16)

#define	REG_F0	(0)
#define	REG_F1	(1)
#define	REG_F2	(2)
#define	REG_F3	(3)
#define	REG_F4	(4)
#define	REG_F5	(5)
#define	REG_F6	(6)
#define	REG_F7	(7)
#define	REG_F8	(8)
#define	REG_F9	(9)
#define	REG_F10	(10)
#define	REG_F11	(11)
#define	REG_F12	(12)
#define	REG_F13	(13)
#define	REG_F14	(14)
#define	REG_F15	(15)

#define	REG_A0	(0)
#define	REG_A1	(1)
#define	REG_A2	(2)
#define	REG_A3	(3)
#define	REG_A4	(4)
#define	REG_A5	(5)
#define	REG_A6	(6)
#define	REG_A7	(7)
#define	REG_A8	(8)
#define	REG_A9	(9)
#define	REG_A10	(10)
#define	REG_A11	(11)
#define	REG_A12	(12)
#define	REG_A13	(13)
#define	REG_A14	(14)
#define	REG_A15	(15)

/* the following defines are for portability */
#define	REG_SP	REG_G15

#endif
/*
 * A gregset_t is defined as an array type for compatibility with the reference
 * source. This is important due to differences in the way the C language
 * treats arrays and structures as parameters.
 *
 */
#define	_NGREG	17
#define	_NAREG	16
#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
# define	NGREG	_NGREG
#endif

#ifndef	_ASM

#define USERMODE(psw)	(((pswg_t *) psw)->prob != 0)

/*------------------------------------------------------*/
/* Program Status Word...				*/
/*------------------------------------------------------*/
typedef struct _pswg_t {
	struct _mask {
		uchar_t	fill_1	:1;
		uchar_t	per	:1;	/* PER enabled flag		*/
		uchar_t	fill_2	:3;
		uchar_t	dat	:1;	/* DAT enabled flag		*/
		uchar_t	io	:1;	/* I/O interrupts flag		*/
		uchar_t	ext	:1;	/* External interrupts		*/
	} mask;
	uchar_t	key	:4;		/* Protection key		*/
	uchar_t	fill_3	:1;		
	uchar_t	mc	:1;		/* Machine check flag		*/
	uchar_t	wait	:1;		/* Wait state			*/
	uchar_t	prob	:1;		/* Problem state		*/

	uchar_t	as	:2;		/* Address space control	*/
	uchar_t	cc	:2;		/* Condition code		*/
	uchar_t	pmask	:4;		/* Program mask			*/

	uchar_t	fill_4	:7;		
	uchar_t	ea	:1;		/* Extended addressing		*/

	uchar_t	ba	:1;		/* Basic addressing		*/
	uchar_t	fill_5	:7;

	uchar_t	fill_6[3];

	uint64_t pc;			/* Instruction address		*/
} pswg_t;

typedef long	 greg_t;
typedef greg_t 	 gregset_t[_NGREG];
typedef uint32_t aregset_t[_NAREG];

#ifdef _SYSCALL32

typedef int	 greg32_t;
typedef greg32_t gregset32_t[_NGREG];

#endif

#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
/*
 * Floating point definitions.
 */

/*
 * struct fpu is the floating point processor state. struct fpu is the sum
 * total of all possible floating point state which includes the state of
 * external floating point hardware, fpa registers, etc..., if it exists.
 *
 * A floating point instuction queue may or may not be associated with
 * the floating point processor state. If a queue does exist, the field
 * fpu_q will point to an array of fpu_qcnt entries where each entry is
 * fpu_q_entrysize long. fpu_q_entry has a lower bound of sizeof (union FQu)
 * and no upper bound. If no floating point queue entries are associated
 * with the processor state, fpu_qcnt will be zeo and fpu_q will be NULL.
 */

/*
 * The following #define's are obsolete and may be removed in a future release.
 * The corresponding integer types should be used instead (i.e. uint64_t).
 */
#define	FPU_REGS_TYPE		uint32_t
#define	FPU_DREGS_TYPE		uint64_t

typedef struct _fpregset_t {
	union {					/* FPU floating point regs */
		uint32_t	fe[16];		/* 16 singles */
		double		fd[16];		/* 16 doubles */
		long double	fx[8];		/* 8 quads */
	} fr;
	uint32_t	fpc;    		/* FP Control Register */
} fpregset_t;

/*
 * The ABI uses struct fpu, so we use this to describe the kernel's view of the
 * fpu.
 */
typedef struct {
	union _fpu_fr {				/* FPU floating point regs */
		uint32_t	fe[16];		/* 16 singles */
		double		fd[16];		/* 16 doubles */
		long double	fx[8];		/* 8 quads */
	} fr;
	uint32_t	fpc;    		/* FP Control Register */
} kfpu_t;

#endif /* !defined(_XPG4_2) || defined(__EXTENSIONS__) */

# if !defined(_XPG4_2) || defined(__EXTENSIONS__)
/*
 * Structure mcontext defines the complete hardware machine state. 
 *
 */
typedef struct {
	pswg_t		psw;	/* Program Status Word */
	gregset_t	gregs;	/* General Register Set */
	aregset_t	aregs;	/* Access Register Set */
	fpregset_t	fpregs;	/* Floating point register set */
} mcontext_t;

#  ifdef _SYSCALL32

typedef struct {
	pswg_t		psw;	/* Program Status Word */
	gregset32_t	gregs;	/* General Register Set */
	aregset_t	aregs;	/* Access Register Set */
	fpregset_t	fpregs;	/* Floating point register set */
} mcontext32_t;

#  endif

# endif /* !defined(_XPG4_2) || defined(__EXTENSIONS__) */
#endif	/* _ASM */

/*
 * The following is here for XPG4.2 standards compliance.
 * regset.h is included in ucontext.h for the definition of
 * mcontext_t, all of which breaks XPG4.2 namespace.
 */

# if defined(_XPG4_2) && !defined(__EXTENSIONS__)
/*
 * The following is here for UNIX 95 compliance (XPG Issue 4, Version 2
 * System Interfaces and Headers. The structures included here are identical
 * to those visible elsewhere in this header except that the structure
 * element names have been changed in accordance with the X/Open namespace
 * rules.  Specifically, depending on the name and scope, the names have
 * been prepended with a single or double underscore (_ or __).  See the
 * structure definitions in the non-X/Open namespace for more detailed
 * comments describing each of these structures.
 */

#  ifndef	_ASM

/*
 * The following #define's are obsolete and may be removed in a future release.
 * The corresponding integer types should be used instead (i.e. uint64_t).
 */
#define	_FPU_REGS_TYPE		uint32_t
#define	_FPU_DREGS_TYPE		uint64_t

typedef struct _fpregset_t {
	union {					/* FPU floating point regs */
		uint32_t	fe[16];		/* 16 singles */
		double		fd[16];		/* 16 doubles */
		long double	fx[8];		/* 8 quads */
	} fr;
	uint32_t	fpc;    		/* FP Control Register */
} fpregset_t;

/*
 * Structure mcontext defines the complete hardware machine state.
 */
typedef struct {
	pswg_t		psw;	/* Program Status Word */
	gregset_t	gregs;	/* General Register Set */
	aregset_t	aregs;	/* Access Register Set */
	fpregset_t	fpregs;	/* Floating point register set */
} mcontext_t;

#  endif	/* _ASM */
# endif /* defined(_XPG4_2) && !defined(__EXTENSIONS__) */


#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_REGSET_H */
