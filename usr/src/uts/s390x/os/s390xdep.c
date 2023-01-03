/*------------------------------------------------------------------*/
/* 								    */
/* Name        - s390xdep.c 					    */
/* 								    */
/* Function    - Some System z specific routines.                   */
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


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/vmparam.h>
#include <sys/systm.h>
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/proc.h>
#include <sys/ucontext.h>
#include <sys/cpuvar.h>
#include <sys/asm_linkage.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/bootconf.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/debug.h>
#include <sys/privregs.h>
#include <sys/cmn_err.h>
#include <sys/copyops.h>
#include <sys/model.h>
#include <sys/panic.h>
#include <sys/exec.h>
#include <sys/user.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

struct sigframe {
	ucontext_t uc;
};

#ifdef _SYSCALL32_IMPL

struct sigframe32 {
	ucontext32_t uc;
};

#endif

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
/* Name		- setfpregs.                                        */
/*                                                                  */
/* Function	- Set floating-point registers. Note: lwp might not */
/*		  correspond to 'curthread' since this is called    */
/*		  from code in /proc to set the registers of an-    */
/*		  other lwp.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
setfpregs(klwp_t *lwp, fpregset_t *fp)
{
	struct regs *rp = lwptoregs(lwp);

	rp->r_f0  = fp->fr.fd[REG_F0];
	rp->r_f1  = fp->fr.fd[REG_F1];
	rp->r_f2  = fp->fr.fd[REG_F2];
	rp->r_f3  = fp->fr.fd[REG_F3];
	rp->r_f4  = fp->fr.fd[REG_F4];
	rp->r_f5  = fp->fr.fd[REG_F5];
	rp->r_f6  = fp->fr.fd[REG_F6];
	rp->r_f7  = fp->fr.fd[REG_F7];
	rp->r_f8  = fp->fr.fd[REG_F8];
	rp->r_f9  = fp->fr.fd[REG_F9];
	rp->r_f10 = fp->fr.fd[REG_F10];
	rp->r_f11 = fp->fr.fd[REG_F11];
	rp->r_f12 = fp->fr.fd[REG_F12];
	rp->r_f13 = fp->fr.fd[REG_F13];
	rp->r_f14 = fp->fr.fd[REG_F14];
	rp->r_f15 = fp->fr.fd[REG_F15];
	rp->r_fpc = fp->fpc;
 
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getfpregs.                                        */
/*                                                                  */
/* Function	- Get floating-point registers. Note: lwp might not */
/*		  correspond to 'curthread' since this is called    */
/*		  from code in /proc to set the registers of an-    */
/*		  other lwp.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
getfpregs(klwp_t *lwp, fpregset_t *fp)
{
	struct regs *rp = lwptoregs(lwp);

	fp->fr.fd[REG_F0]  = rp->r_f0;
	fp->fr.fd[REG_F1]  = rp->r_f1;
	fp->fr.fd[REG_F2]  = rp->r_f2;
	fp->fr.fd[REG_F3]  = rp->r_f3;
	fp->fr.fd[REG_F4]  = rp->r_f4;
	fp->fr.fd[REG_F5]  = rp->r_f5;
	fp->fr.fd[REG_F6]  = rp->r_f6;
	fp->fr.fd[REG_F7]  = rp->r_f7;
	fp->fr.fd[REG_F8]  = rp->r_f8;
	fp->fr.fd[REG_F9]  = rp->r_f9;
	fp->fr.fd[REG_F10] = rp->r_f10;
	fp->fr.fd[REG_F11] = rp->r_f11;
	fp->fr.fd[REG_F12] = rp->r_f12;
	fp->fr.fd[REG_F13] = rp->r_f13;
	fp->fr.fd[REG_F14] = rp->r_f14;
	fp->fr.fd[REG_F15] = rp->r_f15;
	fp->fpc 	   = rp->r_fpc;
 
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setgregs.                                         */
/*                                                                  */
/* Function	- Set general registers. Note 'lwp' might not       */
/*		  correspond to 'curthread' since this is called    */
/*		  from code in /proc to set the registers of an-    */
/*		  other lwp.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
setgregs(klwp_t *lwp, gregset_t grp)
{
	struct regs *rp = lwptoregs(lwp);

	rp->r_g0  = grp[REG_G0];
	rp->r_g1  = grp[REG_G1];
	rp->r_g2  = grp[REG_G2];
	rp->r_g3  = grp[REG_G3];
	rp->r_g4  = grp[REG_G4];
	rp->r_g5  = grp[REG_G5];
	rp->r_g6  = grp[REG_G6];
	rp->r_g7  = grp[REG_G7];
	rp->r_g8  = grp[REG_G8];
	rp->r_g9  = grp[REG_G9];
	rp->r_g10 = grp[REG_G10];
	rp->r_g11 = grp[REG_G11];
	rp->r_g12 = grp[REG_G12];
	rp->r_g13 = grp[REG_G13];
	rp->r_g14 = grp[REG_G14];
	rp->r_g15 = grp[REG_G15];
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setaregs.                                         */
/*                                                                  */
/* Function	- Set access registers. Note 'lwp' might not        */
/*		  correspond to 'curthread' since this is called    */
/*		  from code in /proc to set the registers of an-    */
/*		  other lwp.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
setaregs(klwp_t *lwp, aregset_t arp)
{
	struct regs *rp = lwptoregs(lwp);

	rp->r_a0  = arp[REG_A0];
	rp->r_a1  = arp[REG_A1];
	rp->r_a2  = arp[REG_A2];
	rp->r_a3  = arp[REG_A3];
	rp->r_a4  = arp[REG_A4];
	rp->r_a5  = arp[REG_A5];
	rp->r_a6  = arp[REG_A6];
	rp->r_a7  = arp[REG_A7];
	rp->r_a8  = arp[REG_A8];
	rp->r_a9  = arp[REG_A9];
	rp->r_a10 = arp[REG_A10];
	rp->r_a11 = arp[REG_A11];
	rp->r_a12 = arp[REG_A12];
	rp->r_a13 = arp[REG_A13];
	rp->r_a14 = arp[REG_A14];
	rp->r_a15 = arp[REG_A15];
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getgregs.                                         */
/*                                                                  */
/* Function	- Set general registers. Note 'lwp' might not       */
/*		  correspond to 'curthread' since this is called    */
/*		  from code in /proc to set the registers of an-    */
/*		  other lwp.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
getgregs(klwp_t *lwp, gregset_t grp)
{
	struct regs *rp = lwptoregs(lwp);

	grp[REG_G0]  = rp->r_g0;
	grp[REG_G1]  = rp->r_g1;
	grp[REG_G2]  = rp->r_g2;
	grp[REG_G3]  = rp->r_g3;
	grp[REG_G4]  = rp->r_g4;
	grp[REG_G5]  = rp->r_g5;
	grp[REG_G6]  = rp->r_g6;
	grp[REG_G7]  = rp->r_g7;
	grp[REG_G8]  = rp->r_g8;
	grp[REG_G9]  = rp->r_g9;
	grp[REG_G10] = rp->r_g10;
	grp[REG_G11] = rp->r_g11;
	grp[REG_G12] = rp->r_g12;
	grp[REG_G13] = rp->r_g13;
	grp[REG_G14] = rp->r_g14;
	grp[REG_G15] = rp->r_g15;
	grp[REG_ILC] = rp->r_ilc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getaregs.                                         */
