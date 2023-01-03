/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mach_mp_startup.				    */
/* 								    */
/* Function    - SMP startup support routines.                      */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
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


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/machsystm.h>
#include <sys/dtrace.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

struct mp_find_cpu_arg {
	short cpuid;		/* set by mp_cpu_configure() */
	dev_info_t *dip;	/* set by mp_find_cpu() */
};

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

/*
 * Useful for disabling MP bring-up for an MP capable kernel
 * (a kernel that was built with MP defined)
 */
int use_mp = 1;			/* set to come up mp */

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- init_cpu_info.                                    */
/*                                                                  */
/* Function	- Initialize CPU information.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
init_cpu_info(struct cpu *cp)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_cpu_unconfigure.                               */
/*                                                                  */
/* Function	- Routine used to cleanup a CPU that has been       */
/*		  removed from the configuration. This will destroy */
/*		  all per-cpu information related to this CPU.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
mp_cpu_unconfigure(short cpuid)
{
	int retval;

	return (retval);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_find_cpu.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
mp_find_cpu(dev_info_t *dip, void *arg)
{
	struct mp_find_cpu_arg *target = (struct mp_find_cpu_arg *)arg;
	char *type;
	short cpuid;
	int rv;

	return (rv);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mp_cpu_configure.                                 */
/*                                                                  */
/* Function	- Setup a newly inserted CPU in preparation for     */
/*		  starting it running code.    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
mp_cpu_configure(short cpuid)
{
	extern void setup_cpu_common(int);
	struct mp_find_cpu_arg target;

	ASSERT(MUTEX_HELD(&cpu_lock));

	target.dip = NULL;
	target.cpuid = cpuid;

	if (target.dip == NULL)
		return (ENODEV);

	setup_cpu_common(cpuid);

	return (0);
}

/*========================= End of Function ========================*/
