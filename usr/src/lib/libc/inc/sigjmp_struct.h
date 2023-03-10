/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
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
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SIGJMP_STRUCT_H
#define	_SIGJMP_STRUCT_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stack.h>
#include <ucontext.h>
#include <setjmp.h>

#if defined(__sparc)

/*
 * The following structure MUST match the ABI size specifier _SIGJBLEN.
 * This is 19 (longs).  The ABI value for _JBLEN is 12 (longs).
 * A greg_t is a long.  A sigset_t is 4 ints and a stack_t is 3 longs.
 *
 * The layout of this structure must match the layout of the same
 * structures defined in usr/src/lib/libbc/libc/sys/common/ucontext.h
 * and usr/src/ucblib/libucb/sparc/sys/setjmp.c.  Other than that,
 * the layout is private, not known to applications.
 *
 * We make the first 5 members match the implementations of
 * setjmp()/longjmp(), so that an application could (stupidly)
 * do sigsetjmp(env) followed by longjmp(env) (but not setjmp(env)
 * followed by siglongjmp(env)).
 */
typedef struct {
	int		sjs_flags;	/* JBUF[ 0]	*/
	greg_t		sjs_sp;		/* JBUF[ 1]	*/
	greg_t		sjs_pc;		/* JBUF[ 2]	*/
	greg_t		sjs_fp;		/* JBUF[ 3]	*/
	greg_t		sjs_i7;		/* JBUF[ 4]	*/
	ucontext_t	*sjs_uclink;
	ulong_t		sjs_pad[_JBLEN - 6];
	sigset_t	sjs_sigmask;
#if defined(_LP64)
	ulong_t		sjs_pad1[2];
#endif
	stack_t		sjs_stack;
} sigjmp_struct_t;

#define	JB_SAVEMASK	0x1
#define	JB_FRAMEPTR	0x2

#elif __s390x

/*
 * The following structure MUST match the ABI size specifier _SIGJBLEN.
 * This is 19 (longs).  The ABI value for _JBLEN is 18 (longs).
 * A greg_t is a long.  A sigset_t is 4 ints and a stack_t is 3 longs.
 *
 * The layout of this structure must match the layout of the same
 * structures defined in usr/src/lib/libbc/libc/sys/common/ucontext.h
 * and usr/src/ucblib/libucb/sparc/sys/setjmp.c.  Other than that,
 * the layout is private, not known to applications.
 *
 * We make the first 11 members match the implementations of
 * setjmp()/longjmp(), so that an application could (stupidly)
 * do sigsetjmp(env) followed by longjmp(env) (but not setjmp(env)
 * followed by siglongjmp(env)).
 */
typedef struct {
	greg_t		sjs_r6;
	greg_t		sjs_r7;
	greg_t		sjs_r8;
	greg_t		sjs_r9;
	greg_t		sjs_r10;
	greg_t		sjs_r11;
	greg_t		sjs_r12;
	greg_t		sjs_r13;
	greg_t		sjs_pc;	
	greg_t		sjs_sp;
	double 		sjs_f1;
	double 		sjs_f3;
	double 		sjs_f5;
	double 		sjs_f7;
	double 		sjs_f4;
	double 		sjs_f6;
	uint32_t	sjs_fpc;
	uint32_t	sjs_flags;	
	ucontext_t	*sjs_uclink;
	sigset_t	sjs_sigmask;
	stack_t		sjs_stack;
} sigjmp_struct_t;

#define	JB_SAVEMASK	0x1
#define	JB_FRAMEPTR	0x2

#elif __s390

/*
 * The following structure MUST match the ABI size specifier _SIGJBLEN.
 * This is 18 (longs).  The ABI value for _JBLEN is 12 (longs).
 * A greg_t is a long.  A sigset_t is 4 ints and a stack_t is 3 longs.
 *
 * The layout of this structure must match the layout of the same
 * structures defined in usr/src/lib/libbc/libc/sys/common/ucontext.h
 * and usr/src/ucblib/libucb/sparc/sys/setjmp.c.  Other than that,
 * the layout is private, not known to applications.
 *
 * We make the first 11 members match the implementations of
 * setjmp()/longjmp(), so that an application could (stupidly)
 * do sigsetjmp(env) followed by longjmp(env) (but not setjmp(env)
 * followed by siglongjmp(env)).
 */
typedef struct {
	greg_t		sjs_r6;
	greg_t		sjs_r7;
	greg_t		sjs_r8;
	greg_t		sjs_r9;
	greg_t		sjs_r10;
	greg_t		sjs_r11;
	greg_t		sjs_r12;
	greg_t		sjs_r13;
	greg_t		sjs_pc;	
	greg_t		sjs_sp;
	double 		sjs_f4;	
	double 		sjs_f6;	
	uint32_t	sjs_fpc;
	uint32_t	sjs_flags;	
	ucontext_t	*sjs_uclink;
	sigset_t	sjs_sigmask;
	stack_t		sjs_stack;
} sigjmp_struct_t;

#define	JB_SAVEMASK	0x1
#define	JB_FRAMEPTR	0x2

#endif	/* __sparc */

#ifdef	__cplusplus
}
#endif

#endif /* _SIGJMP_STRUCT_H */
