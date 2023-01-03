/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License                  
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *                                                              
 * Copyright 2008 Sine Nomine Associates.                        
 * All rights reserved.                                           
 * Use is subject to license terms.                                
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#if !defined(lint)
#include "assym.h"
#endif	/* lint */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/asm_linkage.h>
#include <sys/vtrace.h>
#include <sys/machthread.h>
#include <sys/clock.h>
#include <sys/privregs.h>

/*
 * LOFAULT_SET : Flag set by kzero and kcopy to indicate that t_lofault
 * handler was set
 *
 * XXX
 * Is this needed?  Only (b|k)copy use it.  If needed, should all the
 * rest use it too?
 * XXX
 */
#define	LOFAULT_SET 1


/*
 * Copy a block of storage, returning an error code if `from' or
 * `to' takes a kernel pagefault which cannot be resolved.
 * Returns errno value on pagefault error, 0 if all ok
 */



#if defined(lint)

/* ARGSUSED */
int
kcopy(const void *from, void *to, size_t count)
{ return(0); }

#else	/* lint */

	.section ".text"
	.align	4

	ENTRY(kcopy)

	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	lg	%r7,T_LOFAULT(%r9)
	oill	%r7,LOFAULT_SET
	larl	%r8,.copyerr
	stg	%r8,T_LOFAULT(%r9)
	j	.do_copy			// common code

/*
 * We got here because of a fault during kcopy.
 * Errno value is in %r2.
 */
.copyerr:
	// The kcopy() *always* sets a t_lofault handler and it ORs LOFAULT_SET
	// into %r7 to indicate it has set t_lofault handler. Need to clear
	// LOFAULT_SET flag before restoring the error handler.

	nill	%r7,65535-LOFAULT_SET
	stg	%r7,T_LOFAULT(%r9)		// Restore old fault handler
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14

	SET_SIZE(kcopy)
#endif	/* lint */


/*
 * Copy a block of storage - must not overlap (from + len <= to).
 */
#if defined(lint)

/* ARGSUSED */
void
bcopy(const void *from, void *to, size_t count)
{}

#else	/* lint */

	ENTRY(bcopy)

	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	lghi	%r7,0			// Indicate not fault handler set

.do_copy:
	ltgr	%r4,%r4			// len == 0?
	jz	.cpdone			// Don't do anything

	cghi	%r4,256			// If will fit in one mvc
	jle	1f			// Just go use it

	lgr	%r0,%r3			// Get A(Source)
	lgr	%r1,%r4			// Copy length
	lgr	%r3,%r4			// ....
	lghi	%r12,0
0:
	mvcle	%r0,%r2,0(%r12)
	jo	0b
	j	.cpdone

1:	
	aghi	%r4,-1
	larl	%r13,.Lmvc		// Address MVC instruction
	ex	%r4,0(%r13)		// mvc 0(L'(R4),R2),0(R3)
	
.cpdone:
	// Restore t_lofault handler, if came here from kcopy().
	ltgr	%r7,%r7			// T_LOFAULT set?
	jz	2f			// No... Skip

	nill	%r7,65535-LOFAULT_SET	// Turn off flag
	stg	%r7,T_LOFAULT(%r9)	// Restore old handler
2:
	lghi	%r2,0			// Set the all clear
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14

.Lmvc:	mvc	0(1,%r3),0(%r2)		// Copy src->dst

	SET_SIZE(bcopy)

#endif	/* lint */

/*
 * Block copy with possibly overlapped operands.
 */

#if defined(lint)

/*ARGSUSED*/
void
ovbcopy(const void *from, void *to, size_t count)
{}

#else	/* lint */

	ENTRY(ovbcopy)
	ltgr	%r4,%r4			// Count > 0?
	bzr	%r14			// No... just return

	lgr	%r0,%r2			// Copy "from"
	sgr	%r0,%r3			// Difference of "from" and "to"
	jnm	0f			// Skip if not negative
	lpgr	%r0,%r0			// Make positive
0:
	cgr	%r4,%r0			// Size <= abs(diff)?
	jgnh	bcopy			// Yes... Use bcopy

	cgr	%r2,%r3			// Is from < to?
	jl	2f			// Yes... Copy backwards

	//
	// Copy forwards
	//
1:
	ic 	%r0,0(%r2)		// Get a byte
	aghi	%r2,1			// Bump from address
	stc	%r0,0(%r3)		// Save in to address
	aghi	%r3,1			// Bump to address
	brctg	%r4,1b			// Loop until done
	br	%r14			// Return

	//
	// Copy backwards
	//