/*                                                                  */
/* Function	- Set access registers. Note 'lwp' might not        */
/*		  correspond to 'curthread' since this is called    */
/*		  from code in /proc to set the registers of an-    */
/*		  other lwp.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
getaregs(klwp_t *lwp, aregset_t arp)
{
	struct regs *rp = lwptoregs(lwp);

	arp[REG_A0]  = rp->r_a0;
	arp[REG_A1]  = rp->r_a1;
	arp[REG_A2]  = rp->r_a2;
	arp[REG_A3]  = rp->r_a3;
	arp[REG_A4]  = rp->r_a4;
	arp[REG_A5]  = rp->r_a5;
	arp[REG_A6]  = rp->r_a6;
	arp[REG_A7]  = rp->r_a7;
	arp[REG_A8]  = rp->r_a8;
	arp[REG_A9]  = rp->r_a9;
	arp[REG_A10] = rp->r_a10;
	arp[REG_A11] = rp->r_a11;
	arp[REG_A12] = rp->r_a12;
	arp[REG_A13] = rp->r_a13;
	arp[REG_A14] = rp->r_a14;
	arp[REG_A15] = rp->r_a15;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setgregs32.                                       */
/*                                                                  */
/* Function	- Set general registers. Note 'lwp' might not       */
/*		  correspond to 'curthread' since this is called    */
/*		  from code in /proc to set the registers of an-    */
/*		  other lwp.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
setgregs32(klwp_t *lwp, gregset32_t grp)
{
	struct regs *rp = lwptoregs(lwp);

	rp->r_g0  = grp[REG_G0];
	rp->r_g1  = grp[REG_G1];
	rp->r_g2  = grp[REG_G2];
	rp->r_g3  = grp[REG_G3];
	rp->r_g4  = grp[REG_G4];
	rp->r_g5  = grp[REG_G5];
	rp->r_g6  = grp[REG_G6];
	rp->r_g7  = grp[REG_G7];
	rp->r_g8  = grp[REG_G8];
	rp->r_g9  = grp[REG_G9];
	rp->r_g10 = grp[REG_G10];
	rp->r_g11 = grp[REG_G11];
	rp->r_g12 = grp[REG_G12];
	rp->r_g13 = grp[REG_G13];
	rp->r_g14 = grp[REG_G14];
	rp->r_g15 = grp[REG_G15];
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getgregs32.                                       */
/*                                                                  */
/* Function	- Set general registers. Note 'lwp' might not       */
/*		  correspond to 'curthread' since this is called    */
/*		  from code in /proc to set the registers of an-    */
/*		  other lwp.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
getgregs32(klwp_t *lwp, gregset32_t grp)
{
	struct regs *rp = lwptoregs(lwp);

	grp[REG_G0]  = rp->r_g0;
	grp[REG_G1]  = rp->r_g1;
	grp[REG_G2]  = rp->r_g2;
	grp[REG_G3]  = rp->r_g3;
	grp[REG_G4]  = rp->r_g4;
	grp[REG_G5]  = rp->r_g5;
	grp[REG_G6]  = rp->r_g6;
	grp[REG_G7]  = rp->r_g7;
	grp[REG_G8]  = rp->r_g8;
	grp[REG_G9]  = rp->r_g9;
	grp[REG_G10] = rp->r_g10;
	grp[REG_G11] = rp->r_g11;
	grp[REG_G12] = rp->r_g12;
	grp[REG_G13] = rp->r_g13;
	grp[REG_G14] = rp->r_g14;
	grp[REG_G15] = rp->r_g15;
	grp[REG_ILC] = rp->r_ilc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getpsw.                                           */
