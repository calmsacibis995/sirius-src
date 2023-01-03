/*------------------------------------------------------------------*/
/* 								    */
/* Name        - prmachdep.c					    */
/* 								    */
/* Function    - proc file system support routines.                 */
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
 * modify the lower 32bits of a uint64_t
 */
#define	SET_LOWER_32(all, lower)	\
	(((uint64_t)(all) & 0xffffffff00000000) | (uint32_t)(lower))

/* conversion from 64-bit register to 32-bit register */
#define	R32(r)	(prgreg32_t)(uint32_t)(r)

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/t_lock.h>
#include <sys/param.h>
#include <sys/cred.h>
#include <sys/debug.h>
#include <sys/inline.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/vmsystm.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/pcb.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/cpuvar.h>
#include <sys/copyops.h>
#include <sys/regset.h>
#include <sys/watchpoint.h>

#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procfs.h>
#include <sys/archsystm.h>
#include <sys/cmn_err.h>
#include <sys/stack.h>
#include <sys/machpcb.h>

#include <sys/vmem.h>
#include <sys/mman.h>
#include <sys/vmparam.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_kmem.h>
#include <vm/seg_kp.h>
#include <vm/page.h>

#include <fs/proc/prdata.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern void ppmapout(caddr_t);

#ifdef _SYSCALL32_IMPL
extern void getgregs32(klwp_t *, gregset32_t);
#endif

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void prsettrap(struct as *, caddr_t, caddr_t, klwp_t *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

int	prnwatch = 10000;	/* maximum number of watched areas */

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prpokethread.                                     */
/*                                                                  */
/* Function	- Force a thread into the kernel if it is not       */
/*		  already there. This is a no-op on uniprocessors.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
prpokethread(kthread_t *t)
{
	if (t->t_state == TS_ONPROC && t->t_cpu != CPU)
		poke_cpu(t->t_cpu->cpu_id);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetprregs.                                      */
/*                                                                  */
/* Function	- Return general registers.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prgetprregs(klwp_t *lwp, prgregset_t prp)
{
	gregset_t gr;
	aregset_t ar;

	ASSERT(MUTEX_NOT_HELD(&lwptoproc(lwp)->p_lock));

	getgregs(lwp, gr);
	getaregs(lwp, ar);
	bzero(prp, sizeof (prp));

	/*
	 * Can't copy since prgregset_t and gregset_t
	 * use different defines.
	 */
	getpsw(lwp, (pswg_t *) &prp[R_PSWM]);
//	prp[R_PC]  = prp[R_PC] - gr[REG_ILC];
	prp[R_PC]  = prp[R_PC];
	prp[R_G0]  = gr[REG_G0];
	prp[R_G1]  = gr[REG_G1];
	prp[R_G2]  = gr[REG_G2];
	prp[R_G3]  = gr[REG_G3];
	prp[R_G4]  = gr[REG_G4];
	prp[R_G5]  = gr[REG_G5];
	prp[R_G6]  = gr[REG_G6];
	prp[R_G7]  = gr[REG_G7];
	prp[R_G8]  = gr[REG_G8];
	prp[R_G9]  = gr[REG_G9];
	prp[R_G10] = gr[REG_G10];
	prp[R_G11] = gr[REG_G11];
	prp[R_G12] = gr[REG_G12];
	prp[R_G13] = gr[REG_G13];
	prp[R_G14] = gr[REG_G14];
	prp[R_G15] = gr[REG_G15];
	prp[R_A0]  = ar[REG_A0];
	prp[R_A1]  = ar[REG_A1];
	prp[R_A2]  = ar[REG_A2];
	prp[R_A3]  = ar[REG_A3];
	prp[R_A4]  = ar[REG_A4];
	prp[R_A5]  = ar[REG_A5];
	prp[R_A6]  = ar[REG_A6];
	prp[R_A7]  = ar[REG_A7];
	prp[R_A8]  = ar[REG_A8];
	prp[R_A9]  = ar[REG_A9];
	prp[R_A10] = ar[REG_A10];
	prp[R_A11] = ar[REG_A11];
	prp[R_A12] = ar[REG_A12];
	prp[R_A13] = ar[REG_A13];
	prp[R_A14] = ar[REG_A14];
	prp[R_A15] = ar[REG_A15];

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prsetprregs.                                      */
/*                                                                  */
/* Function	- Set general registers.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prsetprregs(klwp_t *lwp, prgregset_t prp, int initial)
{
	gregset_t gr;
	aregset_t ar;

	getaregs(lwp, ar);
	setpc(lwp, prp[R_PC]);
	gr[REG_G0]  = prp[R_G0];
	gr[REG_G1]  = prp[R_G1];
	gr[REG_G2]  = prp[R_G2];
	gr[REG_G3]  = prp[R_G3];
	gr[REG_G4]  = prp[R_G4];
	gr[REG_G5]  = prp[R_G5];
	gr[REG_G6]  = prp[R_G6];
	gr[REG_G7]  = prp[R_G7];
	gr[REG_G8]  = prp[R_G8];
	gr[REG_G9]  = prp[R_G9];
	gr[REG_G10] = prp[R_G10];
	gr[REG_G11] = prp[R_G11];
	gr[REG_G12] = prp[R_G12];
	gr[REG_G13] = prp[R_G13];
	gr[REG_G14] = prp[R_G14];
	gr[REG_G15] = prp[R_G15];
	ar[REG_A0]  = prp[R_A0];
	ar[REG_A1]  = prp[R_A1];

	/*
	 * setgregs will only allow the condition codes to be set.
	 */

	setgregs(lwp, gr);
	setaregs(lwp, ar);
}

