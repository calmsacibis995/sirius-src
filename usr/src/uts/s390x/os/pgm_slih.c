/*------------------------------------------------------------------*/
/* 								    */
/* Name        - pgm_slih.c 					    */
/* 								    */
/* Function    - Program exception handling.                        */
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

#define BREAKPOINT	0x0001		// Breakpoint instruction opcode

#define CR12MASK	0x3ffffffffffff000

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/regset.h>
#include <sys/trap.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/trap.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/tnf.h>
#include <sys/tnf_probe.h>
#include <sys/ftrace.h>
#include <sys/ontrap.h>
#include <sys/kcpc.h>
#include <sys/kobj.h>
#include <sys/procfs.h>
#include <sys/sdt.h>
#include <vm/as.h>

#ifdef  TRAPTRACE
#include <sys/traptrace.h>
#endif


/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern void aio_cleanup();

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static int die(unsigned, mcontext_t *, caddr_t);
static faultcode_t handle_fault(enum seg_rw, mcontext_t *, int, 
				caddr_t, proc_t *, klwp_id_t); 
static void setRetry(struct regs *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

#if defined(DEBUG) || defined(lint)
static int lodebug = 0;
#else
#define	lodebug	0
#endif 

int tudebug = 0;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pgm_slih.                                         */
/*                                                                  */
/* Function	- Process program exceptions.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void 
pgm_slih(mcontext_t *ctx, short code, caddr_t addr)
{
	uint32_t type = 0;
	faultcode_t res = 0;
	int	isKernel;
	uint_t	fault = 0,
		trap  = 0;
	char	fpc[4];
	kthread_t   *thr = curthread;
	proc_t 	    *p   = ttoproc(thr);
	klwp_id_t   lwp  = ttolwp(thr);
	struct 	    machpcb *mpcb = NULL;
	k_siginfo_t siginfo;
	int	addrCode;
	short	opcode;
	_pfxPage *pfx = NULL;
	int	watchpage = 0,
		watchcode,
		ta;
	caddr_t vaddr = (caddr_t) pfx->__lc_per_addr,
		ttrTbl,
		eTraceTbl;
	struct regs *rp = (struct regs *) ctx;
	ctlr12	cr12;

	bzero(&siginfo, sizeof (siginfo));

	/*
	 * Determine if we were in userland during the exception
	 */
	if ((ctx->psw.prob == 1) || (p->p_as != &kas)) {
		isKernel  = 0;
	} else {
		isKernel  = 1;
	}

	/*
	 * Check if we've hit a program recording event
	 */
	if ((code & PXC_PER) != 0) {
//		type |= T_BREAK;
		code ^= PXC_PER;
		trap  = 1;
	}

	addrCode = (int) ((uint64_t) addr & 0xf);
	addr	 = (caddr_t) ((uint64_t) addr & ~0xfff);

	switch(code) {
	
	case PXC_SGT :		// Pagefault
	case PXC_PGT :
	case PXC_RFT :
	case PXC_RST :
	case PXC_RTT :
		res = handle_fault(S_OTHER, ctx, isKernel, addr, p, lwp);
		break;

	case PXC_PRT : 		// Protection exception
		vaddr     = (caddr_t) pfx->__lc_per_addr;
		watchpage = (pr_watch_active(p) && 
			     pr_is_watchpage(vaddr, S_WRITE));
		if (watchpage) {
			if ((watchcode = pr_is_watchpoint(&vaddr, &ta, rp->r_ilc,
		    					  NULL, S_WRITE)) != 0) {
				if (ta) {
					do_watch_step(vaddr, rp->r_ilc, S_WRITE,
						watchcode, ctx->psw.pc);
					fault = F_INVAL;
				} else {
					siginfo.si_signo = SIGTRAP;
					siginfo.si_code  = watchcode;
					siginfo.si_addr  = vaddr;
					siginfo.si_trapafter = 0;
					siginfo.si_pc    = (caddr_t) ctx->psw.pc;
					fault		 = FLTWATCH;
					break;
				}
			}
		}
		res = handle_fault(S_WRITE, ctx, isKernel, addr, p, lwp);
		if (res == 0)
			setRetry(rp);
		break;

	case PXC_ADR :		// Addressing exception
		res = handle_fault(S_READ, ctx, isKernel, addr, p, lwp);
		if (res == 0)
			setRetry(rp);
		break;

	case PXC_OPR :		// Operation exception
		/*
		 * Program check in supervisor state?
		 */
		if ((isKernel) || (ctx->psw.prob == 0))
			die(code, ctx, (caddr_t) ctx->psw.pc);
		
		copyin((caddr_t)(ctx->psw.pc - rp->r_ilc), &opcode, sizeof(opcode));
		if (opcode == BREAKPOINT) {
			siginfo.si_signo = SIGTRAP;
			siginfo.si_code  = TRAP_BRKPT;
			fault		 = FLTBPT;
		} else {
			siginfo.si_signo = SIGILL;
			siginfo.si_code  = ILL_ILLOPN;
			fault		 = FLTILL;
		}
		siginfo.si_addr  = (caddr_t) ctx->psw.pc;
		break;

	case PXC_EXC :		// Execute exception
	case PXC_CRY :
		/*
		 * Program check in supervisor state?
		 */
		if (isKernel)
			die(code, ctx, (caddr_t) ctx->psw.pc);
		siginfo.si_signo = SIGILL;
		siginfo.si_code  = ILL_ILLOPN;
		siginfo.si_addr  = (caddr_t) ctx->psw.pc;
		fault		 = FLTILL;
		break;

	case PXC_PRV :
	case PXC_SPC :
	case PXC_SOP :
		/*
		 * Program check in supervisor state?
		 */
		if (isKernel)
			die(code, ctx, (caddr_t) ctx->psw.pc);
		siginfo.si_signo = SIGILL;
		siginfo.si_code  = ILL_ILLOPN;
		siginfo.si_addr  = (caddr_t) ctx->psw.pc;
		fault		 = FLTILL;
		break;

	case PXC_DTA :		// Decimal or FP data exception
		__asm__ ("	stfpc %0\n"
			 : "=m" (fpc));
		siginfo.si_addr  = (caddr_t) ctx->psw.pc;

		if (fpc[2] == 0) {			// Decimal data exception
			siginfo.si_signo = SIGILL;
			siginfo.si_code  = ILL_ILLOPN;
			fault		 = FLTILL;
		} else if (fpc[2] & DX_IAT) {		// Invalid result
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTRES;
			fault		 = FLTFPE;
		} else if (fpc[2] & DX_UEX) {		// Underflow
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTUND;
			fault		 = FLTFPE;
		} else if (fpc[2] & DX_OVE) {		// Overflow
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTOVF;
			fault		 = FLTFPE;
		} else if (fpc[2] & DX_DBZ) {		// Division by zero
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTDIV;
			fault		 = FLTFPE;
		} else if (fpc[2] & DX_INV) {		// Invalid operation
			siginfo.si_signo = SIGFPE;
			siginfo.si_code  = FPE_FLTINV;
			fault		 = FLTFPE;
		}
		break;

	case PXC_DOV :		// Decimal and IBM FP exceptions
	case PXC_DDV :
	case PXC_HEO :
	case PXC_HEU :
	case PXC_HES :
	case PXC_HFD :
	case PXC_HSR :
		/*
		 * Program check in supervisor state?
		 */
		if (isKernel)
			die(code, ctx, (caddr_t) ctx->psw.pc);
		siginfo.si_signo = SIGFPE;
		siginfo.si_code  = ILL_ILLOPN;
		siginfo.si_addr  = (caddr_t) ctx->psw.pc;
		fault		 = FLTILL;
		break;

	case PXC_FOV :		// Floating point overflow
		siginfo.si_signo = SIGFPE;
		siginfo.si_code  = FPE_INTOVF;
		siginfo.si_addr  = (caddr_t) ctx->psw.pc;
		fault		 = FLTFPE;
		break;

	case PXC_FPD :		// Floating point divide by zero
		siginfo.si_signo = SIGFPE;
		siginfo.si_code  = FPE_INTDIV;
		siginfo.si_addr  = (caddr_t) ctx->psw.pc;
		fault		 = FLTIZDIV;
		break;

	case PXC_TRT :		// Trace Table Overflow
		/*
		 * Is handled within pgm_flih.s
		 */
		return;
		break;

	case PXC_TRS :		// Translation problems
	case PXC_SSE :		// Bizarre problems
	case PXC_OPN :
	case PXC_PCT :
	case PXC_AFX :
	case PXC_ASX :
	case PXC_LXT :
	case PXC_EXT :
	case PXC_PRA :
	case PXC_SCA :
	case PXC_ALT :
	case PXC_ALN :
	case PXC_ALE :
	case PXC_ASV :
	case PXC_ASS :
	case PXC_EXA :
	case PXC_STF :
	case PXC_STE :
	case PXC_STS :
	case PXC_STT :
	case PXC_STO :
	case PXC_ASC :
		die(code, ctx, (caddr_t) ctx->psw.pc);
		break;

	case PXC_MON :		// dtrace event
		break;
	}

	if ((fault == 0) && (trap)) {
		if (pfx->__lc_per_code & _per_evt_fetch) {
			prundostep();
			siginfo.si_signo = SIGTRAP;
			siginfo.si_code  = TRAP_BRKPT;
			fault		 = FLTBPT;
		}
	}

	if (res != 0) {
		siginfo.si_addr = addr;
		if (FC_CODE(res) == FC_OBJERR) {
			siginfo.si_errno = FC_ERRNO(res);
			if (siginfo.si_errno != EINTR) {
				siginfo.si_signo = SIGBUS;
				siginfo.si_code  = BUS_OBJERR;
				fault            = FLTACCESS;
			}
		} else { /* FC_NOMAP || FC_PROT */
			siginfo.si_signo = SIGSEGV;
			siginfo.si_code  = (res == FC_NOMAP) ?
					   SEGV_MAPERR : SEGV_ACCERR;
			fault 		 = FLTBOUNDS;
		}
#if 1
msgnoh("program check at %lx code: %x signo: %x si_code: %x addr: %p thr: %p\n",
ctx->psw.pc, code, siginfo.si_signo, siginfo.si_code, siginfo.si_addr, thr);
#endif
	}

	trap_cleanup((struct regs *) &ctx->gregs, fault, &siginfo);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- handle_fault.                                     */
/*                                                                  */
/* Function	- Handle a potential page fault incident.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static faultcode_t
handle_fault(enum seg_rw type, 
	     mcontext_t *ctx,
	     int isKernel, 
	     caddr_t addr, 
	     proc_t *p, 
	     klwp_id_t lwp) 
{
	int	    mstate;
	uintptr_t   lofault;
	faultcode_t res;
	kthread_t   *thr = curthread;

	lofault		= thr->t_lofault;
	thr->t_lofault	= 0;
	mstate		= new_mstate(thr, LMS_KFAULT);

	res = pagefault(addr, FC_NOMAP, type, isKernel);
	if (!isKernel && res == FC_NOMAP &&
		addr < p->p_usrstack && grow(addr))
		res = 0;

	(void) new_mstate(thr, mstate);

	/*
	 * Restore lofault.  If we resolved the fault, exit.
	 * If we didn't and lofault wasn't set, die.
	 */
	thr->t_lofault = lofault;

	if (res != 0) {
		if (lofault == 0)
			if (isKernel)
				(void) die(type, ctx, addr);
			else	
				return (res);

		/*
		 * Cannot resolve fault.  Return to lofault.
		 */
		if (lodebug) {
			showregs(type, (struct regs *) ctx, addr, 0);
			traceback((caddr_t) ctx->gregs[15]);
		}
		if (FC_CODE(res) == FC_OBJERR)
			res = FC_ERRNO(res);
		else
			res = EFAULT;

		ctx->psw.pc   = (uint64_t) thr->t_lofault;
		ctx->psw.as   = 0;
		ctx->psw.key  = 0;
		ctx->psw.prob = 0;
		ctx->gregs[2] = res;
		res	      = 0;
	}
	return (res);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- die.                                              */
/*                                                                  */
/* Function	- Spring trap and die.                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
die(unsigned type, mcontext_t *ctx, caddr_t addr)
{
	struct panic_trap_info ti;

#ifdef TRAPTRACE
	TRAPTRACE_FREEZE;
#endif

	ti.trap_regs = (struct regs *) ctx;
	ti.trap_type = type;
	ti.trap_addr = addr;

	curthread->t_panic_trap = &ti;

	panic("BAD TRAP: type=%x ctx=%p addr=%p",
	      type, (void *) ctx, (void *)addr);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setRetry.                                         */
/*                                                                  */
/* Function	- If an operation is to be retried then adjust the  */
/*		  PSW according to the ILC.    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
setRetry(struct regs *rp)
{
	int ilc;

	rp->r_pc -= rp->r_ilc;

}

/*========================= End of Function ========================*/
