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

	.file	"unwind_frame.s"

#include "SYS.h"
#include <../assym.h>

/* Cancellation/thr_exit() stuff */

/*
 * _ex_unwind_local(void)
 *
 * Called only from _t_cancel().
 * Unwind two frames and invoke _t_cancel(fp) again.
 *
 * Before this the call stack is: f4 f3 f2 f1 _t_cancel
 * After this the call stack is:  f4 f3 f2 _t_cancel
 *	(as if "call f1" is replaced by "call _t_cancel(fp)" in f2)
 */
	ENTRY(_ex_unwind_local)
	lgr	%r2,%r15
	jg	_t_cancel
	SET_SIZE(_ex_unwind_local)

/*
 * _ex_clnup_handler(void *arg, void (*clnup)(void *))
 *
 * Called only from _t_cancel().
 * Unwind one frame, call the cleanup handler with argument arg from the
 * restored frame, then jump to _t_cancel(fp) again from the restored frame.
 */
	ENTRY(_ex_clnup_handler)
	GET_THR(1)			// Get thread pointer
	lgr	%r4,%r3			// Handler address
	stg	%r14,UL_UNWIND_RET(%r1) // Save caller's return address
	basr	%r14,%r4		// Call handler
	GET_THR(1)			// Get thread pointer
	lg	%r14,UL_UNWIND_RET(%r1) // Restore return address
	lgr	%r2,%r15		// Frame pointer is the arg register
	jg	_t_cancel		// Cancel
	SET_SIZE(_ex_clnup_handler)

/*
 * _thrp_unwind(void *arg)
 *
 * Ignore the argument; jump to _t_cancel(fp) with caller's fp
 */
	ENTRY(_thrp_unwind)
	lgr	%r2,%r15
	jg	_t_cancel		// tailcall _t_cancel(fp)
	SET_SIZE(_thrp_unwind)
