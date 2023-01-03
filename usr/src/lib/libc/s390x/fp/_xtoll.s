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

	.file	"_xtoll.s"

#include <SYS.h>

// This function converts a double to a signed long

	ENTRY(__xtol)	// double to signed long
	lghi	%r2,0
	cfdbr	%f0,0,%r2
	br	%r14
	SET_SIZE(__xtol)

// This function converts a double to a signed long long

	ENTRY(__xtoll)	// double to signed long long
	cgdbr	%f0,0,%r2
	br	%r14
	SET_SIZE(__xtoll)

// This function truncates the top of the 387 stack into a unsigned long.

	ENTRY(__xtoul)	// double to unsigned long
	larl	%r4,__xto_con
	lghi	%r2,0
	cdb	%f0,.c1-__xto_con(%r4)
	jl	0f
	sdb	%f0,.c2-__xto_con(%r4)
	cfdbr	%r2,7,%f0
	j	1f
0:
	cfdbr	%r2,5,%f0	
1:	
	br	%r14
	SET_SIZE(__xtoul)
	
	.section ".rodata"
	.global	__xto_con
	.align	8
__xto_con:
.c1:	.long	1105199104	// 41e00000
	.long	0
.c2:	.long	1106247680	// 41f00000
	.long 	0
	.section ".text"
