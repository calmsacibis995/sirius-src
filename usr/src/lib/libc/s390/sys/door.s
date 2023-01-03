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

	.file	"door.s"

#include <sys/asm_linkage.h>


#include <sys/door.h>
#include "SYS.h"

/*
 * Offsets within struct door_results
 */
#define	DOOR_COOKIE	(SA(MINFRAME) + 0*CLONGSIZE)
#define	DOOR_DATA_PTR	(SA(MINFRAME) + 1*CLONGSIZE)
#define	DOOR_DATA_SIZE	(SA(MINFRAME) + 2*CLONGSIZE)
#define	DOOR_DESC_PTR	(SA(MINFRAME) + 3*CLONGSIZE)
#define	DOOR_DESC_SIZE	(SA(MINFRAME) + 4*CLONGSIZE)
#define	DOOR_PC		(SA(MINFRAME) + 5*CLONGSIZE)
#define	DOOR_SERVERS	(SA(MINFRAME) + 6*CLONGSIZE)
#define	DOOR_INFO_PTR	(SA(MINFRAME) + 7*CLONGSIZE)

/*
 * All of the syscalls except door_return() follow the same pattern.  The
 * subcode goes on the stack, after all of the other arguments.
 */
#define	DOOR_SYSCALL(name, code)					\
	ENTRY(name);							\
	ahi	%r15,-SA(MINFRAME32+CLONGSIZE);				\
	lhi	0,code;							\
	st	0,96(15);						\
	SYSTRAP_RVAL1(door);						\
	ahi	%r15,SA(MINFRAME32+CLONGSIZE);				\
	SYSCERROR;							\
	RET;								\
	SET_SIZE(name)

	DOOR_SYSCALL(__door_bind,	DOOR_BIND)
	DOOR_SYSCALL(__door_call,	DOOR_CALL)
	DOOR_SYSCALL(__door_create,	DOOR_CREATE)
	DOOR_SYSCALL(__door_getparam,	DOOR_GETPARAM)
	DOOR_SYSCALL(__door_info,	DOOR_INFO)
	DOOR_SYSCALL(__door_revoke,	DOOR_REVOKE)
	DOOR_SYSCALL(__door_setparam,	DOOR_SETPARAM)
	DOOR_SYSCALL(__door_ucred,	DOOR_UCRED)
	DOOR_SYSCALL(__door_unbind,	DOOR_UNBIND)
	DOOR_SYSCALL(__door_unref,	DOOR_UNREFSYS)

/*
 * int
 * __door_return(
 *	void 			*data_ptr,
 *	size_t			data_size,	(in bytes)
 *	door_return_desc_t	*door_ptr,	(holds returned desc info)
 *	caddr_t			stack_base,
 *	size_t			stack_size)
 */
	ENTRY(__door_return)
door_restart:
	ahi	%r15,-SA(MINFRAME32+CLONGSIZE)
	lhi	%r0,DOOR_RETURN
	st	%r0,96(%r15)
	SYSTRAP_RVAL1(door)
//
//	After this call r15 has been changed by the call
//
	l	%r1,DOOR_SERVERS(%r15)
	ltr	%r0,%r0			// Check success of syscall
	jl	2f			// Go if there was an error

	/*
	 * On return, we're serving a door_call.  Our stack looks like this:
	 *
	 *		descriptors (if any)
	 *		data (if any)
	 *		struct door_results
	 *		MINFRAME
	 *	sp ->
	 */
	ltr	%r1,%r1
	jh	1f

	/*
	 * this is the last server thread - call creation func for more
	 */
	ahi	%r15,-SA(MINFRAME32)
	larl	%r6,door_server_func
	l	%r1,0(%r6)
	l	%r2,DOOR_INFO_PTR(%r15)
	basr	%r14,%r1
	ahi	%r15,SA(MINFRAME32)
1:
	/* Call the door server function now */
	l	%r2,DOOR_COOKIE(%r15)
	l	%r3,DOOR_DATA_PTR(%r15)
	l	%r4,DOOR_DATA_SIZE(%r15)
	l	%r5,DOOR_DESC_PTR(%r15)
	l	%r6,DOOR_DESC_SIZE(%r15)
	l	%r1,DOOR_PC(%r15)
	basr	%r14,%r1

	/* Exit the thread if we return here */
	lhi	%r2,0
	brasl	%r14,_thrp_terminate@PLT
	/* NOTREACHED */
2:
	/*
	 * Error during door_return call.  Repark the thread in the kernel if
	 * the error code is EINTR (or ERESTART) and this lwp is still part
	 * of the same process.
	 */
	chi	%r2,ERESTART
	je	3f
	lhi	%r2,EINTR
3:
	chi	%r2,EINTR		// Interrupted while waiting?
	jgne	__cerror		// No.. return the error

	ahi	%r15,-SA(MINFRAME32)
	brasl	%r14,getpid@PLT
	ahi	%r15,SA(MINFRAME32)
	larl	%r6,door_create_pid
	l	%r1,0(%r6)
	cr	%r2,%r1
	je	4f

	lhi	%r2,EINTR
	jgne	__cerror		// Return the error
4:
	lhi	%r2,0
	lr	%r3,%r2
	lr	%r4,%r2
	jg	door_restart

	SET_SIZE(__door_return)

	/*
	 * weak aliases for public interfaces
	 */
	ANSI_PRAGMA_WEAK2(door_bind,__door_bind,function)
	ANSI_PRAGMA_WEAK2(door_call,__door_call,function)
	ANSI_PRAGMA_WEAK2(door_getparam,__door_getparam,function)
	ANSI_PRAGMA_WEAK2(door_info,__door_info,function)
	ANSI_PRAGMA_WEAK2(door_revoke,__door_revoke,function)
	ANSI_PRAGMA_WEAK2(door_setparam,__door_setparam,function)
	ANSI_PRAGMA_WEAK2(door_unbind,__door_unbind,function)
	ANSI_PRAGMA_WEAK2(_door_bind,__door_bind,function)
	ANSI_PRAGMA_WEAK2(_door_call,__door_call,function)
	ANSI_PRAGMA_WEAK2(_door_getparam,__door_getparam,function)
	ANSI_PRAGMA_WEAK2(_door_info,__door_info,function)
	ANSI_PRAGMA_WEAK2(_door_revoke,__door_revoke,function)
	ANSI_PRAGMA_WEAK2(_door_setparam,__door_setparam,function)
	ANSI_PRAGMA_WEAK2(_door_unbind,__door_unbind,function)
