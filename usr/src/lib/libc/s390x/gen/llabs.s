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
 * Copyright (c) 1997 Sun Microsystems, Inc.
 */
	.file	"llabs.s"

#include <sys/asm_linkage.h>

/*
 * long long llabs(register long long arg);
 * long labs(register long int arg);
 */
	ENTRY2(labs,llabs)
	lpgr	%r2,%r2
	br	%r14	

	SET_SIZE(labs)
	SET_SIZE(llabs)

#undef llabs
	ANSI_PRAGMA_WEAK2(_llabs,llabs,function)
