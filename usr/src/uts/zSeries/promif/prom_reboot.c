/*------------------------------------------------------------------*/
/* 								    */
/* Name        - prom_reboot.c					    */
/* 								    */
/* Function    - Reboot the system.                                 */
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

#include <sys/promif.h>
#include <sys/promimpl.h>
#include <sys/archsystm.h>
#include <sys/machs390x.h>

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
/* Name		- prom_reboot_prompt.                               */
/*                                                                  */
/* Function	- Prompt the user to press a key to start reboot.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prom_reboot_prompt(void)
{
	prom_printf("Press any key to reboot.\n");
	(void) prom_getchar();
	prom_printf("Resetting...\n");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_reboot.                                      */
/*                                                                  */
/* Function	- Reset the system.                                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
prom_reboot(char *bootstr)
{
	quiesce_cpu();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_panic.                                       */
/*                                                                  */
/* Function	- Print a panic message and reboot.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prom_panic(char *panicMsg)
{
	char *msg;

	if (panicMsg == NULL)
		msg = "Unknown panic condition encountered ";
	else	
		msg = panicMsg;

	prom_printf("%s called from %p\n",msg,__builtin_return_address(0));
	prom_reboot_prompt();
	prom_reboot(NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bop_panic.                                        */
/*                                                                  */
/* Function	- Print a panic message and reboot.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
bop_panic(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	prom_vprintf(fmt, args);
	va_end(args);
	prom_reboot_prompt();
	prom_reboot(NULL);
}

/*========================= End of Function ========================*/
