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

#ifndef	_SYS_MACHCPUVAR_H
#define	_SYS_MACHCPUVAR_H

#ifndef _ASM
# include <sys/types.h>
# include <sys/machparam.h>
# include <sys/intr.h>
#endif

#include <sys/privregs.h>
#include <sys/machlock.h>
#include <sys/machpcb.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	_ASM

#include <sys/avintr.h>
#include <sys/obpdefs.h>
#include <sys/fm/protocol.h>

/*
 * The OpenBoot Standalone Interface supplies the kernel with
 * implementation dependent parameters through the devinfo/property mechanism
 */
#define	MAXSYSNAME	20

struct cpu_node {
	char	name[MAXSYSNAME];
	int	implementation;
	int	version;
	int	portid;
	pnode_t	nodeid;
	union {
		int	dummy;
	}	u_info;
	uint64_t	device_id;
};

extern struct cpu_node cpunodes[];

/*
 * Machine specific fields of the cpu struct
 * defined in common/sys/cpuvar.h.
 */
struct	machcpu {
	machpcb_t     	*mpcb;
	void		*prefix;	/* Pointer to prefix page */
	uint64_t	mpcb_pa;
	uint64_t	intrstat[PIL_MAX+1][2];
	uint64_t	pil_high_start[HIGH_LEVELS];
	int		mcpu_pri;	/* CPU Priority */
	uint8_t		intrcnt;	/* number of back-to-back interrupts */
	uint32_t	idling;  	/* CPU is idling */
	u_longlong_t	tmp1;		/* per-cpu tmps */
	u_longlong_t	tmp2;		/*  used in trap processing */

	void		*func;		/* Function to be called */
	uint64_t	arg1;		/* Argument 1 for signal call */
	uint64_t	arg2;		/* Argument 2 for signal call */
	
	uint64_t	clk;		/* Clock when sckc performed */
	uint64_t	ckc;		/* Clock comparator value */

	caddr_t		traceTbl;	/* Pointer to first trace table */
	size_t 		lTraceTbl;	/* Length of trace table */

	kmutex_t	sigMut;		/* Mutex to serialize signal calls */
	ksema_t		sigSem;		/* Signal received indicator */

	struct intr_vec	*intr_head[PIL_LEVELS];	/* intr queue heads per pil */
	struct intr_vec	*intr_tail[PIL_LEVELS];	/* intr queue tails per pil */

	struct softint  mcpu_softinfo;

	boolean_t	poke_cpu_outstanding;
	struct hat	*current_hat;

	struct hat_cpu_info	*mcpu_hat_info;

	/*
	 * The cpu module allocates a private data structure for the
	 * E$ data, which is needed for the specific cpu type.
	 */
	void		*cpu_private;		/* ptr to cpu private data */
};

typedef	struct machcpu	machcpu_t;

#define	cpu_pri 		cpu_m.mcpu_pri
#define	cpu_softinfo		cpu_m.mcpu_softinfo
#define cpu_current_hat		cpu_m.current_hat
#define cpu_hat_info		cpu_m.mcpu_hat_info

/*
 * Macro to access the "cpu private" data structure.
 */
#define	CPU_PRIVATE(cp)		((cp)->cpu_m.cpu_private)

#define	NINTR_THREADS	(LOCK_LEVEL)	/* number of interrupt threads */

#endif	/* _ASM */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_MACHCPUVAR_H */
