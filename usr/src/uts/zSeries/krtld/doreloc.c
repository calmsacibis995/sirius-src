/*------------------------------------------------------------------*/
/* 								    */
/* Name        - doreloc.c  					    */
/* 								    */
/* Function    - Perform basic relocations for kernel modules.      */
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

#ifndef _KERNEL
# define ELF_TARGET_S390
# if defined(DO_RELOC_LIBLD)
#  undef  DO_RELOC_LIBLD
#  define DO_RELOC_LIBLD_S390
# else
#  define bswap 	0
# endif
#else
# define lml	0	/* Needed by arglist of REL_ERR_* macros */
# define bswap  0	/* Needed by setFieldSize etc.	*/
#endif

#ifndef DORELOC_NATIVE
# if !defined(__sparc) && !defined(__s390)
#  define SWAPIT(a,b) 				\
if (swapit) {					\
	int i, j;				\
	char tmp[b];				\
	char *dst = (char *) &a;		\
	j = b - 1;				\
	for (i = 0; i < b; i++, j--)		\
		tmp[i] = dst[j];		\
	memcpy(dst,&tmp,b);			\
}
# else
#  define SWAPIT(a,b)
# endif
#else
# define SWAPIT(a,b)
#endif

/*
 * We need to build this code differently when it is used for
 * cross linking:
 *	- Data alignment requirements can differ from those
 *		of the running system, so we can't access data
 *		in units larger than a byte
 *	- We have to include code to do byte swapping when the
 *		target and linker host use different byte ordering,
 *		but such code is a waste when running natively.
 */
#if !defined(DO_RELOC_LIBLD) || defined(__s390)
# define DORELOC_NATIVE
#endif

#ifdef __GNUC__
# define INLINE __inline__
#else
# define INLINE
#endif

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#if	defined(_KERNEL)
# include	<sys/types.h>
# include	<sys/bootconf.h>
# include	"krtld/reloc.h"
#else
# include	<stdio.h>
# include	"sgs.h"
# include	"machdep.h"
# include	"libld.h"
# include	"reloc.h"
# include	"conv.h"
# include	"msg.h"
#endif

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

typedef struct _gotPltData {
	void	*gotData;
	void	*pltData;
} gotPltData;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern struct bootops *ops;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static INLINE Xword getBaseValue(int, uchar_t *, int);
static INLINE int setFieldSize(int, uchar_t *, Xword, int);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*
 * This table represents the current relocations that do_reloc() is able to
 * process.  The relocations below that are marked SPECIAL are relocations that
 * take special processing and shouldn't actually ever be passed to do_reloc().
 */
