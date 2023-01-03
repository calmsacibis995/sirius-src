/*------------------------------------------------------------------*/
/* 								    */
/* Name        - s390_elf.c     				    */
/* 								    */
/* Function    - s390 machine dependent and ELF file class 	    */
/*		 dependent functions. Contains routines for 	    */
/*		 performing function binding and symbol relocations.*/
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

#include	"gnu_pragma.h"

#include	<stdio.h>
#include	<sys/elf.h>
#include	<sys/elf_s390.h>
#include	<sys/mman.h>
#include	<dlfcn.h>
#include	<synch.h>
#include	<string.h>
#include	<debug.h>
#include	<reloc.h>
#include	<conv.h>
#include	"_rtld.h"
#include	"_audit.h"
#include	"_elf.h"
#include	"msg.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

typedef struct __plt0 {
	char	st[4];		/* st	%r1,28(%r15)		    */
	char	larl[2];	/* larl	%r1,			    */
	long	got;		/* __GLOBAL_OFFSET_TABLE__	    */
	char	mvc[6];		/* mvc	24(4,%r15),8(%r1)	    */
	char	loff[4];	/* l	%r1,8(%r1)		    */
	char	br[2];		/* br	%r1			    */
	short	nop[5];		/* Padding			    */
} __attribute__ ((packed)) plt0_t;

typedef struct __plt1 {
	char	larl[2];	/* larl	%r1,			    */
	long	gotent;		/* ep@GOTENT              	    */
	char	lgot[4];	/* l	%r1,0(%r1)		    */
	char	br[2];		/* br	%r1			    */
	char	basr[2];	/* basr	%r1,0			    */
	char	loff[4];	/* l	%r1,12(%r1)		    */
	char	jg[2];		/* jg				    */
	long	plt0;		/* offset(plt0)			    */
	long	gotoff;		/* offset(symbol table)		    */
	long	pad;		/* padding       		    */
} __attribute__ ((packed)) plt1_t;

typedef struct __dynplt {
	char	lr[2];		/* lr	%r1,%r15		    */
	char	ahi[4];		/* ahi	%r15,-SA(MINFRAME32+...)    */
	char	stysp[6];	/* sty	%r1,-8(%r15)		    */
	char	larl[2];	/* larl	%r1,			    */
	uchar_t dyndata[4];	/* offset(dyndata)		    */
	char	stydd[6];	/* sty	%r1,-4(%r15)		    */
	char	jg[2];		/* jg				    */
	uchar_t elftrc[4];	/* offset(elftrace)		    */
	char	br[2];		/* br	%r0			    */
} __attribute__ ((packed)) dynplt_t;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern void	elf_rtbndr(Rt_map *, ulong_t, caddr_t);
extern char	pltTemplate[32];
extern char	plt0Template[32];
extern char	dynpltTemplate[32];
extern int	elf_plt_trace();

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/* The GOT holds the address within the PLT to be executed.  
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

static void
PLTTemplate(void)
{
	__asm__ ("	.section \".rodata\"\n"
		 "	.global	 pltTemplate\n"
		 "pltTemplate:\n"
		 "	larl	1,0\n"
		 "	l	1,0(1)\n"
		 "	br	1\n"
		 "	basr	1,0\n"
		 "	l	1,12(1)\n"
		 "	jg	0\n"
		 "	.long	0\n"
		 "	.long	0\n"
		 "	.size	pltTemplate, (.-pltTemplate)\n"
		 "	.section \".text\"\n"
		 : : : "1");
}

/* The first PLT entry is special it is responsible for 
 * putting the offset into the symbol table from R1 onto 
 * the stack at 28(15) and into the loader object information
 * at 24(15). It loads the loader address in R1 and branches to it.  
 */

/* The first entry in the PLT looks like:
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

static void
PLT0Template(void)
{
	__asm__ ("	.section \".rodata\"\n"
		 "	.global	 plt0Template\n"
		 "plt0Template:\n"
		 "	st	1,28(15)\n"
		 "	larl	1,0\n"
		 "	mvc	24(4,15),4(1)\n"
		 "	l	1,8(1)\n"
		 "	br	1\n"
		 "	br	0\n"
		 "	br	0\n"
		 "	br	0\n"
		 "	br	0\n"
		 "	br	0\n"
		 "	.size	plt0Template, (.-plt0Template)\n"
		 "	.section \".text\"\n"
		 : : : "1");
}

static void
dynPltTemplate(void)
{
	__asm__ ("	.section \".rodata\"\n"
		 "	.global	 dynpltTemplate\n"
		 "dynpltTemplate:\n"
		 "	lr	1,15\n"
		 "	ahi	15,-168\n"
		 "	sty	1,-8(15)\n"
		 "	larl	1,0\n" /* 0 is placeholder for dyndata */
		 "	sty	1,-4(15)\n"
		 "	jg	elf_plt_trace\n"
		 "	br	0\n"
		 "	.size	dynpltTemplate, (.-dynpltTemplate)\n"
		 "	.section \".text\"\n"
		 : : : "1");
}

