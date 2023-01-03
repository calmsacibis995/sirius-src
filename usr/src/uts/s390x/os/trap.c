/*------------------------------------------------------------------*/
/* 								    */
/* Name        - trap.c     					    */
/* 								    */
/* Function    - Various trap and pre-emption related routines.     */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
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

/*
 * Patch non-zero to disable preemption of threads in the kernel.
 */
int IGNORE_KERNEL_PREEMPTION = 0;	/* XXX - delete this someday */

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/machsystm.h>
#include <sys/intr.h>
#include <sys/asm_linkage.h>
#include <sys/cpuvar.h>
#include <sys/regset.h>
#include <sys/thread.h>
#include <sys/sdt.h>
#include <sys/old_procfs.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

struct kpreempt_cnts {		/* kernel preemption statistics */
	int	kpc_idle;	/* executing idle thread */
	int	kpc_intr;	/* executing interrupt thread */
	int	kpc_clock;	/* executing clock thread */
	int	kpc_blocked;	/* thread has blocked preemption (t_preempt) */
	int	kpc_notonproc;	/* thread is surrendering processor */
	int	kpc_inswtch;	/* thread has ratified scheduling decision */
	int	kpc_prilevel;	/* processor interrupt level is too high */
	int	kpc_apreempt;	/* asynchronous preemption */
	int	kpc_spreempt;	/* synchronous preemption */
} kpreempt_cnts;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- interrupts_enabled.                               */
/*                                                                  */
/* Function	- Extract the interrupt mask from the PSW and return*/
/*		  whether or not we are enabled for interrupts.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static __inline__ int
interrupts_enabled()
{
	pswg_t	psw;

	__asm__ ("epsw	1,0\n"
		 "stg	1,%0\n" 
		 : "=m" (psw) : : "1" );

	return ((psw.mask.io) || (psw.mask.ext));
} 

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kpreempt.                                         */
/*                                                                  */
/* Function	- Force rescheduling, preempt the running kernel    */
/*		  thread. The argument is the old PIL for an        */
/*		  interrupt or the distinguished value KPREEMPT_SYNC*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kpreempt(int asyncspl)
{
	kthread_t *ct = curthread;

	if (IGNORE_KERNEL_PREEMPTION) {
		aston(CPU->cpu_dispthread);
		return;
	}

	/*
	 * Check that conditions are right for kernel preemption
	 */
	do {
		if (ct->t_preempt) {
			/*
			 * either a privileged thread (idle, panic, interrupt)
			 *	or will check when t_preempt is lowered
			 */
			if (ct->t_pri < 0)
				kpreempt_cnts.kpc_idle++;
			else if (ct->t_flag & T_INTR_THREAD) {
				kpreempt_cnts.kpc_intr++;
				if (ct->t_pil == CLOCK_LEVEL)
					kpreempt_cnts.kpc_clock++;
			} else
				kpreempt_cnts.kpc_blocked++;
			aston(CPU->cpu_dispthread);
			return;
		}
		if (ct->t_state != TS_ONPROC ||
		    ct->t_disp_queue != CPU->cpu_disp) {
			/* this thread will be calling swtch() shortly */
			kpreempt_cnts.kpc_notonproc++;
			if (CPU->cpu_thread != CPU->cpu_dispthread) {
				/* already in swtch(), force another */
				kpreempt_cnts.kpc_inswtch++;
				siron();
			}
			return;
		}
		if (getpil() >= DISP_LEVEL) {
			/*
			 * We can't preempt this thread if it is at
			 * a PIL >= DISP_LEVEL since it may be holding
			 * a spin lock (like sched_lock).
			 */
			siron();	/* check back later */
			kpreempt_cnts.kpc_prilevel++;
			return;
		}
		if (!interrupts_enabled()) {
			/*
			 * Can't preempt while running with ints disabled
			 */
			kpreempt_cnts.kpc_prilevel++;
			return;
		}
		if (asyncspl != KPREEMPT_SYNC)
			kpreempt_cnts.kpc_apreempt++;
		else
			kpreempt_cnts.kpc_spreempt++;

		ct->t_preempt++;
		preempt();
		ct->t_preempt--;
	} while (CPU->cpu_kprunrun);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- trap_cleanup.                                     */
