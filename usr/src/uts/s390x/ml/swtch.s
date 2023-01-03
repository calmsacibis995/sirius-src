/*------------------------------------------------------------------*/
/* 								    */
/* Name        - swtch.c    					    */
/* 								    */
/* Function    - Context switching routines.                        */
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

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

/*
 * Process switching routines.
 */

#if !defined(lint)
# include "assym.h"
#else	/* lint */
# include <sys/thread.h>
#endif	/* lint */

#include <sys/param.h>
#include <sys/machthread.h>
#include <sys/asm_linkage.h>
#include <sys/pcb.h>
#include <sys/privregs.h>
#include <sys/vtrace.h>
#include <vm/hat_s390x.h>

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

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- resume.                                           */
/*                                                                  */
/* Function	- A thread can only run on one processor at a time. */
/*		  There exists a window on MPs where the current    */
/*		  thread on one processor is capable of being 	    */
/*		  dispatched by another processor. Some overlap     */
/*		  between outgoing and incoming threads can happen  */
/*		  when they are the same thread. in this case where */
/*		  the threads are the same, resume() on one 	    */
/*		  processor will spin on the incoming thread until  */
/*		  resume() on the other processor has finished with */
/* 		  the outgoing thread.				    */
/*                                                                  */
/*		  The MMU context changes when the resuming thread  */
/*		  resides in a different process.  Kernel threads   */
/*		  are known by resume to reside in process 0.	    */
/*		  The MMU context, therefore, only changes when     */
/*		  resuming a thread in a process different from     */
/*		  curproc.					    */
/*                                                                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * resume(kthread_id_t)
 *
 */

#if defined(lint)

/* ARGSUSED */
void
resume(kthread_id_t t)
{}

#else	/* lint */

	ENTRY(resume)
	stmg	%r2,%r15,16(%r15)
	lgr	%r9,%r15
	lgr	%r8,%r14
	lgr	%r7,%r2
	aghi	%r15,-SA(MINFRAME)
	stg	%r9,0(%r15)
	brasl	%r14,__dtrace_probe___sched_off__cpu 

	GET_THR(6)

	stg	%r8,T_PC(%r6)			// Save return address
	stg	%r9,T_SP(%r6)			// Save stack pointer 
	lg	%r8,T_LWP(%r6)			// Get LWP pointer
	lg	%r12,T_CPU(%r6)			// Get CPU pointer
	lg	%r4,T_PROCP(%r6)		// Get old Proc pointer

	larl	%r1,kas				// Get A(KAS)
	cg	%r1,P_AS(%r4)			// Is this a kernel thread
	jne	.res_user			// No... Handle user thread
	
	//
	// Kernel thread
	// 
	lg	%r10,T_STACK(%r6)		// Get stack pointer
	lg	%r11,T_CTX(%r6)			// Get context pointer
	lay	%r2,-FPSAVESZ(%r10)    		// Point at save area
	bras	%r14,fp_save			// Go save FP state
	j	.skip_user

	//
	// User thread
	//
.res_user:
	lg	%r10,T_SP(%r6)			// Get stack
	lg	%r11,T_CTX(%r6)			// Get context pointer
	lg	%r2,LWP_FPU(%r8)		// Point at save area
	brasl	%r14,fp_save			// Save FP state

	//
	// Perform context switch callback if set.
	// This handles coprocessor state saving.
	// r12 = cpu ptr
	// r11 = ctx pointer
	//
.skip_user:
	ltgr	%r11,%r11			// CTX zero?
	jz	.noctxsave			// Yes... Skip CTX save

	lgr	%r2,%r6				// Set savectx argument
	brasl	%r14,savectx			// Go save context
	
	//
	// Temporarily switch to idle thread's stack
	//
