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
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

	.file	"cerror.s"

#include "SYS.h"

	/*
	 * All system call wrappers call __cerror on error.
	 * __cerror is private to libc and must remain so.
	 * _cerror is exported from libc and is retained
	 * only for historical reasons.
	 */
	ENTRY2(_cerror,__cerror)
	chi	%r2,ERESTART
	jne	1f
	lghi	%r2,EINTR
1:
	stmg	%r6,%r7,48(%r15)
	aghi	%r15,-SA(MINFRAME)
	lgr	%r6,%r14
	lgr	%r7,%r2
	brasl	%r14,___errno@PLT
	st	%r7,0(%r2)
	lghi	%r2,-1
	aghi	%r15,SA(MINFRAME)
	lgr	%r14,%r6
	lmg	%r6,%r7,48(%r15)
	br      %r14

	SET_SIZE(_cerror)
	SET_SIZE(__cerror)
