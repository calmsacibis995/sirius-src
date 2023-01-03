/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/asm_linkage.h>

	DGDEF(__fsr_init_value)
	.word 0

	ENTRY(waiting)
	lgb	%r2,0(%r2)
	br	%r14
	SET_SIZE(waiting)

	ENTRY(main)
	stmg	%r6,%r14,48(%r15)
	aghi	%r15,-SA(MINFRAME+4)
	lghi	%r0,0
	sty	%r0,-4(%r15)
1:
	brasl	%r14,waiting
	lgr	%r1,%r15
	aghi	%r1,-4
	cli	0(%r1)
	jz	1b

	brasl	%r14,test

	aghi	%r15,SA(MINFRAME+4)
	lmg	%r6,%r14,48(%r15)
	br	%r14

	ALTENTRY(inner)
	nop
	nop
	nop
	SET_SIZE(inner)

	lghi	%r2,0
	br	%r14
	SET_SIZE(main)