2:
	aghi	%r4,-1			// Decrement count
	ic	%r0,0(%r4,%r2)		// Get byte at end of src
	stc	%r0,0(%r4,%r3)		// Save in to address
	ltgr	%r4,%r4			// Anything left?
	jnz	2b			// Yes... Loop until done

	br	%r14			// Return
	SET_SIZE(ovbcopy)

#endif	/* lint */

/*
 * hwblkpagecopy()
 *
 * Copies exactly one page.  This routine assumes the caller (ppcopy)
 * has already disabled kernel preemption.
 */
#ifdef lint
/*ARGSUSED*/
void
hwblkpagecopy(const void *src, void *dst)
{ }
#else /* lint */
	ENTRY(hwblkpagecopy)
	lghi	%r0,0
	mvpg	%r3,%r2
	br	%r14
	SET_SIZE(hwblkpagecopy)
#endif	/* lint */


/*
 * Transfer data to and from user space -
 * Note that these routines can cause faults
 * It is assumed that the kernel has nothing at
 * less than KERNELBASE in the virtual address space.
 *
 * Note that copyin(9F) and copyout(9F) are part of the
 * DDI/DKI which specifies that they return '-1' on "errors."
 *
 * Sigh.
 *
 * So there's two extremely similar routines - xcopyin() and xcopyout()
 * which return the errno that we've faithfully computed.  This
 * allows other callers (e.g. uiomove(9F)) to work correctly.
 * Given that these are used pretty heavily, we expand the calling
 * sequences inline for all flavours (rather than making wrappers).
 *
 */

/*
 * Copy user data to kernel space (copyOP/xcopyOP/copyOP_noerr)
 *
 * General theory of operation:
 *
 * Flow:
 *
 * If count == zero return zero.
 *
 * Fault handlers are invoked if we reference memory that has no
 * current mapping.  All forms share the same copyio_fault handler.
 * This routine handles fixing up the stack and general housecleaning.
 * Each copy operation has a simple fault handler that is then called
 * to do the work specific to the invidual operation.  The handler
 * for copyOP and xcopyOP are found at the end of individual function.
 * The handlers for xcopyOP_little are found at the end of xcopyin_little.
 * The handlers for copyOP_noerr are found at the end of copyin_noerr.
 */

/*
 * Copy kernel data to user space (copyout/xcopyout).
 */

#if defined(lint)

/*ARGSUSED*/
int
copyout(const void *kaddr, void *uaddr, size_t count)
{ return (0); }

#else	/* lint */

/*
 * We save the arguments in the following registers in case of a fault:
 * 	kaddr - %r9
 * 	uaddr - %r10
 * 	count - %r11
 */
#define	SAVE_SRC	%r6
#define	SAVE_DST	%r10
#define	SAVE_COUNT	%r11

#define	REAL_LOFAULT	%r8
#define	SAVED_LOFAULT	%r7

/*
 * Generic copyio fault handler.  This is the first line of defense when a 
 * fault occurs in (x)copyin/(x)copyout.  In order for this to function
 * properly, the value of the 'real' lofault handler should be in REAL_LOFAULT.
 * This allows us to share common code for all the flavors of the copy
 * operations, including the _noerr versions.
 *
 * Note that this function will restore the original input parameters before
 * calling REAL_LOFAULT.  So the real handler can vector to the appropriate
 * member of the t_copyop structure, if needed.
 */
	ENTRY(copyio_fault)
	stg	SAVED_LOFAULT,T_LOFAULT(%r9)
	lgr	%r0,%r2				// R0 contains errno
	lgr	%r2,SAVE_SRC
	lgr	%r3,SAVE_DST
	lgr	%r4,SAVE_COUNT
	br	REAL_LOFAULT
	SET_SIZE(copyio_fault)

	ENTRY(copyout)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	larl	REAL_LOFAULT,.copyout_err

.do_copyout:
	//
	// Check the length and bail if zero.
	//
	ltgr	%r4,%r4
	jnz	1f

	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	lghi	%r2,0
	br	%r14
1:
	lg	SAVED_LOFAULT,T_LOFAULT(%r9)
	larl	%r5,copyio_fault
	stg	%r5,T_LOFAULT(%r9)
	lgr	SAVE_SRC,%r2
	lgr	SAVE_DST,%r3
	lgr	SAVE_COUNT,%r4

	lghi	%r0,1				// Set for secondary mode
	sar	%a12,%r0
	lghi	%r0,0				// Set for primary mode
	sar	%a2,%r0
	lgr	%r12,%r3			// Copy "to" pointer
	lgr	%r13,%r4			// Copy length
	lgr	%r3,%r4				// ....
	sacf	AC_ACCESS
