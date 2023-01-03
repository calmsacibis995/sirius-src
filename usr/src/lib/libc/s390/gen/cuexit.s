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

/* C library -- exit						*/
/* void exit (int status);					*/

	.file	"cuexit.s"
#include <sys/stack.h>
#include "SYS.h"

	ENTRY(exit)
	stm	%r2,%r14,8(%r15)
	ahi	%r15,-SA(MINFRAME32)
	brasl	%r14,_exithandle@PLT
	ahi	%r15,SA(MINFRAME32)
	lm	%r2,%r14,8(%r15)
	SYSTRAP_RVAL1(exit)

	SET_SIZE(exit)