/*                                                                  */
/* Function	- Set the PSW. Note 'lwp' might not correspond to   */
/*		  'curthread' since this is called from code in     */
/*		  /proc to set the registers of another lwp.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
getpsw(klwp_t *lwp, pswg_t *psw)
{
	struct regs *rp = lwptoregs(lwp);

	memcpy(psw, &rp->r_psw, sizeof(pswg_t));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setpc.                                            */
/*                                                                  */
/* Function	- Set the PC portion of the PSW. Note 'lwp' might   */
/*		  not correspond to 'curthread' since this is	    */
/*		  called from code in /proc to set the registers of */
/*		  another lwp.                   		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
setpc(klwp_t *lwp, uint64_t pc)
{
	struct regs *rp = lwptoregs(lwp);

	rp->r_psw[1] = pc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getuserpc.                                        */
/*                                                                  */
/* Function	- Return the user-level PC. If in a system call,    */
/*		  return the address of the syscall svc.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

greg_t
getuserpc()
{
	return (lwptoregs(ttolwp(curthread))->r_pc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setregs.                                          */
/*                                                                  */
/* Function	- Clear registers on exec(2).                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
setregs(uarg_t *args)
{
	struct	regs *rp;
	klwp_t	*lwp = ttolwp(curthread);
	proc_t	*p   = ttoproc(curthread);
	pswg_t	*psw;

	/*
	 * Initialize user registers.
	 */
	(void) save_syscall_args();	/* copy args from registers first */
	rp 	  = lwptoregs(lwp);
	psw 	  = (pswg_t *) &rp->r_psw;
	memset(psw, 0, sizeof(pswg_t));
	memset(&rp->r_g0, 0, 15 * sizeof(rp->r_g0));
	memcpy(&rp->r_a0, &args->thrptr, sizeof(args->thrptr));
	curthread->t_post_sys	= 1;
	lwp->lwp_eosys 		= JUSTRETURN;
	lwp->lwp_pcb.pcb_start	= NULL;
	lwp->lwp_pcb.pcb_end  	= NULL;
	lwp->lwp_pcb.pcb_mask	= 0;
	if (p->p_model == DATAMODEL_ILP32) {
		rp->r_sp -= SA(MINFRAME32);
		psw->ea   = 0; 
	} else {
		rp->r_sp -= SA(MINFRAME);
		psw->ea   = 1;
	}

	psw->mask.dat	= 1;
	psw->mask.ext	= 1;
	psw->mask.io 	= 1;
	psw->mc		= 1;
	psw->prob	= 1;
	psw->key	= 0;
	psw->as 	= 3;
	psw->ba 	= 1;
	psw->pc		= args->entry;
	/*
	 * Clear the fixalignment flag
	 */
	p->p_fixalignment = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sendsig.                                          */
