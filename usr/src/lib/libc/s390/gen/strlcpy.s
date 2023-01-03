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

	.file	"strlcpy.s"

/*
 * The strlcpy() function copies at most dstsize-1 characters
 * (dstsize being the size of the string buffer dst) from src
 * to dst, truncating src if necessary. The result is always
 * null-terminated.  The function returns strlen(src). Buffer
 * overflow can be checked as follows:
 *
 *   if (strlcpy(dst, src, dstsize) >= dstsize)
 *           return -1;
 */

#include <sys/asm_linkage.h>

	// strlcpy implementation is similar to that of strcpy, except
	// in this case, the maximum size of the detination must be
	// tracked since it bounds our maximum copy size.  However,
	// we must still continue to check for zero since the routine
	// is expected to null-terminate any string that is within
	// the dest size bound.
	//

	ENTRY(strlcpy)
	stm	%r6,%r15,24(%r15)
	lr	%r6,%r3			// Copy source
	lr	%r8,%r2			// Copy destination
	lr	%r5,%r4			// Copy max length
	larl	%r1,eosTab		// Point at end of string table
0:
	trt	0(256,%r6),0(%r1)	// Locate end of string
	jnz	1f			// Go if found

	ahi	%r6,256			// Next block
	j	0b			// Try again
	
1:
	sr	%r1,%r3			// Determine source length
	lr	%r2,%r1			// Copy
	ltr	%r4,%r4			// Do we want 0 bytes?
	jz	6f			// Yes... Get out of here

	ltr	%r2,%r2			// Do we have 0 bytes?
	jnz	2f			// No... Skip to move

	mvi	0(%r8),0		// Terminate 
	j	6f			// Get out of here

2:
	lr	%r10,%r3		// Get *str2
	lr	%r1,%r8			// Save *str1
	cr	%r2,%r4			// Is length of src < wanted?
	jnl	3f			// No... Skip

	lr	%r4,%r2			// Use max length
3:
	lr	%r9,%r4			// Get l'str1
	lr	%r11,%r4		// Get l'str1
4:	mvcle	%r8,%r10,0		// Move string
	jo	4b			// If not complete.. keep checking	

	cr	%r4,%r5			// Is length moved < source
	jl	5f			// Yes... Skip

	ahi	%r4,-1			// Backpeddle 
5:
	ar	%r4,%r1			// Point at end of string
	mvi	0(%r4),0		// Null terminate *str1
6:
	lm	%r6,%r15,24(%r15)
	br	%r14
	SET_SIZE(strlcpy)