int	dyn_plt_ent_size = sizeof (dynpltTemplate);

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_mach_flags_check.                             */
/*                                                                  */
/* Function	- Check machine type and flags.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
elf_mach_flags_check(Rej_desc *rej, Ehdr *ehdr)
{
	/*
	 * Check machine type and flags.
	 */
	if (ehdr->e_flags != 0) {
		rej->rej_type = SGS_REJ_BADFLAG;
		rej->rej_info = (uint_t)ehdr->e_flags;
		return (0);
	}
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ldso_plt_init.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ldso_plt_init(Rt_map * lmp)
{
	/*
	 * There is no need to analyze ld.so because we don't map in any of
	 * its dependencies.  However we may map these dependencies in later
	 * (as if ld.so had dlopened them), so initialize the plt and the
	 * permission information.
	 */
	if (PLTGOT(lmp))
		elf_plt_init((PLTGOT(lmp)), (caddr_t)lmp);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_plt_trace_write.                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*
 * the dynamic plt entry is:
 *
 *	lr	%r1,%r15
 *	ahi	%r15,-SA(MINFRAME32 + 2*CLONGSIZE) 
 *	sty	%r1,-8(%r15)
 *	larl	%r1,dyndata
 *	sty	%r1,-4(%r15)
 *	jg	elf_plt_trace
 * dyn_data:
 *	.align  4
 *	uintptr_t	reflmp
 *	uintptr_t	deflmp
 *	uint_t		symndx
 *	uint_t		sb_flags
 *	Sym		symdef
 */
static caddr_t
elf_plt_trace_write(uint_t roffset, Rt_map *rlmp, Rt_map *dlmp, Sym *sym,
    uint_t symndx, uint_t pltndx, caddr_t to, uint_t sb_flags, int *fail)
{
	ulong_t		got_entry;
	uchar_t		*dyn_plt;
	uintptr_t	*dyndata;

	/*
	 * We only need to add the glue code if there is an auditing
	 * library that is interested in this binding.
	 */
	dyn_plt = (uchar_t *)((uintptr_t)AUDINFO(rlmp)->ai_dynplts +
	    (pltndx * dyn_plt_ent_size));

	/*
	 * Have we initialized this dynamic plt entry yet?  If we haven't do it
	 * now.  Otherwise this function has been called before, but from a
	 * different plt (ie. from another shared object).  In that case
	 * we just set the plt to point to the new dyn_plt.
	 */
	if (*dyn_plt == 0) {
		Sym	*symp;
		Word	symvalue;
		Lm_list	*lml = LIST(rlmp);
		dynplt_t *dp = (dynplt_t *) dyn_plt;

		(void) memcpy((void *)dyn_plt, dynpltTemplate,
			      sizeof (dynpltTemplate));
		dyndata = (uintptr_t *)((uintptr_t)dyn_plt +
			  ROUND(sizeof (dynpltTemplate), M_WORD_ALIGN));

		/*
		 * relocate:
		 *	pushl	dyn_data
		 */
		symvalue = (Word) dyndata;
		if (do_reloc_rtld(R_390_PC32, (uchar_t *) &dp->dyndata, &symvalue,
		    MSG_ORIG(MSG_SYM_LADYNDATA),
		    MSG_ORIG(MSG_SPECFIL_DYNPLT), lml) == 0) {
			*fail = 1;
			return (0);
		}

		/*
		 * jmps are relative, so I need to figure out the relative
		 * address to elf_plt_trace.
		 *
		 * relocating:
		 *	jg	elf_plt_trace
		 */
		symvalue = (ulong_t)(elf_plt_trace) - (ulong_t)(dp->elftrc);
		if (do_reloc_rtld(R_390_PC32, (uchar_t *) &dp->elftrc, &symvalue,
		    MSG_ORIG(MSG_SYM_ELFPLTTRACE),
		    MSG_ORIG(MSG_SPECFIL_DYNPLT), lml) == 0) {
			*fail = 1;
			return (0);
		}

		*dyndata++ = (uintptr_t)rlmp;
		*dyndata++ = (uintptr_t)dlmp;
		*dyndata++ = (uint_t)symndx;
		*dyndata++ = (uint_t)sb_flags;
		symp = (Sym *)dyndata;
		*symp = *sym;
		symp->st_name += (Word)STRTAB(dlmp);
		symp->st_value = (Addr)to;
	}

	got_entry = (ulong_t)roffset;
	*(ulong_t *)got_entry = (ulong_t)dyn_plt;
	return ((caddr_t)dyn_plt);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_bndr.                                         */
/*                                                                  */
/* Function	- Function binding routine - invoked on the first   */
/*		  call to a function through the procedure linkage  */
/*		  table; passes first through an assembly language  */
/*		  interface.					    */
/*								    */
/*		  Takes the offset into the relocation table of the */
/*		  associated relocation entry and the address of    */
/*		  the link map (rt_private_map struct) for the 	    */
/*		  entry.					    */
/*								    */
/*		  Returns the address of the function referenced    */
/*		  after re-writing the PLT entry to invoke the 	    */
/*		  function directly.				    */
/*								    */
/*		  On error, causes process to terminate with a 	    */
/*		  signal.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

ulong_t
elf_bndr(Rt_map *lmp, ulong_t reloff, caddr_t from)
{
	Rt_map		*nlmp, *llmp;
	ulong_t		addr, symval, rsymndx;
	char		*name;
	Rela		*rptr;
	Sym		*rsym, *nsym;
	uint_t		binfo, sb_flags = 0, dbg_class;
	Slookup		sl;
	int		entry, lmflags;
	Lm_list		*lml;

	/*
	 * For compatibility with libthread (TI_VERSION 1) we track the entry
	 * value.  A zero value indicates we have recursed into ld.so.1 to
	 * further process a locking request.  Under this recursion we disable
	 * tsort and cleanup activities.
	 */
	entry = enter(0);

	lml = LIST(lmp);
	if ((lmflags = lml->lm_flags) & LML_FLG_RTLDLM) {
		dbg_class = dbg_desc->d_class;
		dbg_desc->d_class = 0;
	}

	/*
	 * Perform some basic sanity checks.  If we didn't get a load map or
	 * the relocation offset is invalid then its possible someone has walked
	 * over the .got entries or jumped to plt0 out of the blue.
	 */
	if (!lmp) {
		Conv_inv_buf_t inv_buf;

		eprintf(lml, ERR_FATAL, MSG_INTL(MSG_REL_PLTREF),
		    conv_reloc_s390_type(R_390_JMP_SLOT, 0, &inv_buf),
		    EC_NATPTR(lmp), EC_XWORD(reloff), EC_NATPTR(from));
		rtldexit(lml, 1);
	}

	/*
	 * Use relocation entry to get symbol table entry and symbol name.
	 */
	addr 	= (ulong_t)JMPREL(lmp);
	rptr 	= (Rela *)(addr + reloff);
	rsymndx = ELF_R_SYM(rptr->r_info);
	rsym 	= (Sym *)((ulong_t)SYMTAB(lmp) + (rsymndx * SYMENT(lmp)));
	name 	= (char *)(STRTAB(lmp) + rsym->st_name);

	/*
	 * Determine the last link-map of this list, this'll be the starting
	 * point for any tsort() processing.
	 */
	llmp = lml->lm_tail;

	/*
	 * Find definition for symbol.  Initialize the symbol lookup data
	 * structure.
	 */
	SLOOKUP_INIT(sl, name, lmp, lml->lm_head, ld_entry_cnt, 0,
	    rsymndx, rsym, 0, LKUP_DEFT);

	if ((nsym = lookup_sym(&sl, &nlmp, &binfo, NULL)) == 0) {
		eprintf(lml, ERR_FATAL, MSG_INTL(MSG_REL_NOSYM), NAME(lmp),
		    demangle(name));
		rtldexit(lml, 1);
	}

	symval = nsym->st_value;
	if (!(FLAGS(nlmp) & FLG_RT_FIXED) &&
	    (nsym->st_shndx != SHN_ABS)) {
		symval += ADDR(nlmp);
	}
	if ((lmp != nlmp) && ((FLAGS1(nlmp) & FL1_RT_NOINIFIN) == 0)) {
		/*
		 * Record that this new link map is now bound to the caller.
		 */
		if (bind_one(lmp, nlmp, BND_REFER) == 0)
			rtldexit(lml, 1);
	}

	if ((lml->lm_tflags | FLAGS1(lmp)) & LML_TFLG_AUD_SYMBIND) {
		uint_t	symndx = (((uintptr_t)nsym -
		    (uintptr_t)SYMTAB(nlmp)) / SYMENT(nlmp));
		symval = audit_symbind(lmp, nlmp, nsym, symndx, symval,
		    &sb_flags);
	}

	if (!(rtld_flags & RT_FL_NOBIND)) {
		addr = rptr->r_offset;
		if (!(FLAGS(lmp) & FLG_RT_FIXED))
			addr += ADDR(lmp);
		if (((lml->lm_tflags | FLAGS1(lmp)) &
		    (LML_TFLG_AUD_PLTENTER | LML_TFLG_AUD_PLTEXIT)) &&
		    AUDINFO(lmp)->ai_dynplts) {
			int	fail = 0;
			uint_t	pltndx = reloff / sizeof(Rela);
			uint_t	symndx = (((uintptr_t)nsym -
			    (uintptr_t)SYMTAB(nlmp)) / SYMENT(nlmp));

			symval = (ulong_t)elf_plt_trace_write(addr, lmp, nlmp,
			    nsym, symndx, pltndx, (caddr_t)symval, sb_flags,
			    &fail);
			if (fail)
				rtldexit(lml, 1);
		} else {
			/*
			 * Write standard PLT entry to jump directly
			 * to newly bound function.
			 */
			*(ulong_t *)addr = symval;
		}
	}

	/*
	 * Print binding information and rebuild PLT entry.
	 */
	DBG_CALL(Dbg_bind_global(lmp, (Addr)from, (Off)(from - ADDR(lmp)),
	    (Xword)(reloff / sizeof(Rela)), PLT_T_FULL, nlmp, (Addr)symval,
	    nsym->st_value, name, binfo));

	/*
	 * Complete any processing for newly loaded objects.  Note we don't
	 * know exactly where any new objects are loaded (we know the object
	 * that supplied the symbol, but others may have been loaded lazily as
	 * we searched for the symbol), so sorting starts from the last
	 * link-map know on entry to this routine.
	 */
	if (entry)
		load_completion(llmp);

	/*
	 * Some operations like dldump() or dlopen()'ing a relocatable object
	 * result in objects being loaded on rtld's link-map, make sure these
	 * objects are initialized also.
	 */
	if ((LIST(nlmp)->lm_flags & LML_FLG_RTLDLM) && LIST(nlmp)->lm_init)
		load_completion(nlmp);

	/*
	 * If the object we've bound to is in the process of being initialized
	 * by another thread, determine whether we should block.
	 */
	is_dep_ready(nlmp, lmp, DBG_WAIT_SYMBOL);

	/*
	 * Make sure the object to which we've bound has had it's .init fired.
	 * Cleanup before return to user code.
	 */
	if (entry) {
		is_dep_init(nlmp, lmp);
		leave(lml, 0);
	}

	if (lmflags & LML_FLG_RTLDLM)
		dbg_desc->d_class = dbg_class;
 
	return (symval);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_reloc_relative.                               */
/*                                                                  */
/* Function	- When the relocation loop realizes that it's 	    */
/*		  dealing with relative relocations in a shared     */
/*		  object, it breaks into this tighter loop as an    */
/*		  optimization.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

ulong_t
elf_reloc_relative(ulong_t relbgn, ulong_t relend, ulong_t relsiz,
    ulong_t basebgn, ulong_t etext, ulong_t emap)
{
	ulong_t roffset = ((Rela *)relbgn)->r_offset;
	char rtype;

	do {
		roffset += basebgn;

		/*
		 * If this relocation is against an address not mapped in,
		 * then break out of the relative relocation loop, falling
		 * back on the main relocation loop.
		 */
		if (roffset < etext || roffset > emap)
			break;

		/*
		 * Perform the actual relocation.
		 */
		*((ulong_t *)roffset) = basebgn +
		    ((Rela *)relbgn)->r_addend;

		relbgn += relsiz;

		if (relbgn >= relend)
			break;

		rtype = ELF_R_TYPE(((Rela *)relbgn)->r_info, M_MACH);
		roffset = ((Rela *)relbgn)->r_offset;

	} while (rtype == R_390_RELATIVE);

	return (relbgn);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_reloc_relacount.                              */
/*                                                                  */
/* Function	- This is the tightest loop for RELATIVE reloc-     */
/*		  ations for those objects built with the 	    */
/*		  DT_RELACOUNT .dynamic entry. 		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

ulong_t
elf_reloc_relacount(ulong_t relbgn, ulong_t relacount, ulong_t relsiz,
    ulong_t basebgn)
{
	ulong_t roffset = ((Rela *) relbgn)->r_offset;

	for (; relacount; relacount--) {
		roffset += basebgn;

		/*
		 * Perform the actual relocation.
		 */
		*((ulong_t *)roffset) += basebgn;

		relbgn += relsiz;

		roffset = ((Rela *)relbgn)->r_offset;

	}

	return (relbgn);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_reloc.                                        */
