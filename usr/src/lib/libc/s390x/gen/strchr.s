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
	.file	"strchr.s"

/*
 * The strchr() function returns a pointer to the first occurrence of c 
 * (converted to a char) in string s, or a null pointer if c does not occur
 * in the string.
 */

#include <sys/asm_linkage.h>

	// Here, we start by checking to see if we're searching the dest
	// string for a null byte.  We have fast code for this, so it's
	// an important special case.  Otherwise, if the string is not
	// word aligned, we check a for the search char a byte at a time
	// until we've reached a word boundary.  Once this has happened
	// some zero-byte finding values are initialized and the string
	// is checked a word at a time

	ENTRY(strchr)

	//
	// First we get the length of the string to be checked by looking for the
	// null byte
	//
	lgr	%r4,%r2			// Save string pointer	
	larl	%r1,eosTab		// Point at delimiter table
0:
	trt	0(256,%r2),0(%r1)	// Search for end of string
	jnz	1f			// Found... Leave

	aghi	%r2,256			// Next block of characters
	j	0b			// Try again
	
1:
	chi	%r3,0			// Are we looking for '0'
	je	3f			// Yes... So we found it
	
	sgr	%r1,%r4			// Determine length

	//
	// At this stage r1 is the length of the string
	//
	lgr	%r2,%r4			// Point at start of string
	lgfr	%r0,%r3			// Get byte to scan for
	agr	%r1,%r2			// Point at end of string
2:
	srst	%r1,%r2			// Search for our byte of interest
	jo	2b			// Keep going if there's nore to do
	brc	13,3f			// Found - let's go
	lghi	%r1,0			// Indicate not found
3:
	lgr	%r2,%r1			// Copy result
	br	%r14			// Return

	SET_SIZE(strchr)