2:
	mvcle	%r12,%r2,0			// Copy kernel to user
	jo	2b				// Do until complete

	sacf	AC_PRIMARY			// Exit AR Mode
	stg	SAVED_LOFAULT,T_LOFAULT(%r9)	// Restore handler
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	lghi	%r2,0
	br	%r14
	
.copyout_err:
	sacf	AC_PRIMARY			// Exit AR mode
	stg	SAVED_LOFAULT,T_LOFAULT(%r9)	// Restore original handler
	lgr	%r1,%r9				// Save thread
	aghi	%r15,SA(MINFRAME)		// Reset frame
	lmg	%r6,%r14,48(%r15)		// Restore registers

	lg	%r1,T_COPYOPS(%r1)		// Get copyops
	ltgr	%r1,%r1				// Installed?
	jz	3f				// No...just return

	lg	%r1,CP_COPYOUT(%r1)		// Get copyout()
	br	%r1				// and go there
3:
	lghi	%r2,-1				// Set error
	br	%r14				// and return

	SET_SIZE(copyout)

#endif	/* lint */


#ifdef	lint

/*ARGSUSED*/
int
xcopyout(const void *kaddr, void *uaddr, size_t count)
{ return (0); }

#else	/* lint */

	ENTRY(xcopyout)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	larl	REAL_LOFAULT,.xcopyout_err
	j	.do_copyout

.xcopyout_err:
	sacf	AC_PRIMARY			// Exit AR mode
	stg	SAVED_LOFAULT,T_LOFAULT(%r9)	// Restore original handler
	lgr	%r1,%r9				// Save thread
	aghi	%r15,SA(MINFRAME)		// Reset frame
	lmg	%r6,%r14,48(%r15)		// Restore registers

	lg	%r1,T_COPYOPS(%r1)		// Get copyops
	ltgr	%r1,%r1				// Installed?
	jz	3f				// No...just return

	lg	%r1,CP_XCOPYOUT(%r1)		// Get xcopyout()
	br	%r1				// and go there
3:
	lgr	%r2,%r0				// Set error (from copyio_fault)
	br	%r14				// and return

	SET_SIZE(xcopyout)

#endif	/* lint */
	
/*
 * Copy user data to kernel space (copyin/xcopyin/xcopyin)
 */

#if defined(lint)

/*ARGSUSED*/
int
copyin(const void *uaddr, void *kaddr, size_t count)
{ return (0); }

#else	/* lint */

	ENTRY(copyin)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	larl	REAL_LOFAULT,.copyin_err

.do_copyin:
	//
	// Check the length and bail if zero.
	//
	ltgr	%r4,%r4
	jnz	1f

	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	lghi	%r2,0
	br	%r14
1:
	lg	SAVED_LOFAULT,T_LOFAULT(%r9)	// Get handler
	larl	%r5,copyio_fault		// Get our handler
	stg	%r5,T_LOFAULT(%r9)		// Plugh in
	lgr	SAVE_SRC,%r2
	lgr	SAVE_DST,%r3
	lgr	SAVE_COUNT,%r4

	lghi	%r0,1				// Set for secondary space
	sar	%a2,%r0				// Set ALET
	lghi	%r0,0				// Set for primary space
	sar	%a12,%r0			// Set ALET
	lgr	%r12,%r3			// Get "to" pointer
	lgr	%r13,%r4			// Copy length
	lgr	%r3,%r4				// ....
	sacf	AC_ACCESS			// Enter AR mode
2:
	mvcle	%r12,%r2,0			// Move from user to kernel
	jo	2b				// Keep going until complete

	sacf	AC_PRIMARY			// Exit AR mode
	stg	SAVED_LOFAULT,T_LOFAULT(%r9)	// Restore handler
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	lghi	%r2,0
	br	%r14
	
.copyin_err:
	sacf	AC_PRIMARY			// Exit AR mode
	stg	SAVED_LOFAULT,T_LOFAULT(%r9)	// Restore original handler
	lgr	%r1,%r9				// Save thread
	aghi	%r15,SA(MINFRAME)		// Reset frame
	lmg	%r6,%r14,48(%r15)		// Restore registers

	lg	%r1,T_COPYOPS(%r1)		// Get copyops
	ltgr	%r1,%r1				// Installed?
	jz	3f				// No...just return

	lg	%r1,CP_COPYIN(%r1)		// Get copyin()
	br	%r1				// and go there
3:
	lghi	%r2,-1				// Set error
	br	%r14				// and return

	SET_SIZE(copyin)

#endif	/* lint */

#ifdef	lint

/*ARGSUSED*/
int
xcopyin(const void *uaddr, void *kaddr, size_t count)
{ return (0); }

