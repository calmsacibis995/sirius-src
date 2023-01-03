/*------------------------------------------------------------------*/
/* 								    */
/* Name        - machrel.s390.c					    */
/* 								    */
/* Function    - Manage relocation activities for linker.           */
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
/* Common Development and Distribution License (the "License").     */
/* You may not use this file except in compliance with the License. */
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

/* Get the s390x version of the relocation engine */
#define	DO_RELOC_LIBLD_S390 1

#if defined(_ELF64)
# define plt0Template		plt0Template64
# define pltTemplate		pltTemplate64
# define R_390_TLS_LDO		R_390_TLS_LDO64
# define R_390_TLS_LE		R_390_TLS_LE64
# define R_390_TLS_IE		R_390_TLS_IE64
#else
# define plt0Template		plt0Template32
# define plt0TemplateABS	plt0TemplateABS32
# define pltTemplate		pltTemplate32
# define R_390_TLS_LDO		R_390_TLS_LDO32
# define R_390_TLS_LE		R_390_TLS_LE32
# define R_390_TLS_IE		R_390_TLS_IE32
#endif

#ifdef __GNUC__
# define OFFSETOF(a, b, c)	 __builtin_offsetof(a, c)
#else
# define OFFSETOF(a, b, c)	 ((intptr_t) &b->c - (intptr_t) b)
#endif

#define SWORD(a)	(*(Sword *) &a)
#define WORD(a)		(*(Word *) &a)

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <sys/elf_s390.h>
#include <debug.h>
#include <reloc.h>
#include <s390/machdep_s390.h>
#include "msg.h"
#include "_libld.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

#if defined(_ELF64)

typedef struct __plt0 {
	uint8_t	st[6];		/* stg	%r1,56(%r15)		    */
	uint8_t	larl[2];	/* larl	%r1,			    */
	uint8_t	got[4];		/* __GLOBAL_OFFSET_TABLE__	    */
	uint8_t	mvc[6];		/* mvc	48(4,%r15),8(%r1)	    */
	uint8_t	loff[6];	/* lg	%r1,8(%r1)		    */
	uint8_t	br[2];		/* br	%r1			    */
	short	nop[3];		/* Padding			    */
} plt0_t;

typedef struct __plt1 {
	uint8_t	larl[2];	/* larl	%r1,			    */
	uint8_t gotent[4];	/* ep@GOTENT              	    */
	uint8_t	lgot[6];	/* lg	%r1,0(%r1)		    */
	uint8_t	br[2];		/* br	%r1			    */
	uint8_t	basr[2];	/* basr	%r1,0			    */
	uint8_t	loff[6];	/* lgf	%r1,12(%r1)		    */
	uint8_t	jg[2];		/* jg				    */
	uint8_t	plt0[4];	/* offset(plt0)			    */
	uint8_t	gotoff[4];	/* offset(symbol table)		    */
} plt1_t;

#else

typedef struct __plt0_pic {
	uint8_t	st[4];		/* st	%r1,28(%r15)		    */
	uint8_t	larl[2];	/* larl	%r1,			    */
	uint8_t	got[4];		/* __GLOBAL_OFFSET_TABLE__	    */
	uint8_t	mvc[6];		/* mvc	24(4,%r15),8(%r1)	    */
	uint8_t	loff[4];	/* l	%r1,8(%r1)		    */
	uint8_t	br[2];		/* br	%r1			    */
	short	nop[3];		/* Padding			    */
} plt0_pic_t;

typedef struct __plt0_abs {
	uint8_t	st[4];		/* st	%r1,28(%r15)		    */
	uint8_t	basr[2];	/* basr %r1,0			    */
	uint8_t	lgot[4];	/* l	%r1,agot-base(%r1)	    */
	uint8_t	mvc[6];		/* mvc	24(4,%r15),8(%r1)	    */
	uint8_t	loff[4];	/* l	%r1,8(%r1)		    */
	uint8_t	br[2];		/* br	%r1			    */
	short   pad;		/* Padding			    */
	uint8_t	gotaddr[4];	/* AGOT .long got		    */
	short   fill[2];	/* Padding                          */
} plt0_abs_t;

typedef struct __plt1_pic {
	uint8_t	larl[2];	/* larl	%r1,			    */
	uint8_t	gotent[4];	/* ep@GOTENT              	    */
	uint8_t	lgot[4];	/* l	%r1,0(%r1)		    */
	uint8_t	br[2];		/* br	%r1			    */
	uint8_t	basr[2];	/* basr	%r1,0			    */
	uint8_t	loff[4];	/* l	%r1,12(%r1)		    */
	uint8_t	jg[2];		/* jg				    */
	uint8_t	plt0[4];	/* offset(plt0)			    */
	uint8_t	gotoff[4];	/* offset(symbol table)		    */
	uint8_t pad[4];		/* Padding			    */
} plt1_pic_t;

typedef struct __plt1_abs {
	uint8_t	basr[2];	/* basr	%r1,%r0			    */
	uint8_t	lasym[4];	/* l	%r1,22(%r1)		    */
	uint8_t	lrtn[4];	/* l	%r1,0(%r1)		    */
	uint8_t	br[2];		/* br	%r1			    */
	uint8_t	basr2[2];	/* basr	%r1,0			    */
	uint8_t	loff[4];	/* l	%r1,14(%r1)		    */
	uint8_t	jg[2];		/* jg				    */
	uint8_t	plt0[4];	/* offset(plt0)			    */
	uint8_t	gotaddr[4];	/* Address(GOT entry)      	    */
	uint8_t symoff[4];	/* Symtable offset		    */
} plt1_abs_t;

#endif

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

/* Forward declarations */
static Xword ld_calc_got_offset(Rel_desc *, Ofl_desc *);
static Gotndx *ld_find_gotndx(List *, Gotref, Ofl_desc *, Rel_desc *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*
 * Template for generating "void (*)(void)" function
 */
static const uchar_t nullfunc_tmpl[] = {
/* 0x00 */	0x07, 0xfe, 0x07, 0x00		/* br %r14*/
};

#if !defined(_ELF64)

/*  The 32-bit GOT holds the address within the PLT to be executed.  
 *  According to the S390 ELF supplement, the GOT layout is as 
 *  follows:
 *
 *  Word	Description
 *  ----	--------------------------------------------
 *  0		The address of the dynamic table
 *  1		A pointer to a structure describing the object
 *  2		Points to the loader entry address
 *
 *  The code for PLT entries is of the following form:
 *
 *  The loader then gets:
 *  24(15) =  Pointer to the structure describing the object.
 *  28(15) =  Offset in symbol table
 *  The loader  must  then find the module where the function is
 *  and insert the address in the GOT.
 *
 *  Loc	Code				Description
 *  0	PLT1: 	larl r1,<fn>@GOTENT 	// R1 = A(GOT)
 *  6    	l    r1,0(r1)      	// R1 = *(A(GOT))
 *  10    	br   r1        		// Branch to GOT entry address
 *  12	RET1: 	basr r1,0         	// Return point from 1st call to GOT
 *  14    	l    r1,12(r1)     	// R1 = Offset in symbol table 
 *  18    	jg   -x       	 	// Jump to start of PLT
 *  24    	.long ?          	// Symbol table offset
 *  28    	.long 0          	// Filler
 *  32					// PLT = 32 bytes in length
 *
 *  There needs to be a couple of dynamic patches:
 *  Offset 2:  Subject of larl = Relative address to GOT entry
 *  Offset 20: Relative branch to PLT0
 *  Offset 24: 32 bit offset into symbol table (max 2GB for symbol table)

*/

/*------------------------------------------------------------------*/

plt1_pic_t pltTemplate = {
	{0xc0, 0x10},
	{0, 0, 0, 0},
	{0x58, 0x10, 0x10, 0x00},
	{0x07, 0xf1},
	{0x0d, 0x10},
	{0x58, 0x10, 0x10, 0x0c},
	{0xc0, 0xf4},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0x00, 0x00, 0x00, 0x00}
};

/*  The 32-bit GOT holds the address within the PLT to be executed.  
 *  According to the S390 ELF supplement, the GOT layout is as 
 *  follows:
 *
 *  Word	Description
 *  ----	--------------------------------------------
 *  1		A pointer to a structure describing the object
 *  2		Points to the loader entry address
 *
 *  The code for PLT entries is of the following form:
 *
 *  The loader then gets:
 *  24(15) =  Pointer to the structure describing the object.
 *  28(15) =  Offset in symbol table
 *  The loader  must  then find the module where the function is
 *  and insert the address in the GOT.
 *
 *  Loc	Code				Description
 *  0	PLT1: 	basr r1,r0          	// Establish addressability
 *  2    	l    r1,22(r1)      	// R1 = *(A(GOT))
 *  6    	l    r1,0(r1)      	// R1 = Function
 *  10    	br   r1        		// Branch to GOT entry address
 *  12	RET1: 	basr r1,0         	// Return point from 1st call to GOT
 *  14    	l    r1,14(r1)     	// R1 = Offset in symbol table 
 *  18    	j    -x       	 	// Jump to start of PLT
 *  22          nop			// Filler
 *  24    	.long 0          	// A(GOT entry)
 *  28    	.long ?          	// Symbol table offset
 *  32					// PLT = 32 bytes in length
 *
 *  There needs to be a couple of dynamic patches:
 *  Offset 20: Relative branch to PLT0
 *  Offset 24: 32 bit address of GOT entry
 *  Offset 28: 32 bit offset into symbol table (max 2GB for symbol table)

*/

/*------------------------------------------------------------------*/

