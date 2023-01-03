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
	.file	"strcmp.s"

/* strcmp(s1, s2)
 *
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 *
 * Fast assembler language version of the following C-program for strcmp
 * which represents the `standard' for the C-library.
 *
 *	int
 *	strcmp(s1, s2)
 *	register const char *s1;
 *	register const char *s2;
 *	{
 *	
 *		if(s1 == s2)
 *			return(0);
 *		while(*s1 == *s2++)
 *			if(*s1++ == '\0')
 *				return(0);
 *		return(*s1 - s2[-1]);
 *	}
 */

#include <sys/asm_linkage.h>

	ENTRY(strcmp)

	.align 32

	lghi	%r0,0
0:	
	clst	%r2,%r3
	jo	0b	
	je	1f
	jl	2f
	
	lghi	%r2,1
	j	3f
1:
	lghi	%r2,0
	j	3f
2:
	lghi	%r2,-1
3:
	br	%r14	
	
	SET_SIZE(strcmp)
