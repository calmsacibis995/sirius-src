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
 * Copyright 1987-1997,2002-2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_FRAME_H
#define	_SYS_FRAME_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Definition of the System z stack frame (when it is pushed on the stack).
 */
struct frame {
	long	fr_bc;			/* Backchain pointer (opt) */
	long	fr_resv;		/* Reserved */
	long	fr_argd[4];		/* Function arguments (if saved) */
	long	fr_regs[10];		/* R6-R15 (if saved) */
	double	fr_fpr[4];		/* Floating point registers (if saved) */
	long	fr_argx[1];		/* array of args past the fifth */
};

#define fr_savpc	fr_regs[8]
#define fr_savfp	fr_regs[9]

#ifdef _SYSCALL32
/*
 * Kernels view of a 32-bit stack frame
 */
struct frame32 {
	uint32_t fr_bc;			/* Backchain pointer (opt) */
	uint32_t fr_resv;		/* Reserved */
	uint32_t fr_argd[4];		/* Function arguments (if saved) */
	uint32_t fr_regs[10];		/* R6-R15 (if saved) */
	double	 fr_fpr[2];		/* Floating point registers (if saved) */
	uint32_t fr_argx[1];		/* array of args past the fifth */
};
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_FRAME_H */
