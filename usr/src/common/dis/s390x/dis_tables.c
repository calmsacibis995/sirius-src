/*------------------------------------------------------------------*/
/* 								    */
/* Name        -            					    */
/* 								    */
/* Function    -                                                    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
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

#define IFMT_E		0
#define IFMT_I		1
#define IFMT_RI1	2
#define IFMT_RI2	3
#define IFMT_RIE	4
#define IFMT_RIL1	5
#define IFMT_RIL2	6
#define IFMT_RR		7
#define IFMT_RRE	8
#define IFMT_RRF1	9
#define IFMT_RRF2	10
#define IFMT_RRF3	11
#define IFMT_RRR	12
#define IFMT_RS1	13
#define IFMT_RS2	14
#define IFMT_RSI	15
#define IFMT_RSL	16
#define IFMT_RSY1	17
#define IFMT_RSY2	18
#define IFMT_RX		19
#define IFMT_RXE	20
#define IFMT_RXF	21
#define IFMT_RXY	22
#define IFMT_S		23
#define IFMT_SI		24
#define IFMT_SIY	25
#define IFMT_SS1	26
#define IFMT_SS2	27
#define IFMT_SS3	28
#define IFMT_SS4	29
#define IFMT_SS5	30
#define IFMT_SSE	31
#define IFMT_SSF	32

#define TBL_INV		-1
#define TBL_01		-2
#define TBL_A5		-3
#define TBL_A7		-4
#define TBL_B2		-5
#define TBL_B3		-6
#define TBL_B9		-7
#define TBL_C0		-8
#define TBL_C2		-9
#define TBL_C8		-10
#define TBL_E3		-11
#define TBL_E5		-12
#define TBL_EB		-13
#define TBL_EC		-14
#define TBL_ED		-15

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/asm_linkage.h>
#include "dis_tables.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

typedef struct _opCode_t {
	uint8_t  opcode;	/* First byte of operation code	    */
	char	 *desc;		/* Instruction description	    */
	char	 *mnemonic;	/* Instruction mnemonic		    */
	int8_t	 type;		/* Entry type: IFMT_xxx, TBL_xxx    */
} opCode_t;

typedef struct _E_t {
	uint16_t opcode;
} __attribute__ ((packed)) E_t;

typedef struct _I_t {
	uint8_t	opcode;
	uint8_t i1;
} __attribute__ ((packed)) I_t;

typedef struct _RI1_t {
	uint8_t	op1;
	uint8_t r1  : 4;
	uint8_t op2 : 4;
	int16_t i2;
} __attribute__ ((packed)) RI1_t ;

typedef struct _RI2_t {
	uint8_t	op1;
	uint8_t m1  : 4;
	uint8_t op2 : 4;
	int16_t i2;
} __attribute__ ((packed)) RI2_t;

typedef struct _RIE_t {
	uint8_t	op1;
	uint8_t r1 : 4;
	uint8_t r3 : 4;
	int16_t i2;
	uint8_t xxx;
	uint8_t op2;
} __attribute__ ((packed)) RIE_t;

typedef struct _RIL1_t {
	uint8_t	op1;
	uint8_t r1  : 4;
	uint8_t op2 : 4;
	int32_t i2;
} __attribute__ ((packed)) RIL1_t;

typedef struct _RIL2_t {
	uint8_t	op1;
	uint8_t m1  : 4;
	uint8_t op2 : 4;
	int32_t i2;
} __attribute__ ((packed)) RIL2_t;

typedef struct _RR_t {
	uint8_t	opcode;
	uint8_t r1 : 4;
	uint8_t r2 : 4;
} __attribute__ ((packed)) RR_t;

typedef struct _RRE_t {
	uint16_t opcode;
	uint8_t  xxx;
	uint8_t  r1 : 4;
	uint8_t  r2 : 4;
} __attribute__ ((packed)) RRE_t;

typedef struct _RRF1_t {
	uint16_t opcode;
	uint8_t  r1 : 4;
	uint8_t  xx : 4;
	uint8_t  r3 : 4;
	uint8_t  r2 : 4;
} __attribute__ ((packed)) RRF1_t;

typedef struct _RRF2_t {
	uint16_t opcode;
	uint8_t  m3 : 4;
	uint8_t  xx : 4;
	uint8_t  r1 : 4;
	uint8_t  r2 : 4;
} __attribute__ ((packed)) RRF2_t;

typedef struct _RRF3_t {
	uint16_t opcode;
	uint8_t  r3 : 4;
	uint8_t  m4 : 4;
	uint8_t  r1 : 4;
	uint8_t  r2 : 4;
} __attribute__ ((packed)) RRF3_t;

typedef struct _RRR_t {
	uint16_t opcode;
	uint8_t  r3 : 4;
	uint8_t  xx : 4;
	uint8_t  r1 : 4;
	uint8_t  r2 : 4;
} __attribute__ ((packed)) RRR_t;

typedef struct _RS1_t {
	uint8_t  opcode;
	uint8_t  r1 : 4;
	uint8_t  r3 : 4;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) RS1_t;

typedef struct _RS2_t {
	uint8_t  opcode;
	uint8_t  r1 : 4;
	uint8_t  m3 : 4;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) RS2_t;

typedef struct _RSI_t {
	uint8_t  opcode;
	uint8_t  r1 : 4;
	uint8_t  r3 : 4;
	int16_t  i2;
} __attribute__ ((packed)) RSI_t;

typedef struct _RSL_t {
	uint8_t  op1;
	uint8_t  l1 : 4;
	uint8_t  xx : 4;
	uint16_t b1 : 4;
	uint16_t d1 : 12;
	uint8_t  zz;
	uint8_t  op2;
} __attribute__ ((packed)) RSL_t;

typedef struct _RSY1_t {
	uint8_t  op1;
	uint8_t  r1 : 4;
	uint8_t  r3 : 4;
	uint16_t b2 : 4;
	uint16_t dl : 12;
	uint8_t  dh;
	uint8_t  op2;
} __attribute__ ((packed)) RSY1_t;

typedef struct _RSY2_t {
	uint8_t  op1;
	uint8_t  r1 : 4;
	uint8_t  m3 : 4;
	uint16_t b2 : 4;
	uint16_t dl : 12;
	uint8_t  dh;
	uint8_t  op2;
} __attribute__ ((packed)) RSY2_t;

typedef struct _RX_t {
	uint8_t  opcode;
	uint8_t  r1 : 4;
	uint8_t  x2 : 4;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) RX_t;

typedef struct _RXE_t {
	uint8_t  op1;
	uint8_t  r1 : 4;
	uint8_t  x2 : 4;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
	uint8_t  xx;
	uint8_t  op2;
} __attribute__ ((packed)) RXE_t;

typedef struct _RXF_t {
	uint8_t  op1;
	uint8_t  r3 : 4;
	uint8_t  x2 : 4;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
	uint8_t  r1 : 4;
	uint8_t  xx : 4;
	uint8_t  op2;
} __attribute__ ((packed)) RXF_t;

typedef struct _RXY_t {
	uint8_t  op1;
	uint8_t  r1 : 4;
	uint8_t  x2 : 4;
	uint16_t b2 : 4;
	uint16_t dl : 12;
	uint8_t  dh;
	uint8_t  op2;
} __attribute__ ((packed)) RXY_t;

typedef struct _S_t {
	uint16_t opcode;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) S_t;

typedef struct _SI_t {
	uint8_t  opcode;
	uint8_t  i2;
	uint16_t b1 : 4;
	uint16_t d1 : 12;
} __attribute__ ((packed)) SI_t;

typedef struct _SIY_t {
	uint8_t  opcode;
	uint8_t  i2;
	uint16_t b1 : 4;
	uint16_t dl : 12;
	uint8_t  dh;
	uint8_t  op2;
} __attribute__ ((packed)) SIY_t;

typedef struct _SS1_t {
	uint8_t  opcode;
	uint8_t  l;
	uint16_t b1 : 4;
	uint16_t d1 : 12;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) SS1_t;

typedef struct _SS2_t {
	uint8_t  opcode;
	uint8_t  l1 : 4;
	uint8_t  l2 : 4;
	uint16_t b1 : 4;
	uint16_t d1 : 12;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) SS2_t;

typedef struct _SS3_t {
	uint8_t  opcode;
	uint8_t  l1 : 4;
	uint8_t  i3 : 4;
	uint16_t b1 : 4;
	uint16_t d1 : 12;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) SS3_t;

typedef struct _SS4_t {
	uint8_t  opcode;
	uint8_t  r1 : 4;
	uint8_t  r3 : 4;
	uint16_t b1 : 4;
	uint16_t d1 : 12;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) SS4_t;

typedef struct _SS5_t {
	uint8_t  opcode;
	uint8_t  r1 : 4;
	uint8_t  r3 : 4;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
	uint16_t b4 : 4;
	uint16_t d4 : 12;
} __attribute__ ((packed)) SS5_t;

typedef struct _SSE_t {
	uint16_t opcode;
	uint16_t b1 : 4;
	uint16_t d1 : 12;
	uint16_t b2 : 4;
	uint16_t d2 : 12;
} __attribute__ ((packed)) SSE_t;

typedef struct _SSF_t {
	uint8_t  op1;
	uint8_t  r3  : 4;
	uint8_t  op2 : 4;
	uint16_t b1  : 4;
	uint16_t d1  : 12;
	uint16_t b2  : 4;
	uint16_t d2  : 12;
} __attribute__ ((packed)) SSF_t;

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

