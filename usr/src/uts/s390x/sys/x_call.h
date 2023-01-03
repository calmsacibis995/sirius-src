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

#ifndef	_SYS_X_CALL_H
#define	_SYS_X_CALL_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _ASM

#include <sys/cpuvar.h>

#if defined(_KERNEL)

#if defined(_MACHDEP)
#define	CPU_XCALL_READY(cpuid)			\
	(CPU_IN_SET(cpu_ready_set, (cpuid)))

extern cpuset_t cpu_ready_set;	/* cpus ready for x-calls */
#endif /* _MACHDEP */

#if defined(TRAPTRACE)
# define	XC_TRACE(type, cpu, func, arg1, arg2) \
		xc_trace((type), (cpu), (func), (arg1), (arg2))
#else /* !TRAPTRACE */
# define	XC_TRACE(type, cpu, func, arg1, arg2)
#endif /* TRAPTRACE */

#if defined(DEBUG) || defined(TRAPTRACE)
/*
 * get some statistics when xc/xt routines are called
 */

#define	XC_STAT_INC(a)	(a)++;

#else

#define XC_STAT_INC(a)

#endif

#define	XC_CPUID	0

#define	XT_ALL_SELF	1
#define	XT_ALL_OTHER	2
#define	XC_ONE_SELF	3
#define	XC_ONE_OTHER	4
#define	XC_SOME_SELF	5
#define	XC_SOME_OTHER	6
#define	XC_ALL_SELF	7
#define	XC_ALL_OTHER	8
#define	XC_SERV		9
#define XC_MAX		10
extern	uint_t x_dstat[NCPU][XC_MAX];
extern	uint_t x_rstat[NCPU][4];
typedef int64_t xc_arg_t;

#define	XC_STAT_INIT(cpuid) 				\
{							\
	x_dstat[cpuid][XC_CPUID] = 0xffffff00 | cpuid;	\
	x_rstat[cpuid][XC_CPUID] = 0xffffff00 | cpuid;	\
}

#define XC_SOFT_PIL	10 

#define XC_WAKE_FN	(void *) -1

/*
 * Cross-call function prototype.
 */
typedef void xcfunc_t(uint64_t, uint64_t);

/*
 * Cross-call routines.
 */
extern uint_t xc_serv(void);
extern void xc_one(int, xcfunc_t *, uint64_t, uint64_t);
extern void xc_init(struct cpu *);
extern void xc_all(xcfunc_t *, uint64_t, uint64_t);
extern void xc_some(cpuset_t, xcfunc_t *, uint64_t, uint64_t);

#endif	/* _KERNEL */

#endif	/* !_ASM */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_X_CALL_H */
