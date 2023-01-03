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
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#if !defined(__lint)
#include <sys/asm_linkage.h>
#include <sys/privregs.h>
#endif

/*
 * The interface for a client programs that call the 64-bit romvec OBP
 */

#if defined(__lint)
/* ARGSUSED */
int
client_handler(void *cif_handler, void *arg_array)
{
	return (0);
}
#else	/* __lint */

	ENTRY(client_handler)
	stmg	%r14,48(%r15)
	aghi	%r15,-SA(MINFRAME)
	lgr	%r1,%r2				// Get A(Handler)
	lgr	%r2,%r3				// Get A(Argument Array)
	basr	%r14,%r1			// Call handler
	aghi	%r15,SA(MINFRAME)		
	lmg	%r14,48(%r15)
	br	%r14
	SET_SIZE(client_handler)

#endif	/* __lint */
