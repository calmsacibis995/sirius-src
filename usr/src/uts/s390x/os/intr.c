/*------------------------------------------------------------------*/
/* 								    */
/* Name        - intr.c     					    */
/* 								    */
/* Function    - Process interrupts and dispatch threads to handle  */
/* 	 	 the work request.				    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - June, 2007  					    */
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
#include <sys/asm_linkage.h>
#include <sys/cpuvar.h>
#include <sys/regset.h>
#include <sys/thread.h>
#include <sys/systm.h>
#include <sys/trap.h>
#include <sys/clock.h>
#include <sys/panic.h>
#include <sys/disp.h>
#include <vm/seg_kp.h>
#include <sys/stack.h>
#include <sys/sysmacros.h>
#include <sys/cmn_err.h>
#include <sys/kstat.h>
#include <sys/zone.h>
#include <sys/bitmap.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/ontrap.h>
#include <sys/trap.h>
#include <sys/fault.h>
#include <sys/promif.h>
#include <sys/smp_impldefs.h>
#include <sys/intr.h>
#include <sys/sdt.h>
#include <sys/exts390x.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern void mutex_exit_critical_start();
extern long mutex_exit_critical_size;
extern void mutex_owner_running_critical_start();
extern long mutex_owner_critical_size;
extern int tudebug;
extern void _sys_rtt();
extern struct av_head autovect[];

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static int add_spl(int, int, int, int);
static int del_spl(int, int, int, int);
static int slvl_to_vect(int);
uint64_t intr_get_time(void);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

#ifdef DEBUG
int intr_thread_cnt;
#endif

int (*addspl)(int, int, int, int) = add_spl;
int (*delspl)(int, int, int, int) = del_spl;
int (*slvltovect)(int) = slvl_to_vect;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dispatch_autovect.                                */
/*                                                                  */
/* Function	- This is a modified version of the autovect dis-   */
/*                patch routine that allows claiming interrupts     */
/*		  without running any other handlers.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
dispatch_autovect(intparms *ip)
{
	struct autovec *av;
	int vec = ip->vector;

	ASSERT_STACK_ALIGNED();

	for (av = autovect[vec].avh_link; av != NULL; av = av->av_link) {
		uint_t r;
		uint_t (*intr)() = av->av_vector;

		if (intr == NULL) {
			continue;
		}

		ip->arg2 = av->av_intarg2;
		caddr_t arg1 = av->av_intarg1;
		caddr_t arg2 = (caddr_t) ip;
		dev_info_t *dip = av->av_dip;

		DTRACE_PROBE4(interrupt__start, dev_info_t *, dip,
		    void *, intr, caddr_t, arg1, caddr_t, arg2);

		r = (*intr)(arg1, arg2);

		DTRACE_PROBE4(interrupt__complete, dev_info_t *, dip,
		    void *, intr, caddr_t, arg1, uint_t, r);

		if (av->av_ticksp && av->av_prilevel <= LOCK_LEVEL) {
			atomic_add_64(av->av_ticksp, intr_get_time());
		}

		if (r == DDI_INTR_CLAIMED) {
			break;
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- set_base_spl.                                     */
/*                                                                  */
/* Function	- Set CPU's base SPL level to the highest active    */
/*		  interrupt level.             		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
set_base_spl(void)
{
	struct cpu *cpu = CPU;
	uint16_t active = (uint16_t)cpu->cpu_intr_actv;

	cpu->cpu_base_spl = active == 0 ? 0 : flogr(active);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_entr.                                        */
/*                                                                  */
/* Function	- Raise the level based on interrupt class and      */
/*		  subclass.  Returns with interrupts re-enabled.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
intr_enter(int oldipl, int *trapno, intparms *ip)
{
	int newipl;
	int subclass;

	bzero(ip, sizeof(*ip));

	switch (*trapno) {
	case S390_INTR_IO:
		ip->u.io.intparm = *((uint32_t *)__LC_IO_INTPARM);
		ip->u.io.idw = *((uint32_t *)__LC_IO_IDW);
		ip->u.io.schid = *((uint32_t *)__LC_SCHID);

		switch ((ip->u.io.idw >> 27) & 7) {
		case 0: case 1:
			newipl = PIL_1;
			break;
		case 2: case 3:
			newipl = PIL_2;
			break;
		case 4: case 5:
			newipl = PIL_3;
			break;
		case 6:
			newipl = PIL_4;
			break;
		case 7:
			newipl = PIL_5;
			break;
		}

		break;

	case S390_INTR_MCHK:
		ip->u.mch.u.intcode = *((uint64_t *)__LC_MC_INTCODE);

		if (ip->u.mch.u.mcic.chanRpt) {	/* Channel report (I/O)	*/
			newipl = PIL_6;
		}
		break;

	case S390_INTR_EXT:
		ip->u.ext.intparm = *((uint32_t *)__LC_EXT_INTPARM);
		ip->u.ext.intcode = *((uint16_t *)__LC_EXT_INTCODE);
		ip->u.ext.subcode = *((uint16_t *)__LC_EXT_SUBCODE);

		switch (ip->u.ext.intcode) {
		case EXT_SSIG:			/* PFault coord/BlockIO */
			switch (ip->u.ext.subcode >> 8) {
			case SSG_BIOZ:		/* Z/Arch blockio       */
				ip->u.ext.extparm =
					*((uint64_t *)__LC_BLKIO_PARM);
				newipl = PIL_9;
				break;
			default:
				/* Not recognized, so overprotect       */
				newipl = PIL_11;
				prom_printf("Spurious EXT_SSIG interrupt subcode: %d\n",
					ip->u.ext.subcode >> 8);
				break;
			}
			break;
		case EXT_IUCV:			/* IUCV	(VM only)	*/
			newipl = PIL_7;
			break;
		case EXT_HWCN:			/* Service signal	*/
			newipl = PIL_8;
			break;
		case EXT_CLKC:			/* Clock comparator	*/
		case EXT_CPUT:			/* CPU timer		*/
		case EXT_TIMA:			/* Timing alert		*/
		case EXT_CALL:			/* External call	*/
			newipl = PIL_10;
			break;
		case EXT_ESIG:			/* Emergency signal     */
			newipl = PIL_11;
			break;
		case EXT_OPRI:			/* Operator key - debug */
			newipl = PIL_12;
			break;
		default:			/* all other externals	*/
			newipl = PIL_11;
			break;
		}
		break;

	default:
		newipl = PIL_15;
		break;
	}

	setspl(newipl);

	return (newipl);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_exit.                                        */
