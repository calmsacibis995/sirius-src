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
	.file	"strncpy.s"

/*
 * strncpy(s1, s2)
 *
 * Copy string s2 to s1, truncating or null-padding to always copy n bytes
 * return s1.
 *
 * Fast assembler language version of the following C-program for strncpy
 * which represents the `standard' for the C-library.
 *
 *	char *
 *	strncpy(char *s1, const char *s2, size_t n)
 *	{
 *		char *os1 = s1;
 *	
 *		n++;				
 *		while ((--n != 0) &&  ((*s1++ = *s2++) != '\0'))
 *			;
 *		if (n != 0)
 *			while (--n != 0)
 *				*s1++ = '\0';
 *		return (os1);
 *	}
 */

#include <sys/asm_linkage.h>

	// strncpy works similarly to strcpy, except that n bytes of s2
	// are copied to s1. If a null character is reached in s2 yet more
	// bytes remain to be copied, strncpy will copy null bytes into
	// the destination string.
	//
	// This implementation works by first aligning the src ptr and
	// performing small copies until it is aligned.  Then, the string
	// is copied based upon destination alignment.  (byte, half-word,
	// word, etc.)

	ENTRY(strncpy)
	stmg	%r6,%r15,48(%r15)
	lgr	%r11,%r4		// Set default l'str2 
	lgr	%r9,%r4			// ....
	lghi	%r0,0			// Get test byte
	lgr	%r6,%r3			// Get *str2
	lgr	%r7,%r6			// Starting point
	lgr	%r8,%r6			// ......
	agr	%r6,%r4			// Point at end of *str2
0:	srst	%r6,%r7			// Locate 0x00
	jo	0b			// If partial test keep looking
	jz	1f			// No test byte found
	
	lgr	%r11,%r6		// Get size constraint
	sgr	%r11,%r8		// Set l'str2
1:
	lghi	%r1,0			// Get padding byte
	lgr	%r9,%r4			// Get l'str1
	lgr	%r8,%r2			// Get *str1
	lgr	%r10,%r3		// Get *str2
2:	mvcle	%r8,%r10,0(%r1)		// Move string
	jo	2b			// If not complete.. keep checking	
	lmg	%r6,%r15,48(%r15)
	br	%r14	
	SET_SIZE(strncpy)