plt1_abs_t pltTemplateABS = {
	{0x0d, 0x10},
	{0x58, 0x10, 0x10, 0x16},
	{0x58, 0x10, 0x10, 0x00},
	{0x07, 0xf1},
	{0x0d, 0x10},
	{0x58, 0x10, 0x10, 0x0e},
	{0xc0, 0xf4},
	{0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

/* The first PLT entry is special it is responsible for 
 * putting the offset into the symbol table from R1 onto 
 * the stack at 28(15) and into the loader object information
 * at 24(15). It loads the loader address in R1 and branches to it.  
 */

/* The first entry in the PLT for dynamic objects looks like:
 *
 * Offset	Code				
 * 0	PLT0:	st   %r1,28(%r15)  
 * 4  		larl %r1,_GLOBAL_OFFSET_TABLE
 * 10 		mvc  24(4,%r15),4(%r1) 
 * 16 		l    %r1,8(%r1)   
 * 20 		br   %r1     
 * 22		nop
 * 24		nop
 * 26		nop
 * 28		nop
 * 30		nop
 * 32
 *
 * The code needs to be patched as follows:
 * Offset 8:  Relative address to start of GOT
 */

plt0_pic_t plt0Template = {
	{0x50, 0x10, 0xf0, 0x1c},
	{0xc0, 0x10},
	{0, 0, 0, 0},
	{0xd2, 0x03, 0xf0, 0x18, 0x10, 0x04},
	{0x58, 0x10, 0x10, 0x08},
	{0x07, 0xf1},
	{0x0700, 0x0700, 0x0700}
};

/* The first entry in the PLT for static objects looks like:
 *
 * Offset	Code				
 * 0	PLT0:	st   %r1,28(%r15)  
 * 4  		basr %r1,0
 * 6 		l    %r1,agot-base(%r1)
 * 10		mvc  24(4,%r15),8(%r1)
 * 16		l    %r1,8(%r1)
 * 20 		br   %r1     
 * 22		nop
 * 24		nop
 * 26		nop
 * 28	AGOT:	.long	got
 *
 * The code needs to be patched as follows:
 * Offset 28:  Address of GOT
 */

plt0_abs_t plt0TemplateABS = {
	{0x50, 0x10, 0xf0, 0x1c},
	{0x0d, 0x10},
	{0x58, 0x10, 0x10, 0x12},
	{0xd2, 0x03, 0xf0, 0x18, 0x10, 0x04},
	{0x58, 0x10, 0x10, 0x08},
	{0x07, 0xf1},
	0x0700,
	{0, 0, 0, 0},
	{0x0700, 0x0700}
};

#else

/*  The 64-bit GOT holds the address within the PLT to be executed.  
 *  According to the S390X ELF supplement, the GOT layout is as 
 *  follows:
 *
 *  Word	Description
 *  ----	--------------------------------------------
 *  0		The address of the dynamic table
 *  1		A pointer to a structure describing the object
 *  2		Points to the loader entry address
 *
 *  The code for PLT entries is of the following form:
 *
 *  The loader then gets:
 *  48(15) =  Pointer to the structure describing the object.
 *  56(15) =  Offset in symbol table
 *  The loader  must  then find the module where the function is
 *  and insert the address in the GOT.
 *
 *  Loc	Code				Description
 *  0	PLT1: 	larl r1,<fn>@GOTENT 	// R1 = A(GOT)
 *  6    	lg   r1,0(r1)      	// R1 = *(A(GOT))
 *  10    	br   r1        		// Branch to GOT entry address
 *  12	RET1: 	basr r1,0         	// Return point from 1st call to GOT
 *  14    	lgf  r1,12(r1)     	// R1 = Offset in symbol table 
 *  18    	jg   -x       	 	// Jump to start of PLT
 *  24    	.long ?          	// Symbol table offset
 *  28    	.long 0          	// Filler
 *  32					// PLT = 32 bytes in length
 *
 *  There needs to be a couple of dynamic patches:
 *  Offset 2:  Subject of larl = Relative address to GOT entry
 *  Offset 20: Relative branch to PLT0
 *  Offset 24: 32 bit offset into symbol table (max 2GB for symbol table)

*/

/*------------------------------------------------------------------*/

plt1_t pltTemplate = {
	{0xc0, 0x10},
	{0, 0, 0, 0},
	{0xe3, 0x10, 0x10, 0x00, 0x00, 0x04},
	{0x07, 0xf1},
	{0x0d, 0x10},
	{0xe3, 0x10, 0x10, 0x0c, 0x00, 0x14},
	{0xc0, 0xf4},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

/* The first PLT entry is special it is responsible for 
 * putting the offset into the symbol table from R1 onto 
 * the stack at 56(15) and into the loader object information
 * at 48(15). It loads the loader address in R1 and branches to it.  
 */

/* The first entry in the PLT for dynamic objects looks like:
 *
 * Offset	Code				
 * 0	PLT0:	stg  %r1,56(%r15)  
 * 6  		larl %r1,_GLOBAL_OFFSET_TABLE
 * 12 		mvc  48(8,%r15),8(%r1) 
 * 18 		lg   %r1,16(%r1)   
 * 24 		br   %r1     
 * 26		nop
 * 28		nop
 * 30		nop
 * 32
 *
 * The code needs to be patched as follows:
 * Offset 8:  Relative address to start of GOT
 */

plt0_t plt0Template = {
	{0xe3, 0x10, 0xf0, 0x38, 0x00, 0x24},
	{0xc0, 0x10},
	{0, 0, 0, 0},
	{0xd2, 0x07, 0xf0, 0x30, 0x10, 0x08},
	{0xe3, 0x10, 0x10, 0x10, 0x00, 0x04},
	{0x07, 0xf1},
	{0x0700, 0x0700, 0x0700}
};

#endif

#if defined(_ELF64)

/*
 *	TLS Fix ups...
 *			lg	%r2,0(%r2,%r12)
 *			brcl	0,.
 *			brcl	0,.
 *			sllg	%r9,%r8,0
 */
static uint8_t TLS_GD_IE[6] = {0xe3, 0x22, 0xc0, 0x00, 0x00, 0x04};
static uint8_t TLS_GD_LE[6] = {0xc0, 0x04, 0x00, 0x00, 0x00, 0x00};
static uint8_t TLS_LD_LE[6] = {0xc0, 0x04, 0x00, 0x00, 0x00, 0x00};
static uint8_t TLS_IE_LE[6] = {0xeb, 0x98, 0x00, 0x00, 0x00, 0x0d};

#else

/*
 *	TLS Fix ups...
 *			l	%r2,0(%r2,%r12)
 *			bc	0,0
 *			bc	0,0
 *			lr	%r9,%r8; bcr	0,%r0
 */
static uint8_t TLS_GD_IE[4] = {0x58, 0x22, 0xc0, 0x00};
static uint8_t TLS_GD_LE[4] = {0x47, 0x00, 0x00, 0x00};
static uint8_t TLS_LD_LE[4] = {0x47, 0x00, 0x00, 0x00};
static uint8_t TLS_IE_LE[4] = {0x18, 0x98, 0x07, 0x00};

#endif

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_init_rel.                                      */
/*                                                                  */
/* Function	- Initialize relocation regime.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static Word
ld_init_rel(Rel_desc *reld, void *reloc)
{
	Rela *	rela = (Rela *)reloc;

	/* LINTED */
	reld->rel_rtype    = (Word)ELF_R_TYPE(rela->r_info, M_MACH);
	reld->rel_roffset  = rela->r_offset;
	reld->rel_raddend  = rela->r_addend;
	reld->rel_typedata = (Word)ELF_R_TYPE_DATA(rela->r_info);

	reld->rel_flags   |= FLG_REL_RELA;

	return ((Word)ELF_R_SYM(rela->r_info));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_mach_eflags.                                   */
/*                                                                  */
/* Function	- Set the machine specific elf flags.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ld_mach_eflags(Ehdr *ehdr, Ofl_desc *ofl)
{
	ofl->ofl_dehdr->e_flags |= ehdr->e_flags;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_mach_make_dynamic.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ld_mach_make_dynamic(Ofl_desc *ofl, size_t *cnt)
{
	if (!(ofl->ofl_flags & FLG_OF_RELOBJ)) {
		/*
		 * Create this entry if we are going to create a PLT table.
		 */
		if (ofl->ofl_pltcnt)
			(*cnt)++;		/* DT_PLTGOT */
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_mach_update_odynamic.                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ld_mach_update_odynamic(Ofl_desc *ofl, Dyn **dyn)
{
	if (((ofl->ofl_flags & FLG_OF_RELOBJ) == 0) && ofl->ofl_pltcnt) {
		(*dyn)->d_tag = DT_PLTGOT;
		if (ofl->ofl_osplt)
			(*dyn)->d_un.d_ptr = ofl->ofl_osgot->os_shdr->sh_addr;
		else
			(*dyn)->d_un.d_ptr = 0;
		(*dyn)++;
	}
}

/*========================= End of Function ========================*/

#if defined(_ELF64)

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_calc_plt_addr.                                 */
/*                                                                  */
/* Function	- Calculate PLT address.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static Xword
ld_calc_plt_addr(Sym_desc *sdp, Ofl_desc *ofl)
{
	Xword	value;

	value = (Xword)(ofl->ofl_osplt->os_shdr->sh_addr) +
	    M_PLT_RESERVSZ + ((sdp->sd_aux->sa_PLTndx - 1) * M_PLT_ENTSIZE);
	return (value);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plt_entry.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
plt_entry(Ofl_desc *ofl, Word rel_off, Sym_desc *sdp)
{
	plt1_t	*pltent;	/* PLT entry being created */
	Sxword	pltoff,		/* Offset of this entry from PLT top */
		gotoff,		/* Offset of the got entry */
		*gotent,	/* GOT entry being created */
		pltgot;		/* Offset from PLT entry to GOT */
	int	bswap = (ofl->ofl_flags1 & FLG_OF1_ENCDIFF) != 0;

	gotoff = sdp->sd_aux->sa_PLTGOTndx * M_GOT_ENTSIZE;
	pltoff = M_PLT_RESERVSZ + ((sdp->sd_aux->sa_PLTndx - 1) * M_PLT_ENTSIZE);
	pltent = (plt1_t *) ((intptr_t) ofl->ofl_osplt->os_outdata->d_buf + pltoff);
	gotent = (Sxword *)((intptr_t) ofl->ofl_osgot->os_outdata->d_buf + gotoff);

	/* 
	 * Fill in the GOT entry with the address of the next instruction.
	 */
	*gotent = ofl->ofl_osplt->os_shdr->sh_addr + pltoff + 
		  OFFSETOF(plt1_t, pltent, basr);

	(void) memcpy(pltent, &pltTemplate, M_PLT_ENTSIZE);
	pltgot = (((Sxword) ofl->ofl_osgot->os_shdr->sh_addr + gotoff) - 
		  ((Sxword) ofl->ofl_osplt->os_shdr->sh_addr + pltoff +
		   OFFSETOF(plt1_t, pltent, larl))) >> 1;
	SWORD(pltent->gotent) = (Sword) pltgot;
	SWORD(pltent->plt0)   = (Sword) -(pltoff + OFFSETOF(plt1_t, pltent, jg)) >> 1;
	SWORD(pltent->gotoff) = rel_off;
	if (bswap) {
		WORD(pltent->gotent) = ld_bswap_Word((Word) pltent->gotent);
		WORD(pltent->plt0)   = ld_bswap_Word((Word) pltent->plt0);
		WORD(pltent->gotoff) = ld_bswap_Word((Word) pltent->gotoff);
		*gotent = ld_bswap_Xword(*gotent);
	}
}

/*========================= End of Function ========================*/

#else  /* Elf 32 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_calc_plt_addr.                                 */
/*                                                                  */
/* Function	- Calculate PLT address.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static Xword
ld_calc_plt_addr(Sym_desc *sdp, Ofl_desc *ofl)
{
	Xword	value;

	value = (Xword)(ofl->ofl_osplt->os_shdr->sh_addr) +
	    M_PLT_RESERVSZ + ((sdp->sd_aux->sa_PLTndx - 1) * M_PLT_ENTSIZE);

	return (value);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plt_entry.                                        */
/*                                                                  */
/* Function	- Build a single PLT entry.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
plt_entry(Ofl_desc *ofl, Word rel_off, Sym_desc *sdp)
{
	Sword	pltoff,		/* Offset of this entry from PLT top */
		gotoff,		/* Offset of the got entry */
		*gotent;	/* GOT entry being created */
	int	bswap = (ofl->ofl_flags1 & FLG_OF1_ENCDIFF) != 0;
	ofl_flag_t flags = ofl->ofl_flags;

	gotoff = sdp->sd_aux->sa_PLTGOTndx * M_GOT_ENTSIZE;
	pltoff = M_PLT_RESERVSZ + ((sdp->sd_aux->sa_PLTndx - 1) * M_PLT_ENTSIZE);
	gotent = (Sword *)((intptr_t) ofl->ofl_osgot->os_outdata->d_buf + gotoff);

	if (!(flags & FLG_OF_SHAROBJ)) {
		plt1_abs_t *pltent;

		pltent  = (plt1_abs_t *) ((intptr_t) ofl->ofl_osplt->os_outdata->d_buf + pltoff);
		(void) memcpy(pltent, &pltTemplateABS, M_PLT_ENTSIZE);
		*gotent = ofl->ofl_osplt->os_shdr->sh_addr + pltoff + 
			  OFFSETOF(plt1_abs_t, pltent, basr2);
		SWORD(pltent->plt0)    = (Sword) -(pltoff + OFFSETOF(plt1_abs_t, pltent, jg)) >> 1;
		SWORD(pltent->gotaddr) = ofl->ofl_osgot->os_shdr->sh_addr + gotoff;
		SWORD(pltent->symoff)  = rel_off;
		if (bswap) {
			WORD(pltent->plt0)    = ld_bswap_Word((Word) pltent->plt0);
			WORD(pltent->gotaddr) = ld_bswap_Word((Word) pltent->gotaddr);
			WORD(pltent->symoff)  = ld_bswap_Word((Word) pltent->symoff);
		}
	} else {
		plt1_pic_t *pltent;

		pltent = (plt1_pic_t *) ((intptr_t) ofl->ofl_osplt->os_outdata->d_buf + pltoff);
		(void) memcpy(pltent, &pltTemplate, M_PLT_ENTSIZE);
		*gotent = ofl->ofl_osplt->os_shdr->sh_addr + pltoff + 
			  OFFSETOF(plt1_pic_t, pltent, basr);

		SWORD(pltent->gotent) = (((Sword) ofl->ofl_osgot->os_shdr->sh_addr + gotoff) - 
					  ((Sword) ofl->ofl_osplt->os_shdr->sh_addr + pltoff +
					  OFFSETOF(plt1_pic_t, pltent, larl))) >> 1;
		SWORD(pltent->plt0)   = (Sword) -(pltoff + OFFSETOF(plt1_pic_t, pltent, jg)) >> 1;
		SWORD(pltent->gotoff) = rel_off;
		if (bswap) {
			WORD(pltent->gotent) = ld_bswap_Word((Word) pltent->gotent);
			WORD(pltent->plt0)   = ld_bswap_Word((Word) pltent->plt0);
			WORD(pltent->gotoff) = ld_bswap_Word((Word) pltent->gotoff);
			*gotent = ld_bswap_Word(*gotent);
		}
	}
}

/*========================= End of Function ========================*/

#endif /* _ELF64 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_perform_outreloc.                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uintptr_t
ld_perform_outreloc(Rel_desc * orsp, Ofl_desc * ofl)
{
	Os_desc *	relosp, * osp = 0;
	Xword		ndx, roffset, value;
	Sxword		raddend;
	const Rel_entry	*rep;
	Rela		rea;
	char		*relbits;
	Sym_desc *	sdp, * psym = (Sym_desc *)0;
	int		sectmoved = 0;
	Word		dtflags1 = ofl->ofl_dtflags_1;
	ofl_flag_t	flags = ofl->ofl_flags;

	raddend = orsp->rel_raddend;
	sdp = orsp->rel_sym;

	/*
	 * If the section this relocation is against has been discarded
	 * (-zignore), then also discard (skip) the relocation itself.
	 */
	if (orsp->rel_isdesc && ((orsp->rel_flags &
	    (FLG_REL_GOT | FLG_REL_BSS | FLG_REL_PLT | FLG_REL_NOINFO)) == 0) &&
	    (orsp->rel_isdesc->is_flags & FLG_IS_DISCARD)) {
		DBG_CALL(Dbg_reloc_discard(ofl->ofl_lml, M_MACH, orsp));
		return (1);
	}

	/*
	 * If this is a relocation against a move table, or expanded move
	 * table, adjust the relocation entries.
	 */
	if (orsp->rel_move)
		ld_adj_movereloc(ofl, orsp);

	/*
	 * If this is a relocation against a section then we need to adjust the
	 * raddend field to compensate for the new position of the input section
	 * within the new output section.
	 */
	if (ELF_ST_TYPE(sdp->sd_sym->st_info) == STT_SECTION) {
		if (ofl->ofl_parsym.head &&
		    (sdp->sd_isc->is_flags & FLG_IS_RELUPD) &&
		    (psym = ld_am_I_partial(orsp, orsp->rel_raddend))) {
			/*
			 * If the symbol is moved, adjust the value
			 */
			DBG_CALL(Dbg_move_outsctadj(ofl->ofl_lml, psym));
			sectmoved = 1;
			if (ofl->ofl_flags & FLG_OF_RELOBJ)
				raddend = psym->sd_sym->st_value;
			else
				raddend = psym->sd_sym->st_value -
				    psym->sd_isc->is_osdesc->os_shdr->sh_addr;
			/* LINTED */
			raddend += (Off)_elf_getxoff(psym->sd_isc->is_indata);
			if (psym->sd_isc->is_shdr->sh_flags & SHF_ALLOC)
				raddend +=
				    psym->sd_isc->is_osdesc->os_shdr->sh_addr;
		} else {
			/* LINTED */
			raddend += (Off)_elf_getxoff(sdp->sd_isc->is_indata);
			if (sdp->sd_isc->is_shdr->sh_flags & SHF_ALLOC)
				raddend +=
				    sdp->sd_isc->is_osdesc->os_shdr->sh_addr;
		}
	}

	value = sdp->sd_sym->st_value;

	if (orsp->rel_flags & FLG_REL_GOT) {
		osp = ofl->ofl_osgot;
		roffset = ld_calc_got_offset(orsp, ofl);

	} else if (orsp->rel_flags & FLG_REL_PLT) {
		/*
		 * Note that relocations for PLT's actually
		 * cause a relocation against the GOT.
		 */
		osp = ofl->ofl_osplt;
		roffset = (Word) (ofl->ofl_osgot->os_shdr->sh_addr) +
		    sdp->sd_aux->sa_PLTGOTndx * M_GOT_ENTSIZE;

		plt_entry(ofl, osp->os_relosdesc->os_szoutrels, sdp);

#if 0
	} else if (orsp->rel_flags & FLG_REL_PLTOFF) {
		/*
		 * We need to create a GOT entry and a PLT entry
		 */
		osp = ofl->ofl_osgot;
		roffset = ld_calc_got_offset(orsp, ofl);

		osp = ofl->ofl_osplt;
		roffset = (Word) (ofl->ofl_osgot->os_shdr->sh_addr) +
		    sdp->sd_aux->sa_PLTGOTndx * M_GOT_ENTSIZE;

		plt_entry(ofl, osp->os_relosdesc->os_szoutrels, sdp);

#endif
	} else if (orsp->rel_flags & FLG_REL_BSS) {
		/*
		 * This must be a R_390_COPY.  For these set the roffset to
		 * point to the new symbols location.
		 */
		osp = ofl->ofl_isbss->is_osdesc;
		roffset = (Xword)value;

		/*
		 * The raddend doesn't mean anything in an R_390_COPY
		 * relocation.  Null it out because it can confuse people.
		 */
		raddend = 0;
	} else if (orsp->rel_flags & FLG_REL_REG) {
		/*
		 * The offsets of relocations against register symbols
		 * identifiy the register directly - so the offset
		 * does not need to be adjusted.
		 */
		roffset = orsp->rel_roffset;
	} else {
		osp = orsp->rel_osdesc;

		/*
		 * Calculate virtual offset of reference point; equals offset
		 * into section + vaddr of section for loadable sections, or
		 * offset plus section displacement for nonloadable sections.
		 */
		roffset = orsp->rel_roffset +
		    (Off)_elf_getxoff(orsp->rel_isdesc->is_indata);
		if (!(ofl->ofl_flags & FLG_OF_RELOBJ))
			roffset += orsp->rel_isdesc->is_osdesc->
			    os_shdr->sh_addr;
	}

	if ((osp == 0) || ((relosp = osp->os_relosdesc) == 0))
		relosp = ofl->ofl_osrel;

	/*
	 * Verify that the output relocations offset meets the
	 * alignment requirements of the relocation being processed.
	 */
	rep = &reloc_table[orsp->rel_rtype];
	if (((flags & FLG_OF_RELOBJ) || !(dtflags1 & DF_1_NORELOC)) &&
	    !(rep->re_flags & FLG_RE_UNALIGN)) {
		if (((rep->re_fsize == 2) && (roffset & 0x1)) ||
		    ((rep->re_fsize == 4) && (roffset & 0x3)) ||
		    ((rep->re_fsize == 8) && (roffset & 0x7))) {
			Conv_inv_buf_t inv_buf;

			eprintf(ofl->ofl_lml, ERR_FATAL,
			    MSG_INTL(MSG_REL_NONALIGN),
			    conv_reloc_s390_type(orsp->rel_rtype, 0, &inv_buf),
			    orsp->rel_isdesc->is_file->ifl_name,
			    demangle(orsp->rel_sname), EC_XWORD(roffset));
			return (S_ERROR);
		}
	}

	/*
	 * Assign the symbols index for the output relocation.  If the
	 * relocation refers to a SECTION symbol then it's index is based upon
	 * the output sections symbols index.  Otherwise the index can be
	 * derived from the symbols index itself.
	 */
	if (orsp->rel_rtype == R_390_RELATIVE)
		ndx = STN_UNDEF;
	else if ((orsp->rel_flags & FLG_REL_SCNNDX) ||
	    (ELF_ST_TYPE(sdp->sd_sym->st_info) == STT_SECTION)) {
		if (sectmoved == 0) {
			/*
			 * Check for a null input section. This can
			 * occur if this relocation references a symbol
			 * generated by sym_add_sym().
			 */
			if ((sdp->sd_isc != 0) &&
			    (sdp->sd_isc->is_osdesc != 0))
				ndx = sdp->sd_isc->is_osdesc->os_scnsymndx;
			else
				ndx = sdp->sd_shndx;
		} else
			ndx = ofl->ofl_sunwdata1ndx;
	} else
		ndx = sdp->sd_symndx;

	/*
	 * Add the symbols 'value' to the addend field.
	 */
	if (orsp->rel_flags & FLG_REL_ADVAL)
		raddend += value;

	/*
	 * The addend field for R_390_TLS_DTPMOD32 and R_390_TLS_DTPMOD64
	 * mean nothing.  The addend is propagated in the corresponding
	 * R_390_TLS_DTPOFF* relocations.
	 */
	if (orsp->rel_rtype == M_R_DTPMOD)
		raddend = 0;

	relbits = (char *)relosp->os_outdata->d_buf;

	rea.r_info = ELF_R_INFO(ndx, ELF_R_TYPE_INFO(orsp->rel_typedata,
				orsp->rel_rtype));
	rea.r_offset = roffset;
	rea.r_addend = raddend;
	DBG_CALL(Dbg_reloc_out(ofl, ELF_DBG_LD, SHT_RELA, &rea, relosp->os_name,
	    orsp->rel_sname));

	/*
	 * Assert we haven't walked off the end of our relocation table.
	 */
	assert(relosp->os_szoutrels <= relosp->os_shdr->sh_size);

	(void) memcpy((relbits + relosp->os_szoutrels),
	    (char *)&rea, sizeof (Rela));
	relosp->os_szoutrels += (Xword)sizeof (Rela);

	/*
	 * Determine if this relocation is against a non-writable, allocatable
	 * section.  If so we may need to provide a text relocation diagnostic.
	 */
	ld_reloc_remain_entry(orsp, osp, ofl);
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- tls_fixups.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static Fixupret
tls_fixups(Ofl_desc *ofl, Rel_desc *arsp)
{
	Sym_desc	*sdp = arsp->rel_sym;
	Word		rtype = arsp->rel_rtype;
	Word		*offset, w;
	int		bswap = OFL_SWAP_RELOC_DATA(ofl, arsp);


	offset = (Word *)((uintptr_t)arsp->rel_roffset +
	    (uintptr_t)_elf_getxoff(arsp->rel_isdesc->is_indata) +
	    (uintptr_t)arsp->rel_osdesc->os_outdata->d_buf);

	if (sdp->sd_ref == REF_DYN_NEED) {
		/*
		 * IE reference model
		 */
		switch (rtype) {
		case R_390_TLS_GD64:
			/* 
			 * Transition:
			 *	0x00	ear	%r7,%a0
			 *	0x04	sllg	%r7,%r7,32
			 *	0x0a	ear	%r7,%a1
			 *	0x0e	lg	%r2,.L1-.L0(%r13)
			 *	0x14	brasl	%r14,__tls_get_offset@plt
			 * To:
			 *	0x00	ear	%r7,%a0
			 *	0x04	sllg	%r7,%r7,32
			 *	0x0a	ear	%r7,%a1
			 *	0x0e	lg	%r2,.L1-.L0(%r13)
			 *	0x14	lg	%r2,0(%r2,%r12)
			 */
			DBG_CALL(Dbg_reloc_transition(ofl->ofl_lml, M_MACH,
			    R_390_TLS_GOTIE64, arsp));
			arsp->rel_rtype = R_390_TLS_GOTIE64;
			arsp->rel_roffset += 5;

			/*
			 * Adjust 'offset' to beginning of instruction sequence
			 */
			(void) memcpy(offset, TLS_GD_IE, sizeof(TLS_GD_IE));
			return (FIX_RELOC);

		}
		return (FIX_RELOC);
	}

	/*
	 * LE reference model
	 */
	switch (rtype) {
	case R_390_TLS_GD32:
		/* 
		 * Transition:
		 *	0x00	ear	%r7,%a0
		 *	0x04	sllg	%r7,%r7,32
		 *	0x0a	ear	%r7,%a1
		 *	0x0e	lg	%r2,.L1-.L0(%r13)
		 *	0x14	brasl	%r14,__tls_get_offset@plt
		 * To:
		 *	0x00	ear	%r7,%a0
		 *	0x04	sllg	%r7,%r7,32
		 *	0x0a	ear	%r7,%a1
		 *	0x0e	lg	%r2,.L1-.L0(%r13)
		 *	0x14	brcl	0,.
		 */
		DBG_CALL(Dbg_reloc_transition(ofl->ofl_lml, M_MACH,
		    R_390_TLS_LE64, arsp));

		arsp->rel_rtype = R_390_TLS_LE64;
		arsp->rel_roffset += 4;

		/*
		 * Adjust 'offset' to beginning of instruction sequence.
		 */
		offset -= 3;
		(void) memcpy(offset, TLS_GD_LE, sizeof (TLS_GD_LE));
		return (FIX_RELOC);

	case R_390_TLS_LDM64:
		/* 
		 * Transition:
		 *	0x00	ear	%r7,%a0
		 *	0x04	sllg	%r7,%r7,32
		 *	0x0a	ear	%r7,%a1
		 *	0x0e	lg	%r2,.L1-.L0(%r13)
		 *	0x14	brasl	%r14,__tls_get_offset@plt
		 * To:
		 *	0x00	ear	%r7,%a0
		 *	0x04	sllg	%r7,%r7,32
		 *	0x0a	ear	%r7,%a1
		 *	0x0e	lg	%r2,.L1-.L0(%r13)
		 *	0x14	brcl	0,.
		 */
		DBG_CALL(Dbg_reloc_transition(ofl->ofl_lml, M_MACH,
		    R_390_TLS_LE64, arsp));
		arsp->rel_rtype = R_390_TLS_LE64;

		/*
		 * Adjust 'offset' to beginning of instruction sequence.
		 */
		offset -= 3;
		(void) memcpy(offset, TLS_LD_LE, sizeof (TLS_LD_LE));
		return (FIX_RELOC);

	case R_390_TLS_GOTIE64:
		/* 
		 * Transition:
		 *	0x00	ear	%r7,%a0
		 *	0x04	sllg	%r7,%r7,32
		 *	0x0a	ear	%r7,%a1
		 *	0x0e	lg	%r8,.L1-.L0(%r13)
		 *	0x14	lg	%r9,0(%r8,%r12)
		 * To:
		 *	0x00	ear	%r7,%a0
		 *	0x04	sllg	%r7,%r7,32
		 *	0x0a	ear	%r7,%a1
		 *	0x0e	lg	%r8,.L1-.L0(%r13)
		 *	0x14	sllg	%r9,%r8,0
		 */
		DBG_CALL(Dbg_reloc_transition(ofl->ofl_lml, M_MACH,
		    R_390_TLS_LE64, arsp));
		arsp->rel_rtype = R_390_TLS_LE64;

		/*
		 * Adjust 'offset' to beginning of instruction sequence.
		 */
		offset -= 3;
		(void) memcpy(offset, TLS_IE_LE, sizeof (TLS_IE_LE));
		return (FIX_RELOC);

	}
	return (FIX_RELOC);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_do_activerelocs.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uintptr_t
ld_do_activerelocs(Ofl_desc *ofl)
{
	Rel_desc	*arsp;
	Rel_cache	*rcp;
	Listnode	*lnp;
	uintptr_t	return_code = 1;
	ofl_flag_t	flags = ofl->ofl_flags;

	if (ofl->ofl_actrels.head)
		DBG_CALL(Dbg_reloc_doact_title(ofl->ofl_lml));

	/*
	 * Process active relocations.
	 */
	for (LIST_TRAVERSE(&ofl->ofl_actrels, lnp, rcp)) {
		/* LINTED */
		for (arsp = (Rel_desc *)(rcp + 1);
		    arsp < rcp->rc_free; arsp++) {
			uchar_t		*addr;
			Xword 		value;
			Sym_desc	*sdp;
			const char	*ifl_name;
			Xword		refaddr;
			int		moved = 0;
			Gotref		gref;

			/*
			 * If the section this relocation is against has been
			 * discarded (-zignore), then discard (skip) the
			 * relocation itself.
			 */
			if ((arsp->rel_isdesc->is_flags & FLG_IS_DISCARD) &&
			    ((arsp->rel_flags &
			    (FLG_REL_GOT | FLG_REL_BSS |
			    FLG_REL_PLT | FLG_REL_NOINFO)) == 0)) {
				DBG_CALL(Dbg_reloc_discard(ofl->ofl_lml,
				    M_MACH, arsp));
				continue;
			}

			/*
			 * We deteremine what the 'got reference'
			 * model (if required) is at this point.  This
			 * needs to be done before tls_fixup() since
			 * it may 'transition' our instructions.
			 *
			 * The got table entries have already been assigned,
			 * and we bind to those initial entries.
			 */
			if (arsp->rel_flags & FLG_REL_DTLS)
				gref = GOT_REF_TLSGD;
			else if (arsp->rel_flags & FLG_REL_MTLS)
				gref = GOT_REF_TLSLD;
			else if (arsp->rel_flags & FLG_REL_STLS)
				gref = GOT_REF_TLSIE;
			else
				gref = GOT_REF_GENERIC;

			/*
			 * Perform any required TLS fixups.
			 */
			if (arsp->rel_flags & FLG_REL_TLSFIX) {
				Fixupret	ret;

				if ((ret = tls_fixups(ofl, arsp)) == FIX_ERROR)
					return (S_ERROR);
				if (ret == FIX_DONE)
					continue;
			}

			/*
			 * If this is a relocation against a move table, or
			 * expanded move table, adjust the relocation entries.
			 */
			if (arsp->rel_move)
				ld_adj_movereloc(ofl, arsp);

			sdp = arsp->rel_sym;
			refaddr = arsp->rel_roffset +
			    (Off)_elf_getxoff(arsp->rel_isdesc->is_indata);

			if (arsp->rel_flags & FLG_REL_CLVAL)
				value = 0;
			else if (ELF_ST_TYPE(sdp->sd_sym->st_info) ==
			    STT_SECTION) {
				Sym_desc	*sym;

				/*
				 * The value for a symbol pointing to a SECTION
				 * is based off of that sections position.
				 *
				 * The second argument of the ld_am_I_partial()
				 * is the value stored at the target address
				 * relocation is going to be applied.
				 */
				if ((sdp->sd_isc->is_flags & FLG_IS_RELUPD) &&
				    /* LINTED */
				    (sym = ld_am_I_partial(arsp, *(Xword *)
				    ((uchar_t *)
				    arsp->rel_isdesc->is_indata->d_buf +
				    arsp->rel_roffset)))) {
					/*
					 * If the symbol is moved,
					 * adjust the value
					 */
					value = sym->sd_sym->st_value;
					moved = 1;
				} else {
					value = _elf_getxoff(
					    sdp->sd_isc->is_indata);
					if (sdp->sd_isc->is_shdr->sh_flags &
					    SHF_ALLOC)
						value += sdp->sd_isc->
						    is_osdesc->os_shdr->sh_addr;
				}
				if (sdp->sd_isc->is_shdr->sh_flags & SHF_TLS)
					value -= ofl->ofl_tlsphdr->p_vaddr;

			} else if (IS_SIZE(arsp->rel_rtype)) {
				/*
				 * Size relocations require the symbols size.
				 */
				value = sdp->sd_sym->st_size;
			} else {
				/*
				 * Else the value is the symbols value.
				 */
				value = sdp->sd_sym->st_value;
			}

			/*
			 * Relocation against the GLOBAL_OFFSET_TABLE.
			 */
			if (arsp->rel_flags & FLG_REL_GOT)
				arsp->rel_osdesc = ofl->ofl_osgot;

			/*
			 * If loadable and not producing a relocatable object
			 * add the sections virtual address to the reference
			 * address.
			 */
			if ((arsp->rel_flags & FLG_REL_LOAD) &&
			    ((flags & FLG_OF_RELOBJ) == 0))
				refaddr += arsp->rel_isdesc->is_osdesc->
				    os_shdr->sh_addr;

			/*
			 * If this entry has a PLT assigned to it, it's
			 * value is actually the address of the PLT (and
			 * not the address of the function).
			 */
			if (IS_PLT(arsp->rel_rtype)) {
				if (sdp->sd_aux && sdp->sd_aux->sa_PLTndx)
					value = ld_calc_plt_addr(sdp, ofl);
			}

			/*
			 * Add relocations addend to value.  Add extra
			 * relocation addend if needed.
			 */
			value += arsp->rel_raddend;

			/*
			 * Determine whether the value needs further adjustment.
			 * Filter through the attributes of the relocation to
			 * determine what adjustment is required.  Note, many
			 * of the following cases are only applicable when a
			 * .got is present.  As a .got is not generated when a
			 * relocatable object is being built, any adjustments
			 * that require a .got need to be skipped.
			 */
			if ((arsp->rel_flags & FLG_REL_GOT) &&
			    ((flags & FLG_OF_RELOBJ) == 0)) {
				Xword		R1addr;
				uintptr_t	R2addr;
				Word		gotndx;
				Gotndx		*gnp;

				/*
				 * Perform relocation against GOT table.  Since
				 * this doesn't fit exactly into a relocation
				 * we place the appropriate byte in the GOT
				 * directly
				 *
				 * Calculate offset into GOT at which to apply
				 * the relocation.
				 */
				gnp = ld_find_gotndx(&(sdp->sd_GOTndxs), gref,
				    ofl, 0);
				assert(gnp);

				if (arsp->rel_rtype == R_390_TLS_DTPOFF)
					gotndx = gnp->gn_gotndx + 1;
				else
					gotndx = gnp->gn_gotndx;

				R1addr = (Xword)(gotndx * M_GOT_ENTSIZE);

				/*
				 * Add the GOTs data's offset.
				 */
				R2addr = R1addr + (uintptr_t)
				    arsp->rel_osdesc->os_outdata->d_buf;

				DBG_CALL(Dbg_reloc_doact(ofl->ofl_lml,
				    ELF_DBG_LD, M_MACH, SHT_REL,
				    arsp->rel_rtype, R1addr, value,
				    arsp->rel_sname, arsp->rel_osdesc));

				/*
				 * And do it.
				 */
				if (ofl->ofl_flags1 & FLG_OF1_ENCDIFF)
					*(Xword *)R2addr =
					    ld_bswap_Xword(value);
				else
					*(Xword *)R2addr = value;
				continue;

			} else if (IS_GOT_BASED(arsp->rel_rtype) &&
			    ((flags & FLG_OF_RELOBJ) == 0)) {
				value -= ofl->ofl_osgot->os_shdr->sh_addr;

			} else if (IS_GOT_PC(arsp->rel_rtype) &&
			    ((flags & FLG_OF_RELOBJ) == 0)) {
				value =
				    (Xword)(ofl->ofl_osgot->os_shdr->sh_addr) -
				    refaddr;

			} else if ((IS_PC_RELATIVE(arsp->rel_rtype)) &&
			    (((flags & FLG_OF_RELOBJ) == 0) ||
			    (arsp->rel_osdesc == sdp->sd_isc->is_osdesc))) {
				value -= refaddr;

			} else if (IS_TLS_INS(arsp->rel_rtype) &&
			    IS_GOT_RELATIVE(arsp->rel_rtype) &&
			    ((flags & FLG_OF_RELOBJ) == 0)) {
				Gotndx	*gnp;

				gnp = ld_find_gotndx(&(sdp->sd_GOTndxs), gref,
				    ofl, 0);
				assert(gnp);
				value = (Xword)gnp->gn_gotndx * M_GOT_ENTSIZE;
				if (arsp->rel_rtype == R_390_TLS_IE) {
					value +=
					    ofl->ofl_osgot->os_shdr->sh_addr;
				}

			} else if (IS_GOT_RELATIVE(arsp->rel_rtype) &&
			    ((flags & FLG_OF_RELOBJ) == 0)) {
				Gotndx *gnp;

				gnp = ld_find_gotndx(&(sdp->sd_GOTndxs),
				    GOT_REF_GENERIC, ofl, 0);
				assert(gnp);
				value = (Xword)gnp->gn_gotndx * M_GOT_ENTSIZE;

			} else if ((arsp->rel_flags & FLG_REL_STLS) &&
			    ((flags & FLG_OF_RELOBJ) == 0)) {
				Xword	tlsstatsize;

				/*
				 * This is the LE TLS reference model.  Static
				 * offset is hard-coded.
				 */
				tlsstatsize =
				    S_ROUND(ofl->ofl_tlsphdr->p_memsz,
				    M_TLSSTATALIGN);
				value = tlsstatsize - value;

				/*
				 * Since this code is fixed up, it assumes a
				 * negative offset that can be added to the
				 * thread pointer.
				 */
				if ((arsp->rel_rtype == R_390_TLS_LDO) ||
				    (arsp->rel_rtype == R_390_TLS_LE))
					value = -value;
			} else if ((arsp->rel_rtype == R_390_PLTOFF16) ||
				   (arsp->rel_rtype == R_390_PLTOFF32)) {
					/* 
					 * PLTOFF values are displacements
					 * from the GOT
					 */
					value -= ofl->ofl_osgot->os_shdr->sh_addr;
			}

			if (arsp->rel_isdesc->is_file)
				ifl_name = arsp->rel_isdesc->is_file->ifl_name;
			else
				ifl_name = MSG_INTL(MSG_STR_NULL);

			/*
			 * Make sure we have data to relocate.  Compiler and
			 * assembler developers have been known to generate
			 * relocations against invalid sections (normally .bss),
			 * so for their benefit give them sufficient information
			 * to help analyze the problem.  End users should never
			 * see this.
			 */
			if (arsp->rel_isdesc->is_indata->d_buf == 0) {
				Conv_inv_buf_t	inv_buf;

				eprintf(ofl->ofl_lml, ERR_FATAL,
				    MSG_INTL(MSG_REL_EMPTYSEC),
				    conv_reloc_s390_type(arsp->rel_rtype,
				    0, &inv_buf),
				    ifl_name, demangle(arsp->rel_sname),
				    arsp->rel_isdesc->is_name);
				return (S_ERROR);
			}

			/*
			 * Get the address of the data item we need to modify.
			 */
			addr = (uchar_t *)((uintptr_t)arsp->rel_roffset +
			    (uintptr_t)_elf_getxoff(arsp->rel_isdesc->
			    is_indata));

			DBG_CALL(Dbg_reloc_doact(ofl->ofl_lml, ELF_DBG_LD,
			    M_MACH, SHT_REL, arsp->rel_rtype, EC_NATPTR(addr),
			    value, arsp->rel_sname, arsp->rel_osdesc));
			addr += (uintptr_t)arsp->rel_osdesc->os_outdata->d_buf;

			if ((((uintptr_t)addr - (uintptr_t)ofl->ofl_nehdr) >
			    ofl->ofl_size) || (arsp->rel_roffset >
			    arsp->rel_osdesc->os_shdr->sh_size)) {
				Conv_inv_buf_t	inv_buf;
				int		class;

				if (((uintptr_t)addr -
				    (uintptr_t)ofl->ofl_nehdr) > ofl->ofl_size)
					class = ERR_FATAL;
				else
					class = ERR_WARNING;

				eprintf(ofl->ofl_lml, class,
				    MSG_INTL(MSG_REL_INVALOFFSET),
				    conv_reloc_s390_type(arsp->rel_rtype,
				    0, &inv_buf),
				    ifl_name, arsp->rel_isdesc->is_name,
				    demangle(arsp->rel_sname),
				    EC_ADDR((uintptr_t)addr -
				    (uintptr_t)ofl->ofl_nehdr));

				if (class == ERR_FATAL) {
					return_code = S_ERROR;
					continue;
				}
			}

			/*
			 * The relocation is additive.  Ignore the previous
			 * symbol value if this local partial symbol is
			 * expanded.
			 */
			if (moved)
				value -= *addr;

			/*
			 * If we have a replacement value for the relocation
			 * target, put it in place now.
			 */
			if (arsp->rel_flags & FLG_REL_NADDEND) {
				Xword addend = arsp->rel_raddend;

				if (ld_reloc_targval_set(ofl, arsp,
				    addr, addend) == 0)
					return (S_ERROR);
			}

			/*
			 * If '-z noreloc' is specified - skip the do_reloc_ld
			 * stage.
			 */
			if (OFL_DO_RELOC(ofl)) {
				if (do_reloc_ld((uchar_t)arsp->rel_rtype, addr,
				    &value, arsp->rel_sname, ifl_name,
				    OFL_SWAP_RELOC_DATA(ofl, arsp),
				    ofl->ofl_lml) == 0)
					return_code = S_ERROR;
			}
		}
	}
	return (return_code);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_add_outrel.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uintptr_t
ld_add_outrel(Word flags, Rel_desc *rsp, Ofl_desc *ofl)
{
	Rel_desc	*orsp;
	Rel_cache	*rcp;
	Sym_desc	*sdp = rsp->rel_sym;
	Conv_inv_buf_t	inv_buf;

	/*
	 * Static executables *do not* want any relocations against them.
	 * Since our engine still creates relocations against a WEAK UNDEFINED
	 * symbol in a static executable, it's best to disable them here
	 * instead of through out the relocation code.
	 */
	if ((ofl->ofl_flags & (FLG_OF_STATIC | FLG_OF_EXEC)) ==
	    (FLG_OF_STATIC | FLG_OF_EXEC))
		return (1);

	/*
	 * If no relocation cache structures are available allocate
	 * a new one and link it into the cache list.
	 */
	if ((ofl->ofl_outrels.tail == 0) ||
	    ((rcp = (Rel_cache *)ofl->ofl_outrels.tail->data) == 0) ||
	    ((orsp = rcp->rc_free) == rcp->rc_end)) {
		static size_t	nextsize = 0;
		size_t		size;

		/*
		 * Output relocation numbers can vary considerably between
		 * building executables or shared objects (pic vs. non-pic),
		 * etc.  But, they typically aren't very large, so for these
		 * objects use a standard bucket size.  For building relocatable
		 * objects, typically there will be an output relocation for
		 * every input relocation.
		 */
		if (nextsize == 0) {
			if (ofl->ofl_flags & FLG_OF_RELOBJ) {
				if ((size = ofl->ofl_relocincnt) == 0)
					size = REL_LOIDESCNO;
				if (size > REL_HOIDESCNO)
					nextsize = REL_HOIDESCNO;
				else
					nextsize = REL_LOIDESCNO;
			} else
				nextsize = size = REL_HOIDESCNO;
		} else
			size = nextsize;

		size = size * sizeof (Rel_desc);

		if (((rcp = libld_malloc(sizeof (Rel_cache) + size)) == 0) ||
		    (list_appendc(&ofl->ofl_outrels, rcp) == 0))
			return (S_ERROR);

		/* LINTED */
		rcp->rc_free = orsp = (Rel_desc *)(rcp + 1);
		/* LINTED */
		rcp->rc_end = (Rel_desc *)((char *)rcp->rc_free + size);
	}


	/*
	 * If we are adding a output relocation against a section
	 * symbol (non-RELATIVE) then mark that section.  These sections
	 * will be added to the .dynsym symbol table.
	 */
	if (sdp && (rsp->rel_rtype != M_R_RELATIVE) &&
	    ((flags & FLG_REL_SCNNDX) ||
	    (ELF_ST_TYPE(sdp->sd_sym->st_info) == STT_SECTION))) {

		/*
		 * If this is a COMMON symbol - no output section
		 * exists yet - (it's created as part of sym_validate()).
		 * So - we mark here that when it's created it should
		 * be tagged with the FLG_OS_OUTREL flag.
		 */
		if ((sdp->sd_flags & FLG_SY_SPECSEC) &&
		    (sdp->sd_sym->st_shndx == SHN_COMMON)) {
			if (ELF_ST_TYPE(sdp->sd_sym->st_info) != STT_TLS)
				ofl->ofl_flags1 |= FLG_OF1_BSSOREL;
			else
				ofl->ofl_flags1 |= FLG_OF1_TLSOREL;
		} else {
			Os_desc	*osp = sdp->sd_isc->is_osdesc;

			if (osp && ((osp->os_flags & FLG_OS_OUTREL) == 0)) {
				ofl->ofl_dynshdrcnt++;
				osp->os_flags |= FLG_OS_OUTREL;
			}
		}
	}

	*orsp = *rsp;
	orsp->rel_flags |= flags;

	rcp->rc_free++;
	ofl->ofl_outrelscnt++;

	if (flags & FLG_REL_GOT)
		ofl->ofl_relocgotsz += (Xword)sizeof (Rela);
	else if (flags & FLG_REL_PLT)
		ofl->ofl_relocpltsz += (Xword)sizeof (Rela);
	else if (flags & FLG_REL_BSS)
		ofl->ofl_relocbsssz += (Xword)sizeof (Rela);
	else if (flags & FLG_REL_NOINFO)
		ofl->ofl_relocrelsz += (Xword)sizeof (Rela);
	else
		orsp->rel_osdesc->os_szoutrels += (Xword)sizeof (Rela);

	if (orsp->rel_rtype == M_R_RELATIVE)
		ofl->ofl_relocrelcnt++;

	/*
	 * We don't perform sorting on PLT relocations because
	 * they have already been assigned a PLT index and if we
	 * were to sort them we would have to re-assign the plt indexes.
	 */
	if (!(flags & FLG_REL_PLT))
		ofl->ofl_reloccnt++;

	/*
	 * Insure a GLOBAL_OFFSET_TABLE is generated if required.
	 */
	if (IS_GOT_REQUIRED(orsp->rel_rtype))
		ofl->ofl_flags |= FLG_OF_BLDGOT;

	/*
	 * Identify and possibly warn of a displacement relocation.
	 */
	if (orsp->rel_flags & FLG_REL_DISP) {
		ofl->ofl_dtflags_1 |= DF_1_DISPRELPND;

		if (ofl->ofl_flags & FLG_OF_VERBOSE)
			ld_disp_errmsg(MSG_INTL(MSG_REL_DISPREL4), orsp, ofl);
	}
	DBG_CALL(Dbg_reloc_ors_entry(ofl->ofl_lml, ELF_DBG_LD, SHT_RELA,
	    M_MACH, orsp));
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_reloc_local.                                   */
/*                                                                  */
/* Function	- Process relocations for a LOCAL symbol.           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uintptr_t
ld_reloc_local(Rel_desc * rsp, Ofl_desc * ofl)
{
	ofl_flag_t	flags = ofl->ofl_flags;
	Sym_desc	*sdp = rsp->rel_sym;
	Word		shndx = sdp->sd_sym->st_shndx;

	/*
	 * if ((shared object) and (not pc relative relocation) and
	 *    (not against ABS symbol))
	 * then
	 *	if (rtype != R_390_32)
	 *	then
	 *		build relocation against section
	 *	else
	 *		build R_390_RELATIVE
	 *	fi
	 * fi
	 */
	if ((flags & FLG_OF_SHAROBJ) && (rsp->rel_flags & FLG_REL_LOAD) &&
	    !(IS_PC_RELATIVE(rsp->rel_rtype)) && !(IS_SIZE(rsp->rel_rtype)) &&
	    !(IS_GOT_BASED(rsp->rel_rtype)) &&
	    !(rsp->rel_isdesc != NULL &&
	    (rsp->rel_isdesc->is_shdr->sh_type == SHT_SUNW_dof)) &&
	    (((sdp->sd_flags & FLG_SY_SPECSEC) == 0) ||
	    (shndx != SHN_ABS) || (sdp->sd_aux && sdp->sd_aux->sa_symspec))) {
		Word	ortype = rsp->rel_rtype;

		if ((rsp->rel_rtype != R_390_32) &&
		    (rsp->rel_rtype != R_390_PLT32) &&
		    (rsp->rel_rtype != R_390_64))
			return (ld_add_outrel((FLG_REL_SCNNDX | FLG_REL_ADVAL),
			    rsp, ofl));

		rsp->rel_rtype = R_390_RELATIVE;
		if (ld_add_outrel(FLG_REL_ADVAL, rsp, ofl) == S_ERROR)
			return (S_ERROR);
		rsp->rel_rtype = ortype;
		return (1);
	}

	/*
	 * If the relocation is against a 'non-allocatable' section
	 * and we can not resolve it now - then give a warning
	 * message.
	 *
	 * We can not resolve the symbol if either:
	 *	a) it's undefined
	 *	b) it's defined in a shared library and a
	 *	   COPY relocation hasn't moved it to the executable
	 *
	 * Note: because we process all of the relocations against the
	 *	text segment before any others - we know whether
	 *	or not a copy relocation will be generated before
	 *	we get here (see reloc_init()->reloc_segments()).
	 */
	if (!(rsp->rel_flags & FLG_REL_LOAD) &&
	    ((shndx == SHN_UNDEF) ||
	    ((sdp->sd_ref == REF_DYN_NEED) &&
	    ((sdp->sd_flags & FLG_SY_MVTOCOMM) == 0)))) {
		Conv_inv_buf_t	inv_buf;

		/*
		 * If the relocation is against a SHT_SUNW_ANNOTATE
		 * section - then silently ignore that the relocation
		 * can not be resolved.
		 */
		if (rsp->rel_osdesc &&
		    (rsp->rel_osdesc->os_shdr->sh_type == SHT_SUNW_ANNOTATE))
			return (0);
		(void) eprintf(ofl->ofl_lml, ERR_WARNING,
		    MSG_INTL(MSG_REL_EXTERNSYM),
		    conv_reloc_s390_type(rsp->rel_rtype, 0, &inv_buf),
		    rsp->rel_isdesc->is_file->ifl_name,
		    demangle(rsp->rel_sname), rsp->rel_osdesc->os_name);
		return (1);
	}

	/*
	 * Perform relocation.
	 */
	return (ld_add_actrel(NULL, rsp, ofl));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_reloc_TLS.                                     */
/*                                                                  */
/* Function	- Do relocation related to TLS.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uintptr_t
ld_reloc_TLS(Boolean local, Rel_desc *rsp, Ofl_desc *ofl)
{
	Word		rtype = rsp->rel_rtype;
	Sym_desc	*sdp = rsp->rel_sym;
	ofl_flag_t	flags = ofl->ofl_flags;
	Gotndx		*gnp;

	/*
	 * If we're building an executable - use either the IE or LE access
	 * model.  If we're building a shared object process any IE model.
	 */
	if ((flags & FLG_OF_EXEC) || (IS_TLS_IE(rtype))) {
		/*
		 * Set the DF_STATIC_TLS flag.
		 */
		ofl->ofl_dtflags |= DF_STATIC_TLS;

		if (!local || ((flags & FLG_OF_EXEC) == 0)) {
			/*
			 * Assign a GOT entry for static TLS references.
			 */
			if ((gnp = ld_find_gotndx(&(sdp->sd_GOTndxs),
			    GOT_REF_TLSIE, ofl, 0)) == 0) {

				if (ld_assign_got_TLS(local, rsp, ofl, sdp,
				    gnp, GOT_REF_TLSIE, FLG_REL_STLS,
				    rtype, R_390_TLS_TPOFF, 0) == S_ERROR)
					return (S_ERROR);
			}

			/*
			 * IE access model.
			 */
			if (IS_TLS_IE(rtype)) {
				if (ld_add_actrel(FLG_REL_STLS,
				    rsp, ofl) == S_ERROR)
					return (S_ERROR);

				/*
				 * A non-pic shared object needs to adjust the
				 * active relocation (indntpoff).
				 */
				if (((flags & FLG_OF_EXEC) == 0) &&
				    (rtype == R_390_TLS_IE64)) {
					rsp->rel_rtype = R_390_RELATIVE;
					return (ld_add_outrel(NULL, rsp, ofl));
				}
				return (1);
			}

			/*
			 * Fixups are required for other executable models.
			 */
			return (ld_add_actrel((FLG_REL_TLSFIX | FLG_REL_STLS),
			    rsp, ofl));
		}

		/*
		 * LE access model.
		 */
		if (IS_TLS_LE(rtype) || (rtype == R_390_TLS_LDO))
			return (ld_add_actrel(FLG_REL_STLS, rsp, ofl));

		return (ld_add_actrel((FLG_REL_TLSFIX | FLG_REL_STLS),
		    rsp, ofl));
	}

	/*
	 * Building a shared object.
	 *
	 * 	Assign a GOT entry for a dynamic TLS reference.
	 */
	if (IS_TLS_LD(rtype) && ((gnp = ld_find_gotndx(&(sdp->sd_GOTndxs),
	    GOT_REF_TLSLD, ofl, 0)) == 0)) {

		if (ld_assign_got_TLS(local, rsp, ofl, sdp, gnp, GOT_REF_TLSLD,
		    FLG_REL_MTLS, rtype, R_390_TLS_DTPMOD, 0) == S_ERROR)
			return (S_ERROR);

	} else if (IS_TLS_GD(rtype) &&
	    ((gnp = ld_find_gotndx(&(sdp->sd_GOTndxs), GOT_REF_TLSGD,
	    ofl, 0)) == 0)) {

		if (ld_assign_got_TLS(local, rsp, ofl, sdp, gnp, GOT_REF_TLSGD,
		    FLG_REL_DTLS, rtype, R_390_TLS_DTPMOD,
		    R_390_TLS_DTPOFF) == S_ERROR)
			return (S_ERROR);
	}

	/*
	 * For GD/LD TLS reference - TLS_{GD,LD}_CALL, this will eventually
	 * cause a call to __tls_get_offset().  Convert this relocation to that
	 * symbol now, and prepare for the PLT magic.
	 */
	if ((rtype == R_390_TLS_GDCALL) || (rtype == R_390_TLS_LDCALL)) {
		Sym_desc	*tlsgetsym;

		if ((tlsgetsym = ld_sym_add_u(MSG_ORIG(MSG_SYM_TLSGETOFFS_U),
		    ofl, MSG_STR_TLSREL)) == (Sym_desc *)S_ERROR)
			return (S_ERROR);

		rsp->rel_sym   = tlsgetsym;
		rsp->rel_sname = tlsgetsym->sd_name;
		rsp->rel_rtype = R_390_PLT32DBL;

		if (ld_reloc_plt(rsp, ofl) == S_ERROR)
			return (S_ERROR);

		rsp->rel_sym = sdp;
		rsp->rel_sname = sdp->sd_name;
		rsp->rel_rtype = rtype;
		return (1);
	}

	if (IS_TLS_LD(rtype))
		return (ld_add_actrel(FLG_REL_MTLS, rsp, ofl));

	return (ld_add_actrel(FLG_REL_DTLS, rsp, ofl));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_find_gotndx.                                   */
/*                                                                  */
/* Function	- Search the GOT index list for a GOT entry with    */
/*		  the proper addend.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static Gotndx *
ld_find_gotndx(List * lst, Gotref gref, Ofl_desc * ofl, Rel_desc * rdesc)
{
	Listnode *	lnp;
	Gotndx *	gnp;

	if ((gref == GOT_REF_TLSLD) && ofl->ofl_tlsldgotndx)
		return (ofl->ofl_tlsldgotndx);

	for (LIST_TRAVERSE(lst, lnp, gnp)) {
//		if ((rdesc->rel_raddend == gnp->gn_addend) &&
		if ((gref == gnp->gn_gotref))
			return (gnp);
	}
	return ((Gotndx *)0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_calc_got_offset.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static Xword
ld_calc_got_offset(Rel_desc * rdesc, Ofl_desc * ofl)
{
	Os_desc		*osp = ofl->ofl_osgot;
	Sym_desc	*sdp = rdesc->rel_sym;
	Xword		gotndx;
	Gotref		gref;
	Gotndx		*gnp;

	if (rdesc->rel_flags & FLG_REL_DTLS)
		gref = GOT_REF_TLSGD;
	else if (rdesc->rel_flags & FLG_REL_MTLS)
		gref = GOT_REF_TLSLD;
	else if (rdesc->rel_flags & FLG_REL_STLS)
		gref = GOT_REF_TLSIE;
	else
		gref = GOT_REF_GENERIC;

	gnp = ld_find_gotndx(&(sdp->sd_GOTndxs), gref, ofl, rdesc);
	assert(gnp);

	gotndx = (Xword)gnp->gn_gotndx;

	if ((rdesc->rel_flags & FLG_REL_DTLS) &&
	    (rdesc->rel_rtype == R_390_TLS_DTPOFF))
		gotndx++;

	return ((Xword)(osp->os_shdr->sh_addr + (gotndx * M_GOT_ENTSIZE)));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_assign_got_ndx.                                */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uintptr_t
ld_assign_got_ndx(List * lst, Gotndx * pgnp, Gotref gref, Ofl_desc * ofl,
    Rel_desc * rsp, Sym_desc * sdp)
{
	Gotndx	*gnp;
	uint_t	gotents;

	if (pgnp)
		return (1);

	if ((gref == GOT_REF_TLSGD) || (gref == GOT_REF_TLSLD))
		gotents = 2;
	else
		gotents = 1;

	if ((gnp = libld_calloc(sizeof (Gotndx), 1)) == 0)
		return (S_ERROR);
	gnp->gn_gotndx = ofl->ofl_gotcnt;
	gnp->gn_gotref = gref;

	ofl->ofl_gotcnt += gotents;

	if (gref == GOT_REF_TLSLD) {
		ofl->ofl_tlsldgotndx = gnp;
		return (1);
	}

	if (list_appendc(lst, (void *)gnp) == 0)
		return (S_ERROR);

	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_assign_plt_ndx.                                */
/*                                                                  */
/* Function	- Assign plt index, got index, and flag that we     */
/*		  want got built.              		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ld_assign_plt_ndx(Sym_desc * sdp, Ofl_desc *ofl)
{
	sdp->sd_aux->sa_PLTndx    = 1 + ofl->ofl_pltcnt++;
	sdp->sd_aux->sa_PLTGOTndx = ofl->ofl_gotcnt++;
	ofl->ofl_flags           |= FLG_OF_BLDGOT;
}

/*========================= End of Function ========================*/

#ifdef _ELF64

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_fillin_gotplt.                                 */
/*                                                                  */
/* Function	- Initializes .got[0] with the _DYNAMIC symbol      */
/*		  value.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uintptr_t
ld_fillin_gotplt(Ofl_desc *ofl)
{
	ofl_flag_t	flags = ofl->ofl_flags;
	int		bswap = (ofl->ofl_flags1 & FLG_OF1_ENCDIFF) != 0;
	Sxword		gottbl;

	if (ofl->ofl_osgot) {
		Sym_desc	*sdp;

		if ((sdp = ld_sym_find(MSG_ORIG(MSG_SYM_DYNAMIC_U),
		    SYM_NOHASH, 0, ofl)) != NULL) {
			Xword *genptr;

			genptr = ((Xword *) ofl->ofl_osgot->os_outdata->d_buf +
			    (M_GOT_XDYNAMIC * M_GOT_ENTSIZE));
			/* LINTED */
			*genptr = (Xword) sdp->sd_sym->st_value;
			if (bswap)
				*genptr = ld_bswap_Xword(*genptr);
		}
	}

	/*
	 * Fill in the reserved slot in the procedure linkage table.
	 */
	if ((flags & FLG_OF_DYNAMIC) && ofl->ofl_osplt) {
		plt0_t *pltent;

		pltent = (plt0_t *) ofl->ofl_osplt->os_outdata->d_buf;
		(void *) memcpy(pltent, &plt0Template, M_PLT_ENTSIZE);
		gottbl = ((Sxword) ofl->ofl_osgot->os_shdr->sh_addr - 
			  OFFSETOF(plt0_t, pltent, larl) -
			  (Sxword) ofl->ofl_osplt->os_shdr->sh_addr) >> 1;
		SWORD(pltent->got) = (Sword) gottbl;
	}
	return (1);
}

/*========================= End of Function ========================*/

#else 

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_fillin_gotplt.                                 */
/*                                                                  */
/* Function	- Initializes .got[0] with the _DYNAMIC symbol      */
/*		  value.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uintptr_t
ld_fillin_gotplt(Ofl_desc *ofl)
{
	ofl_flag_t	flags = ofl->ofl_flags;
	int		bswap = (ofl->ofl_flags1 & FLG_OF1_ENCDIFF) != 0;

	if (ofl->ofl_osgot) {
		Sym_desc	*sdp;

		if ((sdp = ld_sym_find(MSG_ORIG(MSG_SYM_DYNAMIC_U),
		    SYM_NOHASH, 0, ofl)) != NULL) {
			Word *genptr;

			genptr = ((Word *)ofl->ofl_osgot->os_outdata->d_buf +
			    (M_GOT_XDYNAMIC * M_GOT_ENTSIZE));
			/* LINTED */
			*genptr = (Xword) sdp->sd_sym->st_value;
			if (bswap)
				*genptr = ld_bswap_Word(*genptr);
		}
	}

	/*
	 * Fill in the reserved slot in the procedure linkage table.
	 */
	if ((flags & FLG_OF_DYNAMIC) && ofl->ofl_osplt) {
		if (!(flags & FLG_OF_SHAROBJ)) {
			plt0_abs_t *pltent;

			pltent = (plt0_abs_t *) ofl->ofl_osplt->os_outdata->d_buf;
			(void *) memcpy(pltent, &plt0TemplateABS, M_PLT_ENTSIZE);
			WORD(pltent->gotaddr) = ofl->ofl_osgot->os_shdr->sh_addr;
		} else {
			plt0_pic_t *pltent;

			pltent = (plt0_pic_t *) ofl->ofl_osplt->os_outdata->d_buf;
			(void *) memcpy(pltent, &plt0Template, M_PLT_ENTSIZE);
			SWORD(pltent->got) = ((Sword) ofl->ofl_osgot->os_shdr->sh_addr - 
					      OFFSETOF(plt0_pic_t, pltent, larl) -
				  	      (Sword) ofl->ofl_osplt->os_shdr->sh_addr) >> 1;
		}
	}
	return (1);
}

