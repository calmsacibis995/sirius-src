/*------------------------------------------------------------------*/
/* 								    */
/* Name        - prom_version.			   		    */
/* 								    */
/* Function    - Simulate returning version string from PROM.       */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - September, 2007 				    */
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

#define	PROM_VERSION_NUMBER 4	/* Obsolete under new boot */

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/promif.h>
#include <sys/promimpl.h>
#include <sys/obpdefs.h>
#include <sys/types.h>
#include <sys/kmem.h>

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
/* Name		- prom_getversion.                                  */
/*                                                                  */
/* Function	- Return PROM version number.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prom_getversion(void)
{
	return (PROM_VERSION_NUMBER);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_is_openprom.                                 */
/*                                                                  */
/* Function	- Humor /dev/openprom by returning true.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prom_is_openprom(void)
{
	return (1);	
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_version_name.                                */
/*                                                                  */
/* Function	- Return a string representing the prom version.    */
/*		  In this case a string representing the version of */
/*		  devconf that created the device tree will be 	    */
/*		  returned. Return the actual length of the string  */
/*		  including the NULL terminator.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
prom_version_name(char *buf, int len)
{
	static char prom_versionstr[] = "DevConf 2.0";
	extern size_t strlcpy(char *, const char *, size_t);

	(void) strlcpy(buf, prom_versionstr, len - 1);
	return (sizeof (prom_versionstr));
}

/*========================= End of Function ========================*/
