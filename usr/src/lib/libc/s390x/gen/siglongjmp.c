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

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

#include <sys/types.h>
#include <sys/ucontext.h>
#include <setjmp.h>
#include <ucontext.h>
#include <sigjmp_struct.h>

extern int _setcontext(const ucontext_t *);

#pragma weak siglongjmp = _siglongjmp

void
_siglongjmp(sigjmp_buf env, int val)
{
	ucontext_t uc;
	pswg_t	   *psw = &uc.uc_mcontext.psw; 
	greg_t     *gr  = &uc.uc_mcontext.gregs[0];
	fpregset_t *fp  = &uc.uc_mcontext.fpregs;
	volatile sigjmp_struct_t *bp = (sigjmp_struct_t *) env;

	/* 
	 * Create a ucontext_t structure from scratch.
	 */
	memset(&uc, 0, sizeof(uc));
	uc.uc_flags = UC_STACK | UC_CPU;
	uc.uc_stack = bp->sjs_stack;
	uc.uc_link  = bp->sjs_uclink;
	psw->pc     = bp->sjs_pc;
	gr[REG_SP]  = bp->sjs_sp;

	if (bp->sjs_flags & JB_SAVEMASK) {
		uc.uc_flags  |= UC_SIGMASK;
		uc.uc_sigmask = bp->sjs_sigmask;
	}

	if (val)
		gr[REG_G2] = val;
	else
		gr[REG_G2] = 1;

	gr[REG_G6]	  = bp->sjs_r6;
	gr[REG_G7]	  = bp->sjs_r7;
	gr[REG_G8]	  = bp->sjs_r8;
	gr[REG_G9]	  = bp->sjs_r9;
	gr[REG_G10]	  = bp->sjs_r10;
	gr[REG_G11]	  = bp->sjs_r11;
	gr[REG_G12]	  = bp->sjs_r12;
	gr[REG_G13]	  = bp->sjs_r13;
	fp->fr.fd[REG_F1] = bp->sjs_f1;
	fp->fr.fd[REG_F3] = bp->sjs_f3;
	fp->fr.fd[REG_F5] = bp->sjs_f5;
	fp->fr.fd[REG_F7] = bp->sjs_f7;
	fp->fr.fd[REG_F4] = bp->sjs_f4;
	fp->fr.fd[REG_F6] = bp->sjs_f6;
	fp->fpc		  = bp->sjs_fpc;

	(void) _setcontext(&uc);
}
