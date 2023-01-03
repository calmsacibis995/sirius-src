/*------------------------------------------------------------------*/
/* 								    */
/* Name        - lock_prim.s					    */
/* 								    */
/* Function    - Various mutex/lock low level atomic routines.      */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - September, 2007 				    */
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

/* #define DEBUG */

#define membar		br	%r0
#define	MEMBAR_RETURN	br	%r14

#define MC_LOCK_TRY	1
#define MC_LOCK_SET	2
#define MC_LOCK_CLEAR	3
#define MC_LOCK_SET_SPL	4
#define MC_RW_RE_LOCK	5
#define MC_RW_WE_LOCK	6
#define MC_RW_RX_LOCK	7
#define MC_RW_WX_LOCK	8
#define MC_MX_ENTER	9

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#if defined(lint)
#include <sys/types.h>
#include <sys/thread.h>
#include <sys/cpuvar.h>
#else	/* lint */
#include "assym.h"
#endif	/* lint */

#include <sys/t_lock.h>
#include <sys/mutex.h>
#include <sys/mutex_impl.h>
#include <sys/rwlock_impl.h>
#include <sys/asm_linkage.h>
#include <sys/machlock.h>
#include <sys/lockstat.h>

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

	.global	mutex_exit_critical_size
	.global	mutex_exit_critical_start

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ldstub.                                           */
/*                                                                  */
/* Function	- Store 0xFF at the specified location, and return  */
/*		  its previous content.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)
uint8_t
ldstub(uint8_t *cp)
{
	uint8_t	rv;
	rv = *cp;
	*cp = 0xFF;
	return rv;
}
#else	/* lint */

	ENTRY(ldstub)
	llgc	%r0,0
	mvi	0(%r2),0xff
	lgr	%r2,%r0
	br	%r14
	SET_SIZE(ldstub)

#endif	/* lint */

/*========================= End of Function ========================*/

/********************************************************************
 *	MEMORY BARRIERS -- see atomic.h for full descriptions.
 *******************************************************************/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- membar_xxxx                                       */
/*                                                                  */
/* Function	- Various memory barriers.                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/


#if defined(lint)

void
membar_enter(void)
{}

void
membar_exit(void)
{}

void
membar_producer(void)
{}

void
membar_consumer(void)
{}

#else	/* lint */

	ENTRY(membar_enter)
	membar
	MEMBAR_RETURN
	SET_SIZE(membar_enter)

	ENTRY(membar_exit)
	membar
	MEMBAR_RETURN
	SET_SIZE(membar_exit)

	ENTRY(membar_producer)
	membar
	MEMBAR_RETURN
	SET_SIZE(membar_producer)

	ENTRY(membar_consumer)
	membar	
	MEMBAR_RETURN
	SET_SIZE(membar_consumer)

#endif	/* lint */


/*========================= End of Function ========================*/

/********************************************************************
 *	MINIMUM LOCKS
 *******************************************************************/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lock_try.                                         */
/*                                                                  */
/* Function	- Try a lock in kernel space.			    */
/*		  - returns non-zero on success.		    */
/*		  - doesn't block interrupts so don't use this to   */
/*		    spin on a lock.				    */
/*		  - uses "0xFF is busy, anything else is free" 	    */
/*		    model.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

int
lock_try(lock_t *lp)
{
	return (0xFF ^ ldstub(lp));
}

#else	/* lint */

	ENTRY(lock_try)
	ts	0(%r2)
	jnz	0f
	lghi	%r2,1
	mc	MC_LOCK_TRY,0
	br	%r14
0:
	lghi	%r2,0
	mc	MC_LOCK_TRY,0
	br	%r14
	SET_SIZE(lock_try)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lock_spin_try.                                    */
/*                                                                  */
/* Function	- Try a lock in kernel space.			    */
/*		  - returns non-zero on success.		    */
/*		  - uses "0xFF is busy, anything else is free" 	    */
/*		    model.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

int
lock_spin_try(lock_t *lp)
{
	return (0xFF ^ ldstub(lp));
}


#else	/* lint */

	ENTRY(lock_spin_try)
	ts	0(%r2)
	jnz	0f
	lghi	%r2,1
	mc	MC_LOCK_TRY,0
	br	%r14
