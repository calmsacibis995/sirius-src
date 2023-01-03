/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mp_startup.c					    */
/* 								    */
/* Function    - Start up other CPUs in this complex.               */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - May, 2007   					    */
/* 								    */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/*                   L I C E N S E                                  */
/*------------------------------------------------------------------*/

/*==================================================================*/
/* 								    */
/* CDDL HEADER START						    */
/* 								    */
/* The contents of this file are subject to the terms of the	    */
/* Common Development and Distribution License                      */
/* (the "License").  You may not use this file except in compliance */
/* with the License.						    */
/* 								    */
/* You can obtain a copy of the license at: 			    */
/* - usr/src/OPENSOLARIS.LICENSE, or,				    */
/* - http://www.opensolaris.org/os/licensing.			    */
/* See the License for the specific language governing permissions  */
/* and limitations under the License.				    */
/* 								    */
/* When distributing Covered Code, include this CDDL HEADER in each */
/* file and include the License file at usr/src/OPENSOLARIS.LICENSE.*/
/* If applicable, add the following below this CDDL HEADER, with    */
/* the fields enclosed by brackets "[]" replaced with your own      */
/* identifying information: 					    */
/* Portions Copyright [yyyy] [name of copyright owner]		    */
/* 								    */
/* CDDL HEADER END						    */
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
/* 								    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/thread.h>
#include <sys/cpuvar.h>
#include <sys/sunddi.h>
#include <sys/t_lock.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/class.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/asm_linkage.h>
#include <sys/systm.h>
#include <sys/var.h>
#include <sys/vtrace.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg_kmem.h>
#include <vm/seg_kp.h>
#include <sys/kmem.h>
#include <sys/stack.h>
#include <sys/machsystm.h>
#include <sys/clock.h>
#include <sys/cpc_impl.h>
#include <sys/pg.h>
#include <sys/cmt.h>
#include <sys/dtrace.h>
#include <sys/archsystm.h>
#include <sys/reboot.h>
#include <sys/kdi_machimpl.h>
#include <vm/hat_s390x.h>
#include <sys/memnode.h>
#include <sys/sysmacros.h>
#include <sys/smp.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

struct mp_find_cpu_arg {
	int cpuid;		/* set by mp_cpu_configure() */
	dev_info_t *dip;	/* set by mp_find_cpu() */
};

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern void idle();
extern int get_portid_ddi(dev_info_t *, dev_info_t **);

extern struct cpu cpu0;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void	mp_startup(void);
static void	smp_setPrefix(struct cpu *);
static void 	*mp_get_pages(void *, int, pfn_t);
static void 	mp_free_pages(void *, int);

static void	cpu_sep_enable(void);
static void	cpu_sep_disable(void);
static void	cpu_asysc_enable(void);
static void	cpu_asysc_disable(void);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

cpu_core_t	cpu_core[NCPU];			/* cpu_core structures */

static cpuset_t procset;

/*
 * Useful for disabling MP bring-up for an MP capable kernel
 * (a kernel that was built with MP defined)
 */
int use_mp = 1;			/* set to come up mp */

/*
 * set to indicate what cpus are sitting around on the system.
 */
cpuset_t mp_cpus;

cpuset_t cpu_ready_set = 0;

int	cpu_are_paused = 0;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- smp_setPrefix.                                    */
/*                                                                  */
/* Function	- Set the prefix register of the other CPUs.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
smp_setPrefix(struct cpu *cp)
{
	if (sigp(cp->cpu_id, 
		 sigp_SetPrefix, 
		 cp->cpu_m.prefix, NULL) == sigp_NotOp)
		panic("Error setting prefix register for CPU %d\n",cp->cpu_id);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_startup_init.                                  */