/*                                                                  */
/* Function	- Lower IPL to given level.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
intr_exit(int newipl, int trapno)
{
	setspl(newipl);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hilevel_intr_prolog.                              */
/*                                                                  */
/* Function	- Do all the work necessary to set up the CPU and   */
/*		  thread structures to dispatch a high-level int-   */
/*		  errupt.                      		 	    */
/*		                               		 	    */
/*		  Returns 0 if we're NOT already on the high-level  */
/*		  interrupt stack (and must switch to it), non-zero */
/*		  if we are already on that stack.		    */
/*		                               		 	    */
/*		  Called with interrupts masked. The PIL is already */
/*		  set to the appropriate level for the trap number. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
hilevel_intr_prolog(struct cpu *cpu, uint_t pil, uint_t oldpil, struct regs *rp)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t mask;
	hrtime_t intrtime;
	hrtime_t now = gettick_counter();

	ASSERT(pil > LOCK_LEVEL);

	if (pil == CBE_HIGH_PIL) {
		cpu->cpu_profile_pil = oldpil;
		if (USERMODE(rp->r_psw)) {
			cpu->cpu_profile_pc = 0;
			cpu->cpu_profile_upc = rp->r_pc;
		} else {
			cpu->cpu_profile_pc = rp->r_pc;
			cpu->cpu_profile_upc = 0;
		}
	}

	mask = cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK;
	if (mask != 0) {
		int nestpil;

		/*
		 * We have interrupted another high-level interrupt.
		 * Load starting timestamp, compute interval, update
		 * cumulative counter.
		 */
		nestpil = frogr((uint16_t)mask);
		ASSERT(nestpil < pil);
		intrtime = now -
		    mcpu->pil_high_start[nestpil - (LOCK_LEVEL + 1)];
		mcpu->intrstat[nestpil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		/*
		 * Another high-level interrupt is active below this one, so
		 * there is no need to check for an interrupt thread.  That
		 * will be done by the lowest priority high-level interrupt
		 * active.
		 */
	} else {
		kthread_t *t = cpu->cpu_thread;

		/*
		 * See if we are interrupting a low-level interrupt thread.
		 * If so, account for its time slice only if its time stamp
		 * is non-zero.
		 */
		if ((t->t_flag & T_INTR_THREAD) != 0 && t->t_intr_start != 0) {
			intrtime = now - t->t_intr_start;
			mcpu->intrstat[t->t_pil][0] += intrtime;
			cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
			t->t_intr_start = 0;
		}
	}

	/*
	 * Store starting timestamp in CPU structure for this PIL.
	 */
	mcpu->pil_high_start[pil - (LOCK_LEVEL + 1)] = now;

	ASSERT((cpu->cpu_intr_actv & (1 << pil)) == 0);

	if (pil == 15) {
		/*
		 * To support reentrant level 15 interrupts, we maintain a
		 * recursion count in the top half of cpu_intr_actv.  Only
		 * when this count hits zero do we clear the PIL 15 bit from
		 * the lower half of cpu_intr_actv.
		 */
		uint16_t *refcntp = (uint16_t *)&cpu->cpu_intr_actv;
		(*refcntp)++;
	}

	mask = cpu->cpu_intr_actv;

	cpu->cpu_intr_actv |= (1 << pil);

	return (mask & CPU_INTR_ACTV_HIGH_LEVEL_MASK);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hilevel_intr_epilog.                              */
