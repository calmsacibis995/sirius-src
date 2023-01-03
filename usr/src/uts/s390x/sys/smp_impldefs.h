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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_SMP_IMPLDEFS_H
#define	_SYS_SMP_IMPLDEFS_H

#include <sys/types.h>
#include <sys/sunddi.h>
#include <sys/cpuvar.h>
#include <sys/avintr.h>
#include <sys/pic.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* timer modes for clkinitf */
#define	TIMER_ONESHOT		0x1
#define	TIMER_PERIODIC		0x2

/*
 *	External Reference Functions
 */

extern int (*slvltovect)(int);	/* ipl interrupt priority level		*/
extern void setlvl(int);	/* set intr pri to specified level	*/
extern void setspl(int);	/* mask intr below or equal given ipl	*/
extern int (*addspl)(int, int, int, int); /* add intr mask of vector 	*/
extern int (*delspl)(int, int, int, int); /* delete intr mask of vector */
extern void kdi_av_set_softint_pending(); /* kmdb private entry point */

/* trigger a software intr */
extern void (*setsoftint)(int, struct av_softinfo *);

/* kmdb private entry point */
extern void (*kdisetsoftint)(int, struct av_softinfo *);

extern void av_set_softint_pending();	/* set software interrupt pending */

/*
 *	External Reference Data
 */
extern struct av_head autovect[]; /* array of auto intr vectors		*/

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SMP_IMPLDEFS_H */
