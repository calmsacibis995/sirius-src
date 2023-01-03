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

	.file	"_quad.s"

#include <sys/asm_linkage.h>
#include <SYS.h>

	.set	cw,0
	.set	cw_old,2
	.set	two_words,4
	.set	four_words,8

	ENTRY (_Qp_add)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	axbr	%f0,%f2
	std	%f0,0(%r4)
	std	%f1,8(%r4)
	br	%r14
	SET_SIZE(_Qp_add)

	ENTRY (_Qp_div)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	dxbr	%f0,%f2
	std	%f0,0(%r4)
	std	%f1,8(%r4)
	br	%r14
	SET_SIZE(_Qp_div)

	ENTRY (_Qp_dtoq)
	ler	%f2,%f0
	lxdbr	%f0,%f2
	std	%f0,0(%r3)
	std	%f1,8(%r3)
	br	%r14
	SET_SIZE(_Qp_dtoq)

	ENTRY (_Qp_feq)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	lghi	%r2,0
	kxbr	%f0,%f2
	ber	%r14
	lghi	%r2,1
	br	%r14
	SET_SIZE(_Qp_feq)

	ENTRY (_Qp_fge)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	lghi	%r2,0
	kxbr	%f0,%f2
	bher	%r14
	lghi	%r2,1
	br	%r14
	SET_SIZE(_Qp_fge)

	ENTRY (_Qp_fgt)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	lghi	%r2,0
	kxbr	%f0,%f2
	bhr	%r14
	lghi	%r2,1
	br	%r14
	SET_SIZE(_Qp_fgt)

	ENTRY (_Qp_fle)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	lghi	%r2,0
	kxbr	%f0,%f2
	bler	%r14
	lghi	%r2,1
	br	%r14
	SET_SIZE(_Qp_fle)

	ENTRY (_Qp_flt)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	lghi	%r2,0
	kxbr	%f0,%f2
	blr	%r14
	lghi	%r2,1
	br	%r14
	SET_SIZE(_Qp_flt)

	ENTRY (_Qp_fne)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	lghi	%r2,0
	kxbr	%f0,%f2
	bner	%r14
	lghi	%r2,1
	br	%r14
	SET_SIZE(_Qp_feq)

	ENTRY (_Qp_itoq)
	cxfbr	%f0,%r2
	std	%f0,0(%r3)
	std	%f1,8(%r3)
	br	%r14
	SET_SIZE(_Qp_itoq)

	ENTRY (_Qp_lltoq)
	cxgbr	%f0,%r2
	std	%f0,0(%r3)
	std	%f1,8(%r3)
	br	%r14
	SET_SIZE(_Qp_lltoq)

	ENTRY (_Qp_mul)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	mxbr	%f0,%f2
	std	%f0,0(%r4)
	std	%f1,8(%r4)
	br	%r14
	SET_SIZE(_Qp_mul)

	ENTRY (_Qp_neg)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	lnxbr	%f2,%f0
	std	%f2,0(%r3)
	std	%f3,8(%r3)
	br	%r14
	SET_SIZE(_Qp_neg)

	ENTRY (_Qp_qtod)
	ld	%f2,0(%r2)
	ld	%f3,8(%r2)
	ldxbr	%f0,%f2
	br	%r14
	SET_SIZE(_Qp_qtod)

	ENTRY (_Qp_qtoi)
	ld	%f2,0(%r2)
	ld	%f3,8(%r2)
	cfxbr	%f0,0,%f2
	br	%r14
	SET_SIZE(_Qp_qtoi)

	ENTRY (_Qp_qtos)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	lexbr	%f0,%f2
	br	%r14
	SET_SIZE(_Qp_qtos)

// This function converts a long double to a signed long long

	ENTRY(_Qp_qtox)
	ld	%f2,0(%r2)
	ld	%f3,8(%r2)
	cgdbr	%f0,0,%r2
	br	%r14
	SET_SIZE(_Qp_qtox)