.noctxsave:
	lg	%r4,T_PROCP(%r7)		// Get new Proc pointer
	lg	%r13,T_PROCP(%r6)		// Get old curproc
	lg	%r2,CPU_IDLE_THREAD(%r12)	// Get idle thread pointer
	lg	%r15,T_SP(%r2)			// Get onto idle thread stack
	aghi	%r15,-SA(MINFRAME)		// Get some room to work
	

	//
	// Set the idle thread as the current thread
	//
	SET_THR(%r2)				// Make idle thread current

	//
	// Clear and unlock previous thread's t_lock
	// to allow it to be dispatched by another processor.
	//
	mvi 	T_LOCK(%r6),0			// Clear tp->t_lock

	//
	// IMPORTANT: Registers at this point must be:
	//	%r7  = new thread
	//	%r12 = cpu pointer
	//	%r13 = old proc pointer
	//	%r4  = new proc pointer
	//	%r6  = old thread
	//	
	// Here we are in the idle thread, have dropped the old thread.
	// 
	ALTENTRY(_resume_from_idle)

	cgr	%r13,%r4		// Same process?
	je	.same_proc		// Yes... Skip

	lg	%r8,P_AS(%r4)		// Load p->p_as
	lg	%r9,A_HAT(%r8)		// (p->p_as)->a_hat

#if 0
	//
	// update cpusran field
	//
	lgf	%r8,CPU_ID(%r12)
	la	%r9,HAT_CPUS(%r9)

	CPU_INDEXTOSET(%r9, %r8, %r0)	

	lg	%r10,0(%r9)		// r10 = cpusran field
	lghi	%r0,1	
	sllg	%r0,%r0,0(%r8)		// r0 = bit for this CPU
	lgr	%r11,%r0
	ngr	%r0,%r10		// Bit already set?
	jnz	.skip_up		// Yes... Skip update

	lgr	%r0,%r11
	og	%r0,0(%r9)

.retryup:
	csg	%r11,%r0,0(%r9)
	jnz	.retryup
#endif

	//
	// Switch to different address space.
	//
.skip_up:
	stnsm	__LC_SCRATCH,0x04	// Disable interrupts
	lg	%r1,P_AS(%r4)		// Get address space pointer
	lg	%r2,A_HAT(%r1)		// Get HAT pointer
	brasl	%r14,hat_switch		// Switch HATs
	ssm	__LC_SCRATCH		// Re-enable (any) interrupts
	
	//
	// spin until dispatched thread's mutex has
	// been unlocked. this mutex is unlocked when
	// it becomes safe for the thread to run.
	// 
.same_proc:
	ts	T_LOCK(%r7)		// Attempt to lock
	jnz	.same_proc		// If locked already wait again

	lg	%r13,T_PC(%r7)		// Get PC 
	cg	%r12,T_CPU(%r7)		// Same processor?
	je	.same_cpu 		// Yes

	lg	%r1,CPU_STATS_SYS_CPUMIGRATE(%r12)
	aghi	%r1,1
	stg	%r1,CPU_STATS_SYS_CPUMIGRATE(%r12)
	stg	%r12,T_CPU(%r7)

	//
	// Fix CPU structure to indicate new running thread.
	// Set pointer in new thread to the CPU structure.
	//
.same_cpu:

	SET_THR(%r7)			// Set thread register & set CPU's thr ptr

	lg	%r3,T_LWP(%r7)		// Get associated LWP
	stg	%r3,CPU_LWP(%r12)	// Set CPU's LWP pointer
	lghi	%r0,0			
	stg	%r0,CPU_MPCB(%r12)	// Clear MPCB pointer
	ltgr	%r3,%r3			// Any LWP 
	jz	.nolwp			// No... Skip

	//
	// user thread
	// 	r3 = lwp
	// 	r7 = new thread
	//
	lg	%r2,T_STACK(%r7)	// Get stack
	stg	%r2,CPU_MPCB(%r12)	// Set MPCB pointer

	//
	// Switch to new thread's stack
	//
	lg	%r15,T_SP(%r7)		// Get stack pointer
	aghi	%r15,-SA(MINFRAME)

	//
	// Check if PER trace wanted and set control registers
	//
	cli	PCB_MASK(%r3),0		// Any tracing active?
	jz	.notrc			// No... Skip

	tm 	PCB_STEP(%r3),STEP_ACTIVE 
	jo	.stepreg		// Load c9/c10/c11 for stepping
	
	lctlg	%c9,%r11,PCB_MASK(%r3) 	// Set PER registers
	j	.notrc

