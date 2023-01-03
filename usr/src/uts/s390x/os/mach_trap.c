/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mach_trap.c					    */
/* 								    */
/* Function    - System trap support routines.                      */
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
#include <sys/trap.h>
#include <sys/machsystm.h>
#include <sys/panic.h>
#include <sys/uadmin.h>
#include <sys/kobj.h>
#include <sys/contract/process_impl.h>
#include <sys/reboot.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern int tudebug;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

void showregs(unsigned, struct regs *, caddr_t);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- showregs.                                         */
/*                                                                  */
/* Function	- Print out debugging information.                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
showregs(uint_t type, struct regs *rp, caddr_t addr)
{
	int s;

	s = spl7();
	printf("addr=0x%p\n", (void *)addr);

	printf("pid=%d, pc=0x%lx, sp=0x%llx\n",
	    (ttoproc(curthread) && ttoproc(curthread)->p_pidp) ?
	    (ttoproc(curthread)->p_pid) : 0, rp->r_pc, rp->r_sp);
	    
	printf("R0  = %016llx %016llx %016llx %016llx\n",
	       rp->r_g0,  rp->r_g1,  rp->r_g2,  rp->r_g3);
	printf("R4  = %016llx %016llx %016llx %016llx\n",
	       rp->r_g4,  rp->r_g5,  rp->r_g6,  rp->r_g7);
	printf("R8  = %016llx %016llx %016llx %016llx\n",
	       rp->r_g8,  rp->r_g9,  rp->r_g10, rp->r_g11);
	printf("R12 = %016llx %016llx %016llx %016llx\n",
	       rp->r_g12, rp->r_g13, rp->r_g14, rp->r_g15);

	if (tudebug > 1 && (boothowto & RB_DEBUG)) {
		debug_enter((char *)NULL);
	}
	splx(s);
}

/*========================= End of Function ========================*/
