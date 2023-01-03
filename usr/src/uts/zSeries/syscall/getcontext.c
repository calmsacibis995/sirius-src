/*------------------------------------------------------------------*/
/* 								    */
/* Name        - getcontext.c					    */
/* 								    */
/* Function    - System z implementation of get/save/restore        */
/* 		 context.					    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - September, 2007 				    */
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
#include <sys/vmparam.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/proc.h>
#include <sys/ucontext.h>
#include <sys/asm_linkage.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/archsystm.h>
#include <sys/debug.h>
#include <sys/model.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/privregs.h>
#include <sys/schedctl.h>

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

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- savecontext.                                      */
/*                                                                  */
/* Function	- Save user context.                                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
savecontext(ucontext_t *ucp, k_sigset_t mask)
{
	proc_t *p = ttoproc(curthread);
	klwp_t *lwp = ttolwp(curthread);

	bzero(&ucp->uc_mcontext, sizeof (mcontext_t));

	ucp->uc_flags = UC_ALL;
	ucp->uc_link = (ucontext_t *)lwp->lwp_oldcontext;

	/*
	 * Try to copyin() the ustack if one is registered. If the stack
	 * has zero size, this indicates that stack bounds checking has
	 * been disabled for this LWP. If stack bounds checking is disabled
	 * or the copyin() fails, we fall back to the legacy behavior.
	 */
	if (lwp->lwp_ustack == NULL ||
	    copyin((void *)lwp->lwp_ustack, &ucp->uc_stack,
	    sizeof (ucp->uc_stack)) != 0 ||
	    ucp->uc_stack.ss_size == 0) {

		if (lwp->lwp_sigaltstack.ss_flags == SS_ONSTACK) {
			ucp->uc_stack = lwp->lwp_sigaltstack;
		} else {
			ucp->uc_stack.ss_sp    = p->p_usrstack - p->p_stksize;
			ucp->uc_stack.ss_size  = p->p_stksize;
			ucp->uc_stack.ss_flags = 0;
		}
	}

	getpsw(lwp, &ucp->uc_mcontext.psw);
	getgregs(lwp, ucp->uc_mcontext.gregs);
	getaregs(lwp, ucp->uc_mcontext.aregs);
	getfpregs(lwp, &ucp->uc_mcontext.fpregs);

	/*
	 * Save signal mask.
	 */
	sigktou(&mask, &ucp->uc_sigmask);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- restorecontext.                                   */
/*                                                                  */
/* Function	- Restore user context.                             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
restorecontext(ucontext_t *ucp)
{
	kthread_t *t = curthread;
	klwp_t *lwp = ttolwp(t);
	mcontext_t *mcp = &ucp->uc_mcontext;
	model_t model = lwp_getdatamodel(lwp);
	fpregset_t *fp = &ucp->uc_mcontext.fpregs;

	lwp->lwp_oldcontext = (uintptr_t)ucp->uc_link;

	if (ucp->uc_flags & UC_STACK) {
		if (ucp->uc_stack.ss_flags == SS_ONSTACK)
			lwp->lwp_sigaltstack = ucp->uc_stack;
		else
			lwp->lwp_sigaltstack.ss_flags &= ~SS_ONSTACK;
	}

	setgregs(lwp, mcp->gregs);
	setaregs(lwp, mcp->aregs);
	setfpregs(lwp, fp);
	lwptoregs(lwp)->r_pc = mcp->psw.pc;

	if (ucp->uc_flags & UC_SIGMASK) {
		proc_t *p = ttoproc(t);

		mutex_enter(&p->p_lock);
		schedctl_finish_sigblock(t);
		sigutok(&ucp->uc_sigmask, &t->t_hold);
		if (sigcheck(p, t))
			t->t_sig_check = 1;
		mutex_exit(&p->p_lock);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getsetcontext.                                    */
/*                                                                  */
/* Function	- Process get/set user context type requests.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
getsetcontext(int flag, void *arg)
{
	ucontext_t uc;
	fpregset_t *fpp;
	ucontext_t *ucp;
	klwp_t *lwp = ttolwp(curthread);
	stack_t dummy_stk;

	/*
	 * In future releases, when the ucontext structure grows,
	 * getcontext should be modified to only return the fields
	 * specified in the uc_flags.  That way, the structure can grow
	 * and still be binary compatible will all .o's which will only
	 * have old fields defined in uc_flags
	 */

	switch (flag) {
	default:
		return (set_errno(EINVAL));

	case GETCONTEXT:
		if (schedctl_sigblock(curthread)) {
			proc_t *p = ttoproc(curthread);
			mutex_enter(&p->p_lock);
			schedctl_finish_sigblock(curthread);
			mutex_exit(&p->p_lock);
		}
		savecontext(&uc, curthread->t_hold);
		if (copyout(&uc, arg, sizeof (ucontext_t)))
			return (set_errno(EFAULT));
		return (0);

	case SETCONTEXT:
		ucp = arg;
		if (ucp == NULL)
			exit(CLD_EXITED, 0);
		/*
		 * Don't copyin filler or floating state unless we need it.
		 * The ucontext_t struct and fields are specified in the ABI.
		 */
		if (copyin(ucp, &uc, sizeof (ucontext_t))) {
			return (set_errno(EFAULT));
		}
		restorecontext(&uc);

		if ((uc.uc_flags & UC_STACK) && (lwp->lwp_ustack != 0)) {
			(void) copyout(&uc.uc_stack, (stack_t *)lwp->lwp_ustack,
			    sizeof (stack_t));
		}

		lwp->lwp_unused = -1;

		/*
		 * We need to return to the syscall handler a value that will
		 * be placed in register 2. So we use the value that we put in
		 * the user context structure
		 */
		return (uc.uc_mcontext.gregs[2]);

	case GETUSTACK:
		if (copyout(&lwp->lwp_ustack, arg, sizeof (caddr_t)))
			return (set_errno(EFAULT));

		return (0);

	case SETUSTACK:
		if (copyin(arg, &dummy_stk, sizeof (dummy_stk)))
			return (set_errno(EFAULT));

		lwp->lwp_ustack = (uintptr_t)arg;

		return (0);
	}
}

