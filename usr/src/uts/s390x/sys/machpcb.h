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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_MACHPCB_H
#define	_SYS_MACHPCB_H

#include <sys/stack.h>
#include <sys/regset.h>
#if defined(__lint)
#include <sys/thread.h>
#endif /* __lint */

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * This file is machine dependent.
 */

/*
 * Machine dependent per-thread data.
 */

#define	MAXWIN	8	/* max # of windows currently supported - not required for System z */
			/* Just needs to be defined until System z dtrace functions are     */
			/* completed.							    */

/*
 * The system actually supports one more than the above number.
 * There is always one window reserved for trap handlers that
 * never has to be saved into the pcb struct.
 */

/*
 * Distance from beginning of thread stack (t_stk) to saved regs struct.
 */
#define	REGOFF	SA(MINFRAME)

#ifndef _ASM

/*
 * The struct machpcb is always allocated stack aligned.
 */
typedef struct machpcb {
	char	 mpcb_frame[REGOFF];
	mcontext_t mpcb_ctx;		/* user's saved registers */
	caddr_t	 mpcb_illexcaddr; 	/* address of last illegal instruction */
	uint64_t mpcb_illexcinsn; 	/* last illegal instruction */
	int	 mpcb_flags;		/* various state flags */
	kthread_t *mpcb_thread;		/* associated thread */
	uint64_t mpcb_pa;		/* pcb physical */
} machpcb_t;
#endif /* ! _ASM */

/*
 * We can use lwp_regs to find the mpcb base.
 */
#ifndef _ASM
#define	lwptompcb(lwp)	((struct machpcb *) \
	    ((caddr_t)(lwp)->lwp_regs - REGOFF))
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_MACHPCB_H */