/*                                                                  */
/* Function	- Construct the execution environment for the user's*/
/*		  signal handler and arrange for control to be given*/
/*		  to it on return to userland. The library code now */
/*		  calls setcontext() to clean up after the signal   */
/*		  handler, so sigret() is no longer needed.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
sendsig(int sig, k_siginfo_t *sip, void (*hdlr)())
{
	/*
	 * 'volatile' is needed to ensure that values are
	 * correct on the error return from on_fault().
	 */
	volatile int minstacksz; /* min stack required to catch signal */
	int newstack = 0;	/* if true, switching to altstack */
	label_t ljb;
	caddr_t sp;
	struct regs *volatile rp;
	kthread_t *tp = curthread;
	klwp_t *lwp = ttolwp(tp);
	proc_t *volatile p = ttoproc(tp);
	siginfo_t *sip_addr;
	struct sigframe *volatile fp;
	ucontext_t *volatile tuc = NULL;
	volatile int watched = 0;
	caddr_t tos;

	rp = lwptoregs(lwp);

	minstacksz = sizeof (struct sigframe);

	/*
	 * We know that sizeof (siginfo_t) is stack-aligned:
	 * 128 bytes for ILP32, 256 bytes for LP64.
	 */
	if (sip != NULL)
		minstacksz += sizeof (siginfo_t);

	/*
	 * Figure out whether we will be handling this signal on
	 * an alternate stack specified by the user. Then allocate
	 * and validate the stack requirements for the signal handler
	 * context. on_fault will catch any faults.
	 */

	newstack = (sigismember(&PTOU(curproc)->u_sigonstack, sig) &&
	    !(lwp->lwp_sigaltstack.ss_flags & (SS_ONSTACK|SS_DISABLE)));

	tos = (caddr_t)rp->r_sp + STACK_BIAS;
	if (newstack != 0) {
		fp = (struct sigframe *)
		    (SA((uintptr_t)lwp->lwp_sigaltstack.ss_sp) +
			SA((int)lwp->lwp_sigaltstack.ss_size) - STACK_ALIGN -
			SA(minstacksz));
	} else {
		fp = (struct sigframe *)(tos - SA(minstacksz));
		/*
		 * Could call grow here, but stack growth now handled below
		 * in code protected by on_fault().
		 */
	}
	sp = (caddr_t)fp + sizeof (struct sigframe);

	/*
	 * Make sure process hasn't trashed its stack.
	 */
	if (((uintptr_t)fp & (STACK_ALIGN - 1)) != 0 ||
	    (caddr_t)fp >= p->p_usrstack ||
	    (caddr_t)fp + SA(minstacksz) >= p->p_usrstack) {
#ifdef DEBUG
		printf("sendsig: bad signal stack cmd=%s, pid=%d, sig=%d\n",
		    PTOU(p)->u_comm, p->p_pid, sig);
		printf("sigsp = 0x%p, action = 0x%p, upc = 0x%lx\n",
		    (void *)fp, (void *)hdlr, rp->r_pc);

		if (((uintptr_t)fp & (STACK_ALIGN - 1)) != 0)
			printf("bad stack alignment\n");
		else
			printf("fp above USRSTACK\n");
#endif
		return (0);
	}

	watched = watch_disable_addr((caddr_t)fp, SA(minstacksz), S_WRITE);
	if (on_fault(&ljb))
		goto badstack;

	tuc = kmem_alloc(sizeof (ucontext_t), KM_SLEEP);
	savecontext(tuc, lwp->lwp_sigoldmask);
	copyout_noerr(tuc, &fp->uc, sizeof (*tuc));
	kmem_free(tuc, sizeof (*tuc));
	tuc = NULL;

	if (sip != NULL) {
		zoneid_t zoneid;

		uzero(sp, sizeof (siginfo_t));
		if (SI_FROMUSER(sip) &&
		    (zoneid = p->p_zone->zone_id) != GLOBAL_ZONEID &&
		    zoneid != sip->si_zoneid) {
			k_siginfo_t sani_sip = *sip;
			sani_sip.si_pid = p->p_zone->zone_zsched->p_pid;
			sani_sip.si_uid = 0;
			sani_sip.si_ctid = -1;
			sani_sip.si_zoneid = zoneid;
			copyout_noerr(&sani_sip, sp, sizeof (sani_sip));
		} else {
			copyout_noerr(sip, sp, sizeof (*sip));
		}
		sip_addr = (siginfo_t *)sp;
		sp += sizeof (siginfo_t);

		if (sig == SIGPROF &&
		    tp->t_rprof != NULL &&
		    tp->t_rprof->rp_anystate) {
			/*
			 * We stand on our head to deal with
			 * the real time profiling signal.
			 * Fill in the stuff that doesn't fit
			 * in a normal k_siginfo structure.
			 */
			int i = sip->si_nsysarg;
			while (--i >= 0) {
				sulword_noerr(
				    (ulong_t *)&sip_addr->si_sysarg[i],
				    (ulong_t)lwp->lwp_arg[i]);
			}
			copyout_noerr(tp->t_rprof->rp_state,
			    sip_addr->si_mstate,
			    sizeof (tp->t_rprof->rp_state));
		}
	} else {
		sip_addr = (siginfo_t *)NULL;
	}

	lwp->lwp_oldcontext = (uintptr_t)&fp->uc;

	if (newstack != 0) {
		lwp->lwp_sigaltstack.ss_flags |= SS_ONSTACK;

		if (lwp->lwp_ustack) {
			copyout_noerr(&lwp->lwp_sigaltstack,
			    (stack_t *)lwp->lwp_ustack, sizeof (stack_t));
		}
	}

	no_fault();

	/*
	 * Set up user registers for execution of signal handler.
	 */
	rp->r_sp = (uintptr_t)fp - (sizeof(*tuc) + MINFRAME);
	rp->r_pc = (uintptr_t)hdlr;
	rp->r_g2 = sig;
	rp->r_g3 = (uintptr_t)sip_addr;
	rp->r_g4 = (uintptr_t)&fp->uc;

	/*
	 * Don't set lwp_eosys here.  sendsig() is called via psig() after
	 * lwp_eosys is handled, so setting it here would affect the next
	 * system call.
	 */
	return (1);

