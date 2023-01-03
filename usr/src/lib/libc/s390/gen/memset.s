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

	.file	"memset.s"

/*
 * memset(sp, c, n)
 *
 * Set an array of n chars starting at sp to the character c.
 * Return sp.
 *
 * Fast assembler language version of the following C-program for memset
 * which represents the `standard' for the C-library.
 *
 *	void *
 *	memset(void *sp1, int c, size_t n)
 *	{
 *	    if (n != 0) {
 *		char *sp = sp1;
 *		do {
 *		    *sp++ = (char)c;
 *		} while (--n != 0);
 *	    }
 *	    return (sp1);
 *	}
 *
 *
 * Inputs:
 *	r2:  pointer to start of area to be set to a given value
 *	r3:  character used to set memory at location in r2
 *	r4:  number of bytes to be set
 *
 * Outputs:
 *	r2:  pointer to start of area set (same as input value in r2)
 *
 */

#include <sys/asm_linkage.h>
#include "lint.h"

	ENTRY(memset)
	ltr	%r4,%r4
	jz	.done

	lr	%r0,%r2		# save return value
	lr	%r1,%r3		# get pad byte
	lr	%r3,%r4		# get length
	lhi	%r4,0		# source is unimportant
	lr	%r5,%r4		# force mvcle to use pad
0:
	mvcle	%r2,%r4,0(%r1)	# propagate pad byte
	jo	0b		# do until done
	
	lr	%r2,%r0		# restore return value
.done:
	br	%r14
	SET_SIZE(memset)

	ANSI_PRAGMA_WEAK2(_memset,memset,function)
	ANSI_PRAGMA_WEAK2(_private_memset,memset,function)
