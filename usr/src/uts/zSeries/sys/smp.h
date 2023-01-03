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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef SMP_H
#define SMP_H

#ifndef _ASM

#include <sys/types.h>

void smp_init(void);

/*------------------------------------------------------*/
/* Signal Processor Commands...				*/
/*------------------------------------------------------*/
typedef enum
{
	sigp_UA=0,
	sigp_Sense,
	sigp_ExtCall,
	sigp_Emergency,
	sigp_Start,
	sigp_Stop,
	sigp_Restart,
	sigp_UA1,
	sigp_UA2,
	sigp_StopStoreStatus,
	sigp_UA3,
	sigp_InitCPUReset,
	sigp_CPUReset,
	sigp_SetPrefix,
	sigp_StoreStatusAtAddress,
	sigp_StoreExtStatusAtAddress
} sigp_Command;

typedef uint32_t sigp_StatusWord;

/*------------------------------------------------------*/
/* Signal Processor Return Codes...			*/
/*------------------------------------------------------*/
typedef enum
{
	sigp_OrderCodeAccepted=0,
	sigp_StatusStored,
	sigp_Busy,
	sigp_NotOp
} sigp_Rc;

/*------------------------------------------------------*/
/* Signal Processor Operation...   			*/
/*------------------------------------------------------*/

static __inline__ sigp_Rc
sigp (uint16_t cpuid, sigp_Command cmd, uint32_t parm, void *status)
{
	sigp_Rc rc;

	__asm__ ("	lghi	2,0\n"
		 "	lgfr	3,%2\n"
		 "	sigp	2,%3,0(%4)\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 "	lg	1,%1\n"
		 "	ltgr	1,1\n"
		 "	jz	0f\n"
		 "	st	2,0(1)\n"
		 "0:\n"
		 : "=r" (rc), "+m" (status)
		 : "a" (parm), "a" (cpuid),
		   "a" (cmd)
		 : "cc" , "memory", "1", "2", "3");

	return rc;
}

#endif 

#endif 
