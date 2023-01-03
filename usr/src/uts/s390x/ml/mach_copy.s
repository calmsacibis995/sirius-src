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
#include <sys/privregs.h>
#include <sys/machthread.h>

#define	FP_USED 1
#define	LOFAULT_SET 1

/*
 * Error barrier:
 * We use membar sync to establish an error barrier for
 * deferred errors. Membar syncs are added before any update
 * to t_lofault to ensure that deferred errors from earlier
 * accesses will not be reported after the membar. This error
 * isolation is important when we try to recover from async
 * errors which tries to distinguish kernel accesses to user
 * data.
 */

/*
 * Zero a block of storage.
 *
 * uzero is used by the kernel to zero a block in user address space.
 */

#if defined(lint)

/* ARGSUSED */
int
kzero(void *addr, size_t count)
{ return(0); }

/* ARGSUSED */
void
uzero(void *addr, size_t count)
{}

#else	/* lint */

	ENTRY(uzero)
	stmg	%r6,%r14,48(%r15)
	lgr	%r10,%r14
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	//
	// Set a new lo_fault handler only if we came in with one
	// already specified.
	//

	GET_THR(9)

	lg	%r7,T_LOFAULT(%r9)
#ifdef DEBUG
	tracg	%r0,%r15,__LC_ANY_TRACE
#endif
	ltgr	%r7,%r7
	jz	0f

	larl	%r8,.zeroerr
	stg	%r8,T_LOFAULT(%r9)
0:
	lghi	%r0,1
	sar	%a2,%r0
	sacf	AC_ACCESS

	lghi	%r6,-1			// 
	srlg	%r6,%r6,20		// 2**25 - 1
	lgr	%r4,%r3			// Copy count
	lgr	%r0,%r2			// Get from
2:
	cgr	%r4,%r6			// Count <= 2**25-1?
	jge	3f			// No... Go clear a chunk

	lgr	%r3,%r4			// Set destination length
	lghi	%r1,0  			// Set pad to zero
	mvcl	%r2,%r0			// Copy remaining block of data
	j	4f			// All Done
3:
	lgr	%r3,%r6			// Set destination length
	lghi	%r1,0  			// Set pad to zero
	mvcl	%r2,%r0			// Copy 2**25-1 worth of data
	sgr	%r4,%r6			// Adjust count
	jp	2b			// Do next chunk of data
4:
	sacf	AC_PRIMARY
	//
	// We're just concerned with whether t_lofault was set
	// when we came in. We end up here from either kzero()
	// or bzero(). kzero() *always* sets a lofault handler.
	//

	nill	%r7,65535-LOFAULT_SET	// Turn off flag
	stg	%r7,T_LOFAULT(%r9)	// Restore old handler
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	lghi	%r2,0
	br	%r14
	SET_SIZE(uzero)

	ENTRY(kzero)
	stmg	%r6,%r14,48(%r15)
	lgr	%r10,%r14
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

	//
	// Always set a lo_fault handler
	//

	GET_THR(9)

	lg	%r7,T_LOFAULT(%r9)
	larl	%r8,.zeroerr
	stg	%r8,T_LOFAULT(%r9)

	lgr	%r4,%r3			// Copy count
#ifdef DEBUG
	tracg	%r0,%r15,__LC_ANY_TRACE
#endif
	lghi	%r6,-1			// 
	srlg	%r6,%r6,20		// 2**25 - 1
	lgr	%r0,%r2			// Get to
1:
	cgr	%r4,%r6			// Count <= 2**25-1?
	jge	2f			// No... Go clear a chunk

	lgr	%r3,%r4			// Set destination length
	lghi	%r1,0  			// Set pad to zero
	mvcl	%r2,%r0			// Copy remaining block of data
	j	3f			// All Done
2:
	lgr	%r3,%r6			// Set destination length
	lghi	%r1,0  			// Set pad to zero
	mvcl	%r2,%r0			// Copy 2**25-1 worth of data
	sgr	%r4,%r6			// Adjust count
	jp	1b			// Do next chunk of data