const Rel_entry	reloc_table[R_390_NUM] = {
/* R_390_NONE */	{0x0, FLG_RE_NOTREL, 0, 0, 0},
/* R_390_8 */		{0x0, FLG_RE_VERIFY, 1, 0, 0},
/* R_390_12 */		{0xfff, FLG_RE_VERIFY, 2, 0, 0},
/* R_390_16 */		{0x0, FLG_RE_VERIFY, 2, 0, 0},
/* R_390_32 */		{0x0, FLG_RE_VERIFY, 4, 0, 0},
/* R_390_PC32 */	{0x0, FLG_RE_PCREL | FLG_RE_VERIFY | FLG_RE_SIGN |
				FLG_RE_LOCLBND, 4, 0, 32},
/* R_390_GOT12 */	{0xfff, FLG_RE_GOTADD, 2, 0, 12},
/* R_390_GOT32 */	{0x0, FLG_RE_GOTADD | FLG_RE_SIGN, 4, 0, 32},
/* R_390_PLT32 */	{0x0, FLG_RE_PLTREL | FLG_RE_PCREL, 4, 0, 0},
/* R_390_COPY */	{0x0, FLG_RE_NOTREL, 8, 0, 0},		/* SPECIAL */
#ifdef _ELF64
/* R_390_GLOB_DAT */	{0x0, FLG_RE_NOTREL, 8, 0, 0},
#else
/* R_390_GLOB_DAT */	{0x0, FLG_RE_NOTREL, 4, 0, 0},
#endif
/* R_390_JMP_SLOT */	{0x0, FLG_RE_NOTREL, 4, 0, 0},		/* SPECIAL */
#ifdef _ELF64
/* R_390_RELATIVE */	{0x0, FLG_RE_NOTREL, 8, 0, 0},
#else
/* R_390_RELATIVE */	{0x0, FLG_RE_NOTREL, 4, 0, 0},
#endif
/* R_390_GOTOFF32 */	{0x0, FLG_RE_GOTREL | FLG_RE_SIGN, 4, 0, 0},
/* R_390_GOTPC */	{0x0, FLG_RE_PCREL | FLG_RE_SIGN, 4, 0, 0},
/* R_390_GOT16 */	{0x0, FLG_RE_GOTADD | FLG_RE_SIGN, 4, 0, 16},
/* R_390_PC16 */	{0x0, FLG_RE_PCREL | FLG_RE_SIGN | FLG_RE_LOCLBND,
				2, 0, 16},
/* R_390_PC16DBL */	{0x0, FLG_RE_PCREL | FLG_RE_SIGN | FLG_RE_LOCLBND,
				2, 1, 16},
/* R_390_PLT16DBL */	{0x0, FLG_RE_PLTREL | FLG_RE_PCREL, 2, 1, 16},
/* R_390_PC32DBL */	{0x0, FLG_RE_PCREL | FLG_RE_ADDRELATIVE | FLG_RE_UNALIGN,
				4, 1, 32},
/* R_390_PLT32DBL */	{0x0, FLG_RE_PLTREL | FLG_RE_PCREL, 4, 0, 0},
/* R_390_GOTPCDBL */	{0x0, FLG_RE_PCREL | FLG_RE_GOTPC | FLG_RE_LOCLBND,
				4, 1, 16},
/* R_390_64 */		{0x0, FLG_RE_VERIFY, 8, 0, 0},
/* R_390_PC64 */	{0x0, FLG_RE_PCREL | FLG_RE_VERIFY | FLG_RE_SIGN | 
				FLG_RE_LOCLBND, 8, 0, 64},
/* R_390_GOT64 */	{0x0, FLG_RE_GOTADD | FLG_RE_SIGN, 4, 0, 32},
/* R_390_PLT64 */	{0x0, FLG_RE_PLTREL | FLG_RE_VERIFY |
				FLG_RE_ADDRELATIVE, 4, 0, 0},
/* R_390_GOTENT */	{0x0, FLG_RE_GOTPC | FLG_RE_PCREL | FLG_RE_UNALIGN | FLG_RE_SIGN, 4, 1, 32},
/* R_390_GOTOFF16 */	{0x0, FLG_RE_NOTREL | FLG_RE_GOTADD | FLG_RE_SIGN, 2, 0, 0},
/* R_390_GOTOFF64 */	{0x0, FLG_RE_NOTREL | FLG_RE_GOTADD | FLG_RE_SIGN, 8, 0, 0},
/* R_390_GOTPLT12 */	{0xfff, FLG_RE_NOTREL | FLG_RE_GOTADD | FLG_RE_SIGN, 2, 0, 0},
/* R_390_GOTPLT16 */	{0x0, FLG_RE_NOTREL | FLG_RE_GOTADD | FLG_RE_SIGN, 2, 0, 0},
/* R_390_GOTPLT32 */	{0x0, FLG_RE_NOTREL | FLG_RE_GOTADD | FLG_RE_SIGN, 4, 0, 0},
/* R_390_GOTPLT64 */	{0x0, FLG_RE_NOTREL | FLG_RE_GOTADD | FLG_RE_SIGN, 8, 0, 0},
/* R_390_GOTPLTENT */	{0x0, FLG_RE_PCREL | FLG_RE_GOTADD | FLG_RE_SIGN, 8, 0, 0},
/* R_390_PLTOFF16 */	{0x0, FLG_RE_PLTREL | FLG_RE_SIGN, 2, 0, 0},
/* R_390_PLTOFF32 */	{0x0, FLG_RE_PLTREL | FLG_RE_SIGN, 4, 0, 0},
/* R_390_TLS_LOAD */	{0x0, 0, 0, 0, 0},
/* R_390_TLS_GDCALL */	{0x0, FLG_RE_TLSGD, 0, 0, 0},
/* R_390_TLS_LDCALL */	{0x0, 0, 0, 0, 0},
/* R_390_TLS_GD32 */	{0x0, FLG_RE_GOTADD | FLG_RE_TLSGD, 4, 0, 0},
/* R_390_TLS_GD64 */	{0x0, FLG_RE_GOTADD | FLG_RE_TLSGD, 8, 0, 0},
/* R_390_TLS_GOTIE12 */	{0xfff, 0, 2, 0, 0},
/* R_390_TLS_GOTIE32 */	{0x0, 0, 4, 0, 0},
/* R_390_TLS_GOTIE64 */	{0x0, 0, 8, 0, 0},
/* R_390_TLS_LDM32 */	{0x0, FLG_RE_TLSLD, 4, 0, 0},
/* R_390_TLS_LDM64 */	{0x0, FLG_RE_TLSLD, 8, 0, 0},
/* R_390_TLS_IE32 */	{0x0, FLG_RE_GOTADD | FLG_RE_TLSIE, 4, 0, 0},
/* R_390_TLS_IE64 */	{0x0, FLG_RE_GOTADD | FLG_RE_TLSIE, 8, 0, 0},
/* R_390_TLS_IEENT */	{0x0, FLG_RE_PCREL | FLG_RE_GOTADD | FLG_RE_TLSIE, 4, 0, 0},
/* R_390_TLS_LE32 */	{0x0, FLG_RE_GOTADD | FLG_RE_TLSLE, 4, 0, 0},
/* R_390_TLS_LE64 */	{0x0, FLG_RE_GOTADD | FLG_RE_TLSLE, 8, 0, 0},
/* R_390_TLS_LDO32 */	{0x0, FLG_RE_GOTADD | FLG_RE_TLSLD, 4, 0, 0},
/* R_390_TLS_LDO64 */	{0x0, FLG_RE_GOTADD | FLG_RE_TLSLD, 8, 0, 0},
/* R_390_TLS_DTPMOD */	{0x0, 0, 0, 0, 0},
/* R_390_TLS_DTPOFF */	{0x0, 0, 0, 0, 0},
/* R_390_TLS_DTPOFF */	{0x0, 0, 0, 0, 0},
/* R_390_20 */		{0xfffff, FLG_RE_VERIFY | FLG_RE_DISP20 | FLG_RE_SIGN, 4, 0, 20},
/* R_390_GOT20 */	{0xfffff, FLG_RE_GOTADD | FLG_RE_DISP20 | FLG_RE_SIGN, 4, 0, 20},
/* R_390_GOTPLT20 */	{0xfffff, FLG_RE_NOTREL | FLG_RE_GOTADD | FLG_RE_DISP20 | FLG_RE_SIGN, 
				4, 0, 20},
/* R_390_TLS_GOTIE20 */	{0xfffff, FLG_RE_DISP20 | FLG_RE_SIGN, 4, 0, 20},

};


