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
 * Copyright (c) 1997-1998 by Sun Microsystems, Inc.
 * All rights reserved.
 */

	.file	"memcmp.s"

/*
 * memcmp(s1, s2, len)
 *
 * Compare n bytes:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 *
 */

#include <sys/asm_linkage.h>
#include "lint.h"

	ENTRY(memcmp)
	stm	%r6,%r9,24(%r15)
	lhi	%r0,0
	lr	%r6,%r2
	lr	%r7,%r4
	lr	%r8,%r3
	lr	%r9,%r4
0:
	clcle	%r6,%r8,0
	jo	0b
	je	1f
	jl	2f
	lhi 	%r2,1
	j	3f
1:
	lhi	%r2,0
	j	3f
2:
	lhi	%r2,-1
3:
	lm	%r6,%r9,24(%r15)	
	br	%r14

	SET_SIZE(memcmp)

	ANSI_PRAGMA_WEAK2(_memcmp,memcmp,function)
