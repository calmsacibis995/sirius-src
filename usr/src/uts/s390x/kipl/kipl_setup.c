/*------------------------------------------------------------------*/
/* 								    */
/* Name        - kipl_setup.c					    */
/* 								    */
/* Function    - OpenSolaris on System z bootstrapper.              */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - January, 2007					    */
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

/*
 * Macros to add attribute/values to the ELF bootstrap vector
 * and the aux vector.
 */
#define	AUX(p, a, v)	{ (p)->a_type = (a); \
			((p)++)->a_un.a_val = (uint64_t)(v); }

#define	EBV(p, a, v)	{ (p)->eb_tag = (a); \
			((p)++)->eb_un.eb_val = (Elf64_Xword)(v); }

#define	MASK(n)		((1l<<(n))-1l)
#define	IN_RANGE(v, n)	((-(1l<<((n)-1l))) <= (v) && (v) < (1l<<((n)-1l)))

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/asm_linkage.h>
#include <sys/elf.h>
#include <sys/bootconf.h>
#include <sys/link.h>
#include <sys/auxv.h>
#include <sys/kobj.h>
#include <sys/machparam.h>
#include <sys/kobj_impl.h>
#include <sys/archsystm.h>
#include <sys/kdi.h>
#include <sys/promif.h>
#include <sys/machs390x.h>
#include <sys/memlist_plat.h>
#include "kipl_mem.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern	caddr_t	bkern_alloc(caddr_t, size_t, int);
extern	caddr_t	bkern_allreal(size_t, int);
extern	void	bkern_free(caddr_t, size_t);
extern	int	getproplen(const char *);
extern	int	getprop(const char *, void *);
extern	int	setprop(const char *, int, void *);
extern	void	propInit(void);
extern	void 	*init_DAT(memoryChunk *, int);
extern	void 	kobj_init(void *, void *, struct bootops *, val_t *);
extern  int	splsclp(void);

extern 	void   	*bootScratch,		// Scratch area for bop_alloc()
		*bootScratchEnd;	// End of scratch area

//
// Memory lists 
//
extern	struct memlist	*pinstalledp, 
			*pfreelistp, 
			*pavailistp, 
			*vfreelistp, 
			*pbooterp,
			*pramdiskp;

extern void *_start(void);

extern sysib_1_1_1 stsiInfo;

extern const char *version_data;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

void _kipl_setup(void **, long, memoryChunk *, int);
static void _kipl_boot(void **, Boot *);
static void loadUnix(void *, void *);
void kipl_kprintf(struct bootops *, char *, ...);
static void xInfo(int *, char *, char *, int, int);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static Boot	ebp[4],
		*bv;

static auxv_t	vectors[16],
		*av;

//
// Boot operations
//
static struct bootops bops;

