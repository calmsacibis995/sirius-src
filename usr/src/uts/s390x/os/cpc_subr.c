/*------------------------------------------------------------------*/
/* 								    */
/* Name        - kcpc_subr.c					    */
/* 								    */
/* Function    - Platform specific CPU counter subroutines.         */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - October, 2007.  				    */
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
#include <sys/time.h>
#include <sys/atomic.h>
#include <sys/thread.h>
#include <sys/regset.h>
#include <sys/archsystm.h>
#include <sys/machsystm.h>
#include <sys/cpc_impl.h>
#include <sys/sunddi.h>
#include <sys/intr.h>
#include <sys/x_call.h>
#include <sys/cpuvar.h>
#include <sys/machcpuvar.h>
#include <sys/cpc_pcbe.h>
#include <sys/modctl.h>
#include <sys/sdt.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern int kcpc_counts_include_idle;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

char	*boot_cpu_compatible_list[2] = {"ibmz9", NULL};

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kcpc_hw_init.                                     */
/*                                                                  */
/* Function	- Called on the boot CPU during startup.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kcpc_hw_init(cpu_t *cp)
{
	/*
	 * Make sure the boot CPU gets set up.
	 */
	kcpc_hw_startup_cpu(CPU->cpu_flags);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	- Install an idle thread CPC context.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kcpc_hw_startup_cpu(ushort_t cpflags)
{
	cpu_t		*cp = CPU;
	kthread_t	*t = cp->cpu_idle_thread;

	ASSERT(t->t_bound_cpu == cp);

	mutex_init(&cp->cpu_cpc_ctxlock, "cpu_cpc_ctxlock", MUTEX_DEFAULT, 0);

	if (kcpc_counts_include_idle)
		return;

	installctx(t, cp, kcpc_idle_save, kcpc_idle_restore, NULL, NULL,
	    NULL, NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kcpc_hw_load_pcbe.                                */
/*                                                                  */
/* Function	- Load the appropriate PCBE.                        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
kcpc_hw_load_pcbe(void)
{
	char		modname[MODMAXNAMELEN+1];
	char		*p, *q;
	int		len, stat;

	for (stat = -1, p = (char *) boot_cpu_compatible_list; p != NULL; p = q) {
		/*
		 * Get next CPU module name from boot_cpu_compatible_list
		 */
		q = strchr(p, ':');
		len = (q) ? (q - p) : strlen(p);
		if (len < sizeof (modname)) {
			(void) strncpy(modname, p, len);
			modname[len] = '\0';
			stat = kcpc_pcbe_tryload(modname, 0, 0, 0);
			if (stat == 0)
				break;
		}
		if (q)
			q++;			/* skip over ':' */
	}
	return (stat);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kcpc_remotestop_func.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
kcpc_remotestop_func(uint64_t arg1, uint64_t arg2)
{
	ASSERT(CPU->cpu_cpc_ctx != NULL);
	pcbe_ops->pcbe_allstop();
	atomic_or_uint(&CPU->cpu_cpc_ctx->kc_flags, KCPC_CTX_INVALID_STOPPED);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kcpc_remote_stop.                                 */
/*                                                                  */
/* Function	- Ensure the counters are stopped on the given      */
/*		  processor. Callers must ensure kernel preemption  */
/*		  is disabled.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kcpc_remote_stop(cpu_t *cp)
{
	xc_one(cp->cpu_id, kcpc_remotestop_func, 0, 0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kcpc_hw_cpu_hook.                                 */
/*                                                                  */
/* Function	- Called by the generic framework to check if it's  */
/*		  OK to bind a set to a CPU. 			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
kcpc_hw_cpu_hook(processorid_t cpuid, ulong_t *kcpc_cpumap)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kcpc_hw_lwp_hook.                                 */
/*                                                                  */
/* Function	- Called by the generic framework to check if it's  */
/*		  OK to bind a set to an LWP.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
kcpc_hw_lwp_hook(void)
{
	return (0);
}

/*========================= End of Function ========================*/
