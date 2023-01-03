/*------------------------------------------------------------------*/
/* 								    */
/* Name        - x_call.c   					    */
/* 								    */
/* Function    - Cross-CPU call support routines.                   */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - June, 2007  					    */
/* 								    */
/*------------------------------------------------------------------*/

/*
 * Although these routines protect the services from migrating to other cpus
 * "after" they are called, it is the caller's choice or responsibility to
 * prevent the cpu migration "before" calling them.
 *
 * X-call routines:
 *
 *	xc_one()  - send a request to one processor
 *	xc_some() - send a request to some processors
 *	xc_all()  - send a request to all processors
 *
 *	Their common parameters:
 *		func - a TL=0 handler address
 *		arg1 and arg2  - optional
 *
 *	The services provided by x-call routines allow callers
 *	to send a request to target cpus to execute a TL=0
 *	handler.
 *
 *	The interface of the registers of the TL=0 handler:
 *		%r2: arg1
 *		%r3: arg2
 *
 *	The  parameters:
 *		func - a TL>0 handler address or an interrupt number
 *		arg1, arg2
 *		       optional when "func" is an address;
 *		       0        when "func" is an interrupt number
 *
 */

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

#include <sys/systm.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/cpuvar.h>
#include <sys/intreg.h>
#include <sys/x_call.h>
#include <sys/cmn_err.h>
#include <sys/disp.h>
#include <sys/debug.h>
#include <sys/privregs.h>
#include <sys/avintr.h>
#include <sys/sysmacros.h>
#include <sys/exts390x.h>
#include <sys/smp.h>
#include <sys/spl.h>

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

#if defined(DEBUG) || defined(TRAPTRACE)

uint_t x_dstat[NCPU][XC_MAX];
uint_t x_rstat[NCPU][4];

#endif /* DEBUG || TRAPTRACE */

/*
 * if == 1, an xc_loop timeout will cause a panic
 * otherwise print a warning
 */
uint_t xc_loop_panic = 0;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xc_init.                                          */
/*                                                                  */
/* Function	- Initialize x-call related locks.                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
xc_init(struct cpu *cp)
{
	mutex_init(&cp->cpu_m.sigMut, NULL, MUTEX_DEFAULT,
		   (void *)ipltospl(XCALL_PIL));

	sema_init(&cp->cpu_m.sigSem, 0, NULL, SEMA_DEFAULT, NULL);

#if defined(DEBUG) || defined(TRAPTRACE)
	XC_STAT_INIT(cp->cpu_id);
#endif /* TRAPTRACE */

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xc_one.                                           */
/*                                                                  */
/* Function	- Send an "x-call" to a CPU.                        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
xc_one(int cix, xcfunc_t *func, uint64_t arg1, uint64_t arg2)
{
	int lcx = CPU->cpu_id;
	int opl, rc, done = 0;
	uint64_t loop_cnt = 0;
	cpu_t	 *cp;

	/*
	 * send to nobody; just return
	 */
	if (!CPU_IN_SET(cpu_ready_set, cix))
		return;

	cp = cpu[cix];

	kpreempt_disable();

	if (cix == lcx) {	/* same cpu just do it */
		XC_TRACE(XC_ONE_SELF, lcx, func, arg1, arg2);
		(*func)(arg1, arg2);
		XC_STAT_INC(x_dstat[lcx][XC_ONE_SELF]);
	} else {

		mutex_enter(&cp->cpu_m.sigMut);
		cp->cpu_m.func = func;
		cp->cpu_m.arg1 = arg1; 
		cp->cpu_m.arg2 = arg2; 
		do {
			rc = sigp(cix, sigp_ExtCall, NULL, NULL);
			/*
			 * If the wakeup function failed then it's because
			 * the other CPU has become active so there's no need
			 * to poke it
			 */
			if (func == XC_WAKE_FN)
				break;
			switch (rc) {
			case sigp_OrderCodeAccepted :
//				yield_cpu(cix);		FIXME - z/VM 5.3 required
				while (cp->cpu_m.func != NULL);
				done = 1;
			break;
			case sigp_StatusStored :
				done = 0;
			break;
			sigp_Busy :
				done = 0;
			break;
			case sigp_NotOp :
				cmn_err(CE_PANIC, "xc_one(%d) CPU not operational",
					cix);
				done = 1;
			}
		} while (!done);
		mutex_exit(&cp->cpu_m.sigMut);
		XC_TRACE(XC_ONE_OTHER, cix, func, arg1, arg2);
		XC_STAT_INC(x_dstat[cix][XC_ONE_OTHER]);
	}

	kpreempt_enable();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xc_some.                                          */