struct bootops	*bop;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _kipl_setup.                                      */
/*                                                                  */
/* Function	- Set up the various parameters required by         */
/*		  _kobj_init.                  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
_kipl_setup(void **boot, long avail, memoryChunk *chunk, int nChunk) 
{
	Elf64_Ehdr	*eHdr;
	Phdr		*pHdr;
	void   		*dynamic,
			*rd,
			*erd;
	size_t		lrd;
	_pfxPage	*pfx = NULL;
	int		iPh,
			iParm,
			iMfg;
	char		mfg_id[65];
	char		*iplparms;

	//
	// Initialize PIL and CPU PIL-related fields
	//
	splsclp();

	//
	// Initialize our bops structure
	//
	pfx->__lc_bootops    = &bops;
	bop 		     = &bops;
	bops.bsys_version    = BO_VERSION;
	bops.bsys_alloc	     = bkern_alloc;
	bops.bsys_allreal    = bkern_allreal;
	bops.bsys_free 	     = bkern_free;
	bops.bsys_getproplen = getproplen;
	bops.bsys_getprop    = getprop;
	bops.bsys_setprop    = setprop;
	bops.bsys_vprintf    = prom_vprintf;
	bops.bsys_kprintf    = kipl_kprintf;
	bops.bsys_printf     = prom_printf;
	
	eHdr   = (Elf64_Ehdr *) boot[0];
	pHdr   = ((void *) eHdr) + eHdr->e_phoff;

	for (iPh = 0; iPh < eHdr->e_phnum; iPh++) {
		if (pHdr->p_type == PT_DYNAMIC) {
			dynamic = (void *) pHdr->p_vaddr;
			break;
		}
		pHdr = (Phdr *)((void *) pHdr + eHdr->e_phentsize);
	}

	pHdr   = ((void *) eHdr) + eHdr->e_phoff;

	bv   = &ebp[0];
	EBV(bv, EB_AUXV, &vectors[0]); 
	EBV(bv, EB_PAGESIZE, MMU_PAGESIZE);
	EBV(bv, EB_DYNAMIC, dynamic);
	EBV(bv, EB_NULL, 0);

	av   = &vectors[0];
	AUX(av, AT_BASE, boot[0]);
	AUX(av, AT_ENTRY, _start);
	AUX(av, AT_SUN_LDELF, boot[1]);
	AUX(av, AT_SUN_LDSHDR, ((void *) eHdr + eHdr->e_shoff));
	AUX(av, AT_PHDR, pHdr);
	AUX(av, AT_PHNUM, eHdr->e_phnum);
	AUX(av, AT_PHENT, eHdr->e_phentsize);
	AUX(av, AT_SUN_CPU, "s390x");
	AUX(av, AT_SUN_MMU, "s390x");
	AUX(av, AT_PAGESZ, MMU_PAGESIZE);
	AUX(av, AT_SUN_LDNAME, NULL);
	AUX(av, AT_NULL, 0);

	//
	// Tell user we're on our way up
	//
	prom_printf("Boot commenced for kernel built on %s\n",version_data);

	//
	// Initialize free storage pointers
	//
	setup_memlists();

	//
	// Initialize DAT
	//
	if (bops.bootRSP = init_DAT(chunk, nChunk) == NULL)
		return;

	//
	// Initialize the bootops structure
	//
	bops.bootSysMem   = sysMemory;
	bops.bootAvailMem = availmem;
	bops.bootNChunks  = nMemChunk;
	bops.boot_mem     = kipl_alloc(sizeof (struct bsys_mem));

	bops.boot_mem->scratchLimit  = &bootScratchEnd;

	//
	// Initialize our proposition structures
	//
	propInit();

	//
	// Set the ramdisk addresses
	//
	rd  = boot[2];
	erd = boot[3];

	//
	// Update proposition table with these addresses
	//
	setprop("ramdisk_start", sizeof(rd), &rd);
	setprop("ramdisk_end", sizeof(erd), &erd);

	//
	// Get and set boot arguments and device
	// - SALIPL will point r9 to a 240 byte area for parms
	//   in EBCDIC
	//
	iplparms = (char *) pfx->__lc_parmreg[9];
	e2a(iplparms, 240);
	if (iplparms[0] != 0) {
		for (iParm = 239; iParm > 0 && iplparms[iParm] == ' '; iParm--);
		setprop("bootargs", ++iParm, iplparms);
	} else { 
		setprop("bootargs", 0, iplparms);
	}
	setprop("bios-boot-device", 4, &pfx->__lc_schid);

	// 
	// Extract the mfg-name information from stsiInfo
	//
	iMfg = 0; 
	xInfo(&iMfg, &mfg_id[0], stsiInfo.mfg, 
	      sizeof(stsiInfo.mfg), sizeof(mfg_id));
	xInfo(&iMfg, &mfg_id[0], stsiInfo.type, 
	      sizeof(stsiInfo.type), sizeof(mfg_id));
	xInfo(&iMfg, &mfg_id[0], stsiInfo.model, 
	      sizeof(stsiInfo.model), sizeof(mfg_id));
	e2a(mfg_id, sizeof(mfg_id));
	mfg_id[iMfg] = 0;
	setprop("mfg-id", iMfg, mfg_id);
	setprop("mfg-name", 6, "s390x");
	setprop("impl-arch-name", 6, "s390x");

	// 
	// Go load the krtld/unix blob
	//
	_kipl_boot(boot, &ebp[0]);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _kipl_boot.                                       */
