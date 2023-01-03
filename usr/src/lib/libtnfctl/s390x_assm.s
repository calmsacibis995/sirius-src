/*==================================================================*/
/* 								    */
/* CDDL HEADER START						    */
/* 								    */
/* The contents of this file are subject to the terms of the	    */
/* Common Development and Distribution License, Version 1.0 only    */
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

	.file		"s390x_assm.s"
	.section	".data"
	.align		4
	.global		prb_callinfo
prb_callinfo:
	.word		0		// offset
	.word		2		// shift right
	.word		0x3fffffff	// mask

	.section	".text"
	.align		4
	.global		prb_chain_entry
	.global		prb_chain_down
	.global		prb_chain_next
	.global		prb_chain_end
	.local		chain_down
	.local		chain_next
prb_chain_entry:
	stmg		%r6,%r14,48(%r15)
	aghi		%r15,-160
prb_chain_down:
chain_down:
	brasl		%r14,chain_down
prb_chain_next:
chain_next:
	brasl		%r14,chain_next
	aghi		%r15,160
	lmg		%r6,%r14,48(%r15)
	br		%r14
prb_chain_end:
