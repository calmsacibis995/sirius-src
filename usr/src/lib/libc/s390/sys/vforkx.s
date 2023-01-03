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

	.file	"vfork.s"

#include <sys/asm_linkage.h>

#ifndef __GNUC__
	ANSI_PRAGMA_WEAK(vfork,function)
	ANSI_PRAGMA_WEAK(vforkx,function)
#endif

#include "SYS.h"
#include <../assym.h>

/*
 * pid = vforkx(flags);
 * syscall trap: forksys(2, flags)
 *
 * pid = vfork();
 * syscall trap: forksys(2, 0)
 *
 * From the syscall:
 * %r3 == 0 in parent process, %r3 == 1 in child process.
 * %r2 == pid of child in parent, %r2 == pid of parent in child.
 *
 * The child process gets a zero return value from vfork; the parent
 * gets the pid of the child.
 *
 * We block all blockable signals while performing the vfork() system call
 * trap.  This enables us to set curthread->ul_vfork safely, so that we
 * don't end up in a signal handler with curthread->ul_vfork set wrong.
 *
 */

	ENTRY(vforkx)
	lr	%r5,%r2
	j	0f

	ENTRY(vfork)
	lhi	%r5,0
0:
	stm	%r6,%r7,24(%r15)
	larl	%r1,.Lcdat
	lhi	%r2,SIG_SETMASK
	l	%r3,.Lcst0-.Lcdat(%r1)
	l	%r4,.Lcst1-.Lcdat(%r1)
	SYSTRAP_2RVALS(lwp_sigmask)

	lhi	%r2,2
	lr	%r3,%r5
	SYSTRAP_2RVALS(forksys)
	ltr	%r0,%r0
	jz	1f

	lr	%r6,%r2			// Save the vfork() error number

	GET_THR(7)

	larl	%r1,.Lcdat
	lhi	%r2,SIG_SETMASK
	l	%r3,UL_SIGMASK(%r7)
	l	%r4,UL_SIGMASK+4(%r7)
	SYSTRAP_2RVALS(lwp_sigmask)

	lr	%r2,%r6 		// Restore the vfork() error number
	jg  	__cerror
1:
	/*
	 * To determine if we are (still) a child of vfork(), the child
	 * increments curthread->ul_vfork by one and the parent decrements
	 * it by one.  If the result is zero, then we are not a child of
	 * vfork(), else we are.  We do this to deal with the case of
	 * a vfork() child calling vfork().
	 */
	GET_THR(7)

	l	%r4,UL_VFORK(%r7)	// curthread->ul_vfork
	ltr	%r3,%r3			// Child or parent
	jnz	2f			// Child
	
	ltr	%r4,%r4			// Already zero?
	jz	3f			// Yes.. don't go negative

	ahi	%r4,-1			// curthread->ul_vfork--
	j	3f

2:
	lhi	%r2,0			// Child return 0
	ahi	%r4,1			// curthread->ul_vfork++
3:
	st	%r4,UL_VFORK(%r7)	// Set ul_vfork
	/*
	 * Clear the schedctl interface in both parent and child.
	 * (The child might have modified the parent.)
	 */
	lhi	%r0,0
	st	%r0,UL_SCHEDCTL(%r7)
	st	%r0,UL_SCHEDCTL_CALLED(%r7)

	lr	%r6,%r2			// Save the vfork() return value
	larl	%r1,.Lcdat
	lhi	%r2,SIG_SETMASK
	l	%r3,UL_SIGMASK(%r7)
	l	%r4,UL_SIGMASK+4(%r7)
	SYSTRAP_2RVALS(lwp_sigmask)

	lr	%r2,%r6 		// Restore the vfork() error number
	lm	%r6,%r7,24(%r15)
	br	%r14			// Return
	SET_SIZE(vfork)
	.section ".rodata"
	.align	4
.Lcdat: 
.Lcst1:	.long   MASKSET1
.Lcst0:	.long   MASKSET0
	.section ".text"

#ifdef __GNUC__
	ANSI_PRAGMA_WEAK2(_vfork,vfork,function)
	ANSI_PRAGMA_WEAK2(_vforkx,vforkx,function)
#endif