/*                                                                  */
/* Function	- Read and process the relocations for one link     */
/*		  object, we assume all relocation sections for     */
/*		  loadable segments are stored contiguously in	    */
/*		  the file.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
elf_reloc(Rt_map *lmp, uint_t plt, int *in_nfavl)
{
	ulong_t		relbgn, relend, relsiz, basebgn;
	ulong_t		pltbgn, pltend, _pltbgn, _pltend;
	ulong_t		roffset, rsymndx, psymndx = 0, etext  = ETEXT(lmp);
	ulong_t		emap, dsymndx;
	uchar_t		rtype;
	long		reladd, value, pvalue;
	Sym		*symref, *psymref, *symdef, *psymdef;
	char		*name, *pname;
	Rt_map		*_lmp, *plmp;
	int		textrel = 0, ret = 1, noplt = 0;
	int		relacount = RELACOUNT(lmp), plthint = 0;
	Rela		*rel;
	uint_t		binfo, pbinfo;
	APlist		*bound = NULL;

	/*
	 * Although only necessary for lazy binding, initialize the first
	 * global offset entry to go to elf_rtbndr().  dbx(1) seems
	 * to find this useful.
	 */
	if ((plt == 0) && PLTGOT(lmp)) {
		if ((ulong_t)PLTGOT(lmp) < etext) {
			if (elf_set_prot(lmp, PROT_WRITE) == 0)
				return (0);
			textrel = 1;
		}
		elf_plt_init(PLTGOT(lmp), (caddr_t)lmp);
	}

	/*
	 * Initialize the plt start and end addresses.
	 */
	if ((pltbgn = (ulong_t)JMPREL(lmp)) != 0)
		pltend = pltbgn + (ulong_t)(PLTRELSZ(lmp));


	relsiz = (ulong_t)(RELENT(lmp));
	basebgn = ADDR(lmp);
	emap = ADDR(lmp) + MSIZE(lmp);
