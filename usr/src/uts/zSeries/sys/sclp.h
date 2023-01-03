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

#ifndef __SCLP_H__

#define __SCLP_H__

#include <sys/types.h>

typedef unsigned int sclp_req_t;

//
// Common header for Service processor Command Control Block
//
typedef struct _sccbhdr_ {
	uint16_t len;		// Length of request
	uint8_t	 code;		// Function code
	uint8_t	 mask[3];	// Control mask
#define SCVLRSP 0x80         	// Variable length response

	uint16_t rsp;		// Response code
#define SCREADN 0x0010          // Normal read completion
#define SCRESVD 0x0310          // ... Resource in Reserved mode
#define SCSTDBY 0x0410          // ... Resource in Standby mode
#define SCBKOUT 0x0040          // Normal backout; operation is retriable
#define SCIVRID 0x09F0          // Invalid Resource ID in SCCB
#define SCNORML 0x0020          // Normal completion
#define SCMALF  0x0040          // Equipment check
#define SCMCNFG 0x0010          // Resource is configured
#define SCNO4KB 0x0100          // Address crosses 4K boundard
#define SCNOACT 0x0120          // No action required
#define SCINVCI 0x01F0          // Invalid SCLP command
#define SCPCFLG 0x02F0          // Invalid SCCB
#define SCPCMPL 0x0240          // Partially complete
#define SCNRMRP 0x0220          // Normal completion
#define SCBADLN 0x0330          // Length wrong for data
#define SCRSRVD 0x0310          // Resource in reserved state
#define SCINVCP 0x03F0          // Invalid resource ID in command
#define SCSTNBY 0x0410          // Resource in standby state
#define SCXNOCN 0x05F0          // Resource in improper state
#define SCXINVN 0x09F0          // Invalid resource ID in SCCB
#define SCRQRES 0x0AF0          // Required resource
#define SCPOWOF 0x10F0          // Power off status
#define SCREJRC 0x40F0          // Invalid function code
#define SCNOEVT 0x60F0          // No outstanding event buffers
#define SCSUPPR 0x62F0          // All events are suppressed
#define SCINVSL 0x70F0          // Invalid selection
#define SCEBSPC 0x71F0          // Event buffer exceeds specs
#define SCEBLEN 0x72F0          // Event buffer length incorrect
#define SCEBSYN 0x73F0          // Event buffer syntax error
#define SCINVMK 0x74F0          // Invalid mask selection
#define SCEXCMX 0x75F0          // Exceeds SCCB max capacity
#define SCDBER  0x00            // SCCB error
#define SCINFO  0x10            // Normal read completion
#define SCCMPL  0x20            // Normal completion
#define SCBUSY  0x30            // Function busy
#define SCEQCK  0x40            // Equipment check
#define SCRJCT  0xF0            // Reject
#define SCEVTX  0xF3            // Event buffer exceeds space
#define SCLVER  0xF4            // Length verification error
} __attribute__ ((packed)) sccbhdr;

//
// Message to Operator
//
typedef struct __scmsgto__ {
	uint16_t mtlen;		// Block length
	uint16_t mttype;	// Type
#define GO_MTO	0x04		// Type: Message to operator

	uint16_t mtlflg;	// Line type flags
#define LT_PMT	0x0800		// Flags: Prompt
#define LT_ETX	0x1000		// 	  End text
#define LT_DAT  0x2000		// 	  Data
#define LT_LBL  0x4000		// 	  Label  
#define LT_CTL  0x8000		// 	  Control

	uint8_t	 mtalrm;	// Alarm control
	uint8_t  mtxxx[3];	// Reserved
} __attribute__ ((packed)) scmsgto;

//
// GDS
//
typedef struct __scmsggo__ {
	uint16_t golen;		// Block length
	uint16_t gotype;	// Type
	uint32_t godomid;	// Domain id
	uint8_t	 gohhmmss[8];	// HH:MM:SS
	uint8_t	 goths[3];	// Thousandths of seconds
	uint8_t	 goxxx1;	
	uint8_t	 gojulian[7];	// Julian date - bloody MVS
	uint8_t	 goxxx2;	
	uint16_t gomflags;	// Message flags
	uint8_t	 goxxx3[10];	
	uint8_t	 goorig[8];	// Originating system
	uint8_t	 goguest[8];	// Guest name
} __attribute__ ((packed)) scmsggo;

//
// Message Data Block Header
//
typedef struct __scmdbhd__ {
	uint16_t mdlen;		// Length
	uint16_t mdtype;	// Type
	uint32_t mdtag;		// Tag
#define MDBT_MDB 0xD4C4C240	// Tag: 'MDB' in EBCDIC

	uint32_t mdrev;		// Revision
} __attribute__ ((packed)) scmdbhd;

