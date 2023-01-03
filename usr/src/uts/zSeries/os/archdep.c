/*------------------------------------------------------------------*/
/* 								    */
/* Name        - archdep.c  					    */
/* 								    */
/* Function    - Architecturally dependent routines.                */
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

#define BOOT_SUCCESS	0
#define BOOT_FAILURE	-1

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/machparam.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/vmparam.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/proc.h>
#include <sys/ucontext.h>
#include <sys/siginfo.h>
#include <sys/intr.h>
#include <sys/cpuvar.h>
#include <sys/asm_linkage.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/bootconf.h>
#include <sys/archsystm.h>
#include <sys/auxv.h>
#include <sys/debug.h>
#include <sys/elf.h>
#include <sys/elf_SPARC.h>
#include <sys/cmn_err.h>
#include <sys/spl.h>
#include <sys/privregs.h>
#include <sys/kobj.h>
#include <sys/modctl.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <sys/panic.h>
#include <sys/ios390x.h>
#include <vm/seg_kmem.h>
#include <vm/page.h>
#include <vm/mm_s390x.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

//
// Boot proposition structure
//
typedef struct __bootProp {
	const	char *name;	// Name of proposition
	size_t	size;		// Size of proposition
	char	value[65];	// Value of proposition
} bootProp;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern struct memlist ndata;	// memlist of nucleus allocatable memory 

extern void   *bootScratch,	// Scratch area for bop_alloc()
	      *curScrTiny,	// Current small area 
	      *curScrPage,	// Current big area 
	      *bootTinyEnd,	// End of small scratch area
	      *bootPageEnd;	// End of big scratch area

extern int    physMemInit;	// page_t has been initialized

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*
 * The following ELF header fields are defined as processor-specific
 * in the S390X ABI:
 *
 *	e_ident[EI_DATA]	encoding of the processor-specific
 *				data in the object file
 *	e_machine		processor identification
 *	e_flags			processor-specific flags associated
 *				with the file
 */

/*
 * The value of at_flags reflects a platform's cpu module support.
 * at_flags is used to check for allowing a binary to execute and
 * is passed as the value of the AT_FLAGS auxiliary vector.
 */
int at_flags = 0;

uint_t auxv_hwcap_include = 0;	/* patch to enable unrecognized features */
uint_t auxv_hwcap_exclude = 0;	/* patch for broken cpus, debugging */

uint_t cpu_hwcap_flags = 0;	/* set by cpu-dependent code */

static bootProp bp[] = {{"bios-boot-device", 4, 0},
		 {"bootprog",  4, "hmc"},
		 {"boot-args", 65, 0},
		 {"impl-arch-name", 5, "s390x"},
		 {"mfg-name", 5, "s390x"},
		 {"ramdisk_start", 8, 0},
		 {"ramdisk_end", 8, 0},
		 {NULL, 0, 0}};

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getpcstack.                                       */
/*                                                                  */
/* Function	- Get a pc-only stacktrace. Used for kmem_alloc()   */
/*		  buffer ownership tracking.   		 	    */
/*		                               		 	    */
/*		  Returns MIN(current stack depth, pcstack_limit)   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
getpcstack(pc_t *pcstack, int pcstack_limit)
{
	struct frame *fp, *minfp, *stacktop;
	uintptr_t nextfp;
	pc_t nextpc;
	int depth;
	int on_intr;

	if ((on_intr = CPU_ON_INTR(CPU)) != 0)
		stacktop = (struct frame *)(CPU->cpu_intr_stack + SA(MINFRAME));
	else
		stacktop = (struct frame *)curthread->t_stk;

	minfp = (struct frame *)((uintptr_t)getfp());

	fp = (struct frame *)minfp;

	while (depth < pcstack_limit) {
		if (fp <= minfp || fp >= stacktop) {
			if (on_intr) {
				/*
				 * Hop from interrupt stack to thread stack.
				 */
				stacktop = (struct frame *)curthread->t_stk;
				minfp = (struct frame *)curthread->t_stkbase;
				on_intr = 0;
				continue;
			}
			break;
		}

		pcstack[depth++] = nextpc;
		minfp = fp;

		nextpc = (pc_t)fp->fr_savpc;
		fp = (struct frame *)((uintptr_t)fp->fr_bc);
	}

	return (depth);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elfheadcheck.                                     */
