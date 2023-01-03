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
/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Useful asm routines and data types for grungy hacking */

#define EXCPMASK (FP_X_INV|FP_X_DZ|FP_X_OFL|FP_X_UFL|FP_X_IMP)

struct fpc {
	int  exMasks : 5;	/* Exception masks */
	int  exFillM : 3;	/* Filler */
	int  exFlags : 5;	/* Exceptions flags */
	int  exFillF : 3;	/* Filler */
	char DXC;		/* Data exception code */
	int  rndFill : 6;	/* Filler */
	int  rnd     : 2;	/* Rounding mode */	
};

extern void _getcw(struct fpc *);
extern void _putcw(struct fpc *);
