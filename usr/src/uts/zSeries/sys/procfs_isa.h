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
 * Copyright (c) 1996-1998 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SYS_PROCFS_ISA_H
#define	_SYS_PROCFS_ISA_H

/*
 * Instruction Set Architecture specific component of <sys/procfs.h>
 */

#include <sys/regset.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Possible values of pr_dmodel.
 * This isn't isa-specific, but it needs to be defined here for other reasons.
 */
#define	PR_MODEL_UNKNOWN 0
#define	PR_MODEL_ILP32	1	/* process data model is ILP32 */
#define	PR_MODEL_LP64	2	/* process data model is LP64 */

/*
 * To determine whether application is running native.
 */
#define	PR_MODEL_NATIVE	PR_MODEL_LP64

/*
 * Holds one s390x instruction, for both ILP32 and LP64.
 */
typedef	uint32_t	instr_t;

/*
 * General register access (sparc).
 * Don't confuse definitions here with definitions in <sys/regset.h>.
 * Registers are 32 bits for ILP32, 64 bits for LP64.
 * The floating point registers follow the access registers. For convenience
 * we define prfpregset with an array indexed via R_Yx
 */
#define R_PSWM	(0)
#define R_PSWA	(1)
#define	R_G0	(2)
#define	R_G1	(3)
#define	R_G2	(4)
#define	R_G3	(5)
#define	R_G4	(6)
#define	R_G5	(7)
#define	R_G6	(8)
#define	R_G7	(9)
#define	R_G8	(10)
#define	R_G9	(11)
#define	R_G10	(12)
#define	R_G11	(13)
#define	R_G12	(14)
#define	R_G13	(15)
#define	R_G14	(16)
#define	R_G15	(17)
#define	R_A0	(18)
#define	R_A1	(19)
#define	R_A2	(20)
#define	R_A3	(21)
#define	R_A4	(22)
#define	R_A5	(23)
#define	R_A6	(24)
#define	R_A7	(25)
#define	R_A8	(26)
#define	R_A9	(27)
#define	R_A10	(28)
#define	R_A11	(29)
#define	R_A12	(30)
#define	R_A13	(31)
#define	R_A14	(32)
#define	R_A15	(33)
#define	R_FPC	(34)
#define	R_F0	(35)
#define	R_F1	(36)
#define	R_F2	(37)
#define	R_F3	(38)
#define	R_F4	(39)
#define	R_F5	(40)
#define	R_F6	(41)
#define	R_F7	(42)
#define	R_F8	(43)
#define	R_F9	(44)
#define	R_F10	(45)
#define	R_F11	(46)
#define	R_F12	(47)
#define	R_F13	(48)
#define	R_F14	(49)
#define	R_F15	(50)
#define	NPRGREG	51

/*
 * The following defines are for portability (see <sys/regset.h>).
 */
#define	R_SP	R_G15
#define	R_FP	R_G11
#define	R_R0	R_G2
#define	R_R1	R_G3
#define R_PC	R_PSWA

/*
 * Floating point registers
 */
#define	R_Y0	(0)
#define	R_Y1	(1)
#define	R_Y2	(2)
#define	R_Y3	(3)
#define	R_Y4	(4)
#define	R_Y5	(5)
#define	R_Y6	(6)
#define	R_Y7	(7)
#define	R_Y8	(8)
#define	R_Y9	(9)
#define	R_Y10	(10)
#define	R_Y11	(11)
#define	R_Y12	(12)
#define	R_Y13	(13)
#define	R_Y14	(14)
#define	R_Y15	(15)
#define	NPRFREG	16

typedef struct prfpregset {
	uint32_t fpc;
	uint32_t pad;
	union {
		float f[NPRFREG];
		double g[NPRFREG];
		long double h[NPRFREG/2];
	} fp;
} prfpregset_t;

typedef	long		prgreg_t;
typedef	prgreg_t	prgregset_t[NPRGREG];
typedef	double		prfpreg_t;

#ifdef _SYSCALL32

typedef	int		prgreg32_t;
typedef	prgreg32_t	prgregset32_t[NPRGREG];
typedef double		prfreg32_t;
typedef prfpregset_t	prfpregset32_t;

typedef struct _stack390x {
	uint64_t *stk_last;
	uint64_t *stk_next;
	uint64_t *stk_regs[14];
} stack390x_t;

typedef struct _stack390 {
	uint32_t *stk_last;
	uint32_t *stk_next;
	uint32_t *stk_regs[14];
} stack390_t;

#endif	/* _SYSCALL32 */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_PROCFS_ISA_H */
