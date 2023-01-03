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
	.file	"strcpy.s"

/*
 * strcpy(s1, s2)
 *
 * Copy string s2 to s1.  s1 must be large enough. Return s1.
 *
 * Fast assembler language version of the following C-program strcpy
 * which represents the `standard' for the C-library.
 *
 *	char *
 *	strcpy(s1, s2)
 *	register char *s1;
 *	register const char *s2;
 *	{
 *		char *os1 = s1;
 *	
 *		while(*s1++ = *s2++)
 *			;
 *		return(os1);
 *	}
 *
 */

#include <sys/asm_linkage.h>

	ENTRY(strcpy)

	.align 32
	lghi	%r0,0
	lgr	%r1,%r2
0:
	mvst	%r1,%r3
	jo	0b

	br	%r14

	SET_SIZE(strcpy)

