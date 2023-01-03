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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/asm_linkage.h>
#include <sys/stack.h>

#ifdef _KERNEL

#include <sys/privregs.h>
#include <sys/regset.h>
#include <sys/vis.h>
#include <sys/machthread.h>

#endif /* _KERNEL */

#if defined(lint)

#ifdef _KERNEL

/* ARGSUSED */
void
SHA1TransformVIS(uint64_t *X0, uint64_t *blk, uint32_t *cstate, uint64_t *VIS)
{}

#else /* defined(lint) */

	ENTRY(SHA1TransformVIS)
/* S390X FIXME - use KLMD instructions to accomplish this */
	lghi	%r2,0
	br	%r14
	SET_SIZE(SHA1TransformVIS)

#endif	/* defined(lint) */
