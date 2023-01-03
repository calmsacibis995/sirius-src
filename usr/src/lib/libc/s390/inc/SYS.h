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

#ifndef	_LIBC_S390_INC_SYS_H
#define	_LIBC_S390_INC_SYS_H

/*
 * This file defines common code sequences for system calls.  Note that
 * it is assumed that __cerror is within the short branch distance from
 * all the traps (so that a simple bcs can follow the trap, rather than
 * a position independent code sequence.)
 */

#include <sys/asm_linkage.h>
#include <sys/syscall.h>
#include <sys/errno.h>

/*
 * Trap number for system calls
 */
#define	SYSCALL_TRAPNUM	0

/*
 * Define the external symbol __cerror for all files.
 */
	.global	__cerror

/*
 * __SYSTRAP provides the basic trap sequence.  It assumes that an entry
 * of the form SYS_name exists (probably from sys/syscall.h).
 */
#define	__SYSTRAP(name)			\
	/* CSTYLED */			\
	lhi	%r0,SYS_##name;		\
	svc	SYSCALL_TRAPNUM

#define	SYSTRAP_RVAL1(name)		__SYSTRAP(name)
#define	SYSTRAP_RVAL2(name)		__SYSTRAP(name)
#define	SYSTRAP_2RVALS(name)		__SYSTRAP(name)
#define	SYSTRAP_64RVAL(name)		__SYSTRAP(name)

/*
 * SYSFASTTRAP provides the fast system call trap sequence.  It assumes
 * that an entry of the form ST_name exists (probably from sys/trap.h).
 */
#define	SYSFASTTRAP(name)		\
	/* CSTYLED */			\
	svc	ST_##name

/*
 * SYSCERROR provides the sequence to branch to __cerror if an error is
 * indicated by the R0 being non-zero
 */
#define	SYSCERROR			\
	/* CSTYLED */			\
	ltr	%r0,%r0;		\
	jgnz	__cerror

/*
 * SYSCERROR64 provides the sequence to branch to __cerror64 if an error is
 * indicated by the R0 being non-zero
 */
#define	SYSCERROR64			\
	ltr	%r0,%r0;		\
	jgnz	__cerror64

/*
 * SYSLWPERR provides the sequence to return 0 on a successful trap
 * and the error code if unsuccessful.
 * Error is indicated R0 being non zero
 */
#define	SYSLWPERR			\
	/* CSTYLED */			\
	ltr	0,0;			\
	jz	1f;			\
	chi	2,ERESTART;		\
	jne	2f;			\
	lhi	2,EINTR;		\
	j	2f;			\
1:	lhi	2,0;			\
2:

/*
 * SYSREENTRY provides the entry sequence for restartable system calls.
 */
#define	SYSREENTRY(name)			\
	ENTRY(name);				\
	st	2,4(15);			\
/* CSTYLED */					\
.restart_##name:;				\
.restart##name:

/*
 * SYSRESTART provides the error handling sequence for restartable
 * system calls.
 */
#define	SYSRESTART(name)					\
	/* CSTYLED */						\
	ltr	0,0;						\
	jz	1f;						\
	chi	2,ERESTART;					\
	jgne	__cerror;					\
	l	2,4(15);					\
	j	name;						\
1:

/*
 * SYSINTR_RESTART provides the error handling sequence for restartable
 * system calls in case of EINTR or ERESTART.
 */
#define	SYSINTR_RESTART(name)					\
	ltr	0,0;						\
	jz	1f;						\
	chi	2,ERESTART;					\
	je	2f;						\
	chi	2,EINTR;					\
	jne	3f;						\
2:	l	2,4(15);					\
	j	name;						\
1:	lhi	2,0;						\
3:

/*
 * SYSCALL provides the standard (i.e.: most common) system call sequence.
 */
#define	SYSCALL(name)						\
	ENTRY(name);						\
	SYSTRAP_2RVALS(name);					\
	SYSCERROR

#define	SYSCALL_RVAL1(name)					\
	ENTRY(name);						\
	SYSTRAP_RVAL1(name);					\
	SYSCERROR

/*
 * SYSCALL64 provides the standard (i.e.: most common) system call sequence
 * for system calls that return 64-bit values.
 */
#define	SYSCALL64(name)						\
	ENTRY(name);						\
	SYSTRAP_64RVAL(name);					\
	SYSCERROR64

/*
 * SYSCALL_RESTART provides the most common restartable system call sequence.
 */
#define	SYSCALL_RESTART(name)					\
	SYSREENTRY(name);					\
	SYSTRAP_2RVALS(name);					\
	/* CSTYLED */						\
	SYSRESTART(.restart_##name)

#define	SYSCALL_RESTART_RVAL1(name)				\
	SYSREENTRY(name);					\
	SYSTRAP_RVAL1(name);					\
	/* CSTYLED */						\
	SYSRESTART(.restart_##name)

/*
 * SYSCALL2 provides a common system call sequence when the entry name
 * is different than the trap name.
 */
#define	SYSCALL2(entryname, trapname)				\
	ENTRY(entryname);					\
	SYSTRAP_2RVALS(trapname);				\
	SYSCERROR

#define	SYSCALL2_RVAL1(entryname, trapname)			\
	ENTRY(entryname);					\
	SYSTRAP_RVAL1(trapname);				\
	SYSCERROR

/*
 * SYSCALL2_RESTART provides a common restartable system call sequence when the
 * entry name is different than the trap name.
 */
#define	SYSCALL2_RESTART(entryname, trapname)			\
	SYSREENTRY(entryname);					\
	SYSTRAP_2RVALS(trapname);				\
	/* CSTYLED */						\
	SYSRESTART(.restart_##entryname)

#define	SYSCALL2_RESTART_RVAL1(entryname, trapname)		\
	SYSREENTRY(entryname);					\
	SYSTRAP_RVAL1(trapname);				\
	/* CSTYLED */						\
	SYSRESTART(.restart_##entryname)

/*
 * SYSCALL_NOERROR provides the most common system call sequence for those
 * system calls which don't check the error return (carry bit).
 */
#define	SYSCALL_NOERROR(name)					\
	ENTRY(name);						\
	SYSTRAP_2RVALS(name)

#define	SYSCALL_NOERROR_RVAL1(name)				\
	ENTRY(name);						\
	SYSTRAP_RVAL1(name)

/*
 * Standard syscall return sequence, return code equal to rval1.
 */
#define	RET			\
	br	%r14

/*
 * Syscall return sequence, return code equal to rval2.
 */
#define	RET2			\
	lr	%r2,%r3;	\
	br	%r14

/*
 * Syscall return sequence with return code forced to zero.
 */
#define	RETC			\
	lhi	%r2,0;		\
	br	%r14

#endif	/* _LIBC_S390_INC_SYS_H */
