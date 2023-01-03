/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
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
 */
#pragma ident	"%Z%%M%	%I%	%E% SMI"
/*
 *	Copyright (c) 1994, by Sun Microsytems, Inc.
 */

#include <sys/asm_linkage.h>

	.file		__FILE__
/*
 * int tnfw_b_get_lock(tnf_byte_lock_t *);
 */
	ENTRY(tnfw_b_get_lock)
#ifdef __s390x__
	llgc	%r2,0(%r2)
#else
	llc	%r2,0(%r2)
#endif
	br	%r14	
	SET_SIZE(tnfw_b_get_lock)

/*
 * void tnfw_b_clear_lock(tnf_byte_lock_t *);
 */
	ENTRY(tnfw_b_clear_lock)
	mvi	0(%r2),0
	br	%r14
	SET_SIZE(tnfw_b_clear_lock)

/*
 * u_long tnfw_b_atomic_swap(u_long *, u_long);
 */
	ENTRY(tnfw_b_atomic_swap)
#ifdef ___s390x__
	lg	%r4,0(%r2)
0:	csg	%r4,%r3,0(%r2)
	jnz	0b
	lgr	%r2,%r4
#else
	l	%r4,0(%r2)
0:	cs	%r4,%r3,0(%r2)
	jnz	0b
	lr	%r2,%r4
#endif
	br	%r14
	SET_SIZE(tnfw_b_atomic_swap)