/*
 * Write a single relocated value to its reference location.
 * We assume we wish to add the relocation amount, value, to the
 * value of the address already present in the instruction.
 *
 * Relocation calculations:
 *
 * The FIELD names indicate whether the relocation type checks for overflow.
 * A calculated relocation value may be larger than the intended field, and
 * the relocation type may verify (V) that the value fits, or truncate (T)
 * the result.
 *
 * CALCULATION uses the following notation:
 *      A       the addend used
 *      B       the base address of the shared object in memory
 *      G       the offset into the global offset table
 *      L       the procedure linkage entry
 *      P       the place of the storage unit being relocated
 *      S       the value of the symbol
 *      R       the offset of the symbol within the section in which the symbol is defined (its section-relative address)
 *	O	secondary addend (extra offset) in v9 r_info field
 *
 *	@dtlndx(x): Allocate two contiguous entries in the GOT table to hold
 *	   a Tls_index structure (for passing to __tls_get_addr()). The
 *	   instructions referencing this entry will be bound to the first
 *	   of the two GOT entries.
 *
 *	@tmndx(x): Allocate two contiguous entries in the GOT table to hold
 *	   a Tls_index structure (for passing to __tls_get_addr()). The
 *	   ti_offset field of the Tls_index will be set to 0 (zero) and the
 *	   ti_module will be filled in at run-time. The call to
 *	   __tls_get_addr() will return the starting offset of the dynamic
 *	   TLS block.
 *
 *	@dtpoff(x): calculate the tlsoffset relative to the TLS block.
 *
 *	@tpoff(x): calculate the negative tlsoffset relative to the static
 *	   TLS block. This value can be added to the thread-pointer to
 *	   calculate the tls address.
 *
 *	@dtpmod(x): calculate the module id of the object containing symbol x.
 *
 * The calculations in the CALCULATION column are assumed to have been performed
 * before calling this function except for the addition of the addresses in the
 * instructions.
 *
 * Upon successful completion of do_reloc() *value will be set to the
 * 'bit-shifted' value that will be or'ed into memory.
 *
 * NAME			 VALUE	FIELD		CALCULATION
 * R_390_NONE 		0 	none 		none
 * R_390_8 		1 	byte8 		S + A
 * R_390_12 		2 	low12 		S + A
 * R_390_16 		3 	half16 		S + A
 * R_390_32 		4 	word32 		S + A
 * R_390_PC32		5 	word32 		S + A - P
 * R_390_GOT12 		6 	low12 		O + A
 * R_390_GOT32		7 	word32 		O + A
 * R_390_PLT32 		8 	word32 		L + A
 * R_390_COPY 		9 	none 		(see below)
 * R_390_GLOB_DAT 	10 	quad64 		S + A (see below)
 * R_390_JMP_SLOT 	11 	none 		(see below)
 * R_390_RELATIVE 	12 	quad64 		B + A (see below)
 * R_390_GOTOFF 	13 	quad64 		S + A - G
 * R_390_GOTPC 		14 	quad64 		G + A - P
 * R_390_GOT16 		15 	half16 		O + A
 * R_390_PC16 		16 	half16 		S + A - P
 * R_390_PC16DBL 	17 	pc16 		(S + A - P) >> 1
 * R_390_PLT16DBL 	18 	pc16 		(L + A - P) >> 1
 * R_390_PC32DBL 	19 	pc32 		(S + A - P) >> 1
 * R_390_PLT32DBL 	20 	pc32 		(L + A - P) >> 1
 * R_390_GOTPCDBL 	21 	pc32 		(G + A - P) >> 1
 * R_390_64 		22 	quad64 		S + A
 * R_390_PC64 		23 	quad64 		S + A - P
 * R_390_GOT64 		24 	quad64 		O + A
 * R_390_PLT64 		25 	quad64 		L + A
 * R_390_GOTENT 	26 	pc32 		(G + O + A - P) >> 1
 *
 */

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- do_reloc.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED5 */
int
#ifdef _KERNEL
do_reloc_krtld(uchar_t rtype, uchar_t *off, Xword *value, const char *sym,
         const char *file)
#elif defined(DO_RELOC_LIBLD)
do_reloc_ld(uchar_t rtype, uchar_t *off, Xword *value, const char *sym,
         const char *file, int bswap, void *lml)
#else
do_reloc_rtld(uchar_t rtype, uchar_t *off, Xword *value, const char *sym,
         const char *file, void *lml)
#endif
{
	Xword			uvalue = 0;
	Xword			basevalue, sigbit_mask;
	uchar_t			bshift;
	int			field_size, re_flags, rc;
	const Rel_entry *	rep;
	gotPltData		*gpData = lml;

	rep 	    = &reloc_table[rtype];
	bshift 	    = rep->re_bshift;
	field_size  = rep->re_fsize;
	re_flags    = rep->re_flags;
	sigbit_mask = S_MASK(rep->re_sigbits);

	if ((basevalue = getBaseValue(field_size, off, bswap)) < 0) {
		REL_ERR_UNNOBITS(lml, file, sym, rtype,
		    (rep->re_fsize * 8));
		return (0);
	}

	/*
	 *
	 * NOTES:
	 *
	 * - P is handled in caller (kobj_reloc)
	 *
	 */
	
	switch(rtype) {
	// S+A
	case R_390_8 :
	case R_390_12 :
	case R_390_16 :
	case R_390_32 :
	case R_390_64 :
 
	// O+A
 	case R_390_GOT12 :
	case R_390_GOT16 :
	case R_390_GOT32 :
	case R_390_GOT64 :

	// L+A
	case R_390_PLT32 :
	case R_390_PLT64 :

	// S+A (special)
	case R_390_GLOB_DAT :

	// B+A (special)
	case R_390_RELATIVE :

	// S+A - P 
	// - P is handled in caller (kobj_reloc)
	case R_390_PC16 :
	case R_390_PC32 :
	case R_390_PC64 :

	// G + A - P
	// - P is handled in caller (kobj_reloc)
	case R_390_GOTPC :
		uvalue = basevalue + *value;
		break;

	// S + A - G
	case R_390_GOTOFF16 :
	case R_390_GOTOFF32 :
	case R_390_GOTOFF64 :
		uvalue = basevalue + *value;
		break;

	// (S + A - P) >> 1
	case R_390_PC16DBL :
	case R_390_PC32DBL :
		uvalue = basevalue + *value;
		uvalue = ((Sxword) uvalue >> 1);
		break;

	// (L + A - P) >> 1
	case R_390_PLT16DBL :
	case R_390_PLT32DBL :
		uvalue = basevalue + *value;
		uvalue = ((Sxword) uvalue >> 1);
		break;
	
	case R_390_PLTOFF32 :
	case R_390_PLTOFF16 :
		uvalue = basevalue + *value;
		break;
	
	// (G + A - P) >> 1
	case R_390_GOTPCDBL :

	// (G + O + A - P) >> 1
	case R_390_GOTENT :
		uvalue = basevalue + *value;
		uvalue = ((Sxword) uvalue >> 1);
		break;

	case R_390_COPY:
	case R_390_JMP_SLOT:
	case R_390_NONE:
		// Do nothing?
		break;

	default :
		REL_ERR_UNIMPL(lml, file, sym, rtype);
		break;
	}

	rc = setFieldSize(field_size, off, uvalue, bswap);
	if (rc == 0) {
		REL_ERR_UNSUPSZ(lml, file, sym, rtype, rep->re_fsize);
	}
	
	return (rc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setFieldSize.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static INLINE int
setFieldSize(int size, uchar_t *off, Xword uvalue, int swapit)
{
	switch (size) {
	case 1:
		/* LINTED */
		*((uchar_t *)off) = (uchar_t)uvalue;
		SWAPIT(off, size);
		break;
	case 2:
		/* LINTED */
		*((Half *)off) = (Half)uvalue;
		SWAPIT(off, size);
		break;
	case 4:
		/* LINTED */
		*((Word *)off) = uvalue;
		SWAPIT(off, size);
		break;
	case 8:
		/* LINTED */
		*((Xword *)off) = uvalue;
		SWAPIT(off, size);
		break;
	default:
		return (0);
	}
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getBaseValue.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static INLINE Xword
getBaseValue(int field_size, uchar_t *off, int swapit)
{
	Xword basevalue;

	switch (field_size) {
	case 1:
		basevalue = (Xword)*((uchar_t *)off);
		SWAPIT(basevalue, field_size);
		break;
	case 2:
		/* LINTED */
		basevalue = (Xword)*((Half *)off);
		SWAPIT(basevalue, field_size);
		break;
	case 4:
		/* LINTED */
		basevalue = (Xword)*((Word *)off);
		SWAPIT(basevalue, field_size);
		break;
	case 8:
		/* LINTED */
		basevalue = (Xword)*((Xword *)off);
		SWAPIT(basevalue, field_size);
		break;
	default:
		basevalue = -1;
	}

	return (basevalue);
}

/*========================= End of Function ========================*/