/*                                                                  */
/* Function	- Examine the program headers and relocate the      */
/*		  krtld executable before handing off to _kobj_init */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
_kipl_boot(void **boot, Boot *ebp)
{
	Shdr 	*section[24];		/* cache */
	val_t 	bootaux[BA_NUM];
	Phdr 	*pHdr,
		*bssHdr;
	Ehdr 	*eHdr;
	auxv_t 	*auxv = NULL;
	uint_t 	sh, sh_num, sh_size;
	uintptr_t end, edata, bdata = 0;
	int 	i;
	char 	*secName,
	     	*strTab;
	void 	*secRelo,
		*pgmBase;

	prom_printf("Relocating the KRTLD/UNIX executable\n");

	pgmBase = boot[4];

	for (i = 0; i < BA_NUM; i++) {
		bootaux[i].ba_val = NULL;
		bootaux[i].ba_ptr = NULL;
	}

	/*
	 * Check the bootstrap vector.
	 */
	for (; ebp->eb_tag != EB_NULL; ebp++) {
		switch (ebp->eb_tag) {
		case EB_AUXV:
			auxv = (auxv_t *)ebp->eb_un.eb_ptr;
			break;
		case EB_DYNAMIC:
			bootaux[BA_DYNAMIC].ba_ptr = (void *)ebp->eb_un.eb_ptr;
			break;
		}
	}
	if (auxv == NULL)
		return;
	/*
	 * Now the aux vector.
	 */
	for (; auxv->a_type != AT_NULL; auxv++) {
		switch (auxv->a_type) {
		case AT_PHDR:
			bootaux[BA_PHDR].ba_ptr = auxv->a_un.a_ptr;
			break;
		case AT_PHENT:
			bootaux[BA_PHENT].ba_val = auxv->a_un.a_val;
			break;
		case AT_PHNUM:
			bootaux[BA_PHNUM].ba_val = auxv->a_un.a_val;
		case AT_PAGESZ:
			bootaux[BA_PAGESZ].ba_val = auxv->a_un.a_val;
			break;
		case AT_SUN_LDELF:
			bootaux[BA_LDELF].ba_ptr = auxv->a_un.a_ptr;
			break;
		case AT_SUN_LDSHDR:
			bootaux[BA_LDSHDR].ba_ptr = auxv->a_un.a_ptr;
			break;
		case AT_SUN_LDNAME:
			bootaux[BA_LDNAME].ba_ptr = auxv->a_un.a_ptr;
			break;
		case AT_SUN_LPAGESZ:
			bootaux[BA_LPAGESZ].ba_val = auxv->a_un.a_val;
			break;
		case AT_SUN_IFLUSH:
			bootaux[BA_IFLUSH].ba_val = auxv->a_un.a_val;
			break;
		case AT_SUN_CPU:
			bootaux[BA_CPU].ba_ptr = auxv->a_un.a_ptr;
			break;
		case AT_SUN_MMU:
			bootaux[BA_MMU].ba_ptr = auxv->a_un.a_ptr;
			break;
		case AT_ENTRY:
			bootaux[BA_ENTRY].ba_ptr = auxv->a_un.a_ptr;
			break;
		}
	}

	sh      = (uint_t)(uintptr_t)bootaux[BA_LDSHDR].ba_ptr;
	sh_num  = ((Ehdr *)bootaux[BA_LDELF].ba_ptr)->e_shnum;
	sh_size = ((Ehdr *)bootaux[BA_LDELF].ba_ptr)->e_shentsize;
	eHdr	= (Ehdr *)bootaux[BA_LDELF].ba_ptr;

	/*
	 * Build cache table for section addresses.
	 */
	for (i = 0; i < sh_num; i++) {
		section[i] = (Shdr *)(uintptr_t)sh;
		sh += sh_size;
	}

	/*
	 * Determine end of bss by looking through the headers
	 */
	pHdr = (Phdr *) bootaux[BA_PHDR].ba_ptr;
	for (i = 0; i < bootaux[BA_PHNUM].ba_val; i++) {
		if (pHdr->p_type == PT_LOAD &&
		    (pHdr->p_flags & PF_W) && (pHdr->p_flags & PF_R)) {
			bdata  = pHdr->p_vaddr;
			edata  = end = bdata + pHdr->p_memsz;
			bssHdr = pHdr;
			break;
		}
		pHdr = (Phdr *)((void *) pHdr + bootaux[BA_PHENT].ba_val);
	}

	if (edata == NULL)
		return;

	/*
	 * Done loading the kernel,
	 * so we hand off.
	 */
	kobj_init(NULL, NULL, &bops, &bootaux[0]);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kipl_kprintf.                                     */
/*                                                                  */
/* Function	- Front-end the bops.bsys_printf routine just to    */
/*		  strip the bootops operand from the parameter list.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kipl_kprintf(struct bootops *ops, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	prom_vprintf(fmt, args);
	va_end(args);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- xInfo.                                            */
/*                                                                  */
/* Function	- Extract a field from the SYSIB structure.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
xInfo(int *ix, char *dest, char *field, int szField, int limit)
{
	int iField;

	for (iField = 0; 
	     iField < szField && *ix < limit;
	     iField++, (*ix)++) {
		/*
		 * If this is an empty field just put in an EBCDIC space
		 */
		if (field[iField] == 0)
			dest[*ix] = 0x40;
		else
			dest[*ix] = field[iField];

		/*
		 * When we get an EBCDIC space then we bail out
		 */
		if (field[iField] == 0x40) {
			(*ix)++;
			break;
		}
	}
}

/*========================= End of Function ========================*/
