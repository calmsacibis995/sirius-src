/*------------------------------------------------------------------*/
/* 								    */
/* Name        - hardclk.c  					    */
/* 								    */
/* Function    - Time of day (TOD) clock support functions.         */
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

#define NS2TOD(a)	(((a) / 1000) << 12)
#define TOD2NS(a)	(((a) * 1000) >> 12)
#define EPCSTRT		0x7d9104a0c5180000UL	// 1970-01-01

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/machparam.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/clock.h>
#include <sys/intr.h>
#include <sys/cpuvar.h>
#include <sys/promif.h>
#include <sys/mman.h>
#include <sys/sysmacros.h>
#include <sys/lockstat.h>
#include <vm/as.h>
#include <vm/hat.h>
#include <sys/intr.h>
#include <sys/machsystm.h>
#include <sys/reboot.h>
#include <sys/atomic.h>

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

/*
 * Hardware watchdog parameters and knobs - FUTURE add-on
 */
int watchdog_enable = 0;		/* user knob */
int watchdog_available = 0;		/* system has a watchdog */
int watchdog_activated = 0;		/* the watchdog is armed */
uint_t watchdog_timeout_seconds = CLK_WATCHDOG_DEFAULT;

uint64_t todAdjust = 0;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- tod_get.                                          */
/*                                                                  */
/* Function	- Get the current time-of-day and put in 'ts'.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

timestruc_t
tod_get(void)
{
	hrtime_t tod = cvt2nano(stckepoch());
	uint64_t nano = NANOSEC;
	timespec_t now;

	__asm__ ("	lgr	1,%2\n"
		 "	lghi	0,0\n"
		 "	dlgr	0,%3\n"
		 "	lgr	%0,1\n"
		 "	lgr	%1,0\n"
		 : "=r" (now.tv_sec), "=r" (now.tv_nsec)
		 : "r" (tod), "r" (nano)
		 : "0", "1", "cc");

	return (now);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- tod_set.                                          */
/*                                                                  */
/* Function	- Set the current time-of-day from 'ts'.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
tod_set(timestruc_t ts)
{
	uint64_t    curtod,
		    newtod,
		    ns;
	int	    cc;

	tod_fault_reset();
	ns        = ts.tv_nsec + (ts.tv_sec * 1000000000);
	newtod    = NS2TOD(ns) + EPCSTRT;
	curtod    = stck();
	todAdjust = newtod - curtod;

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hr_clock_lock.                                    */
/*                                                                  */
/* Function	- Lock access to the clock.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
hr_clock_lock(void)
{
	ushort_t s;

	CLOCK_LOCK(&s);
	return (s);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hr_clock_unlock.                                  */
/*                                                                  */
/* Function	- Unlock access to the clock.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hr_clock_unlock(int s)
{
	CLOCK_UNLOCK(s);
}

/*========================= End of Function ========================*/
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mon_clock_init.                                   */
/*                                                                  */
/* Function	- Nop.                                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mon_clock_init(void)
{}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mon_clock_start.                                  */
/*                                                                  */
/* Function	- Nop.                                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mon_clock_start(void)
{}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mon_clock_share.                                  */
/*                                                                  */
/* Function	- Nop.                                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mon_clock_share(void)
{}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mon_clock_unshare.                                */
/*                                                                  */
/* Function	- Nop.                                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mon_clock_unshare(void)
{}

/*========================= End of Function ========================*/
