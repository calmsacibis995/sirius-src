/*------------------------------------------------------------------*/
/* 								    */
/* Name        - prom_getchar.c					    */
/* 								    */
/* Function    - Get a character from the HMC.                      */
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


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/promif.h>
#include <sys/promimpl.h>

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

static char promInBuffer[3072];
static size_t bhCharPos;
static size_t bhCharSize;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_getchar.                                     */
/*                                                                  */
/* Function	- Always return a character; waits for input if     */
/*		  none is available.           		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uchar_t
prom_getchar(void)
{
	uchar_t ch;

	if (bhCharPos == 0)
		bhCharSize = sclp_read(promInBuffer, sizeof(promInBuffer));
	
	ch = promInBuffer[bhCharPos++];
	if (bhCharPos >= bhCharSize)
		bhCharPos = 0;

	return (ch);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_mayget.                                      */
/*                                                                  */
/* Function	- Returns a character from the HMC if on is avail-  */
/*		  available, otherwise return -1. 	    	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * prom_mayget
 *
 * returns a character from the keyboard if one is available,
 * otherwise returns a -1.
 */
int
prom_mayget(void)
{
	if (bhCharPos == 0)
		return (-1);
	else
		return (prom_getchar());
}

/*========================= End of Function ========================*/
