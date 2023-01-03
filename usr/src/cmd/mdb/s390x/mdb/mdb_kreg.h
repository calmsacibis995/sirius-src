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
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* S390X FIXME */

#ifndef	_MDB_KREG_H
#define	_MDB_KREG_H

#ifndef _ASM
#include <sys/types.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define	KREG_NGREG	48
#ifndef _ASM
 typedef uint64_t kreg_t;
#endif

/*
 * mdb_tgt_gregset_t register indicies
 */

#define	KREG_G0		0
#define	KREG_G1		1
#define	KREG_G2		2
#define	KREG_G3		3
#define	KREG_G4		4
#define	KREG_G5		5
#define	KREG_G6		6
#define	KREG_G7		7
#define	KREG_G8		8
#define	KREG_G9		9
#define	KREG_G10	10
#define	KREG_G11	11
#define	KREG_G12	12
#define	KREG_G13	13
#define	KREG_G14	14
#define	KREG_G15	15
#define	KREG_FP		KREG_G15
#define KREG_PSW	16
#define KREG_C0		17
#define KREG_C1		18
#define KREG_C2		19
#define KREG_C3		20
#define KREG_C4		21
#define KREG_C5		22
#define KREG_C6		23
#define KREG_C7		24
#define KREG_C8		25
#define KREG_C9		26
#define KREG_C10	27
#define KREG_C11	28
#define KREG_C12	29
#define KREG_C13	30
#define KREG_C14	31
#define KREG_C15	32
#define	KREG_A0		33
#define KREG_A1		34
#define KREG_A2		35
#define KREG_A3		36
#define KREG_A4		37
#define KREG_A5		38
#define KREG_A6		39
#define KREG_A7		40
#define KREG_A8		41
#define KREG_A9		42
#define KREG_A10	43
#define KREG_A11	44
#define KREG_A12	45
#define KREG_A13	46
#define KREG_A14	47 
#define KREG_A15	48

#ifdef __sparcv9cpu

#define	KREG_CCR_XCC_N_MASK	0x80
#define	KREG_CCR_XCC_Z_MASK	0x40
#define	KREG_CCR_XCC_V_MASK	0x20
#define	KREG_CCR_XCC_C_MASK	0x10

#define	KREG_CCR_ICC_N_MASK	0x08
#define	KREG_CCR_ICC_Z_MASK	0x04
#define	KREG_CCR_ICC_V_MASK	0x02
#define	KREG_CCR_ICC_C_MASK	0x01

#define	KREG_FPRS_FEF_MASK	0x4
#define	KREG_FPRS_FEF_SHIFT	2

#define	KREG_FPRS_DU_MASK	0x2
#define	KREG_FPRS_DU_SHIFT	1

#define	KREG_FPRS_DL_MASK	0x1
#define	KREG_FPRS_DL_SHIFT	0

#define	KREG_TICK_NPT_MASK	0x8000000000000000ULL
#define	KREG_TICK_NPT_SHIFT	63

#define	KREG_TICK_CNT_MASK	0x7fffffffffffffffULL
#define	KREG_TICK_CNT_SHIFT	0

#define	KREG_PSTATE_CLE_MASK	0x200
#define	KREG_PSTATE_CLE_SHIFT	9

#define	KREG_PSTATE_TLE_MASK	0x100
#define	KREG_PSTATE_TLE_SHIFT	8

#define	KREG_PSTATE_MM_MASK	0x0c0
#define	KREG_PSTATE_MM_SHIFT	6

#define	KREG_PSTATE_MM_TSO(x)	(((x) & KREG_PSTATE_MM_MASK) == 0x000)
#define	KREG_PSTATE_MM_PSO(x)	(((x) & KREG_PSTATE_MM_MASK) == 0x040)
#define	KREG_PSTATE_MM_RMO(x)	(((x) & KREG_PSTATE_MM_MASK) == 0x080)
#define	KREG_PSTATE_MM_UNDEF(x)	(((x) & KREG_PSTATE_MM_MASK) == 0x0c0)

#define	KREG_PSTATE_RED_MASK	0x020
#define	KREG_PSTATE_RED_SHIFT	5