.stepreg:
	lctlg	%c9,%r9,PCB_MASK(%r3) 	// Set PER registers
	lctlg	%c10,%c10,PCB_TRACEPC(%r3)
	lctlg	%c11,%c11,PCB_TRACEPC(%r3)

	//
	// Restore resuming thread's floating-point regs
	//
.notrc:
	lg	%r5,T_CPU(%r7)		// Get CPU pointer
	lg	%r8,T_CTX(%r7)		// Get CTX pointer
	lg	%r2,LWP_FPU(%r3)	// Get FPR pointer
	ltgr	%r2,%r2			// If no FPU saved
	jz	.restctx		// Skip restore

	bras	%r14,fp_restore		// Restore FP registers
	j	.restctx		// Go restore CTX if needed

	//
	// kernel thread
	// 	r7 = new thread
	//

	// Switch to new thread's stack
	//
.nolwp:
	lg	%r15,T_SP(%r7)		//  Get stack pointer
	aghi	%r15,-SA(MINFRAME)

	//
	// Restore resuming thread's floating-point regs
	//
	lg	%r10,T_STACK(%r7)
	lay	%r2,-FPSAVESZ(%r10)	// Address FP save area
	bras	%r14,fp_restore		// Restore FP registers

	//
	// Restore resuming thread's context
	// 	r8 = ctx ptr
	//
.restctx:
	ltgr	%r8,%r8			// Any context to restore?
	jz	.norestctx		// No... Skip the restore

	lgr	%r2,%r7			// Get thread pointer
	brasl	%r14,restorectx		// Restore context

	//
	// Set priority as low as possible, blocking all interrupt threads
	// that may be active.
	//
.norestctx:
	stnsm	__LC_SCRATCH,0xfc
	brasl	%r14,set_base_spl
	lgf	%r2,CPU_BASE_SPL(%r12)
	brasl	%r14,setspl
	stosm	__LC_SCRATCH,0x07

	//
	// If we are resuming an interrupt thread, store a starting timestamp
	// in the thread structure.
	//
	llgh	%r2,T_FLAGS(%r7)
	tmll	%r2,T_INTR_THREAD
	jz	.notinthr

0:
	brasl	%r14,gettick_counter
	lg	%r0,T_INTR_START(%r7)
	csg	%r0,%r2,T_INTR_START(%r7)
	jnz	0b

.notinthr:
	brasl	%r14,__dtrace_probe___sched_on__cpu
	lgr	%r14,%r13		// Restore T_PC
	aghi	%r15,SA(MINFRAME)
	lmg	%r2,%r13,16(%r15)
#ifdef DEBUG
	tracg	%r0,%r15,__LC_RSM_TRACE
#endif
	br	%r14
	SET_SIZE(_resume_from_idle)
	SET_SIZE(resume)

/*========================= End of Function ========================*/

