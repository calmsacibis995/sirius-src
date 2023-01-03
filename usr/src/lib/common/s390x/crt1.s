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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * This crt1.o module is provided as the bare minimum required to build
 * a 32-bit executable with gcc.  It is installed in /usr/lib
 * where it will be picked up by gcc, along with crti.o and crtn.o
 */

	.ident	"@(#)crt1.s	1.2	05/06/08 SMI"

	.file	"crt1.s"

	.globl	_start

/* global entities defined elsewhere but used here */
	.globl	main
	.globl	exit
	.globl	_exit
	.weak	_DYNAMIC

	.section	.data

	.weak	environ
	.set	environ,_environ
	.globl	_environ
	.type	_environ,@object
	.size	_environ,8
	.align	8
_environ:
	.quad	0x0

	.globl	___Argv
	.type	___Argv,@object
	.size	___Argv,8
	.align	8
___Argv:
	.quad	0x0

	.section	.text
	.align	8
pDYNAMIC:
	.quad   _DYNAMIC

/*
 * C language startup routine.
 * R2 - argc
 * R3 - A(argv)
 * R4 - A(envp)
 *
 * Allocate a NULL return address and a NULL previous SP as if
 * there was a genuine call to _start.
 * sdb stack trace shows _start(argc,argv[0],argv[1],...,envp[0],...)
 */
	.type	_start,@function
_start:
	larl	%r13,pDYNAMIC
	lgr	%r6,%r2			// Save argc
	lgr	%r7,%r3			// Save **argv
	lgr	%r8,%r4			// Save envp
	lgr	%r9,%r5			// Save exit function
	lhi	%r0,0			// 
	st	%r0,24(%r15)		// Clear FPC
	lfpc	24(%r15)		//
	lg	%r5,0(%r13)		// Get _DYNAMIC
	ltgr	%r5,%r5			// Set?
	jz	1f			// No... Skip

	lgr	%r2,%r9			// Copy
	brasl	%r14,atexit@PLT		// Go set
1:
	larl	%r2,_fini		// Our atexit function
	brasl	%r14,atexit@PLT		// Go set

/*
 * The following code provides almost standard static destructor handling
 * for systems that do not have the modified atexit processing in their
 * system libraries.  It checks for the existence of the new routine
 * "_get_exit_frame_monitor()", which is in libc.so when the new exit-handling
 * code is there.  It then check for the existence of "__Crun::do_exit_code()"
 * which will be in libCrun.so whenever the code was linked with the C++
 * compiler.  If there is no enhanced atexit, and we do have do_exit_code,
 * we register the latter with atexit.  There are 5 extra slots in
 * atexit, so this will still be standard conforming.  Since the code
 * is registered after the .fini section, it runs before the library
 * cleanup code, leaving nothing for the calls to _do_exit_code_in_range
 * to handle.
 *
 * Remove this code and the associated code in libCrun when the earliest
 * system to be supported is Solaris 8.
 */
	.weak	_get_exit_frame_monitor
	.weak	__1cG__CrunMdo_exit_code6F_v_

	.section	.data
	.align	4
__get_exit_frame_monitor_ptr:
	.4byte	_get_exit_frame_monitor
	.type	__get_exit_frame_monitor_ptr,@object
	.size	__get_exit_frame_monitor_ptr,4

	.align	4
__do_exit_code_ptr:
	.4byte	__1cG__CrunMdo_exit_code6F_v_
	.type	__do_exit_code_ptr,@object
	.size	__do_exit_code_ptr,4

	.section	.text

	larl	%r2,__get_exit_frame_monitor_ptr
	lg	%r2,0(%r2)
	ltgr	%r2,%r2
	jz	2f

	larl	%r2,__do_exit_code_ptr 
	lg	%r2,0(%r2)
	ltgr	%r2,%r2
	jz	2f

	brasl	%r14,atexit@PLT
2:

/*
 * End of destructor handling code
 */

/*
 * Calculate the location of the envp array by adding the size of
 * the argv array to the start of the argv array.
 */

	larl	%r4,_environ		/* Get A(A(Environment)) */
	larl	%r10,___Argv
	lgr	%r2,%r6			/* Restore argc */
	lgr	%r3,%r7			/* Get Argv */
	lg	%r11,0(%r4)		/* Get A(Environment) */
	ltgr	%r11,%r11		/* _environ set? */
	jnz	3f			/* Yep... Skip */

	stg	%r8,0(%r4)		/* Copy to _environ */
3:
	lgr	%r2,%r6			/* Restore argc */
	stg	%r3,0(%r10)
	lg	%r4,0(%r4)		/* envp */
	lgr	%r8,%r4			/* Save envp */
	brasl	%r14,_init
	lgr	%r2,%r6			/* Restore argc - again */
	lgr	%r3,%r7			/* .... argv */
	lgr	%r4,%r8			/* .... envp */
	brasl	%r14,main
	lgr	%r8,%r2			/* Save return value */
	brasl	%r14,exit		
	lgr	%r2,%r8
	jg      _exit

	.size	_start, .-_start

/*
 * The following is here in case any object module compiled with cc -p
 *	was linked into this module.
 */
	.section	.text
	.align	4
	.globl	_mcount
	.type	_mcount,@function
_mcount:
	br	%r14
	.size	_mcount, .-_mcount

	.section	.data

	.globl	__longdouble_used
	.type	__longdouble_used,@object
	.size	__longdouble_used,4
	.align	4
__longdouble_used:
	.4byte	0x0
