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
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

	.file	"%M%"

/* C library -- ntp_adjtime					*/
/* int ntp_adjtime (struct timex *);				*/

#include <sys/asm_linkage.h>

#ifndef __GNUC__
	ANSI_PRAGMA_WEAK(ntp_adjtime,function)
#endif

#include "SYS.h"

	SYSCALL_RVAL1(ntp_adjtime)
	RET
	SET_SIZE(ntp_adjtime)

#ifdef __GNUC__
	ANSI_PRAGMA_WEAK2(_ntp_adjtime,ntp_adjtime,function)
#endif