/*========================= End of Function ========================*/

#ifdef _SYSCALL32_IMPL

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgregset_32ton.                                  */
/*                                                                  */
/* Function	- Convert prgregset32 to native prgregset.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prgregset_32ton(klwp_t *lwp, prgregset32_t src, prgregset_t dest)
{
	struct regs *r = lwptoregs(lwp);

	dest[R_G0]  = SET_LOWER_32(r->r_g0, src[R_G0]);
	dest[R_G1]  = SET_LOWER_32(r->r_g1, src[R_G1]);
	dest[R_G2]  = SET_LOWER_32(r->r_g2, src[R_G2]);
	dest[R_G3]  = SET_LOWER_32(r->r_g3, src[R_G3]);
	dest[R_G4]  = SET_LOWER_32(r->r_g4, src[R_G4]);
	dest[R_G5]  = SET_LOWER_32(r->r_g5, src[R_G5]);
	dest[R_G6]  = SET_LOWER_32(r->r_g6, src[R_G6]);
	dest[R_G7]  = SET_LOWER_32(r->r_g7, src[R_G7]);
	dest[R_G8]  = SET_LOWER_32(r->r_g8, src[R_G8]);
	dest[R_G9]  = SET_LOWER_32(r->r_g9, src[R_G9]);
	dest[R_G10] = SET_LOWER_32(r->r_g10, src[R_G10]);
	dest[R_G11] = SET_LOWER_32(r->r_g11, src[R_G11]);
	dest[R_G12] = SET_LOWER_32(r->r_g12, src[R_G12]);
	dest[R_G13] = SET_LOWER_32(r->r_g13, src[R_G13]);
	dest[R_G14] = SET_LOWER_32(r->r_g14, src[R_G14]);
	dest[R_G15] = SET_LOWER_32(r->r_g15, src[R_G15]);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetprregs32.                                    */
/*                                                                  */
/* Function	- Return 32-bit general registers.                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prgetprregs32(klwp_t *lwp, prgregset32_t prp)
{
	gregset32_t gr;
	aregset_t   ar;

	ASSERT(MUTEX_NOT_HELD(&lwptoproc(lwp)->p_lock));

	getgregs32(lwp, gr);
	getaregs(lwp, ar);
	bzero(prp, sizeof (prp));

	/*
	 * Can't copy since prgregset_t and gregset_t
	 * use different defines.
	 */
	getpsw(lwp, (pswg_t *) &prp[R_PSWM]);