3:
	//
	// We're just concerned with whether t_lofault was set
	// when we came in. We end up here from either kzero()
	// or bzero(). kzero() *always* sets a lofault handler.
	//

	nill	%r7,65535-LOFAULT_SET	// Turn off flag
	stg	%r7,T_LOFAULT(%r9)	// Restore old handler
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	lghi	%r2,0
	br	%r14

/*
 * We got here because of a fault during kzero or if
 * uzero or bzero was called with t_lofault non-zero.
 * Otherwise we've already run screaming from the room.
 * Errno value is in %r2. Note that we're here iff
 * we did set t_lofault.
 */
.zeroerr:
	sacf	AC_PRIMARY
	//
	// We did set t_lofault. It may well have been zero coming in.
	//
1:
	ltgr	%r7,%r7
	jnz	2f

	nill	%r7,65535-LOFAULT_SET
	jnz	3f
2:
	//
	// Old handler was zero. Just return the error.
	//

	stg	%r7,T_LOFAULT(%r9)
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14

3:
	//
	// We're here because %r7 was non-zero. It was non-zero
	// because either LOFAULT_SET was present, a previous fault
	// handler was present or both. In all cases we need to reset
	// T_LOFAULT to the value of %r7 after clearing LOFAULT_SET
	// before we either simply return the error or we invoke the
	// previously specified handler.
	//

	stg	%r7,T_LOFAULT(%r9)
	lgr	%r1,%r7
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r1
	SET_SIZE(kzero)

#endif	/* lint */

/*
 * Zero a block of storage.
 */

#if defined(lint)

/* ARGSUSED */
void
bzero(void *addr, size_t count)
{}

#else	/* lint */

	ENTRY(bzero)
	ltgr	%r3,%r3			// Count > 0?
	jnh	5f			// No... just return

	stmg	%r6,%r14,48(%r15)
	lgr	%r10,%r14
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)

.do_zero:
	lgr	%r4,%r3			// Copy count
	
	GET_THR(9)

	lg	%r7,T_LOFAULT(%r9)	// Get old handler
#ifdef DEBUG
	tracg	%r0,%r15,__LC_ANY_TRACE
#endif
	ltgr	%r7,%r7			// Is there one?
	jnz	0f			// Yes... Don't replace

	larl	%r8,.zeroerr		// Get our handler
	stg	%r8,T_LOFAULT(%r9)	// Plug it in
0:
	oill	%r7,LOFAULT_SET		// Indicate fault handler set
	lghi	%r6,-1			// 
	srlg	%r6,%r6,20		// 2**25 - 1
	lgr	%r0,%r2			// Get to
1:
	cgr	%r4,%r6			// Count <= 2**25-1?
	jge	2f			// No... Go clear a chunk

	lgr	%r3,%r4			// Set destination length
	lghi	%r1,0  			// Set pad to zero
	mvcl	%r2,%r0			// Copy remaining block of data
	j	3f			// All Done
2:
	lgr	%r3,%r6			// Set destination length
	lghi	%r1,0  			// Set pad to zero
	mvcl	%r2,%r0			// Copy 2**25-1 worth of data
	sgr	%r4,%r6			// Adjust count
	jp	1b			// Do next chunk of data
3:
	//
	// We're just concerned with whether t_lofault was set
	// when we came in. We end up here from either kzero()
	// or bzero(). kzero() *always* sets a lofault handler.
	//

	nill	%r7,65535-LOFAULT_SET	// Turn off flag
	stg	%r7,T_LOFAULT(%r9)	// Restore old handler
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
5:
	lghi	%r2,0
	br	%r14
	SET_SIZE(bzero)
#endif	/* lint */

/*
 * Zero a block of storage. Different forms of above. Need to choose which is best. FIXME
 *
 * uzero is used by the kernel to zero a block in user address space.
 */

#if defined(lint)

/* ARGSUSED */
int
kzero(void *addr, size_t count)
{ return(0); }

/* ARGSUSED */
void
uzero(void *addr, size_t count)
{}

