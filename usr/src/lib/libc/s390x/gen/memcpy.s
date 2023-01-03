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
 * Copyright 1997-2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
	.file	"memcpy.s"

/*
 * memcpy(s1, s2, len)
 *
 * Copy s2 to s1, always copy n bytes.
 * Note: this does not work for overlapped copies, bcopy() does
 *
 * Added entry __align_cpy_1 is generally for use of the compilers.
 *
 *
 * Fast assembler language version of the following C-program for memcpy
 * which represents the `standard' for the C-library.
 *
 *	void *
 */

#include <sys/asm_linkage.h>

	ENTRY(memcpy)
	ENTRY(__align_cpy_1)
	stg	%r6,48(%r15)
	cgr	%r2,%r3			# If matching addresses
	je	0f   			# ... Nothing to do
	ltgr	%r4,%r4			# If no bytes to move
	je	0f			# ... Then we're done here
	lghi	%r6,0
	lgr	%r5,%r2
	lgr	%r0,%r3
	lgr	%r1,%r4
	lgr	%r3,%r4
1:
	mvcle 	%r2,%r0,0(%r6)
	jnz	1b
0:
	lgr	%r2,%r5
	lg	%r6,48(%r15)
	br	%r14

	SET_SIZE(memcpy)
	SET_SIZE(__align_cpy_1)

	ANSI_PRAGMA_WEAK2(_memcpy,memcpy,function)
	ANSI_PRAGMA_WEAK2(_private_memcpy,memcpy,function)