/*                                                                  */
/* Function	- After analysing the fault perform the required    */
/*		  cleanup actions.             		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
trap_cleanup(struct regs *rp, uint_t fault, k_siginfo_t *sip)
{
	kthread_t *tp = curthread;
	proc_t *p     = ttoproc(tp);
	klwp_id_t lwp = ttolwp(tp);

	if (fault) {
		/*
		 * Remember the fault and fault address
		 * for real-time (SIGPROF) profiling.
		 */
		lwp->lwp_lastfault = fault;
		lwp->lwp_lastfaddr = sip->si_addr;

		DTRACE_PROC2(fault, int, fault, ksiginfo_t *, sip);

		/*
		 * If a debugger has declared this fault to be an
		 * event of interest, stop the lwp.  Otherwise just
		 * deliver the associated signal.
		 */
		if (sip->si_signo != SIGKILL &&
		    prismember(&p->p_fltmask, fault) &&
		    stop_on_fault(fault, sip) == 0)
			sip->si_signo = 0;
	}

	if (sip->si_signo)
		trapsig(sip, 1);

	if (lwp->lwp_oweupc)
		profil_tick(rp->r_pc);

	if (tp->t_astflag | tp->t_sig_check) {
		/*
		 * Turn off the AST flag before checking all the conditions that
		 * may have caused an AST.  This flag is on whenever a signal or
		 * unusual condition should be handled after the next trap or
		 * syscall.
		 */
		astoff(tp);
		tp->t_sig_check = 0;

		/*
		 * The following check is legal for the following reasons:
		 *	1) The thread we are checking, is ourselves, so there is
		 *	   no way the proc can go away.
		 *	2) The only time we need to be protected by the
		 *	   lock is if the binding is changed.
		 *
		 *	Note we will still take the lock and check the binding
		 *	if the condition was true without the lock held.  This
		 *	prevents lock contention among threads owned by the
		 *	same proc.
		 */

		if (tp->t_proc_flag & TP_CHANGEBIND) {
			mutex_enter(&p->p_lock);
			if (tp->t_proc_flag & TP_CHANGEBIND) {
				timer_lwpbind();
				tp->t_proc_flag &= ~TP_CHANGEBIND;
			}
			mutex_exit(&p->p_lock);
		}

		/*
		 * for kaio requests that are on the per-process poll queue,
		 * aiop->aio_pollq, they're AIO_POLL bit is set, the kernel
		 * should copyout their result_t to user memory. by copying
		 * out the result_t, the user can poll on memory waiting
		 * for the kaio request to complete.
		 */
		if (p->p_aio)
			aio_cleanup(0);

		/*
		 * If this LWP was asked to hold, call holdlwp(), which will
		 * stop.  holdlwps() sets this up and calls pokelwps() which
		 * sets the AST flag.
		 *
		 * Also check TP_EXITLWP, since this is used by fresh new LWPs
		 * through lwp_rtt().  That flag is set if the lwp_create(2)
		 * syscall failed after creating the LWP.
		 */
		if (ISHOLD(p))
			holdlwp();

		if (lwp->lwp_pcb.pcb_step == STEP_REQUESTED)
			prdostep();

		/*
		 * All code that sets signals and makes ISSIG evaluate true must
		 * set t_astflag afterwards.
		 */
		if (ISSIG_PENDING(tp, lwp, p)) {
			if (issig(FORREAL))
				psig();
			tp->t_sig_check = 1;
		}

		if (tp->t_rprof != NULL) {
			realsigprof(0, 0);
			tp->t_sig_check = 1;
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- trap_rtt.                                         */
/*                                                                  */
/* Function	- Trap return processing.                           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
trap_rtt(void)
{
	kthread_t *thr = curthread;
	klwp_id_t lwp  = ttolwp(thr);

	/*
	 * If there's an LWP active then let's look further
	 */
	if (lwp != NULL) {

		/*
		 * Set state to LWP_USER here so preempt won't give us a kernel
		 * priority if it occurs after this point.  Call CL_TRAPRET() to
		 * restore the user-level priority.
		 *
		 * It is important that no locks (other than spinlocks) be entered
		 * after this point before returning to user mode (unless lwp_state
		 * is set back to LWP_SYS).
		 */
		lwp->lwp_state = LWP_USER;

		if (thr->t_trapret) {
			thr->t_trapret = 0;
			thread_lock(thr);
			CL_TRAPRET(thr);
			thread_unlock(thr);
		}

		if (lwp->lwp_pcb.pcb_step == STEP_REQUESTED)
			prdostep();

		if (CPU->cpu_runrun || thr->t_schedflag & TS_ANYWAITQ) {
			preempt();

			/*
			 * It's possible that we got here without
			 * being "resumed" so the CPU priority may
			 * still be at DISP_LOCK. If so we reset to 
			 * the CPU_BASE_SPL level.
			 */
			if (CPU->cpu_m.mcpu_pri == DISP_LEVEL) 
				splx(CPU->cpu_base_spl);
		}
	}
}

/*========================= End of Function ========================*/
