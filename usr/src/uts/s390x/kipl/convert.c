/*------------------------------------------------------------------*/
/* 								    */
/* Name        - convert.c  					    */
/* 								    */
/* Function    - Convert ihex format executable to TXT style output.*/
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
/* 								    */
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

#define ESDID	0x02C5E2C4
#define TXTID	0x02E3E7E3
#define ENDID	0x02C5D5C4
#define SLCID	0x02E2D3C3
#define LDTID	0x02D3C4E3

#define ID2	0xf0f1f0f5

#define TXTSIZE	65535

#define IX_DATA	'0'
#define IX_NOP	'1'
#define IX_XADR	'2'
#define IX_STRT '3'
#define IX_LINR '4'
#define IX_XSTR '5'

#define RAMDISK	0x2000000	// Start of RAMDISK
#define UNIX	0xd00000	// Start of UNIX

#define MIN(a,b)	((a) < (b) ? (a) : (b))

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

typedef struct _ESD_ {
	int	esd;		// 0x02 || 'ESD'
	char	f_01[6];	// Blanks
	short	dat;		// X'0010'
	char	f_02[2];	// Blanks
	short	id;		// ESD id number
	char	modName[8];	// Name "SOLARIS "
	int 	f_03 : 8;	// X'00'
	int	org : 24;    	// Entry point address
	int 	amode1 : 8;	// AMODE = X'07'
	int	modLen : 24;   	// Length
	char	epName[8];	// Name "SIRIUS  "
	int 	ldid : 8;	// X'01'
	int	ep : 24;    	// Entry point address
	int 	amode2 : 8;	// AMODE = X'07'
	int	epLen : 24;    	// Length
	char	f_04[24];	// Blanks
	char	seq[8];		// Sequence number
} __attribute__ ((packed)) ESD;

typedef struct _TXT_ {
	int	txt;		// 0x02 || 'TXT'
	int 	f_01 : 8;	// Blank
	int	offset : 24;	// Offset
	short	f_02;		// Blanks
	short	len;		// Length
	short	f_03;		// Blanks
	short	esdid;		// ESD ID
	char	data[56];	// Object data
	char	seq[8];		// Sequence number
} __attribute__ ((packed)) TXT;

typedef struct _END_ {
	int	end;		// 0x02 || 'END'
	int 	f_01 : 8;	// Blank
	int	ep : 24;	// Entry address
	char	f_02[6];	// Blanks
	short	esdid;		// ESD ID
	char	f_03[16];	// Blanks
	char    idr;		// '1'
	char	asmId[9];	// Assembler id
	char    f_04;		// Blank
	int 	id2;		// Additional id
	char	asmYY[2];	// Year
	char	asmDDD[3];	// Day
	char	f_05[20];	// Blanks
	char	seq[8];		// Sequence number
} __attribute__ ((packed)) END;

typedef struct _SLC_ {
	int	slc;		// 0x02 || 'SLC'
	char	f_01;		// Blank
	char	org[8];		// Origin
	char	f_02[59];	// Blanks
	char	seq[8];		// Sequence number
} __attribute__ ((packed)) SLC;

typedef struct _LDT_ {
	int	ldt;		// 0x02 || 'LDT'
	char	f_01;		// Blank
	char	epName[8];	// Name of entry point
	char	f_02[59];	// Blanks
	char	seq[8];		// Sequence number
} __attribute__ ((packed)) LDT;

typedef struct _xrec_ {
	char 	ind;		// Indicator
	char	len[2];		// Byte count
	char	addr[4];	// Address / Offset
	char	type[2];	// Record type
	char	data[80];	// Object data
} xrec;

typedef struct _xExAddr_ {	
	char 	ind;		// Indicator
	char	len[2];		// Byte count
	char	offset[4];	// Offset
	char	type[2];	// Record type '02'
	char	usba[4];	// Linear base address
	char	cksum[2];	// Checksum
} xExAddr;

