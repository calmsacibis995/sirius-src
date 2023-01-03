/*------------------------------------------------------------------*/
/* 								    */
/* Name        - machdep.c  					    */
/* 								    */
/* Function    -                                                    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - December, 2006					    */
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
#include <sys/kstat.h>
#include <sys/param.h>
#include <sys/machparam.h>
#include <sys/machcpuvar.h>
#include <sys/intr.h>
#include <sys/avintr.h>
#include <sys/stack.h>
#include <sys/regset.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/procfs_isa.h>
#include <sys/kmem.h>
#include <sys/cpuvar.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/machpcb.h>
#include <sys/privregs.h>
#include <sys/archsystm.h>
#include <sys/atomic.h>
#include <sys/cmn_err.h>
#include <sys/time.h>
#include <sys/clock.h>
#include <sys/bl.h>
#include <sys/nvpair.h>
#include <sys/kdi_impl.h>
#include <sys/machsystm.h>
#include <sys/sysmacros.h>
#include <sys/promif.h>
#include <sys/pool_pset.h>
#include <sys/machs390x.h>
#include <sys/pghw.h>
#include <sys/hold_page.h>
#include <sys/dtrace.h>
#include <sys/reboot.h>
#include <sys/smp.h>
#include <sys/smp_impldefs.h>
#include <sys/sunndi.h>
#include <sys/dditypes.h>
#include <sys/ddi_isa.h>
#include <sys/errno.h>
#include <sys/x_call.h>
#include <sys/exts390x.h>
#include <sys/ts.h>
#include <vm/hat_pte.h>
#include <vm/htable.h>
#include <vm/hat_s390x.h>
#include <vm/as.h>

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

