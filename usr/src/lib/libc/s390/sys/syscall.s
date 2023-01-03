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

#undef _syscall		/* override "synonyms.h" */
#undef __systemcall

	ENTRY(_syscall)
	ALTENTRY(_syscall6)
	stm	%r6,%r14,24(%r15)
	lr	%r11,%r15
	ahi	%r15,-SA(MINFRAME32+3*4)
	lr	%r0,%r2			// sysnum
	lr	%r2,%r3			// arg1
	lr	%r3,%r4			// arg2
	lr	%r4,%r5			// arg3
	lr	%r5,%r6			// arg4
	l	%r6,MINFRAME32(%r11)	// arg5
	mvc	MINFRAME32(3*4,%r15),MINFRAME32+4(%r11)
	svc	SYSCALL_TRAPNUM
	ltr	%r0,%r0
	jz	0f

	ahi	%r15,SA(MINFRAME32+3*4)
	lm	%r6,%r14,24(%r15)
	jg	__cerror
0:
	ahi	%r15,SA(MINFRAME32+3*4)
	lm	%r6,%r14,24(%r15)
	br	%r14
	SET_SIZE(_syscall)
	SET_SIZE(_syscall6)

	ENTRY(__systemcall)
	ALTENTRY(__systemcall6)
	stm	%r6,%r14,24(%r15)
	lr	%r11,%r15
	ahi	%r15,-SA(MINFRAME32+3*4)
	lr	%r10,%r2
	lr	%r0,%r3			// sysnum
	lr	%r2,%r4			// arg1
	lr	%r3,%r5			// arg2
	lr	%r4,%r6			// arg3
	l	%r5,MINFRAME32(%r11)	// arg4
	l	%r6,MINFRAME32+4(%r11)	// arg5
	mvc	MINFRAME32(3*4,%r15),MINFRAME32+8(%r11)
	svc	SYSCALL_TRAPNUM
	ltr	%r0,%r0
	jz	1f

	lhi	%r0,-1			// Error!
	st	%r0,0(%r10)
	st	%r0,4(%r10)
	j	2f
1:
	st	%r2,0(%r10)		// Rval1
	st	%r3,4(%r10)		// Rval2
	lhi	%r2,0			// No error
2:
	ahi	%r15,SA(MINFRAME32+3*4)
	lm	%r6,%r14,24(%r15)
	br	%r14
	SET_SIZE(__systemcall)
	SET_SIZE(__systemcall6)

#ifdef __GNUC__
	ANSI_PRAGMA_WEAK2(syscall,_syscall,function)
#endif
