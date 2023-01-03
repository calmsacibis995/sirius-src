/*------------------------------------------------------------------*/
/* 								    */
/* Name        - cpu_module.c					    */
/* 								    */
/* Function    -                                                    */
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
#include <sys/asm_linkage.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/avintr.h>
#include <sys/clock.h>
#include <sys/lockstat.h>
#include <sys/time.h>
#include <sys/archsystm.h>

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

static __inline__ timespec_t todhrestime(void);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

uint_t		root_phys_addr_lo_mask;
int64_t 	timedelta;
hrtime_t 	hres_last_tick;
volatile 	timestruc_t	hrestime;
int64_t 	hrestime_adj;
uint32_t 	hres_lock;
uint_t 		nsec_scale;
uint_t 		nsec_shift;
uint_t 		adj_shift;
hrtime_t 	hrtime_base;
int 		traptrace_use_stick;
uchar_t 	*ctx_pgsz_array;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- todhrestime.                                      */
/*                                                                  */
/* Function	- Return a timestruct_t that has the current time.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static __inline__ timespec_t
todhrestime()
{
	uint64_t tod;
	uint64_t nano = NANOSEC;
	timespec_t now;

	tod = tod2nano();

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
/* Name		- flush_instr_mem.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
flush_instr_mem(caddr_t addr, size_t len)
{}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- syncfpu.                                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
syncfpu(void)
{}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getthrestime.                                     */
/*                                                                  */
/* Function	- Return the hres time.                             */
/*                                                                  */
/* 		  This routine is almost correct now, but not quite.*/  
/*		  It still needs the equivalent concept of 	    */
/*		  "hres_last_tick", just like on the sparc side.    */
/* 		  The idea is to take a snapshot of the hi-res timer*/
/*		  while doing the hrestime_adj updates under 	    */
/*		  hres_lock in locore, so that the small interval   */
/*		  between interrupt assertion and interrupt process-*/
/*		  ing is accounted for correctly.  Once we have     */
/*		  this, the code below should be modified to sub-   */
/*		  tract off hres_last_tick rather than hrtime_base. */
/*                                                                  */
/*------------------------------------------------------------------*/

void
gethrestime(timespec_t *tp)
{
	int lock_prev;
	timestruc_t now;
	int nslt;		/* nsec since last tick */
	int adj;		/* amount of adjustment to apply */

	hres_tick();
	*tp = hrestime;
	hrestime_adj   = 0;
#if 0	// S390X FIXME - remove code below if above works out
loop:
	lock_prev = hres_lock;
	now = hrestime = todhrestime();
	nslt = (int)(gethrtime_unscaled() - hres_last_tick);
	if (nslt < 0) {
		/*
		 * nslt < 0 means a tick came between sampling
		 * gethrtime() and hres_last_tick; restart the loop
		 */

		goto loop;
	}
	now.tv_nsec += nslt;
	if (hrestime_adj != 0) {
		if (hrestime_adj > 0) {
			adj = (nslt >> ADJ_SHIFT);
			if (adj > hrestime_adj)
				adj = (int)hrestime_adj;
		} else {
			adj = -(nslt >> ADJ_SHIFT);
			if (adj < hrestime_adj)
				adj = (int)hrestime_adj;
		}
		now.tv_nsec += adj;
	}
	while ((unsigned long)now.tv_nsec >= NANOSEC) {

		/*
		 * We might have a large adjustment or have been in the
		 * debugger for a long time; take care of (at most) four
		 * of those missed seconds (tv_nsec is 32 bits, so
		 * anything >4s will be wrapping around).  However,
		 * anything more than 2 seconds out of sync will trigger
		 * timedelta from clock() to go correct the time anyway,
		 * so do what we can, and let the big crowbar do the
		 * rest.  A similar correction while loop exists inside
		 * hres_tick(); in all cases we'd like tv_nsec to
		 * satisfy 0 <= tv_nsec < NANOSEC to avoid confusing
		 * user processes, but if tv_sec's a little behind for a
		 * little while, that's OK; time still monotonically
		 * increases.
		 */

		now.tv_nsec -= NANOSEC;
		now.tv_sec++;
	}
	if ((hres_lock & ~1) != lock_prev)
		goto loop;

	*tp = now;
#endif
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- gethrestime_sec.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

time_t
gethrestime_sec(void)
{
	timestruc_t now;
	uint64_t tod = tod2nano();
	uint64_t nano = NANOSEC;

	__asm__ ("	lgr	1,%1\n"
		 "	lghi	0,0\n"
		 "	dlgr	0,%2\n"
		 "	srag	%2,%2,1\n"
		 "	sgr	%2,0\n"
		 "	jnm	0f\n"
		 "	aghi	1,1\n"
		 "0:	lgr	%0,1\n"
		 : "=r" (now.tv_sec)
		 : "r" (tod), "r" (nano)
		 : "0", "1", "cc");

	return (now.tv_sec);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- gethrestime_lasttick.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
gethrestime_lasttick(timespec_t *tp)
{
	int s;

	s = hr_clock_lock();
	*tp = hrestime = todhrestime();
	hr_clock_unlock(s);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- gethrtime.                                        */
/*                                                                  */
/* Function	- Return the current time in nanoseconds.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hrtime_t
gethrtime(void)
{
	hrtime_t tod = stckbase();

	return (cvt2nano(tod));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- gethrtime_unscaled.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hrtime_t
gethrtime_unscaled(void)
{ 
	return(gettick_counter());
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- gethrtime_waitfree.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hrtime_t
gethrtime_waitfree(void)
{ 
	return ((hrtime_t)tod2nano());
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dtrace_gethrtime.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hrtime_t
dtrace_gethrtime(void)
{
	return ((hrtime_t)tod2nano());
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- get_hrestime.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

hrtime_t
get_hrestime(void)
{ 
	timespec_t ts;
	hrtime_t   *hp;
		
	gethrestime(&ts);
	hp = (hrtime_t *) &ts;

	return(*hp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- scalehrtime.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
scalehrtime(hrtime_t *hrt)
{
	*hrt = (cvt2nano(*hrt));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- hres_tick.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
hres_tick(void)
{
	hrestime    = todhrestime();
	hrtime_base = hrestime.tv_sec * NANOSEC + hrestime.tv_nsec;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- drv_usecwait.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
drv_usecwait(clock_t n)
{
	uint64_t next;

	next = nano2tod(gethrtime() + n * 1000);

	while (next > stck());
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpu_init_private.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
dtrace_blksuword32(uintptr_t addr, uint32_t *data, int tryagain)
{ return (-1); }

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- scancc.                                           */
/*                                                                  */
/* Function	- C version of scanc instruction for testing.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
scancc(size_t length, uchar_t *string, uchar_t table[], uchar_t mask)
{
	uchar_t *end = &string[length];

	while (string < end) {
		if ((table[*string] & mask) != 0) {
			break;
		}
		string++;
	}

	return end - string;
}

/*========================= End of Function ========================*/

