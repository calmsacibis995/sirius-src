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
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved	*/


/*	Copyright (c) 1989 by Sun Microsystems, Inc.		*/

	.file	"dtstubs.s"

#include <sys/asm_linkage.h>
#include "SYS.h"

/* 
 * S390X FIXME - required until native solaris linker is used to 
 * create the PT_SUNWDTRACE program header
 */

	ENTRY(__dtrace_plockstat___mutex__acquire)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___mutex__acquire)
	ENTRY(__dtrace_plockstat___mutex__block)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___mutex__block)
	ENTRY(__dtrace_plockstat___mutex__blocked)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___mutex__blocked)
	ENTRY(__dtrace_plockstat___mutex__error)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___mutex__error)
	ENTRY(__dtrace_plockstat___mutex__release)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___mutex__release)
	ENTRY(__dtrace_plockstat___mutex__spin)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___mutex__spin)
	ENTRY(__dtrace_plockstat___mutex__spun)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___mutex__spun)
	ENTRY(__dtrace_plockstat___rw__acquire)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___rw__acquire)
	ENTRY(__dtrace_plockstat___rw__block)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___rw__block)
	ENTRY(__dtrace_plockstat___rw__blocked)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___rw__blocked)
	ENTRY(__dtrace_plockstat___rw__error)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___rw__error)
	ENTRY(__dtrace_plockstat___rw__release)
	lghi	%r2,0
	RET
	SET_SIZE(__dtrace_plockstat___rw__release)