#else	/* lint */

	ENTRY(xcopyin)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	larl	REAL_LOFAULT,.xcopyin_err
	j	.do_copyin

.xcopyin_err:
	sacf	AC_PRIMARY			// Exit AR mode
	stg	SAVED_LOFAULT,T_LOFAULT(%r9)	// Restore original handler
	lgr	%r1,%r9				// Save thread
	aghi	%r15,SA(MINFRAME)		// Reset frame
	lmg	%r6,%r14,48(%r15)		// Restore registers

	lg	%r1,T_COPYOPS(%r1)		// Get copyops
	ltgr	%r1,%r1				// Installed?
	jz	3f				// No...just return

	lg	%r1,CP_XCOPYIN(%r1)		// Get xcopyin()
	br	%r1				// and go there
3:
	lgr	%r2,%r0				// Set error (from copyio_fault)
	br	%r14				// and return

	SET_SIZE(xcopyin)

#endif	/* lint */


/*
 * Copy a block of storage - must not overlap (from + len <= to).
 * No fault handler installed (to be called under on_fault())
 */
#if defined(lint)

/* ARGSUSED */
void
copyin_noerr(const void *ufrom, void *kto, size_t count)
{}

#else	/* lint */

	ENTRY(copyin_noerr)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	larl	REAL_LOFAULT,.copyio_noerr
	j	.do_copyin

.copyio_noerr:

	// XXX
	// Do we need to retore AR mode here?
	// What about registers?
	// What happens if there isn't a SAVED_LOFAULT handler?
	// XXX

	br	SAVED_LOFAULT
	SET_SIZE(copyin_noerr)

#endif /* lint */

/*
 * Copy a block of storage - must not overlap (from + len <= to).
 * No fault handler installed (to be called under on_fault())
 */

#if defined(lint)

/* ARGSUSED */
void
copyout_noerr(const void *kfrom, void *uto, size_t count)
{}

#else	/* lint */

	ENTRY(copyout_noerr)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	larl	REAL_LOFAULT,.copyio_noerr
	j	.do_copyout
	SET_SIZE(copyout_noerr)

#endif /* lint */

/*
 * Copy a null terminated string from one point to another in
 * the kernel address space.
 * NOTE - don't use %r7 in this routine as copy{in,out}str uses it.
 *
 * copystr(from, to, maxlength, lencopied)
 *	caddr_t from, to;
 *	u_int maxlength, *lencopied;
 */

#if defined(lint)

/* ARGSUSED */
int
copystr(const char *from, char *to, size_t maxlength, size_t *lencopied)
{ return(0); }

#else	/* lint */

	ENTRY(copystr)
	ltgr	%r4,%r4			// Check max length
	jz	4f			// If zero then set rc and leave
	jh	5f			// If positive then check string
	
	lghi	%r2,EFAULT		// Set in error
	br	%r14
5:
	cli	0(%r2),0		// Null string
	jne	0f
4:
	mvi	0(%r3),0		// Null terminate
	lghi	%r2,0			// Set return code
	ltgr	%r5,%r5			// Resulting count wanted?
	bzr	%r14			// No... Just return
	lghi	%r4,1			// Set length
	stg	%r4,0(%r5)		// Set lencopied
	br	%r14
0:
	lghi	%r0,0			// Set comparison character
	lgr	%r1,%r2 		// Copy from address
	agr 	%r1,%r4			// Point at potential end of string
6:	srst	%r1,%r2			// Locate end-of-string
	jo	6b			// Go until done

	lgr	%r0,%r1			// Copy start of string
	sgr	%r0,%r2			// Determine length of "from"
	aghi	%r0,1			// Adjust
	cgr	%r0,%r4			// Compare to maxlength
	jl	1f			// If less then use MOVST

	lgr	%r1,%r4			// Copy length
	lgr	%r0,%r3			// Copy "to" pointer
	lgr	%r3,%r4			// Get length
	ltgr	%r5,%r5			// Resulting count wanted?
	jz	2f			// No... Skip it
	stg	%r3,0(%r5)		// Set lencopied
2:
	mvcle	%r0,%r2,0		// Copy string
	jo	2b			// Keep copying until done

	lghi	%r2,0			// Set success value
	cli     0(%r0),0		// Last byte 0?
	ber	%r14			// Yes...return
	lghi	%r2,ENAMETOOLONG	// Set error value
	br	%r14
1:
	ltgr	%r5,%r5			// Resulting count wanted?
	jz	7f			// No... Skip
	stg	%r0,0(%r5)		// Set count
7:
	lghi	%r0,0			// Set delimiter character
