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

#ifndef	_SYS_MACHTHREAD_H
#define	_SYS_MACHTHREAD_H

#include <sys/bitmap.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	_ASM

/*
 * Get the processor implementation from the version register.
 */
#define	CPU_MASK	0x3ff

/*
 * CPU_INDEX(r, scr)
 * Returns cpu id in r.
 */
#define	CPU_INDEX(r, scr)		\
	stap	scr;			\
	llgh	r,scr	

/*
 * CPU_ADDR(r, scr)
 * Returns pointer to CPU structure
 */
#define	CPU_ADDR(r, scr)		\
	larl	%r1,cpu;		\
	stap	scr;			\
	llgh	r,scr;			\
	sllg	r,r,3;			\
	agr	r,%r1		

/*
 * Given a cpu id extract the appropriate word
 * in the cpuset mask for this cpu id.
 */
#define	CPU_INDEXTOSET(base, index, scr)	

#endif	/* _ASM */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_MACHTHREAD_H */