//	prp[R_PC]  = prp[R_PC] - gr[REG_ILC];
	prp[R_PC]  = prp[R_PC];
	prp[R_G0]  = gr[REG_G0];
	prp[R_G1]  = gr[REG_G1];
	prp[R_G2]  = gr[REG_G2];
	prp[R_G3]  = gr[REG_G3];
	prp[R_G4]  = gr[REG_G4];
	prp[R_G5]  = gr[REG_G5];
	prp[R_G6]  = gr[REG_G6];
	prp[R_G7]  = gr[REG_G7];
	prp[R_G8]  = gr[REG_G8];
	prp[R_G9]  = gr[REG_G9];
	prp[R_G10] = gr[REG_G10];
	prp[R_G11] = gr[REG_G11];
	prp[R_G12] = gr[REG_G12];
	prp[R_G13] = gr[REG_G13];
	prp[R_G14] = gr[REG_G14];
	prp[R_G15] = gr[REG_G15];
	prp[R_A0]  = ar[REG_A0];
	prp[R_A1]  = ar[REG_A1];
	prp[R_A2]  = ar[REG_A2];
	prp[R_A3]  = ar[REG_A3];
	prp[R_A4]  = ar[REG_A4];
	prp[R_A5]  = ar[REG_A5];
	prp[R_A6]  = ar[REG_A6];
	prp[R_A7]  = ar[REG_A7];
	prp[R_A8]  = ar[REG_A8];
	prp[R_A9]  = ar[REG_A9];
	prp[R_A10] = ar[REG_A10];
	prp[R_A11] = ar[REG_A11];
	prp[R_A12] = ar[REG_A12];
	prp[R_A13] = ar[REG_A13];
	prp[R_A14] = ar[REG_A14];
	prp[R_A15] = ar[REG_A15];

}

/*========================= End of Function ========================*/

#endif	/* _SYSCALL32_IMPL */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetrvals.                                       */
/*                                                                  */
/* Function	- Get the syscall return values for the lwp.        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prgetrvals(klwp_t *lwp, long *rval1, long *rval2)
{
	struct regs *r = lwptoregs(lwp);

	if (r->r_g0 != 0)
		return ((int)r->r_g2);
	if (lwp->lwp_eosys == JUSTRETURN) {
		*rval1 = 0;
		*rval2 = 0;
		return (0);
	} else if (lwptoproc(lwp)->p_model == DATAMODEL_ILP32) {
		*rval1 = r->r_g2 & (uint32_t)0xffffffffU;
		*rval2 = r->r_g3 & (uint32_t)0xffffffffU;
	} else {
		*rval1 = r->r_g2;
		*rval2 = r->r_g3;
	}
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prhasfp.                                          */
/*                                                                  */
/* Function	- Tell caller that we support floating-point.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prhasfp(void)
{
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetprfpregs.                                    */
/*                                                                  */
/* Function	- Get floating-point registers.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prgetprfpregs(klwp_t *lwp, prfpregset_t *pfp)
{
	bzero(pfp, sizeof (*pfp));
	/*
	 * This works only because prfpregset_t is intentionally
	 * constructed to be identical to fpregset_t, with additional
	 * space for the floating-point queue at the end.
	 */
	getfpregs(lwp, (fpregset_t *)pfp);
}

/*========================= End of Function ========================*/

#ifdef	_SYSCALL32_IMPL

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetprfpregs32.                                  */
/*                                                                  */
/* Function	- Get floating-point registers.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prgetprfpregs32(klwp_t *lwp, prfpregset_t *pfp)
{
	bzero(pfp, sizeof (*pfp));
	/*
	 * This works only because prfpregset_t is intentionally
	 * constructed to be identical to fpregset_t.
	 */
	getfpregs(lwp, (fpregset_t *)pfp);
}

/*========================= End of Function ========================*/

#endif	/* _SYSCALL32_IMPL */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prsetprfpregs.                                    */
/*                                                                  */
/* Function	- Set floating-point registers.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prsetprfpregs(klwp_t *lwp, prfpregset_t *pfp)
{
	/*
	 * This works only because prfpregset_t is intentionally
	 * constructed to be identical to fpregset_t, with additional
	 * space for the floating-point queue at the end.
	 */
	setfpregs(lwp, (fpregset_t *)pfp);
}