#endif	/* lint */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- fp_save.                                          */
/*                                                                  */
/* Function	- Save the floating point registers.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

fp_save:
	std	%f0,0(%r2)			// Save FP 0-15
	std	%f1,8(%r2)
	std	%f2,16(%r2)
	std	%f3,24(%r2)
	std	%f4,32(%r2)
	std	%f5,40(%r2)
	std	%f6,48(%r2)
	std	%f7,56(%r2)
	std	%f8,64(%r2)			
	std	%f9,72(%r2)
	std	%f10,80(%r2)
	std	%f11,88(%r2)
	std	%f12,96(%r2)
	std	%f13,104(%r2)
	std	%f14,112(%r2)
	std	%f15,120(%r2)
	stfpc	128(%r2)			// Save FPC
	br	%r14


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- fp_restore.                                       */
/*                                                                  */
/* Function	- Restore the floating-point registers.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

fp_restore:
	ld	%f0,0(%r2)			// Restore FP 0-15
	ld	%f1,8(%r2)
	ld	%f2,16(%r2)
	ld	%f3,24(%r2)
	ld	%f4,32(%r2)
	ld	%f5,40(%r2)
	ld	%f6,48(%r2)
	ld	%f7,56(%r2)
	ld	%f8,64(%r2)			
	ld	%f9,72(%r2)
	ld	%f10,80(%r2)
	ld	%f11,88(%r2)
	ld	%f12,96(%r2)
	ld	%f13,104(%r2)
	ld	%f14,112(%r2)
	ld	%f15,120(%r2)
	lgf	%r1,128(%r2)
	sfpc	%r1,0				// Restore FPC
	br	%r14

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- resume_from_zombie.                               */
/*                                                                  */
/* Function	- resume_from_zombie() is the same as resume except */
/*		  the calling thread is a zombie and must be put on */
/*		  the deathrow list after the CPU is off the stack. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
void
resume_from_zombie(kthread_id_t t)
{}

#else	/* lint */

	ENTRY(resume_from_zombie)
	stmg	%r2,%r15,16(%r15)
	lgr	%r14,%r15
	lgr	%r10,%r2			// Save thread to switch to
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)
	brasl	%r14,__dtrace_probe___sched_off__cpu 

	GET_THR(6)

	lmg	%r2,%r5,SA(MINFRAME+16)(%r15)
	lg	%r12,T_CPU(%r6)			// CPU pointer
	lg	%r13,T_PROCP(%r6)		// Old procp for mmu CTX

	//
	// Temporarily switch to the idle thread's stack so that
	// the zombie thread's stack can be reclaimed by the reaper.
	//
	lg	%r7,CPU_IDLE_THREAD(%r12)	// Idle thread pointer
	lg	%r15,T_SP(%r7)			// Get idle thread stack
	aghi	%r15,-SA(MINFRAME)		// Make room for stack stuff

	//
	// Set the idle thread as the current thread.
	// Put the zombie on death-row.
	// 	

	SET_THR(%r7)

	lgr	%r2,%r6				// Arg to reapq_add
	brasl	%r14,reapq_add			// reapq_add(old_thread)

	//
	// resume_from_idle args:
	//	%r7  = new thread
	//	%r12 = cpu
	//	%r13 = old proc
	//	%r6  = old thread
	//	%r4  = new proc

	lgr	%r7,%r10			// Restore thread to switch to
	lg	%r4,T_PROCP(%r7)		// Get new proc pointer
	j	_resume_from_idle		// finish job of resume
	SET_SIZE(resume_from_zombie)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- resume_from_intr.                                 */
/*                                                                  */
/* Function	- resume_from_intr() is called when the thread being*/ 
/*		  resumed was not passivated by resume (e.g. was    */
/*		  interrupted).  This means that the resume lock is */
/*		  already held and that a restore context is not    */
/*		  needed. Also, the MMU context is not changed on   */
/*		  the resume in this case.			    */
/*		                              		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

/* ARGSUSED */
void
resume_from_intr(kthread_id_t t)
{}

