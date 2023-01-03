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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

	.file	"_stack_grow.s"

#include <sys/asm_linkage.h>
#include "SYS.h"
#include <../assym.h>

/*
 * void *
 * _stack_grow(void *addr)
 * {
 *	uintptr_t base = (uintptr_t)curthread->ul_ustack.ss_sp;
 *	size_t size = curthread->ul_ustack.ss_size;
 *
 *	if (size > (uintptr_t)addr - (base - STACK_BIAS))
 *		return (addr);
 *
 *	if (size == 0)
 *		return (addr);
 *
 *	if (size > %sp - (base - STACK_BIAS))
 *		%sp = base - STACK_BIAS - STACK_ALIGN;
 *
 *	*((char *)(base - 1));
 *
 *	_lwp_kill(_lwp_self(), SIGSEGV);
 * }
 */

	/*
	 * r2: address to which the stack will be grown (biased)
	 */
	ENTRY(_stack_grow)
	
	GET_THR(1)
	lg	%r3,UL_USTACK+SS_SP(%r1)
	lg	%r4,UL_USTACK+SS_SIZE(%r1)
	lgr	%r5,%r2
	sgr	%r5,%r3
	cgr	%r4,%r2
	bhr	%r14

	/*
	 * If the stack size is 0, stack checking is disabled.
	 */
1:
	ltgr	%r4,%r4
	bzr	%r14
	/*
	 * Move the stack pointer outside the stack bounds if it isn't already.
	 */
	lgr	%r5,%r15
	sgr	%r5,%r3
	cgr	%r4,%r5
	jle	3f
	aghi	%r5,-STACK_ALIGN
	lgr	%r15,%r5
3:
	/*
	 * Dereference an address in the guard page.
	 */
	lgr	%r8,%r3
	aghi	%r8,-1
	ic	%r9,0(%r8)

	/*
	 * If the above load doesn't raise a SIGSEGV then do it ourselves.
	 */
	SYSTRAP_RVAL1(lwp_self)
	lghi	%r3,SIGSEGV
	SYSTRAP_RVAL1(lwp_kill)

	/*
	 * We should never get here; explode if we do.
	 */
	.long 0
	SET_SIZE(_stack_grow)
