/*------------------------------------------------------------------*/
/* 								    */
/* Name        - _tls_get_addr.s				    */
/* 								    */
/* Function    - Return address of TLS.                             */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - August, 2007  					    */
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

	.file	"%M%"

#include <sys/asm_linkage.h>
#include <sys/trap.h>
#include <../assym.h>
#include "SYS.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- __tls_get_addr.                                   */
/*                                                                  */
/* Function	-                                                   */
/*                                                                  */
/* To make thread-local storage accesses as fast as possible, we    */
/* hand-craft the __tls_get_addr() function below, from this C code:*/
/* void *							    */
/* __tls_get_addr(TLS_index *tls_index)				    */
/* {								    */
/*	ulwp_t *self = curthread;				    */
/*	tls_t *tlsent = self->ul_tlsent;			    */
/*	ulong_t moduleid;					    */
/*	caddr_t	base;						    */
/*								    */
/*	if ((moduleid = tls_index->ti_moduleid) < 		    */
/*		self->ul_ntlsent &&				    */
/*	    (base = tlsent[moduleid].tls_data) != NULL)		    */
/*		return (base + tls_index->ti_tlsoffset);	    */
/*								    */
/*	return (slow_tls_get_addr(tls_index));			    */
/* }								    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * We assume that the tls_t structure contains two pointer-sized elements.
 */
	ENTRY(__tls_get_addr)
	ear	%r1,%a0
	l	%r3,UL_TLSENT(%r1)
	l	%r5,UL_NTLSENT(%r1)
	l	%r4,TI_MODULEID(%r2)
	cr	%r4,%r5
	jnl	0f

	sll	%r4,3
	l	%r5,TLS_DATA(%r4,%r3)
	ltr	%r5,%r5
	jz	0f

	a	%r5,TI_TLSOFFSET(%r2)
	lr	%r2,%r5
	br	%r14
0:
	st	%r14,56(%r15)
	aghi	%r15,-SA(MINFRAME32)
	brasl	%r14,slow_tls_get_addr@PLT
	aghi	%r15,SA(MINFRAME32)
	l	%r14,56(%r15)
	br	%r14
	SET_SIZE(__tls_get_addr)

/*========================= End of Function ========================*/