/*========================= End of Function ========================*/

#endif

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ld_targ_init_s390.                                */
/*                                                                  */
/* Function	- Return the ld_targ definition for this target.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

const Target *
ld_targ_init_s390(void)
{
	static const Target _ld_targ = {
		{			/* Target_mach */
			M_MACH,			/* m_mach */
			M_MACHPLUS,		/* m_machplus */
			M_FLAGSPLUS,		/* m_flagsplus */
			M_CLASS,		/* m_class */
			M_DATA,			/* m_data */

			M_SEGM_ALIGN,		/* m_segm_align */
			M_SEGM_ORIGIN,		/* m_segm_origin */
			M_DATASEG_PERM,		/* m_dataseg_perm */
			M_WORD_ALIGN,		/* m_word_align */
						/* m_def_interp */
#if defined(_ELF64)
			MSG_ORIG(MSG_PTH_RTLD_S390X),
#else
			MSG_ORIG(MSG_PTH_RTLD),
#endif

			/* Relocation type codes */
			M_R_ARRAYADDR,		/* m_r_arrayaddr */
			M_R_COPY,		/* m_r_copy */
			M_R_GLOB_DAT,		/* m_r_glob_dat */
			M_R_JMP_SLOT,		/* m_r_jmp_slot */
			M_R_NUM,		/* m_r_num */
			M_R_NONE,		/* m_r_none */
			M_R_RELATIVE,		/* m_r_relative */
			M_R_REGISTER,		/* m_r_register */

			/* Relocation related constants */
			M_REL_DT_COUNT,		/* m_rel_dt_count */
			M_REL_DT_ENT,		/* m_rel_dt_ent */
			M_REL_DT_SIZE,		/* m_rel_dt_size */
			M_REL_DT_TYPE,		/* m_rel_dt_type */
			M_REL_SHT_TYPE,		/* m_rel_sht_type */

			/* GOT related constants */
			M_GOT_ENTSIZE,		/* m_got_entsize */
			M_GOT_XNumber,		/* m_got_xnumber */

			/* PLT related constants */
			M_PLT_ALIGN,		/* m_plt_align */
			M_PLT_ENTSIZE,		/* m_plt_entsize */
			M_PLT_RESERVSZ,		/* m_plt_reservsz */
			M_PLT_SHF_FLAGS,	/* m_plt_shf_flags */

			M_DT_REGISTER,		/* m_dt_register */
		},
		{			/* Target_machid */
			M_ID_ARRAY,		/* id_array */
			M_ID_BSS,		/* id_bss */
			M_ID_CAP,		/* id_cap */
			M_ID_DATA,		/* id_data */
			M_ID_DYNAMIC,		/* id_dynamic */
			M_ID_DYNSORT,		/* id_dynsort */
			M_ID_DYNSTR,		/* id_dynstr */
			M_ID_DYNSYM,		/* id_dynsym */
			M_ID_DYNSYM_NDX,	/* id_dynsym_ndx */
			M_ID_GOT,		/* id_got */
			M_ID_GOTDATA,		/* id_gotdata */
			M_ID_HASH,		/* id_hash */
			M_ID_INTERP,		/* id_interp */
			M_ID_UNKNOWN,		/* id_lbss (unused) */
			M_ID_LDYNSYM,		/* id_ldynsym */
			M_ID_NOTE,		/* id_note */
			M_ID_NULL,		/* id_null */
			M_ID_PLT,		/* id_plt */
			M_ID_REL,		/* id_rel */
			M_ID_STRTAB,		/* id_strtab */
			M_ID_SYMINFO,		/* id_syminfo */
			M_ID_SYMTAB,		/* id_symtab */
			M_ID_SYMTAB_NDX,	/* id_symtab_ndx */
			M_ID_TEXT,		/* id_text */
			M_ID_TLS,		/* id_tls */
			M_ID_TLSBSS,		/* id_tlsbss */
			M_ID_UNKNOWN,		/* id_unknown */
			M_ID_UNKNOWN,		/* id_unwind (unused) */
			M_ID_USER,		/* id_user */
			M_ID_VERSION,		/* id_version */
		},
		{			/* Target_nullfunc */
			nullfunc_tmpl,		/* nf_template */
			sizeof (nullfunc_tmpl),	/* nf_size */
		},
		{			/* Target_machrel */
			reloc_table,

			ld_init_rel,		/* mr_init_rel */
			ld_mach_eflags,		/* mr_mach_eflags */
			ld_mach_make_dynamic,	/* mr_mach_make_dynamic */
			ld_mach_update_odynamic, /* mr_mach_update_odynamic */
			ld_calc_plt_addr,	/* mr_calc_plt_addr */
			ld_perform_outreloc,	/* mr_perform_outreloc */
			ld_do_activerelocs,	/* mr_do_activerelocs */
			ld_add_outrel,		/* mr_add_outrel */
			NULL,			/* mr_reloc_register */
			ld_reloc_local,		/* mr_reloc_local */
			NULL,			/* mr_reloc_GOTOP */
			ld_reloc_TLS,		/* mr_reloc_TLS */
			NULL,			/* mr_assign_got */
			ld_find_gotndx,		/* mr_find_gotndx */
			ld_calc_got_offset,	/* mr_calc_got_offset */
			ld_assign_got_ndx,	/* mr_assign_got_ndx */
			ld_assign_plt_ndx,	/* mr_assign_plt_ndx */
			NULL,			/* mr_allocate_got */
			ld_fillin_gotplt,	/* mr_fillin_gotplt */
		},
		{			/* Target_machsym */
			NULL,			/* ms_reg_check */
			NULL, 			/* ms_mach_sym_typecheck */
			NULL,			/* ms_is_regsym */
			NULL,			/* ms_reg_find */
			NULL			/* ms_reg_enter */
		},
		{			/* Target_unwind */
			NULL,		/* uw_make_unwindhdr */
			NULL,		/* uw_populate_unwindhdr */
			NULL,		/* uw_append_unwind */
		}
	};

	return (&_ld_targ);
}

/*========================= End of Function ========================*/
