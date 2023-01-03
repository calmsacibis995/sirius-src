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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * @(#)libm.m4	1.0	08/03/09 LLL
 *
 */
undefine(`_C')dnl
define(`_C',`')dnl
define(NAME,$1)dnl
dnl
ifdef(`LOCALLIBM',`

#define __get_rd(X)        (X&0x03)                                                                                            
#define __set_rd(X,Y)      X=(X&~0x00000003ul)|Y                                                                               
                                                                                                                                    
#define __get_te(X)        ((X>>27)&0x1f)                                                                                      
#define __set_te(X,Y)      X=(X&~0xf8000000ul)|((Y)<<27)                                                                       
                                                                                                                                    
#define __get_ex(X)        ((X>>19)&0x1f)                                                                                      
#define __set_ex(X,Y)      X=(X&~0x00f80000ul)|((Y)<<19)                                                                       

static __inline__ void __getfsr(unsigned long *fsr)
{
#if defined(__s390x)
	__asm__ ("	lghi	1,0\n"
		 "	efpc	1\n"
		 "	stg	1,0(%0)\n"
		 :
		 : "r" (fsr)
		 : "memory", "1");
#else
	__asm__ ("	stfpc	0(%0)\n"
		 :
		 : "r" (fsr)
		 : "memory");
#endif
}

static __inline__ void __setfsr(const unsigned long *fsr)
{
	__asm__ ("	sfpc	%0\n"
		 :
		 : "r" (*fsr));
}

static __inline__ int __swapRP(int type)
{
	return 0;
}

static __inline__ int __swapTE(int traps)
{
	unsigned long fsr, new;

	__getfsr(&fsr);

	new = fsr;

	__set_te(new, traps);

	return __get_te(fsr);
}

static __inline__ int __swapEX(int excs)
{
	unsigned long fsr, new;

	__getfsr(&fsr);

	new = fsr;

	__set_ex(new, excs);

	return __get_ex(fsr);
}

static __inline__ int __swapRD(int mode)
{
	unsigned long fsr, new;

	__getfsr(&fsr);

	new = fsr;

	__set_rd(new, mode);

	return __get_rd(fsr);
}
')dnl
