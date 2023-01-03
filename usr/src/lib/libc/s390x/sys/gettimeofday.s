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

	.file	"gettimeofday.s"

/*
 * C library -- gettimeofday
 * int gettimeofday (struct timeval *tp);
 */

#include <sys/asm_linkage.h>

#ifndef __GNUC__
	ANSI_PRAGMA_WEAK(gettimeofday,function)
#else
	ANSI_PRAGMA_WEAK2(gettimeofday,_gettimeofday,function)
#endif

#include "SYS.h"

/*
 * The interface below calls the trap to get the timestamp in 
 * secs and nsecs. It than converts the nsecs value into usecs before
 * it returns.
 *
 */

	ENTRY(_gettimeofday)
	ltgr	%r2,%r2
	jz	1f
	aghi	%r15,-SA(MINFRAME+CLONGSIZE)
	stg	%r2,160(%r15)
	SYSFASTTRAP(GETHRESTIME)
	lg	%r1,160(%r15)
	stg  	%r2,0(%r1)
	lghi	%r4,1000
	dsgfr	%r2,%r4
	stg	%r3,CLONGSIZE(%r1)
	lghi	%r2,0
	aghi	%r15,SA(MINFRAME+CLONGSIZE)
1:	RETC
	SET_SIZE(_gettimeofday)
