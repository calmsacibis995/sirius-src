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
 *	Copyright (c) 1991,2001 by Sun Microsystems, Inc.
 *	All rights reserved.
 */
#ident	"@(#)lddstub.s	1.3	05/06/08 SMI"

/*
 * Stub file for ldd(1).  Provides for preloading shared libraries.
 */
#include <sys/syscall.h>

/*
 * Trap number for system calls
 */
#define SYSCALL_TRAPNUM 0

	.file		"lddstub.s"
	.section	".text"
	.global		stub

stub:	lghi	%r2,0
	lghi	%r0,SYS_exit
	svc	SYSCALL_TRAPNUM