badstack:
	no_fault();
	if (watched)
		watch_enable_addr((caddr_t)fp, SA(minstacksz), S_WRITE);
	if (tuc)
		kmem_free(tuc, sizeof (ucontext_t));
#ifdef DEBUG
	printf("sendsig: bad signal stack cmd=%s, pid=%d, sig=%d\n",
	    PTOU(p)->u_comm, p->p_pid, sig);
	printf("on fault, sigsp = %p, action = %p, upc = 0x%lx\n",
	    (void *)fp, (void *)hdlr, rp->r_pc);
#endif
	return (0);
}

/*========================= End of Function ========================*/

#ifdef _SYSCALL32_IMPL

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sendsig32.                                        */
/*                                                                  */
/* Function	- Construct the execution environment for the user's*/
/*		  signal handler and arrange for control to be given*/
/*		  to it on return to userland. The library code now */
/*		  calls setcontext() to clean up after the signal   */
/*		  handler, so sigret() is no longer needed.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
sendsig32(int sig, k_siginfo_t *sip, void (*hdlr)())
{
	/*
	 * 'volatile' is needed to ensure that values are
	 * correct on the error return from on_fault().
	 */
	volatile int minstacksz; /* min stack required to catch signal */
	int newstack = 0;	/* if true, switching to altstack */
	label_t ljb;
	caddr_t sp;
	struct regs *volatile rp;
	kthread_t *tp = curthread;
	klwp_t *lwp = ttolwp(tp);
	proc_t *volatile p = ttoproc(tp);
	struct sigframe32 *volatile fp;
	siginfo32_t *sip_addr;
	ucontext32_t *volatile tuc = NULL;
	struct machpcb *mpcb;
	volatile int watched = 0;
	caddr_t tos;

	mpcb = lwptompcb(lwp);
	rp   = lwptoregs(lwp);

	minstacksz = sizeof (struct sigframe32);

	if (sip != NULL)
		minstacksz += sizeof (siginfo32_t);

	/*
	 * Figure out whether we will be handling this signal on
	 * an alternate stack specified by the user. Then allocate
	 * and validate the stack requirements for the signal handler
	 * context. on_fault will catch any faults.
	 */
	newstack = (sigismember(&PTOU(curproc)->u_sigonstack, sig) &&
	    !(lwp->lwp_sigaltstack.ss_flags & (SS_ONSTACK|SS_DISABLE)));

	tos = (void *)(uintptr_t)(uint32_t)rp->r_sp;
	/*
	 * Force proper stack pointer alignment, even in the face of a
	 * misaligned stack pointer from user-level before the signal.
	 * Don't use the SA32() macro because that rounds up, not down.
	 */
	tos = (caddr_t)((uintptr_t)tos & ~(STACK_ALIGN32 - 1ul));

	if (newstack != 0) {
		fp = (struct sigframe32 *)
		    (SA32((uintptr_t)lwp->lwp_sigaltstack.ss_sp) +
			SA32((int)lwp->lwp_sigaltstack.ss_size) -
			STACK_ALIGN32 -
			SA32(minstacksz));
	} else {
		fp = (struct sigframe32 *)(tos - SA32(minstacksz));
		/*
		 * Could call grow here, but stack growth now handled below
		 * in code protected by on_fault().
		 */
	}
	sp = (caddr_t)fp + sizeof (struct sigframe32);

	/*
	 * Make sure process hasn't trashed its stack.
	 */
	if ((caddr_t)fp >= p->p_usrstack ||
	    (caddr_t)fp + SA32(minstacksz) >= p->p_usrstack) {
#ifdef DEBUG
		prom_printf("sendsig32: bad signal stack cmd=%s, pid=%d, sig=%d\n",
		    PTOU(p)->u_comm, p->p_pid, sig);
		prom_printf("sigsp = 0x%p, action = 0x%p, upc = 0x%lx\n",
		    (void *)fp, (void *)hdlr, rp->r_pc);
		prom_printf("fp above USRSTACK32\n");
#endif
		return (0);
	}

	watched = watch_disable_addr((caddr_t)fp, SA32(minstacksz), S_WRITE);
	if (on_fault(&ljb))
		goto badstack;

	tuc = kmem_alloc(sizeof (ucontext32_t), KM_SLEEP);
	savecontext32(tuc, lwp->lwp_sigoldmask);

	copyout_noerr(tuc, &fp->uc, sizeof (*tuc));
	kmem_free(tuc, sizeof (*tuc));
	tuc = NULL;

	if (sip != NULL) {
		siginfo32_t si32;
		zoneid_t zoneid;

		siginfo_kto32(sip, &si32);
		if (SI_FROMUSER(sip) &&
		    (zoneid = p->p_zone->zone_id) != GLOBAL_ZONEID &&
		    zoneid != sip->si_zoneid) {
			si32.si_pid = p->p_zone->zone_zsched->p_pid;
			si32.si_uid = 0;
			si32.si_ctid = -1;
			si32.si_zoneid = zoneid;
		}
		uzero(sp, sizeof (siginfo32_t));
		copyout_noerr(&si32, sp, sizeof (siginfo32_t));
		sip_addr = (siginfo32_t *)sp;
		sp += sizeof (siginfo32_t);

		if (sig == SIGPROF &&
		    tp->t_rprof != NULL &&
		    tp->t_rprof->rp_anystate) {
			/*
			 * We stand on our head to deal with
			 * the real time profiling signal.
			 * Fill in the stuff that doesn't fit
			 * in a normal k_siginfo structure.
			 */
			int i = sip->si_nsysarg;
			while (--i >= 0) {
				suword32_noerr(&sip_addr->si_sysarg[i],
				    (uint32_t)lwp->lwp_arg[i]);
			}
			copyout_noerr(tp->t_rprof->rp_state,
			    sip_addr->si_mstate,
			    sizeof (tp->t_rprof->rp_state));
		}
	} else {
		sip_addr = NULL;
	}

	lwp->lwp_oldcontext = (uintptr_t)&fp->uc;

	if (newstack != 0) {
		lwp->lwp_sigaltstack.ss_flags |= SS_ONSTACK;
		if (lwp->lwp_ustack) {
			stack32_t stk32;

			stk32.ss_sp =
			    (caddr32_t)(uintptr_t)lwp->lwp_sigaltstack.ss_sp;
			stk32.ss_size = (size32_t)lwp->lwp_sigaltstack.ss_size;
			stk32.ss_flags = (int32_t)lwp->lwp_sigaltstack.ss_flags;

			copyout_noerr(&stk32, (stack32_t *)lwp->lwp_ustack,
			    sizeof (stack32_t));
		}
	}

	no_fault();

	if (watched)
		watch_enable_addr((caddr_t)fp, SA32(minstacksz), S_WRITE);

