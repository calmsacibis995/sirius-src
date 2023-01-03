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
	.file	"strlen.s"

/*
 * strlen(s)
 *
 * Given string s, return length (not including the terminating null).
 *	
 * Fast assembler language version of the following C-program strlen
 * which represents the `standard' for the C-library.
 *
 *	size_t
 *	strlen(s)
 *	register const char *s;
 *	{
 *		register const char *s0 = s + 1;
 *	
 *		while (*s++ != '\0')
 *			;
 *		return (s - s0);
 *	}
 */

#include <sys/asm_linkage.h>

	// The object of strlen is to, as quickly as possible, find the
	// null byte.  To this end, we attempt to get our string aligned
	// and then blast across it using Alan Mycroft's algorithm for
	// finding null bytes. If we are not aligned, the string is
	// checked a byte at a time until it is.  Once this occurs,
	// we can proceed word-wise across it.  Once a word with a
	// zero byte has been found, we then check the word a byte
	// at a time until we've located the zero byte, and return
	// the proper length.

	ENTRY(strlen)
	
	.align 32
	lgr	%r3,%r2
	larl	%r1,eosTab
0:
	trt	0(256,%r2),0(%r1)
	jnz	1f

	aghi	%r2,256
	j	0b
	
1:
	sgr	%r1,%r3
	lgr	%r2,%r1
	br	%r14

	SET_SIZE(strlen)

	.section ".rodata"
	.align	8
	.global eosTab
	.type	eosTab, @object
eosTab:
	.quad	0xff00000000000000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.quad	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.section ".text"
