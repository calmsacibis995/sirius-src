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
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_CPU_MDESC_H
#define	_CPU_MDESC_H

#pragma ident	"@(#)cpu_mdesc.h	1.3	06/02/13 SMI"

#include <sys/types.h>
#include <sys/mdesc.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct md_cpumap {
	uint32_t cpumap_id;
	uint32_t cpumap_pid;
	uint64_t cpumap_serialno;
} md_cpumap_t;

typedef struct cpu {
	md_cpumap_t *cpu_mdesc_cpus;	/* head of ptr list for cpu maps */
	uint32_t cpu_mdesc_ncpus;	/* number of cpu maps */
} cpu_t;

extern cpu_t cpu;

extern int cpu_get_serialid_mdesc(uint32_t, uint64_t *);
extern int cpu_mdesc_init(void);
extern void cpu_mdesc_fini(void);
extern md_t *mdesc_devinit(size_t *);

#ifdef __cplusplus
}
#endif

#endif	/* _CPU_MDESC_H */
