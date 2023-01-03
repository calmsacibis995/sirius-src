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
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved	*/


/*	Copyright (c) 1989 by Sun Microsystems, Inc.		*/

.ident	"@(#)dtstubs.s	1.8	05/06/08 SMI"	/* SVr4.0 1.8	*/

	.file	"dtstubs.s"

#include <sys/asm_linkage.h>
#include "SYS.h"

	ENTRY(__dtrace_plockstat___mutex__acquire)
	ALTENTRY(__dtrace_plockstat___mutex__block)
	ALTENTRY(__dtrace_plockstat___mutex__blocked)
	ALTENTRY(__dtrace_plockstat___mutex__error)
	ALTENTRY(__dtrace_plockstat___mutex__release)
	ALTENTRY(__dtrace_plockstat___mutex__spin)
	ALTENTRY(__dtrace_plockstat___mutex__spun)
	ALTENTRY(__dtrace_plockstat___rw__acquire)
	ALTENTRY(__dtrace_plockstat___rw__block)
	ALTENTRY(__dtrace_plockstat___rw__blocked)
	ALTENTRY(__dtrace_plockstat___rw__error)
	ALTENTRY(__dtrace_plockstat___rw__release)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___mutex__acquire)
	SET_SIZE(__dtrace_plockstat___mutex__block)
	SET_SIZE(__dtrace_plockstat___mutex__blocked)
	SET_SIZE(__dtrace_plockstat___mutex__error)
	SET_SIZE(__dtrace_plockstat___mutex__release)
	SET_SIZE(__dtrace_plockstat___mutex__spin)
	SET_SIZE(__dtrace_plockstat___mutex__spun)
	SET_SIZE(__dtrace_plockstat___rw__acquire)
	SET_SIZE(__dtrace_plockstat___rw__block)
	SET_SIZE(__dtrace_plockstat___rw__blocked)
	SET_SIZE(__dtrace_plockstat___rw__error)
	SET_SIZE(__dtrace_plockstat___rw__release)