0:
	lghi	%r2,0
	br	%r14
	SET_SIZE(lock_spin_try)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lock_set.                                         */
/*                                                                  */
/* Function	- Set a lock in kernel space.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
lock_set(lock_t *lp)
{
	extern void lock_set_spin(lock_t *);

	if (!lock_try(lp))
		lock_set_spin(lp);
	membar_enter();
}

#else	/* lint */

	ENTRY(lock_set)
	ts	0(%r2)
	jgnz	lock_set_spin
	mc	MC_LOCK_SET,0
	membar
	br	%r14
	SET_SIZE(lock_set)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lock_clear.                                       */
/*                                                                  */
/* Function	- Clear a kernel lock.        			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
lock_clear(lock_t *lp)
{
	membar_exit();
	*lp = 0;
}

#else	/* lint */

	ENTRY(lock_clear)
	membar	
	mvi 	0(%r2),0
	mc	MC_LOCK_CLEAR,0
	br	%r14
	SET_SIZE(lock_clear)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ulock_try.                                        */
/*                                                                  */
/* Function	- Try a lock in user space.			    */
/*		  - returns non-zero on success.		    */
/*		  - doesn't block interrupts so don't use this to   */
/*		    spin on a lock.				    */
/*		  - uses "0xFF is busy, anything else is free" 	    */
/*		    model.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

int
ulock_try(lock_t *lp)
{
	return (0xFF ^ ldstub(lp));
}

#else	/* lint */

	ENTRY(ulock_try)
	sacf	AC_ACCESS
	lghi	%r0,1
	sar	%a2,%r0
	ts	0(%r2)
	sacf	AC_PRIMARY
	jnz	0f
	membar
	lghi	%r2,1
	br	%r14
0:
	membar
	lghi	%r2,0
	br	%r14
	SET_SIZE(ulock_try)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ulock_clear.                                      */
/*                                                                  */
/* Function	- Clear a lock in user space.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
ulock_clear(lock_t *lp)
{
	membar_exit();
	*lp = 0;
}

#else	/* lint */

	ENTRY(ulock_clear)
	sacf	AC_ACCESS
	lghi	%r0,1
	sar	%a2,%r0
	mvi 	0(%r2),0
	sacf	AC_PRIMARY
	br	%r14
	SET_SIZE(ulock_clear)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lock_set_spl.                                     */
/*                                                                  */
/* Function	- Sets pil to new_pil, grabs lp, stores old pil in  */
/*		  *old_pil_addr.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
void
lock_set_spl(lock_t *lp, int new_pil, u_short *old_pil_addr)
{
	extern int splr(int);
	extern void lock_set_spl_spin(lock_t *, int, u_short *, int);
	int old_pil;

	old_pil = splr(new_pil);
	if (!lock_try(lp)) {
		lock_set_spl_spin(lp, new_pil, old_pil_addr, old_pil);
	} else {
		*old_pil_addr = (u_short)old_pil;
		membar_enter();
	}
}

#else	/* lint */

	ENTRY(lock_set_spl)
	stmg	%r6,%r15,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)
	lgr	%r6,%r2			// Save
	lgr	%r7,%r3
	lgr	%r8,%r4
	lgr	%r2,%r3			// Get new priority level
	brasl	%r14,splr
	lgfr	%r5,%r2			// Save old pil value
	lgr	%r2,%r6
	lgr	%r3,%r7
	lgr	%r4,%r8
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r15,48(%r15)

	ts	0(%r2)
	jgnz	lock_set_spl_spin

	sth	%r5,0(%r4)
	mc	MC_LOCK_SET_SPL,0
	br	%r14
	SET_SIZE(lock_set_spl)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lock_clear_splx.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
