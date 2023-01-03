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
 *	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
 *	Copyright (c) 1988 AT&T
 *	  All Rights Reserved
 */

/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Establish the default settings for the floating-point state for a C language
 * program:
 *	rounding mode		-- round to nearest default by OS,
 *	exceptions enabled	-- all masked
 *	sticky bits		-- all clear by default by OS.
 *      precision control       -- double extended
 * Set _fp_hw according to what floating-point hardware is available.
 * Set _sse_hw according to what SSE hardware is available.
 * Set __flt_rounds according to the rounding mode.
 */

#pragma weak _fpstart = __fpstart

#include	"lint.h"
#include	<sys/types.h>
#include	"fp.h"

void
__fpstart()
{
	struct fpc cw;

	memset(&cw, 0, sizeof(cw));
	_putcw((struct fpc *) &cw);

	/*
	 * At this point the fp environment that has been (or more
	 * hopefully, will be) established by the kernel is a default of 0
	 * which conforms to the s390 ABI definition.
	 */
}