/*                                                                  */
/* Function	- Allocate and initialize the CPU structure,        */
/*		  TRAPTRACE buffer, and the startup and idle 	    */
/*		  threads for the specified CPU.	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

struct cpu *
mp_startup_init(int cpun)
{
	struct cpu *cp;
	kthread_id_t tp;
	caddr_t	 sp;
	proc_t 	 *procp;
	_pfxPage *pfx;

	ASSERT(cpun < NCPU && cpu[cpun] == NULL);

	cp = kmem_zalloc(sizeof (struct cpu), KM_SLEEP);
	procp = curthread->t_procp;

	mutex_enter(&cpu_lock);
	/*
	 * Initialize the dispatcher first.
	 */
	disp_cpu_init(cp);
	mutex_exit(&cpu_lock);

	cpu_vm_data_init(cp);

	/*
	 * Allocate and initialize the startup thread for this CPU.
	 * Interrupt and process switch stacks get allocated later
	 * when the CPU starts running.
	 */
	tp = thread_create(NULL, 0, NULL, NULL, 0, procp,
			   TS_STOPPED, maxclsyspri);

	/*
	 * Set state to TS_ONPROC since this thread will start running
	 * as soon as the CPU comes online.
	 *
	 * All the other fields of the thread structure are setup by
	 * thread_create().
	 */
	THREAD_ONPROC(tp, cp);
	tp->t_preempt = 1;
	tp->t_bound_cpu = cp;
	tp->t_affinitycnt = 1;
	tp->t_cpu = cp;
	tp->t_disp_queue = cp->cpu_disp;

	/*
	 * Setup thread to start in mp_startup.
	 */
	sp       = tp->t_stk;
	tp->t_pc = (uintptr_t)mp_startup;
	tp->t_sp = (uintptr_t)(sp - MINFRAME);

	cp->cpu_id 	     = cpun;
	cp->cpu_self 	     = cp;
	cp->cpu_thread 	     = tp;
	cp->cpu_lwp 	     = NULL;
	cp->cpu_dispthread   = tp;
	cp->cpu_dispatch_pri = DISP_PRIO(tp);

	/*
	 * Allocate prefix page and initialize from current prefix.
	 */
	pfx = cp->cpu_m.prefix = mp_get_pages(cp, 2, 2);
	if (pfx == NULL)
		panic("Unable to obtain prefix page for CPU %d\n",cpun);

	bcopy(NULL, pfx, sizeof(*pfx));
	pfx->__lc_cpu = cp;

	/*
	 * cpu_base_spl must be set explicitly here to prevent any blocking
	 * operations in mp_startup from causing the spl of the cpu to drop
	 * to 0 (allowing device interrupts before we're ready) in resume().
	 * cpu_base_spl MUST remain at LOCK_LEVEL until the cpu is CPU_READY.
	 * As an extra bit of security on DEBUG kernels, this is enforced with
	 * an assertion in mp_startup() -- before cpu_base_spl is set to its
	 * proper value.
	 */
	cp->cpu_base_spl = ipltospl(LOCK_LEVEL);

	/* 
	 * Initialize the xcall environment for this CPU
	 */
	xc_init(cp);

	/*
	 * Now, initialize per-CPU idle thread for this CPU.
	 */
	tp = thread_create(NULL, PAGESIZE, idle, NULL, 0, procp, TS_ONPROC, -1);

	cp->cpu_idle_thread = tp;
	cp->cpu_m.idling    = 0;
	cp->cpu_m.poke_cpu_outstanding = 0;

	tp->t_preempt = 1;
	tp->t_bound_cpu = cp;
	tp->t_affinitycnt = 1;
	tp->t_cpu = cp;
	tp->t_disp_queue = cp->cpu_disp;

	/*
	 * Bootstrap the CPU's PG data
	 */
	pg_cpu_bootstrap(cp);

	/*
	 * Perform CPC initialization on the new CPU.
	 */
//	kcpc_hw_init(cp);	/* S390X FIXME */

#ifdef TRAPTRACE
	/*
	 * If this is a TRAPTRACE kernel, allocate TRAPTRACE buffers
	 */
	/* S390X FIXMEB */