/*                                                                  */
/* Function	- Send an "x-call" to some CPUs (except to self).   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
xc_some(cpuset_t cpuset, xcfunc_t *func, uint64_t arg1, uint64_t arg2)
{
	int 	 lcx,
		 cix = 0;
	cpuset_t xc_cpuset;
	cpu_t	 *cp;


	lcx = CPU->cpu_id;

	kpreempt_disable();

	/*
	 * only send to the CPU_READY ones
	 */
	xc_cpuset = cpu_ready_set;
	CPUSET_AND(xc_cpuset, cpuset);

	/*
	 * send to nobody; just return
	 */
	while (!CPUSET_ISNULL(xc_cpuset)) {

		if (CPU_IN_SET(xc_cpuset, lcx)) {
			/*
			 * same cpu just do it
			 */
			(*func)(arg1, arg2);
			CPUSET_DEL(xc_cpuset, lcx);
			XC_TRACE(XC_SOME_SELF, lcx, func, arg1, arg2);
			XC_STAT_INC(x_dstat[lcx][XC_SOME_SELF]);
		} else {
			cp = cpu[cix];
			mutex_enter(&cp->cpu_m.sigMut);
			cp->cpu_m.func = func;
			cp->cpu_m.arg1 = arg1; 
			cp->cpu_m.arg2 = arg2; 
			if (sigp(cix, sigp_ExtCall, NULL, NULL) != sigp_NotOp)
				sema_p(&cp->cpu_m.sigSem);
			else
				cmn_err(CE_PANIC, "xc_some(%d) CPU not operational",
					cix);
			mutex_exit(&cp->cpu_m.sigMut);
			CPUSET_DEL(xc_cpuset, cix);
			XC_TRACE(XC_SOME_OTHER, cix, func, arg1, arg2);
			XC_STAT_INC(x_dstat[cix][XC_SOME_OTHER]);
		}
		cix++;
	}
	kpreempt_enable();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xc_all.                                           */
/*                                                                  */
/* Function	- Send an "x-call" to all CPUs.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
xc_all(xcfunc_t *func, uint64_t arg1, uint64_t arg2)
{
	int 	 lcx,
		 cix = 0;
	cpuset_t xc_cpuset;
	cpu_t	 *cp;

	lcx = CPU->cpu_id;

	kpreempt_disable();

	/*
	 * only send to the CPU_READY ones
	 */
	xc_cpuset = cpu_ready_set;

	/*
	 * send to nobody; just return
	 */
	while (!CPUSET_ISNULL(xc_cpuset)) {

		if (CPU_IN_SET(xc_cpuset, lcx)) {
			/*
			 * same cpu just do it
			 */
			(*func)(arg1, arg2);
			CPUSET_DEL(xc_cpuset, lcx);
			XC_TRACE(XC_ALL_SELF, lcx, func, arg1, arg2);
			XC_STAT_INC(x_dstat[lcx][XC_ALL_SELF]);
		} else {
			cp = cpu[cix];
			mutex_enter(&cp->cpu_m.sigMut);
			cp->cpu_m.func = func;
			cp->cpu_m.arg1 = arg1; 
			cp->cpu_m.arg2 = arg2; 
			if (sigp(cix, sigp_ExtCall, NULL, NULL) != sigp_NotOp)
				sema_p(&cp->cpu_m.sigSem);
			else
				cmn_err(CE_PANIC, "xc_all(%d) CPU not operational",
					cix);
			mutex_exit(&cp->cpu_m.sigMut);
			CPUSET_DEL(xc_cpuset, cix);
			XC_TRACE(XC_ALL_OTHER, cix, func, arg1, arg2);
			XC_STAT_INC(x_dstat[cix][XC_ALL_OTHER]);
		}
		cix++;
	}
	kpreempt_enable();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xc_serv.                                          */
/*                                                                  */
/* Function	- X-call handler at TL=0; serves only one x-call    */
/*		  request. Runs at XCALL_PIL level.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint_t
xc_serv(void)
{
	int lcx = (int)(CPU->cpu_id);
	xcfunc_t *func;
	uint64_t arg1,
		 arg2;

	XC_TRACE(XC_SERV, lcx, CPU->cpu_m.func, CPU->cpu_m.arg1, CPU->cpu_m.arg2);
	func = CPU->cpu_m.func;
	arg1 = CPU->cpu_m.arg1;
	arg2 = CPU->cpu_m.arg2;
	if (func != XC_WAKE_FN) {
		(*func)(arg1, arg2);
	}
	CPU->cpu_m.func = NULL;
	XC_STAT_INC(x_rstat[lcx][XC_SERV]);
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xc_trace.                                         */
/*                                                                  */
/* Function	- Trace an x-call request or servicing.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void 
xc_trace(uint_t type, int cpuid, xcfunc_t *func, uint64_t arg1, uint64_t arg2)
{
}

/*========================= End of Function ========================*/
