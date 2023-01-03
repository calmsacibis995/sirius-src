/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

	.file	"%M%"

/*
 * int __fcntl_syscall(int fildes, int cmd [, arg])
 */

#include "SYS.h"

	SYSCALL2_RESTART_RVAL1(__fcntl_syscall,fcntl)
	RET
	SET_SIZE(__fcntl_syscall)

#ifdef __GNUC__
	ANSI_PRAGMA_WEAK2(__fcntl,__fcntl_syscall,function)
	ANSI_PRAGMA_WEAK2(_fcntl,__fcntl_syscall,function)
	ANSI_PRAGMA_WEAK2(fcntl,__fcntl_syscall,function)
#endif