/* S390X FIXME - What about the FP registers & state? */

	/*
	 * Set up user registers for execution of signal handler.
	 */
	rp->r_sp = (uintptr_t)fp - (sizeof(*tuc) + MINFRAME32);
	rp->r_pc = (uintptr_t)hdlr;
	rp->r_g2 = sig;
	rp->r_g3 = (uintptr_t)sip_addr;
	rp->r_g4 = (uintptr_t)&fp->uc;

	/*
	 * Don't set lwp_eosys here.  sendsig() is called via psig() after
	 * lwp_eosys is handled, so setting it here would affect the next
	 * system call.
	 */
	return (1);

badstack:
	no_fault();
	if (watched)
		watch_enable_addr((caddr_t)fp, SA32(minstacksz), S_WRITE);
	if (tuc)
		kmem_free(tuc, sizeof (*tuc));
#ifdef DEBUG
	prom_printf("sendsig32: bad signal stack cmd=%s, pid=%d, sig=%d\n",
	    PTOU(p)->u_comm, p->p_pid, sig);
	prom_printf("on fault, sigsp = 0x%p, action = 0x%p, upc = 0x%lx\n",
	    (void *)fp, (void *)hdlr, rp->r_pc);
#endif
	return (0);
}

/*========================= End of Function ========================*/