/*========================= End of Function ========================*/

#ifdef	_SYSCALL32_IMPL

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prsetprfpregs32.                                  */
/*                                                                  */
/* Function	- Set floating-point registers.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prsetprfpregs32(klwp_t *lwp, prfpregset32_t *pfp)
{
	/*
	 * This works only because prfpregset32_t is intentionally
	 * constructed to be identical to fpregset32_t, with additional
	 * space for the floating-point queue at the end.
	 */
	setfpregs(lwp, (fpregset_t *) pfp);
}

/*========================= End of Function ========================*/

#endif	/* _SYSCALL32_IMPL */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prhasx.                                           */
/*                                                                  */
/* Function	- Tells the caller we have no extra registers.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
int
prhasx(proc_t *p)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetprxregsize.                                  */
/*                                                                  */
/* Function	- Get the size of the extra registers - 0 on s390x. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prgetprxregsize(proc_t *p)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetprxregs.                                     */
/*                                                                  */
/* Function	- Get extra registers.                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
prgetprxregs(klwp_t *lwp, caddr_t prx)
{
	/* no extra registers */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prsetprxregs.                                     */
/*                                                                  */
/* Function	- Set extra registers.                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
prsetprxregs(klwp_t *lwp, caddr_t prx)
{
	/* no extra registers */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetstackbase.                                   */
/*                                                                  */
/* Function	- Return the lower limit of the process stack.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
prgetstackbase(proc_t *p)
{
	return (p->p_usrstack - p->p_stksize);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prgetpsaddr.                                      */
/*                                                                  */
/* Function	- Return the "addr" field for pr_addr in prpsinfo_t.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
prgetpsaddr(proc_t *p)
{
	return ((caddr_t)p);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prvsaddr.                                         */
/*                                                                  */
/* Function	- Set the PC to the specified virtual address.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prsvaddr(klwp_t *lwp, caddr_t vaddr)
{
	struct regs *r = lwptoregs(lwp);

	ASSERT(MUTEX_NOT_HELD(&lwptoproc(lwp)->p_lock));

	/*
	 * pc must be halfword aligned on s390x.
	 * We silently make it so to avoid a watchdog reset.
	 */
	r->r_pc = (uintptr_t)vaddr & ~01L;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prmapin.                                          */
/*                                                                  */
/* Function	- Map address "addr" in address space "as" into a   */
/*		  kernel virtual address. The memory is guaranteed  */
/*		  to be resident and locked down. 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
prmapin(struct as *as, caddr_t addr, int writing)
{
	page_t *pp;
	caddr_t kaddr;
	pfn_t pfnum;

	/*
	 * XXX - Because of past mistakes, we have bits being returned
	 * by getpfnum that are actually the page type bits of the pte.
	 * When the object we are trying to map is a memory page with
	 * a page structure everything is ok and we can use the optimal
	 * method, ppmapin.  Otherwise, we have to do something special.
	 */
	pfnum = hat_getpfnum(as->a_hat, addr);
	if (pf_is_memory(pfnum)) {
		pp = page_numtopp_nolock(pfnum);
		if (pp != NULL) {
			ASSERT(PAGE_LOCKED(pp));
			kaddr = ppmapin(pp, writing ?
				(PROT_READ | PROT_WRITE) : PROT_READ,
				(caddr_t)-1);
			return (kaddr + ((uintptr_t)addr & PAGEOFFSET));
		}
	}

	/*
	 * Oh well, we didn't have a page struct for the object we were
	 * trying to map in; ppmapin doesn't handle devices, but allocating a
	 * heap address allows ppmapout to free virutal space when done.
	 */
	kaddr = vmem_alloc(heap_arena, PAGESIZE, VM_SLEEP);

	hat_devload(kas.a_hat, kaddr, PAGESIZE, pfnum,
		writing ? (PROT_READ | PROT_WRITE) : PROT_READ, HAT_LOAD_LOCK);

	return (kaddr + ((uintptr_t)addr & PAGEOFFSET));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prmapout.                                         */
