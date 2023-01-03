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
 *                                                              
 * Copyright 2008 Sine Nomine Associates.                        
 * All rights reserved.                                           
 * Use is subject to license terms.                                
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_INTREG_H
#define	_SYS_INTREG_H

#ifdef	__cplusplus
extern "C" {
#endif

#define	IGN_SIZE	5			/* Interrupt Group Number bit size */
#define	INO_SIZE	6			/* Interrupt Number Offset bit size */
#define	INR_SIZE	(IGN_SIZE + INO_SIZE)	/* Interrupt Number bit size */
#define	MAX_IGN		(1 << IGN_SIZE) 	/* Max Interrupt Group Number size */
#define	MAX_INO		(1 << INO_SIZE) 	/* Max Interrupt Number per group */
#define	MAXIVNUM	(MAX_IGN * MAX_INO) 	/* Max hardware intrs allowed */

/*
 * Per-Processor Soft Interrupt Register
 */
#define	SOFTINT_MASK	0xFFFE		/* <15:1> */
#define	TICK_INT_MASK	0x1		/* <0> */
#define	STICK_INT_MASK	0x10000		/* <0> */


#ifndef _ASM

/*
 * Leftover bogus stuff; removed them later
 */
struct cpu_intreg {
	uint_t	pend;
	uint_t	clr_pend;
	uint_t	set_pend;
	uchar_t	filler[0x1000 - 0xc];
};

struct sys_intreg {
	uint_t	sys_pend;
	uint_t	sys_m;
	uint_t	sys_mclear;
	uint_t	sys_mset;
	uint_t	itr;
};

#endif  /* _ASM */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_INTREG_H */