static opCode_t opCodes[256] = {
	{0x00, "", "", TBL_INV},
	{0x01, "", "", TBL_01},
	{0x02, "", "", TBL_INV},
	{0x03, "", "", TBL_INV},
	{0x04, "SET PROGRAM MASK", "SPM", IFMT_RR},
	{0x05, "BRANCH AND LINK", "BALR", IFMT_RR},
	{0x06, "BRANCH ON COUNT (32)", "BCTR", IFMT_RR},
	{0x07, "BRANCH ON CONDITION", "BCR", IFMT_RR},
	{0x08, "", "", TBL_INV},
	{0x09, "", "", TBL_INV},
	{0x0A, "SUPERVISOR CALL", "SVC", IFMT_I},
	{0x0B, "BRANCH AND SET MODE", "BSM", IFMT_RR},
	{0x0C, "BRANCH AND SAVE AND SET MODE", "BASSM", IFMT_RR},
	{0x0D, "BRANCH AND SAVE", "BASR", IFMT_RR},
	{0x0E, "MOVE LONG", "MVCL", IFMT_RR},
	{0x0F, "COMPARE LOGICAL LONG", "CLCL", IFMT_RR},
	{0x10, "LOAD POSITIVE (32)", "LPR", IFMT_RR},
	{0x11, "LOAD NEGATIVE (32)", "LNR", IFMT_RR},
	{0x12, "LOAD AND TEST (32)", "LTR", IFMT_RR},
	{0x13, "LOAD COMPLEMENT (32)", "LCR", IFMT_RR},
	{0x14, "AND (32)", "NR", IFMT_RR},
	{0x15, "COMPARE LOGICAL (32)", "CLR", IFMT_RR},
	{0x16, "OR (32)", "OR", IFMT_RR},
	{0x17, "EXCLUSIVE OR (32)", "XR", IFMT_RR},
	{0x18, "LOAD (32)", "LR", IFMT_RR},
	{0x19, "COMPARE (32)", "CR", IFMT_RR},
	{0x1A, "ADD (32)", "AR", IFMT_RR},
	{0x1B, "SUBTRACT (32)", "SR", IFMT_RR},
	{0x1C, "MULTIPLY (64 32)", "MR", IFMT_RR},
	{0x1D, "DIVIDE (32 64)", "DR", IFMT_RR},
	{0x1E, "ADD LOGICAL (32)", "ALR", IFMT_RR},
	{0x1F, "SUBTRACT LOGICAL (32)", "SLR", IFMT_RR},
	{0x20, "LOAD POSITIVE (long HFP)", "LPDR", IFMT_RR},
	{0x21, "LOAD NEGATIVE (long HFP)", "LNDR", IFMT_RR},
	{0x22, "LOAD AND TEST (long HFP)", "LTDR", IFMT_RR},
	{0x23, "LOAD COMPLEMENT (long HFP)", "LCDR", IFMT_RR},
	{0x24, "HALVE (long HFP)", "HDR", IFMT_RR},
	{0x25, "LOAD ROUNDED (extended to long HFP)", "LDXR", IFMT_RR},
	{0x26, "MULTIPLY (extended HFP)", "MXR", IFMT_RR},
	{0x27, "MULTIPLY (long to extended HFP)", "MXDR", IFMT_RR},
	{0x28, "LOAD (long)", "LDR", IFMT_RR},
	{0x29, "COMPARE (long HFP)", "CDR", IFMT_RR},
	{0x2A, "ADD NORMALIZED (long HFP)", "ADR", IFMT_RR},
	{0x2B, "SUBTRACT NORMALIZED (long HFP)", "SDR", IFMT_RR},
	{0x2C, "MULTIPLY (long HFP)", "MDR", IFMT_RR},
	{0x2D, "DIVIDE (long HFP)", "DDR", IFMT_RR},
	{0x2E, "ADD UNNORMALIZED (long HFP)", "AWR", IFMT_RR},
	{0x2F, "SUBTRACT UNNORMALIZED (long HFP)", "SWR", IFMT_RR},
	{0x30, "LOAD POSITIVE (short HFP)", "LPER", IFMT_RR},
	{0x31, "LOAD NEGATIVE (short HFP)", "LNER", IFMT_RR},
	{0x32, "LOAD AND TEST (short HFP)", "LTER", IFMT_RR},
	{0x33, "LOAD COMPLEMENT (short HFP)", "LCER", IFMT_RR},
	{0x34, "HALVE (short HFP)", "HER", IFMT_RR},
	{0x35, "LOAD ROUNDED (long to short HFP)", "LEDR", IFMT_RR},
	{0x36, "ADD NORMALIZED (extended HFP)", "AXR", IFMT_RR},
	{0x37, "SUBTRACT NORMALIZED (extended HFP)", "SXR", IFMT_RR},
	{0x38, "LOAD (short)", "LER", IFMT_RR},
	{0x39, "COMPARE (short HFP)", "CER", IFMT_RR},
	{0x3A, "ADD NORMALIZED (short HFP)", "AER", IFMT_RR},
	{0x3B, "SUBTRACT NORMALIZED (short HFP)", "SER", IFMT_RR},
	{0x3C, "MULTIPLY (short to long HFP)", "MER", IFMT_RR},
	{0x3D, "DIVIDE (short HFP)", "DER", IFMT_RR},
	{0x3E, "ADD UNNORMALIZED (short HFP)", "AUR", IFMT_RR},
	{0x3F, "SUBTRACT UNNORMALIZED (short HFP)", "SUR", IFMT_RR},
	{0x40, "STORE HALFWORD", "STH", IFMT_RX},
	{0x41, "LOAD ADDRESS", "LA", IFMT_RX},
	{0x42, "STORE CHARACTER", "STC", IFMT_RX},
	{0x43, "INSERT CHARACTER", "IC", IFMT_RX},
	{0x44, "EXECUTE", "EX", IFMT_RX},
	{0x45, "BRANCH AND LINK", "BAL", IFMT_RX},
	{0x46, "BRANCH ON COUNT (32)", "BCT", IFMT_RX},
	{0x47, "BRANCH ON CONDITION", "BC", IFMT_RX},
	{0x48, "LOAD HALFWORD (32)", "LH", IFMT_RX},
	{0x49, "COMPARE HALFWORD", "CH", IFMT_RX },
	{0x4A, "ADD HALFWORD", "AH", IFMT_RX},
	{0x4B, "SUBTRACT HALFWORD", "SH", IFMT_RX},
	{0x4C, "MULTIPLY HALFWORD (32)", "MH", IFMT_RX},
	{0x4D, "BRANCH AND SAVE", "BAS", IFMT_RX},
	{0x4E, "CONVERT TO DECIMAL (32)", "CVD", IFMT_RX},
	{0x4F, "CONVERT TO BINARY (32)", "CVB", IFMT_RX},
	{0x50, "STORE (32)", "ST", IFMT_RX },
	{0x51, "LOAD ADDRESS EXTENDED", "LAE", IFMT_RX},
	{0x52, "", "", TBL_INV},
	{0x53, "", "", TBL_INV},
	{0x54, "AND (32)", "N", IFMT_RX},
	{0x55, "COMPARE LOGICAL (32)", "CL", IFMT_RX},
	{0x56, "OR (32)", "O", IFMT_RX},
	{0x57, "EXCLUSIVE OR (32)", "X", IFMT_RX},
	{0x58, "LOAD (32)", "L", IFMT_RX},
	{0x59, "COMPARE (32)", "C", IFMT_RX},
	{0x5A, "ADD (32)", "A", IFMT_RX},
	{0x5B, "SUBTRACT (32)", "S", IFMT_RX},
	{0x5C, "MULTIPLY (64 32)", "M", IFMT_RX },
	{0x5D, "DIVIDE (32 64)", "D", IFMT_RX },
	{0x5E, "ADD LOGICAL (32)", "AL", IFMT_RX},
	{0x5F, "SUBTRACT LOGICAL (32)", "SL", IFMT_RX},
	{0x60, "STORE (long)", "STD", IFMT_RX},
	{0x61, "", "", TBL_INV},
	{0x62, "", "", TBL_INV},
	{0x63, "", "", TBL_INV},
	{0x64, "", "", TBL_INV},
	{0x65, "", "", TBL_INV},
	{0x66, "", "", TBL_INV},
	{0x67, "MULTIPLY (long to extended HFP)", "MXD", IFMT_RX},
	{0x68, "LOAD (long)", "LD", IFMT_RX},
	{0x69, "COMPARE (long HFP)", "CD", IFMT_RX},
	{0x6A, "ADD NORMALIZED (long HFP)", "AD", IFMT_RX},
	{0x6B, "SUBTRACT NORMALIZED (long HFP)", "SD", IFMT_RX},
	{0x6C, "MULTIPLY (long HFP)", "MD", IFMT_RX },
	{0x6D, "DIVIDE (long HFP)", "DD", IFMT_RX},
	{0x6E, "ADD UNNORMALIZED (long HFP)", "AW", IFMT_RX},
	{0x6F, "SUBTRACT UNNORMALIZED (long HFP)", "SW", IFMT_RX},
	{0x70, "STORE (short)", "STE", IFMT_RX},
	{0x71, "MULTIPLY SINGLE (32)", "MS", IFMT_RX},
	{0x72, "", "", TBL_INV},
	{0x73, "", "", TBL_INV},
	{0x74, "", "", TBL_INV},
	{0x75, "", "", TBL_INV},
	{0x76, "", "", TBL_INV},
	{0x77, "", "", TBL_INV},
	{0x78, "LOAD (short)", "LE", IFMT_RX},
	{0x79, "COMPARE (short HFP)", "CE", IFMT_RX},
	{0x7A, "ADD NORMALIZED (short HFP)", "AE", IFMT_RX},
	{0x7B, "SUBTRACT NORMALIZED (short HFP)", "SE", IFMT_RX},
	{0x7C, "MULTIPLY (short to long HFP)", "MDE", IFMT_RX},
	{0x7D, "DIVIDE (short HFP)", "DE", IFMT_RX},
	{0x7E, "ADD UNNORMALIZED (short HFP)", "AU", IFMT_RX},
	{0x7F, "SUBTRACT UNNORMALIZED (short HFP)", "SU", IFMT_RX},
	{0x80, "SET SYSTEM MASK", "SSM", IFMT_S},
	{0x82, "LOAD PSW", "LPSW", IFMT_S},
	{0x83, "DIAGNOSE", "DIAG", IFMT_RSI},
	{0x84, "BRANCH RELATIVE ON INDEX HIGH (32)", "BRXH", IFMT_RSI},
	{0x85, "BRANCH RELATIVE ON INDEX LOW OR EQ. (32)", "BRXLE", IFMT_RSI},
	{0x86, "BRANCH ON INDEX HIGH (32)", "BXH", IFMT_RS1},
	{0x87, "BRANCH ON INDEX LOW OR EQUAL (32)", "BXLE", IFMT_RS1},
	{0x88, "SHIFT RIGHT SINGLE LOGICAL (32)", "SRL", IFMT_RS1},
	{0x89, "SHIFT LEFT SINGLE LOGICAL (32)", "SLL", IFMT_RS1},
	{0x8A, "SHIFT RIGHT SINGLE (32)", "SRA", IFMT_RS1},
	{0x8B, "SHIFT LEFT SINGLE (32)", "SLA", IFMT_RS1},
	{0x8C, "SHIFT RIGHT DOUBLE LOGICAL", "SRDL", IFMT_RS1},
	{0x8D, "SHIFT LEFT DOUBLE LOGICAL", "SLDL", IFMT_RS1},
	{0x8E, "SHIFT RIGHT DOUBLE", "SRDA", IFMT_RS1},
	{0x8F, "SHIFT LEFT DOUBLE", "SLDA", IFMT_RS1},
	{0x90, "STORE MULTIPLE (32)", "STM", IFMT_RS1},
	{0x91, "TEST UNDER MASK", "TM", IFMT_SI},
	{0x92, "MOVE (immediate)", "MVI", IFMT_SI},
	{0x93, "TEST AND SET", "TS", IFMT_S},
	{0x94, "AND (immediate)", "NI", IFMT_SI},
	{0x95, "COMPARE LOGICAL (immediate)", "CLI", IFMT_SI},
	{0x96, "OR (immediate)", "OI", IFMT_SI},
	{0x97, "EXCLUSIVE OR (immediate)", "XI", IFMT_SI},
	{0x98, "LOAD MULTIPLE (32)", "LM", IFMT_RS1},
	{0x99, "TRACE (32)", "TRACE", IFMT_RS1},
	{0x9A, "LOAD ACCESS MULTIPLE", "LAM", IFMT_RS1},
	{0x9B, "STORE ACCESS MULTIPLE", "STAM", IFMT_RS1},
	{0x9C, "", "", TBL_INV},
	{0x9D, "", "", TBL_INV},
	{0x9E, "", "", TBL_INV},
	{0x9F, "", "", TBL_INV},
	{0xA0, "", "", TBL_INV},
	{0xA1, "", "", TBL_INV},
	{0xA2, "", "", TBL_INV},
	{0xA3, "", "", TBL_INV},
	{0xA4, "", "", TBL_INV},
	{0xA5, "", "", TBL_A5},
	{0xA6, "", "", TBL_INV},
	{0xA7, "", "", TBL_A7},
	{0xA8, "MOVE LONG EXTENDED", "MVCLE", IFMT_RS1},
	{0xA9, "COMPARE LOGICAL LONG EXTENDED", "CLCLE", IFMT_RS1},
	{0xAA, "", "", TBL_INV},
	{0xAB, "", "", TBL_INV},
	{0xAC, "STORE THEN AND SYSTEM MASK", "STNSM", IFMT_SI},
	{0xAD, "STORE THEN OR SYSTEM MASK", "STOSM", IFMT_SI},
	{0xAE, "SIGNAL PROCESSOR", "SIGP", IFMT_RS1},
	{0xAF, "MONITOR CALL", "MC", IFMT_SI},
	{0xB0, "", "", TBL_INV},
	{0xB1, "LOAD REAL ADDRESS (32)", "LRA", IFMT_RX},
	{0xB2, "", "", TBL_B2},
	{0xB3, "", "", TBL_B3},
	{0xB4, "", "", TBL_INV},
	{0xB5, "", "", TBL_INV},
	{0xB6, "STORE CONTROL (32)", "STCTL", IFMT_RS1},
	{0xB7, "LOAD CONTROL (32)", "LCTL", IFMT_RS1},
	{0xB8, "", "", TBL_INV},
	{0xB9, "", "", TBL_B9},
	{0xBA, "COMPARE AND SWAP (32)", "CS", IFMT_RS1},
	{0xBB, "COMPARE DOUBLE AND SWAP (32)", "CDS", IFMT_RS1},
	{0xBC, "", "", TBL_INV},
	{0xBD, "COMPARE LOGICAL CHAR. UNDER MASK (low)", "CLM", IFMT_RS2},
	{0xBE, "STORE CHARACTERS UNDER MASK (low)", "STCM", IFMT_RS2},
	{0xBF, "INSERT CHARACTERS UNDER MASK (low)", "ICM", IFMT_RS2},
	{0xC0, "", "", TBL_C0},
	{0xC1, "", "", TBL_INV},
	{0xC2, "", "", TBL_C2},
	{0xC3, "", "", TBL_INV},
	{0xC4, "", "", TBL_INV},
	{0xC5, "", "", TBL_INV},
	{0xC6, "", "", TBL_INV},
	{0xC7, "", "", TBL_INV},
	{0xC8, "", "", TBL_C8},
	{0xC9, "", "", TBL_INV},
	{0xCA, "", "", TBL_INV},
	{0xCB, "", "", TBL_INV},
	{0xCC, "", "", TBL_INV},
	{0xCD, "", "", TBL_INV},
	{0xCE, "", "", TBL_INV},
	{0xCF, "", "", TBL_INV},
	{0xD0, "TRANSLATE AND TEST REVERSE", "TRTR", IFMT_SS1},
	{0xD1, "MOVE NUMERICS", "MVN", IFMT_SS1},
	{0xD2, "MOVE (character)", "MVC", IFMT_SS1},
	{0xD3, "MOVE ZONES", "MVZ", IFMT_SS1},
	{0xD4, "AND (character)", "NC", IFMT_SS1},
	{0xD5, "COMPARE LOGICAL (character)", "CLC", IFMT_SS1},
	{0xD6, "OR (character)", "OC", IFMT_SS1},
	{0xD7, "EXCLUSIVE OR (character)", "XC", IFMT_SS1},
	{0xD8, "", "", TBL_INV},
	{0xD9, "MOVE WITH KEY", "MVCK", IFMT_SS4},
	{0xDA, "MOVE TO PRIMARY", "MVCP", IFMT_SS4},
	{0xDB, "MOVE TO SECONDARY", "MVCS", IFMT_SS4},
	{0xDC, "TRANSLATE", "TR", IFMT_SS1},
	{0xDD, "TRANSLATE AND TEST", "TRT", IFMT_SS1},
	{0xDE, "EDIT", "ED", IFMT_SS1},
	{0xDF, "EDIT AND MARK", "EDMK", IFMT_SS1},
	{0xE0, "", "", TBL_INV},
	{0xE1, "PACK UNICODE", "PKU", IFMT_SS1},
	{0xE2, "UNPACK UNICODE", "UNPKU", IFMT_SS1},
	{0xE3, "", "", TBL_E3},
	{0xE4, "", "", TBL_INV},
	{0xE5, "", "", TBL_E5},
	{0xE6, "", "", TBL_INV},
	{0xE7, "", "", TBL_INV},
	{0xE8, "MOVE INVERSE", "MVCIN", IFMT_SS1},
	{0xE9, "PACK ASCII", "PKA", IFMT_SS1},
	{0xEA, "UNPACK ASCII", "UNPKA", IFMT_SS1},
	{0xEB, "", "", TBL_EB},
	{0xEC, "", "", TBL_EC},
	{0xED, "", "", TBL_ED},
	{0xEE, "PERFORM LOCKED OPERATION", "PLO", IFMT_SS5},
	{0xEF, "LOAD MULTIPLE DISJOINT", "LMD", IFMT_SS5},
	{0xF0, "SHIFT AND ROUND DECIMAL", "SRP", IFMT_SS3},
	{0xF1, "MOVE WITH OFFSET", "MVO", IFMT_SS1},
	{0xF2, "PACK", "PACK", IFMT_SS2},
	{0xF3, "UNPACK", "UNPK", IFMT_SS2},
	{0xF4, "", "", TBL_INV},
	{0xF5, "", "", TBL_INV},
	{0xF6, "", "", TBL_INV},
	{0xF7, "", "", TBL_INV},
	{0xF8, "ZERO AND ADD", "ZAP", IFMT_SS2},
	{0xF9, "COMPARE DECIMAL", "CP", IFMT_SS2},
	{0xFA, "ADD DECIMAL", "AP", IFMT_SS2},
	{0xFB, "SUBTRACT DECIMAL", "SP", IFMT_SS2},
	{0xFC, "MULTIPLY DECIMAL", "MP", IFMT_SS2},
	{0xFD, "DIVIDE DECIMAL", "DP", IFMT_SS2},
	{0xFE, "", "", TBL_INV},
	{0xFF, "", "", TBL_INV}
};

