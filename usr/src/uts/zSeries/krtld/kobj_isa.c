/*------------------------------------------------------------------*/
/* 								    */
/* Name        - kobj_isa.c 					    */
/* 								    */
/* Function    - Various kobj support routines for z/Architecture.  */
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


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

/*
 * Miscellaneous ISA-specific code.
 */
#include <sys/types.h>
#include <sys/elf.h>
#include <sys/kobj.h>
#include <sys/kobj_impl.h>
#include <sys/archsystm.h>
#include <sys/bootconf.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern	int	use_iflush;
extern  struct  bootops *ops;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- elf_mach_ok.                                      */
/*                                                                  */
/* Function	- Check that an ELF header corresponds to this      */
/*		  machine's instruction set architecture. Used by   */
/*		  kobj_load_module() to not get confused by mis-    */
/*		  placed driver or kernel module built for a diff-  */
/*		  erent ISA.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
elf_mach_ok(Ehdr *h)
{
	return (h->e_machine == EM_S390);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kobj_addrcheck.                                   */
/*                                                                  */
/* Function	- Return non-zero for a bad address.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
kobj_addrcheck(void *xmp, caddr_t adr)
{
	struct module *mp;

	mp = (struct module *)xmp;

	if ((adr >= mp->text && adr < mp->text + mp->text_size) ||
	    (adr >= mp->data && adr < mp->data + mp->data_size))
		return (0); /* ok */
	if (mp->bss && adr >= (caddr_t)mp->bss &&
	    adr < (caddr_t)mp->bss + mp->bss_size)
		return (0);
	return (1);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kobj_sync_instruction_memory.                     */
/*                                                                  */
/* Function	- Flush instruction cache after updating text.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kobj_sync_instruction_memory(caddr_t addr, size_t len)
{
	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- get_progbits_size.                                */
/*                                                                  */
/* Function	- Calculate memory image required for relocatable   */
/*		  object.                      		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED3 */
int
get_progbits_size(struct module *mp, struct proginfo *tp, struct proginfo *dp,
	struct proginfo *sdp)
{
	struct proginfo *pp;
	uint_t shn;
	Shdr *shp;

	/*
	 * loop through sections to find out how much space we need
	 * for text, data, (also bss that is already assigned)
	 */
	for (shn = 1; shn < mp->hdr.e_shnum; shn++) {
		shp = (Shdr *)(mp->shdrs + shn * mp->hdr.e_shentsize);
		if (!(shp->sh_flags & SHF_ALLOC))
			continue;
		if (shp->sh_addr != 0) {
			_kobj_printf(ops,
			    "%s non-zero sect addr in input file\n",
			    mp->filename);
			return (-1);
		}
		pp = (shp->sh_flags & SHF_WRITE)? dp : tp;

		if (shp->sh_addralign > pp->align)
			pp->align = shp->sh_addralign;
		pp->size = ALIGN(pp->size, shp->sh_addralign);
		pp->size += ALIGN(shp->sh_size, 8);
	}
	tp->size = ALIGN(tp->size, 8);
	dp->size = ALIGN(dp->size, 8);
	return (0);
}

/*========================= End of Function ========================*/
