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

#ifndef _SYS_MACHSYSTM_H
#define	_SYS_MACHSYSTM_H

/*
 * Numerous platform-dependent interfaces that don't seem to belong
 * in any other header file.
 *
 * This file should not be included by code that purports to be
 * platform-independent.
 *
 */

#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/varargs.h>
#include <sys/thread.h>
#include <sys/cpuvar.h>
#include <sys/memlist_plat.h>
#include <sys/dditypes.h>
#include <vm/page.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _KERNEL

/*
 * The following enum types determine how interrupts are distributed
 * on a s390x system.
 */
enum intr_policies {
	/*
	 * Target interrupt at the CPU running the add_intrspec
	 * thread. Also used to target all interrupts at the panicing
	 * CPU.
	 */
	INTR_CURRENT_CPU = 0,

	/*
	 * Target all interrupts at the boot cpu
	 */
	INTR_BOOT_CPU,

	/*
	 * Flat distribution of all interrupts
	 */
	INTR_FLAT_DIST,

	/*
	 * Weighted distribution of all interrupts
	 */
	INTR_WEIGHTED_DIST
};

struct panic_trap_info {
	struct regs *trap_regs;
	uint_t	trap_type;
	caddr_t trap_addr;
};

/*
 * Structure that defines the interrupt distribution list. It contains
 * enough info about the interrupt so that it can callback the parent
 * nexus driver and retarget the interrupt to a different CPU.
 */
struct intr_dist {
	struct intr_dist *next;	/* link to next in list */
	void (*func)(void *);	/* Callback function */
	void *arg;		/* Nexus parent callback arg 1 */
};

extern processorid_t getbootcpuid(void);
extern void mp_halt(char *);
extern void mach_cpu_idle(void);
extern void mach_cpu_pause(volatile char *);
extern int  mach_cpu_start(struct cpu *, void *);
extern void mach_cpu_stop(int);

extern int Cpudelay;
extern void setcpudelay(void);

extern void init_intr_threads(struct cpu *);
extern void init_clock_thread(void);

extern void ndata_alloc_init(struct memlist *, uintptr_t, uintptr_t);
extern void *ndata_alloc(struct memlist *, size_t, size_t);
extern void *ndata_extra_base(struct memlist *, size_t);
extern int ndata_alloc_rsp(struct memlist *);

extern caddr_t alloc_page_freelists(int, caddr_t, int);
extern caddr_t page_ctrs_alloc(caddr_t);

extern size_t ndata_maxsize(struct memlist *);
extern size_t ndata_spare(struct memlist *, size_t, size_t);
extern int kcpc_hw_load_pcbe(void);
extern void kcpc_hw_init(cpu_t *cp);
extern int kcpc_hw_overflow_intr_installed;
extern void kcpc_hw_startup_cpu(ushort_t);

extern struct system_hardware system_hardware;
extern void get_system_configuration(void);
extern void mmu_init(void);
extern void map_kaddr(caddr_t, pfn_t, int, int);
extern void memscrub_init(void);

extern int use_mp;

extern struct cpu	cpus[];		/* pointer to other cpus */
extern struct cpu	*cpu[];		/* pointer to all cpus */

/*
 * externally initiated panic
 */
extern struct regs sync_reg_buf;
extern uint64_t sync_tt;
extern void sync_handler(void);

/* kpm mapping window */
extern size_t   kpm_size;
extern uchar_t  kpm_size_shift;
extern caddr_t  kpm_vbase;

extern void intr_dist_add(void (*f)(void *), void *);
extern void intr_dist_rem(void (*f)(void *), void *);
extern void intr_dist_add_weighted(void (*f)(void *, int32_t, int32_t), void *);
extern void intr_dist_rem_weighted(void (*f)(void *, int32_t, int32_t), void *);

extern uint32_t intr_dist_cpuid(void);
extern uint32_t intr_dist_mycpuid(void);

void intr_dist_cpuid_add_device_weight(uint32_t cpuid, dev_info_t *dip,
		int32_t weight);
void intr_dist_cpuid_rem_device_weight(uint32_t cpuid, dev_info_t *dip);

extern void intr_redist_all_cpus(void);
extern void intr_redist_all_cpus_shutdown(void);

extern void send_dirint(int, int);
extern void setsoftint_tl1(uint64_t, uint64_t);
extern void siron(void);

extern void trap_cleanup(struct regs *, uint_t, k_siginfo_t *);

extern int  frogr(uint16_t);

#endif /* _KERNEL */

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_MACHSYSTM_H */
