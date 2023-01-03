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

#include <sys/asm_linkage.h>
#include <sys/machparam.h>
#include <sys/stack.h>
#include <sys/trap.h>
#include <sys/reboot.h>

/* XXX No sharing here -- make this into two files */

/*
 * Exit routine from linker/loader to kernel.
 */

#if defined(lint) || defined(__lint)
#warning lint defined

/* ARGSUSED */
void
exitto(caddr_t entrypoint)
{}

#else	/* lint */

	ENTRY_NP(exitto)
	stmg	%r12,%r15,48(%r15)
	aghi	%r15,-SA(MINFRAME)
	lgr	%r12,%r2
	larl	%r1,boothowto
	basr	%r13,0
	j	0f
	.quad	RB_DEBUGENTER
0:	lg	%r1,0(%r1)
	lg	%r13,4(%r13)
	ngr	%r1,%r13
	jz	1f
	lghi	%r0,ST_KMDB_TRAP
	svc	255
1:
	larl	%r2,romp
	larl	%r3,ops
	lg	%r2,0(%r2)
	lg	%r3,0(%r3)
	br	%r12

	/*  there is no return from here */
	SET_SIZE(exitto)

#endif	/* lint */