lock_clear_splx(lock_t *lp, ushort s)
{
	extern void splx(int);

	lock_clear(lp);
	splx(s);
}

#else	/* lint */

	ENTRY(lock_clear_splx)
	lg	%r1,__LC_CPU			// Get CPU pointer
	mvi 	0(%r2),0			// Clear lock
	lgf	%r4,CPU_BASE_SPL(%r1)		// Get base SPL
	cr	%r3,%r4				// Is new greater than base?
	jnl	0f				// Yes... Use it

	lgr	%r3,%r4				// Use base to set pri
0:
	lgr	%r2,%r3				// Load PIL
	jg	splx				// Go set it
	SET_SIZE(lock_clear_splx)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- unlock_hres_lock.                                 */
/*                                                                  */
/* Function	- Unlock the hres clock lock.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
unlock_hres_lock(lock_t *lp)
{
	lock_clear(lp);
}

#else	/* lint */

	ENTRY(unlock_hres_lock)
	lgf 	%r0,0(%r2)		// Get current lock
0:
	lgr	%r1,%r0			// Transfer
	aghi	%r1,1			// Update the counter (& reset latch)
	cs 	%r0,%r1,0(%r2)		// Clear the lock
	jnz	0b			// Try again it if changed
	br	%r14			// Return
	SET_SIZE(unlock_hres_lock)

#endif	/* lint */


/*========================= End of Function ========================*/

/********************************************************************
 * mutex_enter() and mutex_exit().
 * 
 * These routines handle the simple cases of mutex_enter() (adaptive
 * lock, not held) and mutex_exit() (adaptive lock, held, no waiters).
 * If anything complicated is going on we punt to mutex_vector_enter().
 *
 * mutex_tryenter() is similar to mutex_enter() but returns zero if
 * the lock cannot be acquired, nonzero on success.
 *
 * If mutex_exit() gets preempted in the window between checking waiters
 * and clearing the lock, we can miss wakeups.  Disabling preemption
 * in the mutex code is prohibitively expensive, so instead we detect
 * mutex preemption by examining the trapped PC in the interrupt path.
 * If we interrupt a thread in mutex_exit() that has not yet cleared
 * the lock, pil_interrupt() resets its PC back to the beginning of
 * mutex_exit() so it will check again for waiters when it resumes.
 *
 * The lockstat code below is activated when the lockstat driver
 * calls lockstat_hot_patch() to hot-patch the kernel mutex code.
 * Note that we don't need to test lockstat_event_mask here -- we won't
 * patch this code in unless we're gathering ADAPTIVE_HOLD lockstats.
 *
 *******************************************************************/
 
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mutex_enter.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/


#if defined (lint)

/* ARGSUSED */
void
mutex_enter(kmutex_t *lp)
{}

#else
	ENTRY(mutex_enter)

	GET_THR(1)				// Get thread pointer
	
	lghi	%r0,0
	csg	%r0,%r1,0(%r2)
	jgnz 	mutex_vector_enter
	
	mc	MC_MX_ENTER,0
	br	%r14
	SET_SIZE(mutex_enter)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mutex_tryenter.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/


#if defined (lint)

/* ARGSUSED */
int
mutex_tryenter(kmutex_t *lp)
{ return (0); }

#else
	ENTRY(mutex_tryenter)

	GET_THR(1)				// Get thread pointer

	lghi	%r0,0
	csg	%r0,%r1,0(%r2)
	jgnz 	mutex_vector_tryenter
	
	lghi	%r2,1				// Indicate success
	br	%r14
	SET_SIZE(mutex_tryenter)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mutex_adaptive_tryenter.                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/


#if defined (lint)

/* ARGSUSED */
int
mutex_adaptive_tryenter(kmutex_t *lp)
{ return (0); }

#else
	ENTRY(mutex_adaptive_tryenter)

	GET_THR(1)				// Get thread pointer

	lghi	%r0,0
	csg	%r0,%r1,0(%r2)
	jnz 	1f
	
	lghi	%r2,1				// Indicate success
	br	%r14