/*========================= End of Function ========================*/

#ifdef _SYSCALL32_IMPL

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- savecontext32.                                    */
/*                                                                  */
/* Function	- Save user context for 32-bit processes.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
savecontext32(ucontext32_t *ucp, k_sigset_t mask)
{
	proc_t *p = ttoproc(curthread);
	klwp_t *lwp = ttolwp(curthread);

	/*
	 * We assign to every field through uc_mcontext.fpregs.fpu_en,
	 * but we have to bzero() everything after that.
	 */
	bzero(&ucp->uc_mcontext, sizeof (mcontext32_t));
	ucp->uc_flags = UC_ALL;
	ucp->uc_link = (caddr32_t)lwp->lwp_oldcontext;

	/*
	 * Try to copyin() the ustack if one is registered. If the stack
	 * has zero size, this indicates that stack bounds checking has
	 * been disabled for this LWP. If stack bounds checking is disabled
	 * or the copyin() fails, we fall back to the legacy behavior.
	 */
	if (lwp->lwp_ustack == NULL ||
	    copyin((void *)lwp->lwp_ustack, &ucp->uc_stack,
	    sizeof (ucp->uc_stack)) != 0 ||
	    ucp->uc_stack.ss_size == 0) {

		if (lwp->lwp_sigaltstack.ss_flags == SS_ONSTACK) {
			ucp->uc_stack.ss_sp =
			    (caddr32_t)(uintptr_t)lwp->lwp_sigaltstack.ss_sp;
			ucp->uc_stack.ss_size =
			    (size32_t)lwp->lwp_sigaltstack.ss_size;
			ucp->uc_stack.ss_flags = SS_ONSTACK;
		} else {
			ucp->uc_stack.ss_sp =
			    (caddr32_t)(uintptr_t)p->p_usrstack - p->p_stksize;
			ucp->uc_stack.ss_size =
			    (size32_t)p->p_stksize;
			ucp->uc_stack.ss_flags = 0;
		}
	}

	getpsw(lwp, &ucp->uc_mcontext.psw);
	getgregs32(lwp, ucp->uc_mcontext.gregs);
	getaregs(lwp, ucp->uc_mcontext.aregs);
	getfpregs(lwp, &ucp->uc_mcontext.fpregs);

	/*
	 * Save signal mask (the 32- and 64-bit sigset_t structures are
	 * identical).
	 */
	sigktou(&mask, (sigset_t *)&ucp->uc_sigmask);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getsetcontext32.                                  */
/*                                                                  */
/* Function	- Process get/set context type requests for 32-bit  */
/*		  processes.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
getsetcontext32(int flag, void *arg)
{
	ucontext32_t uc;
	ucontext_t   ucnat;
	fpregset_t *fpp;
	extern int nwindows;
	klwp_t *lwp = ttolwp(curthread);
	ucontext32_t *ucp;
	uint32_t ustack32;
	stack32_t dummy_stk32;

	/*
	 * In future releases, when the ucontext structure grows,
	 * getcontext should be modified to only return the fields
	 * specified in the uc_flags.  That way, the structure can grow
	 * and still be binary compatible will all .o's which will only
	 * have old fields defined in uc_flags
	 */

	switch (flag) {
	default:
		return (set_errno(EINVAL));

	case GETCONTEXT:
		if (schedctl_sigblock(curthread)) {
			proc_t *p = ttoproc(curthread);
			mutex_enter(&p->p_lock);
			schedctl_finish_sigblock(curthread);
			mutex_exit(&p->p_lock);
		}
		savecontext32(&uc, curthread->t_hold);
		if (copyout(&uc, arg, sizeof (ucontext32_t)))
			return (set_errno(EFAULT));
		return (0);

	case SETCONTEXT:
		ucp = arg;
		if (ucp == NULL)
			exit(CLD_EXITED, 0);
		if (copyin(ucp, &uc, sizeof (uc))) {
			return (set_errno(EFAULT));
		}
		ucontext_32ton(&uc, &ucnat);

		restorecontext(&ucnat);

		if ((uc.uc_flags & UC_STACK) && (lwp->lwp_ustack != 0)) {
			(void) copyout(&uc.uc_stack,
			    (stack32_t *)lwp->lwp_ustack, sizeof (stack32_t));
		}

		lwp->lwp_unused = -1;

		/*
		 * We need to return to the syscall handler a value that will
		 * be placed in register 2. So we use the value that we put in
		 * the user context structure
		 */
		return (uc.uc_mcontext.gregs[2]);

	case GETUSTACK:
		ustack32 = (uint32_t)lwp->lwp_ustack;
		if (copyout(&ustack32, arg, sizeof (caddr32_t)))
			return (set_errno(EFAULT));

		return (0);

	case SETUSTACK:
		if (copyin(arg, &dummy_stk32, sizeof (dummy_stk32)))
			return (set_errno(EFAULT));

		lwp->lwp_ustack = (uintptr_t)arg;

		return (0);
	}
}

/*========================= End of Function ========================*/

#endif