#endif
	/*
	 * Record that we have another CPU.
	 */
	mutex_enter(&cpu_lock);

	/*
	 * Initialize interrupt related stuff
	 */
	cpu_intr_alloc(cp, NINTR_THREADS);

	/*
	 * Add CPU to list of available CPUs.  It'll be on the active list
	 * after mp_startup().
	 */
	cpu_add_unit(cp);

	mutex_exit(&cpu_lock);

	return (cp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_startup_fini.                                  */
/*                                                                  */
/* Function	- Reverse what was done in startup.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
mp_startup_fini(struct cpu *cp)
{
	page_t *pp;
	pfn_t  pfn;
	_pfxPage *pfx;

	mutex_enter(&cpu_lock);

	/*
	 * Remove the CPU from the list of available CPUs.
	 */
	cpu_del_unit(cp->cpu_id);

	/*
	 * At this point, the only threads bound to this CPU should
	 * special per-cpu threads: it's idle thread, it's pause threads,
	 * and it's interrupt threads.  Clean these up.
	 */
	cpu_destroy_bound_threads(cp);
	cp->cpu_idle_thread = NULL;

	/*
	 * Free the interrupt stack.
	 */
	segkp_release(segkp,
	    cp->cpu_intr_stack - (INTR_STACK_SIZE - SA(MINFRAME)));

	mutex_exit(&cpu_lock);

#ifdef TRAPTRACE
	/*
	 * Discard the trap trace buffer
	 */
	{
	/* S390X FIXME */
	}
#endif

	cp->cpu_dispthread = NULL;
	cp->cpu_thread     = NULL;	/* discarded by cpu_destroy_bound_threads() */

	pfx = cp->cpu_m.prefix;
	if (pfx) 
		mp_free_pages(pfx, 2);
	
	if (cp->cpu_m.traceTbl)
		mp_free_pages(cp->cpu_m.traceTbl, 2);

	cpu_vm_data_destroy(cp);

	mutex_enter(&cpu_lock);
	disp_cpu_fini(cp);
	mutex_exit(&cpu_lock);

	kmem_free(cp, sizeof (*cp));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- start_cpu.                                        */
/*                                                                  */
/* Function	- Start a single CPU, assuming that the kernel      */
/*		  context is available to successfully start 	    */
/*		  another CPU.                 		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
start_cpu(processorid_t who)
{
	void *ctx;
	cpu_t *cp;
	int delays;
	int error = 0;

	/*
	 * Check if there's at least a Mbyte of kmem available
	 * before attempting to start the cpu.
	 */
	if (kmem_avail() < 1024 * 1024) {
		/*
		 * Kick off a reap in case that helps us with
		 * later attempts ..
		 */
		kmem_reap();
		return (ENOMEM);
	}

	cp = mp_startup_init(who);
	if ((error = mach_cpu_start(cp, mp_startup)) != 0) {
		mp_startup_fini(cp);
		return (error);
	}

	for (delays = 0; !CPU_IN_SET(procset, who); delays++) {
		prom_printf("Waiting for CPU %d to be readied\n",who);
		if (delays == 500) {
			/*
			 * After five seconds, things are probably looking
			 * a bit bleak - explain the hang.
			 */
			cmn_err(CE_NOTE, "cpu%d: started, "
			    "but not running in the kernel yet", who);
		} else if (delays > 2000) {
			/*
			 * We waited at least 20 seconds, bail ..
			 */
			error = ETIMEDOUT;
			cmn_err(CE_WARN, "cpu%d: timed out", who);
			mp_startup_fini(cp);
			return (error);
		}

		/*
		 * wait at least 10ms, then check again..
		 */
		delay(USEC_TO_TICK_ROUNDUP(10000));
	}

	if (dtrace_cpu_init != NULL) {
		/*
		 * DTrace CPU initialization expects cpu_lock to be held.
		 */
		mutex_enter(&cpu_lock);
		(*dtrace_cpu_init)(who);
		mutex_exit(&cpu_lock);
	}

	sema_p(&cp->cpu_m.sigSem);	// Wait until other CPU ready

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- start_other_cpus.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
start_other_cpus(int cprboot)
{
	uint_t who;
	uint_t skipped = 0;
	uint_t bootcpuid = getprocessorid();

	/*
	 * Initialize our own cpu_info.
	 */
	init_cpu_info(CPU);

	/*
	 * Take the boot cpu out of the mp_cpus set because we know
	 * it's already running.  Add it to the cpu_ready_set for
	 * precisely the same reason.
	 */
	CPUSET_DEL(mp_cpus, bootcpuid);
	CPUSET_ADD(cpu_ready_set, bootcpuid);

	/*
	 * if only 1 cpu or not using MP, skip the rest of this
	 */
	if (CPUSET_ISNULL(mp_cpus) || use_mp == 0) {
		if (use_mp == 0)
			cmn_err(CE_CONT, "?***** Not in MP mode\n");
		return;
	}

	/*
	 * perform such initialization as is needed
	 * to be able to take CPUs on- and off-line.
	 */
	cpu_pause_init();

	/*
	 * We lock our affinity to the master CPU to ensure that all slave CPUs
	 * do their TSC syncs with the same CPU.
	 */
	affinity_set(CPU_CURRENT);

	for (who = 0; who < NCPU; who++) {

		if (!CPU_IN_SET(mp_cpus, who))
			continue;
		ASSERT(who != bootcpuid);
		if (ncpus >= max_ncpus) {
			skipped = who;
			continue;
		}
		if (start_cpu(who) != 0)
			CPUSET_DEL(mp_cpus, who);
	}

	affinity_clear();

	if (skipped) {
		cmn_err(CE_NOTE,
		    "System detected %d cpus, but "
		    "only %d cpu(s) were enabled during boot.",
		    skipped + 1, ncpus);
		cmn_err(CE_NOTE,
		    "Use \"boot-ncpus\" parameter to enable more CPU(s). "
		    "See eeprom(1M).");
	}

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_startup.                                       */
/*                                                                  */
/* Function	- Start function for non-boot CPUs.                 */
/*		                               		 	    */
/*		  WARNING: Until CPU_READY is set, mp_startup and   */
/*		  routines called by mp_startup should not call     */
/*		  routines (e.g. kmem_free) that could call         */
/*		  hat_unload which requires CPU_READY to be set.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mp_startup(void)
{
	struct cpu *cp = CPU;
	ctlr12 	cr12;

	/* 
	 * Allocate a trace table and set cr12 
	 */
	cr12.data.tbl = cp->cpu_m.traceTbl = mp_get_pages(cp, TRACETBL_SIZE, 1);
	cr12.data.bits.brTrc = 0;
	cr12.data.bits.mdTrc = 0;
	cr12.data.bits.exTrc = 1;
	SET_TRC_CR(cr12);
	cp->cpu_m.lTraceTbl = TRACETBL_SIZE * MMU_PAGESIZE;

	/*
	 * We need to get TSC on this proc synced (i.e., any delta
	 * from cpu0 accounted for) as soon as we can, because many
	 * many things use gethrtime/pc_gethrestime, including
	 * interrupts, cmn_err, etc.
	 */

	/* Let cpu0 continue into tsc_sync_master() */
	CPUSET_ATOMIC_ADD(procset, cp->cpu_id);

	/*
	 * Enable interrupts with spl set to LOCK_LEVEL. LOCK_LEVEL is the
	 * highest level at which a routine is permitted to block on
	 * an adaptive mutex (allows for cpu poke interrupt in case
	 * the cpu is blocked on a mutex and halts). Setting LOCK_LEVEL blocks
	 * device interrupts that may end up in the hat layer issuing cross
	 * calls before CPU_READY is set.
	 */
	splx(ipltospl(LOCK_LEVEL));
	sti();

	init_cpu_info(cp);

	mutex_enter(&cpu_lock);

	/*
	 * Processor group initialization for this CPU is dependent on the
	 * cpuid probing, which must be done in the context of the current
	 * CPU.
	 */
	pghw_physid_create(cp);
	pg_cpu_init(cp);
	pg_cmt_cpu_startup(cp);

	cp->cpu_flags |= CPU_RUNNING | CPU_READY | CPU_ENABLE | CPU_EXISTS;
	cpu_add_active(cp);

	splx(ipltospl(LOCK_LEVEL));

	if (dtrace_cpu_init != NULL) {
		(*dtrace_cpu_init)(cp->cpu_id);
	}

	mutex_exit(&cpu_lock);

	/*
	 * Enable preemption here so that contention for any locks acquired
	 * later in mp_startup may be preempted if the thread owning those
	 * locks is continously executing on other CPUs (for example, this
	 * CPU must be preemptible to allow other CPUs to pause it during their
	 * startup phases).  It's safe to enable preemption here because the
	 * CPU state is pretty-much fully constructed.
	 */
	curthread->t_preempt = 0;

	add_cpunode2devtree(cp->cpu_id);

	/*
	 * Setting the bit in cpu_ready_set must be the last operation in
	 * processor initialization; the boot CPU will continue to boot once
	 * it sees this bit set for all active CPUs.
	 */
	CPUSET_ATOMIC_ADD(cpu_ready_set, cp->cpu_id);

	cyclic_mp_init();

	/* The base spl should still be at LOCK LEVEL here */
	ASSERT(cp->cpu_base_spl == ipltospl(LOCK_LEVEL));
	set_base_spl();		/* Restore the spl to its proper value */

	(void) spl0();				/* enable interrupts */

//	if (boothowto & RB_DEBUG)		/* S390X FIXME */
//		kdi_cpu_init();

	/*
	 * Because mp_startup() gets fired off after init() starts, we
	 * can't use the '?' trick to do 'boot -v' printing - so we
	 * always direct the 'cpu .. online' messages to the log.
	 */
	cmn_err(CE_CONT, "!cpu%d initialization complete - online\n",
		cp->cpu_id);

	/*
	 * Tell initialiting process that we've started
	 */
	sema_v(&cp->cpu_m.sigSem);

	/*
	 * Now we are done with the startup thread, so free it up.
	 */
	prom_printf("CPU %d trace table starts at %lx\n",
		    cp->cpu_id,cp->cpu_m.traceTbl);
	thread_exit();
	panic("mp_startup: cannot return");
	/*NOTREACHED*/
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- Start CPU on user request.                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
int
mp_cpu_start(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_cpu_stop.                                      */
/*                                                                  */
/* Function	- Stop CPU on user request.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
int
mp_cpu_stop(struct cpu *cp)
{
	extern int cbe_psm_timer_mode;
	ASSERT(MUTEX_HELD(&cpu_lock));

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_cpu_unconfigure.                               */
/*                                                                  */
/* Function	- Routine used to cleanup a CPU that has been       */
/*		  removed from the configuration. This will destroy */
/*		  all per-cpu information related to this CPU.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
mp_cpu_unconfigure(int cpuid)
{
	ASSERT(MUTEX_HELD(&cpu_lock));

	mp_startup_fini(cpu[cpuid]);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_cpu_configure.                                 */
/*                                                                  */
/* Function	- Setup a newly inserted CPU in preparation for     */
/*		  starting it running code.    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
mp_cpu_configure(short cpuid)
{
	struct mp_find_cpu_arg target;

	ASSERT(MUTEX_HELD(&cpu_lock));

	target.dip = NULL;
	target.cpuid = cpuid;

	if (target.dip == NULL)
		return (ENODEV);

	start_cpu(cpuid);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- init_cpu_info.                                    */
/*                                                                  */
/* Function	- Initialize CPU information.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
init_cpu_info(struct cpu *cp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- stop_other_cpus.                                  */
/*                                                                  */
/* Function	- Stop all other CPUs before halting or rebooting.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
stop_other_cpus(void)
{
	int who;

	prom_printf("Stopping other CPUs\n");
	mutex_enter(&cpu_lock);
	if (cpu_are_paused) {
		mutex_exit(&cpu_lock);
		prom_printf("CPUs are stopped\n");
		return;
	}

	for (who = 0; who < NCPU; who++) {

		if (who != CPU->cpu_id && cpu[who] != NULL &&
		    (cpu[who]->cpu_flags & CPU_EXISTS)) { 

			if (!CPU_IN_SET(cpu_ready_set, who))
				continue;
		
			sigp(cpu[who]->cpu_id, sigp_StopStoreStatus, NULL, NULL);
			CPUSET_DEL(mp_cpus, who);
		}
	}

	cpu_are_paused = 1;
	prom_printf("Other CPUs are stopped\n");

	mutex_exit(&cpu_lock);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_get_pages.                                     */
/*                                                                  */
/* Function	- Get contiguous pages for use by a CPU.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void *
mp_get_pages(void *id, int nPages, pfn_t bndry)
{
	page_t *pp, *npp;
	static struct seg tmpseg;
	
	if (page_resv(nPages, KM_NOSLEEP) != 0) {

		pp = page_create_contig(&kvp, id, (nPages * MMU_PAGESIZE),
					PG_EXCL | PG_NORELOC, &tmpseg, 2, 0);

		if (pp != NULL) {

			npp = pp;
			do {
				page_io_unlock(npp);
				page_hashout(npp, NULL);

				page_downgrade(npp);
				ASSERT(PAGE_SHARED(npp));

				npp = npp->p_next;
			} while (npp != pp);
			return ((void *) (pp->p_pagenum << MMU_PAGESHIFT));
		}
	}
	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_get_pages.                                     */
/*                                                                  */
/* Function	- Free pages used by a CPU.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void 
mp_free_pages(void *ra, int nPages)
{
	page_t *pp, *npp;
	pfn_t  pfn;

	pfn = (uintptr_t) ra >> MMU_PAGESHIFT;
	if (pfn != 0) {
		pp  = page_numtopp_nolock(pfn);
		if (pp == NULL)
			panic("ptable_free(): no page for pfn!");
		
		page_free(pp, 1);
		page_unresv(nPages);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_disable_intr.                                 */
/*                                                                  */
/* Function	- Take the specified CPU out of participation in    */
/*		  interrupts.                  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
cpu_disable_intr(struct cpu *cp)
{
	char mask;

	__asm__ ("	stnsm	%0,0xfc\n"
		 : "=m" (mask));

	cp->cpu_flags &= ~CPU_ENABLE;
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_enable_intr.                                  */
/*                                                                  */
/* Function	- Allow the specified CPU to participate in         */
/*		  interrupts.                  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cpu_enable_intr(struct cpu *cp)
{
	ASSERT(MUTEX_HELD(&cpu_lock));
	cp->cpu_flags |= CPU_ENABLE;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_cpu_faulted_enter.                             */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
mp_cpu_faulted_enter(struct cpu *cp)
{
	cpu_faulted_enter(cp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_cpu_faulted_exit.                              */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
mp_cpu_faulted_exit(struct cpu *cp)
{
	cpu_faulted_enter(cp);
}

/*========================= End of Function ========================*/

