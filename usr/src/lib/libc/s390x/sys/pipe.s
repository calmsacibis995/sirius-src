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

/* C library -- pipe						*/
/* int pipe (int fildes[2]);					*/

	.file	"pipe.s"

#include <sys/asm_linkage.h>

#include "SYS.h"

	ENTRY(pipe)
	stg	%r6,48(%r15)
	lgr	%r6,%r2
	SYSTRAP_2RVALS(pipe)
	SYSCERROR
	st	%r2,0(%r6)
	st	%r3,4(%r6)
	lg	%r6,48(%r15)
	RETC

	SET_SIZE(pipe)

#undef pipe
	ANSI_PRAGMA_WEAK2(_pipe,pipe,function)
