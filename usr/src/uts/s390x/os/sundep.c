/*------------------------------------------------------------------*/
/* 								    */
/* Name        - sundep.c   					    */
/* 								    */
/* Function    - Various startup support routines.                  */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - May, 2007  					    */
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
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/class.h>
#include <sys/proc.h>
#include <sys/procfs.h>
#include <sys/kmem.h>
#include <sys/cred.h>
#include <sys/archsystm.h>
#include <sys/contract_impl.h>

#include <sys/reboot.h>
#include <sys/uadmin.h>

#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/session.h>
#include <sys/ucontext.h>

#include <sys/dnlc.h>
#include <sys/var.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/thread.h>
#include <sys/vtrace.h>
#include <sys/consdev.h>
#include <sys/frame.h>
#include <sys/stack.h>
#include <sys/swap.h>
#include <sys/vmparam.h>
#include <sys/cpuvar.h>
#include <sys/cpu.h>

#include <sys/privregs.h>

#include <vm/hat.h>
#include <vm/anon.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/seg_kmem.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>

#include <sys/exec.h>
#include <sys/acct.h>
#include <sys/corectl.h>
#include <sys/modctl.h>
#include <sys/tuneable.h>

#include <c2/audit.h>

#include <sys/trap.h>
#include <sys/sunddi.h>
#include <sys/bootconf.h>
#include <sys/memlist.h>
#include <sys/systeminfo.h>
#include <sys/promif.h>
#include <sys/stack.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern void thread_start();

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- check_boot_version.                               */
/*                                                                  */
/* Function	- Compare the version of boot that boot says it is  */
/*		  against the version of boot the kernel expects.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
check_boot_version(int boots_version)
{
	if (boots_version == BO_VERSION)
		return (0);

	prom_printf("Wrong boot interface - kernel needs v%d found v%d\n",
	    BO_VERSION, boots_version);
	prom_panic("halting");
	/*NOTREACHED*/
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kern_setup1.                                      */
/*                                                                  */
/* Function	- Kernel setup code that is called from startup().  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kern_setup1(void)
{
	proc_t *pp;

	pp = &p0;

	proc_sched = pp;

	/*
	 * Initialize process 0 data structures
	 */
	pp->p_stat      = SRUN;
	pp->p_flag      = SSYS;

	pp->p_pidp      = &pid0;
	pp->p_pgidp     = &pid0;
	pp->p_sessp     = &session0;
	pp->p_tlist     = &t0;
	pid0.pid_pglink = pp;
	pid0.pid_pgtail = pp;

	/*
	 * We assume that the u-area is zeroed out.
	 */
	PTOU(curproc)->u_cmask = (mode_t)CMASK;

	thread_init();		/* init thread_free list */
	pid_init();		/* initialize pid (proc) table */
	contract_init();	/* initialize contracts */
	init_pages_pp_maximum();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- thread_load.                                      */
/*                                                                  */
/* Function	- Load a procedure into a thread.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
thread_load(kthread_t *t, void (*start)(), caddr_t arg, size_t len)
{
	caddr_t  sp;
	size_t 	 framesz;
	caddr_t  argp;
	s390xstk *stk;
	pswg_t	 *psw;
	struct	 regs *r;
	klwp_t	 *lwp;
	int32_t	 *fpc;

	/*
	 * Push a "c" call frame onto the stack to represent
	 * the caller of "start".
	 */
	sp       = t->t_stk - SA(MINFRAME + FPSAVESZ);
	if (len != 0) {
		/*
		 * the object that arg points at is copied into the
		 * caller's frame.
		 */
		framesz  = SA(len);
		sp 	-= framesz;
		ASSERT(sp > t->t_stkbase);
		argp 	 = sp + SA(MINFRAME);
		bcopy(arg, argp, len);
		arg	 = argp;
	}

	fpc = (int32_t *) (t->t_stk - FPSAVESZ + FPFPC);
	__asm__ ("	stfpc	%0\n"
		 : "=Q" (*fpc));

	/*
	 * store arg and len into the frames input register save area.
	 * these are then transfered to R2 and R3 by thread_start() 
	 * in swtch.s.
	 */
	stk              = (s390xstk *) sp;
	stk->st_regs[0]  = (uint64_t) arg;		// r2 
	stk->st_regs[1]  = len;				// r3
	stk->st_regs[11] = (uint64_t) start;		// r13
	stk->st_regs[13] = (uintptr_t) sp - STACK_BIAS; // r15

	/*
	 * initialize thread to resume at thread_start().
	 */
	t->t_pc = (uintptr_t) thread_start;
	t->t_sp = (uintptr_t) sp - STACK_BIAS;
	if (t->t_lwp != NULL) {
		lwp 	 = t->t_lwp;
		r   	 = (struct regs *) lwp->lwp_regs;
		psw 	 = (pswg_t *) &r->r_psw;
		psw->pc  = stk->st_regs[11];
		r->r_g2  = stk->st_regs[0];
		r->r_g3  = stk->st_regs[1];
		r->r_g13 = stk->st_regs[11];
		r->r_sp  = t->t_sp;
	}
}

/*========================= End of Function ========================*/

#if !defined(lwp_getdatamodel)

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_getdatamodel.                                 */
/*                                                                  */
/* Function	- Return the datamodel of the given lwp.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/


model_t
lwp_getdatamodel(klwp_t *lwp)
{
	return (lwp->lwp_procp->p_model);
}

/*========================= End of Function ========================*/

#endif	/* !lwp_getdatamodel */

#if !defined(get_udatamodel)

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

model_t
get_udatamodel(void)
{
	return (curproc->p_model);
}

/*========================= End of Function ========================*/

#endif	/* !get_udatamodel */