// This converts a long double into a unsigned long.

	ENTRY(_Qp_qtou)
	larl	%r4,.cons1
	j	.skipcn1
	.align  8
.cons1:
.c1:	.long	1138753536	// 43e00000
	.long	0
.c11:	.long	0
	.long	0
.c2:	.long	1139802112	// 43f00000
	.long 	0
.c22:	.long	0
	.long	0
.skipcn1:
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,.c1-.cons1(%r4)
	ld	%f3,.c11-.cons1(%r4)
	cxbr	%f0,%f2
	jl	0f
	ld	%f2,.c2-.cons1(%r4)
	ld	%f3,.c22-.cons1(%r4)
	sxbr	%f0,%f2
	cfxbr	%r2,7,%f0
	j	1f
0:
	cfxbr	%r2,5,%f0	
1:	
	br	%r14
	SET_SIZE(_Qp_qtou)

// This converts a long double into a unsigned long long

	ENTRY(_Qp_qtoux)
	ALTENTRY(__fixunstfdi)
	larl	%r4,.consts
	j	.skipcon
	.align  8
.consts:
.v1:	.long	1105199104	// 41e00000
	.long	0
.v11:	.long	0
	.long	0
.v2:	.long	1106247680	// 41f00000
	.long 	0
.v22:	.long 	0
	.long 	0
.skipcon:
	lhi	%r2,0
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,.v1-.consts(%r4)
	ld	%f3,.v11-.consts(%r4)
	cxbr	%f0,%f2
	jl	0f
	ld	%f2,.v2-.consts(%r4)
	ld	%f3,.v22-.consts(%r4)
	sxbr	%f0,%f2
	cgxbr	%r2,7,%f0
	j	1f
0:
	cgxbr	%r2,5,%f0	
1:	
	br	%r14
	SET_SIZE(_Qp_qtoux)

	ENTRY (_Qp_sqrt)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	sqxbr	%f0,%f2
	std	%f0,0(%r4)
	std	%f1,8(%r4)
	br	%r14
	SET_SIZE(_Qp_sqrt)

	ENTRY (_Qp_stoq)
	ler	%f2,%f0
	lxebr	%f0,%f2
	br	%r14
	SET_SIZE(_Qp_stoq)

	ENTRY (_Qp_sub)
	ld	%f0,0(%r2)
	ld	%f1,8(%r2)
	ld	%f2,0(%r3)
	ld	%f3,8(%r3)
	sxbr	%f0,%f2
	std	%f0,0(%r4)
	std	%f1,8(%r4)
	br	%r14
	SET_SIZE(_Qp_sub)

	ENTRY (__floatditf)
	sllg	%r3,%r3,32
	or	%r3,%r4
	cxgbr	%f0,%r3
	std	%f0,0(%r2)
	std	%f2,8(%r2)
	br	%r14
	SET_SIZE(__floatditf)

	ENTRY (__fixdfdi)
	cgxbr	%f0,0,%r2
	lgr	%r3,%r2
	srlg	%r2,%r2,32
	nihf	%r3,0
	br	%r14
	SET_SIZE(__fixdfdi)

	ENTRY (isnanl)
	lhi	%r1,0x3002
	lhi	%r2,0
	tcxb	%f0,0(%r1)
	bnlr	%r14
	lhi	%r2,1
	br	%r14
	SET_SIZE(isnanl)

//
//	S390X FIXME
//
	ENTRY (__fixunsdfdi)
	lxdbr	%f0,%f0
	ltxbr	%f0,%f0
	bnlr	%r14
	sxbr 	%f0,%f0
	br	%r14
	SET_SIZE(__fixunsdfdi)

//
//	S390X FIXME
//
	ENTRY (frexpl)
	sxbr 	%f0,%f0
	br	%r14
	SET_SIZE(frexpl)

//
//	S390X FIXME
//
	ENTRY (ldexpl)
	sxbr 	%f0,%f0
	br	%r14
	SET_SIZE(ldexpl)