3:
	mvst	%r3,%r2			// Copy string
	jo	3b			// Keep copying until done

	lghi	%r2,0			// Set the all clear
	br	%r14
	SET_SIZE(copystr)

#endif	/* lint */


/*
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 */
#if defined(lint)

/* ARGSUSED */
int
copyinstr(const char *uaddr, char *kaddr, size_t maxlength,
    size_t *lencopied)
{ return (0); }

#else	/* lint */

	ENTRY(copyinstr)
	ltgr	%r4,%r4			// Check max length
	jp	0f			// If positive then check string
	
	lghi	%r2,ENAMETOOLONG	// Set return code
	ltgr	%r5,%r5			// Resulting length wanted?
	bzr	%r14			// No... Just return
	lghi	%r4,1			// Set length copied
	stg	%r4,0(%r5)		// Return value to caller
	br	%r14
0:
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	lg	%r7,T_LOFAULT(%r9)	// Save existing fault hdlr
	larl	%r8,.copyinstr_err	// Get fault address
	stg	%r8,T_LOFAULT(%r9)	// Set new fault hdlr
	lghi	%r0,1			// Get secondary space ALET
	sar	%a2,%r0			// Set ALET
	sacf	AC_ACCESS		// Get into AR mode
	cli	0(%r2),0		// Null string
	sacf	AC_PRIMARY		// Return to primary mode
	jne	5f

	mvi	0(%r3),0		// Null terminate string
	lghi	%r2,0			// Set copied length
	ltgr	%r5,%r5			// Resulting length wanted?
	jz 	6f   			// No... Just return
	lghi	%r4,1			// Set length copied
	stg	%r4,0(%r5)		// Return value to caller
	j	6f			// Let's get out of here
5:
	cpya	%a12,%a2		// Set ALET
	lghi	%r0,0			// Set comparison character
	lgr	%r6,%r2			// Save for later
	la	%r12,0(%r4,%r2)		// Copy from address
	sacf	AC_ACCESS		// Get into AR mode
8:
	srst	%r12,%r2		// Locate end-of-string
	jo	8b			// Keep looking
	sacf	AC_PRIMARY		// Get out of AR mode
	jl	1f			// End of string found
	
	lgr	%r1,%r4			// Copy length
	lgr	%r0,%r3			// Copy "to" pointer
	lgr	%r2,%r6			// Copy "from" pointer
	lgr	%r3,%r4			// Get max length
	ltgr	%r5,%r5			// Resulting length wanted?
	jz	7f			// No... Skip it

	stg	%r3,0(%r5)		// Set lencopied
7:
	sacf	AC_ACCESS		// Get into AR mode
2:
	mvcle	%r0,%r2,0		// Copy string
	jo	2b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,ENAMETOOLONG	// Set error value
	j	6f
1:
	sgr	%r12,%r6		// Determine length of "from"
	aghi	%r12,1			// Adjust
	ltgr	%r5,%r5			// Resulting length wanted?
	jz	4f

	stg	%r12,0(%r5)		// Set count
4:
	lgr	%r2,%r6			// Re-establish "from" pointer
	lghi	%r0,0			// Get primary space ALET
	sar	%a3,%r0			// Set ALET
	sacf	AC_ACCESS		// Get into AR mode
3:
	mvst	%r3,%r2			// Copy string
	jo	3b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,0			// Set the all clear
6:
	stg	%r7,T_LOFAULT(%r9)	// Restore handler
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14

/*
 * Fault while trying to move from or to user space.
 * Set and return error code.
 */
.copyinstr_err:
	sacf	AC_PRIMARY			// Exit AR mode
	stg	%r7,T_LOFAULT(%r9)		// Restore original handler
	lgr	%r1,%r9				// Save thread
	aghi	%r15,SA(MINFRAME)		// Reset frame
	lmg	%r6,%r14,48(%r15)		// Restore registers

	lg	%r1,T_COPYOPS(%r1)		// Get copyops
	ltgr	%r1,%r1				// Installed?
	jz	3f				// No...just return

	lg	%r1,CP_COPYINSTR(%r1)		// Get copyinstr()
	br	%r1				// and go there
3:
	lghi	%r2,EFAULT			// Set error
	br	%r14				// and return

	SET_SIZE(copyinstr)
#endif

#if defined(lint)

/* ARGSUSED */
int
copyinstr_noerr(const char *uaddr, char *kaddr, size_t maxlength,
    size_t *lencopied)
{ return (0); }

