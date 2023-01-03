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
 *                                                                
 * Copyright 2008 Sine Nomine Associates.                          
 * All rights reserved.                                          
 * Use is subject to license terms.                             
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_TRAPTRACE_H
#define	_SYS_TRAPTRACE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * System z defines the TRAC and TRACG instructions to create
 * trap records in a buffer pointed at by control register 12.
 * The program interrupt handler will manage CR12 so that it
 * works as a circular buffer pointer.
 */

#ifndef	_ASM

struct trap_trace_record {
	uint64		tt_tod;		// TOD clock when trap created
	uint32_t	tt_tt;		// Trace id
	uint32_t	tt_xx;
	uint64_t	tt_regs[16];	// Registers at TRAP time
};

#endif

#ifndef	_ASM

#ifdef _KERNEL


#endif

/*
 * freeze the trap trace
 */
#define	TRAPTRACE_FREEZE	trap_freeze = 1;
#define	TRAPTRACE_UNFREEZE	trap_freeze = 0;

#else /* _ASM */

#include <sys/machthread.h>

#endif	/* _ASM */


#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_TRAPTRACE_H */
