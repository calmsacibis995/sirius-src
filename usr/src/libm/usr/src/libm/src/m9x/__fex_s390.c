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
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#if defined(__s390) || defined(__s390x)
#include "fenv_synonyms.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <siginfo.h>
#include <thread.h>
#include <ucontext.h>
#include <math.h>
#include <fenv.h>
#include "fex_handler.h"

/*
*  Determine which type of invalid operation exception occurred
*/
enum fex_exception
__fex_get_invalid_type(siginfo_t *sip, ucontext_t *uap)
{
	return (enum fex_exception) -1;
}

/*
*  Get the operands, generate the default untrapped result with
*  exceptions, and set a code indicating the type of operation
*/
void
__fex_get_op(siginfo_t *sip, ucontext_t *uap, fex_info_t *info)
{
	unsigned long fsr;
	unsigned short instr;

	/* parse the instruction which caused the exception */
	instr = *((unsigned short *)uap->uc_mcontext.psw.pc);

	/* XXX fixme - Need to decode operands  */
	info->op1.type = fex_nodata;
	info->op2.type = fex_nodata;
	info->res.type = fex_nodata;

	switch (instr) {
	case 0xb34a: /* AXBR - ADD (extended) */
	case 0xb31a: /* ADBR - ADD (long) */
	case 0xb30a: /* AEBR - ADD (short) */
	case 0xed1a: /* ADB - ADD (long) */
	case 0xed0a: /* AEB - ADD (short) */
		info->op = fex_add;
		break;

	case 0xb34b: /* SXBR - SUBTRACT (extended) */
	case 0xb31b: /* SDBR - SUBTRACT (long) */
	case 0xb30b: /* SEBR - SUBTRACT (short) */
	case 0xed1b: /* SDB - SUBTRACT (long) */
	case 0xed0b: /* SEB - SUBTRACT (short) */
		info->op = fex_sub;
		break;

	case 0xb34c: /* MXBR - MULTIPLY (extended) */
	case 0xb31c: /* MDBR - MULTIPLY (long) */
	case 0xb30c: /* MDEBR - MULTIPLY (short to long) */
	case 0xed1c: /* MDB - MULTIPLY (long) */
	case 0xed0c: /* MDEB - MULTIPLY (short to long) */

	case 0xb317: /* MEEBR - MULTIPLY (short) */
	case 0xed17: /* MEEB - MULTIPLY (short) */

	case 0xb307: /* MXDBR - MULTIPLY (long to extended) */
	case 0xed07: /* MXDB - MULTIPLY (long to extended) */

	case 0xb31e: /* MADBR - MULTIPLY AND ADD (long) */
	case 0xed1e: /* MADB - MULTIPLY AND ADD (long) */

	case 0xb30e: /* MAEBR - MULTIPLY AND ADD (short) */
	case 0xed0e: /* MAEB - MULTIPLY AND ADD (short) */

	case 0xb31f: /* MSDBR - MULTIPLY AND SUBTRACT (long) */
	case 0xed1f: /* MSDB - MULTIPLY AND SUBTRACT (long) */

	case 0xb30f: /* MSEBR - MULTIPLY AND SUBTRACT (short) */
	case 0xed0f: /* MSEB - MULTIPLY AND SUBTRACT (short) */
		info->op = fex_mul;
		break;

	case 0xb34d: /* DXBR - DIVIDE (extended) */
	case 0xb31d: /* DDBR - DIVIDE (long) */
	case 0xb30d: /* DEBR - DIVIDE (short) */
	case 0xed1d: /* DDB - DIVIDE (long) */
	case 0xed0d: /* DEB - DIVIDE (short) */

	case 0xb35b: /* DIDBR - DIVIDE TO INTEGER (long) */
	case 0xb353: /* DIEBR - DIVIDE TO INTEGER (short) */
		info->op = fex_div;
		break;

	default:
		info->op = fex_other;
		break;
	}

	__fenv_getfsr(&fsr);
	info->flags = (int)__fenv_get_ex(fsr);
	__fenv_set_ex(fsr, 0);
	__fenv_setfsr(&fsr);
}

/*
*  Store the specified result; if no result is given but the exception
*  is underflow or overflow, supply the default trapped result
*/
void
__fex_st_result(siginfo_t *sip, ucontext_t *uap, fex_info_t *info)
{
	unsigned		instr, opf, rs1, rs2, rd;
	long double		qscl;
	double			dscl;
	float			fscl;

	/* Can't do anything just yet */
	return;
}
#endif	/* defined(__sparc) */
