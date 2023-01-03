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
 * Copyright (c) 1998, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)alloca.s	1.15	05/06/08 SMI"

	.file	"alloca.s"

#include <sys/asm_linkage.h>
#include <sys/stack.h>

	/*
	!
	! r2: # bytes of space to allocate, already rounded to 0 mod 8
	!
	! we want to bump %sp by the requested size
	!
	*/
	ENTRY(__builtin_alloca)
	lr	%r1,%r2
	ahi	%r1,14
	sra	%r1,3
	sla	%r1,3
	lr	%r2,%r15
	sr	%r15,%r1
	br	%r14
	SET_SIZE(__builtin_alloca)