/*                                                                  */
/* Function	- Unmap address "addr" in address space "as".       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
prmapout(struct as *as, caddr_t addr, caddr_t vaddr, int writing)
{
	vaddr = (caddr_t)((uintptr_t)vaddr & PAGEMASK);
	ppmapout(vaddr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prstep.                                           */
/*                                                                  */
/* Function	- Arrange to single-step the lwp.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prstep(klwp_t *lwp, int watchstep)
{
	kthread_t	*t = lwptot(lwp);

	ASSERT(MUTEX_NOT_HELD(&lwptoproc(lwp)->p_lock));

	/*
	 * flag LWP so that its r_efl trace bit (PS_T) will be set on
	 * next return to usermode.
	 */
	lwp->lwp_pcb.pcb_step   = STEP_REQUESTED;

	if (watchstep)
		lwp->lwp_pcb.pcb_flags |= WATCH_STEP;
	else
		lwp->lwp_pcb.pcb_flags |= NORMAL_STEP;

	aston(t);	/* let trap() set PS_T in rp->r_efl */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prnostep.                                         */
/*                                                                  */
/* Function	- Undo prstep().				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prnostep(klwp_t *lwp)
{
	kthread_t	*t = lwptot(lwp);

	ASSERT(ttolwp(curthread) == lwp ||
	    MUTEX_NOT_HELD(&lwptoproc(lwp)->p_lock));

	/*
	 * flag LWP so that its r_efl trace bit (PS_T) will be cleared on
	 * next return to usermode.
	 */
	lwp->lwp_pcb.pcb_step   = STEP_NONE;
	lwp->lwp_pcb.pcb_flags &= ~(NORMAL_STEP|WATCH_STEP|DEBUG_PENDING);

	aston(t);	/* let trap() clear PS_T in rp->r_efl */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prdostep.                                         */