#endif /* _SYSCALL32_IMPL */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_load.                                         */
/*                                                                  */
/* Function	- Load registers into lwp.                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED2 */
void
lwp_load(klwp_t *lwp, gregset_t grp, uintptr_t thrptr)
{
	pswg_t *psw;
	struct regs *r;

	psw = (pswg_t *) &lwptoregs(lwp)->r_psw;
	setgregs(lwp, grp);
	r = lwptoregs(lwp);

	memset(psw, 0, sizeof(pswg_t));

	if (lwptoproc(lwp)->p_model == DATAMODEL_ILP32) {
		psw->ea = 0;
		r->r_a0 = r->r_g2;
	} else {
		psw->ea = 1;
		r->r_a0 = (r->r_g2 >> 32);
		r->r_a1 = r->r_g2;
	}
	
	psw->mask.dat	= 1;
	psw->mask.ext	= 1;
	psw->mask.io 	= 1;
	psw->mc		= 1;
	psw->prob	= 1;
	psw->key	= 0x0;
	psw->as 	= 3;
	psw->ba 	= 1;
	psw->pc	      	= r->r_g13;

	lwp->lwp_eosys		= JUSTRETURN;
	lwptot(lwp)->t_post_sys = 1;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_setrval.                                      */
/*                                                                  */
/* Function	- Set syscall()'s return values for a lwp.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lwp_setrval(klwp_t *lwp, int v1, int v2)
{
	struct regs *rp = lwptoregs(lwp);

	rp->r_g0 = 0;
	rp->r_g2 = v1;
	rp->r_g3 = v2;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_setsp.                                        */
/*                                                                  */
/* Function	- Set stack pointer for a lwp.                      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lwp_setsp(klwp_t *lwp, caddr_t sp)
{
	struct regs *rp = lwptoregs(lwp);

	rp->r_sp = (uintptr_t)sp;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lwp_pcb_exit.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
lwp_pcb_exit(void)
{
	klwp_t *lwp = ttolwp(curthread);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sync_icache.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
sync_icache(caddr_t va, uint_t len)
{
	caddr_t end;

	end = va + len;
	va = (caddr_t)((uintptr_t)va & -8l);	/* s390x needs 8-byte align */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_saveregs.                                   */
/*                                                                  */
/* Function	- The panic code records the contents of a regs     */
/*		  structure into panic_data structure for debuggers.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
panic_saveregs(panic_data_t *pdp, struct regs *rp)
{
	panic_nv_t *pnv = PANICNVGET(pdp);

	PANICNVADD(pnv, "g1", rp->r_g1);
	PANICNVADD(pnv, "g2", rp->r_g2);
	PANICNVADD(pnv, "g3", rp->r_g3);
	PANICNVADD(pnv, "g4", rp->r_g4);
	PANICNVADD(pnv, "g5", rp->r_g5);
	PANICNVADD(pnv, "g6", rp->r_g6);
	PANICNVADD(pnv, "g7", rp->r_g7);
	PANICNVADD(pnv, "g8", rp->r_g8);
	PANICNVADD(pnv, "g9", rp->r_g9);
	PANICNVADD(pnv, "g10", rp->r_g10);
	PANICNVADD(pnv, "g11", rp->r_g11);
	PANICNVADD(pnv, "g12", rp->r_g12);
	PANICNVADD(pnv, "g13", rp->r_g13);
	PANICNVADD(pnv, "g14", rp->r_g14);
	PANICNVADD(pnv, "g15", rp->r_g15);
	PANICNVADD(pnv, "pc", (ulong_t)rp->r_pc);

	PANICNVSET(pdp, pnv);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_savetrap.                                   */
/*                                                                  */
/* Function	- The panic code invokes panic_saveregs() to rec-   */
/*		  ord the contents of a regs structure into panic_  */
/*		  data structure for debuggers.		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
panic_savetrap(panic_data_t *pdp, struct panic_trap_info *tip)
{
	panic_saveregs(pdp, tip->trap_regs);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_showtrap.                                   */
/*                                                                  */
/* Function	- The panic code dislpays the trapped registers.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
panic_showtrap(struct panic_trap_info *tip)
{
	showregs(tip->trap_type, tip->trap_regs, tip->trap_addr);
}

/*========================= End of Function ========================*/
