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
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
	.file "memchr.s"
/*
 * Return the ptr in sptr at which the character c1 appears;
 * or NULL if not found in n chars; don't stop at \0.
 */

#include <sys/asm_linkage.h>

	// The first part of this algorithm focuses on determining
	// whether or not the desired character is in the first few bytes
	// of memory, aligning the memory for word-wise copies, and
	// initializing registers to detect zero bytes

	ENTRY(memchr)

	lghi	%r0,255
	ngr	%r0,%r3
	lgr	%r1,%r2
	agr	%r2,%r4
0:
	srst	%r2,%r1
	jo	0b
	brc	13,1f
	lghi	%r2,0
1:
	br	%r14

	SET_SIZE(memchr)

	ANSI_PRAGMA_WEAK2(_memchr,memchr,function)
