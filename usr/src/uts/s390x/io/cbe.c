/*------------------------------------------------------------------*/
/* 								    */
/* Name        - cbe.c      					    */
/* 								    */
/* Function    - Timer functions for the system.                    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - May, 2007   					    */
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

#define CBE_MAX_HRTIME	0x44b82fa0a
#define HZTIMER		10000000

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/clock.h>
#include <sys/intr.h>
#include <sys/avintr.h>
#include <sys/cpuvar.h>
#include <sys/promif.h>
#include <sys/kmem.h>
#include <sys/machsystm.h>
#include <sys/machs390x.h>
#include <sys/exts390x.h>
#include <sys/cyclic.h>
#include <sys/cyclic_impl.h>
#include <sys/smp_impldefs.h>
#include <sys/x_call.h>

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

cyclic_id_t cbe_hres_cyclic;

uint64_t cbe_level14_inum;

static hrtime_t cbe_suspend_delta = 0;
static hrtime_t cbe_suspend_time = 0;

static ddi_softint_hdl_impl_t cbe_low_hdl =
	{0, NULL, NULL, NULL, 0, NULL, NULL, NULL};

static ddi_softint_hdl_impl_t cbe_clock_hdl =
	{0, NULL, NULL, NULL, 0, NULL, NULL, NULL};

static uint64_t nanosec = NANOSEC;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setClockComparator.                               */
/*                                                                  */
/* Function	- Set the clock comparator.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static __inline__ void
setClockComparator(uint64_t cValue)
{
	uint64_t *pClock;
	cpu_t	 *cpu = CPU;

	pClock = &cValue;

	__asm__ ("	sckc	0(%2)\n"
		 "	stck	%0\n"
		 "	stckc	%1\n"
		 : "=m" (cpu->cpu_m.clk), "=m" (cpu->cpu_m.ckc)
		 : "a" (pClock));

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hrtime2tick.                                      */
/*                                                                  */
/* Function	- Convert high-resolution timer value to a tick.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint64_t
hrtime2tick(hrtime_t ts)
{
	hrtime_t q = ts / nanosec;
	hrtime_t r = ts - (q * nanosec);

	return (q * hz + ((r * hz) / nanosec));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- unscalehrtime.                                    */
/*                                                                  */
/* Function	- Return an unscaled time from the high-res timer.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint64_t
unscalehrtime(hrtime_t ts)
{
	uint64_t unscale = 0;
	hrtime_t rescale;
	hrtime_t diff = ts;

	while (diff > NSECS_PER_TICK) {
		unscale += hrtime2tick(diff);
		rescale = unscale;
		scalehrtime(&rescale);
		diff = (ts - rescale) << 13;
	}

	return (unscale);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- clock_cmp_handler.                                */
/*                                                                  */
/* Function	- Handle a clock comparator interrupt.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint_t
clock_cmp_handler(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;

	if (ip->u.ext.intcode != EXT_CLKC) {
		return (DDI_INTR_UNCLAIMED);
	}

	setClockComparator(stck() + 0x2710000);
	cyclic_fire(CPU);
//	(*setsoftint)(CBE_LOCK_PIL, cbe_clock_hdl.ih_pending);

	return (DDI_INTR_CLAIMED);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_softclock.                                    */
/*                                                                  */
/* Function	- Handle a clock comparator interrupt.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
cbe_softclock(void *arg1, void *arg2)
{
	cyclic_softint(CPU, CY_LOCK_LEVEL);
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_low_level.                                    */
/*                                                                  */
/* Function	- Handle a low level soft interrupt.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
cbe_low_level(void)
{
	cyclic_softint(CPU, CY_LOW_LEVEL);
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_enable.                                       */
/*                                                                  */
/* Function	- Enable the timer.                                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
cbe_enable(cyb_arg_t arg)
{
	ctlr0 cr;
	long ckcInit = -1;

	setClockComparator(ckcInit);

	cr.value = get_cr(0);
	cr.mask.clkCmp = 1;
	set_cr(cr.value, 0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_disable.                                      */
/*                                                                  */
/* Function	- Disable the timer.                                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
cbe_disable(cyb_arg_t arg)
{
	ctlr0 cr;

	cr.value = get_cr(0);
	cr.mask.clkCmp = 0;
	set_cr(cr.value, 0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_reprogram.                                    */
/*                                                                  */
/* Function	- Reprogram the timer.                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
cbe_reprogram(cyb_arg_t arg, hrtime_t time)
{
	uint64_t next,
		 now = gethrtime();
	static   uint64_t delay = (1 << 13);	// 4 microseconds

	next = now + HZTIMER;

	if ((time > next) || (time < now))
		time = next;

	/*
	 * If we are tracing then we could have delayed things 
	 * such that the next timer clock pop has already expired
	 * then we'll cause the timer to pop in a few microseconds
	 */
	if (time > gethrtime())
		setClockComparator(nano2tod(time));
	else
		setClockComparator(stckbase()+delay);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_softint.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
