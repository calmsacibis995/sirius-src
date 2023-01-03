/*------------------------------------------------------------------*/
/*                                                                  */
/* Name         - BLOCKIO.                                          */
/*                                                                  */
/* Function     - Defines the data structures and prototypes for    */
/*                doing disk I/O using the DIAG X'250' interface.   */
/*                                                                  */
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

#ifndef __BLOCKIO_H__

#define __BLOCKIO_H__

#include <sys/types.h>

#define BIO_OP_IN	0
#define BIO_OP_RW	1
#define BIO_OP_RM	2

/*------------------------------------------------------------------*/
/* BELBK - Block I/O data request block.			    */
/*------------------------------------------------------------------*/
typedef struct __blkent__ {
	uint8_t	 belrqtyp;	// Request type
#define BELWRITE 0x01		// Write request
#define BELREAD	 0x02		// Read request

	uint8_t	 belstat;	// Status
#define BELOK	 0x00		// Normal completion
#define BELINVBK 0x01		// Invalid block number specified
#define BELADDRE 0x02		// Address exception
#define BELRODEV 0x03		// Write operation on R/O device
#define BELINVSZ 0x04		// Blocksize doesn't match device
#define BELIOERR 0x05		// Unrecoverable I/O error 
#define BELBADRQ 0x06		// Bad request type
#define BELPROTE 0x07		// Protection exception
#define BELADCPE 0x08		// Addressing-capability exception
#define BELALTRE 0x09		// ALEN-translation exception
#define BELALSPE 0x0a		// ALET-translation exception
#define BELSPECE 0x0b		// Specification exception
#define BELNOTPR 0x0c		// This BELBK entry not processed

	uint16_t belrsd01;	// Reserved
	uint32_t belbalet;	// ALET associated with block
	int64_t  belbknum;	// Block number
	uint64_t belbufad;	// Buffer address
} __attribute__ ((packed,aligned(8))) blkent_t;

/*------------------------------------------------------------------*/
/* BIOPL - Block I/O parameter list - header			    */
/*------------------------------------------------------------------*/
typedef struct __bioplhd__ {
	uint16_t biodevn;	// Device number
	uint8_t	 biomode; 	// Architecture mode
#define BIOESA	 0x00		// 32-bit mode
#define BIOZAR	 0x80		// 64-bit mode

	uint8_t	 biorsd00;	// Reserved
	uchar_t	 biorsd01[20];	// Reserved
} __attribute__ ((packed,aligned(8))) bioplhd_t;

/*------------------------------------------------------------------*/
/* BIOPL - Block I/O parameter list - initialization request	    */
/*------------------------------------------------------------------*/
typedef struct __bioplin__ {
	bioplhd_t bioplhd;	// Header
	uint32_t  bioblksz;	// Block size
	uint32_t  biorsd10;	// Reserved
	int64_t   biooffst;	// Offset of first block
	int64_t   biostart;	// Starting block number
	int64_t   bioend;	// Ending block number
	uint64_t  biorsd11;	// Reserved
} __attribute__ ((packed,aligned(8))) bioplin_t;

/*------------------------------------------------------------------*/
/* BIOPL - Block I/O parameter list - remove request     	    */
/*------------------------------------------------------------------*/
typedef struct __bioplrm__ {
	bioplhd_t bioplhd;	// Header
	uchar_t	  biorsd02[40];	// Reserved
} __attribute__ ((packed,aligned(8))) bioplrm_t;

/*------------------------------------------------------------------*/
/* BIOPL - Block I/O parameter list - read/write request    	    */
/*------------------------------------------------------------------*/
typedef struct __bioplrw__ {
	bioplhd_t bioplhd;	// Header
	uint8_t	  biokey;	// Subchannel key
#define BIOACCBT  0xf0		// Mask to get access ctl key

	uint8_t	  bioflag;	// I/O flag
#define BIOASYN   0x02		// Asynchronous
#define BIOBYPAS  0x01		// Bypass minidisk cache

	uint16_t  biorsd20;	// Reserved
	uint32_t  biolentn;	// Number of BELBK entries
#define BIOMAXCT  256  		// Max. BELBK count

	uint32_t  biolalet;	// ALET of space containing data
	uint32_t  biorsd21;	// Reserved
	uintptr_t bioiparm;	// Interruption parameter
	uintptr_t bioladdr;	// Start of BELBK(s) 
	uint64_t  biorsd22;	// Reserved
} __attribute__ ((packed,aligned(8))) bioplrw_t; 

#endif