#else	/* lint */

	ENTRY(copyinstr_noerr)
	ltgr	%r4,%r4			// Check max length
	jp	0f			// If positive then check string
	
	lghi	%r2,ENAMETOOLONG	// Set return code
	ltgr	%r5,%r5			// Resulting length wanted?
	bzr	%r14			// No... Just return
	lghi	%r4,1			// Set length copied
	stg	%r4,0(%r5)		// Return value to caller
	br	%r14
0:
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)
	lghi	%r0,1			// Get secondary space ALET
	sar	%a2,%r0			// Set ALET
	sacf	AC_ACCESS		// Get into AR mode
	cli	0(%r2),0		// Null string
	sacf	AC_PRIMARY		// Return to primary mode
	jne	5f

	mvi	0(%r3),0		// Null terminate string
	lghi	%r2,0			// Set copied length
	ltgr	%r5,%r5			// Resulting length wanted?
	jz 	6f   			// No... Just return
	lghi	%r4,1			// Set length copied
	stg	%r4,0(%r5)		// Return value to caller
	j	6f			// Let's get out of here
5:
	cpya	%a12,%a2		// Set ALET
	lghi	%r0,0			// Set comparison character
	lgr	%r6,%r2			// Save for later
	la	%r12,0(%r4,%r2)		// Copy from address
	sacf	AC_ACCESS		// Get into AR mode
8:
	srst	%r12,%r2		// Locate end-of-string
	jo	8b			// Keep looking
	sacf	AC_PRIMARY		// Get out of AR mode
	jl	1f			// End of string found
	
	lgr	%r1,%r4			// Copy length
	lgr	%r0,%r3			// Copy "to" pointer
	lgr	%r2,%r6			// Copy "from" pointer
	lgr	%r3,%r4			// Get max length
	ltgr	%r5,%r5			// Resulting length wanted?
	jz	7f			// No... Skip it

	stg	%r3,0(%r5)		// Set lencopied
7:
	sacf	AC_ACCESS		// Get into AR mode
2:
	mvcle	%r0,%r2,0		// Copy string
	jo	2b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,ENAMETOOLONG	// Set error value
	j	6f
1:
	sgr	%r12,%r6		// Determine length of "from"
	aghi	%r12,1			// Adjust
	ltgr	%r5,%r5			// Resulting length wanted?
	jz	4f

	stg	%r12,0(%r5)		// Set count
4:
	lgr	%r2,%r6			// Re-establish "from" pointer
	lghi	%r0,0			// Get primary space ALET
	sar	%a3,%r0			// Set ALET
	sacf	AC_ACCESS		// Get into AR mode
3:
	mvst	%r3,%r2			// Copy string
	jo	3b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,0			// Set the all clear
6:
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14
	SET_SIZE(copyinstr_noerr)

#endif	/* lint */

/*
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 */

#if defined(lint)

/* ARGSUSED */
int
copyoutstr(const char *kaddr, char *uaddr, size_t maxlength,
    size_t *lencopied)
{ return (0); }

#else	/* lint */

	ENTRY(copyoutstr)
	ltgr	%r4,%r4			// Check max length
	jnp	4f			// If zero then set rc and leave

	cli	0(%r2),0		// Null string
	jne	0f
4:
	sacf	AC_ACCESS		// Get into AR mode
	mvi     0(%r3),0		// Null terminate
	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,ENAMETOOLONG	// Set return code
	ltgr	%r5,%r5			// Resulting length wanted?
	bzr	%r14			// No... Just return

	lghi	%r4,1			// Set length copied
	stg	%r4,0(%r5)		// Return value to caller
	br	%r14
0:
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	GET_THR(9)

	lg	%r7,T_LOFAULT(%r9)	// Save existing fault hdlr
	larl	%r8,.copyoutstr_err	// Get fault address
	stg	%r8,T_LOFAULT(%r9)	// Set new fault hdlr
	lghi	%r0,0			// Set comparison character
	lgr	%r8,%r2			// Save for later
	la	%r1,0(%r4,%r2)		// Potential end of string
8:
	srst	%r1,%r2			// Locate end-of-string
	jo	8b			// Keep looking
	jl	1f			// End of string found

	lghi	%r0,1			// Get secondary space ALET
	sar	%a2,%r0			// Set ALET
	lgr	%r1,%r4			// Copy length
	lgr	%r0,%r8			// Copy "from" pointer
	lgr	%r2,%r3			// Copy "to" pointer
	lgr	%r3,%r4			// Get length
	ltgr	%r5,%r5			// Resulting length wanted?
	jz	7f			// No... Skip it

	stg	%r3,0(%r5)		// Set lencopied
7:
	sacf	AC_ACCESS		// Get into AR mode
2:
	mvcle	%r2,%r0,0		// Copy string
	jo	2b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,ENAMETOOLONG	// Set error value
	j	6f
