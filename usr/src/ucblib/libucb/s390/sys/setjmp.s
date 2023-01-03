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
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"@(#)setjmp.s	1.2	05/06/08 SMI"

/*
 * The UCB setjmp(env) is the same as SYSV's sigsetjmp(env, 1)
 * while _setjmp(env) is the same as SYSV's sigsetjmp(env, 0)
 * Both longjmp(env, val) and _longjmp(env, val) are the same
 * as SYSV's siglongjmp(env, val).
 *
 * These are #defined as such in /usr/ucbinclude/setjmp.h
 * but setjmp/longjmp and _setjmp/_longjmp have historically
 * been entry points in libucb, so for binary compatibility
 * we implement them as tail calls into libc in order to make
 * them appear as direct calls to sigsetjmp/siglongjmp, which
 * is essential for the correct operation of sigsetjmp.
 */

	.file	"setjmp.s"

#include <sys/asm_linkage.h>

	ANSI_PRAGMA_WEAK(longjmp,function)

	ENTRY_NP(setjmp)
	lhi	%r3,1
	jg	_sigsetjmp
	SET_SIZE(setjmp)

	ENTRY_NP(_setjmp)
	lhi	%r3,0
	jg	_sigsetjmp
	SET_SIZE(_setjmp)

	ENTRY_NP(_longjmp)
	jg	_siglongjmp
	SET_SIZE(_longjmp)