//
// Event Buffer
//
typedef struct __scevbuf__ {
	uint16_t evlen;		// Length
	uint8_t	 evtype;	// Type
#define EVT_CMD	0x01		// Type: Operator Command
#define EVT_MSG	0x02		// 	 Message
#define EVT_STC	0x08		// 	 State Change
#define EVT_PCM	0x09		// 	 Priority Message Command(?)
#define EVT_CPI	0x0b		// 	 Control Program Identity
#define EVT_VTM	0x1a		// 	 VT220 Message
#define EVT_SGQ	0x1d		// 	 Quiesce Signal
#define EVT_CPO	0x20		// 	 Control Program Operator Command

#define EVM_CMD	0x80000000	// Mask: Operator Command
#define EVM_MSG	0x40000000	// 	 Message
#define EVM_STC	0x01000000	//	 State Change
#define EVM_PCM	0x00800000	// 	 Priority Message Command(?)
#define EVM_CPI	0x00200000	// 	 Control Program Identity
#define EVM_VTM	0x00000040	// 	 VT220 Message
#define EVM_SGQ	0x00000008	// 	 Quiesce Signal
#define EVM_CPO	0x00000001	// 	 Control Program Operator Command

	uint8_t	 evflags;	// Flags
	uint16_t evxxx;
} __attribute__ ((packed)) scevbuf;

//
// Message Data Block
//
typedef struct __scmdb__ {
	scmdbhd hdr;		// Header
	scmsggo go;		// GO element
} __attribute__ ((packed)) scmdb;

//
// Message Buffer
//
typedef struct __scmsgbf__ {
	scevbuf	ev;		// Event header
	scmdb	md;		// MDB
} __attribute__ ((packed)) scmsgbf;

//
// Event Buffer Major Vectors
//
typedef struct __scvecmj__ {
	uint16_t len;		// Length
	uint16_t vec;		// Vector

#define VEC_CPMS 0x1212		// Control Program Message
#define VEC_MDSU 0x1310		// MDS message unit
#define VEC_MDSR 0x1311		// MDS routine information
#define VEC_MSGT 0x1320		// Message text
#define VEC_RTI  0x1549		//
#define VEC_OPRQ 0x8070		// Operator request
} __attribute__ ((packed)) scvecmj;

//
// Event Buffer Minor Vectors
//
typedef struct __scvecmn__ {
	uint8_t	len;		// Length
	uint8_t	vec;		// Vector

#define VCM_NLST 0x06		// Name List
#define VCM_TXTD 0x30		// Text Data
#define VCM_SDEF 0x31		// Self-defining
#define VCM_ORLN 0x81		// Origin Location Name
#define VCM_DSLN 0x82		// Destination Location Name
#define VCM_FLGS 0x90		// Flags
} __attribute__ ((packed)) scvecmn;

//
// Write SCCB
//
typedef struct __wrsccb__ {
	sccbhdr hdr;		// SCCB header
	scmsgbf msgbf;		// Message buffer
} __attribute__ ((packed)) wrsccb;

//
// Read SCCB
//
typedef struct __rdsccb__ {
	sccbhdr hdr;		// SCCB header
	scevbuf ev;		// Event buffer
} __attribute__ ((packed)) rdsccb;

//
// Write Event Mask
//
typedef struct __emsccb__ {
	sccbhdr hdr;		// SCCB header
	uint16_t xxx;		// Reserved
	uint16_t cnt;		// Count of event masks
	uint32_t mask[4];	// Event masks
} __attribute__ ((packed)) emsccb;

//
// SCCB Function Codes
//

	// Read SCP info
	// This call is used to determine the machine characteristics, 
	// installed features, storage sizes and load parameter.
#define SCP_RSCP	0x00020001 	

	// Read SCP info forced
	// This call is used to determine the machine characteristics, 
	// installed features, storage sizes and load parameter.
#define SCP_RSCF	0x00120001 	

	// Read Channel Path info
	// This call indicates the status of the channel paths
#define SCP_RCPI	0x00030001 

	// Read Channel Subsystem info
	// This call is used to determine the channel subsystem 
	// features such as the concurrent sense facility
#define SCP_RCSI	0x001C0001 

	// Read Expanded Storage Map
	// This call is used to determine expanded storage block status
	// a bit map is returned where bits associated with invalid 
	// blocks are set to one
#define SCP_RXSM	0x00250001 

	// Deconfigure CPU nn
	// Physically configure CPU offline
#define SCP_DCPU	0x0100001

	// Configure CPU nn
	// Physically configure CPU online 
#define SCP_CCPU	0x00110001

	// Write Event Data
	// Sent event data such as Write To Operator messages 
	// to the service processor
#define SCP_WEVD	0x00760005

	// Read Event Data
	// Read pending event data such as operator commands 
	// from the service processor
#define SCP_REVD	0x00770005

	// Write Event Mask
	// Indicate to the service processor what events are supported
#define SCP_WEVM	0x00780005

#endif