1:
	lgr	%r0,%r8			// Copy start of string
	sgr	%r8,%r2			// Determine length of "from"
	aghi	%r8,1			// Adjust
	lgr	%r2,%r0			// Restore start of string pointer
	lghi	%r0,1			// Get secondary space ALET
	sar	%a3,%r0			// Set ALET
	lghi	%r0,0			// Get primary space ALET
	sar	%a2,%r0			// Set ALET
	ltgr	%r5,%r5			// Resulting length wanted?
	jz	4f

	stg	%r8,0(%r5)		// Set count
4:
	sacf	AC_ACCESS		// Get into AR mode
3:
	mvst	%r3,%r2			// Copy string
	jo	3b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,0			// Get count
6:
	stg	%r7,T_LOFAULT(%r9)	// Restore handler
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14

/*
 * Fault while trying to move from or to user space.
 * Set and return error code.
 */
.copyoutstr_err:
	sacf	AC_PRIMARY			// Exit AR mode
	stg	%r7,T_LOFAULT(%r9)		// Restore original handler
	lgr	%r1,%r9				// Save thread
	aghi	%r15,SA(MINFRAME)		// Reset frame
	lmg	%r6,%r14,48(%r15)		// Restore registers

	lg	%r1,T_COPYOPS(%r1)		// Get copyops
	ltgr	%r1,%r1				// Installed?
	jz	3f				// No...just return

	lg	%r1,CP_COPYOUTSTR(%r1)		// Get copyoutstr()
	br	%r1				// and go there
3:
	lghi	%r2,EFAULT			// Set error
	br	%r14				// and return

	SET_SIZE(copyoutstr)

#endif	/* lint */

#if defined(lint)

/* ARGSUSED */
int
copyoutstr_noerr(const char *kaddr, char *uaddr, size_t maxlength,
    size_t *lencopied)
{ return (0); }

#else	/* lint */

	ENTRY(copyoutstr_noerr)
	ltgr	%r4,%r4			// Check max length
	jnp	4f			// If zero then set rc and leave

	cli	0(%r2),0		// Null string
	jne	0f
4:
	sacf	AC_ACCESS		// Get into AR mode
	mvi     0(%r3),0		// Null terminate
	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,ENAMETOOLONG	// Set return code
	ltgr	%r5,%r5			// Resulting length wanted?
	bzr	%r14			// No... Just return

	lghi	%r4,1			// Set length copied
	stg	%r4,0(%r5)		// Return value to caller
	br	%r14
0:
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)
	lghi	%r0,0			// Set comparison character
	lgr	%r8,%r2			// Save for later
	la  	%r1,0(%r4,%r2)		// Potential end of string
8:
	srst	%r1,%r2			// Locate end-of-string
	jo	8b			// Keep looking
	jl	1f			// End of string found

	lghi	%r0,1			// Get secondary space ALET
	sar	%a2,%r0			// Set ALET
	lgr	%r1,%r4			// Copy length
	lgr	%r0,%r8			// Copy "from" pointer
	lgr	%r2,%r3			// Copy "to" pointer
	lgr	%r3,%r4			// Get length
	ltgr	%r5,%r5			// Resulting length wanted?
	jz	7f			// No... Skip it

	stg	%r3,0(%r5)		// Set lencopied
7:
	sacf	AC_ACCESS		// Get into AR mode
2:
	mvcle	%r2,%r0,0		// Copy string
	jo	2b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,ENAMETOOLONG	// Set error value
	j	6f
1:
	lgr	%r0,%r8			// Copy start of string
	sgr	%r8,%r2			// Determine length of "from"
	aghi	%r8,1			// Adjust
	lgr	%r2,%r0			// Reload "from" pointer
	lghi	%r0,1			// Get secondary space ALET
	sar	%a3,%r0			// Set ALET
	lghi	%r0,0			// Get primary space ALET
	sar	%a2,%r0			// Set ALET
	ltgr	%r5,%r5			// Resulting length wanted?
	jz	4f

	stg	%r8,0(%r5)		// Set count
4:
	sacf	AC_ACCESS		// Get into AR mode
3:
	mvst	%r3,%r2			// Copy string
	jo	3b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,0			// Get count
6:
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14
	SET_SIZE(copyoutstr_noerr)

#endif	/* lint */


/*
 * Copy a block of storage - must not overlap (from + len <= to).
 * No fault handler installed (to be called under on_fault())
 */

#if defined(lint)
 
/* ARGSUSED */
void
ucopy(const void *ufrom, void *uto, size_t ulength)
{}
 
