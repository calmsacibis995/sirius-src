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

#ifndef __DEVINIT_H__

#define __DEVINIT_H__

#include <sys/ios390x.h>

typedef struct _ioDev {
	void 	*next;		/* Next device 			*/
	void 	*prev;		/* Previous device		*/
	void	*private;	/* Private data			*/
	uint32_t schid;		/* Subchannel id		*/
	uint32_t instance;	/* Reserved     		*/
	struct  schib sch;	/* Subchannel information block */
	struct	vrdcblok dev;	/* Device characteristics	*/
} ioDev;

typedef struct _devList {
	int	devCount;
	ioDev	*devices;
} devList;

#endif