static opCode_t opc_01[] = {
	{0x01, "PROGRAM RETURN", "PR", IFMT_E},
	{0x02, "UPDATE TREE", "UPT", IFMT_E},
	{0x04, "PERFORM TIMING FACILITY FUNCTION", "PTFF", IFMT_E},
	{0x07, "SET CLOCK PROGRAMMABLE FIELD", "SCKPF", IFMT_E},
	{0x0A, "PERFORM FLOATING-POINT OPERATION", "PFPO", IFMT_E},
	{0x0B, "TEST ADDRESSING MODE", "TAM", IFMT_E},
	{0x0C, "SET ADDRESSING MODE (24)", "SAM24", IFMT_E},
	{0x0D, "SET ADDRESSING MODE (31)", "SAM31", IFMT_E},
	{0x0E, "SET ADDRESSING MODE (64)", "SAM64", IFMT_E},
	{0xFF, "TRAP", "TRAP2", IFMT_E},
	{0x00, "", "", TBL_INV}
};

opCode_t opc_a5[] = {
	{0x00, "INSERT IMMEDIATE (high high)", "IIHH", IFMT_RI1},
	{0x01, "INSERT IMMEDIATE (high low)", "IIHL", IFMT_RI1},
	{0x02, "INSERT IMMEDIATE (low high)", "IILH", IFMT_RI1},
	{0x03, "INSERT IMMEDIATE (low low)", "IILL", IFMT_RI1},
	{0x04, "AND IMMEDIATE (high high)", "NIHH", IFMT_RI1},
	{0x05, "AND IMMEDIATE (high low)", "NIHL", IFMT_RI1},
	{0x06, "AND IMMEDIATE (low high)", "NILH", IFMT_RI1},
	{0x07, "AND IMMEDIATE (low low)", "NILL", IFMT_RI1},
	{0x08, "OR IMMEDIATE (high high)", "OIHH", IFMT_RI1},
	{0x09, "OR IMMEDIATE (high low)", "OIHL", IFMT_RI1},
	{0x0A, "OR IMMEDIATE (low high)", "OILH", IFMT_RI1},
	{0x0B, "OR IMMEDIATE (low low)", "OILL", IFMT_RI1},
	{0x0C, "LOAD LOGICAL IMMEDIATE (high high)", "LLIHH", IFMT_RI1},
	{0x0D, "LOAD LOGICAL IMMEDIATE (high low)", "LLIHL", IFMT_RI1},
	{0x0E, "LOAD LOGICAL IMMEDIATE (low high)", "LLILH", IFMT_RI1},
	{0x0F, "LOAD LOGICAL IMMEDIATE (low low)", "LLILL", IFMT_RI1},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_a7[] = {
	{0x00, "TEST UNDER MASK (low high)", "TMLH", IFMT_RI1},
	{0x01, "TEST UNDER MASK (low low)", "TMLL", IFMT_RI1},
	{0x02, "TEST UNDER MASK (high high)", "TMHH", IFMT_RI1},
	{0x03, "TEST UNDER MASK (high low)", "TMHL", IFMT_RI1},
	{0x04, "BRANCH RELATIVE ON CONDITION", "BRC", IFMT_RI2},
	{0x05, "BRANCH RELATIVE AND SAVE", "BRAS", IFMT_RI1},
	{0x06, "BRANCH RELATIVE ON COUNT (32)", "BRCT", IFMT_RI1},
	{0x07, "BRANCH RELATIVE ON COUNT (64)", "BRCTG", IFMT_RI1},
	{0x08, "LOAD HALFWORD IMMEDIATE (32)", "LHI", IFMT_RI1},
	{0x09, "LOAD HALFWORD IMMEDIATE (64)", "LGHI", IFMT_RI1},
	{0x0A, "ADD HALFWORD IMMEDIATE (32)", "AHI", IFMT_RI1},
	{0x0B, "ADD HALFWORD IMMEDIATE (64)", "AGHI", IFMT_RI1},
	{0x0C, "MULTIPLY HALFWORD IMMEDIATE (32)", "MHI", IFMT_RI1},
	{0x0D, "MULTIPLY HALFWORD IMMEDIATE (64)", "MGHI", IFMT_RI1},
	{0x0E, "COMPARE HALFWORD IMMEDIATE (32)", "CHI", IFMT_RI1},
	{0x0F, "COMPARE HALFWORD IMMEDIATE (64)", "CGHI", IFMT_RI1},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_b2[256] = {
	{0x00, "", "", TBL_INV},
	{0x01, "", "", TBL_INV},
	{0x02, "STORE CPU ID", "STIDP", IFMT_S},
	{0x03, "", "", TBL_INV},
	{0x04, "SET CLOCK", "SCK", IFMT_S},
	{0x05, "STORE CLOCK", "STCK", IFMT_S},
	{0x06, "SET CLOCK COMPARATOR", "SCKC", IFMT_S},
	{0x07, "STORE CLOCK COMPARATOR", "STCKC", IFMT_S},
	{0x08, "SET CPU TIMER", "SPT", IFMT_S},
	{0x09, "STORE CPU TIMER", "STPT", IFMT_S},
	{0x0A, "SET PSW KEY FROM ADDRESS", "SPKA", IFMT_S},
	{0x0B, "INSERT PSW KEY", "IPK", IFMT_S},
	{0x0D, "PURGE TLB", "PTLB", IFMT_S},
	{0x0e, "", "", TBL_INV},
	{0x0f, "", "", TBL_INV},
	{0x10, "SET PREFIX", "SPX", IFMT_S},
	{0x11, "STORE PREFIX", "STPX", IFMT_S},
	{0x12, "STORE CPU ADDRESS", "STAP", IFMT_S},
	{0x13, "", "", TBL_INV},
	{0x14, "", "", TBL_INV},
	{0x15, "", "", TBL_INV},
	{0x16, "", "", TBL_INV},
	{0x17, "", "", TBL_INV},
	{0x18, "PROGRAM CALL", "PC", IFMT_S},
	{0x19, "SET ADDRESS SPACE CONTROL", "SAC", IFMT_S},
	{0x1A, "COMPARE AND FORM CODEWORD", "CFC", IFMT_S},
	{0x1B, "", "", TBL_INV},
	{0x1C, "", "", TBL_INV},
	{0x1D, "", "", TBL_INV},
	{0x1E, "", "", TBL_INV},
	{0x1F, "", "", TBL_INV},
	{0x20, "", "", TBL_INV},
	{0x21, "INVALIDATE PAGE TABLE ENTRY", "IPTE", IFMT_RRE},
	{0x22, "INSERT PROGRAM MASK", "IPM", IFMT_RRE},
	{0x23, "INSERT VIRTUAL STORAGE KEY", "IVSK", IFMT_RRE},
	{0x24, "INSERT ADDRESS SPACE CONTROL", "IAC", IFMT_RRE},
	{0x25, "SET SECONDARY ASN", "SSAR", IFMT_RRE},
	{0x26, "EXTRACT PRIMARY ASN", "EPAR", IFMT_RRE},
	{0x27, "EXTRACT SECONDARY ASN", "ESAR", IFMT_RRE},
	{0x28, "PROGRAM TRANSFER", "PT", IFMT_RRE},
	{0x29, "INSERT STORAGE KEY EXTENDED", "ISKE", IFMT_RRE},
	{0x2A, "RESET REFERENCE BIT EXTENDED", "RRBE", IFMT_RRE},
	{0x2B, "SET STORAGE KEY EXTENDED", "SSKE", IFMT_RRF2},
	{0x2C, "TEST BLOCK", "TB", IFMT_RRE},
	{0x2D, "DIVIDE (extended HFP)", "DXR", IFMT_RRE},
	{0x2E, "PAGE IN", "PGIN", IFMT_RRE},
	{0x2F, "PAGE OUT", "PGOUT", IFMT_RRE},
	{0x30, "CLEAR SUBCHANNEL", "CSCH", IFMT_S},
	{0x31, "HALT SUBCHANNEL", "HSCH", IFMT_S},
	{0x32, "MODIFY SUBCHANNEL", "MSCH", IFMT_S},
	{0x33, "START SUBCHANNEL", "SSCH", IFMT_S},
	{0x34, "STORE SUBCHANNEL", "STSCH", IFMT_S},
	{0x35, "TEST SUBCHANNEL", "TSCH", IFMT_S},
	{0x36, "TEST PENDING INTERRUPTION", "TPI", IFMT_S},
	{0x37, "SET ADDRESS LIMIT", "SAL", IFMT_S},
	{0x38, "RESUME SUBCHANNEL", "RSCH", IFMT_S},
	{0x39, "STORE CHANNEL REPORT WORD", "STCRW", IFMT_S},
	{0x3A, "STORE CHANNEL PATH STATUS", "STCPS", IFMT_S},
	{0x3B, "RESET CHANNEL PATH", "RCHP", IFMT_S},
	{0x3C, "SET CHANNEL MONITOR", "SCHM", IFMT_S},
	{0x3D, "", "", TBL_INV},
	{0x3E, "", "", TBL_INV},
	{0x3F, "", "", TBL_INV},
	{0x40, "BRANCH AND STACK", "BAKR", IFMT_RRE},
	{0x41, "CHECKSUM", "CKSM", IFMT_RRE},
	{0x42, "", "", TBL_INV},
	{0x43, "", "", TBL_INV},
	{0x44, "SQUARE ROOT (long HFP)", "SQDR", IFMT_RRE},
	{0x45, "SQUARE ROOT (short HFP)", "SQER", IFMT_RRE},
	{0x46, "STORE USING REAL ADDRESS (32)", "STURA", IFMT_RRE},
	{0x47, "MODIFY STACKED STATE", "MSTA", IFMT_RRE},
	{0x48, "PURGE ALB", "PALB", IFMT_RRE},
	{0x49, "EXTRACT STACKED REGISTERS (32)", "EREG", IFMT_RRE},
	{0x4A, "EXTRACT STACKED STATE", "ESTA", IFMT_RRE},
	{0x4B, "LOAD USING REAL ADDRESS (32)", "LURA", IFMT_RRE},
	{0x4C, "TEST ACCESS", "TAR", IFMT_RRE},
	{0x4D, "COPY ACCESS", "CPYA", IFMT_RRE},
	{0x4E, "SET ACCESS", "SAR", IFMT_RRE},
	{0x4F, "EXTRACT ACCESS", "EAR", IFMT_RRE},
	{0x50, "COMPARE AND SWAP AND PURGE", "CSP", IFMT_RRE},
	{0x51, "", "", TBL_INV},
	{0x52, "MULTIPLY SINGLE (32)", "MSR", IFMT_RRE},
	{0x53, "", "", TBL_INV},
	{0x54, "MOVE PAGE", "MVPG", IFMT_RRE},
	{0x55, "MOVE STRING", "MVST", IFMT_RRE},
	{0x56, "", "", TBL_INV},
	{0x57, "COMPARE UNTIL SUBSTRING EQUAL", "CUSE", IFMT_RRE},
	{0x58, "BRANCH IN SUBSPACE GROUP", "BSG", IFMT_RRE},
	{0x59, "", "", TBL_INV},
	{0x5A, "BRANCH AND SET AUTHORITY", "BSA", IFMT_RRE},
	{0x5B, "", "", TBL_INV},
	{0x5C, "", "", TBL_INV},
	{0x5D, "COMPARE LOGICAL STRING", "CLST", IFMT_RRE},
	{0x5E, "SEARCH STRING", "SRST", IFMT_RRE},
	{0x5F, "", "", TBL_INV},
	{0x60, "", "", TBL_INV},
	{0x61, "", "", TBL_INV},
	{0x62, "", "", TBL_INV},
	{0x63, "COMPRESSION CALL", "CMPSC", IFMT_RRE},
	{0x64, "", "", TBL_INV},
	{0x65, "", "", TBL_INV},
	{0x66, "", "", TBL_INV},
	{0x67, "", "", TBL_INV},
	{0x68, "", "", TBL_INV},
	{0x69, "", "", TBL_INV},
	{0x6A, "", "", TBL_INV},
	{0x6B, "", "", TBL_INV},
	{0x6C, "", "", TBL_INV},
	{0x6D, "", "", TBL_INV},
	{0x6E, "", "", TBL_INV},
	{0x6F, "", "", TBL_INV},
	{0x70, "", "", TBL_INV},
	{0x71, "", "", TBL_INV},
	{0x72, "", "", TBL_INV},
	{0x73, "", "", TBL_INV},
	{0x74, "", "", TBL_INV},
	{0x75, "", "", TBL_INV},
	{0x76, "CANCEL SUBCHANNEL", "XSCH", IFMT_S},
	{0x77, "RESUME PROGRAM", "RP", IFMT_S},
	{0x78, "STORE CLOCK EXTENDED", "STCKE", IFMT_S},
	{0x79, "SET ADDRESS SPACE CONTROL FAST", "SACF", IFMT_S},
	{0x7C, "STORE CLOCK FAST", "STCKF", IFMT_S},
	{0x7D, "STORE SYSTEM INFORMATION", "STSI", IFMT_S},
	{0x7E, "", "", TBL_INV},
	{0x7F, "", "", TBL_INV},
	{0x80, "", "", TBL_INV},
	{0x81, "", "", TBL_INV},
	{0x82, "", "", TBL_INV},
	{0x83, "", "", TBL_INV},
	{0x84, "", "", TBL_INV},
	{0x85, "", "", TBL_INV},
	{0x86, "", "", TBL_INV},
	{0x87, "", "", TBL_INV},
	{0x88, "", "", TBL_INV},
	{0x89, "", "", TBL_INV},
	{0x8A, "", "", TBL_INV},
	{0x8B, "", "", TBL_INV},
	{0x8C, "", "", TBL_INV},
	{0x8D, "", "", TBL_INV},
	{0x8E, "", "", TBL_INV},
	{0x8F, "", "", TBL_INV},
	{0x90, "", "", TBL_INV},
	{0x91, "", "", TBL_INV},
	{0x92, "", "", TBL_INV},
	{0x93, "", "", TBL_INV},
	{0x94, "", "", TBL_INV},
	{0x95, "", "", TBL_INV},
	{0x96, "", "", TBL_INV},
	{0x97, "", "", TBL_INV},
	{0x98, "", "", TBL_INV},
	{0x99, "SET BFP ROUNDING MODE", "SRNM", IFMT_S},
	{0x9A, "", "", TBL_INV},
	{0x9B, "", "", TBL_INV},
	{0x9C, "STORE FPC", "STFPC", IFMT_S},
	{0x9D, "LOAD FPC", "LFPC", IFMT_S},
	{0x9E, "", "", TBL_INV},
	{0x9F, "", "", TBL_INV},
	{0xA0, "", "", TBL_INV},
	{0xA1, "", "", TBL_INV},
	{0xA2, "", "", TBL_INV},
	{0xA3, "", "", TBL_INV},
	{0xA4, "", "", TBL_INV},
	{0xA5, "TRANSLATE EXTENDED", "TRE", IFMT_RRE},
	{0xA6, "CONVERT UNICODE TO UTF-8", "CUUTF", IFMT_RRF2},
	{0xA7, "CONVERT UTF-8 TO UNICODE", "CUTFU", IFMT_RRF2},
	{0xA8, "", "", TBL_INV},
	{0xA9, "", "", TBL_INV},
	{0xAA, "", "", TBL_INV},
	{0xAB, "", "", TBL_INV},
	{0xAC, "", "", TBL_INV},
	{0xAD, "", "", TBL_INV},
	{0xAE, "", "", TBL_INV},
	{0xAF, "", "", TBL_INV},
	{0xB0, "STORE FACILITY LIST EXTENDED", "STFLE", IFMT_S},
	{0xB1, "STORE FACILITY LIST", "STFL", IFMT_S},
	{0xB2, "LOAD PSW EXTENDED", "LPSWE", IFMT_S},
	{0xB3, "", "", TBL_INV},
	{0xB4, "", "", TBL_INV},
	{0xB5, "", "", TBL_INV},
	{0xB6, "", "", TBL_INV},
	{0xB7, "", "", TBL_INV},
	{0xB8, "", "", TBL_INV},
	{0xB9, "SET DFP ROUNDING MODE", "SRNMT", IFMT_S},
	{0xBD, "LOAD FPC AND SIGNAL", "LFAS", IFMT_S},
	{0xBA, "", "", TBL_INV},
	{0xBB, "", "", TBL_INV},
	{0xBC, "", "", TBL_INV},
	{0xBD, "", "", TBL_INV},
	{0xBE, "", "", TBL_INV},
	{0xBF, "", "", TBL_INV},
	{0xC0, "", "", TBL_INV},
	{0xC1, "", "", TBL_INV},
	{0xC2, "", "", TBL_INV},
	{0xC3, "", "", TBL_INV},
	{0xC4, "", "", TBL_INV},
	{0xC5, "", "", TBL_INV},
	{0xC6, "", "", TBL_INV},
	{0xC7, "", "", TBL_INV},
	{0xC8, "", "", TBL_INV},
	{0xC9, "", "", TBL_INV},
	{0xCA, "", "", TBL_INV},
	{0xCB, "", "", TBL_INV},
	{0xCC, "", "", TBL_INV},
	{0xCD, "", "", TBL_INV},
	{0xCE, "", "", TBL_INV},
	{0xCF, "", "", TBL_INV},
	{0xD0, "", "", TBL_INV},
	{0xD1, "", "", TBL_INV},
	{0xD2, "", "", TBL_INV},
	{0xD3, "", "", TBL_INV},
	{0xD4, "", "", TBL_INV},
	{0xD5, "", "", TBL_INV},
	{0xD6, "", "", TBL_INV},
	{0xD7, "", "", TBL_INV},
	{0xD8, "", "", TBL_INV},
	{0xD9, "", "", TBL_INV},
	{0xDA, "", "", TBL_INV},
	{0xDB, "", "", TBL_INV},
	{0xDC, "", "", TBL_INV},
	{0xDD, "", "", TBL_INV},
	{0xDE, "", "", TBL_INV},
	{0xDF, "", "", TBL_INV},
	{0xE0, "", "", TBL_INV},
	{0xE1, "", "", TBL_INV},
	{0xE2, "", "", TBL_INV},
	{0xE3, "", "", TBL_INV},
	{0xE4, "", "", TBL_INV},
	{0xE5, "", "", TBL_INV},
	{0xE6, "", "", TBL_INV},
	{0xE7, "", "", TBL_INV},
	{0xE8, "", "", TBL_INV},
	{0xE9, "", "", TBL_INV},
	{0xEA, "", "", TBL_INV},
	{0xEB, "", "", TBL_INV},
	{0xEC, "", "", TBL_INV},
	{0xED, "", "", TBL_INV},
	{0xEE, "", "", TBL_INV},
	{0xEF, "", "", TBL_INV},
	{0xF0, "", "", TBL_INV},
	{0xF1, "", "", TBL_INV},
	{0xF2, "", "", TBL_INV},
	{0xF3, "", "", TBL_INV},
	{0xF4, "", "", TBL_INV},
	{0xF5, "", "", TBL_INV},
	{0xF6, "", "", TBL_INV},
	{0xF7, "", "", TBL_INV},
	{0xF8, "", "", TBL_INV},
	{0xF9, "", "", TBL_INV},
	{0xFA, "", "", TBL_INV},
	{0xFB, "", "", TBL_INV},
	{0xFC, "", "", TBL_INV},
	{0xFD, "", "", TBL_INV},
	{0xFE, "", "", TBL_INV},
	{0xFF, "TRAP", "TRAP4", IFMT_S}
};

static opCode_t opc_b3[256] = {
	{0x00, "LOAD POSITIVE (short BFP)", "LPEBR", IFMT_RRE},
	{0x01, "LOAD NEGATIVE (short BFP)", "LNEBR", IFMT_RRE},
	{0x02, "LOAD AND TEST (short BFP)", "LTEBR", IFMT_RRE},
	{0x03, "LOAD COMPLEMENT (short BFP)", "LCEBR", IFMT_RRE},
	{0x04, "LOAD LENGTHENED (short to long BFP)", "LDEBR", IFMT_RRE},
	{0x05, "LOAD LENGTHENED (long to extended BFP)", "LXDBR", IFMT_RRE},
	{0x06, "LOAD LENGTHENED (short to extended BFP)", "LXEBR", IFMT_RRE},
	{0x07, "MULTIPLY (long to extended BFP)", "MXDBR", IFMT_RRE},
	{0x08, "COMPARE AND SIGNAL (short BFP)", "KEBR", IFMT_RRE},
	{0x09, "COMPARE (short BFP)", "CEBR", IFMT_RRE},
	{0x0A, "ADD (short BFP)", "AEBR", IFMT_RRE},
	{0x0B, "SUBTRACT (short BFP)", "SEBR", IFMT_RRE},
	{0x0C, "MULTIPLY (short to long BFP)", "MDEBR", IFMT_RRE},
	{0x0D, "DIVIDE (short BFP)", "DEBR", IFMT_RRE},
	{0x0E, "MULTIPLY AND ADD (short BFP)", "MAEBR", IFMT_RRF1},
	{0x0F, "MULTIPLY AND SUBTRACT (short BFP)", "MSEBR", IFMT_RRF1},
	{0x10, "LOAD POSITIVE (long BFP)", "LPDBR", IFMT_RRE},
	{0x11, "LOAD NEGATIVE (long BFP)", "LNDBR", IFMT_RRE},
	{0x12, "LOAD AND TEST (long BFP)", "LTDBR", IFMT_RRE},
	{0x13, "LOAD COMPLEMENT (long BFP)", "LCDBR", IFMT_RRE},
	{0x14, "SQUARE ROOT (short BFP)", "SQEBR", IFMT_RRE},
	{0x15, "SQUARE ROOT (long BFP)", "SQDBR", IFMT_RRE},
	{0x16, "SQUARE ROOT (extended BFP)", "SQXBR", IFMT_RRE},
	{0x17, "MULTIPLY (short BFP)", "MEEBR", IFMT_RRE},
	{0x18, "COMPARE AND SIGNAL (long BFP)", "KDBR", IFMT_RRE},
	{0x19, "COMPARE (long BFP)", "CDBR", IFMT_RRE},
	{0x1A, "ADD (long BFP)", "ADBR", IFMT_RRE},
	{0x1B, "SUBTRACT (long BFP)", "SDBR", IFMT_RRE},
	{0x1C, "MULTIPLY (long BFP)", "MDBR", IFMT_RRE},
	{0x1D, "DIVIDE (long BFP)", "DDBR", IFMT_RRE},
	{0x1E, "MULTIPLY AND ADD (long BFP)", "MADBR", IFMT_RRF1},
	{0x1F, "MULTIPLY AND SUBTRACT (long BFP)", "MSDBR", IFMT_RRF1},
	{0x20, "", "", TBL_INV},
	{0x21, "", "", TBL_INV},
	{0x22, "", "", TBL_INV},
	{0x23, "", "", TBL_INV},
	{0x24, "LOAD LENGTHENED (short to long HFP)", "LDER", IFMT_RRE},
	{0x25, "LOAD LENGTHENED (long to extended HFP)", "LXDR", IFMT_RRE},
	{0x26, "LOAD LENGTHENED (short to extended HFP)", "LXER", IFMT_RRE},
	{0x27, "", "", TBL_INV},
	{0x28, "", "", TBL_INV},
	{0x29, "", "", TBL_INV},
	{0x2A, "", "", TBL_INV},
	{0x2B, "", "", TBL_INV},
	{0x2C, "", "", TBL_INV},
	{0x2D, "", "", TBL_INV},
	{0x2E, "MULTIPLY AND ADD (short HFP)", "MAER", IFMT_RRF1},
	{0x2F, "MULTIPLY AND SUBTRACT (short HFP)", "MSER", IFMT_RRF1},
	{0x30, "", "", TBL_INV},
	{0x31, "", "", TBL_INV},
	{0x32, "", "", TBL_INV},
	{0x33, "", "", TBL_INV},
	{0x34, "", "", TBL_INV},
	{0x35, "", "", TBL_INV},
	{0x36, "SQUARE ROOT (extended HFP)", "SQXR", IFMT_RRE},
	{0x37, "MULTIPLY (short HFP)", "MEER", IFMT_RRE},
	{0x38, "MULTIPLY AND ADD UNNRM. (long to ext. low HFP)", "MAYLR", IFMT_RRF1},
	{0x39, "MULTIPLY UNNORM. (long to ext. low HFP)", "MYLR", IFMT_RRF1},
	{0x3A, "MULTIPLY & ADD UNNORMALIZED (long to ext. HFP)", "MAYR", IFMT_RRF1},
	{0x3B, "MULTIPLY UNNORMALIZED (long to ext. HFP)", "MYR", IFMT_RRF1},
	{0x3C, "MULTIPLY AND ADD UNNRM. (long to ext. high HFP)", "MAYHR", IFMT_RRF1},
	{0x3D, "MULTIPLY UNNORM. (long to ext. high HFP)", "MYHR", IFMT_RRF1},
	{0x3E, "MULTIPLY AND ADD (long HFP)", "MADR", IFMT_RRF1},
	{0x3F, "MULTIPLY AND SUBTRACT (long HFP)", "MSDR", IFMT_RRF1},
	{0x40, "LOAD POSITIVE (extended BFP)", "LPXBR", IFMT_RRE},
	{0x41, "LOAD NEGATIVE (extended BFP)", "LNXBR", IFMT_RRE},
	{0x42, "LOAD AND TEST (extended BFP)", "LTXBR", IFMT_RRE},
	{0x43, "LOAD COMPLEMENT (extended BFP)", "LCXBR", IFMT_RRE},
	{0x44, "LOAD ROUNDED (long to short BFP)", "LEDBR", IFMT_RRE},
	{0x45, "LOAD ROUNDED (extended to long BFP)", "LDXBR", IFMT_RRE},
	{0x46, "LOAD ROUNDED (extended to short BFP)", "LEXBR", IFMT_RRE},
	{0x47, "LOAD FP INTEGER (extended BFP)", "FIXBR", IFMT_RRF2},
	{0x48, "COMPARE AND SIGNAL (extended BFP)", "KXBR", IFMT_RRE},
	{0x49, "COMPARE (extended BFP)", "CXBR", IFMT_RRE},
	{0x4A, "ADD (extended BFP)", "AXBR", IFMT_RRE},
	{0x4B, "SUBTRACT (extended BFP)", "SXBR", IFMT_RRE},
	{0x4C, "MULTIPLY (extended BFP)", "MXBR", IFMT_RRE},
	{0x4D, "DIVIDE (extended BFP)", "DXBR", IFMT_RRE},
	{0x4E, "", "", TBL_INV},
	{0x4F, "", "", TBL_INV},
	{0x50, "CONVERT HFP TO BFP (long to short)", "TBEDR", IFMT_RRF2},
	{0x51, "CONVERT HFP TO BFP (long)", "TBDR", IFMT_RRF2},
	{0x52, "", "", TBL_INV},
	{0x53, "DIVIDE TO INTEGER (short BFP)", "DIEBR", IFMT_RRF2},
	{0x54, "", "", TBL_INV},
	{0x55, "", "", TBL_INV},
	{0x56, "", "", TBL_INV},
	{0x57, "LOAD FP INTEGER (short BFP)", "FIEBR", IFMT_RRF2},
	{0x58, "CONVERT BFP TO HFP (short to long)", "THDER", IFMT_RRE},
	{0x59, "CONVERT BFP TO HFP (long)", "THDR", IFMT_RRE},
	{0x5A, "", "", TBL_INV},
	{0x5B, "DIVIDE TO INTEGER (long BFP)", "DIDBR", IFMT_RRF3},
	{0x5C, "", "", TBL_INV},
	{0x5D, "", "", TBL_INV},
	{0x5E, "", "", TBL_INV},
	{0x5F, "LOAD FP INTEGER (long BFP)", "FIDBR", IFMT_RRF2},
	{0x60, "LOAD POSITIVE (extended HFP)", "LPXR", IFMT_RRE},
	{0x61, "LOAD NEGATIVE (extended HFP)", "LNXR", IFMT_RRE},
	{0x62, "LOAD AND TEST (extended HFP)", "LTXR", IFMT_RRE},
	{0x63, "LOAD COMPLEMENT (extended HFP)", "LCXR", IFMT_RRE},
	{0x64, "", "", TBL_INV},
	{0x65, "LOAD (extended)", "LXR", IFMT_RRE},
	{0x66, "LOAD ROUNDED (extended to short HFP)", "LEXR", IFMT_RRE},
	{0x67, "LOAD FP INTEGER (extended HFP)", "FIXR", IFMT_RRE},
	{0x68, "", "", TBL_INV},
	{0x69, "COMPARE (extended HFP)", "CXR", IFMT_RRE},
	{0x6A, "", "", TBL_INV},
	{0x6B, "", "", TBL_INV},
	{0x6C, "", "", TBL_INV},
	{0x6D, "", "", TBL_INV},
	{0x6E, "", "", TBL_INV},
	{0x6F, "", "", TBL_INV},
	{0x70, "LOAD POSITIVE (long)", "LPDFR", IFMT_RRE},
	{0x71, "LOAD NEGATIVE (long)", "LNDFR", IFMT_RRE},
	{0x72, "COPY SIGN (long)", "CPSDR", IFMT_RRF1},
	{0x73, "LOAD COMPLEMENT (long)", "LCDFR", IFMT_RRE },
	{0x74, "LOAD ZERO (short)", "LZER", IFMT_RRE},
	{0x75, "LOAD ZERO (long)", "LZDR", IFMT_RRE},
	{0x76, "LOAD ZERO (extended)", "LZXR", IFMT_RRE},
	{0x77, "LOAD FP INTEGER (short HFP)", "FIER", IFMT_RRE},
	{0x78, "", "", TBL_INV},
	{0x79, "", "", TBL_INV},
	{0x7A, "", "", TBL_INV},
	{0x7B, "", "", TBL_INV},
	{0x7C, "", "", TBL_INV},
	{0x7D, "", "", TBL_INV},
	{0x7E, "", "", TBL_INV},
	{0x7F, "LOAD FP INTEGER (long HFP)", "FIDR", IFMT_RRE},
	{0x80, "", "", TBL_INV},
	{0x81, "", "", TBL_INV},
	{0x82, "", "", TBL_INV},
	{0x83, "", "", TBL_INV},
	{0x84, "SET FPC", "SFPC", IFMT_RRE},
	{0x85, "SET FPC AND SIGNAL", "SFASR", IFMT_RRE},
	{0x86, "", "", TBL_INV},
	{0x87, "", "", TBL_INV},
	{0x88, "", "", TBL_INV},
	{0x89, "", "", TBL_INV},
	{0x8A, "", "", TBL_INV},
	{0x8B, "", "", TBL_INV},
	{0x8C, "EXTRACT FPC", "EFPC", IFMT_RRE},
	{0x8D, "", "", TBL_INV},
	{0x8E, "", "", TBL_INV},
	{0x8F, "", "", TBL_INV},
	{0x90, "", "", TBL_INV},
	{0x91, "", "", TBL_INV},
	{0x92, "", "", TBL_INV},
	{0x93, "", "", TBL_INV},
	{0x94, "CONVERT FROM FIXED (32 to short BFP)", "CEFBR", IFMT_RRE},
	{0x95, "CONVERT FROM FIXED (32 to long BFP)", "CDFBR", IFMT_RRE},
	{0x96, "CONVERT FROM FIXED (32 to extended BFP)", "CXFBR", IFMT_RRE},
	{0x97, "", "", TBL_INV},
	{0x98, "CONVERT TO FIXED (short BFP to 32)", "CFEBR", IFMT_RRF2},
	{0x99, "CONVERT TO FIXED (long BFP to 32)", "CFDBR", IFMT_RRF2},
	{0x9A, "CONVERT TO FIXED (extended BFP to 32)", "CFXBR", IFMT_RRF2},
	{0x9B, "", "", TBL_INV},
	{0x9C, "", "", TBL_INV},
	{0x9D, "", "", TBL_INV},
	{0x9E, "", "", TBL_INV},
	{0x9F, "", "", TBL_INV},
	{0xA0, "", "", TBL_INV},
	{0xA1, "", "", TBL_INV},
	{0xA2, "", "", TBL_INV},
	{0xA3, "", "", TBL_INV},
	{0xA4, "CONVERT FROM FIXED (64 to short BFP)", "CEGBR", IFMT_RRE},
	{0xA5, "CONVERT FROM FIXED (64 to long BFP)", "CDGBR", IFMT_RRE},
	{0xA6, "CONVERT FROM FIXED (64 to extended BFP)", "CXGBR", IFMT_RRE},
	{0xA7, "", "", TBL_INV},
	{0xA8, "CONVERT TO FIXED (short BFP to 64)", "CGEBR", IFMT_RRF2},
	{0xA9, "CONVERT TO FIXED (long BFP to 64)", "CGDBR", IFMT_RRF2},
	{0xAA, "CONVERT TO FIXED (extended BFP to 64)", "CGXBR", IFMT_RRF2},
	{0xAB, "", "", TBL_INV},
	{0xAC, "", "", TBL_INV},
	{0xAD, "", "", TBL_INV},
	{0xAE, "", "", TBL_INV},
	{0xAF, "", "", TBL_INV},
	{0xB0, "", "", TBL_INV},
	{0xB1, "", "", TBL_INV},
	{0xB2, "", "", TBL_INV},
	{0xB3, "", "", TBL_INV},
	{0xB4, "CONVERT FROM FIXED (32 to short HFP)", "CEFR", IFMT_RRE},
	{0xB5, "CONVERT FROM FIXED (32 to long HFP)", "CDFR", IFMT_RRE},
	{0xB6, "CONVERT FROM FIXED (32 to extended HFP)", "CXFR", IFMT_RRE},
	{0xB7, "", "", TBL_INV},
	{0xB8, "CONVERT TO FIXED (short HFP to 32)", "CFER", IFMT_RRF2},
	{0xB9, "CONVERT TO FIXED (long HFP to 32)", "CFDR", IFMT_RRF2},
	{0xBA, "CONVERT TO FIXED (extended HFP to 32)", "CFXR", IFMT_RRF2},
	{0xBB, "", "", TBL_INV},
	{0xBC, "", "", TBL_INV},
	{0xBD, "", "", TBL_INV},
	{0xBE, "", "", TBL_INV},
	{0xBF, "", "", TBL_INV},
	{0xC0, "", "", TBL_INV},
	{0xC1, "LOAD FPR FROM GR (64 to long)", "LDGR", IFMT_RRE},
	{0xC2, "", "", TBL_INV},
	{0xC3, "", "", TBL_INV},
	{0xC4, "CONVERT FROM FIXED (64 to short HFP)", "CEGR", IFMT_RRE},
	{0xC5, "CONVERT FROM FIXED (64 to long HFP)", "CDGR", IFMT_RRE},
	{0xC6, "CONVERT FROM FIXED (64 to extended HFP)", "CXGR", IFMT_RRE},
	{0xC7, "", "", TBL_INV},
	{0xC8, "CONVERT TO FIXED (short HFP to 64)", "CGER", IFMT_RRF2},
	{0xC9, "CONVERT TO FIXED (long HFP to 64)", "CGDR", IFMT_RRF2},
	{0xCA, "CONVERT TO FIXED (extended HFP to 64)", "CGXR", IFMT_RRF2},
	{0xCB, "", "", TBL_INV},
	{0xCC, "", "", TBL_INV},
	{0xCD, "LOAD GR FROM FPR (long to 64)", "LGDR", IFMT_RRE},
	{0xCE, "", "", TBL_INV},
	{0xCF, "", "", TBL_INV},
	{0xD0, "MULTIPLY (long DFP)", "MDTR", IFMT_RRR},
	{0xD1, "DIVIDE (long DFP)", "DDTR", IFMT_RRR},
	{0xD2, "ADD (long DFP)", "ADTR", IFMT_RRR},
	{0xD3, "SUBTRACT (long DFP)", "SDTR", IFMT_RRR},
	{0xD4, "LOAD LENGTHENED (short to long DFP)", "LDETR", IFMT_RRF1},
	{0xD5, "LOAD ROUNDED (long to short DFP)", "LEDTR", IFMT_RRF3},
	{0xD6, "LOAD AND TEST (long DFP)", "LTDTR", IFMT_RRE},
	{0xD7, "LOAD FP INTEGER (long DFP)", "FIDTR", IFMT_RRF1},
	{0xD8, "MULTIPLY (extended DFP)", "MXTR", IFMT_RRR},
	{0xD9, "DIVIDE (extended DFP)", "DXTR", IFMT_RRR},
	{0xDA, "ADD (extended DFP)", "AXTR", IFMT_RRR},
	{0xDB, "SUBTRACT (extended DFP)", "SXTR", IFMT_RRR},
	{0xDC, "LOAD LENGTHENED (long to extended DFP)", "LXDTR", IFMT_RRF1},
	{0xDD, "LOAD ROUNDED (extended to long DFP)", "LDXTR", IFMT_RRF3},
	{0xDE, "LOAD AND TEST (extended DFP)", "LTXTR", IFMT_RRE},
	{0xDF, "LOAD FP INTEGER (extended DFP)", "FIXTR", IFMT_RRF1},
	{0xE0, "COMPARE AND SIGNAL (long DFP)", "KDTR", IFMT_RRE},
	{0xE1, "CONVERT TO FIXED (long DFP to 64)", "CGDTR", IFMT_RRF2},
	{0xE2, "CONVERT TO UNSIGNED PACKED (long DFP to 64)", "CUDTR", IFMT_RRE},
	{0xE3, "CONVERT TO SIGNED PACKED (long DFP to 64)", "CSDTR", IFMT_RRF2},
	{0xE4, "COMPARE (long DFP)", "CDTR", IFMT_RRE},
	{0xE5, "EXTRACT BIASED EXPONENT (long DFP to 64)", "EEDTR", IFMT_RRE},
	{0xE6, "", "", TBL_INV},
	{0xE7, "EXTRACT SIGNIFICANCE (long DFP)", "ESDTR", IFMT_RRE},
	{0xE8, "COMPARE AND SIGNAL (extended DFP)", "KXTR", IFMT_RRE},
	{0xE9, "CONVERT TO FIXED (extended DFP to 64)", "CGXTR", IFMT_RRF2},
	{0xEA, "CONVERT TO UNSIGNED PACKED (extended DFP to 128)", "CUXTR", IFMT_RRE},
	{0xEB, "CONVERT TO SIGNED PACKED (extended DFP to 128)", "CSXTR", IFMT_RRF2},
	{0xEC, "COMPARE (extended DFP)", "CXTR", IFMT_RRE},
	{0xED, "EXTRACT BIASED EXPONENT (extended DFP to 64)", "EEXTR", IFMT_RRE},
	{0xEF, "EXTRACT SIGNIFICANCE (extended DFP)", "ESXTR", IFMT_RRE},
	{0xF0, "", "", TBL_INV},
	{0xF1, "CONVERT FROM FIXED (64 to long DFP)", "CDGTR", IFMT_RRE},
	{0xF2, "CONVERT FROM UNSIGNED PACKED (64 to long DFP)", "CDUTR", IFMT_RRE},
	{0xF3, "CONVERT FROM SIGNED PACKED (64 to long DFP)", "CDSTR", IFMT_RRE},
	{0xF4, "COMPARE BIASED EXPONENT (long DFP)", "CEDTR", IFMT_RRE},
	{0xF5, "QUANTIZE (long DFP)", "QADTR", IFMT_RRF3},
	{0xF6, "INSERT BIASED EXPONENT (64 to long DFP)", "IEDTR", IFMT_RRF1},
	{0xF7, "REROUND (long DFP)", "RRDTR", IFMT_RRF3},
	{0xF8, "", "", TBL_INV},
	{0xF9, "CONVERT FROM FIXED (64 to extended DFP)", "CXGTR", IFMT_RRE},
	{0xFA, "CONVERT FROM UNSIGNED PACKED (128 to ext. DFP)", "CXUTR", IFMT_RRE},
	{0xFB, "CONVERT FROM SIGNED PACKED (128 to extended DFP)", "CXSTR", IFMT_RRE},
	{0xFC, "COMPARE BIASED EXPONENT (extended DFP)", "CEXTR", IFMT_RRE},
	{0xFD, "QUANTIZE (extended DFP)", "QAXTR", IFMT_RRF3},
	{0xFE, "INSERT BIASED EXPONENT (64 to extended DFP)", "IEXTR", IFMT_RRF1},
	{0xFF, "REROUND (extended DFP)", "RRXTR", IFMT_RRF3}
};

static opCode_t opc_b9[] = {
	{0x00, "LOAD POSITIVE (64)", "LPGR", IFMT_RRE},
	{0x01, "LOAD NEGATIVE (64)", "LNGR", IFMT_RRE},
	{0x02, "LOAD AND TEST (64)", "LTGR", IFMT_RRE},
	{0x03, "LOAD COMPLEMENT (64)", "LCGR", IFMT_RRE},
	{0x04, "LOAD (64)", "LGR", IFMT_RRE},
	{0x05, "LOAD USING REAL ADDRESS (64)", "LURAG", IFMT_RRE},
	{0x06, "LOAD BYTE (64)", "LGBR", IFMT_RRE},
	{0x07, "LOAD HALFWORD (64)", "LGHR", IFMT_RRE},
	{0x08, "ADD (64)", "AGR", IFMT_RRE},
	{0x09, "SUBTRACT (64)", "SGR", IFMT_RRE},
	{0x0A, "ADD LOGICAL (64)", "ALGR", IFMT_RRE},
	{0x0B, "SUBTRACT LOGICAL (64)", "SLGR", IFMT_RRE},
	{0x0C, "MULTIPLY SINGLE (64)", "MSGR", IFMT_RRE},
	{0x0D, "DIVIDE SINGLE (64)", "DSGR", IFMT_RRE},
	{0x0E, "EXTRACT STACKED REGISTERS (64)", "EREGG", IFMT_RRE},
	{0x0F, "LOAD REVERSED (64)", "LRVGR", IFMT_RRE},
	{0x10, "LOAD POSITIVE (64 32)", "LPGFR", IFMT_RRE},
	{0x11, "LOAD NEGATIVE (64 32)", "LNGFR", IFMT_RRE},
	{0x12, "LOAD AND TEST (64 32)", "LTGFR", IFMT_RRE},
	{0x13, "LOAD COMPLEMENT (64 32)", "LCGFR", IFMT_RRE},
	{0x14, "LOAD (64 32)", "LGFR", IFMT_RRE},
	{0x16, "LOAD LOGICAL (64 32)", "LLGFR", IFMT_RRE},
	{0x17, "LOAD LOGICAL THIRTY ONE BITS", "LLGTR", IFMT_RRE},
	{0x18, "ADD (64 32)", "AGFR", IFMT_RRE},
	{0x19, "SUBTRACT (64 32)", "SGFR", IFMT_RRE},
	{0x1A, "ADD LOGICAL (64 32)", "ALGFR", IFMT_RRE},
	{0x1B, "SUBTRACT LOGICAL (64 32)", "SLGFR", IFMT_RRE},
	{0x1C, "MULTIPLY SINGLE (64 32)", "MSGFR", IFMT_RRE},
	{0x1D, "DIVIDE SINGLE (64 32)", "DSGFR", IFMT_RRE},
	{0x1E, "COMPUTE MESSAGE AUTHENTICATION CODE", "KMAC", IFMT_RRE},
	{0x1F, "LOAD REVERSED (32)", "LRVR", IFMT_RRE},
	{0x20, "COMPARE (64)", "CGR", IFMT_RRE },
	{0x21, "COMPARE LOGICAL (64)", "CLGR", IFMT_RRE},
	{0x25, "STORE USING REAL ADDRESS (64)", "STURG", IFMT_RRE},
	{0x26, "LOAD BYTE (32)", "LBR", IFMT_RRE},
	{0x27, "LOAD HALFWORD (32)", "LHR", IFMT_RRE},
	{0x2E, "CIPHER MESSAGE", "KM", IFMT_RRE},
	{0x2F, "CIPHER MESSAGE WITH CHAINING", "KMC", IFMT_RRE},
	{0x30, "COMPARE (64 32)", "CGFR", IFMT_RRE},
	{0x31, "COMPARE LOGICAL (64 32)", "CLGFR", IFMT_RRE},
	{0x3E, "COMPUTE INTERMEDIATE MESSAGE DIGEST", "KIMD", IFMT_RRE},
	{0x3F, "COMPUTE LAST MESSAGE DIGEST", "KLMD", IFMT_RRE},
	{0x46, "BRANCH ON COUNT (64)", "BCTGR", IFMT_RRE},
	{0x80, "AND (64)", "NGR", IFMT_RRE},
	{0x81, "OR (64)", "OGR", IFMT_RRE},
	{0x82, "EXCLUSIVE OR (64)", "XGR", IFMT_RRE},
	{0x83, "FIND LEFTMOST ONE", "FLOGR", IFMT_RRE},
	{0x84, "LOAD LOGICAL CHARACTER (64)", "LLGCR", IFMT_RRE},
	{0x85, "LOAD LOGICAL HALFWORD (64)", "LLGHR", IFMT_RRE},
	{0x86, "MULTIPLY LOGICAL (128 64)", "MLGR", IFMT_RRE},
	{0x87, "DIVIDE LOGICAL (64 128)", "DLGR", IFMT_RRE},
	{0x88, "ADD LOGICAL WITH CARRY (64)", "ALCGR", IFMT_RRE},
	{0x89, "SUBTRACT LOGICAL WITH BORROW (64)", "SLBGR", IFMT_RRE},
	{0x8A, "COMPARE AND SWAP AND PURGE", "CSPG", IFMT_RRE},
	{0x8D, "EXTRACT PSW", "EPSW", IFMT_RRE},
	{0x8E, "INVALIDATE DAT TABLE ENTRY", "IDTE", IFMT_RRF1},
	{0x90, "TRANSLATE TWO TO TWO", "TRTT", IFMT_RRF1},
	{0x91, "TRANSLATE TWO TO ONE", "TRTO", IFMT_RRF1},
	{0x92, "TRANSLATE ONE TO TWO", "TROT", IFMT_RRF1},
	{0x93, "TRANSLATE ONE TO ONE", "TROO", IFMT_RRF1},
	{0x94, "LOAD LOGICAL CHARACTER (32)", "LLCR", IFMT_RRE},
	{0x95, "LOAD LOGICAL HALFWORD (32)", "LLHR", IFMT_RRE},
	{0x96, "MULTIPLY LOGICAL (64 32)", "MLR", IFMT_RRE},
	{0x97, "DIVIDE LOGICAL (32 64)", "DLR", IFMT_RRE},
	{0x98, "ADD LOGICAL WITH CARRY (32)", "ALCR", IFMT_RRE},
	{0x99, "SUBTRACT LOGICAL WITH BORROW (32)", "SLBR", IFMT_RRE},
	{0x9A, "EXTRACT PRIMARY ASN AND INSTANCE", "EPAIR", IFMT_RRE},
	{0x9B, "EXTRACT SECONDARY ASN AND INSTANCE", "ESAIR", IFMT_RRE},
	{0x9D, "EXTRACT AND SET EXTENDED AUTHORITY", "ESEA", IFMT_RRE},
	{0x9E, "PROGRAM TRANSFER WITH INSTANCE", "PTI", IFMT_RRE},
	{0x9F, "SET SECONDARY ASN WITH INSTANCE", "SSAIR", IFMT_RRE},
	{0xAA, "LOAD PAGE-TABLE-ENTRY ADDRESS", "LPTEA", IFMT_RRF3},
	{0xB0, "CONVERT UTF-8 TO UTF-32", "CU14", IFMT_RRF2},
	{0xB1, "CONVERT UTF-16 TO UTF-32", "CU24", IFMT_RRF2},
	{0xB2, "CONVERT UTF-32 TO UTF-8", "CU41", IFMT_RRE},
	{0xB3, "CONVERT UTF-32 TO UTF-16", "CU42", IFMT_RRE},
	{0xBE, "SEARCH STRING UNICODE", "SRSTU", IFMT_RRE},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_c0[] = {
	{0x00, "LOAD ADDRESS RELATIVE LONG", "LARL", IFMT_RIL1},
	{0x01, "LOAD IMMEDIATE (64 32)", "LGFI", IFMT_RIL1},
	{0x04, "BRANCH RELATIVE ON CONDITION LONG", "BRCL", IFMT_RIL2},
	{0x05, "BRANCH RELATIVE AND SAVE LONG", "BRASL", IFMT_RIL1},
	{0x06, "EXCLUSIVE OR IMMEDIATE (high)", "XIHF", IFMT_RIL1},
	{0x07, "EXCLUSIVE OR IMMEDIATE (low)", "XILF", IFMT_RIL1},
	{0x08, "INSERT IMMEDIATE (high)", "IIHF", IFMT_RIL1},
	{0x09, "INSERT IMMEDIATE (low)", "IILF", IFMT_RIL1},
	{0x0A, "AND IMMEDIATE (high)", "NIHF", IFMT_RIL1},
	{0x0B, "AND IMMEDIATE (low)", "NILF", IFMT_RIL1},
	{0x0C, "OR IMMEDIATE (high)", "OIHF", IFMT_RIL1},
	{0x0D, "OR IMMEDIATE (low)", "OILF", IFMT_RIL1},
	{0x0E, "LOAD LOGICAL IMMEDIATE (high)", "LLIHF", IFMT_RIL1},
	{0x0F, "LOAD LOGICAL IMMEDIATE (low)", "LLILF", IFMT_RIL1},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_c2[] = {
	{0x04, "SUBTRACT LOGICAL IMMEDIATE (64 32)", "SLGFI", IFMT_RIL1},
	{0x05, "SUBTRACT LOGICAL IMMEDIATE (32)", "SLFI", IFMT_RIL1},
	{0x08, "ADD IMMEDIATE (64 32)", "AGFI", IFMT_RIL1},
	{0x09, "ADD IMMEDIATE (32)", "AFI", IFMT_RIL1},
	{0x0A, "ADD LOGICAL IMMEDIATE (64 32)", "ALGFI", IFMT_RIL1},
	{0x0B, "ADD LOGICAL IMMEDIATE (32)", "ALFI", IFMT_RIL1},
	{0x0C, "COMPARE IMMEDIATE (64 32)", "CGFI", IFMT_RIL1},
	{0x0D, "COMPARE IMMEDIATE (32)", "CFI", IFMT_RIL1},
	{0x0E, "COMPARE LOGICAL IMMEDIATE (64 32)", "CLGFI", IFMT_RIL1},
	{0x0F, "COMPARE LOGICAL IMMEDIATE (32)", "CLFI", IFMT_RIL1},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_c8[] = {
	{0x01, "EXTRACT CPU TIME", "ECTG", IFMT_SSF},
	{0x02, "COMPARE AND SWAP AND STORE", "CSST", IFMT_SSF},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_e3[] = {
	{0x02, "LOAD AND TEST (64)", "LTG", IFMT_RXY},
	{0x03, "LOAD REAL ADDRESS (64)", "LRAG", IFMT_RXY},
	{0x04, "LOAD (64)", "LG", IFMT_RXY},
	{0x06, "CONVERT TO BINARY (32)", "CVBY", IFMT_RXY},
	{0x08, "ADD (64)", "AG", IFMT_RXY},
	{0x09, "SUBTRACT (64)", "SG", IFMT_RXY},
	{0x0A, "ADD LOGICAL (64)", "ALG", IFMT_RXY},
	{0x0B, "SUBTRACT LOGICAL (64)", "SLG", IFMT_RXY},
	{0x0C, "MULTIPLY SINGLE (64)", "MSG", IFMT_RXY},
	{0x0D, "DIVIDE SINGLE (64)", "DSG", IFMT_RXY},
	{0x0E, "CONVERT TO BINARY (64)", "CVBG", IFMT_RXY},
	{0x0F, "LOAD REVERSED (64)", "LRVG", IFMT_RXY},
	{0x12, "LOAD AND TEST (32)", "LT", IFMT_RXY},
	{0x13, "LOAD REAL ADDRESS (32)", "LRAY", IFMT_RXY},
	{0x14, "LOAD (64 32)", "LGF", IFMT_RXY},
	{0x15, "LOAD HALFWORD (64)", "LGH", IFMT_RXY},
	{0x16, "LOAD LOGICAL (64 32)", "LLGF", IFMT_RXY},
	{0x17, "LOAD LOGICAL THIRTY ONE BITS", "LLGT", IFMT_RXY},
	{0x18, "ADD (64 32)", "AGF", IFMT_RXY},
	{0x19, "SUBTRACT (64 32)", "SGF", IFMT_RXY},
	{0x1A, "ADD LOGICAL (64 32)", "ALGF", IFMT_RXY},
	{0x1B, "SUBTRACT LOGICAL (64 32)", "SLGF", IFMT_RXY},
	{0x1C, "MULTIPLY SINGLE (64 32)", "MSGF", IFMT_RXY},
	{0x1D, "DIVIDE SINGLE (64 32)", "DSGF", IFMT_RXY},
	{0x1E, "LOAD REVERSED (32)", "LRV", IFMT_RXY},
	{0x1F, "LOAD REVERSED (16)", "LRVH", IFMT_RXY},
	{0x20, "COMPARE (64)", "CG", IFMT_RXY },
	{0x21, "COMPARE LOGICAL (64)", "CLG", IFMT_RXY},
	{0x24, "STORE (64)", "STG", IFMT_RXY},
	{0x26, "CONVERT TO DECIMAL (32)", "CVDY", IFMT_RXY},
	{0x2E, "CONVERT TO DECIMAL (64)", "CVDG", IFMT_RXY},
	{0x2F, "STORE REVERSED (64)", "STRVG", IFMT_RXY},
	{0x30, "COMPARE (64 32)", "CGF", IFMT_RXY },
	{0x31, "COMPARE LOGICAL (64 32)", "CLGF", IFMT_RXY},
	{0x3E, "STORE REVERSED (32)", "STRV", IFMT_RXY},
	{0x3F, "STORE REVERSED (16)", "STRVH", IFMT_RXY},
	{0x46, "BRANCH ON COUNT (64)", "BCTG", IFMT_RXY},
	{0x50, "STORE (32)", "STY", IFMT_RXY},
	{0x51, "MULTIPLY SINGLE (32)", "MSY", IFMT_RXY},
	{0x54, "AND (32)", "NY", IFMT_RXY},
	{0x55, "COMPARE LOGICAL (32)", "CLY", IFMT_RXY},
	{0x56, "OR (32)", "OY", IFMT_RXY},
	{0x57, "EXCLUSIVE OR (32)", "XY", IFMT_RXY},
	{0x58, "LOAD (32)", "LY", IFMT_RXY},
	{0x59, "COMPARE (32)", "CY", IFMT_RXY},
	{0x5A, "ADD (32)", "AY", IFMT_RXY},
	{0x5B, "SUBTRACT (32)", "SY", IFMT_RXY},
	{0x5E, "ADD LOGICAL (32)", "ALY", IFMT_RXY},
	{0x5F, "SUBTRACT LOGICAL (32)", "SLY", IFMT_RXY},
	{0x70, "STORE HALFWORD", "STHY", IFMT_RXY},
	{0x71, "LOAD ADDRESS", "LAY", IFMT_RXY},
	{0x72, "STORE CHARACTER", "STCY", IFMT_RXY},
	{0x73, "INSERT CHARACTER", "ICY", IFMT_RXY},
	{0x76, "LOAD BYTE (32)", "LB", IFMT_RXY},
	{0x77, "LOAD BYTE (64)", "LGB", IFMT_RXY},
	{0x78, "LOAD HALFWORD (32)", "LHY", IFMT_RXY},
	{0x79, "COMPARE HALFWORD", "CHY", IFMT_RXY},
	{0x7A, "ADD HALFWORD", "AHY", IFMT_RXY},
	{0x7B, "SUBTRACT HALFWORD", "SHY", IFMT_RXY},
	{0x80, "AND (64)", "NG", IFMT_RXY},
	{0x81, "OR (64)", "OG", IFMT_RXY},
	{0x82, "EXCLUSIVE OR (64)", "XG", IFMT_RXY},
	{0x86, "MULTIPLY LOGICAL (128 64)", "MLG", IFMT_RXY},
	{0x87, "DIVIDE LOGICAL (64 128)", "DLG", IFMT_RXY},
	{0x88, "ADD LOGICAL WITH CARRY (64)", "ALCG", IFMT_RXY},
	{0x89, "SUBTRACT LOGICAL WITH BORROW (64)", "SLBG", IFMT_RXY},
	{0x8E, "STORE PAIR TO QUADWORD", "STPQ", IFMT_RXY},
	{0x8F, "LOAD PAIR FROM QUADWORD", "LPQ", IFMT_RXY},
	{0x90, "LOAD LOGICAL CHARACTER (64)", "LLGC", IFMT_RXY},
	{0x91, "LOAD LOGICAL HALFWORD (64)", "LLGH", IFMT_RXY},
	{0x94, "LOAD LOGICAL CHARACTER (32)", "LLC", IFMT_RXY},
	{0x95, "LOAD LOGICAL HALFWORD (32)", "LLH", IFMT_RXY},
	{0x96, "MULTIPLY LOGICAL (64 32)", "ML", IFMT_RXY},
	{0x97, "DIVIDE LOGICAL (32 64)", "DL", IFMT_RXY},
	{0x98, "ADD LOGICAL WITH CARRY (32)", "ALC", IFMT_RXY},
	{0x99, "SUBTRACT LOGICAL WITH BORROW (32)", "SLB", IFMT_RXY},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_e5[] = {
	{0x00, "LOAD ADDRESS SPACE PARAMETERS", "LASP", IFMT_SSE},
	{0x01, "TEST PROTECTION", "TPROT", IFMT_SSE},
	{0x02, "STORE REAL ADDRESS", "STRAG", IFMT_SSE},
	{0x0E, "MOVE WITH SOURCE KEY", "MVCSK", IFMT_SSE},
	{0x0F, "MOVE WITH DESTINATION KEY", "MVCDK", IFMT_SSE},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_eb[] = {
	{0x04, "LOAD MULTIPLE (64)", "LMG", IFMT_RSY1},
	{0x0A, "SHIFT RIGHT SINGLE (64)", "SRAG", IFMT_RSY1},
	{0x0B, "SHIFT LEFT SINGLE (64)", "SLAG", IFMT_RSY1},
	{0x0C, "SHIFT RIGHT SINGLE LOGICAL (64)", "SRLG", IFMT_RSY1},
	{0x0D, "SHIFT LEFT SINGLE LOGICAL (64)", "SLLG", IFMT_RSY1},
	{0x0F, "TRACE (64)", "TRACG", IFMT_RSY1},
	{0x14, "COMPARE AND SWAP (32)", "CSY", IFMT_RSY1},
	{0x1C, "ROTATE LEFT SINGLE LOGICAL (64)", "RLLG", IFMT_RSY1},
	{0x1D, "ROTATE LEFT SINGLE LOGICAL (32)", "RLL", IFMT_RSY1},
	{0x20, "COMPARE LOGICAL CHAR. UNDER MASK (high)", "CLMH", IFMT_RSY2},
	{0x21, "COMPARE LOGICAL CHAR. UNDER MASK (low)", "CLMY", IFMT_RSY2},
	{0x24, "STORE MULTIPLE (64)", "STMG", IFMT_RSY1},
	{0x25, "STORE CONTROL (64)", "STCTG", IFMT_RSY1},
	{0x26, "STORE MULTIPLE HIGH", "STMH", IFMT_RSY1},
	{0x2C, "STORE CHARACTERS UNDER MASK (high)", "STCMH", IFMT_RSY2},
	{0x2D, "STORE CHARACTERS UNDER MASK (low)", "STCMY", IFMT_RSY2},
	{0x2F, "LOAD CONTROL (64)", "LCTLG", IFMT_RSY1},
	{0x30, "COMPARE AND SWAP (64)", "CSG", IFMT_RSY1},
	{0x31, "COMPARE DOUBLE AND SWAP (32)", "CDSY", IFMT_RSY1},
	{0x3E, "COMPARE DOUBLE AND SWAP (64)", "CDSG", IFMT_RSY1},
	{0x44, "BRANCH ON INDEX HIGH (64)", "BXHG", IFMT_RSY1},
	{0x45, "BRANCH ON INDEX LOW OR EQUAL (64)", "BXLEG", IFMT_RSY1},
	{0x51, "TEST UNDER MASK", "TMY", IFMT_SIY},
	{0x52, "MOVE (immediate)", "MVIY", IFMT_SIY},
	{0x54, "AND (immediate)", "NIY", IFMT_SIY},
	{0x55, "COMPARE LOGICAL (immediate)", "CLIY", IFMT_SIY},
	{0x56, "OR (immediate)", "OIY", IFMT_SIY},
	{0x57, "EXCLUSIVE OR (immediate)", "XIY", IFMT_SIY},
	{0x80, "INSERT CHARACTERS UNDER MASK (high)", "ICMH", IFMT_RSY2},
	{0x81, "INSERT CHARACTERS UNDER MASK (low)", "ICMY", IFMT_RSY2},
	{0x8E, "MOVE LONG UNICODE", "MVCLU", IFMT_RSY1},
	{0x8F, "COMPARE LOGICAL LONG UNICODE", "CLCLU", IFMT_RSY1},
	{0x90, "STORE MULTIPLE (32)", "STMY", IFMT_RSY1},
	{0x96, "LOAD MULTIPLE HIGH", "LMH", IFMT_RSY1},
	{0x98, "LOAD MULTIPLE (32)", "LMY", IFMT_RSY1},
	{0x9A, "LOAD ACCESS MULTIPLE", "LAMY", IFMT_RSY1},
	{0x9B, "STORE ACCESS MULTIPLE", "STAMY", IFMT_RSY1},
	{0xC0, "TEST DECIMAL", "TP", IFMT_RSL},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_ec[] = {
	{0x44, "BRANCH RELATIVE ON INDEX HIGH (64)", "BRXHG", IFMT_RIE},
	{0x45, "BRANCH RELATIVE ON INDEX LOW OR EQ. (64)", "BRXLG", IFMT_RIE},
	{0x00, "", "", TBL_INV}
};

static opCode_t opc_ed[] = {
	{0x04, "LOAD LENGTHENED (short to long BFP)", "LDEB", IFMT_RXE},
	{0x05, "LOAD LENGTHENED (long to extended BFP)", "LXDB", IFMT_RXE},
	{0x06, "LOAD LENGTHENED (short to extended BFP)", "LXEB", IFMT_RXE},
	{0x07, "MULTIPLY (long to extended BFP)", "MXDB", IFMT_RXE},
	{0x08, "COMPARE AND SIGNAL (short BFP)", "KEB", IFMT_RXE},
	{0x09, "COMPARE (short BFP)", "CEB", IFMT_RXE},
	{0x0A, "ADD (short BFP)", "AEB", IFMT_RXE},
	{0x0B, "SUBTRACT (short BFP)", "SEB", IFMT_RXE},
	{0x0C, "MULTIPLY (short to long BFP)", "MDEB", IFMT_RXE},
	{0x0D, "DIVIDE (short BFP)", "DEB", IFMT_RXE},
	{0x0E, "MULTIPLY AND ADD (short BFP)", "MAEB", IFMT_RXF},
	{0x0F, "MULTIPLY AND SUBTRACT (short BFP)", "MSEB", IFMT_RXF},
	{0x10, "TEST DATA CLASS (short BFP)", "TCEB", IFMT_RXE},
	{0x11, "TEST DATA CLASS (long BFP)", "TCDB", IFMT_RXE},
	{0x12, "TEST DATA CLASS (extended BFP)", "TCXB", IFMT_RXE},
	{0x14, "SQUARE ROOT (short BFP)", "SQEB", IFMT_RXE},
	{0x15, "SQUARE ROOT (long BFP)", "SQDB", IFMT_RXE},
	{0x17, "MULTIPLY (short BFP)", "MEEB", IFMT_RXE},
	{0x18, "COMPARE AND SIGNAL (long BFP)", "KDB", IFMT_RXE},
	{0x19, "COMPARE (long BFP)", "CDB", IFMT_RXE},
	{0x1A, "ADD (long BFP)", "ADB", IFMT_RXE},
	{0x1B, "SUBTRACT (long BFP)", "SDB", IFMT_RXE},
	{0x1C, "MULTIPLY (long BFP)", "MDB", IFMT_RXE},
	{0x1D, "DIVIDE (long BFP)", "DDB", IFMT_RXE},
	{0x1E, "MULTIPLY AND ADD (long BFP)", "MADB", IFMT_RXF},
	{0x1F, "MULTIPLY AND SUBTRACT (long BFP)", "MSDB", IFMT_RXF},
	{0x24, "LOAD LENGTHENED (short to long HFP)", "LDE", IFMT_RXE},
	{0x25, "LOAD LENGTHENED (long to extended HFP)", "LXD", IFMT_RXE},
	{0x26, "LOAD LENGTHENED (short to extended HFP)", "LXE", IFMT_RXE},
	{0x2E, "MULTIPLY AND ADD (short HFP)", "MAE", IFMT_RXF},
	{0x2F, "MULTIPLY AND SUBTRACT (short HFP)", "MSE", IFMT_RXF},
	{0x34, "SQUARE ROOT (short HFP)", "SQE", IFMT_RXE},
	{0x35, "SQUARE ROOT (long HFP)", "SQD", IFMT_RXE},
	{0x37, "MULTIPLY (short HFP)", "MEE", IFMT_RXE},
	{0x38, "MULTIPLY AND ADD UNNRM. (long to ext. low HFP)", "MAYL", IFMT_RXF},
	{0x39, "MULTIPLY UNNORM. (long to ext. low HFP)", "MYL", IFMT_RXF},
	{0x3A, "MULTIPLY & ADD UNNORMALIZED (long to ext. HFP)", "MAY", IFMT_RXF},
	{0x3B, "MULTIPLY UNNORMALIZED (long to ext. HFP)", "MY", IFMT_RXF},
	{0x3C, "MULTIPLY AND ADD UNNRM. (long to ext. high HFP)", "MAYH", IFMT_RXF},
	{0x3D, "MULTIPLY UNNORM. (long to ext. high HFP)", "MYH", IFMT_RXF},
	{0x3E, "MULTIPLY AND ADD (long HFP)", "MAD", IFMT_RXF},
	{0x3F, "MULTIPLY AND SUBTRACT (long HFP)", "MSD", IFMT_RXF},
	{0x40, "SHIFT SIGNIFICAND LEFT (long DFP)", "SLDT", IFMT_RXF},
	{0x41, "SHIFT SIGNIFICAND RIGHT (long DFP)", "SRDT", IFMT_RXF},
	{0x48, "SHIFT SIGNIFICAND LEFT (extended DFP)", "SLXT", IFMT_RXF},
	{0x49, "SHIFT SIGNIFICAND RIGHT (extended DFP)", "SRXT", IFMT_RXF},
	{0x50, "TEST DATA CLASS (short DFP)", "TDCET", IFMT_RXE},
	{0x51, "TEST DATA GROUP (short DFP)", "TDGET", IFMT_RXE},
	{0x54, "TEST DATA CLASS (long DFP)", "TDCDT", IFMT_RXE},
	{0x55, "TEST DATA GROUP (long DFP)", "TDGDT", IFMT_RXE},
	{0x58, "TEST DATA CLASS (extended DFP)", "TDCXT", IFMT_RXE},
	{0x59, "TEST DATA GROUP (extended DFP)", "TDGXT", IFMT_RXE},
	{0x64, "LOAD (short)", "LEY", IFMT_RXY},
	{0x65, "LOAD (long)", "LDY", IFMT_RXY},
	{0x66, "STORE (short)", "STEY", IFMT_RXY},
	{0x67, "STORE (long)", "STDY", IFMT_RXY},
	{0x00, "", "", TBL_INV}
};

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- findOpCode.    	 			    */
/*                                                                  */
/* Function	- Locate an entry within the opcode table that      */
/*		  matches this code.           		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static opCode_t *
findOpCode(opCode_t opTbl[], uint8_t instr, char mask)
{
	int iOp;

	for (iOp = 0; opTbl[iOp].type != TBL_INV; iOp++) {
		if (opTbl[iOp].opcode == (instr & mask)) {
			break;
		}
	}
	return(&opTbl[iOp]);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dtrace_dis390x.				    */
/*                                                                  */
/* Function	- Disassemble a single s390x instruction. Returns   */
/*		  non-zero for a bad opcode.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
dtrace_dis390x(dis390x_t *x, uint64_t pc, char *buf, size_t buflen)
{
	uchar_t instr[8];
	int	size,
		iOp,
		fmt;
	opCode_t *op;
	
	size = x->d390x_get_bytes(x->d390x_data, &instr[0], 2);
	if (size == -1)
		return (-1);

	switch(opCodes[instr[0]].type) {
	case TBL_INV:
		return(-1);
		break;
	case TBL_01 :
		op = findOpCode(opc_01, instr[1], 0xff);
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_A5 :
		op = findOpCode(opc_a5, instr[1], 0x0f);
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_A7 :
		op = findOpCode(opc_a7, instr[1], 0x0f);
		if (op->type == TBL_INV)
			return(-1);
	case TBL_B2 :
		op = &opc_b2[instr[1]];
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_B3 :
		op = &opc_b3[instr[1]];
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_C0 :
		op = findOpCode(opc_c0, instr[1], 0x0f);
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_C2 :
		op = findOpCode(opc_c2, instr[1], 0x0f);
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_E3 :
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);
		op = findOpCode(opc_e3, instr[5], 0xff);
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_E5 :
		op = findOpCode(opc_e5, instr[1], 0xff);
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_EB :
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);
		op = findOpCode(opc_eb, instr[5], 0xff);
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_EC :
		x->d390x_data = &instr[2];
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);
		op = findOpCode(opc_ec, instr[5], 0xff);
		if (op->type == TBL_INV)
			return(-1);
		break;
	case TBL_ED :
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);
		op = findOpCode(opc_ed, instr[5], 0xff);
		if (op->type == TBL_INV)
			return(-1);
		break;
	default :
		op = &opCodes[instr[0]];
	}

	/*----------------------------------------------------------*/
	/* At this stage we have read in at two bytes for most of   */
	/* the instruction formats. For RSY, RSL, RIE, RXE, RXF, &  */
	/* SIY formats we've read in 6 bytes because the 2nd part of*/
	/* the op code is in bits 44-47.			    */
	/*----------------------------------------------------------*/
	switch(op->type) {
	case IFMT_E : 
		if (buf != NULL) {
			x->d390x_sprintf_func(buf, buflen, "%p\t%s\n", 
					     pc, op->mnemonic);
		} 
		break;
	case IFMT_I : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			I_t *tInstr = (I_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%d\n", 
					     pc, op->mnemonic, tInstr->i1);
		} 
		break;
	case IFMT_RI1 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RI1_t *ri1Instr = (RI1_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,%d\n", 
					     pc, op->mnemonic, ri1Instr->r1,
					     ri1Instr->i2);
		} 
		break;
	case IFMT_RI2 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RI2_t *ri2Instr = (RI2_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t0x%x,%d\n", 
					     pc, op->mnemonic, ri2Instr->m1,
					     ri2Instr->i2);
		} 
		break;
	case IFMT_RIE : 
		if (buf != NULL) {
			RIE_t *rieInstr = (RIE_t *) &instr[0];
			void  *br = (void *) (pc + rieInstr->i2 * 2);

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,%p\n", 
					     pc, op->mnemonic, rieInstr->r1,
					     rieInstr->r3, br);
		} 
		break;
	case IFMT_RIL1 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			RIL1_t *ril1Instr = (RIL1_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%x,%d\n", 
					     pc, op->mnemonic, ril1Instr->r1,
					     ril1Instr->i2);
		} 
		break;
	case IFMT_RIL2 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			RIL2_t *ril2Instr = (RIL2_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%x,%d\n", 
					     pc, op->mnemonic, ril2Instr->m1,
					     ril2Instr->i2);
		} 
		break;
	case IFMT_RR : 
		if (buf != NULL) {
			RR_t *rrInstr = (RR_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d\n", 
					     pc, op->mnemonic, rrInstr->r1,
					     rrInstr->r2);
		} 
		break;
	case IFMT_RRE : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RRE_t *rreInstr = (RRE_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,%%d\n", 
					     pc, op->mnemonic, rreInstr->r1,
					     rreInstr->r2);
		} 
		break;
	case IFMT_RRF1 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RRF1_t *rrf1Instr = (RRF1_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,R%d\n", 
					     pc, op->mnemonic, rrf1Instr->r1,
					     rrf1Instr->r3, rrf1Instr->r2);
		} 
		break;
	case IFMT_RRF2 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RRF2_t *rrf2Instr = (RRF2_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,0x%x\n", 
					     pc, op->mnemonic, rrf2Instr->r1,
					     rrf2Instr->r2, rrf2Instr->m3);
		} 
		break;
	case IFMT_RRF3 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RRF3_t *rrf3Instr = (RRF3_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,R%d,0x%x\n", 
					     pc, op->mnemonic, rrf3Instr->r1,
					     rrf3Instr->r3, rrf3Instr->r2,
					     rrf3Instr->m4);
		} 
		break;
	case IFMT_RRR : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RRR_t *rrrInstr = (RRR_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,R%d,%d\n", 
					     pc, op->mnemonic, rrrInstr->r1,
					     rrrInstr->r2, rrrInstr->r3);
		} 
		break;
	case IFMT_RS1 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RS1_t *rs1Instr = (RS1_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,%u(R%d)\n", 
					     pc, op->mnemonic, rs1Instr->r1,
					     rs1Instr->r3, rs1Instr->d2,
					     rs1Instr->b2);
		} 
		break;
	case IFMT_RS2 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RS2_t *rs2Instr = (RS2_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,0x%x,%u(R%d)\n", 
					     pc, op->mnemonic, rs2Instr->r1,
					     rs2Instr->m3, rs2Instr->d2,
					     rs2Instr->b2);
		} 
		break;
	case IFMT_RSI : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RSI_t *rsiInstr = (RSI_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,%d\n",
					     pc, op->mnemonic, rsiInstr->r1,
					     rsiInstr->r3, rsiInstr->i2);
		} 
		break;
	case IFMT_RSL : 
		if (buf != NULL) {
			RSL_t *rslInstr = (RSL_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%u(%d,R%d)\n",
					     pc, op->mnemonic, rslInstr->d1,
					     rslInstr->l1+1, rslInstr->b1);
		} 
		break;
	case IFMT_RSY1 : 
		if (buf != NULL) {
			RSY1_t *rsy1Instr = (RSY1_t *) &instr[0];
			int   disp = (rsy1Instr->dh << 12) + rsy1Instr->dl;

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,%d(R%d)\n",
					     pc, op->mnemonic, rsy1Instr->r1,
					     rsy1Instr->r3, disp, rsy1Instr->b2);
		} 
		break;
	case IFMT_RSY2 : 
		if (buf != NULL) {
			RSY2_t *rsy2Instr = (RSY2_t *) &instr[0];
			int   disp = (rsy2Instr->dh << 12) + rsy2Instr->dl;

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,0x%x,%d(R%d)\n",
					     pc, op->mnemonic, rsy2Instr->r1,
					     rsy2Instr->m3, disp, rsy2Instr->b2);
		} 
		break;
	case IFMT_RX : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			RX_t *rxInstr = (RX_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,%u(R%d,R%d)\n", 
					     pc, op->mnemonic, rxInstr->r1,
					     rxInstr->d2, rxInstr->x2,
					     rxInstr->b2);
		} 
		break;
	case IFMT_RXE : 
		if (buf != NULL) {
			RXE_t *rxeInstr = (RXE_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,%u(R%d,R%d)\n", 
					     pc, op->mnemonic, rxeInstr->r1,
					     rxeInstr->d2, rxeInstr->x2, rxeInstr->b2);
		} 
		break;
	case IFMT_RXF : 
		if (buf != NULL) {
			RXF_t *rxfInstr = (RXF_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,%u(R%d,R%d)\n", 
					     pc, op->mnemonic, rxfInstr->r1,
					     rxfInstr->r3, rxfInstr->d2, rxfInstr->x2, 
					     rxfInstr->b2);
		} 
		break;
	case IFMT_RXY : 
		if (buf != NULL) {
			RXY_t *rxfInstr = (RXY_t *) &instr[0];
			int   disp = (rxfInstr->dh << 12) + rxfInstr->dl;

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,%d(R%d,R%d)\n", 
					     pc, op->mnemonic, rxfInstr->r1,
					     disp, rxfInstr->x2, rxfInstr->b2);
		} 
		break;
	case IFMT_S : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			S_t *sInstr = (S_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%u(R%d)\n", 
					     pc, op->mnemonic, sInstr->d2,
					     sInstr->b2);
		} 
		break;
	case IFMT_SI : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 2) == -1)
			return (-1);

		if (buf != NULL) {
			SI_t *siInstr = (SI_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%u(R%d),0x%x\n", 
					     pc, op->mnemonic, siInstr->d1,
					     siInstr->b1, siInstr->i2);
		} 
		break;
	case IFMT_SIY : 
		if (buf != NULL) {
			SIY_t *siyInstr = (SIY_t *) &instr[0];
			int   disp = (siyInstr->dh << 12) + siyInstr->dl;

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%d(R%d),0x%x\n", 
					     pc, op->mnemonic, disp,
					     siyInstr->b1, siyInstr->i2);
		} 
		break;
	case IFMT_SS1 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			SS1_t *ss1Instr = (SS1_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%u(%d,R%d),%u(R%d)\n", 
					     pc, op->mnemonic, ss1Instr->d1,
					     ss1Instr->l+1, ss1Instr->b1,
					     ss1Instr->d2, ss1Instr->b2);
		} 
		break;
	case IFMT_SS2 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			SS2_t *ss2Instr = (SS2_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%u(%d,R%d),%u(%d,R%d)\n", 
					     pc, op->mnemonic, ss2Instr->d1,
					     ss2Instr->l1+1, ss2Instr->b1,
					     ss2Instr->d2, ss2Instr->l2+1, 
					     ss2Instr->b2);
		} 
		break;
	case IFMT_SS3 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			SS3_t *ss3Instr = (SS3_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%u(%d,R%d),%u(R%d),0x%x\n", 
					     pc, op->mnemonic, ss3Instr->d1,
					     ss3Instr->l1+1, ss3Instr->b1,
					     ss3Instr->d2, ss3Instr->b2, ss3Instr->i3);
		} 
		break;
	case IFMT_SS4 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			SS4_t *ss4Instr = (SS4_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,%u(R%d),%u(R%d)\n",
					     pc, op->mnemonic, ss4Instr->r1,
					     ss4Instr->r3, ss4Instr->d1,
					     ss4Instr->b1, ss4Instr->d2, ss4Instr->b2);
		} 
		break;
	case IFMT_SS5 : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			SS5_t *ss5Instr = (SS5_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,R%d,%u(R%d),%u(R%d)\n",
					     pc, op->mnemonic, ss5Instr->r1,
					     ss5Instr->r3, ss5Instr->d2,
					     ss5Instr->b2, ss5Instr->d4, ss5Instr->b4);
		} 
		break;
	case IFMT_SSE : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			SSE_t *sseInstr = (SSE_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\t%u(R%d),%u(R%d)\n",
					     pc, op->mnemonic, sseInstr->d1,
					     sseInstr->b1, sseInstr->d2,
					     sseInstr->b2);
		} 
		break;
	case IFMT_SSF : 
		if (x->d390x_get_bytes(x->d390x_data, &instr[2], 4) == -1)
			return (-1);

		if (buf != NULL) {
			SSF_t *ssfInstr = (SSF_t *) &instr[0];

			x->d390x_sprintf_func(buf, buflen, "%p\t%s\tR%d,%u(R%d),%u(R%d)\n",
					     pc, op->mnemonic, ssfInstr->r3,
					     ssfInstr->d1, ssfInstr->b1,
					     ssfInstr->d2, ssfInstr->b2);
		} 
		break;
	default : 
		return (-1);
	}
	return (0);
}

/*========================= End of Function ========================*/
