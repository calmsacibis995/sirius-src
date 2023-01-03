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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

	.file "_lwp_mutex_unlock.s"

#include <sys/asm_linkage.h>
#include "SYS.h"
#include <sys/synch32.h>
#include <../assym.h>

	ENTRY(_lwp_mutex_unlock)
	lhi	%r1,0
	l	%r0,MUTEX_LOCK_WORD(%r2)
1:
	cs	%r0,%r1,MUTEX_LOCK_WORD(%r2)
	jnz	1b
	tmll	%r0,WAITER_MASK			// Check for waiters
	jnz	2f				// There are some

	lhi	%r2,0				// Set return value
	RET					// Return

2:
	SYSTRAP_RVAL1(lwp_mutex_wakeup)		// call kernel to wakeup waiter
	SYSLWPERR
 	RET
	SET_SIZE(_lwp_mutex_unlock)

	ANSI_PRAGMA_WEAK(__lwp_mutex_unlock,function)
	ANSI_PRAGMA_WEAK2(_private_lwp_mutex_unlock,_lwp_mutex_unlock,function)
