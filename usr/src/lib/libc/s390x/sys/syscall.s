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
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved	*/


/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * C library -- int syscall(int sysnum, ...);
 * C library -- int __systemcall(sysret_t *, int sysnum, ...);
 *
 * Interpret a given system call
 *
 * This version handles up to 8 'long' arguments to a system call.
 *
 */

	.file	"syscall.s"

#include <sys/asm_linkage.h>

#ifndef __GNUC__
	ANSI_PRAGMA_WEAK(syscall,function)
#endif

#include "SYS.h"

	ENTRY(_syscall)
	ALTENTRY(_syscall6)
	stmg	%r6,%r14,48(%r15)
	lgr	%r11,%r15
	aghi	%r15,-SA(MINFRAME+3*8)
	lgfr	%r0,%r2			// sysnum
	lgr	%r2,%r3			// arg1
	lgr	%r3,%r4			// arg2
	lgr	%r4,%r5			// arg3
	lgr	%r5,%r6			// arg4
	lg	%r6,MINFRAME(%r11)	// arg5
	mvc	MINFRAME(3*8,%r15),MINFRAME+8(%r11)
	svc	SYSCALL_TRAPNUM
	ltgr	%r0,%r0
	jz	0f

	aghi	%r15,SA(MINFRAME+3*8)
	lmg	%r6,%r14,48(%r15)
	jg	__cerror
0:
	aghi	%r15,SA(MINFRAME+3*8)
	lmg	%r6,%r14,48(%r15)
	br	%r14
	SET_SIZE(_syscall)
	SET_SIZE(_syscall6)

	ENTRY(__systemcall)
	ALTENTRY(__systemcall6)
	stmg	%r6,%r14,48(%r15)
	lgr	%r11,%r15
	aghi	%r15,-SA(MINFRAME+3*8)
	lgr	%r10,%r2
	lgfr	%r0,%r3			// sysnum
	lgr	%r2,%r4			// arg1
	lgr	%r3,%r5			// arg2
	lgr	%r4,%r6			// arg3
	lg	%r5,MINFRAME(%r11)	// arg4
	lg	%r6,MINFRAME+4(%r11)	// arg5
	mvc	MINFRAME(3*8,%r15),MINFRAME+16(%r11)
	svc	SYSCALL_TRAPNUM
	ltgr	%r0,%r0
	jz	1f

	lghi	%r0,-1			// Error!
	stg	%r0,0(%r10)
	stg	%r0,8(%r10)
	j	2f
1:
	stg	%r2,0(%r10)		// Rval1
	stg	%r3,8(%r10)		// Rval2
	lghi	%r2,0			// No error
2:
	aghi	%r15,SA(MINFRAME+3*8)
	lmg	%r6,%r14,48(%r15)
	br	%r14
	SET_SIZE(__systemcall)
	SET_SIZE(__systemcall6)

#ifdef __GNUC__
	ANSI_PRAGMA_WEAK2(syscall,_syscall,function)
#endif

