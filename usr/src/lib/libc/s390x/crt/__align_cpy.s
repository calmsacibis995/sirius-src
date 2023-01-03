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
 * Copyright (c) 1997, Sun Microsystems, Inc.
 * All rights reserved.
 */

	.file "__align_cpy_4.s"

/* __align_cpy_4(s1, s2, n)
 *
 * Copy 4-byte aligned source to 4-byte aligned target in multiples of 4 bytes.
 *
 * Input:
 *	r2	address of target
 *	r3	address of source
 *	r4	number of bytes to copy (must be a multiple of 4)
 * Output:
 *	r2	address of target
 * Caller's registers that have been changed by this function:
 *	r2-r5, 
 *
 * Note:
 *	This helper routine will not be used by any 32-bit compilations.
 *	To do so would break binary compatibility with previous versions of
 *	Solaris.
 *
 * Assumptions:
 *	Source and target addresses are 4-byte aligned.
 *	Bytes to be copied are non-overlapping or _exactly_ overlapping.
 *	The number of bytes to be copied is a multiple of 4.
 *	Call will usually be made with a byte count of more than 4*4 and
 *	less than a few hundred bytes.  Legal values are 0 to MAX_SIZE_T.
 *
 * Optimization attempt:
 *	Reasonable speed for a generic v9.
 */

#include <sys/asm_linkage.h>
 
	ENTRY(__align_cpy_4)
	stg	%r6,48(%r15)
	cgr	%r2,%r3			# If matching addresses
	je	.done			# ... Nothing to do
	ltgr	%r4,%r4			# If no bytes to move
	je	.done			# ... Then we're done here
	lghi	%r6,0
	lgr	%r5,%r2
	lgr	%r0,%r3
	lgr	%r1,%r4
	lgr	%r3,%r4
.move:	mvcle 	%r2,%r0,0(%r6)
	jnz	.move
.done:
	lgr	%r2,%r5
	lg	%r6,48(%r15)
	br	%r14

	SET_SIZE(__align_cpy_4)
