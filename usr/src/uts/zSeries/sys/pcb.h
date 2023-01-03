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
 * Copyright 1990-2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_PCB_H
#define	_SYS_PCB_H

#include <sys/regset.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Sun software process control block
 */

#ifndef _ASM
typedef struct pcb {
	int	 pcb_flags;	/* various state flags; cleared on fork */
	int	 pcb_step;	/* Step setting */
	uint64_t pcb_instr;	/* /proc: instruction at stop */
	caddr_t	 pcb_tracepc;	/* used while single-stepping */
	/*
	 * Keep these fields together so lctlg %c9,%c11,pcb_mask will work
	 */
	uint64_t pcb_mask;	/* PER trace mask */
	caddr_t  pcb_start;	/* Start of watch area */
	caddr_t  pcb_end;	/* End of watch area */
} pcb_t;
#endif /* ! _ASM */

/* pcb_flags */
#define	DEBUG_PENDING	0x02	/* single-step of lcall for a sys call */
#define	INSTR_VALID	0x08	/* value in pcb_instr is valid (/proc) */
#define	NORMAL_STEP	0x10	/* normal debugger-requested single-step */
#define	WATCH_STEP	0x20	/* single-stepping in watchpoint emulation */
#define	CPC_OVERFLOW	0x40	/* performance counters overflowed */

/* pcb_mask */
#define EVM_BRANCH	0x80000000	/* Successful branch tracing */
#define EVM_FETCH	0x40000000	/* Instruction fetching event */
#define EVM_WATCH	0x20000000	/* Storage alteration event */
#define EVM_STURA	0x08000000	/* Store using real address event */
#define EVM_INSNULL	0x01000000	/* Instruction nullification */
#define CTL_BRANCH	0x00800000	/* Trace branches only in watch area */
#define CTL_SASC	0x00200000	/* Watch storage only in address space */

#define ASCE_SAE	0x80		/* Storage alterationm event bit */

/* pcb_step */
#define	STEP_NONE	0	/* no single step */
#define	STEP_REQUESTED	1	/* arrange to single-step the lwp */
#define	STEP_ACTIVE	2	/* actively patching addr, set active flag */
#define	STEP_WASACTIVE	3	/* wrap up after taking single-step fault */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_PCB_H */