#define	KREG_PSTATE_PEF_MASK	0x010
#define	KREG_PSTATE_PEF_SHIFT	4

#define	KREG_PSTATE_AM_MASK	0x008
#define	KREG_PSTATE_AM_SHIFT	3

#define	KREG_PSTATE_PRIV_MASK	0x004
#define	KREG_PSTATE_PRIV_SHIFT	2

#define	KREG_PSTATE_IE_MASK	0x002
#define	KREG_PSTATE_IE_SHIFT	1

#define	KREG_PSTATE_AG_MASK	0x001
#define	KREG_PSTATE_AG_SHIFT	0

#define	KREG_PSTATE_MASK	0xfff

#define	KREG_TSTATE_CCR(x)	(((x) >> 32) & 0xff)
#define	KREG_TSTATE_ASI(x)	(((x) >> 24) & 0xff)
#define	KREG_TSTATE_PSTATE(x)	(((x) >> 8) & 0xfff)
#define	KREG_TSTATE_CWP(x)	((x) & 0x1f)

#define	KREG_TSTATE_PSTATE_MASK	0x000000000000fff0ULL
#define	KREG_TSTATE_PSTATE_SHIFT 8

#define	KREG_TBA_TBA_MASK	0xffffffffffff8000ULL
#define	KREG_TBA_TBA_SHIFT	0

#define	KREG_TBA_TLG0_MASK	0x4000
#define	KREG_TBA_TLG0_SHIFT	14

#define	KREG_TBA_TT_MASK	0x3fd0
#define	KREG_TBA_TT_SHIFT	5

#define	KREG_VER_MANUF_MASK	0xffff000000000000ULL
#define	KREG_VER_MANUF_SHIFT	48

#define	KREG_VER_IMPL_MASK	0x0000ffff00000000ULL
#define	KREG_VER_IMPL_SHIFT	32

#define	KREG_VER_MASK_MASK	0xff000000
#define	KREG_VER_MASK_SHIFT	24

#define	KREG_VER_MAXTL_MASK	0x0000ff00
#define	KREG_VER_MAXTL_SHIFT	8

#define	KREG_VER_MAXWIN_MASK	0x0000000f
#define	KREG_VER_MAXWIN_SHIFT	0

#else	/* __sparcv9cpu */

#define	KREG_PSR_IMPL_MASK	0xf0000000
#define	KREG_PSR_IMPL_SHIFT	28

#define	KREG_PSR_VER_MASK	0x0f000000
#define	KREG_PSR_VER_SHIFT	24

#define	KREG_PSR_ICC_MASK	0x00f00000
#define	KREG_PSR_ICC_N_MASK	0x00800000
#define	KREG_PSR_ICC_Z_MASK	0x00400000
#define	KREG_PSR_ICC_V_MASK	0x00200000
#define	KREG_PSR_ICC_C_MASK	0x00100000
#define	KREG_PSR_ICC_SHIFT	20

#define	KREG_PSR_EC_MASK	0x00002000
#define	KREG_PSR_EC_SHIFT	13

#define	KREG_PSR_EF_MASK	0x00001000
#define	KREG_PSR_EF_SHIFT	12

#define	KREG_PSR_PIL_MASK	0x00000f00
#define	KREG_PSR_PIL_SHIFT	8

#define	KREG_PSR_S_MASK		0x00000080
#define	KREG_PSR_S_SHIFT	7

#define	KREG_PSR_PS_MASK	0x00000040
#define	KREG_PSR_PS_SHIFT	6

#define	KREG_PSR_ET_MASK	0x00000020
#define	KREG_PSR_ET_SHIFT	5

#define	KREG_PSR_CWP_MASK	0x0000001f
#define	KREG_PSR_CWP_SHIFT	0

#define	KREG_TBR_TBA_MASK	0xfffff000
#define	KREG_TBR_TBA_SHIFT	0

#define	KREG_TBR_TT_MASK	0x00000ff0
#define	KREG_TBR_TT_SHIFT	4

#endif	/* __sparcv9cpu */

#ifdef	__cplusplus
}
#endif

#endif	/* _MDB_KREG_H */