/*                                                                  */
/* Function	- Prepare to single-step the lwp if requested. This */
/*		  is called by the lwp itself just before returning */
/*		  to user level.               		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prdostep(void)
{
	klwp_t *lwp	= ttolwp(curthread);
	struct regs *r	= lwptoregs(lwp);
	proc_t *p	= lwptoproc(lwp);
	struct as *as	= p->p_as;
	caddr_t pc;

	ASSERT(lwp != NULL);
	ASSERT(r != NULL);

	if (lwp->lwp_pcb.pcb_step == STEP_ACTIVE)
		return;

	if (p->p_model == DATAMODEL_ILP32) {
		pc = (caddr_t)(uintptr_t)(caddr32_t)r->r_pc;
	} else {
		pc = (caddr_t)r->r_pc;
	}

	/*
	 * Single-stepping on s390x is effected by setting
	 * the PER start and end registers to the instruction to 
	 * be stepped. We do not ask for instruction nullification
	 * as we want the operation to be executed. 
	 *
	 */
	lwp->lwp_pcb.pcb_tracepc = (void *) pc;
	lwp->lwp_pcb.pcb_step 	 = STEP_ACTIVE;
	lwp->lwp_pcb.pcb_mask	|= EVM_FETCH;
	prsettrap(as, pc, pc, lwp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prundostep.                                       */
/*                                                                  */
/* Function	- Undo the work done to perform single-stepping.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prundostep(void)
{
	klwp_t *lwp	= ttolwp(curthread);
	struct regs *r	= lwptoregs(lwp);
	proc_t *p	= lwptoproc(lwp);
	struct as *as	= p->p_as;
	caddr_t pc;

	ASSERT(lwp != NULL);
	ASSERT(r != NULL);

	lwp->lwp_pcb.pcb_tracepc = NULL;
	lwp->lwp_pcb.pcb_step 	 = 0;
	lwp->lwp_pcb.pcb_mask	&= ~EVM_FETCH;
	prsettrap(as, lwp->lwp_pcb.pcb_start, lwp->lwp_pcb.pcb_end, lwp);

	return(1);
}

/*========================= End of Function ========================*/


/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prisstep.                                         */
/*                                                                  */
/* Function	- Tell caller if single-step is in effect.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prisstep(klwp_t *lwp)
{
	ASSERT(MUTEX_NOT_HELD(&lwptoproc(lwp)->p_lock));

	return ((lwp->lwp_pcb.pcb_flags &
		(NORMAL_STEP|WATCH_STEP|DEBUG_PENDING)) != 0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prstop.                                           */
/*                                                                  */
/* Function	- Make sure the lwp is in an orderly state for 	    */
/*		  inspection by a debugger through /proc. Called    */
/*		  from stop() and from syslwp_create().		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
prstop(int why, int what)
{
	klwp_t *lwp = ttolwp(curthread);
	struct regs *r = lwptoregs(lwp);

	/*
	 * Make sure we don't deadlock on a recursive call
	 * to prstop().  stop() tests the lwp_nostop flag.
	 */
	ASSERT(lwp->lwp_nostop == 0);
	lwp->lwp_nostop = 1;

	if (copyin_nowatch((caddr_t)r->r_pc, &lwp->lwp_pcb.pcb_instr,
		    sizeof (lwp->lwp_pcb.pcb_instr)) == 0)
		lwp->lwp_pcb.pcb_flags |= INSTR_VALID;
	else {
		lwp->lwp_pcb.pcb_flags &= ~INSTR_VALID;
		lwp->lwp_pcb.pcb_instr = 0;
	}

	(void) save_syscall_args();
	ASSERT(lwp->lwp_nostop == 1);
	lwp->lwp_nostop = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prfetchinstr.                                     */
/*                                                                  */
/* Function	- Fetch the user-level instruction on which the lwp */
/*		  is stopped. It was saved by the lwp itself, in    */
/*		  prstop(). Returns non-zero if the instruction is  */
/*		  valid.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prfetchinstr(klwp_t *lwp, ulong_t *ip)
{
	*ip = (ulong_t)(instr_t)lwp->lwp_pcb.pcb_instr;
	return (lwp->lwp_pcb.pcb_flags & INSTR_VALID);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- pr_watch_emul.                                    */
/*                                                                  */
/* Function	- Called from trap() when a load or store instruc-  */
/*		  tion falls in a watched page but is not a watch-  */
/*		  point. We emulate the instruction in the kernel.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
pr_watch_emul(struct regs *rp, caddr_t addr, enum seg_rw rw)
{
#if 0
	char *badaddr = (caddr_t)(-1);
	int res;
	int watched;

	/* prevent recursive calls to pr_watch_emul() */
	ASSERT(!(curthread->t_flag & T_WATCHPT));
	curthread->t_flag |= T_WATCHPT;

	watched = watch_disable_addr(addr, 16, rw);
/* FIXME - need to simulate an instruction */
//	res = do_unaligned(rp, &badaddr);
	if (watched)
		watch_enable_addr(addr, 16, rw);

	curthread->t_flag &= ~T_WATCHPT;
	if (res == SIMU_SUCCESS) {
		return (1);
	}
#endif
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prsettrap.                                        */
/*                                                                  */
/* Function	- Set the PER registers, the mask, and the address  */
/*		  space control element.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
prsettrap(struct as *as, caddr_t start, caddr_t end, klwp_t *lwp)
{
	struct regs *rp = lwptoregs(lwp);
	pswg_t *psw = &rp->r_psw;

	if (lwp->lwp_pcb.pcb_mask != 0) {
		psw->mask.per	 = 1;
		rp->r_c9	 = lwp->lwp_pcb.pcb_mask;
		rp->r_c10	 = (uint64_t) start;
		rp->r_c11	 = (uint64_t) end;
		rp->r_c7  	|= ASCE_SAE;
		rp->r_c13 	|= ASCE_SAE;
	} else {
		psw->mask.per	 = 0;
		rp->r_c9	 = 0;
		rp->r_c10	 = 0;
		rp->r_c11	 = 0;
		rp->r_c7	&= ~ASCE_SAE;
		rp->r_c13	&= ~ASCE_SAE;
	}

	__asm__ ("	lctlg	9,11,%0\n"
		 "	lctlg	7,7,%1\n"
		 "	lctlg	13,13,%2\n"
		 : : "Q" (rp->r_c9), "Q" (rp->r_c7), "Q" (rp->r_c13));
}

/*========================= End of Function ========================*/
