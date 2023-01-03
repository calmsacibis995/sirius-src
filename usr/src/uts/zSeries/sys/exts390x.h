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

#ifndef __EXTS390X__

#define __EXTS390X__

#include <sys/regset.h>

#define EXT_OPRI	0x0040	// Operator interrupt
#define EXT_CLKC	0x1004	// Clock comparator
#define EXT_CPUT	0x1005	// CPU Timer
#define EXT_MALF	0x1200	// Malfunction alert
#define EXT_ESIG	0x1201	// Emergency signal
#define EXT_CALL	0x1202	// External call
#define EXT_TIMA	0x1406	// Timing alert
#define EXT_HWCN	0x2401	// Hardware console
#define EXT_SSIG	0x2603	// Page Fault Handshake / Block IO
#define EXT_IUCV	0x4000	// IUCV

/*
 * 0x2603 subcodes
 */
#define SSG_MAPM	0x01	// MAPMDISK
#define SSG_PFLE	0x02	// Page Fault - ESA
#define SSG_BIOE	0x03	// BIO - ESA
#define SSG_TCTL	0x04	// Tape control
#define SSG_PFLZ	0x06	// Page Fault - z/Architecture
#define SSG_BIOZ	0x07	// BIO - z/Architecture

#ifndef _ASM

typedef void (* hndext_proc_t)(mcontext_t, short, long, void *);

typedef struct hndext_t {
	short code;
	short fill[3];
	hndext_proc_t handler;
	void  *uword;
} __attribute__ ((packed)) hndext_t;

int hndext_set(short, hndext_proc_t, void *);
int hndext_clr(short, hndext_proc_t);

int hndssg_set(short, hndext_proc_t, void *);
int hndssg_clr(short, hndext_proc_t);

void timers_init(void);		// Initialize comparator and CPU timer
void timers_term(void);

void sclp_init(void);		// Initialize SCLP communications
void sclp_term(void);

#endif

#endif