#if 0
	fprintf(stderr,"elf_reloc (s390): init %s=%lx [%lx]\n", NAME(lmp), basebgn, emap);
#endif

	if (PLTRELSZ(lmp))
		plthint = PLTRELSZ(lmp) / relsiz;

	/*
	 * If we've been called upon to promote an RTLD_LAZY object to an
	 * RTLD_NOW then we're only interested in scaning the .plt table.
	 * An uninitialized .plt is the case where the associated got entry
	 * points back to the plt itself.  Determine the range of the real .plt
	 * entries using the _PROCEDURE_LINKAGE_TABLE_ symbol.
	 */
	if (plt) {
		Slookup	sl;

		relbgn = pltbgn;
		relend = pltend;
		if (!relbgn || (relbgn == relend))
			return (1);

		sl.sl_name = MSG_ORIG(MSG_SYM_PLT);
		sl.sl_cmap = lmp;
		sl.sl_imap = lmp;
		sl.sl_hash = elf_hash(MSG_ORIG(MSG_SYM_PLT));
		sl.sl_rsymndx = 0;
		sl.sl_flags = LKUP_DEFT;

		if ((symdef = elf_find_sym(&sl, &_lmp, &binfo, in_nfavl)) == 0)
			return (1);

		_pltbgn = symdef->st_value;
		if (!(FLAGS(lmp) & FLG_RT_FIXED) &&
		    (symdef->st_shndx != SHN_ABS))
			_pltbgn += basebgn;
		_pltend = _pltbgn + (((PLTRELSZ(lmp) / relsiz)) *
		    M_PLT_ENTSIZE) + M_PLT_RESERVSZ;

	} else {
		/*
		 * The relocation sections appear to the run-time linker as a
		 * single table.  Determine the address of the beginning and end
		 * of this table.  There are two different interpretations of
		 * the ABI at this point:
		 *
		 *   o	The REL table and its associated RELSZ indicate the
		 *	concatenation of *all* relocation sections (this is the
		 *	model our link-editor constructs).
		 *
		 *   o	The REL table and its associated RELSZ indicate the
		 *	concatenation of all *but* the .plt relocations.  These
		 *	relocations are specified individually by the JMPREL and
		 *	PLTRELSZ entries.
		 *
		 * Determine from our knowledege of the relocation range and
		 * .plt range, the range of the total relocation table.  Note
		 * that one other ABI assumption seems to be that the .plt
		 * relocations always follow any other relocations, the
		 * following range checking drops that assumption.
		 */
		relbgn = (ulong_t)(REL(lmp));
		relend = relbgn + (ulong_t)(RELSZ(lmp));
		if (pltbgn) {
			if (!relbgn || (relbgn > pltbgn))
				relbgn = pltbgn;
			if (!relbgn || (relend < pltend))
				relend = pltend;
		}
	}
	if (!relbgn || (relbgn == relend)) {
		DBG_CALL(Dbg_reloc_run(lmp, 0, plt, DBG_REL_NONE));
		return (1);
	}
	DBG_CALL(Dbg_reloc_run(lmp, M_REL_SHT_TYPE, plt, DBG_REL_START));

	/*
	 * If we're processing a dynamic executable in lazy mode there is no
	 * need to scan the .rel.plt table, however if we're processing a shared
	 * object in lazy mode the .got addresses associated to each .plt must
	 * be relocated to reflect the location of the shared object.
	 */
	if (pltbgn && ((MODE(lmp) & RTLD_NOW) == 0) &&
	    (FLAGS(lmp) & FLG_RT_FIXED))
		noplt = 1;

	/*
	 * Loop through relocations.
	 */
	while (relbgn < relend) {
		uint_t	sb_flags = 0;

		rtype = ELF_R_TYPE(((Rela *)relbgn)->r_info, M_MACH);

		/*
		 * If this is a RELATIVE relocation in a shared object (the
		 * common case), and if we are not debugging, then jump into a
		 * tighter relocation loop (elf_reloc_relative).  Only make the
		 * jump if we've been given a hint on the number of relocations.
		 */
		if ((rtype == R_390_RELATIVE) &&
		    ((FLAGS(lmp) & FLG_RT_FIXED) == 0) && (DBG_ENABLED == 0)) {
			/*
			 * It's possible that the relative relocation block
			 * has relocations against the text segment as well
			 * as the data segment.  Since our optimized relocation
			 * engine does not check which segment the relocation
			 * is against - just mprotect it now if it's been
			 * marked as containing TEXTREL's.
			 */
			if ((textrel == 0) && (FLAGS1(lmp) & FL1_RT_TEXTREL)) {
				if (elf_set_prot(lmp, PROT_WRITE) == 0) {
					ret = 0;
					break;
				}
				textrel = 1;
			}

			if (relacount) {
				relbgn = elf_reloc_relacount(relbgn, relacount,
				    relsiz, basebgn);
				relacount = 0;
			} else {
				relbgn = elf_reloc_relative(relbgn, relend,
				    relsiz, basebgn, etext, emap);
			}
			if (relbgn >= relend)
				break;
			rtype = ELF_R_TYPE(((Rela *)relbgn)->r_info, M_MACH);
		}

		roffset = ((Rela *)relbgn)->r_offset;

		/*
		 * If this is a shared object, add the base address to offset.
		 */
		if (!(FLAGS(lmp) & FLG_RT_FIXED)) {

			/*
			 * If we're processing lazy bindings, we have to step
			 * through the plt entries and add the base address
			 * to the corresponding got entry.
			 */
			if (plthint && (plt == 0) &&
			    (rtype == R_390_JMP_SLOT) &&
			    ((MODE(lmp) & RTLD_NOW) == 0)) {
				relbgn = elf_reloc_relacount(relbgn,
				    plthint, relsiz, basebgn);
				plthint = 0;
				continue;
			}
			roffset += basebgn;
		}

		reladd = (((Rela *)relbgn)->r_addend);
		rsymndx = ELF_R_SYM(((Rela *)relbgn)->r_info);
		rel = (Rela *)relbgn;
		relbgn += relsiz;

		/*
		 * Optimizations.
		 */
		if (rtype == R_390_NONE)
			continue;
		if (noplt && ((ulong_t)rel >= pltbgn) &&
		    ((ulong_t)rel < pltend)) {
			relbgn = pltend;
			continue;
		}

		/*
		 * If we're promoting plts determine if this one has already
		 * been written.
		 */
		if (plt) {
			if ((*(ulong_t *)roffset < _pltbgn) ||
			    (*(ulong_t *)roffset > _pltend))
				continue;
		}

		/*
		 * If this relocation is not against part of the image
		 * mapped into memory we skip it.
		 */
		if ((roffset < ADDR(lmp)) || (roffset > (ADDR(lmp) +
		    MSIZE(lmp)))) {
			elf_reloc_bad(lmp, (void *)rel, rtype, roffset,
			    rsymndx);
			continue;
		}

		binfo = 0;
		/*
		 * If a symbol index is specified then get the symbol table
		 * entry, locate the symbol definition, and determine its
		 * address.
		 */
		if (rsymndx) {
			/*
			 * Get the local symbol table entry.
			 */
			symref = (Sym *)((ulong_t)SYMTAB(lmp) +
			    (rsymndx * SYMENT(lmp)));

			/*
			 * If this is a local symbol, just use the base address.
			 * (we should have no local relocations in the
			 * executable).
			 */
			if (ELF_ST_BIND(symref->st_info) == STB_LOCAL) {
				value = basebgn;
				name = (char *)0;

				/*
				 * Special case TLS relocations.
				 */
				if (rtype == R_390_TLS_DTPMOD) {
					/*
					 * Use the TLS modid.
					 */
					value = TLSMODID(lmp);

				} else if (rtype == R_390_TLS_TPOFF) {
					if ((value = elf_static_tls(lmp, symref,
					    rel, rtype, 0, roffset, 0)) == 0) {
						ret = 0;
						break;
					}
				}
			} else {
				/*
				 * If the symbol index is equal to the previous
				 * symbol index relocation we processed then
				 * reuse the previous values. (Note that there
				 * have been cases where a relocation exists
				 * against a copy relocation symbol, our ld(1)
				 * should optimize this away, but make sure we
				 * don't use the same symbol information should
				 * this case exist).
				 */
				if ((rsymndx == psymndx) &&
				    (rtype != R_390_COPY)) {
					/* LINTED */
					if (psymdef == 0) {
						DBG_CALL(Dbg_bind_weak(lmp,
						    (Addr)roffset, (Addr)
						    (roffset - basebgn), name));
						continue;
					}
					/* LINTED */
					value = pvalue;
					/* LINTED */
					name = pname;
					/* LINTED */
					symdef = psymdef;
					/* LINTED */
					symref = psymref;
					/* LINTED */
					_lmp = plmp;
					/* LINTED */
					binfo = pbinfo;

					if ((LIST(_lmp)->lm_tflags |
					    FLAGS1(_lmp)) &
					    LML_TFLG_AUD_SYMBIND) {
						value = audit_symbind(lmp, _lmp,
						    /* LINTED */
						    symdef, dsymndx, value,
						    &sb_flags);
					}
				} else {
					Slookup		sl;
					uchar_t		bind;

					/*
					 * Lookup the symbol definition.
					 */
					name = (char *)(STRTAB(lmp) +
					    symref->st_name);

					SLOOKUP_INIT(sl, name, lmp, 0, 
						ld_entry_cnt, 0, rsymndx, symref,
						rtype, LKUP_STDRELOC);

					symdef = lookup_sym(&sl, &_lmp, &binfo, in_nfavl);

					/*
					 * If the symbol is not found and the
					 * reference was not to a weak symbol,
					 * report an error.  Weak references
					 * may be unresolved.
					 * chkmsg: MSG_INTL(MSG_LDD_SYM_NFOUND)
					 */
					/* BEGIN CSTYLED */
					if (symdef == 0) {
						if (sl.sl_bind != STB_WEAK) {
							if (elf_reloc_error(lmp, name, 
									    rel, binfo))
								continue;
						    	ret = 0;
						    	break;
						} else {
							psymndx = rsymndx;
						  	psymdef = 0;

							DBG_CALL(Dbg_bind_weak(lmp,
							    (Addr)roffset, (Addr)
							    (roffset - basebgn), name));
							continue;
						}
					}
					/* END CSTYLED */

					/*
					 * If symbol was found in an object
					 * other than the referencing object
					 * then record the binding.
					 */
					if ((lmp != _lmp) && ((FLAGS1(_lmp) &
					    FL1_RT_NOINIFIN) == 0)) {
						if (aplist_test(&bound, _lmp,
						    AL_CNT_RELBIND) == 0) {
							ret = 0;
							break;
						}
					}

					/*
					 * Calculate the location of definition;
					 * symbol value plus base address of
					 * containing shared object.
					 */
					if (IS_SIZE(rtype))
						value = symdef->st_size;
					else
						value = symdef->st_value;

					if (!(FLAGS(_lmp) & FLG_RT_FIXED) &&
					    !(IS_SIZE(rtype)) &&
					    (symdef->st_shndx != SHN_ABS) &&
					    (ELF_ST_TYPE(symdef->st_info) !=
					    STT_TLS))
						value += ADDR(_lmp);

					/*
					 * Retain this symbol index and the
					 * value in case it can be used for the
					 * subsequent relocations.
					 */
					if (rtype != R_390_COPY) {
						psymndx = rsymndx;
						pvalue = value;
						pname = name;
						psymdef = symdef;
						psymref = symref;
						plmp = _lmp;
						pbinfo = binfo;
					}
					if ((LIST(_lmp)->lm_tflags |
					    FLAGS1(_lmp)) &
					    LML_TFLG_AUD_SYMBIND) {
						dsymndx = (((uintptr_t)symdef -
						    (uintptr_t)SYMTAB(_lmp)) /
						    SYMENT(_lmp));
						value = audit_symbind(lmp, _lmp,
						    symdef, dsymndx, value,
						    &sb_flags);
					}
				}

				/*
				 * If relocation is PC-relative, subtract
				 * offset address.
				 */
				if (IS_PC_RELATIVE(rtype))
					value -= roffset;

				/*
				 * Special case TLS relocations.
				 */
				if (rtype == R_390_TLS_DTPMOD) {
					/*
					 * Relocation value is the TLS modid.
					 */
					value = TLSMODID(_lmp);

				} else if (rtype == R_390_TLS_TPOFF) {
					if ((value = elf_static_tls(_lmp,
					    symdef, rel, rtype, name, roffset,
					    value)) == 0) {
						ret = 0;
						break;
					}
				}
			}
		} else {
			/*
			 * Special cases.
			 */
			if (rtype == R_390_TLS_DTPMOD) {
				/*
				 * TLS relocation value is the TLS modid.
				 */
				value = TLSMODID(lmp);
			} else
				value = basebgn;
			name = (char *)0;
		}

		DBG_CALL(Dbg_reloc_in(LIST(lmp), ELF_DBG_RTLD, M_MACH,
		    M_REL_SHT_TYPE, rel, NULL, name));

		/*
		 * If this object has relocations in the text segment, turn
		 * off the write protect.
		 */
		if ((roffset < etext) && (textrel == 0)) {
			if (elf_set_prot(lmp, PROT_WRITE) == 0) {
				ret = 0;
				break;
			}
			textrel = 1;
		}

		/*
		 * Call relocation routine to perform required relocation.
		 */
		switch (rtype) {
		case R_390_COPY:
			if (elf_copy_reloc(name, symref, lmp, (void *)roffset,
			    symdef, _lmp, (const void *)value) == 0)
				ret = 0;
			break;
		case R_390_JMP_SLOT:
			if (((LIST(lmp)->lm_tflags | FLAGS1(lmp)) &
			    (LML_TFLG_AUD_PLTENTER | LML_TFLG_AUD_PLTEXIT)) &&
			    AUDINFO(lmp)->ai_dynplts) {
				int	fail = 0;
				int	pltndx = (((ulong_t)rel -
				    (uintptr_t)JMPREL(lmp)) / relsiz);
				int	symndx = (((uintptr_t)symdef -
				    (uintptr_t)SYMTAB(_lmp)) / SYMENT(_lmp));

				(void) elf_plt_trace_write(roffset, lmp, _lmp,
				    symdef, symndx, pltndx, (caddr_t)value,
				    sb_flags, &fail);
				if (fail)
					ret = 0;
			} else {
				/*
				 * Write standard PLT entry to jump directly
				 * to newly bound function.
				 */
				DBG_CALL(Dbg_reloc_apply_val(LIST(lmp),
				    ELF_DBG_RTLD, (Xword)roffset,
				    (Xword)value));
				*(ulong_t *)roffset = value;
			}
			break;
		default:
			/* Offset by addendum */
			value += reladd;
			/*
			 * Write the relocation out.
			 */
			if (do_reloc_rtld(rtype, (uchar_t *)roffset, (Word *)&value,
			    name, NAME(lmp), LIST(lmp)) == 0)
				ret = 0;

			DBG_CALL(Dbg_reloc_apply_val(LIST(lmp), ELF_DBG_RTLD,
			    (Xword)roffset, (Xword)value));
		}

		if ((ret == 0) &&
		    ((LIST(lmp)->lm_flags & LML_FLG_TRC_WARN) == 0))
			break;

		if (binfo) {
#if 0
			printf("elf_reloc (s390): %s=%x\n",name,value);
#endif
			DBG_CALL(Dbg_bind_global(lmp, (Addr)roffset,
			    (Off)(roffset - basebgn), (Xword)(-1), PLT_T_FULL,
			    _lmp, (Addr)value, symdef->st_value, name, binfo));
		}
	}

	return (relocate_finish(lmp, bound, textrel, ret));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_plt_init.                                     */
