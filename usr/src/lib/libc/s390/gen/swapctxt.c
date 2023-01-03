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
/*	  All Rights Reserved  	*/

/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma weak _swapcontext = swapcontext

#include "thr_uberdata.h"
#include <ucontext.h>
#include <sys/types.h>

int
swapcontext(ucontext_t *oucp, const ucontext_t *nucp)
{
	greg_t *reg;

	if (__getcontext_syscall(oucp))
		return (-1);

	/*
	 * Note that %r2 s modified by the system call
	 * routine. ABI calling conventions specify that the caller
	 * can not depend upon %r2 thru %r6, so no effort is
	 * made to maintain these registers. %r2 is forced to reflect
	 * an affirmative return code.
	 */
	reg = oucp->uc_mcontext.gregs;
	reg[REG_SP] = getfp();
	reg[REG_G14] = caller();
	reg[REG_G2]  = 0;

	return (setcontext((ucontext_t *)nucp));
}
