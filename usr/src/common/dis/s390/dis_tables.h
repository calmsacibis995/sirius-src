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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/


#ifndef _DIS_TABLES_H
#define	_DIS_TABLES_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * Constants and prototypes for the IA32 disassembler backend.  See dis_tables.c
 * for usage information and documentation.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/inttypes.h>
#include <sys/param.h>

#define	OPLEN	256
#define	PFIXLEN	  8
#define	NCPS	12	/* number of chars per symbol	*/

/*
 * data structures that must be provided to dtrace_dis390x()
 */
typedef struct dis390x {
	int		d390x_flags;
	int		(*d390x_check_func)(void *);
	int		(*d390x_get_bytes)(void *, uchar_t *, int);
	int		(*d390x_sym_lookup)(void *, uint64_t, char *, size_t);
	int		(*d390x_sprintf_func)(char *, size_t, const char *, ...);
	void		*d390x_data;
} dis390x_t;

extern int dtrace_dis390x(dis390x_t *, uint64_t, char *, size_t);

#define	DIS_F_OCTAL	0x1	/* Print all numbers in octal */
#define	DIS_F_NOIMMSYM	0x2	/* Don't print symbols for immediates (.o) */


#ifdef __cplusplus
}
#endif

#endif /* _DIS_TABLES_H */