1:	
	lghi	%r2,0
	br	%r14
	SET_SIZE(mutex_adaptive_tryenter)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mutex_exit.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/


#if defined (lint)

/* ARGSUSED */
void
mutex_exit(kmutex_t *lp)
{}

#else

	ENTRY(mutex_exit)

	GET_THR(3)			// Get thread pointer

	lghi	%r1,0
	csg	%r3,%r1,0(%r2)		// Do we own the lock
	bzr	%r14			// Yes... we're done
mutex_exit_critical_start:		// If we are interrupted we restart from top
	jg	mutex_vector_exit
.mutex_exit_critical_end:
	SET_SIZE(mutex_exit)

	.section ".rodata"
	.align	8
mutex_exit_critical_size: 
	.quad (.mutex_exit_critical_end - mutex_exit_critical_start)
	.section ".text"

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mutex_owner_running.                              */
/*                                                                  */
/* Function	- Return the CPU if we are the owner of the mutex.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined (lint)

/* ARGSUSED */
void *
mutex_owner_running(mutex_impl_t *lp)
{ return (NULL); }

#else

	ENTRY(mutex_owner_running)
mutex_owner_critical_start:		// If interrupted restart here
	lg	%r3,0(%r2)		// Get the owner field
	lghi	%r4,MUTEX_THREAD	// Get mask
	ngr 	%r3,%r4			// Remove any waiter's bit
	jz	1f			// Go if none

	lg	%r4,T_CPU(%r3)		// Get owner->t_cpu
	lg	%r5,CPU_THREAD(%r4)	// Get owner->t_cpu->cpu_thread
.mutex_owner_critical_end:		// for pil_interrupt() hook
	cgr	%r5,%r3			// Owner == running thread?
	jz 	2f  			// Yes... Skip
1:
	lghi	%r2,0			// Set as not owner
	br	%r14
2:
	lgr	%r2,%r4			// Return CPU
	br	%r14			// Back to caller
	SET_SIZE(mutex_owner_running)

	.section ".rodata"
	.align	8
mutex_owner_running_critical_size: 
	.quad (.mutex_owner_critical_end - mutex_owner_critical_start)
	.section ".text"

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mutex_delay_default.                              */
/*                                                                  */
/* Function	- Spin for a few hundred processor cycles & return  */
/*		                               		 	    */
/*------------------------------------------------------------------*/


#if defined(lint)

void
mutex_delay_default(void)
{}

#else	/* lint */

	ENTRY(mutex_delay_default)
	lghi	%r0,72
	brctg	%r0,0
	br	%r14
	SET_SIZE(mutex_delay_default)

#endif  /* lint */

/*========================= End of Function ========================*/

/********************************************************************
 * rw_enter() and rw_exit().
 * 
 * These routines handle the simple cases of rw_enter (write-locking an unheld
 * lock or read-locking a lock that's neither write-locked nor write-wanted)
 * and rw_exit (no waiters or not the last reader).  If anything complicated
 * is going on we punt to rw_enter_sleep() and rw_exit_wakeup(), respectively.
 * 
 *******************************************************************/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- rw_enter.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
void
rw_enter(krwlock_t *lp, krw_t rw)
{}

#else

	ENTRY(rw_enter)

	GET_THR(5)				// Get thread pointer

	lg	%r1,0(%r2)			// Get current lock value
	cghi	%r3,RW_WRITER			// Is this a writer request?
	je	.Lrwrte				// Yes... Go deal with it

	lgf	%r0,T_KPRI_REQ(%r5)		// Get current priority
	aghi	%r0,1				// Increment
	st	%r0,T_KPRI_REQ(%r5)		// Save new kpri
1:
	tmll	%r1,RW_WRITE_CLAIMED		// Check if locked/wanted for write
	jgnz 	rw_enter_sleep			// It is... Go and sleep

	lgr	%r0,%r1   			// Copy old lock value
	aghi	%r1,RW_READ_LOCK		// Set new lock value
	csg	%r0,%r1,0(%r2)			// Try and get lock
	lgr	%r1,%r0				// Copy existing value
	jnz	1b            			// If changed... go try again

	mc	MC_RW_RE_LOCK,0			// Trigger probe if enabled
	br	%r14				// Return to caller

