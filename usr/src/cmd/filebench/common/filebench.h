/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * Portions Copyright 2008 Denis Cheng
 */

#ifndef _FB_FILEBENCH_H
#define	_FB_FILEBENCH_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef HAVE_BOOLEAN_T
typedef enum { B_FALSE, B_TRUE } boolean_t;
#endif

#ifndef HAVE_U_LONGLONG_T
typedef unsigned long long u_longlong_t;
#endif

#include "procflow.h"
#include "misc.h"
#include "ipc.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif


#ifdef __STDC__
#include <stdarg.h>
#define	__V(x)  x
#ifndef __P
#define	__P(x)  x
#endif
#else
#include <varargs.h>
#define	__V(x)  (va_alist) va_dcl
#define	__P(x)  ()
#define	const
#endif

#include <sys/times.h>

#ifdef HAVE_SYS_INT_LIMITS_H
#include <sys/int_limits.h>
#endif /* HAVE_SYS_INT_LIMITS_H */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef TRUE
#define	TRUE 1
#endif

#ifndef FALSE
#define	FALSE 0
#endif

extern pid_t my_pid;		/* this process' process id */
extern procflow_t *my_procflow;	/* if slave process, procflow pointer */
extern int errno;
extern char *execname;
extern int noproc;

void filebench_init();
void filebench_log __V((int level, const char *fmt, ...));
void filebench_shutdown(int error);

#ifndef HAVE_UINT64_MAX
#define	UINT64_MAX (((off64_t)1UL<<63UL) - 1UL)
#endif

#define	FILEBENCH_RANDMAX64 UINT64_MAX
#define	FILEBENCH_RANDMAX32 UINT32_MAX

#if defined(_LP64) || (__WORDSIZE == 64)
#define	filebench_randomno filebench_randomno64
#define	FILEBENCH_RANDMAX FILEBENCH_RANDMAX64
#else
#define	filebench_randomno filebench_randomno32
#define	FILEBENCH_RANDMAX FILEBENCH_RANDMAX32
#endif

#define	KB (1024LL)
#define	MB (KB * KB)
#define	GB (KB * MB)

#define	MMAP_SIZE	(1024UL * 1024UL * 1024UL)

#ifndef MIN
#define	MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define	FILEBENCH_VERSION	"1.3.3"
#define	FILEBENCHDIR	"/usr/benchmarks/filebench"
#define	FILEBENCH_PROMPT	"filebench> "
#define	MAX_LINE_LEN	1024
#define	MAX_CMD_HIST	128
#define	SHUTDOWN_WAIT_SECONDS	5 /* time to wait for proc / thrd to quit */

#define	FILEBENCH_DONE	 1
#define	FILEBENCH_OK	 0
#define	FILEBENCH_ERROR -1
#define	FILEBENCH_NORSC -2

/* For MacOSX */
#ifndef HAVE_OFF64_T
#define	mmap64 mmap
#define	off64_t off_t
#define	open64 open
#define	stat64 stat
#define	pread64 pread
#define	pwrite64 pwrite
#define	lseek64 lseek
#define	fstat64 fstat
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _FB_FILEBENCH_H */
