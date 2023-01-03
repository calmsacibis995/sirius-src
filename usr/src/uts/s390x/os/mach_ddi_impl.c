/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mach_ddi_impl.c				    */
/* 								    */
/* Function    -                                                    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - April, 2007 					    */
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

#include <sys/bootconf.h>
#include <sys/conf.h>
#include <sys/machsystm.h>
#include <sys/sunddi.h>
#include <sys/promif.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern int status_okay(int, char *, int);

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*
 * Favored drivers of this implementation
 * architecture.  These drivers MUST be present for
 * the system to boot at all.
 */
char *impl_module_list[] = {
	"options",
	"sad",		/* Referenced via init_tbl[] */
	"pseudo",
	"ccw",
	"mm",
	(char *)0
};
/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- check_status.                                     */
/*                                                                  */
/* Function	- Check the status of the device node passed as an  */
/*		  argument.                    		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
check_status(int id, char *buf, dev_info_t *parent)
{
	char status_buf[64];

	/*
	 * is the status okay?
	 */
	if (status_okay(id, status_buf, sizeof (status_buf)))
		return (DDI_SUCCESS);

	return (DDI_FAILURE);
}

/*========================= End of Function ========================*/
