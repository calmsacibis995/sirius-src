/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mach_cpu_states.c				    */
/* 								    */
/* Function    -                                                    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - October, 2007   				    */
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

#define WATCHDOG_TIMER 0

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/t_lock.h>
#include <sys/uadmin.h>
#include <sys/panic.h>
#include <sys/reboot.h>
#include <sys/autoconf.h>
#include <sys/machsystm.h>
#include <sys/promif.h>
#include <sys/consdev.h>
#include <sys/kdi_impl.h>
#include <sys/callb.h>
#include <sys/cmn_err.h>
#include <sys/smp.h>
#include <sys/machs390x.h>
#include <sys/dumphdr.h>

#ifdef	TRAPTRACE
#include <sys/traptrace.h>
#endif /* TRAPTRACE */

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern u_longlong_t	gettick();
extern void pm_cfb_check_and_powerup(void);

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void reboot_machine(char *);
static void power_down(char *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

#ifdef	TRAPTRACE
u_longlong_t panic_tick;
#endif /* TRAPTRACE */

#if WATCHDOG_TIMER
int disable_watchdog_on_exit;
int watchdog_activated;
int watchdog_enable;
int watchdog_timeout_seconds;
#endif

extern dumphdr_t	*dumphdr;	/* dump header */
extern vnode_t		*dumpvp;	/* dump device vnode pointer */

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mdboot.                                           */
/*                                                                  */
/* Function	- Machine dependent code to reboot.		    */
/* 		  "mdep" is interpreted as a character pointer;     */
/*		  if non-null, it is a pointer to a string to be    */
/*		  used as the argument string when rebooting.	    */
/*                                                                  */
/* 		  "invoke_cb" is a boolean. It is set to true when  */
/*		  mdboot() can safely invoke CB_CL_MDBOOT callbacks */
/*		  before shutting the system down, i.e. when we are */
/*		  in a normal shutdown sequence (interrupts are not */
/*		  blocked, the system is not panic'ing or being     */
/*		  suspended).					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
mdboot(int cmd, int fcn, char *bootstr, boolean_t invoke_cb)
{

#if WATCHDOG_TIMER
	/*
	 * Disable the hw watchdog timer.
	 */
	if (disable_watchdog_on_exit && watchdog_activated) {
		mutex_enter(&tod_lock);
		(void) tod_clear_watchdog_timer();
		mutex_exit(&tod_lock);
	}
#endif

	/*
	 * XXX - rconsvp is set to NULL to ensure that output messages
	 * are sent to the underlying "hardware" device using the
	 * monitor's printf routine since we are in the process of
	 * either rebooting or halting the machine.
	 */
	rconsvp = NULL;

	/*
	 * At a high interrupt level we can't:
	 *	1) bring up the console
	 * or
	 *	2) wait for pending interrupts prior to redistribution
	 *	   to the current CPU
	 *
	 * so we do them now.
	 */
	pm_cfb_check_and_powerup();

	/* make sure there are no more changes to the device tree */
	devtree_freeze();

	if (invoke_cb)
		(void) callb_execute_class(CB_CL_MDBOOT, NULL);

	/*
	 * Clear any unresolved UEs from memory.
	 */
	page_retire_mdboot();

	/*
	 * stop other cpus which also raise our priority. since there is only
	 * one active cpu after this, and our priority will be too high
	 * for us to be preempted, we're essentially single threaded
	 * from here on out.
	 */
	stop_other_cpus();

	/*
	 * try and reset leaf devices.  reset_leaves() should only
	 * be called when there are no other threads that could be
	 * accessing devices
	 */
	reset_leaves();

	if (fcn == AD_HALT) {
		halt((char *)NULL);
	} else if (fcn == AD_POWEROFF) {
		power_down("Powering down");
	} else {
		if (bootstr == NULL) {
			switch (fcn) {

			case AD_BOOT:
				bootstr = "";
				break;

			case AD_IBOOT:
				bootstr = "-a";
				break;

			case AD_SBOOT:
				bootstr = "-s";
				break;

			case AD_SIBOOT:
				bootstr = "-sa";
				break;
			default:
				cmn_err(CE_WARN,
				    "mdboot: invalid function %d", fcn);
				bootstr = "";
				break;
			}
		}
		reboot_machine(bootstr);
	}
	/* MAYBE REACHED */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mdpreboot.                                        */
/*                                                                  */
/* Function	- Pre-reboot - may be called prior to mdboot while  */
/*		  root fs is still mounted.    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
mdpreboot(int cmd, int fcn, char *bootstr)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- reboot_machine.                                   */
/*                                                                  */
/* Function	- Halt the machine and then reboot with the device  */
/*		  and arguments in bootstr.    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
reboot_machine(char *bootstr)
{
	int  IPL  = 0xc9d7d340;
	long lIPL = sizeof(IPL);

	stop_other_cpus();		/* send stop signal to other CPUs */
	CPU->cpu_flags |= CPU_QUIESCED;
	__asm__ ("	lra	0,%0\n"
		 "	lgr	1,%1\n"
		 "	diag	0,1,0x08\n"
		 : : "m" (IPL), "r" (lIPL)
		 : "0", "1");
	/*NOTREACHED*/
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_idle.                                       */
/*                                                                  */
/* Function	- Once in panic_idle() they raise spl, record their */
/*		  location, and spin.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
panic_idle(void)
{
	(void) spl7();
	(void) setjmp(&curthread->t_pcb);

	quiesce_cpu();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_stopcpus.                                   */
/*                                                                  */
/* Function	- Force the other CPUs to stop.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
panic_stopcpus(cpu_t *cp, kthread_t *t, int spl)
{
	int i;

	spl8();
	for (i = 0; i < NCPU; i++) {
		if (i != cp->cpu_id && cpu[i] != NULL &&
		    (cpu[i]->cpu_flags & CPU_EXISTS) &&
		    !(cpu[i]->cpu_flags & CPU_QUIESCED)) {
			cpu[i]->cpu_flags |= CPU_QUIESCED;
			sigp(i, sigp_StopStoreStatus, NULL, NULL);
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_enter_hw.                                   */
/*                                                                  */
/* Function	- Nothing to do here.                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
panic_enter_hw(int spl)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_quiesce_hw.                                 */
/*                                                                  */
/* Function	- Miscellaneous hardware-specific code to execute   */
/*		  after panicstr is set by the panic code.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
panic_quiesce_hw(panic_data_t *pdp)
{
	char msg[512] = "MSG * AT * ";
	int  msgpos;

	strcpy(&msg[11],&panicbuf[pdp->pd_msgoff]);

	msgpos = strlen(msg);
	
	a2e(msg,  msgpos);

	__asm__("	lra	1,%0\n"
		"	lgr	2,%1\n"
		"	diag	1,2,0x8\n"
		: 
		: "a" (&msg), "r" (msgpos)
		: "1", "2", "cc");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- power_down.                                       */
/*                                                                  */
/* Function	- No-op on s390x.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
power_down(char *str)
{
	halt(str);
	/*NOTREACHED*/
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- panic_dump_hw.                                    */
/*                                                                  */
/* Function	- Platform callback prior to writing crash dump.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
panic_dump_hw(int spl)
{
	char	dumpcmd[31] = "VMDUMP 0.ALL DCSS TO * *Panic";
	int	lcmd = sizeof(dumpcmd);

	a2e(dumpcmd,  lcmd);

#if 0
	if (dumpvp == NULL || dumphdr == NULL) {
		__asm__ ("	lra	1,0(%0)\n"
			 "	diag	1,%1,0x08\n"
			 : : "a" (dumpcmd), "r" (lcmd) : "1");
	}
#endif
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_faulted_enter.                                */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
cpu_faulted_enter(struct cpu *cp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_faulted_exit.                                 */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
cpu_faulted_exit(struct cpu *cp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mach_dump_buffer_init.                            */
/*                                                                  */
/* Function	- Setup dump buffer to store extra crash information*/
/*		  not applicable to s390x.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
mach_dump_buffer_init(void)
{
}

/*========================= End of Function ========================*/

#if WATCHDOG_TIMER

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- clear_watchdog_on_exit.                           */
/*                                                                  */
/* Function	- Only shut down an active hardware watchdog timer  */
/*		  if the platform has expressed an interest in doing*/
/*		  so.						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
clear_watchdog_on_exit()
{
	if (disable_watchdog_on_exit && watchdog_activated) {
		prom_printf("Debugging requested; hardware watchdog "
		    "disabled; reboot to re-enable.\n");
		cmn_err(CE_WARN, "!Debugging requested; hardware watchdog "
		    "disabled; reboot to re-enable.");
		mutex_enter(&tod_lock);
		(void) tod_clear_watchdog_timer();
		mutex_exit(&tod_lock);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kdi_watchdog_disable.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
kdi_watchdog_disable(void)
{
	if (watchdog_activated) {
		mutex_enter(&tod_lock);
		(void) tod_clear_watchdog_timer();
		mutex_exit(&tod_lock);
	}

	return (watchdog_activated);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kdi_watchdog_restore.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kdi_watchdog_restore(void)
{
	if (watchdog_enable) {
		mutex_enter(&tod_lock);
		(void) tod_set_watchdog_timer(watchdog_timeout_seconds);
		mutex_exit(&tod_lock);
	}
}

/*========================= End of Function ========================*/

#endif
