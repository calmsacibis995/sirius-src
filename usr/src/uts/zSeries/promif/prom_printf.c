/*------------------------------------------------------------------*/
/* 								    */
/* Name        - prom_printf.					    */
/* 								    */
/* Function    - Provide a printf() capability for writing to the   */
/* 		 HMC.						    */
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
#include <sys/varargs.h>

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

static void _doprint(const char *, va_list, void (*)(char, char **), char **);
static void _printn(uint64_t, int, int, int, void (*)(char, char **), char **);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static char msgbuf[256];
static size_t msgpos;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- urgentv.                                          */
/*                                                                  */
/* Function	- Output an urgent message via CP MSG facility.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
urgentv(const char *fmt, va_list val)
{
	int i;

	strcpy(msgbuf, "MSG * AT * ");
	msgpos = strlen(msgbuf);

	msgpos += vsnprintf(&msgbuf[msgpos],
			    sizeof(msgbuf) - msgpos,
			    fmt,
			    val);

	a2e(msgbuf,  msgpos);

	__asm__("	la	1,0(%0)\n"
		"	la	1,0(%0)\n"
		"	lgr	3,%1\n"
		"	lghi	4,0\n"
		"	diag	1,3,0x8\n"
		:
		: "r" (msgbuf), "r" (msgpos)
		: "1", "2", "3", "4");
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- urgent.                                           */
/*                                                                  */
/* Function	- Call urgentv to write formatted urgent messages.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
urgent(const char *fmt, ...)
{
	va_list val;

	va_start(val, fmt);
	urgentv(fmt, val);
	va_end(val);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _pput.                                            */
/*                                                                  */
/* Function	- Put a single character to the HMC buffer.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
_pput(char c, char **p)
{
	(void) prom_putchar(c);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _sput.                                            */
/*                                                                  */
/* Function	- Plug a character into a buffer and increment the  */
/*		  pointer.                     		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
_sput(char c, char **p)
{
	**p = c;
	*p += 1;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_printf.                                      */
/*                                                                  */
/* Function	- Manage the printf() operation to the HMC.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*VARARGS1*/
void
prom_printf(const char *fmt, ...)
{
	va_list adx;

	va_start(adx, fmt);
	(void) _doprint(fmt, adx, _pput, (char **)0);
	va_end(adx);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_vprintf.                                     */
/*                                                                  */
/* Function	- Provide a vprintf() facility for the HMC.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prom_vprintf(const char *fmt, va_list adx)
{
	va_list tadx;

	va_copy(tadx, adx);
	(void) _doprint(fmt, tadx, _pput, (char **)0);
	va_end(tadx);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prom_sprintf.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*VARARGS2*/
char *
prom_sprintf(char *s, const char *fmt, ...)
{
	char *bp = s;
	va_list adx;

	va_start(adx, fmt);
	(void) _doprint(fmt, adx, _sput, &bp);
	*bp++ = (char)0;
	va_end(adx);
	return (s);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

char *
prom_vsprintf(char *s, const char *fmt, va_list adx)
{
	char *bp = s;

	(void) _doprint(fmt, adx, _sput, &bp);
	*bp++ = (char)0;
	return (s);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _doprint.                                         */
/*                                                                  */
/* Function	- Perform the formatting and substitution required. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
_doprint(const char *fmt, va_list adx, void (*emit)(char, char **), char **bp)
{
	int b, c, i, pad, width, ells;
	register char *s;
	int64_t	l;
	uint64_t ul;

	if (panicstr) {
		urgentv(fmt, adx);
		return;
	}

	mutex_enter(&prom_lock);
loop:
	width = 0;
	while ((c = *fmt++) != '%') {
		if (c == '\0') {
			mutex_exit(&prom_lock);
			return;
		}
		if (c == '\n')
			(*emit)('\r', bp);
		(*emit)(c, bp);
	}

	c = *fmt++;

	for (pad = ' '; c == '0'; c = *fmt++)
		pad = '0';

	for (width = 0; c >= '0' && c <= '9'; c = *fmt++)
		width = (width * 10) + (c - '0');

	for (ells = 0; c == 'l'; c = *fmt++)
		ells++;

	switch (c) {

	case 'd':
	case 'D':
		b = 10;
		if (ells == 0)
			l = (int64_t)va_arg(adx, int);
		else if (ells == 1)
			l = (int64_t)va_arg(adx, long);
		else
			l = (int64_t)va_arg(adx, int64_t);
		if (l < 0) {
			(*emit)('-', bp);
			width--;
			ul = -l;
		} else
			ul = l;
		goto number;

	case 'p':
		ells = 1;
		/* FALLTHROUGH */
	case 'x':
	case 'X':
		b = 16;
		goto u_number;

	case 'u':
		b = 10;
		goto u_number;

	case 'o':
	case 'O':
		b = 8;
u_number:
		if (ells == 0)
			ul = (uint64_t)va_arg(adx, uint_t);
		else if (ells == 1)
			ul = (uint64_t)va_arg(adx, ulong_t);
		else
			ul = (uint64_t)va_arg(adx, uint64_t);
number:
		_printn(ul, b, width, pad, emit, bp);
		break;

	case 'c':
		b = va_arg(adx, int);
		for (i = 24; i >= 0; i -= 8)
			if ((c = ((b >> i) & 0x7f)) != 0) {
				if (c == '\n')
					(*emit)('\r', bp);
				(*emit)(c, bp);
			}
		break;
	case 's':
		s = va_arg(adx, char *);
		while ((c = *s++) != 0) {
			if (c == '\n')
				(*emit)('\r', bp);
			(*emit)(c, bp);
		}
		break;

	case '%':
		(*emit)('%', bp);
		break;
	}
	goto loop;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _printn.                                          */
/*                                                                  */
/* Function	- Print a number n in base b. We don't use recursion*/
/*		  to avoid deep kernel stacks.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * Printn prints a number n in base b.
 */
static void
_printn(uint64_t n, int b, int width, int pad, void (*emit)(char, char **),
	char **bp)
{
	char prbuf[40];
	register char *cp;

	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
		width--;
	} while (n);
	while (width-- > 0)
		*cp++ = (char)pad;
	do {
		(*emit)(*--cp, bp);
	} while (cp > prbuf);
}

/*========================= End of Function ========================*/
