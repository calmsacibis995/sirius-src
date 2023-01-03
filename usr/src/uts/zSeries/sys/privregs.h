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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_PRIVREGS_H
#define	_SYS_PRIVREGS_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * This file is kernel isa dependent.
 */

/*
 * This file describes the cpu's privileged register set, and
 * how the machine state is saved on the stack when a trap occurs.
 */

#ifndef	_ASM

struct regs {
	uint64_t	r_psw[2];
	uint64_t	r_g0;
	uint64_t	r_g1;
	uint64_t	r_g2;
	uint64_t	r_g3;
	uint64_t	r_g4;
	uint64_t	r_g5;
	uint64_t	r_g6;
	uint64_t	r_g7;
	uint64_t	r_g8;		
	uint64_t	r_g9;
	uint64_t	r_g10;
	uint64_t	r_g11;
	uint64_t	r_g12;
	uint64_t	r_g13;
	uint64_t	r_g14;
	uint64_t	r_g15;
	uint32_t	xxx;
	uint32_t	r_ilc;
	uint32_t	r_a0;
	uint32_t	r_a1;
	uint32_t	r_a2;
	uint32_t	r_a3;
	uint32_t	r_a4;
	uint32_t	r_a5;
	uint32_t	r_a6;
	uint32_t	r_a7;
	uint32_t	r_a8;
	uint32_t	r_a9;
	uint32_t	r_a10;
	uint32_t	r_a11;
	uint32_t	r_a12;
	uint32_t	r_a13;
	uint32_t	r_a14;
	uint32_t	r_a15;
	double		r_f0;
	double		r_f1;
	double		r_f2;
	double		r_f3;
	double		r_f4;
	double		r_f5;
	double		r_f6;
	double		r_f7;
	double		r_f8;
	double		r_f9;
	double		r_f10;
	double		r_f11;
	double		r_f12;
	double		r_f13;
	double		r_f14;
	double		r_f15;
	uint32_t	r_fpc;
	uint32_t	r_pad;
	uint64_t	r_c0;
	uint64_t	r_c1;
	uint64_t	r_c2;
	uint64_t	r_c3;
	uint64_t	r_c4;
	uint64_t	r_c5;
	uint64_t	r_c6;
	uint64_t	r_c7;
	uint64_t	r_c8;		
	uint64_t	r_c9;
	uint64_t	r_c10;
	uint64_t	r_c11;
	uint64_t	r_c12;
	uint64_t	r_c13;
	uint64_t	r_c14;
	uint64_t	r_c15;
	int		tstate;
};

#define	r_sp	r_g15
#define	r_pc	r_psw[1]

/*
 * Not used except to allow things using core.h to build cleanly
 */
struct fpu {
	double	r_dummy;
};

#endif	/* _ASM */

#define PSTATE_IE	0x10

#ifdef _KERNEL

#define	lwptoregs(lwp)	((struct regs *)((lwp)->lwp_regs))
#define	lwptofpu(lwp)	((kfpu_t *)((lwp)->lwp_fpu))

/*
 * Macros for saving/restoring registers.
 */

#define	SAVE_GLOBALS(RP) \
	stg	%r0, [RP + G1_OFF]; \
	stg	%r1, [RP + G1_OFF]; \
	stg	%r2, [RP + G2_OFF]; \
	stg	%r3, [RP + G3_OFF]; \
	stg	%r4, [RP + G4_OFF]; \
	stg	%r5, [RP + G5_OFF]; \
	stg	%r6, [RP + G6_OFF]; \
	stg	%r7, [RP + G7_OFF]; \
	stg	%r8, [RP + G8_OFF]; \
	stg	%r9, [RP + G9_OFF]; \
	stg	%r10, [RP + G10_OFF]; \
	stg	%r11, [RP + G11_OFF]; \
	stg	%r12, [RP + G12_OFF]; \
	stg	%r13, [RP + G13_OFF]; \
	stg	%r14, [RP + G14_OFF]; \
	stg	%r15, [RP + G15_OFF];

#define	RESTORE_GLOBALS(RP) \
	lg	%r0, [RP + G1_OFF]; \
	lg	%r1, [RP + G1_OFF]; \
	lg	%r2, [RP + G2_OFF]; \
	lg	%r3, [RP + G3_OFF]; \
	lg	%r4, [RP + G4_OFF]; \
	lg	%r5, [RP + G5_OFF]; \
	lg	%r6, [RP + G6_OFF]; \
	lg	%r7, [RP + G7_OFF]; \
	lg	%r8, [RP + G8_OFF]; \
	lg	%r9, [RP + G9_OFF]; \
	lg	%r10, [RP + G10_OFF]; \
	lg	%r11, [RP + G11_OFF]; \
	lg	%r12, [RP + G12_OFF]; \
	lg	%r13, [RP + G13_OFF]; \
	lg	%r14, [RP + G14_OFF]; \
	lg	%r15, [RP + G15_OFF];


#define	STORE_FPREGS(FP) \
	std	%f0, [FP]; 	\
	std	%f1, [FP + 8];	\
	std	%f2, [FP + 16]; \
	std	%f3, [FP + 24]; \
	std	%f4, [FP + 32]; \
	std	%f5, [FP + 40]; \
	std	%f6, [FP + 48]; \
	std	%f7, [FP + 56]; \
	std	%f8, [FP + 64];	\
	std	%f9, [FP + 72];	\
	std	%f10, [FP + 80]; \
	std	%f11, [FP + 88]; \
	std	%f12, [FP + 96]; \
	std	%f13, [FP + 104]; \
	std	%f14, [FP + 112]; \
	std	%f15, [FP + 120]; 

#define	LOAD_FPREGS(FP) \
	ld	%f0, [FP]; 	\
	ld	%f1, [FP + 8];	\
	ld	%f2, [FP + 16]; \
	ld	%f3, [FP + 24]; \
	ld	%f4, [FP + 32]; \
	ld	%f5, [FP + 40]; \
	ld	%f6, [FP + 48]; \
	ld	%f7, [FP + 56]; \
	ld	%f8, [FP + 64];	\
	ld	%f9, [FP + 72];	\
	ld	%f10, [FP + 80]; \
	ld	%f11, [FP + 88]; \
	ld	%f12, [FP + 96]; \
	ld	%f13, [FP + 104]; \
	ld	%f14, [FP + 112]; \
	ld	%f15, [FP + 120]; 

#define	STORE_DL_FPREGS(FP) \
	std	%f0, [FP]; \
	std	%f2, [FP + 8]; \
	std	%f4, [FP + 16]; \
	std	%f6, [FP + 24]; \
	std	%f8, [FP + 32]; \
	std	%f10, [FP + 40]; \
	std	%f12, [FP + 48]; \
	std	%f14, [FP + 56]; \
	std	%f16, [FP + 64]; \
	std	%f18, [FP + 72]; \
	std	%f20, [FP + 80]; \
	std	%f22, [FP + 88]; \
	std	%f24, [FP + 96]; \
	std	%f26, [FP + 104]; \
	std	%f28, [FP + 112]; \
	std	%f30, [FP + 120];

#endif /* _KERNEL */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_PRIVREGS_H */
