/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
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
 */
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved	*/


/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

	.file	"%M%"

/* C library -- getdents					*/
/* int getdents (int fildes, struct dirent *buf, size_t count)	*/

#include <sys/asm_linkage.h>

#ifndef __GNUC__
	#if !defined(_LARGEFILE_SOURCE)
		ANSI_PRAGMA_WEAK(getdents,function)
	#else
		ANSI_PRAGMA_WEAK(getdents64,function)
	#endif	
#endif

#include "SYS.h"

#if !defined(_LARGEFILE_SOURCE)
	
	SYSCALL_RVAL1(getdents)
	RET
	SET_SIZE(getdents)

#ifdef __GNUC__
	ANSI_PRAGMA_WEAK2(_getdents,getdents,function)
#endif

#else

/* C library -- getdents64					*/
/* int getdents64 (int fildes, struct dirent64 *buf, size_t count)	*/

	SYSCALL_RVAL1(getdents64)
	RET
	SET_SIZE(getdents64)

#ifdef __GNUC__
	ANSI_PRAGMA_WEAK2(_getdents64,getdents64,function)
#endif

#endif


