/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mp_call.c  					    */
/* 								    */
/* Function    - Facilities for cross-processor subroutine calls    */
/*		 using "mailbox" interrupts.			    */
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

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/avintr.h>
#include <sys/thread.h>
#include <sys/kmem.h>
#include <sys/cpu.h>
#include <sys/cpupart.h>
#include <sys/exts390x.h>
#include <sys/machs390x.h>
#include <sys/smp.h>
#include <sys/x_call.h>
#include <sys/smp_impldefs.h>
#include <sys/systm.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern void *e_cpu;
extern struct cpu cpu0;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void smp_setPrefix(struct cpu *);
static uint_t handle_ext_call(caddr_t arg1, caddr_t arg2);
static uint_t handle_emg_sig(caddr_t arg1, caddr_t arg2);
static uint_t handle_mal_alt(caddr_t arg1, caddr_t arg2);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static ddi_softint_hdl_impl_t xcall_hdl =
	{0, NULL, NULL, NULL, 0, NULL, NULL, NULL};

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- smp_init.                                         */
/*                                                                  */
/* Function	- Establish environment for SMP operation.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
smp_init()
{
	add_avintr(NULL, 1, handle_ext_call, "xcall hi",
		   S390_INTR_EXT, 0, 0, NULL, NULL);
	add_avintr(NULL, 1, handle_emg_sig, "emg sig",
		   S390_INTR_EXT, 0, 0, NULL, NULL);
	add_avintr(NULL, 1, handle_mal_alt, "mal alt",
		   S390_INTR_EXT, 0, 0, NULL, NULL);
	add_avsoftintr((void *) &xcall_hdl, XC_SOFT_PIL,
		       (avfunc) &xc_serv, "xcall", NULL, NULL);

	xc_init(CPU);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- smp_term.                                         */
/*                                                                  */
/* Function	- Clean up SMP environment.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
smp_term()
{
	rem_avintr(NULL, 1, handle_ext_call, S390_INTR_EXT);
	rem_avintr(NULL, 1, handle_emg_sig, S390_INTR_EXT);
	rem_avintr(NULL, 1, handle_mal_alt, S390_INTR_EXT);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- poke_cpu.                                         */
/*                                                                  */
/* Function	- Interrupt another CPU.			    */
/*                                                                  */
/* 		  This is useful to make the other CPU process an   */ 
/* 		  external interrupt so that it recognizes an 	    */
/*		  address space trap (AST) for preempting a thread. */
/*                                                                  */
/*		  It is possible to be preempted here and be 	    */
/*		  resumed on the CPU being poked, so it isn't an    */
/*		  error to poke the current CPU. We could check     */
/*		  this and still get preempted after the check, so  */
/*		  we don't bother.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
poke_cpu(int cpun)
{
	uint32_t *ptr = (uint32_t *)&cpu[cpun]->cpu_m.poke_cpu_outstanding;

	/*
	 * If panicstr is set or a poke_cpu is already pending,
	 * no need to send another one. Use atomic swap to protect
	 * against multiple CPUs sending redundant pokes.
	 */
	if (panicstr || *ptr == B_TRUE ||
		__sync_val_compare_and_swap(ptr, B_FALSE, B_TRUE) == B_TRUE)
		return;

	sigp(cpun, sigp_Emergency, 0, NULL);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- chip_plat_get_coreid.                             */
/*                                                                  */
/* Function	- Return a core "id" for the given cpu_t.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

id_t
chip_plat_get_coreid(cpu_t *cp)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- handle_ext_call.                                  */
/*                                                                  */
/* Function	- External interrupt handler for external call      */
/*		  interrupts.                  		 	    */
/*		                               		 	    */
/*		  If the function to be executed for the external   */
/*		  call is NULL this indicates that we've just been  */
/*		  woken out of our idle slumber and just need to    */
/*		  check for work to do. Otherwise we've been asked  */
/*		  to do some work by another CPU so schedule a      */
/*		  soft interrupt to get that work done.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint_t
handle_ext_call(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;

	if (ip->u.ext.intcode != EXT_CALL) {
		return (DDI_INTR_UNCLAIMED);
	}

	if (CPU->cpu_m.func != NULL)
		(*setsoftint)(XC_SOFT_PIL, xcall_hdl.ih_pending);

	return (DDI_INTR_CLAIMED);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- handle_emg_sig.                                   */
/*                                                                  */
/* Function	- External interrupt handler for an emergency       */
/*		  signal interrupt. This signal is used to "poke"   */
/*		  a CPU so that it looks for work.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint_t
handle_emg_sig(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;

	if (ip->u.ext.intcode != EXT_ESIG) {
		return (DDI_INTR_UNCLAIMED);
	}

	CPU->cpu_m.poke_cpu_outstanding = B_FALSE;

	return (DDI_INTR_CLAIMED);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- handle_mal_alt.                                   */
/*                                                                  */
/* Function	- External interrupt handler for a malfunction      */
/*		  alert interrupt.            		 	    */
/*		                               		 	    */
/* S390X FIXME	- Another CPU has gone down - need to handle	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint_t
handle_mal_alt(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;

	if (ip->u.ext.intcode != EXT_MALF) {
		return (DDI_INTR_UNCLAIMED);
	}

	return (DDI_INTR_CLAIMED);
}

/*========================= End of Function ========================*/
