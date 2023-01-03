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
 * Copyright (c) 1997,1998 by Sun Microsystems, Inc.
 * All rights reserved.
 */
	.file	"strncmp.s"

/*
 * strncmp(s1, s2, n)
 *
 * Compare strings (at most n bytes):  s1>s2: >0  s1==s2: 0  s1<s2: <0
 *
 * Fast assembler language version of the following C-program for strncmp
 * which represents the `standard' for the C-library.
 *
 *	int
 *	strncmp(const char *s1, const char *s2, size_t n)
 *	{
 *		n++;
 *		if (s1 == s2)
 *			return (0);
 *		while (--n != 0 && *s1 == *s2++)
 *			if(*s1++ == '\0')
 *				return(0);
 *		return ((n == 0) ? 0 : (*s1 - s2[-1]));
 *	}
 */

#include <sys/asm_linkage.h>

	ENTRY(strncmp)
	stmg	%r6,%r15,48(%r15)
	lgr 	%r9,%r4			// Set default l'str1 
	lgr	%r11,%r4		// Set default l'str2 
	lghi	%r0,0			// Get test byte
	lgr	%r6,%r2			// Get *str1
	lgr	%r7,%r6			// Starting point
	agr	%r6,%r4			// Point at end of *str1
0:	srst	%r6,%r7			// Locate 0x00
	jo	0b			// If partial test keep looking
	jz	1f			// No test byte found
	
	lgr	%r9,%r6			// Get size constraint
	sgr	%r9,%r7			// Set l'str1
1:	
	lgr	%r6,%r3			// Get *str2
	lgr	%r7,%r6			// Set starting point
	agr	%r6,%r4			// Point at end of *str2
2:	srst	%r6,%r7			// Locate 0x00
	jo	2b			// If partial test keep looking
	jz	3f			// No test byte found
	
	lgr	%r11,%r6		// Get size constraint
	sgr	%r11,%r7		// Set l'str2
3:
	lgr	%r8,%r2			// Get *str1
	lgr	%r10,%r3		// Get *str2
4:	clcle	%r8,%r10,0		// Compare strings
	jo	4b			// If not complete.. keep checking	
	je	5f			// Equal
	jl	6f			// Less than
	
	lghi	%r2,1
	j	7f
5:
	lghi	%r2,0
	j	7f
6:
	lghi	%r2,-1
7:
	lmg	%r6,%r15,48(%r15)
	br	%r14	
	SET_SIZE(strncmp)