static void mach_set_softintr(int, struct av_softinfo *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

int maxphys   = MMU_PAGESIZE * 32;	/* 128K */
int klustsize = MMU_PAGESIZE * 32;	/* 128K */

static dev_info_t *cpu_nex_devi = NULL;
static kmutex_t cpu_node_lock;

void (*setsoftint)(int, struct av_softinfo *) = mach_set_softintr;
void (*kdisetsoftint)(int, struct av_softinfo *)=
	(void (*)(int, struct av_softinfo *)) kdi_av_set_softint_pending;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- tick2ns.                                          */
/*                                                                  */
/* Function	- Convert tod value to nanoseconds. 		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hrtime_t
tick2ns(hrtime_t tod)
{
	hrtime_t nano;

	nano = tod * TODFACTOR / CVT2PICO;
	
	return (nano);
}

/*========================= End of Function ========================*/


/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- thread_stk_init.                                  */
/*                                                                  */
/* Function	- Initialize the kernel thread's stack.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
thread_stk_init(caddr_t stk)
{
	ulong_t align;

	align = ((uintptr_t) stk & 0x3f) + SA(MINFRAME) + sizeof(kthread_t);
	stk  -= align;		

	return (stk);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_stk_init.                                     */
/*                                                                  */
/* Function	- Initialize lwp's kernel stack.                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
lwp_stk_init(klwp_t *lwp, caddr_t stk)
{
	struct machpcb 	*mpcb;
	uintptr_t	aln;
	kthread_t	*t;
	pswg_t		*psw;

	aln  = ((uintptr_t) stk & 0x3F);
	stk -= aln;
	mpcb = (struct machpcb *) (stk - SA(sizeof(struct machpcb)));
	bzero(mpcb, sizeof (struct machpcb));
	stk -= SA(MINFRAME) + SA(sizeof(struct machpcb));

	lwp->lwp_regs	  = (void *)&mpcb->mpcb_ctx;
	lwp->lwp_fpu	  = (void *)&mpcb->mpcb_ctx.fpregs;
	__asm__ ("	stfpc	%0\n"
		 : "=m" (mpcb->mpcb_ctx.fpregs.fpc));
	mpcb->mpcb_thread = lwp->lwp_thread;
	mpcb->mpcb_pa	  = va_to_pa(mpcb);
	t		  = (kthread_t *) lwp->lwp_thread;
	psw		  = lwp->lwp_regs;
	memset(psw, 0, sizeof(pswg_t));
	psw->prob 	  = 1;
	psw->key	  = 0;
	psw->mask.dat     = 1;
	psw->mask.ext 	  = 1;
	psw->mask.io  	  = 1;
	psw->mc		  = 1;
	psw->ba		  = 1;
	psw->as		  = 3;
	if (lwptoproc(lwp)->p_model == DATAMODEL_ILP32)
		psw->ea = 0;
	else
		psw->ea = 1;

	return (stk);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_stk_fini.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lwp_stk_fini(klwp_t *lwp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_forkregs.                                     */
/*                                                                  */
/* Function	- Copy registers from parent to child.              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lwp_forkregs(klwp_t *lwp, klwp_t *clwp)
{
	kthread_t *t, *pt = lwptot(lwp);
	struct machpcb *mpcb = lwptompcb(clwp);
	struct machpcb *pmpcb = lwptompcb(lwp);
	mcontext_t *ctx;

	t = mpcb->mpcb_thread;

	/*
	 * remember child's ctx since it will get erased during the bcopy.
	 */
	ctx    = (mcontext_t *) &mpcb->mpcb_ctx;

	/*
	 * Don't copy mpcb_frame since we hand-crafted it
	 * in thread_load().
	 */
	bcopy(lwp->lwp_regs, clwp->lwp_regs, sizeof (struct machpcb) - REGOFF);
	mpcb->mpcb_thread = t;

	/*
	 * It is theoretically possibly for the lwp's wstate to
	 * be different from its value assigned in lwp_stk_init,
	 * since lwp_stk_init assumed the data model of the process.
	 * Here, we took on the data model of the cloned lwp.
	 */
	mpcb->mpcb_pa = va_to_pa(mpcb);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_freeregs.                                     */
/*                                                                  */
/* Function	- No-op.                                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lwp_freeregs(klwp_t *lwp, int isexec)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_attach_brand_hdlrs.                           */
/*                                                                  */
/* Function	- No-op.                                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
lwp_attach_brand_hdlrs(klwp_t *lwp)
{
	/* This function is currently unused on s390x */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_detach_brand_hdlrs.                           */
/*                                                                  */
/* Function	- No-op.                                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
lwp_detach_brand_hdlrs(klwp_t *lwp)
{
	/* This function is currently unused on s390x */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- blacklist.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
blacklist(int cmd, const char *scheme, nvlist_t *fmri, const char *class)
{
	return (ENOTSUP);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kdi_pread.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
kdi_pread(caddr_t buf, size_t nbytes, uint64_t addr, size_t *ncopiedp)
{
	uint64_t	startPage,
			lastPage,
			dest;
	size_t		maxMove;

	lastPage  = 0;
	
	*ncopiedp = nbytes;

	while (nbytes > 0) {
		startPage = (uint64_t) buf & ~(MMU_PAGESIZE - 1);
	
		if (startPage != lastPage) {		
			__asm__ ("	lra	%0,0(%1)"
				 : "=r" (dest)
				 : "r" (buf)
				 : "cc");
		}
		lastPage = startPage;

		maxMove = MMU_PAGESIZE - (dest % MMU_PAGESIZE);

		if (nbytes > maxMove) {
			bcopy ((void *) addr, (void *) dest, maxMove);
			nbytes    -= maxMove;
			buf	  += maxMove;
			dest	  += maxMove;
		} else {
			bcopy ((void *) addr, (void *) dest, nbytes);
			buf	  += nbytes;
			dest	  += nbytes;
			nbytes 	   = 0;
		}
	}

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kdi_pwrite.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
kdi_pwrite(caddr_t buf, size_t nbytes, uint64_t addr, size_t *ncopiedp)
{
	uint64_t	startPage,
			lastPage,
			dest;
	size_t		maxMove;

	lastPage  = 0;
	
	*ncopiedp = nbytes;

	while (nbytes > 0) {
		startPage = (uint64_t) buf & ~(MMU_PAGESIZE - 1);
	
		if (startPage != lastPage) {		
			__asm__ ("	lra	%0,0(%1)"
				 : "=r" (dest)
				 : "r" (buf)
				 : "cc");
		}
		lastPage = startPage;

		maxMove = MMU_PAGESIZE - (dest % MMU_PAGESIZE);
		if (nbytes > maxMove) {
			bcopy ((void *) dest, (void *) addr, maxMove);
			nbytes    -= maxMove;
			buf	  += maxMove;
			dest	  += maxMove;
		} else {
			bcopy ((void *) dest, (void *) addr, nbytes);
			buf	  += nbytes;
			dest	  += nbytes;
			nbytes 	   = 0;
		}
	}

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kdi_kernpanic.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
kdi_kernpanic(struct regs *regs, uint_t tt)
{
	sync_reg_buf = *regs;
	sync_tt      = tt;

	sync_handler();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kdi_plat_call.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
kdi_plat_call(void (*platfn)(void))
{
	if (platfn != NULL) {
		platfn();
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mach_kdi_init.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mach_kdi_init(kdi_t *kdi)
{
	kdi->kdi_plat_call     = kdi_plat_call;
//	kdi->mkdi_cpu_index    = kdi_cpu_index;		FIXME - What does this do?
	kdi->mkdi_kernpanic    = kdi_kernpanic;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kdi_flush_caches.                                 */
/*                                                                  */
/* Function	- Flush TLB entries.                                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kdi_flush_caches(void)
{
	ptlb();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- get_cpu_mstate.                                   */
/*                                                                  */
/* Function	- This routine is passed an array of timestamps,    */
/*                NCMSTATES long, and it fills in the array with    */
/*		  the time spent on CPU in each of the mstates,     */
/*		  where time is returned in nanoseconds.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * No guarantee is made that the returned values in times[] will
 * monotonically increase on sequential calls, although this will
 * be true in the long run. Any such guarantee must be handled by
 * the caller, if needed. This can happen if we fail to account
 * for elapsed time due to a generation counter conflict, yet we
 * did account for it on a prior call (see below).
 *
 * The complication is that the cpu in question may be updating
 * its microstate at the same time that we are reading it.
 * Because the microstate is only updated when the CPU's state
 * changes, the values in cpu_intracct[] can be indefinitely out
 * of date. To determine true current values, it is necessary to
 * compare the current time with cpu_mstate_start, and add the
 * difference to times[cpu_mstate].
 *
 * This can be a problem if those values are changing out from
 * under us. Because the code path in new_cpu_mstate() is
 * performance critical, we have not added a lock to it. Instead,
 * we have added a generation counter. Before beginning
 * modifications, the counter is set to 0. After modifications,
 * it is set to the old value plus one.
 *
 * get_cpu_mstate() will not consider the values of cpu_mstate
 * and cpu_mstate_start to be usable unless the value of
 * cpu_mstate_gen is both non-zero and unchanged, both before and
 * after reading the mstate information. Note that we must
 * protect against out-of-order loads around accesses to the
 * generation counter. Also, this is a best effort approach in
 * that we do not retry should the counter be found to have
 * changed.
 *
 * cpu_intracct[] is used to identify time spent in each CPU
 * mstate while handling interrupts. Such time should be reported
 * against system time, and so is subtracted out from its
 * corresponding cpu_acct[] time and added to
 * cpu_acct[CMS_SYSTEM]. Additionally, intracct time is stored in
 * %ticks, but acct time may be stored as %sticks, thus requiring
 * different conversions before they can be compared.
 */

void
get_cpu_mstate(cpu_t *cpu, hrtime_t *times)
{
	int i;
	hrtime_t now, start;
	uint16_t gen;
	uint16_t state;
	hrtime_t intracct[NCMSTATES];

	/*
	 * Load all volatile state under the protection of membar.
	 * cpu_acct[cpu_mstate] must be loaded to avoid double counting
	 * of (now - cpu_mstate_start) by a change in CPU mstate that
	 * arrives after we make our last check of cpu_mstate_gen.
	 */

	now = gethrtime_unscaled();
	gen = cpu->cpu_mstate_gen;

	membar_consumer();	/* guarantee load ordering */
	start = cpu->cpu_mstate_start;
	state = cpu->cpu_mstate;
	for (i = 0; i < NCMSTATES; i++) {
		intracct[i] = cpu->cpu_intracct[i];
		times[i] = cpu->cpu_acct[i];
	}
	membar_consumer();	/* guarantee load ordering */

	if (gen != 0 && gen == cpu->cpu_mstate_gen && now > start)
		times[state] += now - start;

	for (i = 0; i < NCMSTATES; i++) {
		scalehrtime(&times[i]);
		intracct[i] = tick2ns((hrtime_t)intracct[i]);
	}

	for (i = 0; i < NCMSTATES; i++) {
		if (i == CMS_SYSTEM)
			continue;
		times[i] -= intracct[i];
		if (times[i] < 0) {
			intracct[i] += times[i];
			times[i] = 0;
		}
		times[CMS_SYSTEM] += intracct[i];
		scalehrtime(&times[i]);
	}
	scalehrtime(&times[CMS_SYSTEM]);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pg_plat_hw_shared.                                */
/*                                                                  */
/* Function	- CMT related subroutine. Just return 0.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
pg_plat_hw_shared(cpu_t *cp, pghw_type_t hw)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pg_plat_hw_instance_id.                           */
/*                                                                  */
/* Function	- CMT related subroutine. Just return -1.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
id_t
pg_plat_hw_instance_id(cpu_t *cpu, pghw_type_t hw)
{
	return (-1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pg_plat_cmt_load_bal_hw.                          */
/*                                                                  */
/* Function	- CMT related subroutine. Just return 0.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
pg_plat_cmt_load_bal_hw(pghw_type_t hw)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pg_plat_cmt_affinity_hw.                          */
/*                                                                  */
/* Function	- CMT related subroutine. Just return 0.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
pg_plat_cmt_affinity_hw(pghw_type_t hw)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pg_plat_get_core_id.                              */
/*                                                                  */
/* Function	- CMT related subroutine. Just return 0.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

id_t
pg_plat_get_core_id(cpu_t *cpu)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pg_plat_cpus_share.                               */
/*                                                                  */
/* Function	- CMT related subroutine. Just return 0.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
pg_plat_cpus_share(cpu_t *cpu_a, cpu_t *cpu_b, pghw_type_t hw)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pg_plat_hw_level.                                 */
/*                                                                  */
/* Function	- Order the relevant hw sharing relationships from  */
/*		  least to greatest physical scope. In s390x - just */
/*		  return -1.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
pg_plat_hw_level(pghw_type_t hw)
{
	return (-1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cmp_set_nosteal_interval.                         */
/*                                                                  */
/* Function	- Set the nosteal interval, used by disp_getbest(), */
/*		  to 100 us.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cmp_set_nosteal_interval(void)
{
	nosteal_nsec = 100000UL;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mach_cpu_pause.                                   */
/*                                                                  */
/* Function	- Pause a CPU until notified to carry on.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mach_cpu_pause(volatile char *safe)
{
	/*
	 * This cpu is now safe.
	 */
	*safe = PAUSE_WAIT;
	membar_enter(); /* make sure stores are flushed */

	/*
	 * Now we wait.  When we are allowed to continue, safe
	 * will be set to PAUSE_IDLE.
	 */
	while (*safe != PAUSE_IDLE)
		SMT_PAUSE();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_mem_valid_page.                              */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
plat_mem_valid_page(uintptr_t pageaddr, uio_rw_t rw)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dump_plat_addr.                                   */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
dump_plat_addr()
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dump_plat_pfn.                                    */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
dump_plat_pfn()
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dump_plat_data.                                   */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
int
dump_plat_data(void *dump_cdata)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_hold_page.                                   */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
int
plat_hold_page(pfn_t pfn, int lock, page_t **pp_ret)
{
	return (PLAT_HOLD_OK);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_release_page.                                */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
plat_release_page(page_t *pp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- abort_sequence_enter.                             */
/*                                                                  */
/* Function	- Machine dependant abort sequence handling.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 *	Machine dependent abort sequence handling
 */
void
abort_sequence_enter(char *msg)
{
	if (abort_enable == 0) {
#ifdef C2_AUDIT
		if (audit_active)
			audit_enterprom(0);
#endif /* C2_AUDIT */
		return;
	}
#ifdef C2_AUDIT
	if (audit_active)
		audit_enterprom(1);
#endif /* C2_AUDIT */
	debug_enter(msg);
#ifdef C2_AUDIT
	if (audit_active)
		audit_exitprom(1);
#endif /* C2_AUDIT */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- console_enter.                                    */
/*                                                                  */
/* Function	- The underlying console output routines are pro-   */
/*		  tected by raising IPL in case we are still call-  */
/*		  ing into the early boot services. Once we start   */
/*		  calling the kernel console emulator, it will dis- */
/*		  able interrupts completely during character ren-  */
/*		  dering (see sysp_putchar, for example). Refer to  */
/*		  the comments and code in common/os/console.c for  */
/*		  more information on these callbacks.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
console_enter(int busy)
{
	return (splzs());
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- console_exit.                                     */
/*                                                                  */
/* Function	- Reverse the operation of consle_enter above.      */
/*                                                                  */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
console_exit(int busy, int spl)
{
	splx(spl);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- halt.                                             */
/*                                                                  */
/* Function	- Halt the machine and load a quiesce PSW.          */
/*                                                                  */
/*------------------------------------------------------------------*/

void
halt(char *s)
{
	stop_other_cpus();	/* send stop signal to other CPUs */
	if (s)
		prom_printf("(%s) \n", s);

	quiesce_cpu();
	/*NOTREACHED*/
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- debug_enter.                                      */
/*                                                                  */
/* Function	- Enter the debugger. Called whenever code wants to */
/*                enter the debugger and possibly resume later.     */
/*                                                                  */
/*------------------------------------------------------------------*/

void
debug_enter(char *msg)
{
	if (dtrace_debugger_init != NULL)
		(*dtrace_debugger_init)();

	if (msg)
		prom_printf("%s\n", msg);

	if (boothowto & RB_DEBUG)
		kmdb_enter();

	if (dtrace_debugger_fini != NULL)
		(*dtrace_debugger_fini)();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mach_cpu_start.                                   */
/*                                                                  */
/* Function	- Send a signal to another CPU to start itself.     */
/*                                                                  */
/*------------------------------------------------------------------*/

int
mach_cpu_start(struct cpu *cp, void *ep)
{
	_pfxPage *pfx;
	pswg_t	 *psw;
	ctlr0 	 *cr;

	pfx = cp->cpu_m.prefix;

	__asm__ ("	stctg	0,15,%0\n"
		 "	stamy	0,15,%1\n"
		 "	stg	%3,%2\n"
		 : "=m" (pfx->__lc_ssmc_cr_area),
		   "=m" (pfx->__lc_ssmc_ar_area),
		   "=m" (pfx->__lc_ssmc_gr_area)
		 : "r" (ep));

	/*
	 * We disable clock comparator interrupts until cyclic is 
	 * set up via a call to cyclic_mp_init
	 */
	cr		= (ctlr0 *) &pfx->__lc_ssmc_cr_area;
	cr->mask.clkCmp = 0;

	if ((sigp(cp->cpu_id, sigp_SetPrefix, 
		  (void *) cp->cpu_m.prefix, NULL) == sigp_NotOp) ||
	    (sigp(cp->cpu_id, sigp_Restart, NULL, NULL) == sigp_NotOp))
		return(-ENODEV);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mach_cpu_stop.                                    */
/*                                                                  */
/* Function	- Send a signal to another CPU to stop itself.      */
/*                                                                  */
/*------------------------------------------------------------------*/

void
mach_cpu_stop(int cpuId)
{
	if (sigp(cpuId, sigp_Stop, NULL, NULL) == sigp_NotOp)
		panic("Attempted to stop a non-operational CPU - %d\n",
		      cpuId);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- add_cpu2devnodetree.                              */
/*                                                                  */
/* Function	- Create a node for the given CPU in the device     */
/*                tree.                                             */
/*                                                                  */
/*------------------------------------------------------------------*/

void
add_cpunode2devtree(processorid_t cpu_id)
{
	dev_info_t *cpu_devi;

	mutex_enter(&cpu_node_lock);

	/*
	 * create a nexus node for all cpus identified as 'cpu_id' under
	 * the root node.
	 */
	if (cpu_nex_devi == NULL) {
		if (ndi_devi_alloc(ddi_root_node(), "cpus",
		    (pnode_t)DEVI_SID_NODEID, &cpu_nex_devi) != NDI_SUCCESS) {
			mutex_exit(&cpu_node_lock);
			return;
		}
		prom_printf("Bringing CPU online\n");
		(void) ndi_devi_online(cpu_nex_devi, 0);
	}
mutex_exit(&cpu_node_lock);
return;

/* S390X FIXME */

	/*
	 * create a child node for cpu identified as 'cpu_id'
	 */
	cpu_devi = ddi_add_child(cpu_nex_devi, "cpu", DEVI_SID_NODEID,
		cpu_id);

	if (cpu_devi == NULL) {
		mutex_exit(&cpu_node_lock);
		return;
	}

	/* device_type */

	(void) ndi_prop_update_string(DDI_DEV_T_NONE, cpu_devi,
				      "device_type", "cpu");

	/* reg */

	(void) ndi_prop_update_int(DDI_DEV_T_NONE, cpu_devi,
				   "reg", cpu_id);

	prom_printf("Bringing CPU %d online\n",cpu_id);
	(void) ndi_devi_online(cpu_devi, 0);

	mutex_exit(&cpu_node_lock);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mach_set_softintr.                                */
/*                                                                  */
/* Function	- Set a soft interrupt pending. Check to see if     */
/*                the CPU is in such a state that it will be doing  */
/*                dosoftint without our intervention. If not issue  */
/*                the softint SVC.                                  */
/*                                                                  */
/*------------------------------------------------------------------*/

static void
mach_set_softintr(int ipl, struct av_softinfo *pending)
{
	/* set software pending bits				    */
	av_set_softint_pending(ipl, pending);

	/* check if dosoftint will be called at the end of intr	    */
	if (CPU_ON_INTR(CPU) || (curthread->t_intr))
		return;

	sigsoftint(ipl);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_wakeup.                                       */
/*                                                                  */
/* Function	- If a thread is to be dispatched is not on this    */
/*                CPU and that CPU  is in the quiesced state then   */
/*		  wake it up using an emergency signal.		    */
/*                                                                  */
/*                First we check if the target is idling, if so     */
/*                we use an emergency signal to interrupt the CPU.  */
/*                If not the we set the cpu_m.idling flag to tell   */
/*                the other CPU not to wait but go back and check   */
/*                that a resource has been freed up.                */
/*                                                                  */
/*                The variable cpu_m.idling can take 1 of 3 values: */
/*                - 0 = CPU is not in wait state                    */
/*                - 1 = CPU is in wait state                        */
/*                - 2 = CPU should not go into wait state but check */
/*                      that a resource may have been freed         */
/*                                                                  */
/*------------------------------------------------------------------*/

void
cpu_wakeup(struct cpu *cp, int bound)
{
	int idleVal;
	
	if (cp != CPU) {
		idleVal = __sync_val_compare_and_swap(&cp->cpu_m.idling,
						      0, 2);
		if (idleVal == 1)
			poke_cpu(cp->cpu_id);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- wake_others.                                      */
/*                                                                  */
/* Function	- 						    */
/*                                                                  */
/*------------------------------------------------------------------*/

void
wake_others()
{
	int who;

	for (who = 0; who < NCPU; who++) {

		if (who != CPU->cpu_id && cpu[who] != NULL &&
		    (cpu[who]->cpu_flags & CPU_EXISTS)) { 

			if (!CPU_IN_SET(cpu_ready_set, who))
				continue;
		
			cpu_wakeup(cpu[who], 0);
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dumpThread.                                       */
/*                                                                  */
/* Function	- 						    */
/*                                                                  */
/*------------------------------------------------------------------*/

void
dumpThread(kthread_t *tp)
{
	proc_t   *pr = tp->t_procp;
	tsproc_t *ts = (tsproc_t *) tp->t_cldata;

	if (tp->t_state != TS_FREE || tp == curthread) {
		msgnoh("Thread: %p",tp);
		msgnoh("   Link:  %p",tp->t_link);
		msgnoh("   Start: %p",tp->t_startpc);
		msgnoh("   Proc:  %p",pr);
		msgnoh("   Pid:   %d",pr->p_pid);
		msgnoh("   Pstat: %d",pr->p_stat);
		msgnoh("   as:    %p",pr->p_as);
		if (pr->p_as != &kas) {
			hat_t *hat = (hat_t *) pr->p_as->a_hat;
			msgnoh("   hat:   %p",hat);
			msgnoh("   ht:    %p",hat->hat_htable);
			msgnoh("   org:   %lx",(hat->hat_htable->ht_pfn << PAGESHIFT));
		}
		msgnoh("   State: %lx",tp->t_state);
		msgnoh("   Flags: %x",tp->t_flag);
		msgnoh("   pc:    %lx",tp->t_pc);
		msgnoh("   Stack: %lx",tp->t_sp);
		msgnoh("   Base:  %lx",tp->t_stk);
		msgnoh("   pil:   %lx",tp->t_pil);
		msgnoh("   lwp:   %lx",tp->t_lwp);
		msgnoh("   lbolt: %lx",tp->t_lbolt);
		msgnoh("   sysno: %d",tp->t_sysnum);
		msgnoh("   runtm: %lx",tp->t_disp_time);
		msgnoh("   queue: %p",tp->t_sleepq);
		msgnoh("   cpu:   %d",tp->t_cpu->cpu_id);
		msgnoh("   intr:  %p",tp->t_intr);
		msgnoh("   intm:  %lx",tp->t_intr_start);
		msgnoh("   chan:  %p",tp->t_wchan);
		msgnoh("   chan0: %p",tp->t_wchan0);
		msgnoh("   schd:  %p",tp->t_schedflag);
		msgnoh("   sigs:  %08x %08x",tp->t_sig.__sigbits[0],tp->t_sig.__sigbits[1]);
		msgnoh("   csig:  %08x %08x",tp->t_extsig.__sigbits[0],tp->t_extsig.__sigbits[1]);
		msgnoh("   left:  %lx",ts->ts_timeleft);
		msgnoh("   tsfl:  %x",ts->ts_flags);
		msgnoh("   schd:  %x",tp->t_schedctl);
		msgnoh(" ");
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dumpCPU.	                                    */
/*                                                                  */
/* Function	- 						    */
/*                                                                  */
/*------------------------------------------------------------------*/

void
dumpCPU()
{
	int iCPU;
	kthread_t *tp, *me = curthread;
	uint64_t clkC, tod;

	__asm__ ("stckc	%0\n"
		 : "=m" (clkC) : : "cc");
	tod = stck();

	msgnoh("Dumping CPUs and Threads at %lx",lbolt);
	msgnoh("---------------------------------");
	msgnoh(" ");
	msgnoh("Clock:   %lx Comparator: %lx",tod,clkC);
	msgnoh("Freemem: %lx",freemem);
	msgnoh(" ");

	for (iCPU = 0; iCPU < NCPU; iCPU++) {
		if (cpu[iCPU]->cpu_flags & CPU_EXISTS) {
			msgnoh("CPU: %d @ %p",iCPU,cpu[iCPU]);
			msgnoh("   id:     %d",cpu[iCPU]->cpu_id);
			msgnoh("   seq:    %d",cpu[iCPU]->cpu_seqid);
			msgnoh("   spl:    %d",cpu[iCPU]->cpu_base_spl);
			msgnoh("   act:    %08x",cpu[iCPU]->cpu_intr_actv);
			msgnoh("   flags:  %x",cpu[iCPU]->cpu_flags);
			msgnoh("   thread: %p",cpu[iCPU]->cpu_thread);
			msgnoh("   lwp:    %p",cpu[iCPU]->cpu_lwp);
			msgnoh("   idle:   %p",cpu[iCPU]->cpu_idle_thread);
			msgnoh("   intr:   %p",cpu[iCPU]->cpu_intr_thread);
			msgnoh("   lstclk: %lx",cpu[iCPU]->cpu_m.clk);
			msgnoh("   lstckc: %lx",cpu[iCPU]->cpu_m.ckc);
			msgnoh("   pri:    %d",cpu[iCPU]->cpu_m.mcpu_pri);
			msgnoh("   runrun: %d",cpu[iCPU]->cpu_runrun);
			msgnoh("   kpremp: %d",cpu[iCPU]->cpu_kprunrun);
			msgnoh("   mutex:  %lx",(uint64_t) cpu[iCPU]->cpu_m.sigMut._opaque[1]);
			msgnoh(" ");
		}
	}

	for (tp = me;
	     tp->t_next != me;
	     tp = tp->t_next) {
		dumpThread(tp);
	}

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dumpSys.	                                    */
/*                                                                  */
/* Function	- Dump the CPU and thread data structures.	    */
/*                                                                  */
/*------------------------------------------------------------------*/

uint_t
dumpSys(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;

	if (ip->u.ext.intcode != EXT_OPRI) {
		return (DDI_INTR_UNCLAIMED);
	}

	dumpCPU();

	return (DDI_INTR_CLAIMED);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_mem_do_mmio.	                            */
/*                                                                  */
/* Function	- Foreign page support for establishing temporary   */
/*                mappings. Not required on System z.               */
/*                                                                  */
/*------------------------------------------------------------------*/
/*ARGSUSED*/
int
plat_mem_do_mmio(struct uio *uio, enum uio_rw rw)
{
	return (ENOTSUP);
}

/*========================= End of Function ========================*/