/*                                                                  */
/* Function	- Does most of the work of returning from a high-   */
/*		  level interrupt.             		 	    */
/*		                               		 	    */
/*		  Returns 0 if there are no more high level int-    */
/*		  errupts (in which case we must back to the int-   */
/*		  errupted thread stack) or non-zero if there are   */
/*		  (in which case we should stay on it).		    */
/*		                               		 	    */
/*		  Called with interrupts masked.	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
hilevel_intr_epilog(struct cpu *cpu, uint_t pil, uint_t oldpil, uint_t vecnum)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t mask;
	hrtime_t intrtime;
	hrtime_t now = gettick_counter();

	ASSERT(getpil() == pil);

	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(cpu->cpu_intr_actv & (1 << pil));

	if (pil == 15) {
		/*
		 * To support reentrant level 15 interrupts, we maintain a
		 * recursion count in the top half of cpu_intr_actv.  Only
		 * when this count hits zero do we clear the PIL 15 bit from
		 * the lower half of cpu_intr_actv.
		 */
		uint16_t *refcntp = (uint16_t *)&cpu->cpu_intr_actv;

		ASSERT(*refcntp > 0);

		if (--(*refcntp) == 0)
			cpu->cpu_intr_actv &= ~(1 << pil);
	} else {
		cpu->cpu_intr_actv &= ~(1 << pil);
	}

	ASSERT(mcpu->pil_high_start[pil - (LOCK_LEVEL + 1)] != 0);

	intrtime = now - mcpu->pil_high_start[pil - (LOCK_LEVEL + 1)];
	mcpu->intrstat[pil][0] += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;

	/*
	 * Check for lower-pil nested high-level interrupt beneath
	 * current one.  If so, place a starting timestamp in its
	 * pil_high_start entry.
	 */
	mask = cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK;
	if (mask != 0) {
		int nestpil;

		/*
		 * find PIL of nested interrupt
		 */
		nestpil = frogr((uint16_t)mask);
		ASSERT(nestpil < pil);
		mcpu->pil_high_start[nestpil - (LOCK_LEVEL + 1)] = now;
		/*
		 * (Another high-level interrupt is active below this one,
		 * so there is no need to check for an interrupt
		 * thread.  That will be done by the lowest priority
		 * high-level interrupt active.)
		 */
	} else {
		/*
		 * Check to see if there is a low-level interrupt active.
		 * If so, place a starting timestamp in the thread
		 * structure.
		 */
		kthread_t *t = cpu->cpu_thread;

		if (t->t_flag & T_INTR_THREAD)
			t->t_intr_start = now;
	}

	mcpu->mcpu_pri  = oldpil;
	intr_exit(oldpil, vecnum);

	return (cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_thread_prolog.                               */
/*                                                                  */
/* Function	- Set up the CPU, thread, and interrupt thread str- */
/*		  uctures for executing an interrupt thread. The    */
/*		  new stack pointer of the interrupt thread (which  */
/*		  must be swiched to) is returned.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static caddr_t
intr_thread_prolog(struct cpu *cpu, caddr_t stackptr, uint_t pil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t, *volatile it;
	hrtime_t now = gettick_counter();

	ASSERT(pil > 0);
	ASSERT((cpu->cpu_intr_actv & (1 << pil)) == 0);
	cpu->cpu_intr_actv |= (1 << pil);

	/*
	 * Get set to run an interrupt thread.
	 * There should always be an interrupt thread, since we
	 * allocate one for each level on each CPU.
	 *
	 * t_intr_start could be zero due to cpu_intr_swtch_enter.
	 */
	t = cpu->cpu_thread;
	if ((t->t_flag & T_INTR_THREAD) && t->t_intr_start != 0) {
		hrtime_t intrtime = now - t->t_intr_start;
		mcpu->intrstat[t->t_pil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		t->t_intr_start = 0;
	}

	ASSERT(SA((uintptr_t)stackptr) == (uintptr_t)stackptr);

	t->t_sp = (uintptr_t)stackptr;	/* mark stack in curthread for resume */

	/*
	 * unlink the interrupt thread off the cpu
	 *
	 * Note that the code in kcpc_overflow_intr -relies- on the
	 * ordering of events here - in particular that t->t_lwp of
	 * the interrupt thread is set to the pinned thread *before*
	 * curthread is changed.
	 */
	it		     = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it->t_link;
	it->t_intr	     = t;
	it->t_lwp	     = t->t_lwp;

	/*
	 * (threads on the interrupt thread free list could have state
	 * preset to TS_ONPROC, but it helps in debugging if
	 * they're TS_FREE.)
	 */
	it->t_state	 = TS_ONPROC;

	cpu->cpu_thread  = it;		/* new curthread on this cpu */
	it->t_pil	 = (uchar_t)pil;
	it->t_pri	 = intr_pri + (pri_t)pil;
	it->t_intr_start = now;

	return (it->t_stk);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_thread_epilog.                               */
/*                                                                  */
/* Function	- Does most of the work of returning from an int-   */
/*		  errupt thread.               		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
intr_thread_epilog(struct cpu *cpu, uint_t vec, uint_t oldpil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t;
	kthread_t *it = cpu->cpu_thread;	/* curthread */
	uint_t pil, basespl;
	hrtime_t intrtime;
	hrtime_t now = gettick_counter();

	pil = it->t_pil;
	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(it->t_intr_start != 0);
	intrtime = now - it->t_intr_start;
	mcpu->intrstat[pil][0] += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;

	ASSERT(cpu->cpu_intr_actv & (1 << pil));
	cpu->cpu_intr_actv &= ~(1 << pil);

	/*
	 * If there is still an interrupted thread underneath this one
	 * then the interrupt was never blocked and the return is
	 * fairly simple.  Otherwise it isn't.
	 */
	if ((t = it->t_intr) == NULL) {
		/*
		 * The interrupted thread is no longer pinned underneath
		 * the interrupt thread.  This means the interrupt must
		 * have blocked, and the interrupted thread has been
		 * unpinned, and has probably been running around the
		 * system for a while.
		 *
		 * Since there is no longer a thread under this one, put
		 * this interrupt thread back on the CPU's free list and
		 * resume the idle thread which will dispatch the next
		 * thread to run.
		 */
#ifdef DEBUG
		intr_thread_cnt++;
#endif
		cpu->cpu_stats.sys.intrblk++;
		/*
		 * Set CPU's base SPL based on active interrupts bitmask
		 */
		set_base_spl();
		basespl 	= cpu->cpu_base_spl;
		mcpu->mcpu_pri  = basespl;
		intr_exit(basespl, vec);
		(void) splhigh();
		sti();
		it->t_state = TS_FREE;
		/*
		 * Return interrupt thread to pool
		 */
		it->t_link = cpu->cpu_intr_thread;
		cpu->cpu_intr_thread = it;
		swtch();
		panic("intr_thread_epilog: swtch returned");
		/*NOTREACHED*/
	}

	/*
	 * Return interrupt thread to the pool
	 */
	it->t_link = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it;
	it->t_state = TS_FREE;

	basespl 	= cpu->cpu_base_spl;
	pil 		= MAX(oldpil, basespl);
	mcpu->mcpu_pri  = pil;
	intr_exit(pil, vec);
	t->t_intr_start = now;
	cpu->cpu_thread = t;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_get_thread.                                  */
/*                                                                  */
/* Function	- This is a resource for interrupt handlers to de-  */
/*		  termine how much time has been spent handling the */
/*		  current interrupt. Such a function is needed be-  */
/*		  cause higher level interrupts can arrive during   */
/*		  the processing of an interrupt. intr_get_time()   */
/*		  only returns the time spent in the current int-   */
/*		  errupt handler.              		 	    */
/*		                               		 	    */
/*		  The caller must be calling from an interrupt hand-*/
/*		  ler running at a PIL below or at lock level. Tim- */
/*		  ings are not provided for high-level interrupts.  */
/*		                               		 	    */
/*		  The 1st time intr_get_time() is called while hand-*/
/*		  ling an interrupt, it returns the time since the  */
/*		  time since the interrupt handler was invoked. Sub-*/
/*		  sequent calls will return the time since the prior*/
/*		  call to intr_get_time(). Time is returned as 	    */
/*		  ticks.                       		 	    */
/*		                               		 	    */
/*		  Theory of Intrstat[][]:      		 	    */
/*		                               		 	    */
/*		  uint64_t intrstat[pil][0..1] is an array indexed  */
/*		  by PIL, with two uint64_t's per PIL.		    */
/*		                               		 	    */
/*		  intrstat[pil][0] is cumulative count of the num-  */
/*		  ber of ticks spent handling all interrupts at the */
/*		  specified PIL on this CPU. It is exported via     */
/*		  kstats to the user.          		 	    */
/*		                               		 	    */
/*		  intrstat[pil][1] is always a count of ticks less  */
/*		  than or equal to the value in [0]. The difference */
/*		  between [1] and [0] is the value returned by a    */
/*		  call to intr_get_time(). At the start of interrupt*/
/*		  processing, [0] and [1] will be almost equal. As  */
/*		  the interrupt consumes time, [0] will increase    */
/*		  but [1] will remain the same. A call to this      */
/*		  rouine will return the difference, then update [1]*/
/*		  to be the same as [0]. Future calls will return   */
/*		  the time since the last call. Finally, when the   */
/*		  interrupt completes, [1] is updated to the same   */
/*		  as [0].                      		 	    */
/*		                               		 	    */
/*		  Implementation -             		 	    */
/*		                               		 	    */
/*		  intr_get_time() works much like a higher level    */
/*		  interrupt arriving. It "checkpoints" the timing   */
/*		  information by incrementing intrstat[pil[0] to    */
/*		  include elapsed running time, and by setting      */
/*		  t_intr_start to gettick_counter(). It then sets the return    */
/*		  return value to intrstat[pil][0] - 		    */
/*		  intrstat[pil][1], and updates intrstat[pil][1] to */
/*		  be the same as the new value of intrstat[pil][0]. */
/*		                               		 	    */
/*		  In the normal handling of interrupts, after an    */
/*		  interrupt handler returns and the code in 	    */
/*		  intr_thread() updates intrstat[pil][0], it then   */
/*		  sets intrstat[pil[1] to the new value of [pil][0].*/
/*		  When [0] == [1] the timings are reset: i.e.	    */
/*		  intr_get_time() will return [0] - [1] which is 0. */
/*		                               		 	    */
/*		  Whenever interrupts arrive on a CPU which is 	    */
/*		  handling a lower PIL interrupt, they update the   */
/*		  lower PIL's [0] to show time spent in the handler */
/*		  that they've interrupted. This results in a grow- */
/*		  ing discrepancy between [0] and [1], which is     */
/*		  returned the next time intr_get_time() is called. */
/*		  Time spent in the higher-PIL interrupt will not   */
/*		  be returned in the next intr_get_time() call from */
/*		  the original interrupt, because the higher-PIL    */
/*		  interrupt's time is accummulated in 		    */
/*		  intrstat[higherpil][].       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint64_t
intr_get_time(void)
{
	struct cpu *cpu;
	struct machcpu *mcpu;
	kthread_t *t;
	uint64_t time, delta, ret;
	uint_t pil;

	cli();
	cpu = CPU;
	mcpu = &cpu->cpu_m;
	t = cpu->cpu_thread;
	pil = t->t_pil;
	ASSERT((cpu->cpu_intr_actv & CPU_INTR_ACTV_HIGH_LEVEL_MASK) == 0);
	ASSERT(t->t_flag & T_INTR_THREAD);
	ASSERT(pil != 0);
	ASSERT(t->t_intr_start != 0);

	time = gettick_counter();
	delta = time - t->t_intr_start;
	t->t_intr_start = time;

	time = mcpu->intrstat[pil][0] + delta;
	ret = time - mcpu->intrstat[pil][1];
	mcpu->intrstat[pil][0] = time;
	mcpu->intrstat[pil][1] = time;
	cpu->cpu_intracct[cpu->cpu_mstate] += delta;

	sti();
	return (ret);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dosoftint_prolog.                                 */
/*                                                                  */
/* Function	- Prepare for soft interrupt handler.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static caddr_t
dosoftint_prolog(
	struct cpu *cpu,
	caddr_t stackptr,
	uint32_t st_pending,
	uint_t oldpil)
{
	kthread_t *t, *volatile it;
	struct machcpu *mcpu = &cpu->cpu_m;
	uint_t pil;
	hrtime_t now;

top:
	ASSERT(st_pending == mcpu->mcpu_softinfo.st_pending);

	pil = frogr((uint16_t)st_pending);
	if (pil <= oldpil || pil <= cpu->cpu_base_spl)
		return (0);

	/*
	 * XX64	Sigh.
	 *
	 * This is a transliteration of the i386 assembler code for
	 * soft interrupts.  One question is "why does this need
	 * to be atomic?"  One possible race is -other- processors
	 * posting soft interrupts to us in set_pending() i.e. the
	 * CPU might get preempted just after the address computation,
	 * but just before the atomic transaction, so another CPU would
	 * actually set the original CPU's st_pending bit.  However,
	 * it looks like it would be simpler to disable preemption there.
	 * Are there other races for which preemption control doesn't work?
	 *
	 * The i386 assembler version -also- checks to see if the bit
	 * being cleared was actually set; if it wasn't, it rechecks
	 * for more.  This seems a bit strange, as the only code that
	 * ever clears the bit is -this- code running with interrupts
	 * disabled on -this- CPU.  This code would probably be cheaper:
	 *
	 * atomic_and_32((uint32_t *)&mcpu->mcpu_softinfo.st_pending,
	 *   ~(1 << pil));
	 *
	 * and t->t_preempt--/++ around set_pending() even cheaper,
	 * but at this point, correctness is critical, so we slavishly
	 * emulate the i386 port.
	 */
	if (atomic_btr32((uint32_t *)
	    &mcpu->mcpu_softinfo.st_pending, pil) == 0) {
		st_pending = mcpu->mcpu_softinfo.st_pending;
		goto top;
	}

	mcpu->mcpu_pri  = pil;
	setspl(pil);

	now = gettick_counter();

	/*
	 * Get set to run interrupt thread.
	 * There should always be an interrupt thread since we
	 * allocate one for each level on the CPU.
	 */
	it 		     = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it->t_link;

	/* t_intr_start could be zero due to cpu_intr_swtch_enter. */
	t = cpu->cpu_thread;
	if ((t->t_flag & T_INTR_THREAD) && t->t_intr_start != 0) {
		hrtime_t intrtime = now - t->t_intr_start;
		mcpu->intrstat[pil][0] += intrtime;
		cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;
		t->t_intr_start = 0;
	}

	/*
	 * Note that the code in kcpc_overflow_intr -relies- on the
	 * ordering of events here - in particular that t->t_lwp of
	 * the interrupt thread is set to the pinned thread *before*
	 * curthread is changed.
	 */
	it->t_lwp   = t->t_lwp;
	it->t_state = TS_ONPROC;

	/*
	 * Push interrupted thread onto list from new thread.
	 * Set the new thread as the current one.
	 * Set interrupted thread's T_SP because if it is the idle thread,
	 * resume() may use that stack between threads.
	 */

	ASSERT(SA((uintptr_t)stackptr) == (uintptr_t)stackptr);
	t->t_sp 	= (uintptr_t)stackptr;

	it->t_intr 	= t;
	cpu->cpu_thread = it;

	/*
	 * Set bit for this pil in CPU's interrupt active bitmask.
	 */
	ASSERT((cpu->cpu_intr_actv & (1 << pil)) == 0);
	cpu->cpu_intr_actv |= (1 << pil);

	/*
	 * Initialize thread priority level from intr_pri
	 */
	it->t_pil 	 = (uchar_t)pil;
	it->t_pri 	 = (pri_t)pil + intr_pri;
	it->t_intr_start = now;

	return (it->t_stk);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dosoftint_epilog.                                 */
/*                                                                  */
/* Function	- Cleanup after handling a soft interrupt.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
dosoftint_epilog(struct cpu *cpu, uint_t oldpil)
{
	struct machcpu *mcpu = &cpu->cpu_m;
	kthread_t *t, *it;
	uint_t pil, basespl;
	hrtime_t intrtime;
	hrtime_t now = gettick_counter();

	it  = cpu->cpu_thread;
	pil = it->t_pil;

	cpu->cpu_stats.sys.intr[pil - 1]++;

	ASSERT(cpu->cpu_intr_actv & (1 << pil));
	cpu->cpu_intr_actv		   &= ~(1 << pil);
	intrtime 			    = now - it->t_intr_start;
	mcpu->intrstat[pil][0] 		   += intrtime;
	cpu->cpu_intracct[cpu->cpu_mstate] += intrtime;

	/*
	 * If there is still an interrupted thread underneath this one
	 * then the interrupt was never blocked and the return is
	 * fairly simple.  Otherwise it isn't.
	 */
	if ((t = it->t_intr) == NULL) {
		/*
		 * Put thread back on the interrupt thread list.
		 * This was an interrupt thread, so set CPU's base SPL.
		 */
		set_base_spl();
		it->t_state 	     = TS_FREE;
		it->t_link 	     = cpu->cpu_intr_thread;
		cpu->cpu_intr_thread = it;
		(void) splhigh();
		sti();
		swtch();
		/*NOTREACHED*/
		panic("dosoftint_epilog: swtch returned");
	}
	it->t_link 	     = cpu->cpu_intr_thread;
	cpu->cpu_intr_thread = it;
	it->t_state 	     = TS_FREE;
	cpu->cpu_thread      = t;
	if (t->t_flag & T_INTR_THREAD)
		t->t_intr_start = now;
	basespl 	= cpu->cpu_base_spl;
	pil 		= MAX(oldpil, basespl);
	mcpu->mcpu_pri  = pil;
	setspl(pil);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sys_rtt_common.                                   */
/*                                                                  */
/* Function	- Prepare for return to interrupt thread.           */
/*		                               		 	    */
/*		  Common tasks required by _sys_rtt. Called with    */
/*		  interrupts disabled.         		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
sys_rtt_common(struct regs *rp)
{
	kthread_t   *tp;

loop:

	/*
	 * Check if returning to user
	 */
	tp = CPU->cpu_thread;
	if (USERMODE(rp->r_psw)) {
		/*
		 * Check if AST pending.
		 */
		if (tp->t_astflag) {
			klwp_t	    *lwp = ttolwp(curthread);
			k_siginfo_t siginfo;
			uint_t	    fault = 0;

			bzero(&siginfo, sizeof (siginfo));
			/*
			 * Let trap() handle the AST
			 */
			if (lwp->lwp_pcb.pcb_flags & CPC_OVERFLOW) {
				lwp->lwp_pcb.pcb_flags &= ~CPC_OVERFLOW;
				if (kcpc_overflow_ast()) {
					/*
					 * Signal performance counter overflow
					 */
					if (tudebug)
						showregs(0, rp, (caddr_t)0);
					siginfo.si_signo = SIGEMT;
					siginfo.si_code  = EMT_CPCOVF;
					siginfo.si_addr  = (caddr_t)rp->r_pc;
					fault            = FLTCPCOVF;
				}
			}
			trap_cleanup(rp, fault, &siginfo);
			trap_rtt();
			goto loop;
		}

		return (1);
	}

	/*
	 * Here if we are returning to supervisor mode.
	 * Check for a kernel preemption request.
	 */
	if (CPU->cpu_kprunrun && interrupts_enabled()) {

		/*
		 * Do nothing if already in kpreempt
		 */
		if (!tp->t_preempt_lk) {
			tp->t_preempt_lk = 1;
			sti();
			kpreempt(1); /* asynchronous kpreempt call */
			cli();
			tp->t_preempt_lk = 0;
		}
	}

#if 0	// Not sure if we need this S390X FIXME
	/*
	 * If we interrupted the mutex_exit() critical region we must
	 * reset the PC back to the beginning to prevent missed wakeups
	 * See the comments in mutex_exit() for details.
	 */
	if ((uintptr_t)rp->r_pc - (uintptr_t)mutex_exit_critical_start <
	    mutex_exit_critical_size) {
		rp->r_pc = (greg_t)mutex_exit_critical_start;
	}

	/*
	 * If we interrupted the mutex_owner_running() critical region we
	 * must reset the PC back to the beginning to prevent dereferencing
	 * of a freed thread pointer. See the comments in mutex_owner_running
	 * for details.
	 */
	if ((uintptr_t)rp->r_pc -
	    (uintptr_t)mutex_owner_critical_start <
	    mutex_owner_running_critical_size) {
		rp->r_pc = (greg_t)mutex_owner_critical_start;
	}

#endif
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_create_intrstat.                              */
/*                                                                  */
/* Function	- Create interrupt kstats for this CPU.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cpu_create_intrstat(cpu_t *cp)
{
	int		i;
	kstat_t		*intr_ksp;
	kstat_named_t	*knp;
	char		name[KSTAT_STRLEN];
	zoneid_t	zoneid;

	ASSERT(MUTEX_HELD(&cpu_lock));

	if (pool_pset_enabled())
		zoneid = GLOBAL_ZONEID;
	else
		zoneid = ALL_ZONES;

	intr_ksp = kstat_create_zone("cpu", cp->cpu_id, "intrstat", "misc",
	    KSTAT_TYPE_NAMED, PIL_MAX * 2, NULL, zoneid);

	/*
	 * Initialize each PIL's named kstat
	 */
	if (intr_ksp != NULL) {
		intr_ksp->ks_update = cpu_kstat_intrstat_update;
		knp = (kstat_named_t *)intr_ksp->ks_data;
		intr_ksp->ks_private = cp;
		for (i = 0; i < PIL_MAX; i++) {
			(void) snprintf(name, KSTAT_STRLEN, "level-%d-time",
			    i + 1);
			kstat_named_init(&knp[i * 2], name, KSTAT_DATA_UINT64);
			(void) snprintf(name, KSTAT_STRLEN, "level-%d-count",
			    i + 1);
			kstat_named_init(&knp[(i * 2) + 1], name,
			    KSTAT_DATA_UINT64);
		}
		kstat_install(intr_ksp);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_delete_intrstat.                              */
/*                                                                  */
/* Function	- Delete interrupt kstats for this CPU.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cpu_delete_intrstat(cpu_t *cp)
{
	kstat_delete_byname_zone("cpu", cp->cpu_id, "intrstat", ALL_ZONES);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_kstat_intrstat_update.                        */
/*                                                                  */
/* Function	- Convert interrupt statistics from CPU ticks to    */
/*		  nanoseconds and update kstat.		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
cpu_kstat_intrstat_update(kstat_t *ksp, int rw)
{
	kstat_named_t	*knp = ksp->ks_data;
	cpu_t		*cpup = (cpu_t *)ksp->ks_private;
	int		i;

	if (rw == KSTAT_WRITE)
		return (EACCES);

	/*
	 * We use separate passes to copy and convert the statistics to
	 * nanoseconds. This assures that the snapshot of the data is as
	 * self-consistent as possible.
	 */

	for (i = 0; i < PIL_MAX; i++) {
		knp[i * 2].value.ui64 = cpup->cpu_m.intrstat[i + 1][0];
		knp[(i * 2) + 1].value.ui64 = cpup->cpu_stats.sys.intr[i];
	}

	for (i = 0; i < PIL_MAX; i++) {
		knp[i * 2].value.ui64 =
		    (uint64_t)tick2ns((hrtime_t)knp[i * 2].value.ui64);
	}

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_intr_swtch_enter.                             */
/*                                                                  */
/* Function	- An interrupt thread is ending a time slice, so    */
/*		  compute the interval it ran for and update the    */
/*		  statistic for its PIL.       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cpu_intr_swtch_enter(kthread_id_t t)
{
	uint64_t	interval;
	uint64_t	start;
	cpu_t		*cpu;

	ASSERT((t->t_flag & T_INTR_THREAD) != 0);
	ASSERT(t->t_pil > 0 && t->t_pil <= LOCK_LEVEL);

	/*
	 * We could be here with a zero timestamp. This could happen if:
	 * an interrupt thread which no longer has a pinned thread underneath
	 * it (i.e. it blocked at some point in its past) has finished running
	 * its handler. intr_thread() updated the interrupt statistic for its
	 * PIL and zeroed its timestamp. Since there was no pinned thread to
	 * return to, swtch() gets called and we end up here.
	 *
	 * It can also happen if an interrupt thread in intr_thread() calls
	 * preempt. It will have already taken care of updating stats. In
	 * this event, the interrupt thread will be runnable.
	 */
	if (t->t_intr_start) {
		do {
			start = t->t_intr_start;
			interval = gettick_counter() - start;
		} while (cas64(&t->t_intr_start, start, 0) != start);
		cpu = CPU;
		cpu->cpu_m.intrstat[t->t_pil][0] += interval;

		atomic_add_64((uint64_t *)&cpu->cpu_intracct[cpu->cpu_mstate],
		    interval);
	} else
		ASSERT(t->t_intr == NULL || t->t_state == TS_RUN);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_intr_swtch_exit.                              */
/*                                                                  */
/* Function	- An interrupt thread is returning from swtch().    */
/*		  Place a starting timestamp in its thread struc-   */
/*		  ture.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cpu_intr_swtch_exit(kthread_id_t t)
{
	uint64_t ts;

	ASSERT((t->t_flag & T_INTR_THREAD) != 0);
	ASSERT(t->t_pil > 0 && t->t_pil <= LOCK_LEVEL);

	do {
		ts = t->t_intr_start;
	} while (cas64(&t->t_intr_start, ts, gettick_counter()) != ts);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dispatch_hilevel.                                 */
/*                                                                  */
/* Function	- Dispatch a high-level interrupt (one that is      */
/*		  above LOCK_LEVEL)                  	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
dispatch_hilevel(intparms *ip, uint_t arg2)
{
	intparms lip;

	bcopy(ip, &lip, sizeof(lip));

	sti();
	dispatch_autovect(&lip);
	cli();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dispatch_hardint.                                 */
/*                                                                  */
/* Function	- Dispatch a normal interrupt.                      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
dispatch_hardint(intparms *ip, uint_t oldipl)
{
	struct cpu *cpu = CPU;
	intparms lip;

	bcopy(ip, &lip, sizeof(lip));

	sti();
	dispatch_autovect(&lip);
	cli();

	/*
	 * Must run intr_thread_epilog() on the interrupt thread stack, since
	 * there may not be a return from it if the interrupt thread blocked.
	 */
	intr_thread_epilog(cpu, lip.vector, oldipl);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dispatch_softint.                                 */
/*                                                                  */
/* Function	- Dispatch a soft interrupt.                        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
dispatch_softint(uint_t oldpil, uint_t arg2)
{
	struct cpu *cpu = CPU;

	sti();
	av_dispatch_softvect((int)cpu->cpu_thread->t_pil);
	cli();

	/*
	 * Must run softint_epilog() on the interrupt thread stack, since
	 * there may not be a return from it if the interrupt thread blocked.
	 */
	dosoftint_epilog(cpu, oldpil);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dosoftint.                                        */
/*                                                                  */
/* Function	- Deliver any softints the current interrupt pri-   */
/*		  ority allows.                		 	    */
/*		                               		 	    */
/*		  Called with interrupts disabled.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
dosoftint(struct regs *rp)
{
	struct cpu *cpu = CPU;
	int	oldipl;
	caddr_t newsp,
		sp;

	sp = (caddr_t) rp;

	while (cpu->cpu_softinfo.st_pending != 0) {
		oldipl = cpu->cpu_pri;
		newsp  = dosoftint_prolog(cpu, sp,
					  cpu->cpu_softinfo.st_pending, 
					  oldipl);
		/*
		 * If returned stack pointer is NULL, priority is too high
		 * to run any of the pending softints now.
		 * Break out and they will be run later.
		 */
		if (newsp == NULL)
			break;
		switch_sp_and_call(newsp, dispatch_softint, oldipl, 0);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- do_interrupt.                                     */
/*                                                                  */
/* Function	- Interrupt service routine, called with interrupts */
/*		  disabled.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
do_interrupt(struct regs *rp, int trapno)
{
	struct cpu *cpu = CPU;
	intparms ip;
	int newipl, oldipl = cpu->cpu_pri;
	uint_t vector;
	caddr_t newsp,
		oldsp;

#ifdef TRAPTRACE
	/* S390X FIXME - TRAPTRACE processing */
#endif	/* TRAPTRACE */

	/*
	 * Raise the interrupt priority.
	 */
	newipl = intr_enter(oldipl, &trapno, &ip);

#ifdef TRAPTRACE
	/* S390X FIXME - TRAPTRACE processing */
#endif	/* TRAPTRACE */

	/*
	 * Bail if it is a spurious interrupt
	 */
	if (newipl == -1) {
		return;
	}
	cpu->cpu_pri = newipl;
	ip.vector = trapno;

#ifdef TRAPTRACE
	/* S390X FIXME - TRAPTRACE processing */
#endif	/* TRAPTRACE */

	if (newipl > LOCK_LEVEL) {
		/*
		 * High priority interrupts run on this cpu's interrupt stack.
		 */
		if (hilevel_intr_prolog(cpu, newipl, oldipl, rp) == 0) {
			newsp = cpu->cpu_intr_stack;
			switch_sp_and_call(newsp, dispatch_hilevel, &ip, 0);
		} else { /* already on the interrupt stack */
			dispatch_hilevel(&ip, 0);
		}
		(void) hilevel_intr_epilog(cpu, newipl, oldipl, ip.vector);
	} else {
		/*
		 * Run this interrupt in a separate thread.
		 */
		oldsp = (caddr_t) rp;
		newsp = intr_thread_prolog(cpu, oldsp, newipl);
		switch_sp_and_call(newsp, dispatch_hardint, &ip, oldipl);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- interrupts_enabled.                               */
/*                                                                  */
/* Function	- Return 1 if I/O or external interrupts are 	    */
/*		  enabled, else return 0.      		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
interrupts_enabled(void)
{
	pswg_t 	psw;
	
	__asm__ ("	epsw	1,2\n"
		 "	stm     1,2,%0\n"
		 : "=m" (psw) : : "1", "2");

	return (psw.mask.io || psw.mask.ext);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- intr_passivate.                                   */
/*                                                                  */
/* Function	- Complete resumption of interrupted thread. This   */
/*		  routine returns the pil of the interrupt thread.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
intr_passivate(
	kthread_t *it,		/* interrupt thread */
	kthread_t *t)		/* interrupted thread */
{
	ASSERT(it->t_flag & T_INTR_THREAD);
	ASSERT(SA(t->t_sp) == t->t_sp);

	t->t_pc = (uintptr_t) _sys_rtt;
	return (it->t_pil);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- add_spl.                                          */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
add_spl(int irqno, int ipl, int min_ipl, int max_ipl)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- del_spl.                                          */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
del_spl(int irqno, int ipl, int min_ipl, int max_ipl)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- slvltovect.                                       */
/*                                                                  */
/* Function	- Translate a software interrupt level to a vector. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
slvl_to_vect(int lvl)
{
	return (-1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_intr_redist_all_cpus.                       */
/*                                                                  */
/* Function	- Initiate interrupt redistribution.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
i_ddi_intr_redist_all_cpus()
{
}

/*========================= End of Function ========================*/
