/*------------------------------------------------------------------*/
/* 								    */
/* Name        - polled_io.c					    */
/* 								    */
/* Function    - Polled I/O support routines - all no-ops as s390x  */
/* 		 does not support polled I/O.			    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
/* 								    */
/*------------------------------------------------------------------*/

/*
 * consconfig is aware of which devices are the stdin and stout.  The
 * post-attach/pre-detach functions are an extension of consconfig because
 * they know about the dynamic changes to the stdin device.  Neither an
 * individual driver nor the DDI framework knows what device is really the
 * stdin.
 */

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

#include <sys/stropts.h>
#include <sys/devops.h>
#include <sys/modctl.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/promif.h>
#include <sys/note.h>
#include <sys/consdev.h>
#include <sys/polled_io.h>

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

int	polled_debug = 0;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- polled_io_init.                                   */
/*                                                                  */
/* Function	- Initialize polled I/O support - no-operation.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
polled_io_init(void)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- polled_io_register_callbacks.                     */
/*                                                                  */
/* Function	- Register a device for input or output - no-op.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
polled_io_register_callbacks(
cons_polledio_t			*polled_io,
int				flags
)
{
_NOTE(ARGUNUSED(flags))
	cons_polledio = NULL;

	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- polled_io_unregister_callbacks.                   */
/*                                                                  */
/* Function	- Unregister a device for input or output - no-op.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
polled_io_unregister_callbacks(
cons_polledio_t			*polled_io,
int				flags
)
{
_NOTE(ARGUNUSED(polled_io,flags))

	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- polled_io_fini.                                   */
/*                                                                  */
/* Function	- Terminate support of polled I/O - no-operation.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
polled_io_fini()
{
}

/*========================= End of Function ========================*/