/*                                                                  */
/* Function	- Initialize the first few got entries so that 	    */
/*		  function calls go to elf_rtbndr:		    */
/*								    */
/*		  GOT[GOT_XLINKMAP] =	the address of the link map */
/*		  GOT[GOT_XRTLD] =	the address of rtbinder	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
elf_plt_init(void *got, caddr_t l)
{
	uint32_t	*_got;
	/* LINTED */
	Rt_map		*lmp = (Rt_map *)l;

	_got  = (uint32_t *) got + M_GOT_XLINKMAP;
	*_got = (uint32_t) lmp;
	_got  = (uint32_t *) got + M_GOT_XRTLD;
	*_got = (uint32_t) elf_rtbndr;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_plt_write.                                    */
/*                                                                  */
/* Function	- Plt writing interface to allow debugging initial- */
/*		  ization to be generic.			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

Pltbindtype
/* ARGSUSED1 */
elf_plt_write(uintptr_t addr, uintptr_t vaddr, void *rptr, uintptr_t symval,
	Xword pltndx)
{
	Rela		*rel = (Rela *)rptr;
	uintptr_t	pltaddr;

	pltaddr = addr + rel->r_offset;
	*(ulong_t *)pltaddr = (ulong_t)symval;
	DBG_CALL(pltcntfull++);
	return (PLT_T_FULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _conv_reloc_type.                                 */
/*                                                                  */
/* Function	- Provide a machine specific interface to the conv- */
/*		  ersion routine.  By calling the machine specific  */
/*		  version, rather than the generic version, we 	    */
/*		  insure that the data tables/strings for all known */
/*		  machine versions aren't dragged into ld.so.1.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

const char *
_conv_reloc_type(uint_t rel)
{
	static Conv_inv_buf_t inv_buf;

	return (conv_reloc_s390_type(rel, 0, &inv_buf));
}

/*========================= End of Function ========================*/