typedef struct _xStart_ {	
	char 	ind;		// Indicator
	char	len[2];		// Byte count
	char	offset[4];	// Offset
	char	type[2];	// Record type '03'
	char	cs[4];		// Code segment
	char	ip[4];		// Instruction Pointer
	char	cksum[2];	// Checksum
} xStart;

typedef struct _xLinear_ {
	char 	ind;		// Indicator
	char	len[2];		// Byte count
	char	offset[4];	// Offset
	char	type[2];	// Record type '04'
	char	ulba[4];	// Linear base address
	char	cksum[2];	// Checksum
} xLinear;

typedef struct _xStartLin_ {	
	char 	ind;		// Indicator
	char	len[2];		// Byte count
	char	offset[6];	// Offset
	char	type[2];	// Record type '05'
	char	eip[6];		// Linear base address
	char	cksum[2];	// Checksum
} xStartLin;

typedef	struct	_loadFn_ {
	char	*rFn;		// Name of RAM disk file
	char	*uFn;		// Name of UNIX image (genunix)
} loadFn;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

void prolog(int, char **, loadFn *);
void data_record(char *);
void start_linear(char *);
void extended_address(char *);
void extended_linear(char *);
void start_address(char *);
static __inline__ unsigned char c2x(char *);
void output_txt(void);
void processBootFile(char *);
void processUNIXFile(char *);
void processRAMDisk(char *);
void * mapFile(char *, int *, size_t *, int);
void writeImage(void *, size_t);
void epilog(void);
void usage(FILE *, int);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static int  sequence = 20;
static int  cursor = 0;
static int  expected = -1;
static int  txtData = 0;
static long lastPointer;
static int  lTxt = 0;

static FILE *oFile = (FILE *) -1,	// File for text output file
	    *rFile = (FILE *) -1,	// File for RAMdisk input file
	    *dFile = (FILE *) -1,	// File for RAMdisk output file
	    *tFile = NULL;		// File to be used for output_txt();

static long long RAMstrAddr = 0,
		 RAMendAddr = 0;

static char 	modName[8] = { 0xE2, 0xD6, 0xD3, 0xC1, 0xD9, 0xC9, 0xE2, 0x40 },
		epName[8]  = { 0xE2, 0xC9, 0xD9, 0xC9, 0xE4, 0xE2, 0x40, 0x40 },
		asmId[9]   = { 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9 },
		asmYY[2]   = { 0xF0, 0xF0 },
		asmDDD[3]  = { 0xF0, 0xF0, 0xF1 };

static ESD esdRec;

static END endRec;

static SLC slcRec;

static LDT ldtRec;

static unsigned char *txtBuffer;

static TXT txtRec;

