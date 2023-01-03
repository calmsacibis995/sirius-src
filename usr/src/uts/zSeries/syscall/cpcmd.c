/*------------------------------------------------------------------*/
/* 								    */
/* Name        - cpcmd.c    					    */
/* 								    */
/* Function    - Execute a CP command on behalf of an authorized    */
/* 		 user and return the results.			    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - June, 2008      				    */
/* 								    */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/*                   L I C E N S E                                  */
/*------------------------------------------------------------------*/

/*==================================================================*/
/* 								    */
/* CDDL HEADER START						    */
/* 								    */
/* The contents of this file are subject to the terms of the	    */
/* Common Development and Distribution License                      */
/* (the "License").  You may not use this file except in compliance */
/* with the License.						    */
/* 								    */
/* You can obtain a copy of the license at: 			    */
/* - usr/src/OPENSOLARIS.LICENSE, or,				    */
/* - http://www.opensolaris.org/os/licensing.			    */
/* See the License for the specific language governing permissions  */
/* and limitations under the License.				    */
/* 								    */
/* When distributing Covered Code, include this CDDL HEADER in each */
/* file and include the License file at usr/src/OPENSOLARIS.LICENSE.*/
/* If applicable, add the following below this CDDL HEADER, with    */
/* the fields enclosed by brackets "[]" replaced with your own      */
/* identifying information: 					    */
/* Portions Copyright [yyyy] [name of copyright owner]		    */
/* 								    */
/* CDDL HEADER END						    */
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
/* 								    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

#define MAXCMDLEN	240
#define MAXRESLEN	4096

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/asm_linkage.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/cred.h>
#include <sys/thread.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- cpcmd.                                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
cpcmd(char *cmd, char *result, ssize_t resize, int *resLen)
{
	char	cmdBuf[MAXCMDLEN],
		*resBuf;
	int	error,
		lResult,
		status;
	size_t	cmdLen;
	cred_t	*cr;

	cr = CRED();

	if (crgetuid(cr) != 0)
		return(set_errno(EPERM));

	if (error = copyinstr(cmd, cmdBuf, MAXCMDLEN, &cmdLen) != 0)
		return (set_errno(EFAULT));

	if ((resize < 0) || (resize > MAXRESLEN))
		return (set_errno(EINVAL));

	a2e(cmdBuf, --cmdLen);

	resBuf = kmem_alloc(resize, KM_SLEEP);

	__asm__ ("	lrag	0,0(%3)\n"
		 "	lgr	2,%4\n"
		 "	lg	1,%0\n"
		 "	lrag	1,0(1)\n"
		 "	lgfr	3,%5\n"
		 "	oilh	2,0xe000\n"
		 "	diag	0,2,0x08\n"
		 "	lgfr	%1,2\n"
		 "	jz	0f\n"
		 "	lnr	3,3\n"
		 "0:\n"
		 "	lgfr	%2,3\n"
		 : "=m" (resBuf), "=r" (status), "=r" (lResult)
		 : "r" (cmdBuf), "r" (cmdLen), "r" (resize));

	if (lResult > 0) {
		e2a(resBuf, lResult);
		error = copyout(resBuf, result, lResult, NULL);
	} else {
		e2a(resBuf, resize);
		error = copyout(resBuf, result, resize, NULL);
	}
	error = copyout(&lResult, resLen, sizeof(lResult), NULL);

	kmem_free(resBuf, resize);

	if (error != 0)
		return (set_errno(EFAULT));
	 
	return(status);
}

/*========================= End of Function ========================*/