#else	/* lint */

	ENTRY(uzeron)
	stg	%r6,48(%r15)
	aghi	%r15,-SA(MINFRAME)
	
	// Set a new lo_fault handler only if we came in with one
	// already specified.
	
	GET_THR(9)

	lg	%r6,T_LOFAULT(%r9)
	ltgr	%r6,%r6
	jz	0f
	
	larl	%r0,.zeroerrn
	stg	%r0,T_LOFAULT(%r9)
0:
	lghi	%r0,1
	sar	%a2,%r0
	lghi	%r4,0
	lghi	%r5,0
	sacf	AC_ACCESS
1:	mvcle	%r2,%r4,0
	jo	1b

	sacf	AC_PRIMARY
	stg	%r6,T_LOFAULT(%r9)
	lghi	%r2,0
	aghi	%r15,SA(MINFRAME)
	lg	%r6,48(%r15)
	br	%r14
	SET_SIZE(uzeron)

	ENTRY(kzeron)
	//
	// Always set a lo_fault handler
	//
	stg	%r6,48(%r15)
	aghi	%r15,-SA(MINFRAME)
	
	GET_THR(9)

	lg	%r6,T_LOFAULT(%r9)
	larl	%r0,.zeroerrn
	stg	%r0,T_LOFAULT(%r9)
	lghi	%r4,0
	lghi	%r5,0
0:	mvcle	%r2,%r4,0
	jo	0b

	stg	%r6,T_LOFAULT(%r9)
	aghi	%r15,SA(MINFRAME)
	lg	%r6,48(%r15)
	lghi	%r2,0
	br	%r14

/*
 * We got here because of a fault during kzero or if
 * uzero or bzero was called with t_lofault non-zero.
 * Otherwise we've already run screaming from the room.
 * Errno value is in %r3. Note that we're here iff
 * we did set t_lofault.
 */
.zeroerrn:
	sacf	AC_PRIMARY
	//
	// We did set t_lofault. It may well have been zero coming in.
	//
1:
	ltgr	%r6,%r6		
	jnz	3f
2:
	//
	// Old handler was zero. Just return the error.
	//

	nill	%r6,65535-LOFAULT_SET
	stg	%r6,T_LOFAULT(%r9)
	lghi	%r2,-EINVAL
	aghi	%r15,SA(MINFRAME)
	lg	%r6,48(%r15)
	br	%r14		// return
3:
	//
	// We're here because %r6 was non-zero. It was non-zero
	// because either LOFAULT_SET was present, a previous fault
	// handler was present or both. In all cases we need to reset
	// T_LOFAULT to the value of %r6 after clearing LOFAULT_SET
	// before we either simply return the error or we invoke the
	// previously specified handler.
	//

	nill	%r6,65535-LOFAULT_SET
	stg	%r6,T_LOFAULT(%r9)
	lgr	%r1,%r6
	aghi	%r15,SA(MINFRAME)
	lg	%r6,48(%r15)
	br	%r1 
	SET_SIZE(kzeron)

#endif	/* lint */

/*
 * Zero a block of storage.
 */

#if defined(lint)

/* ARGSUSED */
void
bzeron(void *addr, size_t count)
{}

#else	/* lint */

	ENTRY(bzeron)
	stg	%r6,48(%r15)
	aghi	%r15,-SA(MINFRAME)
	
	GET_THR(9)

	lg	%r6,T_LOFAULT(%r9)
	ltgr	%r6,%r6
	jz	0f
	
	larl	%r0,.zeroerrn
	stg	%r0,T_LOFAULT(%r9)
0:
	lghi	%r0,1
	sar	%a2,%r0
	lghi	%r4,0
	lghi	%r5,0
	sacf	AC_ACCESS

1:	mvcle	%r2,%r4,0
	jo	1b

	sacf	AC_PRIMARY
	stg	%r6,T_LOFAULT(%r9)
	aghi	%r15,SA(MINFRAME)
	lg	%r6,48(%r15)
	lghi	%r2,0
	br	%r14

	SET_SIZE(bzeron)
#endif	/* lint */
