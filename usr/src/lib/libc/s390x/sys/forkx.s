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
 *
 * Copyright 2008 Sine Nomine Associates.
 * All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

	.file	"forkx.s"

#include "SYS.h"

/*
 * pid = __forkx(flags);
 *
 * syscall trap: forksys(0, flags)
 *
 * From the syscall:
 * %r3 == 0 in parent process, %r3 = 1 in child process.
 * %r2 == pid of child in parent, %r2 == pid of parent in child.
 *
 * The child gets a zero return value.
 * The parent gets the pid of the child.
 */

	ENTRY(__forkx)
	lgr	%r3,%r2
	lghi	%r2,0
	SYSTRAP_2RVALS(forksys)
	SYSCERROR
	ltgr	%r3,%r3
	jz	1f
	lghi	%r2,0
1:
	RET
	SET_SIZE(__forkx)
