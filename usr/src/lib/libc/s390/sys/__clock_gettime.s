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

	.file	"__clock_gettime.s"

#include <sys/time_impl.h>
#include "SYS.h"

/*
 * int
 * __clock_gettime(clockid_t clock_id, timespec_t *tp)
 */

	ENTRY(__clock_gettime)
	chi	%r2,__CLOCK_REALTIME0		// If ((clock_id == __CLOCK_REALTIME0) ||
	je	2f
	chi	%r2,CLOCK_REALTIME		// (clock_id == CLOCK_REALTIME) 
	jne	1f	
2:
	lr	%r6,%r3				// Copy
	SYSFASTTRAP(GETHRESTIME)
	stm	%r2,%r3,0(%r6)
	RETC
1:
	SYSTRAP_RVAL1(clock_gettime)
	SYSCERROR
	RETC
	SET_SIZE(__clock_gettime)