static char c2xTbl[256] = {
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   0,   0,   0,   0,   0,
  0,  10,  11,  12,  13,  14,  15,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,  10,  11,  12,  13,  14,  15,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- main.                                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
main(int argc, char **argv) 
{
	char 	buffer[512],
	     	*dataIn;
	loadFn	fNames;
	xrec 	*xData;

	prolog(argc, argv, &fNames);

	tFile  = oFile;

	if (rFile != (FILE *) -1)
		processRAMDisk(fNames.rFn);
	processUNIXFile(fNames.uFn);

	epilog();
	return (0);
}			

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prolog.                                           */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
prolog(int argc, char **argv, loadFn *fNames)
{
	char c;

	memset(&esdRec, 0, sizeof(esdRec));
	esdRec.esd      = ESDID;
	esdRec.dat	= 0x20;
	esdRec.id	= 1;
	esdRec.amode1	= 7;
	esdRec.amode2	= 7;
	esdRec.ldid	= 1;
	memcpy(&esdRec.modName, &modName, sizeof(esdRec.modName));
	memcpy(&esdRec.epName, &epName, sizeof(esdRec.epName));
	memset(&esdRec.f_01, 0x40, sizeof(esdRec.f_01));
	memset(&esdRec.f_02, 0x40, sizeof(esdRec.f_02));
	memset(&esdRec.f_04, 0x40, sizeof(esdRec.f_04));

	memset(&endRec, 0, sizeof(endRec));
	endRec.end      = ENDID;
	endRec.esdid	= 1;
	endRec.f_01	= 0x40;
	endRec.idr	= 0xf1;
	endRec.id2	= ID2;
	memset(&endRec.f_02, 0x40, sizeof(endRec.f_02));
	memcpy(&endRec.asmId, &asmId, sizeof(endRec.asmId));
	memcpy(&endRec.asmYY, &asmYY, sizeof(endRec.asmYY));
	memcpy(&endRec.asmDDD, &asmDDD, sizeof(endRec.asmDDD));
	memset(&endRec.f_03, 0x40, sizeof(endRec.f_03));
	endRec.f_04	= 0x40;
	memset(&endRec.f_05, 0x40, sizeof(endRec.f_05));

	slcRec.slc      = SLCID;
	slcRec.f_01	= 0x40;
	memset(&slcRec.f_02, 0x40, sizeof(slcRec.f_02));

	ldtRec.ldt      = LDTID;
	ldtRec.f_01	= 0x40;
	memcpy(&ldtRec.epName, &epName, sizeof(ldtRec.epName));
	memset(&ldtRec.f_02, 0x40, sizeof(ldtRec.f_02));

	txtBuffer	= malloc(TXTSIZE+1);

	for (opterr = 0; optind < argc; optind++) {
		while ((c = getopt(argc, argv, "d:o:r:u:")) != (int)EOF) {
			switch (c) {
			case 'd':
				dFile = fopen(optarg, "w");
				if (dFile == NULL) {
					fprintf(stderr,"Error opening %s - %s\n",
						optarg, strerror(errno));
					exit(1);
				}
				break;
			case 'o':
				oFile = fopen(optarg, "w");
				if (oFile == NULL) {
					fprintf(stderr,"Error opening %s - %s\n",
						optarg, strerror(errno));
					exit(1);
				}
				break;
			case 'r':
				rFile = fopen(optarg, "r");
				if (rFile == NULL) {
					fprintf(stderr,"Error opening %s - %s\n",
						optarg, strerror(errno));
					exit(1);
				}
				fNames->rFn = strdup(optarg);
				break;
			case 'u':
				fNames->uFn = strdup(optarg);
				break;
			default:
				if (optopt == '?')
					return (usage(stdout, 0));
				fprintf(stderr,"Illegal option -- %c\n\n", optopt);
				return (usage(stderr, 1));
			}
		}

		if (oFile == (FILE *) -1) {
			fprintf(stderr,"Missing output file specification\n");
			exit(1);
		}
		if (fNames->uFn == NULL) {
			fprintf(stderr,"Missing UNIX module specification\n");
			exit(1);
		}
		if (dFile == (FILE *) -1) 
			dFile = oFile;
	
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- data_record.                                      */
/*                                                                  */
/* Function	- Process type '00' ihex records.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
data_record(char *buffer) 
{
	int xAddr,
	    iLen,
	    len,
	    pointer;
	xrec *xData = (xrec *) buffer;

	xAddr = c2xTbl[xData->addr[0]] * 4096 +
		c2xTbl[xData->addr[1]] * 256  + 
		c2xTbl[xData->addr[2]] * 16   +
		c2xTbl[xData->addr[3]];

	pointer = cursor + xAddr;
	
	if (expected != -1) {
		if (expected != pointer) {
			output_txt();
			lastPointer = pointer;
		}
	}
	else
		lastPointer = pointer;

	len = c2xTbl[xData->len[0]] * 16 |
	      c2xTbl[xData->len[1]];

	len *= 2;

	for (iLen = 0; iLen < len; ) {
		txtBuffer[lTxt++] = c2x(&xData->data[iLen]);
		if (lTxt > TXTSIZE) {
			output_txt();
			lastPointer = pointer + (iLen / 2) + 1;
		}
		iLen += 2;
	}

	expected = pointer + (len / 2);
}
			
/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static inline unsigned char
c2x(char *xChars) 
{
	unsigned char res;

	res = c2xTbl[xChars[0]] * 16 |
	      c2xTbl[xChars[1]];

	return (res);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
output_txt()
{

	int iTxt,
	    txtOffset,
	    remTxt,
	    lData;
	char *outBuffer;
	char seq[9];

	outBuffer     = (char *) &txtRec;
	txtRec.txt    = TXTID;
	txtRec.f_01   = 0x40;
	txtRec.f_02   = 0x4040;
	txtRec.f_03   = 0x4040;
	txtOffset     = lastPointer;
	remTxt        = lTxt;
	for (iTxt = 0; iTxt < lTxt; ) {
		if (remTxt > 56)
			lData = 56;
		else
			lData = remTxt;

		memset(&txtRec.data, 0x40, sizeof(txtRec.data));
		txtRec.offset = txtOffset;  
		txtRec.len    = lData;
		txtRec.esdid  = 1;
		txtOffset    += lData;
		sequence     += 10;
		sprintf(seq, "%08d", sequence);
		memcpy(txtRec.seq, seq, sizeof(txtRec.seq));

		memcpy(&txtRec.data, &txtBuffer[iTxt], lData);
		fwrite(outBuffer, sizeof(txtRec), 1, tFile);
		iTxt   += lData;
		remTxt -= lData;
	}
	lTxt = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- extended_address.                                 */
/*                                                                  */
/* Function	- Process type '02' ihex records.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
extended_address(char *buffer) 
{
	int iLen;
	xExAddr *xData = (xExAddr *) buffer;

	cursor = 0;
	for (iLen = 0; iLen < sizeof(xData->usba); iLen++) {
		cursor = (cursor * 16) + c2xTbl[xData->usba[iLen]];
	}

	cursor     *= 16;
	fprintf(stderr,"Segment: 0x%x\n",cursor);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- start_address.                                    */
/*                                                                  */
/* Function	- Process type '03' ihex records.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
start_address(char *buffer)
{
	int iEnt,
	    entry;
	xStart *xData = (xStart *) buffer;
	char seq[9];

#if 1
	entry = c2xTbl[xData->cs[0]] * 268435456 +
		c2xTbl[xData->cs[1]] * 16777216	 +
		c2xTbl[xData->cs[2]] * 1048576	 +
		c2xTbl[xData->cs[3]] * 65536     +
	        c2xTbl[xData->ip[0]] * 4096	 + 
		c2xTbl[xData->ip[1]] * 256	 +
		c2xTbl[xData->ip[2]] * 16	 +
		c2xTbl[xData->ip[3]];
#else
	entry = c2xTbl[xData->ip[0]] * 4096	 + 
		c2xTbl[xData->ip[1]] * 256	 +
		c2xTbl[xData->ip[2]] * 16	 +
		c2xTbl[xData->ip[3]];
#endif

	fprintf(stderr,"Start Address: 0x%p\n",entry);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- extended_linear.                                  */
/*                                                                  */
/* Function	- Process type '04' ihex records.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
extended_linear(char *buffer)
{
	int iLen;
	xLinear *xData = (xLinear *) buffer;
	
	cursor = 0;
	for (iLen = 0; iLen < sizeof(xData->ulba); iLen++) {
		cursor = (cursor * 16) + c2xTbl[xData->ulba[iLen]];
	}

	cursor     *= 65536;
	fprintf(stderr,"Segment: 0x%x\n",cursor);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- start_linear.                                     */
/*                                                                  */
/* Function	- Process type '05' ihex records.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
start_linear(char *buffer)
{
	int iLen,
	    entry;
	xStartLin *xData = (xStartLin *) buffer;
	char seq[9];
	
	entry = 0;
	for (iLen = 0; iLen < sizeof(xData->eip); iLen++) {
		entry = (entry * 16) + c2xTbl[xData->eip[iLen]];
	}
	fprintf(stderr,"Start Address: 0x%p\n",entry);

	esdRec.ep = entry;
	endRec.ep = entry;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- processUNIXFile.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void 
processUNIXFile(char *fileName)
{
	int 		bFd,
			iPhdr;
	size_t		bSize;
	void 		*elfBoot,
			*patch;
	long long 	offset[3],
			entry;
	Elf64_Ehdr 	*bHdr;
	Elf64_Phdr 	*pHdr;
	char		org[9];

	tFile = oFile;

	fprintf(stderr,"Processing UNIX image in %s\n",fileName);
	
	elfBoot = mapFile(fileName, &bFd, &bSize, PROT_WRITE + PROT_READ);
	bHdr    = (Elf64_Ehdr *) elfBoot;
	entry   = bHdr->e_entry; 

	fprintf(stderr,"Adding text records at %08d\n"
		"ELF object is %ld bytes long\n"
		"Entry point is at 0x%llx\n",
		sequence,
		bSize,
		entry);

	/*--------------------------------------------------*/
	/* Patch the IPL stub with the address of boot      */
	/*--------------------------------------------------*/
	lTxt	    = sizeof(offset);
	endRec.ep   = bHdr->e_entry;
	esdRec.ep   = bHdr->e_entry;
	if (bHdr->e_phoff == 0) {
		fprintf(stderr,"No program header information found\n");
		exit(3);
	}

	for (iPhdr = 0, pHdr = (Elf64_Phdr *) ((void *) bHdr + bHdr->e_phoff); 
		iPhdr < bHdr->e_phnum; 
		iPhdr++, pHdr++) {
		if (pHdr->p_type == PT_LOAD) {
			sprintf(org,"%08llx",pHdr->p_paddr);
			memcpy(&slcRec.org, org, sizeof(slcRec.org));
			esdRec.org  = pHdr->p_paddr;
			offset[0]   = pHdr->p_paddr;
			lastPointer = pHdr->p_paddr;
			break;
		}
	}

	if (offset[0] == NULL) {
		fprintf(stderr,"No PT_LOAD header found\n");
		exit(3);
	}

	offset[1] = RAMstrAddr;
	offset[2] = RAMendAddr;

	fprintf(stderr,"ELF loadpoint is 0x%llx\n",offset[0]);
	memcpy(txtBuffer, (void *) &offset, lTxt);
	output_txt();

	/*--------------------------------------------------*/
	/* Now write the boot image                         */
	/*--------------------------------------------------*/
	lastPointer   = offset[0];
	esdRec.modLen = bSize;
	esdRec.epLen  = 1;
	writeImage(elfBoot, bSize);

	munmap(elfBoot, bSize);
	close(bFd);

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- writeImage.                                       */
/*                                                                  */
/* Function	- Write text records corresponding to the contents  */
/*		  of the data found at image.  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
writeImage(void *image, size_t size)
{
	size_t	fileSize = 0;
	int	fileCount = 0;

	fprintf(stderr,"Writing image from %p for %d bytes at 0x%p\n",
		image,size,lastPointer);

	/*--------------------------------------------------*/
	/* Work through the ELF object and create TXT recs  */
	/*--------------------------------------------------*/
	while (fileSize < size) {
		int lRec;

		lRec = lTxt = MIN((size - fileSize), TXTSIZE);
		memcpy(txtBuffer, image, lTxt);
		output_txt();
		lastPointer += lRec;
		image       += lRec;
		fileSize    += lRec;
		fileCount++;
	}

	fprintf(stderr, "Wrote %ld bytes for image in %ld records\n",
		fileSize, fileCount);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- processRAMDisk.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
processRAMDisk(char *fileName)
{
	int 	fd = fileno(rFile);
	struct 	stat buf;
	long	fileSize = 0,
		fileCount = 0;
	ssize_t lFile;
	
	tFile = dFile;

	fprintf(stderr,"Processing ramdisk image in %s\n",fileName);
	
	fprintf(stderr,"Adding text records at %d\n",sequence);

	/*--------------------------------------------------*/
	/* Determine size of file and calculate the end adr */
	/*--------------------------------------------------*/
	if (fstat(fd, &buf) == -1) {
		fprintf(stderr,"%s\n",strerror(errno));
		exit(4);
	}

	lastPointer = RAMstrAddr = RAMDISK;
	RAMendAddr  = RAMDISK + buf.st_size;
	fprintf(stderr,"RAM disk extents: %p to %llx\n",
		RAMDISK,RAMendAddr);

	/*--------------------------------------------------*/
	/* Read RAM disk file and put out as TXT records    */
	/*--------------------------------------------------*/
	while ((lFile = lTxt = read(fd, txtBuffer, TXTSIZE)) > 0) {
		output_txt();
		lastPointer += lFile;
		fileSize    += lFile;
		fileCount++;
	}

	fprintf(stderr, "Wrote %ld bytes for file image in %ld records\n",
		fileSize, fileCount);
	
	fclose(rFile);

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mapFile.                                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
mapFile(char *fn, int *fd, size_t *size, int permissions)
{
	struct stat	sbuf;
	void *		elfFile;
	Elf64_Ehdr	*ehdr;

	if ((*fd = open(fn, O_RDONLY)) == -1) {
		fprintf(stderr,"open failed for %s - %s\n",fn,strerror(errno));
		exit(1);
	}

	if (fstat(*fd, &sbuf) == -1) {
		fprintf(stderr,"fstat failed for %s - %s\n",fn,strerror(errno));
		exit(1);
	}

	*size = sbuf.st_size;

	/*
	 * mmap in the whole file to work with it.
	 */
	if ((elfFile = (void *)mmap(NULL, sbuf.st_size, permissions,
			MAP_PRIVATE, *fd, 0)) == MAP_FAILED) {
		fprintf(stderr,"mmap for %s failed - %s\n",fn,strerror(errno));
		exit(1);
	}

	ehdr = (Elf64_Ehdr *)elfFile;

	if (*(int *)(ehdr->e_ident) != *(int *)(ELFMAG)) {
		fprintf(stderr,"%s is not elf file\n",fn);
		exit(5);
	}

	return(elfFile);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- epilog.                                           */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
epilog()
{
	char *outBuffer,
	     seq[9];

	if (lTxt > 0)
		output_txt();
	
	outBuffer  = (char *) &slcRec;
	memcpy(slcRec.seq, "00000010", sizeof(slcRec.seq));
	fwrite(outBuffer, sizeof(slcRec), 1, tFile);

	outBuffer  = (char *) &esdRec;
	memcpy(esdRec.seq, "00000020", sizeof(esdRec.seq));
	fwrite(outBuffer, sizeof(esdRec), 1, tFile);

	outBuffer = (char *) &endRec;
	sequence += 10;
	sprintf(seq, "%08d", sequence);
	memcpy(endRec.seq, seq, sizeof(endRec.seq));
	fwrite(outBuffer, sizeof(endRec), 1, tFile);

	outBuffer = (char *) &ldtRec;
	sequence += 10;
	sprintf(seq, "%08d", sequence);
	memcpy(ldtRec.seq, seq, sizeof(ldtRec.seq));
	fwrite(outBuffer, sizeof(ldtRec), 1, tFile);

	if (dFile != oFile)
		fclose(dFile);
	fclose(oFile);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- usage.                                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
usage(FILE *uFile, int eVal)
{
	fprintf(uFile,"Usage: convert -o <oFile> -r <rFile> "
		"-u <uFile> -d <dFile>\n\n"
		"Where:\n"
		"\toFile - File to contain IPL image\n"
		"\trFile - File containing RAM disk data\n"
		"\tuFile - File containing UNIX kernel\n"
		"\tdFile - File to contain RAM disk image (defaults to oFile)\n");

	exit(eVal);
}

/*========================= End of Function ========================*/