cbe_softint(cyb_arg_t arg, cyc_level_t level)
{
	switch (level) {
	case CY_LOW_LEVEL:
		(*setsoftint)(CBE_LOW_PIL, cbe_low_hdl.ih_pending);
		break;
	case CY_LOCK_LEVEL:
		(*setsoftint)(CBE_LOCK_PIL, cbe_clock_hdl.ih_pending);
		break;
	default:
		panic("cbe_softint: unexpected soft level %d", level);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_set_level.                                    */
/*                                                                  */
/* Function	- Set the interrupt level.                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static cyc_cookie_t
cbe_set_level(cyb_arg_t arg, cyc_level_t level)
{
	int ipl;

	switch (level) {
	case CY_LOW_LEVEL:
		ipl = CBE_LOW_PIL;
		break;
	case CY_LOCK_LEVEL:
		ipl = CBE_LOCK_PIL;
		break;
	case CY_HIGH_LEVEL:
		ipl = CBE_HIGH_PIL;
		break;
	default:
		panic("cbe_set_level: unexpected level %d", level);
	}

	return (splr(ipl));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_restore_level.                                */
/*                                                                  */
/* Function	- Restore the interrupt level to the given level.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
cbe_restore_level(cyb_arg_t arg, cyc_cookie_t cookie)
{
	splx(cookie);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_xcall_handler.                                */
/*                                                                  */
/* Function	- Cross-CPU timer call.                             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
cbe_xcall_handler(uint64_t arg1, uint64_t arg2)
{
	cyc_func_t func = (cyc_func_t)arg1;
	void *arg = (void *)arg2;

//	cyclic_fire(CPU);
	(*func)(arg);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_xcall.                                        */
/*                                                                  */
/* Function	- Make a cross-system call.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
cbe_xcall(cyb_arg_t arg, cpu_t *dest, cyc_func_t func, void *farg)
{
	kpreempt_disable();

	if (dest == CPU) {
		(*func)(farg);
	} else
		xc_one(dest->cpu_id, cbe_xcall_handler, (uint64_t)func, (uint64_t)farg);

	kpreempt_enable();

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_configure.                                    */
/*                                                                  */
/* Function	- Configure the timer.                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static cyb_arg_t
cbe_configure(cpu_t *cpu)
{
	cbe_data_t *new_data = kmem_alloc(sizeof (cbe_data_t), KM_SLEEP);

	return (new_data);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_unconfigure.                                  */
/*                                                                  */
/* Function	- Unconfigure the timer.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
cbe_unconfigure(cyb_arg_t arg)
{
	cbe_data_t *data = (cbe_data_t *)arg;

	kmem_free(data, sizeof (cbe_data_t));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_suspend.                                      */
/*                                                                  */
/* Function	- Suspend the timer.                                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
cbe_suspend(cyb_arg_t arg)
{
	cbe_suspend_time = gethrtime_unscaled();
	cbe_suspend_delta = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_resume.                                       */
/*                                                                  */
/* Function	- Resume the timer.                                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
cbe_resume(cyb_arg_t arg)
{
	hrtime_t now;

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_hres_tick.                                    */
/*                                                                  */
/* Function	- Process a timer tick.                             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cbe_hres_tick(void)
{
	dtrace_hres_tick();
	hres_tick();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_init_pre.                                     */
/*                                                                  */
/* Function	- Pre-initialization work for timer.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cbe_init_pre(void)
{
	/* Nothing to do on s390x */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cbe_init.                                         */
/*                                                                  */
/* Function	- Initialize the timer.                             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
cbe_init(void)
{
	cyc_handler_t hdlr;
	cyc_time_t when;
	hrtime_t resolution = NANOSEC / NSECS_PER_TICK;

	cyc_backend_t cbe = {
		cbe_configure,		/* cyb_configure */
		cbe_unconfigure,	/* cyb_unconfigure */
		cbe_enable,		/* cyb_enable */
		cbe_disable,		/* cyb_disable */
		cbe_reprogram,		/* cyb_reprogram */
		cbe_softint,		/* cyb_softint */
		cbe_set_level,		/* cyb_set_level */
		cbe_restore_level,	/* cyb_restore_level */
		cbe_xcall,		/* cyb_xcall */
		cbe_suspend,		/* cyb_suspend */
		cbe_resume		/* cyb_resume */
	};

	(void) add_avintr(NULL, 1, clock_cmp_handler, "clock_comparator",
			  S390_INTR_EXT, 0, 0, NULL, NULL);

	(void) add_avsoftintr((void *)&cbe_clock_hdl, CBE_LOCK_PIL,
			      (avfunc)cbe_softclock, "softclock", NULL, NULL);

	(void) add_avsoftintr((void *)&cbe_low_hdl, CBE_LOW_PIL,
			      (avfunc)cbe_low_level, "low level", NULL, NULL);

	/*
	 * If sys_tick_freq > NANOSEC (i.e. we're on a CPU with a clock rate
	 * which exceeds 1 GHz), we'll specify the minimum resolution,
	 * 1 nanosecond.
	 */
	if (resolution == 0)
		resolution = 1;

	mutex_enter(&cpu_lock);
	cyclic_init(&cbe, resolution);

	/*
	 * Initialize hrtime_base and hres_last_tick to reasonable starting
	 * values.
	 */
	hrtime_base       = gethrtime();
	hres_last_tick    = gethrtime_unscaled();

	hdlr.cyh_level    = CY_HIGH_LEVEL;
	hdlr.cyh_func     = (cyc_func_t)cbe_hres_tick;
	hdlr.cyh_arg      = NULL;

	when.cyt_when     = 0;
	when.cyt_interval = nsec_per_tick;

	cbe_hres_cyclic   = cyclic_add(&hdlr, &when);

	mutex_exit(&cpu_lock);
}

/*========================= End of Function ========================*/