.Lrwrte:
	lghi	%r0,0				// Get unlocked value
	lgr	%r1,%r5				// Copy thread pointer
	oill	%r1,RW_WRITE_LOCKED		// Set lock value
	csg	%r0,%r1,0(%r2)			// Obtain lock
	jgnz	rw_enter_sleep			// Go sleep if not got

	mc	MC_RW_WE_LOCK,0			// Trigger probe if enabled
	br	%r14				// Return to caller
	SET_SIZE(rw_enter)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- rw_exit.                                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
void
rw_exit(krwlock_t *lp)
{}

#else
	
	ENTRY(rw_exit)

	GET_THR(3)				// Get thread pointer

	lg	%r1,0(%r2)			// Get old lock value
	lgr	%r0,%r1				// Copy
	cghi	%r1,RW_READ_LOCK		// Single reader, no writers?
	jne	.Lrwwrtx			// No... Go check write
	
	lghi	%r0,0				// New lock value

.Lrwrdx:
	csg	%r1,%r0,0(%r2)			// Try and clear lock
	jgnz	rw_exit_wakeup			// Go wake if not cleared

	lgf	%r1,T_KPRI_REQ(%r3)		// Get current priority
	aghi	%r1,-1				// Decrement
	st	%r1,T_KPRI_REQ(%r3)		// Save new kpri

	mc	MC_RW_RX_LOCK,0			// Trigger probe if enabled
	br	%r14				// Return to caller

.Lrwwrtx:
	lgr	%r0,%r1				// Copy current value
	tmll	%r1,RW_WRITE_LOCKED		// Are we a writer?
	jo 	.Lrwwtx				// Yes... Go deal with it

	aghi	%r0,-RW_READ_LOCK		// Single reader, no writers?
	cghi	%r0,RW_READ_LOCK		// Are there any readers left?
	je	.Lrwrdx				// Not last reader, safe to drop
	jg	rw_exit_wakeup			// Last reader with waiters

.Lrwwtx:
	lghi	%r0,0				// Get lock clear value
	lgr	%r1,%r3				// Get owner value
	oill	%r1,RW_WRITE_LOCKED		// Form expected value
	csg	%r1,%r0,0(%r2)			// Try and clear lock
	jgnz	rw_exit_wakeup			// Go wake if not cleared

	mc	MC_RW_WX_LOCK,0			// Trigger probe if enabled
	br	%r14				// Return to caller
	SET_SIZE(rw_exit)

#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- lockstat_hot_patch.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
lockstat_hot_patch(void)
{}

#else

	ENTRY(lockstat_hot_patch)
	stctg	%c8,%c8,48(%r15)
	xi	55(%r15),0x01		// Flip the monitor mask
	lctlg	%c8,%c8,48(%r15)
	br	%r14
	SET_SIZE(lockstat_hot_patch)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- thread_onproc.                                    */
/*                                                                  */
/* Function	- Set thread in onproc state for the specified CPU. */
/*		  Also set the thread lock pointer to the CPU's     */
/*		  onproc lock. Since the new lock isn't held, the   */
/*		  store ordering is important. If not done in 	    */
/*		  assembler, the compiler could reorder the stores. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
thread_onproc(kthread_id_t t, cpu_t *cp)
{
	t->t_state = TS_ONPROC;
	t->t_lockp = &cp->cpu_thread_lock;
}

#else	/* lint */

	ENTRY(thread_onproc)
	lghi	%r0,TS_ONPROC
	st	%r0,T_STATE(%r2)
	la	%r1,CPU_THREAD_LOCK(%r3)
	stg	%r1,T_LOCKP(%r2)
	br	%r14
	SET_SIZE(thread_onproc)

#endif	/* lint */

/*========================= End of Function ========================*/