#else	/* lint */

	ENTRY(resume_from_intr)
	stmg	%r6,%r15,48(%r15)
	lgr	%r11,%r15
	lgr	%r10,%r2
	aghi	%r15,-SA(MINFRAME)
	stg	%r11,0(%r15)

	GET_THR(6)

	stg	%r11,T_SP(%r6)		// Save SP
	stg	%r14,T_PC(%r6)		// Save return address
	lg	%r7,T_PC(%r10)		// Resuming thread's PC
	lg	%r8,T_CPU(%r6)		// Get *cpu

	//
	// Fix CPU structure to indicate new running thread.
	// The pinned thread we're resuming already has the CPU pointer set.
	//
	lgr	%r12,%r6		// Save old thread
	stg	%r10,CPU_THREAD(%r8)	// Set CPU's thread pointer

	//
	// Switch to new thread's stack
	//
	lg	%r15,T_SP(%r10)		// Get resuming thread's SP
	aghi	%r15,-SA(MINFRAME)	// Make some room for any calls
	mvi	T_LOCK(%r12),0		// Clear intr thread's tp->t_lock

	//
	// If we are resuming an interrupt thread, store a timestamp in the
	// thread structure.
	//
	llgh	%r0,T_FLAGS(%r10)
	llgc	%r9,CPU_KPRUNRUN(%r8)
	tmll	%r0,T_INTR_THREAD
	jo	0f

	//
	// We're resuming a non-interrupt thread.
	// Clear CPU_INTRCNT and check if cpu_kprunrun set?
	//
	ltgr	%r9,%r9				// Is KCPURUN set?
	jnz	3f
	mvi	CPU_INTRCNT(%r8),0		// Clear intrcount
1:
	lg	%r13,T_PC(%r10)
	lgf	%r2,CPU_BASE_SPL(%r8)		// Get base spl
	brasl	%r14,setspl			// Set as our interrupt level
	aghi	%r15,SA(MINFRAME)		// sp := t->t_sp
	br	%r13
0:
	//
	// We're an interrupt thread. Update t_intr_start and cpu_intrcnt
	//
	lg	%r0,T_INTR_START(%r10)
2:
	brasl	%r14,gettick_counter
	csg	%r0,%r2,T_INTR_START(%r10)
	jnz	2b

	lg	%r4,T_INTR(%r10)
	ltgr	%r4,%r4
	jz	1b

	mvi	CPU_INTRCNT(%r8),0		// Clear intrcount
	j	1b
3:
	//
	// We're a non-interrupt thread and cpu_kprunrun is set. call kpreempt.
	//
	lghi	%r2,KPREEMPT_SYNC
	brasl	%r14,kpreempt
	j	1b
	SET_SIZE(resume_from_intr)

#endif /* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- thread_start.                                     */
/*                                                                  */
/* Function	- The current register setup was crafted by 	    */
/*		  thread_run() to contain an address of a procedure */
/*		  (in register %r13), and its args in registers	%r2 */
/* 		  through %r6. a stack trace of this thread will    */
/*		  show the procedure that thread_start() invoked at */
/*		  the bottom of the stack. An exit routine is 	    */
/* 		  called when started thread returns from its 	    */
/*		  called procedure.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
thread_start(void)
{}

#else	/* lint */

	ENTRY(thread_start)
	lmg	%r2,%r13,16(%r15)
	basr	%r14,%r13
	brasl 	%r14,thread_exit
	SET_SIZE(thread_start)

#endif	/* lint */

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- set_thread.                                       */
/*                                                                  */
/* Function	- Set the current thread pointer.           	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if defined(lint)

void
set_threadp(void)
{}

#else	/* lint */

	ENTRY(set_threadp)
	stmg	%r2,%r6,16(%r15)
	lgr	%r3,%r2			// Get new pointer
0:		
	lghi	%r0,6			// Get function code
	la	%r1,__LC_CPU		// Point at lock
	lg	%r4,__LC_CPU		// Get A(CPU)
	lg	%r2,CPU_THREAD(%r4)	// Get current thread pointer
	plo	%r2,CPU_THREAD(%r4),0,0 // Go swap thread pointer
	jnz	0b			// Try again if we failed

	lmg	%r2,%r6,16(%r15)
	br	%r14
	SET_SIZE(thread_start)

#endif	/* lint */

/*========================= End of Function ========================*/