/*                                                                  */
/* Function	- Check the processor-specific fields of an ELF     */
/*		  header. Returns 1 if the fields are valid, 0      */
/*		  otherwise.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
elfheadcheck(
	unsigned char e_data,
	Elf32_Half e_machine,
	Elf32_Word e_flags)
{
	return (e_machine == EM_S390);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bind_hwcap.                                       */
/*                                                                  */
/* Function	- Gather information about the processor and place  */
/*		  it into auxv_hwcap so that it can be exported to  */
/*		  the linker via the aux vector.	 	    */
/*		                               		 	    */
/*		  We use this seemingly complicated mechanism so    */
/*		  that we can ensuer that /etc/system can be used   */
/*		  to override what the system can or cannot dis-    */
/*		  over for itself.             		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
bind_hwcap(void)
{
	auxv_hwcap = (auxv_hwcap_include | cpu_hwcap_flags) &
	    ~auxv_hwcap_exclude;

	if (auxv_hwcap_include || auxv_hwcap_exclude)
		cmn_err(CE_CONT, "?user ABI extensions: %b\n",
		    auxv_hwcap, FMT_AV_S390X);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- __ipltospl.                                       */
/*                                                                  */
/* Function	- Return the corresponding spl from ipl.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
__ipltospl(int ipl)
{
	return (ipltospl(ipl));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- traceback.                                        */