#else /* lint */
 
	ENTRY(ucopy)
	lghi	%r0,1			// Get secondary space ALET
	sar	%a2,%r0			// Set ALET
	sar	%a4,%r0			// Set ALET
	lgr	%r5,%r4			// Copy length
	lgr	%r4,%r3			// Get "to" pointer
	lgr	%r3,%r5			// Copy length
	sacf	AC_ACCESS		// Get into access mode
0:
	mvcle	%r4,%r2,0		// Copy block
	jo	0b			// Keep going until done

	sacf	AC_PRIMARY		// Return to primary mode
	br	%r14			// Return
	SET_SIZE(ucopy)
#endif /* lint */

/*
 * Set a block of storage to a given character
 */

#if defined(lint)
 
/* ARGSUSED */
void *
_memset(const void *from, char set, size_t length)
{}
 
#else /* lint */
 
	ENTRY(_memset)
	lgr	%r5,%r2			// Save return value
	ltgr	%r4,%r4			// Don't do anything...
	jz 	2f			// ...if it's zero length
	cghi	%r4,256			// Can we do it with MVC?
	jle	0f			// Yes... Skip MVCLE processing
	
	lgr	%r0,%r2			// Set destination
	lgr	%r1,%r4			// Set destination length
	lgr	%r4,%r3			// Get pad character
	lghi	%r3,0			// Set source length	
1:	mvcle	%r0,%r2,0(%r4)		// memset
	jo	1b			// Go until all set
	j	2f			// Done

0:		
	stc	%r3,0(%r2)		// Set first character
	aghi	%r4,-2			// Adjust count for EX
	larl	%r1,3f			// Point at EX 
	ex	%r4,0(%r1)		// Overlap move

2:	lgr	%r2,%r5			// Set return value
	br	%r14			// Return

3:	mvc	1(1,%r2),0(%r2)

	SET_SIZE(_memset)
#endif /* lint */

/*
 * Copy a user-land string.  If the source and target regions overlap,
 * one or both of the regions will be silently corrupted.
 * No fault handler installed (to be called under on_fault())
 */

#if defined(lint)
 
/* ARGSUSED */
void
ucopystr(const char *ufrom, char *uto, size_t umaxlength, size_t *ulencopied)
{}
 
#else /* lint */
 
	ENTRY(ucopystr)
	stmg	%r8,%r9,64(%r15)
	ltgr	%r4,%r4			// Check max length
	jnp	4f			// If zero or negative then set rc and leave
	jh	5f			// If positive then check string
5:
	lghi	%r0,1			// Get secondary space ALET
	sar	%a2,%r0			// Set ALET
	sacf	AC_ACCESS		// Go into AR mode
	cli	0(%r2),0		// Null string
	sacf	AC_PRIMARY		// Return to primary mode
	jne	0f			// No... Skip the early return
4:
	sacf	AC_ACCESS		// Get into AR mode
	mvi     0(%r3),0		// Null terminate
	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,0			// Set copied length
	stg	%r2,0(%r5)		// Set lencopied
	lmg	%r8,%r9,64(%r15)
	br	%r14
0:
	cpya	%a8,%a2			// Set ALET
	lghi	%r0,0			// Set comparison character
	lgr	%r9,%r2 		// Copy for later
	la 	%r8,0(%r4,%r2)		// Potential end of string
	sacf	AC_ACCESS		// Get into AR mode
8:
	srst	%r8,%r2			// Locate end-of-string
	jo	8b			// Keep looking
	sacf	AC_PRIMARY		// Get out of AR mode
	jl	1f			// End of string found

	lgr	%r8,%r9			// Copy "from" pointer
	lgr	%r9,%r4			// Copy length
	lgr	%r2,%r3			// Copy "to" pointer
	lgr	%r3,%r4			// Get length
	stg	%r3,0(%r5)		// Set length copied
	sacf	AC_ACCESS		// Get into AR mode
2:
	mvcle	%r2,%r8,0		// Copy string
	jo	2b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,ENAMETOOLONG	// Indicate too big
	j	6f
1:
	sgr	%r2,%r9			// Determine length
	stg	%r2,0(%r5)		// Set count
	lghi	%r0,0			// Set end-of-string character
	lgr	%r8,%r3			// Copy destination poitner
	sacf	AC_ACCESS		// Get into AR mode
3:
	mvst	%r8,%r9			// Copy string
	jo	3b			// Keep copying until done

	sacf	AC_PRIMARY		// Get out of AR mode
	lghi	%r2,0			// Set the all clear
6:
	lmg	%r8,%r9,64(%r15)
	br	%r14
	SET_SIZE(ucopystr)

#endif /* lint */
