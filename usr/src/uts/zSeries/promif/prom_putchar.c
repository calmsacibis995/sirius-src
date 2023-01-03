/*------------------------------------------------------------------*/
/* 								    */
/* Name        - prom_putchar.c					    */
/* 								    */
/* Function    -                                                    */
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

#define USE_SCLP 1

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

#if USE_SCLP
static char promOutBuffer[3072];
static size_t bhCharPos = 0;
#else
static char promOutBuffer[3072] = "MSGNOH * AT * ";
static size_t bhCharPos = 14;
#endif

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_emit.                                        */
/*                                                                  */
/* Function	- Emit contents of buffer to output device.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
prom_emit(void)
{
	promOutBuffer[bhCharPos] = '\0';

#if USE_SCLP
	sclp_write(promOutBuffer, bhCharPos);
	bhCharPos = 0;
#else
	a2e(promOutBuffer, bhCharPos);

	__asm__("	lgr	1,%0\n"
		"	lgr	3,%1\n"
		"	lghi	4,0\n"
		"	diag	1,3,0x8\n"
		:
		: "r" (promOutBuffer), "r" (bhCharPos)
		: "1", "2", "3", "4");

	strcpy(promOutBuffer, "MSGNOH * AT * ");
	bhCharPos = strlen(promOutBuffer);;
#endif
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_mayput.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
prom_mayput(char c)
{
	prom_putchar(c);
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_putchar.                                     */
/*                                                                  */
/* Function	- Plug character into buffer and emit if we exceed  */
/*		  buffer or encounter a new-line character.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prom_putchar(char c)
{
	switch (c) {
	case '\t':
		do {
			promOutBuffer[bhCharPos] = ' ';
		} while (++bhCharPos % 8);
		break;
	case '\r':
		break;
	case '\n':
		prom_emit();
		break;
	case '\b':
		if (bhCharPos)
			bhCharPos--;
		break;
	default:
		promOutBuffer[bhCharPos++] = c;
		break;
	}

	if (bhCharPos == sizeof(promOutBuffer) - 1) {
		prom_emit();
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_writestr.                                    */
/*                                                                  */
/* Function	- Write a string                                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prom_writestr(const char *s, size_t n)
{
	while (n-- != 0)
		prom_putchar(*s++);
}

/*========================= End of Function ========================*/