/*                                                                  */
/* Function	- Print a stack backtrace using the specified stack */
/*		  pointer. We delay two seconds before continuing,  */
/*		  unless this is the panic traceback. Note that the */
/*		  frame for the starting stack pointer value is     */
/*		  omitted because the corresponding %pc is not      */
/*		  known.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
traceback(caddr_t sp)
{
	struct frame *fp = (struct frame *)(sp + STACK_BIAS);
	struct frame *nextfp, *minfp, *stacktop;
	int on_intr;

	cpu_t *cpu;

	if (!panicstr)
		printf("traceback: %%sp = %p\n", (void *)sp);

	/*
	 * If we are panicking, the high-level interrupt information in
	 * CPU was overwritten.  panic_cpu has the correct values.
	 */
	kpreempt_disable();			/* prevent migration */

	cpu = (panicstr && CPU->cpu_id == panic_cpu.cpu_id)? &panic_cpu : CPU;

	if ((on_intr = CPU_ON_INTR(cpu)) != 0)
		stacktop = (struct frame *)(cpu->cpu_intr_stack + SA(MINFRAME));
	else
		stacktop = (struct frame *)curthread->t_stk;

	kpreempt_enable();

	minfp = fp;

	while ((uintptr_t)fp >= KERNELBASE) {
		uintptr_t pc = (uintptr_t)fp->fr_savpc;
		ulong_t off;
		char *sym;

		nextfp = (struct frame *)((uintptr_t)fp->fr_bc);
		if (nextfp <= minfp || nextfp >= stacktop) {
			if (on_intr) {
				/*
				 * Hop from interrupt stack to thread stack.
				 */
				stacktop = (struct frame *)curthread->t_stk;
				minfp = (struct frame *)curthread->t_stkbase;
				on_intr = 0;
				continue;
			}
			break; /* we're outside of the expected range */
		}

		if ((uintptr_t)nextfp & (STACK_ALIGN - 1)) {
			printf("  >> mis-aligned %%fp = %p\n", (void *)nextfp);
			break;
		}

		if ((sym = kobj_getsymname(pc, &off)) != NULL) {
			printf("%016lx %s:%s+%lx\n",
			    (ulong_t)nextfp,
			    mod_containing_pc((caddr_t)pc), sym, off);
		} else {
			printf("%016lx %p\n",
			    (ulong_t)nextfp, (void *)pc);
		}

		printf("  %%r2-5:   %016lx %016lx %016lx %016lx\n"
		       "  %%r6-9:   %016lx %016lx %016lx %016lx\n"
		       "  %%r10-13: %016lx %016lx %016lx %016lx\n"
		       "  %%r14-15: %016lx %016lx\n",
		    nextfp->fr_argd[0], nextfp->fr_argd[1],
		    nextfp->fr_argd[2], nextfp->fr_argd[3],
		    nextfp->fr_regs[0], nextfp->fr_regs[1],
		    nextfp->fr_regs[2], nextfp->fr_regs[3],
		    nextfp->fr_regs[4], nextfp->fr_regs[5],
		    nextfp->fr_regs[6], nextfp->fr_regs[7],
		    nextfp->fr_regs[8], nextfp->fr_regs[9]);


		fp = nextfp;
		minfp = fp;
	}

	if (!panicstr) {
		printf("end of traceback\n");
		DELAY(2 * MICROSEC);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- traceregs.                                        */
/*                                                                  */
/* Function	- Generate a stack backtrace from a saved register  */
/*		  register set.                		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
traceregs(struct regs *rp)
{
	traceback((caddr_t)rp->r_sp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- exec_set_sp.                                      */
/*                                                                  */
/* Function	- Set the stack pointer.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
exec_set_sp(size_t stksize)
{
	klwp_t *lwp = ttolwp(curthread);

	stksize += STACK_BIAS;
	lwptoregs(lwp)->r_sp = (uintptr_t)curproc->p_usrstack - stksize;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- boot_virt_alloc.                                  */
/*                                                                  */
/* Function	- Allocate a region of virtual address space,       */
/*		  unmapped.                    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
boot_virt_alloc(caddr_t addr, size_t size)
{
	return (addr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bop_alloc.                                        */
/*                                                                  */
/* Function	- Get the IPL support routines to give us some      */
/*		  storage.                     		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
bop_alloc(struct bootops *bop, caddr_t addr, size_t size, int align)
{
	caddr_t vAddr,
		nAddr,
		eAddr;

	pfn_t	pfnum;

	page_t	*pp;

	nAddr = vAddr = bop->bsys_alloc(addr, size, align);
#if 0
	if ((vAddr != NULL) && (physMemInit)) {
		for (eAddr = vAddr + size; 
		     nAddr < eAddr; 
		     nAddr += PAGESIZE) {

			pfnum = va_to_pfn(nAddr);
			pp    = page_numtopp_nolock(pfnum);
			if (pp == NULL) 
				panic("bop_alloc: pp is NULL!");

			while (!page_lock(pp, SE_EXCL, (kmutex_t *)NULL, P_RECLAIM));
			PP_CLRFREE(pp);
			page_hashin(pp, &kvp, (u_offset_t)(uintptr_t) nAddr, NULL);
			page_unlock(pp);
		}
	}
#endif

	return (vAddr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bop_allreal.                                      */
/*                                                                  */
/* Function	- Get the IPL support routines to give us some      */
/*		  real storage.                     		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
bop_allreal(struct bootops *bop, size_t size, int align)
{
	caddr_t rAddr,
		nAddr,
		eAddr;

	pfn_t	pfnum;

	page_t	*pp;

	nAddr = rAddr = bop->bsys_allreal(size, align);
	if ((rAddr != NULL) && (physMemInit)) {
		for (eAddr = nAddr + size; 
		     nAddr < eAddr; 
		     nAddr += MMU_PAGESIZE) {

			pfnum = ((u_longlong_t) nAddr >> MMU_PAGESHIFT);
			pp    = page_numtopp(pfnum, SE_EXCL);
			if (pp != NULL) {
				if (PP_ISFREE(pp))
					page_list_sub(pp, PG_FREE_LIST);
				PP_CLRFREE(pp);
				page_unlock(pp);
			}
		}
	}

	return (rAddr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xcopyin_nta.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
xcopyin_nta(const void *uaddr, void *kaddr, size_t count, int dummy)
{
	return (xcopyin(uaddr, kaddr, count));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xcopyout_nta.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
xcopyout_nta(const void *kaddr, void *uaddr, size_t count, int dummy)
{
	return (xcopyout(kaddr, uaddr, count));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kcopy_nta.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
kcopy_nta(const void *from, void *to, size_t count, int dummy)
{
	return (kcopy(from, to, count));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ucontext_32ton.                                   */
/*                                                                  */
/* Function	- Make a ucontext from a ucontext32.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ucontext_32ton(const ucontext32_t *src, ucontext_t *dest)
{
	int i;

	bzero(dest, sizeof (*dest));

	dest->uc_flags = src->uc_flags;
	dest->uc_link  = (ucontext_t *)(uintptr_t)src->uc_link;
	memcpy(&dest->uc_mcontext.psw, &src->uc_mcontext.psw, sizeof(pswg_t));

	for (i = 0; i < 4; i++) {
		dest->uc_sigmask.__sigbits[i] = src->uc_sigmask.__sigbits[i];
	}

	dest->uc_stack.ss_sp    = (void *)(uintptr_t)src->uc_stack.ss_sp;
	dest->uc_stack.ss_size  = (size_t)src->uc_stack.ss_size;
	dest->uc_stack.ss_flags = src->uc_stack.ss_flags;

	for (i = 0; i < _NGREG; i++) {
		dest->uc_mcontext.gregs[i] =
		    (greg_t)(uint32_t)src->uc_mcontext.gregs[i];
	}

	for (i = 0; i < _NAREG; i++) {
		dest->uc_mcontext.aregs[i] =
		    (greg_t)(uint32_t)src->uc_mcontext.aregs[i];
		dest->uc_mcontext.fpregs.fr.fd[i] =
		    (double)src->uc_mcontext.fpregs.fr.fd[i];
	}
	dest->uc_mcontext.fpregs.fpc = src->uc_mcontext.fpregs.fpc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag_24.                                          */
/*                                                                  */
/* Function	- Device type and features.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
diag_24(int devno, uint32_t *vdevinfo, uint32_t *rdevinfo)
{
	int cc;
	int vi;
	int ri;

	__asm__ ("	lr	1,%3\n"
		 "	sam31\n"
		 "	diag	1,2,0x24\n"
		 "	sam64\n"
		 "	lgr	%0,1\n"
		 "	lgr	%1,2\n"
		 "	lgr	%2,3\n"
		 : "=r" (cc), "=r" (vi), "=r" (ri)
		 : "r" (devno)
		 : "1", "2", "3", "memory", "cc");

	if (vdevinfo != NULL) {
		*vdevinfo = vi;
	}

	if (rdevinfo != NULL) {
		*rdevinfo = ri;
	}

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag_a8.                                          */
/*                                                                  */
/* Function	- Synchronous I/O				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
diag_a8(void *sgio, int *retcode)
{
	int cc;
	int rc;

	__asm__ ("	lgr	2,15\n"
		 "	sam31\n"
		 "	diag	%2,0,0xa8\n"
		 "	sam64\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 "	lgr	%1,15\n"
		 "	lgr	15,2\n"
		 : "=r" (cc), "=r" (rc)
		 : "r" (va_to_pa(sgio))
		 : "2", "15", "cc");

	if (retcode != NULL) {
		*retcode = rc;
	}

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag_210.                                         */
/*                                                                  */
/* Function	- Retrieve device information.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
diag_210(void *rdc)
{
	int cc;

	__asm__ ("	sam31\n"
		 "	diag	%1,0,0x210\n"
		 "	sam64\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg    %0,%0,28\n"
		 : "=r" (cc)
		 : "r" (va_to_pa(rdc))
		 : "memory", "cc");

	return cc;
}

/*========================= End of Function ========================*/
